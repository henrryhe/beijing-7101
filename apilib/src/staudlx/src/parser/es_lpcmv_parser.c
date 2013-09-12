/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_LPCMV_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_LPCMV

#include "es_lpcmv_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

#define LPCM_VIDEO_BASE_FRAME_SIZE 640 //80

static U32 LPCMFrequency[] =
{
    48000,  96000,  0,  0
};


/************************************************************************************
Name         : ES_LPCMV_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_LPCMV_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMV_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_LPCMV_ParserLocalParams_t * LPCMVParserLocalParams_p;
    STAud_LpcmStreamParams_t * AppData_p[2];

    LPCMVParserLocalParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_LPCMV_ParserLocalParams_t));
    if(LPCMVParserLocalParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(LPCMVParserLocalParams_p,0,sizeof(ES_LPCMV_ParserLocalParams_t));

    /* Init the Local Parameters */
    AppData_p[0] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
    AppData_p[1] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));

    LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_COMPUTE_FRAME_LENGTH;


    LPCMVParserLocalParams_p->FrameComplete = TRUE;
    LPCMVParserLocalParams_p->First_Frame   = TRUE;
    LPCMVParserLocalParams_p->Get_Block     = FALSE;
    LPCMVParserLocalParams_p->Put_Block     = TRUE;
    LPCMVParserLocalParams_p->OpBufHandle   = Init_p->OpBufHandle;
    LPCMVParserLocalParams_p->AppData_p[0]  = AppData_p[0];
    LPCMVParserLocalParams_p->AppData_p[1]  = AppData_p[1];

    LPCMVParserLocalParams_p->EvtHandle               = Init_p->EvtHandle;
    LPCMVParserLocalParams_p->ObjectID                = Init_p->ObjectID;
    LPCMVParserLocalParams_p->EventIDNewFrame         = Init_p->EventIDNewFrame;
    LPCMVParserLocalParams_p->EventIDFrequencyChange  = Init_p->EventIDFrequencyChange;

    *handle=(ParserHandle_t )LPCMVParserLocalParams_p;

    return (Error);
}

ST_ErrorCode_t ES_LPCMV_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                 GetFreqFunction_t * GetFreqFunction_p,
                                                 GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p     = (ParsingFunction_t)ES_LPCMV_Parse_Frame;
    *GetFreqFunction_p = (GetFreqFunction_t)ES_LPCMV_Get_Freq;
    *GetInfoFunction_p = (GetInfoFunction_t)ES_LPCMV_Get_Info;

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : ES_LPCMV_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMV_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p;
    LPCMVParserLocalParams_p=(ES_LPCMV_ParserLocalParams_t *)Handle;

    if(LPCMVParserLocalParams_p->Get_Block == TRUE)
    {
        LPCMVParserLocalParams_p->OpBlk_p->Size = LPCMVParserLocalParams_p->FrameSize;
        LPCMVParserLocalParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(LPCMVParserLocalParams_p->OpBufHandle,(U32 *)&LPCMVParserLocalParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_LPCMV Error in Memory Put_Block \n"));
        }
        else
        {
            LPCMVParserLocalParams_p->Put_Block = TRUE;
            LPCMVParserLocalParams_p->Get_Block = FALSE;
        }
    }

    LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_COMPUTE_FRAME_LENGTH;

    LPCMVParserLocalParams_p->FrameComplete = TRUE;
    LPCMVParserLocalParams_p->First_Frame   = TRUE;


    return Error;
}

/************************************************************************************
Name         : ES_LPCMV_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_LPCMV_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p;
    LPCMVParserLocalParams_p=(ES_LPCMV_ParserLocalParams_t *)Handle;


    LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_COMPUTE_FRAME_LENGTH;
    LPCMVParserLocalParams_p->FrameComplete = TRUE;
    LPCMVParserLocalParams_p->First_Frame   = TRUE;
    LPCMVParserLocalParams_p->Put_Block     = TRUE;
    LPCMVParserLocalParams_p->Get_Block     = FALSE;

    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_LPCMV_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p;
    LPCMVParserLocalParams_p=(ES_LPCMV_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=LPCMVParserLocalParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_LPCMV_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p;
    LPCMVParserLocalParams_p=(ES_LPCMV_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = LPCMVParserLocalParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_LPCMV_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p = (ES_LPCMV_ParserLocalParams_t *)Handle;

    if(LPCMVParserLocalParams_p->Get_Block == TRUE)
    {
        memset(LPCMVParserLocalParams_p->Current_Write_Ptr1,0x00,LPCMVParserLocalParams_p->NoBytesRemainingInFrame);

        LPCMVParserLocalParams_p->OpBlk_p->Size = LPCMVParserLocalParams_p->FrameSize;
        LPCMVParserLocalParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        LPCMVParserLocalParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(LPCMVParserLocalParams_p->OpBufHandle,(U32 *)&LPCMVParserLocalParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            LPCMVParserLocalParams_p->Put_Block = TRUE;
            LPCMVParserLocalParams_p->Get_Block = FALSE;
        }
    }

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : Parse_LPCMV_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_LPCMV_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{
    U8 *pos;
    U32 offset;
    ST_ErrorCode_t Error;
    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;
    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_LPCMV Parser Enrty \n"));

    LPCMVParserLocalParams_p=(ES_LPCMV_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {
        if(LPCMVParserLocalParams_p->FrameComplete == TRUE)
        {
            if(LPCMVParserLocalParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(LPCMVParserLocalParams_p->OpBufHandle, (U32 *)&LPCMVParserLocalParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    LPCMVParserLocalParams_p->Put_Block = FALSE;
                    LPCMVParserLocalParams_p->Get_Block = TRUE;
                    LPCMVParserLocalParams_p->Current_Write_Ptr1 = (U8 *)LPCMVParserLocalParams_p->OpBlk_p->MemoryStart;
                    //memset(LPCMVParserLocalParams_p->Current_Write_Ptr1,0x00,LPCMVParserLocalParams_p->OpBlk_p->MaxSize);
                    LPCMVParserLocalParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {
                        LPCMVParserLocalParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        LPCMVParserLocalParams_p->OpBlk_p->Data[PTS_OFFSET]   = CurrentPTS_p->PTS;
                        LPCMVParserLocalParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;
                    }
                }
            }

            CurrentPTS_p->PTSValidFlag      = FALSE;
            LPCMVParserLocalParams_p->FrameComplete = FALSE;
        }

        switch(LPCMVParserLocalParams_p->LPCMVParserState)
        {
        case ES_LPCMV_COMPUTE_FRAME_LENGTH:

            LPCMVParserLocalParams_p->Current_Write_Ptr1 = (U8 *)LPCMVParserLocalParams_p->OpBlk_p->MemoryStart;
            {


                U32 private_data    = Private_Info_p->Private_LPCM_Data[0];
                U32 frame_length;
                U8  sampling_freq   = ((private_data >> 12) & 0x03);            //2 bits
                //Unpack decoding parameters

                //U8  DrcCode         = private_data & 0xFF;                      // 8 bits
                U8  nb_channels     = ((private_data >>  8) & 0x07) + 1;        // 3 bits

                U8  ws              = ((private_data >> 14) & 0x03);            // 2 bits

                //BOOL    MuteFlag;
                //BOOL    EmphasisFlag;

                private_data            >>= 22; /* skip audio frame number and reserved bit */
                //MuteFlag        = private_data & 1;
                //EmphasisFlag    = (private_data >> 1) & 1;

                // Compute length according to Fs / Q / Nch
                frame_length    = LPCM_VIDEO_BASE_FRAME_SIZE  + (LPCM_VIDEO_BASE_FRAME_SIZE/4) * ws ;
                frame_length    = frame_length * nb_channels * 2;

                LPCMVParserLocalParams_p->FrameSize = frame_length;

                if(LPCMVParserLocalParams_p->FrameSize == 0)
                {
                    STTBX_Print((" ES_LPCMV_ Parser ERROR in Frame Length \n "));
                    offset = Size; // Drop the Current Packet
                    break;
                }
                if(LPCMVParserLocalParams_p->First_Frame)
                {
                    LPCMVParserLocalParams_p->First_Frame     = FALSE;
                    LPCMVParserLocalParams_p->Frequency       = LPCMFrequency[sampling_freq];
                    LPCMVParserLocalParams_p->AppData_p[0]->sampleRate        = sampling_freq;
                    LPCMVParserLocalParams_p->AppData_p[0]->sampleBits            = ws;
                    LPCMVParserLocalParams_p->AppData_p[0]->channels      = nb_channels;
                    LPCMVParserLocalParams_p->AppData_p[0]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                    LPCMVParserLocalParams_p->AppData_p[0]->ChangeSet = TRUE;

                    LPCMVParserLocalParams_p->AppData_p[1]->sampleRate        = sampling_freq;
                    LPCMVParserLocalParams_p->AppData_p[1]->sampleBits            = ws;
                    LPCMVParserLocalParams_p->AppData_p[1]->channels      = nb_channels;
                    LPCMVParserLocalParams_p->AppData_p[1]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                    LPCMVParserLocalParams_p->AppData_p[1]->ChangeSet = FALSE;

                    LPCMVParserLocalParams_p->OpBlk_p->AppData_p = LPCMVParserLocalParams_p->AppData_p[0];
                    LPCMVParserLocalParams_p->AppDataFlag = TRUE;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->Data[FREQ_OFFSET]  = LPCMVParserLocalParams_p->Frequency;
                    Error=STAudEVT_Notify(LPCMVParserLocalParams_p->EvtHandle,LPCMVParserLocalParams_p->EventIDFrequencyChange, &LPCMVParserLocalParams_p->Frequency, sizeof(&LPCMVParserLocalParams_p->Frequency), LPCMVParserLocalParams_p->ObjectID);
                    /* CONFIG PARAMETER FOR LPCM

                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= FsChGr1_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->FsChGr1        =  LPCMVParserLocalParams_p->FsChGr1;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= WsChGr1_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->WsChGr1        =  LPCMVParserLocalParams_p->WsChGr1;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= NbChannels_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->NbChannels     =  LPCMVParserLocalParams_p->NbChannels;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= DrcCode_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->DrcCode        =  LPCMVParserLocalParams_p->DrcCode;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= MuteFlag_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->MuteFlag       =  LPCMVParserLocalParams_p->MuteFlag;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags          |= EmphasisFlag_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->EmphasisFlag   =  LPCMVParserLocalParams_p->EmphasisFlag;
                    CONFIG PARAMETER FOR LPCM */

                }
                if(LPCMVParserLocalParams_p->Frequency != LPCMFrequency[sampling_freq])
                {

                    LPCMVParserLocalParams_p->Frequency           = LPCMFrequency[sampling_freq];
                    LPCMVParserLocalParams_p->OpBlk_p->Flags      |= FREQ_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->Data[FREQ_OFFSET]  =LPCMVParserLocalParams_p->Frequency;
                    Error=STAudEVT_Notify(LPCMVParserLocalParams_p->EvtHandle,LPCMVParserLocalParams_p->EventIDFrequencyChange, &LPCMVParserLocalParams_p->Frequency, sizeof(&LPCMVParserLocalParams_p->Frequency), LPCMVParserLocalParams_p->ObjectID);
                }
                /* CONFIG PARAMETER FOR LPCM
                if(LPCMVParserLocalParams_p->FsChGr1 != sampling_freq)
                {

                    LPCMVParserLocalParams_p->FsChGr1             = sampling_freq;
                    LPCMVParserLocalParams_p->OpBlk_p->Flags      |= FsChGr1_VALID;
                    LPCMVParserLocalParams_p->OpBlk_p->FsChGr1    =  LPCMVParserLocalParams_p->FsChGr1;
                }
                WsChGr1  NbChannels  DrcCode  MuteFlag  EmphasisFlag (TO ADD)*/
                if(LPCMVParserLocalParams_p->AppDataFlag == TRUE)
                {
                    if((LPCMVParserLocalParams_p->AppData_p[0]->sampleRate != sampling_freq)||
                    (LPCMVParserLocalParams_p->AppData_p[0]->sampleBits != ws)||
                    (LPCMVParserLocalParams_p->AppData_p[0]->channels != nb_channels))
                    {
                        LPCMVParserLocalParams_p->AppData_p[0]->sampleRate    = sampling_freq;
                        LPCMVParserLocalParams_p->AppData_p[0]->sampleBits    = ws;
                        LPCMVParserLocalParams_p->AppData_p[0]->channels      = nb_channels;
                        LPCMVParserLocalParams_p->AppData_p[0]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                        LPCMVParserLocalParams_p->AppData_p[0]->ChangeSet     = TRUE;
                    }
                    LPCMVParserLocalParams_p->OpBlk_p->AppData_p = LPCMVParserLocalParams_p->AppData_p[0];
                    LPCMVParserLocalParams_p->AppDataFlag = FALSE;
                }
                else
                {
                    if((LPCMVParserLocalParams_p->AppData_p[1]->sampleRate != sampling_freq)||
                    (LPCMVParserLocalParams_p->AppData_p[1]->sampleBits != ws)||
                    (LPCMVParserLocalParams_p->AppData_p[1]->channels != nb_channels))
                    {
                        LPCMVParserLocalParams_p->AppData_p[1]->sampleRate    = sampling_freq;
                        LPCMVParserLocalParams_p->AppData_p[1]->sampleBits    = ws;
                        LPCMVParserLocalParams_p->AppData_p[1]->channels      = nb_channels;
                        LPCMVParserLocalParams_p->AppData_p[1]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                        LPCMVParserLocalParams_p->AppData_p[1]->ChangeSet     = TRUE;
                    }
                    LPCMVParserLocalParams_p->OpBlk_p->AppData_p = LPCMVParserLocalParams_p->AppData_p[1];
                    LPCMVParserLocalParams_p->AppDataFlag = TRUE;
                }
            }

            LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_FRAME;
            break;

        case ES_LPCMV_FRAME:
            if((Size-offset)>=LPCMVParserLocalParams_p->FrameSize)
            {
                /* 1 or more frame */
                LPCMVParserLocalParams_p->FrameComplete = TRUE;

                memcpy(LPCMVParserLocalParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(LPCMVParserLocalParams_p->FrameSize ));

                LPCMVParserLocalParams_p->OpBlk_p->Size = LPCMVParserLocalParams_p->FrameSize;
                if(LPCMVParserLocalParams_p->Get_Block == TRUE)
                {
                    if(LPCMVParserLocalParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = LPCMVParserLocalParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = LPCMVParserLocalParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(LPCMVParserLocalParams_p->OpBufHandle,(U32 *)&LPCMVParserLocalParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_LPCMV Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(LPCMVParserLocalParams_p->EvtHandle,LPCMVParserLocalParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), LPCMVParserLocalParams_p->ObjectID);
                        LPCMVParserLocalParams_p->Put_Block = TRUE;
                        LPCMVParserLocalParams_p->Get_Block = FALSE;
                    }
                }

                LPCMVParserLocalParams_p->Current_Write_Ptr1 += (LPCMVParserLocalParams_p->FrameSize );

                offset =  offset+LPCMVParserLocalParams_p->FrameSize;
                LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_COMPUTE_FRAME_LENGTH;

            }
            else
            {
                /* less than a frame */
                LPCMVParserLocalParams_p->NoBytesRemainingInFrame = LPCMVParserLocalParams_p->FrameSize  - (Size-offset);
                LPCMVParserLocalParams_p->FrameComplete = FALSE;

                memcpy(LPCMVParserLocalParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                LPCMVParserLocalParams_p->Current_Write_Ptr1 += (Size - offset);

                offset =  Size;


                LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_PARTIAL_FRAME;
            }
            break;

        case ES_LPCMV_PARTIAL_FRAME:


            if(Size-offset>=LPCMVParserLocalParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                LPCMVParserLocalParams_p->FrameComplete = TRUE;
                memcpy(LPCMVParserLocalParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),LPCMVParserLocalParams_p->NoBytesRemainingInFrame);


                LPCMVParserLocalParams_p->OpBlk_p->Size = LPCMVParserLocalParams_p->FrameSize;
                if(LPCMVParserLocalParams_p->Get_Block == TRUE)
                {
                    if(LPCMVParserLocalParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = LPCMVParserLocalParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = LPCMVParserLocalParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(LPCMVParserLocalParams_p->OpBufHandle,(U32 *)&LPCMVParserLocalParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_LPCMV Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(LPCMVParserLocalParams_p->EvtHandle,LPCMVParserLocalParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), LPCMVParserLocalParams_p->ObjectID);
                        LPCMVParserLocalParams_p->Put_Block = TRUE;
                        LPCMVParserLocalParams_p->Get_Block = FALSE;
                    }
                }

                LPCMVParserLocalParams_p->Current_Write_Ptr1 += LPCMVParserLocalParams_p->NoBytesRemainingInFrame;
                offset =  offset+LPCMVParserLocalParams_p->NoBytesRemainingInFrame;
                LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_COMPUTE_FRAME_LENGTH;

            }
            else
            {
                /* again less than a frame */
                LPCMVParserLocalParams_p->NoBytesRemainingInFrame -= (Size-offset);
                LPCMVParserLocalParams_p->FrameComplete = FALSE;
                memcpy(LPCMVParserLocalParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                LPCMVParserLocalParams_p->Current_Write_Ptr1 += Size-offset;

                LPCMVParserLocalParams_p->LPCMVParserState = ES_LPCMV_PARTIAL_FRAME;

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

ST_ErrorCode_t ES_LPCMV_Parser_Term(ParserHandle_t *const handle, ST_Partition_t    *CPUPartition_p)
{
    ES_LPCMV_ParserLocalParams_t *LPCMVParserLocalParams_p = (ES_LPCMV_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, LPCMVParserLocalParams_p->AppData_p[0]);
        STOS_MemoryDeallocate(CPUPartition_p, LPCMVParserLocalParams_p->AppData_p[1]);
        STOS_MemoryDeallocate(CPUPartition_p, LPCMVParserLocalParams_p);
    }

    return ST_NO_ERROR;
}

#endif

