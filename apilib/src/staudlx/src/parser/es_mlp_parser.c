/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_MLP_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_MLP
//#define  STTBX_PRINT
#include "es_mlp_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"
/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */
#define ES_MAX_ACCESS_UNIT_SIZE 1536 /* FROM FAQ: MLP Decoder Implementation Points */
#define ES_MIN_ACCESS_UNIT_SIZE 8
#define ES_ACCESS_UNIT_PER_FRAME 24

//static int count = 0;
static U32 MLP_DVD_AUDIO_Frequency[] =
{
    48000,  96000,  192000, 0, 0, 0, 0, 0, 44100, 82200, 176400, 0, 0, 0, 0, 0
};


/* Used for testing Purpose */


/* ----------------------------------------------------------------------------
        488                 External Data Types
--------------------------------------------------------------------------- */


/*---------------------------------------------------------------------
        835             Private Functions
----------------------------------------------------------------------*/


/************************************************************************************
Name         : ES_MLP_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_MLP_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_MLP_Parser_Init(ComParserInit_t * Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MLP_ParserLocalParams_t * MLPParserParams_p;
    MLPParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_MLP_ParserLocalParams_t));
    if(MLPParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(MLPParserParams_p,0,sizeof(ES_MLP_ParserLocalParams_t));
    /* Init the Local Parameters */

    MLPParserParams_p->MLPParserState = ES_MLP_START_FRAME;


    MLPParserParams_p->FrameComplete    = TRUE;
    MLPParserParams_p->First_Frame      = TRUE;
    MLPParserParams_p->Check_Major_Sync = TRUE;
    MLPParserParams_p->NbAccessUnits    = 0;

    MLPParserParams_p->Get_Block   = FALSE;
    MLPParserParams_p->Put_Block   = TRUE;
    MLPParserParams_p->OpBufHandle = Init_p->OpBufHandle;

    MLPParserParams_p->EvtHandle              = Init_p->EvtHandle;
    MLPParserParams_p->ObjectID               = Init_p->ObjectID;
    MLPParserParams_p->EventIDNewFrame        = Init_p->EventIDNewFrame;
    MLPParserParams_p->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;

    *handle=(ParserHandle_t )MLPParserParams_p;

    MLPParserParams_p->Blk_Out = 0;
    return (Error);
}

ST_ErrorCode_t ES_MLP_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_MLP_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_MLP_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_MLP_Get_Info;

    return ST_NO_ERROR;
}


/************************************************************************************
Name         : ES_MLP_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_MLP_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_MLP_ParserLocalParams_t * MLPParserParams_p;
    MLPParserParams_p=(ES_MLP_ParserLocalParams_t *)Handle;

    if(MLPParserParams_p->Get_Block == TRUE)
    {
        MLPParserParams_p->OpBlk_p->Size = MLPParserParams_p->FrameSize ;
        MLPParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(MLPParserParams_p->OpBufHandle,(U32 *)&MLPParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_MLP Error in Memory Put_Block \n"));
        }
        else
        {
            MLPParserParams_p->Put_Block = TRUE;
            MLPParserParams_p->Get_Block = FALSE;
        }
    }

    MLPParserParams_p->MLPParserState = ES_MLP_START_FRAME;


    MLPParserParams_p->FrameComplete    = TRUE;
    MLPParserParams_p->First_Frame      = TRUE;
    MLPParserParams_p->Check_Major_Sync = TRUE;
    MLPParserParams_p->NbAccessUnits    = 0;
    MLPParserParams_p->Blk_Out          = 0;
    return Error;
}

/************************************************************************************
Name         : ES_MLP_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_MLP_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_MLP_ParserLocalParams_t * MLPParserParams_p;
    MLPParserParams_p=(ES_MLP_ParserLocalParams_t *)Handle;


    MLPParserParams_p->MLPParserState   = ES_MLP_START_FRAME;
    MLPParserParams_p->FrameComplete    = TRUE;
    MLPParserParams_p->First_Frame      = TRUE;
    MLPParserParams_p->Check_Major_Sync = TRUE;
    MLPParserParams_p->NbAccessUnits    = 0;
    MLPParserParams_p->Put_Block        = TRUE;
    MLPParserParams_p->Get_Block        = FALSE;

    MLPParserParams_p->Blk_Out = 0;
    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_MLP_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_MLP_ParserLocalParams_t * MLPParserParams_p;
    MLPParserParams_p=(ES_MLP_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p = MLPParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_MLP_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_MLP_ParserLocalParams_t *MLPParserParams_p;
    MLPParserParams_p=(ES_MLP_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency = MLPParserParams_p->Frequency;
    return ST_NO_ERROR;
}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_MLP_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_MLP_ParserLocalParams_t * MLPParserParams_p = (ES_MLP_ParserLocalParams_t *)Handle;

    if(MLPParserParams_p->Get_Block == TRUE)
    {
        memset(MLPParserParams_p->Current_Write_Ptr1,0x00,MLPParserParams_p->NoBytesRemainingInFrame);

        MLPParserParams_p->OpBlk_p->Size = MLPParserParams_p->FrameSize;
        MLPParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        MLPParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(MLPParserParams_p->OpBufHandle,(U32 *)&MLPParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            MLPParserParams_p->Put_Block = TRUE;
            MLPParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(MLPParserParams_p->OpBufHandle, (U32 *)&MLPParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            MLPParserParams_p->Put_Block = FALSE;
            MLPParserParams_p->Get_Block = TRUE;
            MLPParserParams_p->OpBlk_p->Size = 9216;
            MLPParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)MLPParserParams_p->OpBlk_p->MemoryStart,0x00,MLPParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(MLPParserParams_p->OpBufHandle,(U32 *)&MLPParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
            }
            else
            {
                MLPParserParams_p->Put_Block = TRUE;
                MLPParserParams_p->Get_Block = FALSE;
            }
        }
    }

    return ST_NO_ERROR;
}



/************************************************************************************
Name         : Parse_MLP_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_MLP_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info)
{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    ES_MLP_ParserLocalParams_t * MLPParserParams_p;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;


    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_MLP Parser Enrty \n"));

    MLPParserParams_p=(ES_MLP_ParserLocalParams_t *)Handle;


    while(offset<Size)
    {
        if(MLPParserParams_p->FrameComplete == TRUE)
        {
            if(MLPParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(MLPParserParams_p->OpBufHandle, (U32 *)&MLPParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    MLPParserParams_p->Put_Block = FALSE;
                    MLPParserParams_p->Get_Block = TRUE;
                    MLPParserParams_p->Current_Write_Ptr1 = (U8 *)MLPParserParams_p->OpBlk_p->MemoryStart;
                    //memset(MLPParserParams_p->Current_Write_Ptr1,0x00,MLPParserParams_p->OpBlk_p->MaxSize);
                    MLPParserParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {
                        MLPParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        MLPParserParams_p->OpBlk_p->Data[PTS_OFFSET]     = CurrentPTS_p->PTS;
                        MLPParserParams_p->OpBlk_p->Data[PTS33_OFFSET]   = CurrentPTS_p->PTS33Bit;

                    }

                    if(Private_Info->Private_Data[2] == 1) // FADE PAN VALID FLAG
                    {
                        MLPParserParams_p->OpBlk_p->Flags   |= FADE_VALID;
                        MLPParserParams_p->OpBlk_p->Flags   |= PAN_VALID;
                        MLPParserParams_p->OpBlk_p->Data[FADE_OFFSET]    = (Private_Info->Private_Data[5]<<8)|(Private_Info->Private_Data[0]); // FADE Value
                        MLPParserParams_p->OpBlk_p->Data[PAN_OFFSET] = (Private_Info->Private_Data[6]<<8)|(Private_Info->Private_Data[1]); // PAN Value
                    }
                }
            }

            CurrentPTS_p->PTSValidFlag      = FALSE;
            MLPParserParams_p->FrameComplete = FALSE;
            MLPParserParams_p->FrameSize = 0;
            MLPParserParams_p->SerialAccessUnits = 0;
        }

        value=pos[offset];
        switch(MLPParserParams_p->MLPParserState)
        {

        case ES_MLP_START_FRAME:
            MLPParserParams_p->Current_Write_Ptr1 = (U8 *)MLPParserParams_p->OpBlk_p->MemoryStart;
            MLPParserParams_p->SerialAccessUnits = 0;
            MLPParserParams_p->FrameSize = 0;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_1;
            break;

        case ES_MLP_BYTE_1:
            offset++;
            MLPParserParams_p->Data[0] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_2;
            break;

        case ES_MLP_BYTE_2:
            offset++;
            MLPParserParams_p->Data[1] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_3;
            break;

        case ES_MLP_BYTE_3:
            offset++;
            MLPParserParams_p->Data[2] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_4;
            break;

        case ES_MLP_BYTE_4:
            offset++;
            MLPParserParams_p->Data[3] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_5;
            break;

        case ES_MLP_BYTE_5:
            offset++;
            MLPParserParams_p->Data[4] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_6;
            break;

        case ES_MLP_BYTE_6:
            offset++;
            MLPParserParams_p->Data[5] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_7;
            break;

        case ES_MLP_BYTE_7:
            offset++;
            MLPParserParams_p->Data[6] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_BYTE_8;
            break;

        case ES_MLP_BYTE_8:
            offset++;
            MLPParserParams_p->Data[7] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_CHECK_SYNC;
            MLPParserParams_p->NoBytesCopied = 8;
            break;

        case ES_MLP_CHECK_SYNC:
            MLPParserParams_p->Access_Unit_Size = (((MLPParserParams_p->Data[0] & 0x0F)<<8)|MLPParserParams_p->Data[1]) * 2;
            //printf("Access_Unit_Size[%d]= %d\n",count++,MLPParserParams_p->Access_Unit_Size);
            if((MLPParserParams_p->Access_Unit_Size<ES_MIN_ACCESS_UNIT_SIZE)||(MLPParserParams_p->Access_Unit_Size>ES_MAX_ACCESS_UNIT_SIZE))
            {
                STTBX_Print((" ES_MLP Access_Unit_Size ERROR \n"));
                MLPParserParams_p->Check_Major_Sync = TRUE;
            }

            MLPParserParams_p->MLPParserState = ES_MLP_WRITE_SYNC;

            if(((MLPParserParams_p->Data[4]&0xF0) == 0xF0) || (MLPParserParams_p->First_Frame == TRUE) || (MLPParserParams_p->Check_Major_Sync == TRUE) || (MLPParserParams_p->Check_Major_Sync == TRUE) || (MLPParserParams_p->NbAccessUnits >= 32))
            {
            /* In case of Major Sync */
            /* Check for sync */
                if(((MLPParserParams_p->Data[4]) != 0xF8) || ((MLPParserParams_p->Data[5]) != 0x72) || ((MLPParserParams_p->Data[6]) != 0x6F))
                {
                    MLPParserParams_p->Data[0] = MLPParserParams_p->Data[1];
                    MLPParserParams_p->Data[1] = MLPParserParams_p->Data[2];
                    MLPParserParams_p->Data[2] = MLPParserParams_p->Data[3];
                    MLPParserParams_p->Data[3] = MLPParserParams_p->Data[4];
                    MLPParserParams_p->Data[4] = MLPParserParams_p->Data[5];
                    MLPParserParams_p->Data[5] = MLPParserParams_p->Data[6];
                    MLPParserParams_p->Data[6] = MLPParserParams_p->Data[7];
                    MLPParserParams_p->Data[7] = value;
                    offset++;
                    MLPParserParams_p->MLPParserState = ES_MLP_CHECK_SYNC;
                }
                else
                {
                    //printf("Access_Unit= %d\n",MLPParserParams_p->SerialAccessUnits);
                    //printf("Nb Access_Unit= %d\n",MLPParserParams_p->NbAccessUnits);
                    if((MLPParserParams_p->NbAccessUnits != 0)&&(MLPParserParams_p->NbAccessUnits < 8))
                    {
                        STTBX_Print((" ES_MLP Major Sync Found before 8 aceess unit \n"));
                    }

                    MLPParserParams_p->Check_Major_Sync = FALSE;
                    //MLPParserParams_p->First_Frame = FALSE;
                    //MLPParserParams_p->SerialAccessUnits = 0;
                    //MLPParserParams_p->FrameSize = 0;
                    MLPParserParams_p->NbAccessUnits = 0;
                    MLPParserParams_p->MLPParserState = ES_MLP_MAJOR_SYNC_INFO_0;
                }
            }

            break;

        case ES_MLP_MAJOR_SYNC_INFO_0:
            offset++;
            MLPParserParams_p->Data[8] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_MAJOR_SYNC_INFO_1;
            break;

        case ES_MLP_MAJOR_SYNC_INFO_1:
            offset++;
            MLPParserParams_p->Data[9] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_MAJOR_SYNC_INFO_2;
            break;

        case ES_MLP_MAJOR_SYNC_INFO_2:
            offset++;
            MLPParserParams_p->Data[10] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_MAJOR_SYNC_INFO_3;
            break;

        case ES_MLP_MAJOR_SYNC_INFO_3:
            offset++;
            MLPParserParams_p->Data[11] = value;
            MLPParserParams_p->MLPParserState = ES_MLP_MAJOR_SYNC_INFO;
            break;

        case ES_MLP_MAJOR_SYNC_INFO:
            {

                //U8  ch_assign       = (MLPParserParams_p->Data[11] & 0x1F);       5 bits
                //U8  sfreq2          = (MLPParserParams_p->Data[9]  & 0x0F);       4 bits
                U8  sfreq1          = (MLPParserParams_p->Data[9]  >>   4);      /* 4 bits */
                //U8  ws2             = (MLPParserParams_p->Data[8]  & 0x0F);          4 bits
                //U8  ws1             = (MLPParserParams_p->Data[8]  >>   4);          4 bits

                //MLPParserParams_p->Frequency             = MLP_DVD_AUDIO_Frequency[sfreq1];

                if(MLPParserParams_p->First_Frame)
                {
                    MLPParserParams_p->First_Frame = FALSE;
                    MLPParserParams_p->Frequency = MLP_DVD_AUDIO_Frequency[sfreq1];
                    MLPParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                    MLPParserParams_p->OpBlk_p->Data[FREQ_OFFSET]    = MLP_DVD_AUDIO_Frequency[sfreq1];
                    Error=STAudEVT_Notify(MLPParserParams_p->EvtHandle,MLPParserParams_p->EventIDFrequencyChange, &MLPParserParams_p->Frequency, sizeof(MLPParserParams_p->Frequency), MLPParserParams_p->ObjectID);
                }
                else
                {
                    if(MLPParserParams_p->Frequency != MLP_DVD_AUDIO_Frequency[sfreq1])
                    {
                        MLPParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                        MLPParserParams_p->OpBlk_p->Data[FREQ_OFFSET]    = MLP_DVD_AUDIO_Frequency[sfreq1];
                        MLPParserParams_p->Frequency = MLP_DVD_AUDIO_Frequency[sfreq1];
                        Error=STAudEVT_Notify(MLPParserParams_p->EvtHandle,MLPParserParams_p->EventIDFrequencyChange, &MLPParserParams_p->Frequency, sizeof(MLPParserParams_p->Frequency), MLPParserParams_p->ObjectID);
                    }

                }
            }
            MLPParserParams_p->NoBytesCopied = 12;
            MLPParserParams_p->MLPParserState = ES_MLP_WRITE_SYNC;
            break;

        case ES_MLP_WRITE_SYNC:

        {
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[0];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[1];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[2];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[3];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[4];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[5];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[6];
            *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[7];

            if(MLPParserParams_p->NoBytesCopied == 12)
            {
                *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[8];
                *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[9];
                *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[10];
                *MLPParserParams_p->Current_Write_Ptr1++ = MLPParserParams_p->Data[11];
            }

            MLPParserParams_p->MLPParserState = ES_MLP_FRAME;
        }
        break;

        case ES_MLP_FRAME:
            if((Size-offset+MLPParserParams_p->NoBytesCopied)>=MLPParserParams_p->Access_Unit_Size)
            {
                /* 1 or more Access_Unit */
                MLPParserParams_p->MLPParserState = ES_MLP_BYTE_1;

                memcpy(MLPParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(MLPParserParams_p->Access_Unit_Size - MLPParserParams_p->NoBytesCopied));

                MLPParserParams_p->FrameSize += MLPParserParams_p->Access_Unit_Size;
                MLPParserParams_p->SerialAccessUnits++;
                MLPParserParams_p->NbAccessUnits++;

                if(MLPParserParams_p->SerialAccessUnits == ES_ACCESS_UNIT_PER_FRAME)
                {
                    MLPParserParams_p->FrameComplete = TRUE;
                    MLPParserParams_p->OpBlk_p->Size = MLPParserParams_p->FrameSize;

                    if(MLPParserParams_p->Get_Block == TRUE)
                    {
                        if(MLPParserParams_p->OpBlk_p->Flags & PTS_VALID)
                        {
                            /* Get the PTS in the correct structure */
                            NotifyPTS.Low  = MLPParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                            NotifyPTS.High = MLPParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                        }
                        else
                        {
                            NotifyPTS.Low  = 0;
                            NotifyPTS.High = 0;
                        }

                        NotifyPTS.Interpolated  = FALSE;

                        Error = MemPool_PutOpBlk(MLPParserParams_p->OpBufHandle,(U32 *)&MLPParserParams_p->OpBlk_p);

                        if(Error != ST_NO_ERROR)
                        {
                            STTBX_Print((" ES_MLP Error in Memory Put_Block \n"));
                            return Error;
                        }
                        else
                        {
                            STTBX_Print((" MLP Deli:%d\n",MLPParserParams_p->Blk_Out));
                            MLPParserParams_p->Blk_Out++;

                            Error = STAudEVT_Notify(MLPParserParams_p->EvtHandle,MLPParserParams_p->EventIDNewFrame, &NotifyPTS,  sizeof(NotifyPTS), MLPParserParams_p->ObjectID);
                            MLPParserParams_p->Put_Block = TRUE;
                            MLPParserParams_p->Get_Block = FALSE;
                        }
                    }

                    MLPParserParams_p->MLPParserState = ES_MLP_START_FRAME;

                }

                MLPParserParams_p->Current_Write_Ptr1 += (MLPParserParams_p->Access_Unit_Size - MLPParserParams_p->NoBytesCopied);
                offset =  offset-MLPParserParams_p->NoBytesCopied+MLPParserParams_p->Access_Unit_Size;

            }
            else
            {
                /* less than one Access_Unit */
                MLPParserParams_p->NoBytesRemainingInFrame = MLPParserParams_p->Access_Unit_Size  - (Size-offset+MLPParserParams_p->NoBytesCopied);
                MLPParserParams_p->FrameComplete = FALSE;

                MLPParserParams_p->FrameSize += MLPParserParams_p->Access_Unit_Size;
                MLPParserParams_p->SerialAccessUnits++;
                MLPParserParams_p->NbAccessUnits++;

                memcpy(MLPParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                MLPParserParams_p->Current_Write_Ptr1 += (Size - offset);
                offset =  Size;
                MLPParserParams_p->NoBytesCopied = 0;
                MLPParserParams_p->MLPParserState = ES_MLP_PARTIAL_FRAME;
            }
            break;

        case ES_MLP_PARTIAL_FRAME:


            if(Size-offset>=MLPParserParams_p->NoBytesRemainingInFrame)
            {
                /* one Access_Unit */

                MLPParserParams_p->MLPParserState = ES_MLP_BYTE_1;
                memcpy(MLPParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),MLPParserParams_p->NoBytesRemainingInFrame);

                if(MLPParserParams_p->SerialAccessUnits == ES_ACCESS_UNIT_PER_FRAME)
                {
                    MLPParserParams_p->FrameComplete = TRUE;
                    MLPParserParams_p->OpBlk_p->Size = MLPParserParams_p->FrameSize;

                    if(MLPParserParams_p->Get_Block == TRUE)
                    {
                        if(MLPParserParams_p->OpBlk_p->Flags & PTS_VALID)
                        {
                            /* Get the PTS in the correct structure */
                            NotifyPTS.Low  = MLPParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                            NotifyPTS.High = MLPParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                        }
                        else
                        {
                            NotifyPTS.Low  = 0;
                            NotifyPTS.High = 0;
                        }

                        NotifyPTS.Interpolated  = FALSE;

                        Error = MemPool_PutOpBlk(MLPParserParams_p->OpBufHandle,(U32 *)&MLPParserParams_p->OpBlk_p);

                        if(Error != ST_NO_ERROR)
                        {
                            STTBX_Print((" ES_MLP Error in Memory Put_Block \n"));
                            return Error;
                        }
                        else
                        {
                            Error = STAudEVT_Notify(MLPParserParams_p->EvtHandle,MLPParserParams_p->EventIDNewFrame, &NotifyPTS,  sizeof(NotifyPTS), MLPParserParams_p->ObjectID);
                            MLPParserParams_p->Put_Block = TRUE;
                            MLPParserParams_p->Get_Block = FALSE;
                        }
                    }
                    MLPParserParams_p->MLPParserState = ES_MLP_START_FRAME;
                }

                MLPParserParams_p->Current_Write_Ptr1 += MLPParserParams_p->NoBytesRemainingInFrame;

                offset =  offset+MLPParserParams_p->NoBytesRemainingInFrame;

            }

            else
            {
                /* again less than a frame */
                MLPParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                MLPParserParams_p->FrameComplete = FALSE;
                memcpy(MLPParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                MLPParserParams_p->Current_Write_Ptr1 += Size-offset;

                MLPParserParams_p->MLPParserState = ES_MLP_PARTIAL_FRAME;

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

ST_ErrorCode_t ES_MLP_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p)
{

    ES_MLP_ParserLocalParams_t * MLPParserParams_p = (ES_MLP_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, MLPParserParams_p);
    }

    return ST_NO_ERROR;

}
#endif

