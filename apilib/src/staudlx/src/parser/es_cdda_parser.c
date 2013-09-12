/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_CDDA_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
//#define  STTBX_PRINT
#ifdef STAUD_INSTALL_CDDA
#include "es_cdda_parser.h"
#include "acc_multicom_toolbox.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

#define CDDA_FREQUENCY  44100
#define CDDA_NCH 2
#define CDDA_SAMPLESIZE 0
/************************************************************************************
Name         : ES_CDDA_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_CDDA_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_CDDA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    STAud_LpcmStreamParams_t *AppData_p[2];
    CDDAParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_CDDA_ParserLocalParams_t));
    if(CDDAParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(CDDAParserParams_p,0,sizeof(ES_CDDA_ParserLocalParams_t));

    AppData_p[0] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
    AppData_p[1] = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_LpcmStreamParams_t));
#if defined(PES_TO_ES_BY_FDMA)
    CDDAParserParams_p->node_p=Init_p->NodePESToES_p;
    CDDAParserParams_p->FDMAnodeAddress.BaseUnCachedVirtualAddress=(U32 *)Init_p->NodePESToES_p;
    CDDAParserParams_p->FDMAnodeAddress.BasePhyAddress = (U32*)Init_p->NodePESToES_p;
#endif

    CDDAParserParams_p->CDDAParserState = ES_CDDA_COMPUTE_FRAME_LENGTH;

    CDDAParserParams_p->FrameComplete = TRUE;
    CDDAParserParams_p->First_Frame   = TRUE;
    CDDAParserParams_p->UpdatePcm     = FALSE;
    CDDAParserParams_p->Get_Block     = FALSE;
    CDDAParserParams_p->Put_Block     = TRUE;
    CDDAParserParams_p->OpBufHandle   = Init_p->OpBufHandle;
    CDDAParserParams_p->AppData_p[0]  = AppData_p[0];
    CDDAParserParams_p->AppData_p[1]  = AppData_p[1];

    CDDAParserParams_p->EvtHandle              = Init_p->EvtHandle;
    CDDAParserParams_p->ObjectID               = Init_p->ObjectID;
    CDDAParserParams_p->EventIDNewFrame        = Init_p->EventIDNewFrame;
    CDDAParserParams_p->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;
    CDDAParserParams_p->Blk_Out                = 0;
    CDDAParserParams_p->MixerEnabled           = Init_p->MixerEnabled;
    *handle=(ParserHandle_t )CDDAParserParams_p;


    return (Error);
}

ST_ErrorCode_t ES_CDDA_ParserGetParsingFunction( ParsingFunction_t  * ParsingFunc_p,
                                                 GetFreqFunction_t * GetFreqFunction_p,
                                                 GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_CDDA_Parse_Frame;
    *GetFreqFunction_p= (GetFreqFunction_t)ES_CDDA_Get_Freq;
    *GetInfoFunction_p= (GetInfoFunction_t)ES_CDDA_Get_Info;

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_CDDA_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_CDDA_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;

    if(CDDAParserParams_p->Get_Block == TRUE)
    {
        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
        CDDAParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(CDDAParserParams_p->OpBufHandle,(U32 *)&CDDAParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_CDDA Error in Memory Put_Block \n"));
        }
        else
        {
            CDDAParserParams_p->Put_Block = TRUE;
            CDDAParserParams_p->Get_Block = FALSE;
        }
    }



    CDDAParserParams_p->CDDAParserState = ES_CDDA_COMPUTE_FRAME_LENGTH;


    CDDAParserParams_p->FrameComplete = TRUE;
    CDDAParserParams_p->First_Frame   = TRUE;


    return Error;
}

/************************************************************************************
Name         : ES_CDDA_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_CDDA_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;


    CDDAParserParams_p->CDDAParserState = ES_CDDA_COMPUTE_FRAME_LENGTH;
    CDDAParserParams_p->FrameComplete = TRUE;
    CDDAParserParams_p->First_Frame   = TRUE;

    CDDAParserParams_p->Put_Block = TRUE;
    CDDAParserParams_p->Get_Block = FALSE;

    CDDAParserParams_p->Blk_Out    = 0;


    return Error;
}

/* Set the PCM Params for CDDA decoder Decoder */
ST_ErrorCode_t ES_CDDA_Set_Pcm(ParserHandle_t const Handle,U32 Frequency, U32 SampleSize, U32 Nch)
{

    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;

    CDDAParserParams_p->NewFrequency   = Frequency;
    CDDAParserParams_p->NewNch             = (U8)Nch;
    CDDAParserParams_p->NewSampleSize  = SampleSize;

    CDDAParserParams_p->UpdatePcm = TRUE;

    return ST_NO_ERROR;
}

/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_CDDA_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=CDDAParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_CDDA_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = CDDAParserParams_p->Frequency;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_CDDA_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;

    if(CDDAParserParams_p->Get_Block == TRUE)
    {
        memset(CDDAParserParams_p->Current_Write_Ptr1,0x00,CDDAParserParams_p->NoBytesRemainingInFrame);

        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
        CDDAParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        CDDAParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        CDDAParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(CDDAParserParams_p->OpBufHandle,(U32 *)&CDDAParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_CDDA Error in Memory Put_Block \n"));
        }
        else
        {
            CDDAParserParams_p->Put_Block = TRUE;
            CDDAParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(CDDAParserParams_p->OpBufHandle, (U32 *)&CDDAParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            CDDAParserParams_p->Put_Block = FALSE;
            CDDAParserParams_p->Get_Block = TRUE;
            CDDAParserParams_p->OpBlk_p->Size = 384 * CDDAParserParams_p->Nch * CDDAParserParams_p->SampleSize;
            CDDAParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)CDDAParserParams_p->OpBlk_p->MemoryStart,0x00,CDDAParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(CDDAParserParams_p->OpBufHandle,(U32 *)&CDDAParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
            }
            else
            {
                CDDAParserParams_p->Put_Block = TRUE;
                CDDAParserParams_p->Get_Block = FALSE;
            }
        }
    }

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : Parse_CDDA_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_CDDA_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info)
{
    U8 *pos;
    U32 offset;
    ST_ErrorCode_t Error;
    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p;
    STAUD_PTS_t NotifyPTS;
    U32 PES_Frequency;
    U32 PES_Nch;
    U32 PES_SampleSize;
    U8  PES_QWL; /* Quant Word Length*/
    U8  PES_Sign; /* Signed or Unsigned */
    U8  PES_Allign; /* Endianness */
    U8  BytesPerSample=0;
    U32 BytesPlayed = 0;

    CDDAParserParams_p=(ES_CDDA_ParserLocalParams_t *)Handle;
    PES_SampleSize=0;

    offset=*No_Bytes_Used;
    pos=(U8 *)MemoryStart;

    //  STTBX_Print((" ES_CDDA Parser Enrty \n"));

    if((Private_Info->Private_LPCM_Data[0])&&(Private_Info->Private_Data[7])&&(Private_Info->Private_Data[8]))
    {
        /* Extracting PES information */
        PES_Frequency=Private_Info->Private_LPCM_Data[0];
        PES_QWL = Private_Info->Private_Data[7];
        PES_Nch = Private_Info->Private_Data[8];
        PES_Sign = (Private_Info->Private_Data[9]>>6)&0x1;
        PES_Allign = (Private_Info->Private_Data[9]>>5)&0x1;

        if(((PES_Frequency== 48000)||(PES_Frequency== 44100)||(PES_Frequency== 32000)||
        (PES_Frequency== 88200)||(PES_Frequency== 96000)||(PES_Frequency== 64000)||
        (PES_Frequency== 192000)||(PES_Frequency== 176400)||(PES_Frequency== 128000)||
        (PES_Frequency== 384000)||(PES_Frequency== 352800)||(PES_Frequency== 24000)||
        (PES_Frequency== 22050)||(PES_Frequency== 16000)||(PES_Frequency== 256000))&&
        ((PES_QWL==8)||(PES_QWL==16)||(PES_QWL==24)||(PES_QWL==32)||(PES_QWL==20)))
        {/*Passing this test means that PES params are valid and they should be applied*/

            /* Do processing of Sample Size here by taking into consideration
            QWL, Signed/Unsigned and Allignment */
            switch(PES_QWL)
            {

            case 8:
                BytesPerSample=1;
                if(PES_Sign==1)
                    PES_SampleSize=ACC_LPCM_WS8s; //16
                else
                    PES_SampleSize=ACC_LPCM_WS8u; //17
                break;
            case 16:

                BytesPerSample=2;
                switch(PES_Allign)
                {
                case 0: //little endian
                    if(PES_Sign==1)
                        PES_SampleSize=ACC_LPCM_WS16le; //65536
                    else
                        PES_SampleSize=ACC_LPCM_WS16ule; //196608
                    break;
                case 1: //big endian
                    if(PES_Sign==1)
                        PES_SampleSize=ACC_LPCM_WS16; //0
                    else
                        PES_SampleSize=ACC_LPCM_WS16u; // 131072
                    break;
                default:
                    PES_SampleSize=ACC_LPCM_WS16;
                    break;

                }
                break;
            case 24:
                BytesPerSample=3;
                PES_SampleSize = ACC_LPCM_WS24; // 2
                break;
            case 32:
                BytesPerSample=4;
                PES_SampleSize = ACC_LPCM_WS32; // 4
                break;
            default:
                PES_SampleSize = 0;
                break;
            }

        }
        if(CDDAParserParams_p->First_Frame == FALSE)
        {
            /* Changing these 4 will help in all subsequent frames*/
            CDDAParserParams_p->NewFrequency = PES_Frequency;
            CDDAParserParams_p->NewNch = PES_Nch;
            CDDAParserParams_p->NewSampleSize = PES_SampleSize;
            CDDAParserParams_p->UpdatePcm=TRUE;
        }
    }
    else
    {
        /* This is the case of corrupted information in PES PrivateData*/
        PES_Frequency   = CDDA_FREQUENCY;
        PES_Nch         = CDDA_NCH;
        PES_SampleSize  = CDDA_SAMPLESIZE;
    }

    while(offset<Size)
    {

        if(CDDAParserParams_p->FrameComplete == TRUE)
        {
            if(CDDAParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(CDDAParserParams_p->OpBufHandle, (U32 *)&CDDAParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    CDDAParserParams_p->Put_Block = FALSE;
                    CDDAParserParams_p->Get_Block = TRUE;
                    CDDAParserParams_p->Current_Write_Ptr1 = (U8 *)CDDAParserParams_p->OpBlk_p->MemoryStart;
                    //memset(CDDAParserParams_p->Current_Write_Ptr1,0x00,CDDAParserParams_p->OpBlk_p->MaxSize);
                    CDDAParserParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {

                        CDDAParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        CDDAParserParams_p->OpBlk_p->Data[PTS_OFFSET]  = CurrentPTS_p->PTS;
                        CDDAParserParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;

                    }

                }
            }

            CurrentPTS_p->PTSValidFlag      = FALSE;
            CDDAParserParams_p->FrameComplete = FALSE;
        }


        switch(CDDAParserParams_p->CDDAParserState)
        {
        case ES_CDDA_COMPUTE_FRAME_LENGTH:

            CDDAParserParams_p->Current_Write_Ptr1 = (U8 *)CDDAParserParams_p->OpBlk_p->MemoryStart;

            {
                if(CDDAParserParams_p->First_Frame)
                {

                    CDDAParserParams_p->First_Frame    = FALSE;

                    CDDAParserParams_p->Frequency  = PES_Frequency;
                    CDDAParserParams_p->Nch            = PES_Nch;
                    CDDAParserParams_p->SampleSize     = PES_SampleSize;
                    CDDAParserParams_p->WordSizeInBytes = (CovertFromLpcmWSCode(CDDAParserParams_p->SampleSize ));

                    CDDAParserParams_p->AppData_p[0]->sampleRate           = CDDAParserParams_p->Frequency;
                    CDDAParserParams_p->AppData_p[0]->sampleBits       = CDDAParserParams_p->SampleSize;
                    CDDAParserParams_p->AppData_p[0]->channels         = CDDAParserParams_p->Nch;
                    CDDAParserParams_p->AppData_p[0]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                    CDDAParserParams_p->AppData_p[0]->ChangeSet        = TRUE;

                    CDDAParserParams_p->AppData_p[1]->sampleRate           = CDDAParserParams_p->Frequency;
                    CDDAParserParams_p->AppData_p[1]->sampleBits       = CDDAParserParams_p->SampleSize;
                    CDDAParserParams_p->AppData_p[1]->channels         = CDDAParserParams_p->Nch;
                    CDDAParserParams_p->AppData_p[1]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                    CDDAParserParams_p->AppData_p[1]->ChangeSet        = FALSE;

                    CDDAParserParams_p->AppDataFlag = TRUE;

                    CDDAParserParams_p->OpBlk_p->AppData_p = CDDAParserParams_p->AppData_p[0];
                    CDDAParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                    CDDAParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = CDDAParserParams_p->Frequency;
                    Error=STAudEVT_Notify(CDDAParserParams_p->EvtHandle,CDDAParserParams_p->EventIDFrequencyChange, &CDDAParserParams_p->Frequency, sizeof(CDDAParserParams_p->Frequency), CDDAParserParams_p->ObjectID);


                }

                if(CDDAParserParams_p->UpdatePcm == TRUE)
                {
                    CDDAParserParams_p->UpdatePcm = FALSE;
                    if(CDDAParserParams_p->Frequency   != CDDAParserParams_p->NewFrequency)
                    {
                        CDDAParserParams_p->Frequency  =CDDAParserParams_p->NewFrequency;
                        CDDAParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                        CDDAParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = CDDAParserParams_p->Frequency;
                        Error=STAudEVT_Notify(CDDAParserParams_p->EvtHandle,CDDAParserParams_p->EventIDFrequencyChange, &CDDAParserParams_p->Frequency, sizeof(CDDAParserParams_p->Frequency), CDDAParserParams_p->ObjectID);
                    }

                    CDDAParserParams_p->Nch            = CDDAParserParams_p->NewNch;
                    CDDAParserParams_p->SampleSize     = CDDAParserParams_p->NewSampleSize;

                    CDDAParserParams_p->WordSizeInBytes = (CovertFromLpcmWSCode(CDDAParserParams_p->SampleSize ));
                    if(CDDAParserParams_p->AppDataFlag == TRUE)
                    {
                        CDDAParserParams_p->AppData_p[0]->sampleRate       = CDDAParserParams_p->Frequency;
                        CDDAParserParams_p->AppData_p[0]->sampleBits       = CDDAParserParams_p->SampleSize;
                        CDDAParserParams_p->AppData_p[0]->channels         = CDDAParserParams_p->Nch;
                        CDDAParserParams_p->AppData_p[0]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                        CDDAParserParams_p->AppData_p[0]->ChangeSet        = TRUE;
                        CDDAParserParams_p->AppDataFlag                    = FALSE;
                        CDDAParserParams_p->OpBlk_p->AppData_p                 = CDDAParserParams_p->AppData_p[0];

                    }
                    else
                    {
                        CDDAParserParams_p->AppData_p[1]->sampleRate           = CDDAParserParams_p->Frequency;
                        CDDAParserParams_p->AppData_p[1]->sampleBits       = CDDAParserParams_p->SampleSize;
                        CDDAParserParams_p->AppData_p[1]->channels         = CDDAParserParams_p->Nch;
                        CDDAParserParams_p->AppData_p[1]->channelAssignment = LPCM_DEFAULT_CHANNEL_ASSIGNMENT;
                        CDDAParserParams_p->AppData_p[1]->ChangeSet        = TRUE;
                        CDDAParserParams_p->AppDataFlag                    = TRUE;
                        CDDAParserParams_p->OpBlk_p->AppData_p                 = CDDAParserParams_p->AppData_p[1];

                    }


                }
#if defined(ST_51XX)
                if(CDDAParserParams_p->MixerEnabled)
                {
                    CDDAParserParams_p->FrameSize  = (CDDA_NB_SAMPLES * CDDAParserParams_p->WordSizeInBytes * CDDAParserParams_p->Nch);

                }
                else
                {
                    CDDAParserParams_p->FrameSize  = (CDDA_NB_SAMPLES * 1 * CDDAParserParams_p->Nch);
                }
#else
                CDDAParserParams_p->FrameSize  = (CDDA_NB_SAMPLES * CDDAParserParams_p->WordSizeInBytes * CDDAParserParams_p->Nch);
#endif
                CDDAParserParams_p->NoBytesRemainingInFrame = CDDAParserParams_p->FrameSize ;
            }

            CDDAParserParams_p->CDDAParserState = ES_CDDA_FRAME;
            break;

        case ES_CDDA_FRAME:

            if(Size-offset>=CDDAParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */

                BytesPlayed = CDDAParserParams_p->NoBytesRemainingInFrame;
                CDDAParserParams_p->FrameComplete = TRUE;
#if defined (PES_TO_ES_BY_FDMA) && defined(ST_51XX)
                if(!CDDAParserParams_p->MixerEnabled)
                {
                    if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS16le))
                    {

                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1+1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed/2,2,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset+1);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed/2,2,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                        ((STAud_LpcmStreamParams_t *)CDDAParserParams_p->OpBlk_p->AppData_p)->sampleBits=16;

                    }
                    else if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS8s))
                    {
                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1+1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed,1,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize*2;
                        CDDAParserParams_p->Current_Write_Ptr1 += 2*BytesPlayed;
                        ((STAud_LpcmStreamParams_t *)CDDAParserParams_p->OpBlk_p->AppData_p)->sampleBits=16;

                    }
                    else
                    {
                        memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                }
                else
                {
                    memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                    CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                    CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                }
#elif defined(ST_51XX)
                if(!CDDAParserParams_p->MixerEnabled)
                {

                    if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS16le))
                    {
                        swap_input((U8 *)((char *)MemoryStart+offset),CDDAParserParams_p->Current_Write_Ptr1,BytesPlayed);
                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                        ((STAud_LpcmStreamParams_t *)CDDAParserParams_p->OpBlk_p->AppData_p)->sampleBits=16;

                    }
                    else if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS8s))
                    {
                        Convert8bitTo16bit((U8 *)((char *)MemoryStart+offset),CDDAParserParams_p->Current_Write_Ptr1,BytesPlayed);
                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize*2;
                        CDDAParserParams_p->Current_Write_Ptr1 += 2*BytesPlayed;
                        ((STAud_LpcmStreamParams_t *)CDDAParserParams_p->OpBlk_p->AppData_p)->sampleBits=16;

                    }
                    else
                    {
                        memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                        CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                }
                else
                {
                    memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                    CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                    CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                }
#else
                memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                CDDAParserParams_p->OpBlk_p->Size = CDDAParserParams_p->FrameSize;
                CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
#endif


                if(CDDAParserParams_p->Get_Block == TRUE)
                {
                    if(CDDAParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {

                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = CDDAParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = CDDAParserParams_p->OpBlk_p->Data[PTS33_OFFSET];

                        //STTBX_Print(("PTSToBeSent = %u\n",CDDAParserParams_p->PTSToBeSent));
                    }
                    else
                    {

                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

                    Error = MemPool_PutOpBlk(CDDAParserParams_p->OpBufHandle,(U32 *)&CDDAParserParams_p->OpBlk_p);

                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_CDDA Error in Memory Put_Block,%u \n",NotifyPTS.Low));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(CDDAParserParams_p->EvtHandle,CDDAParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), CDDAParserParams_p->ObjectID);
                        STTBX_Print((" CDDA deli,%u\n",CDDAParserParams_p->Blk_Out));
                        CDDAParserParams_p->Put_Block = TRUE;
                        CDDAParserParams_p->Get_Block = FALSE;
                    }
                }
                offset +=  BytesPlayed;
                CDDAParserParams_p->NoBytesRemainingInFrame = 0;
                CDDAParserParams_p->CDDAParserState = ES_CDDA_COMPUTE_FRAME_LENGTH;

            }
            else
            {
                /* again less than a frame */
                BytesPlayed = Size-offset;

                CDDAParserParams_p->FrameComplete = FALSE;

#if defined (PES_TO_ES_BY_FDMA) && defined(ST_51XX)
                if(!CDDAParserParams_p->MixerEnabled)
                {
                    if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS16le))
                    {

                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1+1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed/2,2,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset+1);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed/2,2,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                    else if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS8s))
                    {
                        CDDAParserParams_p->node_p->Node.DestinationAddress_p = CDDAParserParams_p->Current_Write_Ptr1+1;
                        CDDAParserParams_p->node_p->Node.SourceAddress_p = ((char *)MemoryStart+offset);

                        FDMAMemcpy(CDDAParserParams_p->node_p->Node.DestinationAddress_p,CDDAParserParams_p->node_p->Node.SourceAddress_p,1,BytesPlayed,1,2,&CDDAParserParams_p->FDMAnodeAddress);

                        CDDAParserParams_p->Current_Write_Ptr1 += 2*BytesPlayed;

                    }
                    else
                    {
                        memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                }
                else
                {
                    memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                    CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                }
#elif defined(ST_51XX)
                if(!CDDAParserParams_p->MixerEnabled)
                {
                    if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS16le))
                    {
                        swap_input((U8 *)((char *)MemoryStart+offset),CDDAParserParams_p->Current_Write_Ptr1,BytesPlayed);
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                    else if((CovertToLpcmWSCode(CDDAParserParams_p->SampleSize)==ACC_LPCM_WS8s))
                    {
                        Convert8bitTo16bit((U8 *)((char *)MemoryStart+offset),CDDAParserParams_p->Current_Write_Ptr1,BytesPlayed);
                        CDDAParserParams_p->Current_Write_Ptr1 += 2*BytesPlayed;

                    }
                    else
                    {
                        memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                        CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;

                    }
                }
                else
                {
                    memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                    CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
                }

#else
                memcpy(CDDAParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),BytesPlayed);
                CDDAParserParams_p->Current_Write_Ptr1 += BytesPlayed;
#endif

                CDDAParserParams_p->NoBytesRemainingInFrame -= BytesPlayed;

                CDDAParserParams_p->CDDAParserState = ES_CDDA_FRAME;

                offset =  Size;
            }

            if(CurrentPTS_p->PTSValidFlag == TRUE)
            {
                U32 InterpolatedPTS = (BytesPlayed*1000*90)/(CDDAParserParams_p->Nch * BytesPerSample * CDDAParserParams_p->Frequency);
                CurrentPTS_p->PTS += InterpolatedPTS;
            }

            break;
        default:
            break;
        }
    }
    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_CDDA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t *CPUPartition_p)
{

    ES_CDDA_ParserLocalParams_t *CDDAParserParams_p = (ES_CDDA_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, CDDAParserParams_p->AppData_p[0]);
        STOS_MemoryDeallocate(CPUPartition_p, CDDAParserParams_p->AppData_p[1]);
        STOS_MemoryDeallocate(CPUPartition_p, CDDAParserParams_p);
    }

    return ST_NO_ERROR;
}
#endif
#ifdef ST_51XX
void swap_input(U8 *pcmin,U8 *pcmout, U32 nbspl)
{
    U32   spl,tmp;
    U16 * spl_out;
    U16   input ;
    U16 * spl_in,in_byte;
    int   in_idx , out_idx;
    spl_in  = (U16 *)pcmin;
    spl_out = (U16 *)pcmout;
    in_idx = 0;
    out_idx = 0;
    for ( spl = 0; spl < nbspl/2; spl++ )
    {
        in_byte = spl_in[in_idx];
        in_idx++;
        input = in_byte<<0x8;
        tmp = (unsigned short)in_byte>>0x8;
        input = input|tmp;
        spl_out[out_idx] = input;
        out_idx++;
    }
}
void Convert8bitTo16bit(U8 *pcmin,U8 *pcmout, U32 nbspl)
{

    int   spl;
    U16 * spl_out;
    U16   input;
    U8  * spl_in,in_byte;
    U32   in_idx   , out_idx;
    spl_in  = (U8 *)pcmin ;
    spl_out = (U16 *)pcmout ;
    in_idx = 0;
    out_idx = 0;
    for ( spl = 0; spl < nbspl; spl++ )
    {
        in_byte = spl_in[in_idx];
        in_idx  ++;
        input = (short)in_byte<<8;
        spl_out[out_idx] = input;
        out_idx ++;
        input=0;
    }
}
#endif

