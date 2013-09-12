/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_DTS_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_DTS

#include "es_dts_parser.h"

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

static U32 DTSFrequency[] =
{
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0, 12000, 24000, 48000, 0, 0
};

/* Used for testing Purpose */


/* ----------------------------------------------------------------------------
        488                 External Data Types
--------------------------------------------------------------------------- */


/*---------------------------------------------------------------------
        835             Private Functions
----------------------------------------------------------------------*/


/*****************************************************************************
 * Function     : DTSCheckSyncWord
 *                Validates a Sync Word & sets bitstream type in DTSParserLocalParams
 * Input        : DTSParserLocalParams
 * Output       : Returns Sync Status
 *****************************************************************************/

BOOL DTSCheckSyncWord(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    U32 nWord = 0,nWord1=0,i;
    U32 nTmpWord, m = 0xffff00ff;
    for(i=0;i<4;i++)
    {
        nWord = ((nWord<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    }
    if(DTSParserParams_p->First_Frame)
    {
        if(nWord == 0x41555052)
        {
            for(i=0;i<4;i++)
            {
                nWord1 = ((nWord1<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
            }
            if(nWord1 == 0x2d484452)
            {
                DTSParserParams_p->TempBuff.ReadPos -= 4;
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                DTSParserParams_p->AUPR_HDR = TRUE;
                return TRUE;
            }
            else
            {
                DTSParserParams_p->TempBuff.Size -= 3;
                DTSParserParams_p->TempBuff.ReadPos--;
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                return FALSE;
            }
        }
    }


    if ( nWord == DTS_SYNCWORD_CORE_16 )
    {
        // 16 bit core stream
        DTSParserParams_p->BitStreamType = DTS_BITSTREAM_LOSSY_16;
        return TRUE;
    }
    if ( nWord == DTS_SYNCWORD_CORE_14 )
    {
        // 14 bit core stream
        DTSParserParams_p->BitStreamType = DTS_BITSTREAM_LOSSY_14;
        return TRUE;
    }

    nTmpWord = nWord & m;

    if ( nTmpWord == DTS_SYNCWORD_CORE_24 )
    {
        // 24 bit core stream
        if((DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos]) == 01)
        {
            DTSParserParams_p->BitStreamType = DTS_BITSTREAM_LOSSY_24;
            return TRUE;
        }
    }
    if (nWord == DTS_SYNCWORD_SUBSTREAM)
    {
        DTSParserParams_p->BitStreamType = DTS_BITSTREAM_SUBSTREAM_32;
        return TRUE;
    }
    return FALSE;
}


/*****************************************************************************
 * Function     : DTS_READ_NEXT_WORD32
 *                Read 32 bit Word from temp buffer of DTSParserParams_p
 * Input        : DTSParserParams_p
 * Output       : Returns 32 bit word
 *****************************************************************************/

U32 DTS_READ_NEXT_WORD32(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    U32 nWord = 0,i;
    for(i=0;i<4;i++)
    {
        nWord = ((nWord<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    }
    DTSParserParams_p->TempBuff.ReadPos -= 4;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    return nWord;
}

/*****************************************************************************
 * Function     : DTS_READ_NEXT_WORD32_64
 *                Read second 32 bit Word from temp buffer of DTSParserParams_p
 * Input        : DTSParserParams_p
 * Output       : Returns 32 bit word
 *****************************************************************************/

U32 DTS_READ_NEXT_WORD32_64(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    U32 nWord = 0,i;
    DTSParserParams_p->TempBuff.ReadPos +=4;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    for(i=0;i<4;i++)
    {
        nWord = ((nWord<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    }
    DTSParserParams_p->TempBuff.ReadPos -= 8;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    return nWord;
}

/*****************************************************************************
 * Function     : DTS_READ_NEXT_WORD64_96
 *                Read third 32 bit Word from temp buffer of DTSParserParams_p
 * Input        : DTSParserParams_p
 * Output       : Returns 32 bit word
 *****************************************************************************/

U32 DTS_READ_NEXT_WORD64_96(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    U32 nWord = 0,i;
    DTSParserParams_p->TempBuff.ReadPos +=8;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    for(i=0;i<4;i++)
    {
        nWord = ((nWord<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    }
    DTSParserParams_p->TempBuff.ReadPos -= 12;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    return nWord;
}


/*****************************************************************************
 * Function     : DTS_READ_NEXT_WORD96_128
 *                Read third 32 bit Word from temp buffer of DTSParserParams_p
 * Input        : DTSParserParams_p
 * Output       : Returns 32 bit word
 *****************************************************************************/

U32 DTS_READ_NEXT_WORD96_128(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    U32 nWord = 0,i;
    DTSParserParams_p->TempBuff.ReadPos +=12;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    for(i=0;i<4;i++)
    {
        nWord = ((nWord<<8)|(DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++]));
        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    }
    DTSParserParams_p->TempBuff.ReadPos -= 16;
    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
    return nWord;
}

/*****************************************************************************
 * Function     : ReadSubStreamHeader                                        *
 *                Extracts Substream Header from stream to char buffer       *
 * Input        : DTSParserParams_p      - "ES_DTS_ParserLocalParams_t"   *
 * Output       : Header Size                                                            *
 *****************************************************************************/
BOOL ReadSubStreamHeader(ES_DTS_ParserLocalParams_t * DTSParserParams_p)
{
    //int          i;
    unsigned int SyncWord = 0, nHeaderSizeType, nWord, cWord;

    SyncWord = DTS_READ_NEXT_WORD32(DTSParserParams_p);

    if (SyncWord != 0x64582025)
    {
        return FALSE; //DTS-HD Substream Sync word Not found
    }

    nWord = DTS_READ_NEXT_WORD32_64(DTSParserParams_p);

    DTSParserParams_p->SubStreamIndex = (nWord << 8) >> 30;

    nHeaderSizeType  = (nWord << 10) >> 31;

/*  if(nHeaderSizeType == 1)
    {
        DTSParserParams_p->SubStreamHeaderSize = ((nWord << 11) >> 20) + 1;
    }
    else
    {
        DTSParserParams_p->SubStreamHeaderSize = ((nWord << 11) >> 24) + 1;
    }*/

    if(nHeaderSizeType == 1)
    {
        // 11 more bits to read.
        nWord       = (nWord << 23) >> 23;
        nWord       = (nWord << 11);
        cWord       = DTS_READ_NEXT_WORD64_96(DTSParserParams_p) >> 21 ;
        DTSParserParams_p->SubStreamSize  = (nWord | cWord) + 1;
    }
    else
    {
        //unsigned int cWord;

        // 3 more bits to read.
        nWord       = (nWord << 19) >> 19;
        nWord       = (nWord << 3);
        cWord       = DTS_READ_NEXT_WORD64_96(DTSParserParams_p) >> 29;
        DTSParserParams_p->SubStreamSize       = (nWord | cWord) + 1;
    }
    return TRUE;
}


/************************************************************************************
Name         : ES_DTS_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_DTS_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_DTS_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    STAud_DtsStreamParams_t *AppData_p;
    DTSParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_DTS_ParserLocalParams_t));
    if(DTSParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(DTSParserParams_p,0,sizeof(ES_DTS_ParserLocalParams_t));

    AppData_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(STAud_DtsStreamParams_t));
    if(AppData_p == NULL)
    {
        /*deallocate the allocated memory*/
        STOS_MemoryDeallocate(Init_p->Memory_Partition, DTSParserParams_p);
        return ST_ERROR_NO_MEMORY;
    }
    memset(AppData_p,0,sizeof(STAud_DtsStreamParams_t));

    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;


    DTSParserParams_p->FrameComplete       = TRUE;
    DTSParserParams_p->First_Frame         = TRUE;
    DTSParserParams_p->Frequency           = 0xFFFFFFFF;
    DTSParserParams_p->AppData_p           = AppData_p;
    DTSParserParams_p->FirstByteEncSamples = 0;
    DTSParserParams_p->Last4ByteEncSamples = 0;
    DTSParserParams_p->DelayLossLess       = 0;
    DTSParserParams_p->AppData_p->CoreSize = 0;
    DTSParserParams_p->Get_Block           = FALSE;
    DTSParserParams_p->Put_Block           = TRUE;
    DTSParserParams_p->OpBufHandle         = Init_p->OpBufHandle;
    DTSParserParams_p->EvtHandle              = Init_p->EvtHandle;
    DTSParserParams_p->ObjectID               = Init_p->ObjectID;
    DTSParserParams_p->EventIDNewFrame        = Init_p->EventIDNewFrame;
    DTSParserParams_p->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;

    *handle=(ParserHandle_t )DTSParserParams_p;

    return (Error);
}

ST_ErrorCode_t ES_DTS_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                 GetFreqFunction_t * GetFreqFunction_p,
                                                 GetInfoFunction_t * GetInfoFunction_p)


{

    *ParsingFunc_p      = (ParsingFunction_t)ES_DTS_Parse_Frame;
    *GetFreqFunction_p= (GetFreqFunction_t)ES_DTS_Get_Freq;
    *GetInfoFunction_p= (GetInfoFunction_t)ES_DTS_Get_Info;

    return ST_NO_ERROR;
}



/************************************************************************************
Name         : ES_DTS_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_DTS_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    DTSParserParams_p=(ES_DTS_ParserLocalParams_t *)Handle;

    if(DTSParserParams_p->Get_Block == TRUE)
    {
        DTSParserParams_p->OpBlk_p->Size = DTSParserParams_p->FrameSize ;
        DTSParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(DTSParserParams_p->OpBufHandle,(U32 *)&DTSParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_DTS Error in Memory Put_Block \n"));
        }
        else
        {
            DTSParserParams_p->Put_Block = TRUE;
            DTSParserParams_p->Get_Block = FALSE;
        }
    }


    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;


    DTSParserParams_p->FrameComplete = TRUE;
    DTSParserParams_p->First_Frame   = TRUE;

    return Error;
}

/************************************************************************************
Name         : ES_DTS_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_DTS_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    DTSParserParams_p=(ES_DTS_ParserLocalParams_t *)Handle;

    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;


    DTSParserParams_p->FrameComplete = TRUE;
    DTSParserParams_p->First_Frame   = TRUE;
    DTSParserParams_p->FirstByteEncSamples = 0;
    DTSParserParams_p->Last4ByteEncSamples = 0;
    DTSParserParams_p->DelayLossLess = 0;

    DTSParserParams_p->Put_Block = TRUE;
    DTSParserParams_p->Get_Block = FALSE;

    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_DTS_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    DTSParserParams_p=(ES_DTS_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=DTSParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_DTS_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    DTSParserParams_p=(ES_DTS_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = DTSParserParams_p->Frequency;

    return ST_NO_ERROR;
}


/* Handle EOF from Parser */

ST_ErrorCode_t ES_DTS_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_DTS_ParserLocalParams_t *DTSParserParams_p = (ES_DTS_ParserLocalParams_t *)Handle;

    if(DTSParserParams_p->Get_Block == TRUE)
    {
        memset(DTSParserParams_p->Current_Write_Ptr1,0x00,DTSParserParams_p->NoBytesRemainingInFrame);

        DTSParserParams_p->OpBlk_p->Size = DTSParserParams_p->FrameSize;
        DTSParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        DTSParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(DTSParserParams_p->OpBufHandle,(U32 *)&DTSParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            DTSParserParams_p->Put_Block = TRUE;
            DTSParserParams_p->Get_Block = FALSE;
        }
    }

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : Parse_DTS_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_DTS_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info)
{
    U8  *pos;
    U32 offset;
    U8  value,i;
    ST_ErrorCode_t Error;
    ES_DTS_ParserLocalParams_t *DTSParserParams_p;
    STAUD_PTS_t NotifyPTS;
    U32 nWord = 0,nSyncExt = 0,SizeToWrite;
    BOOL SyncResult,bResult;
    U8 samp_freq_code=0;
    U8 TempBuffSize;

    offset=*No_Bytes_Used;
    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_DTS Parser Enrty \n"));

    DTSParserParams_p=(ES_DTS_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {

        if(DTSParserParams_p->FrameComplete == TRUE)
        {
            if(DTSParserParams_p->Put_Block == TRUE)
            {
                Error = MemPool_GetOpBlk(DTSParserParams_p->OpBufHandle, (U32 *)&DTSParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    *No_Bytes_Used = offset;
                    return STAUD_MEMORY_GET_ERROR;
                }
                else
                {
                    DTSParserParams_p->Put_Block = FALSE;
                    DTSParserParams_p->Get_Block = TRUE;
                    DTSParserParams_p->Current_Write_Ptr1 = (U8 *)DTSParserParams_p->OpBlk_p->MemoryStart;
                    //memset(DTSParserParams_p->Current_Write_Ptr1,0x00,DTSParserParams_p->OpBlk_p->MaxSize);
                    DTSParserParams_p->OpBlk_p->Flags = 0;
                    if(CurrentPTS_p->PTSValidFlag == TRUE)
                    {
                        DTSParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                        DTSParserParams_p->OpBlk_p->Data[PTS_OFFSET] = CurrentPTS_p->PTS;
                        DTSParserParams_p->OpBlk_p->Data[PTS33_OFFSET] = CurrentPTS_p->PTS33Bit;
                    }

                    if(Private_Info->Private_Data[2] == 1) // FADE PAN VALID FLAG
                    {
                        DTSParserParams_p->OpBlk_p->Flags   |= FADE_VALID;
                        DTSParserParams_p->OpBlk_p->Flags   |= PAN_VALID;
                        DTSParserParams_p->OpBlk_p->Data[FADE_OFFSET]    = (Private_Info->Private_Data[5]<<8)|(Private_Info->Private_Data[0]); // FADE Value
                        DTSParserParams_p->OpBlk_p->Data[PAN_OFFSET] = (Private_Info->Private_Data[6]<<8)|(Private_Info->Private_Data[1]); // PAN Value
                    }


                    DTSParserParams_p->OpBlk_p->Flags   |= DATAFORMAT_VALID;
                    DTSParserParams_p->OpBlk_p->Data[DATAFORMAT_OFFSET]  = BE;
                }
            }

            CurrentPTS_p->PTSValidFlag      = FALSE;
            DTSParserParams_p->FrameComplete = FALSE;
        }

        value=pos[offset];
        switch(DTSParserParams_p->DTSParserState)
        {
        case ES_DTS_SYNC_0:
#ifdef PARSE_LOSS_LESS_HEADER
            if(DTSParserParams_p->First_Frame)
            {
                if(value == "C")
                {
                    DTSParserParams_p->DTSParserState = ES_DTS_LOSS_LESS_HEADER_1;
                    DTSParserParams_p->First_Frame = FALSE;
                    offset++;
                    break;
                }
            }
#endif
            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {
                SyncResult = DTSCheckSyncWord(DTSParserParams_p);
                if(SyncResult == TRUE)
                {
                    DTSParserParams_p->TempBuff.ReadPos -= 4;
                    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;

                    DTSParserParams_p->First_Frame = FALSE;

                    if(DTSParserParams_p->AUPR_HDR)
                    {
                        DTSParserParams_p->AUPR_HDR = FALSE;
                        DTSParserParams_p->nByteSkip = 28;
                        DTSParserParams_p->DTSParserState = ES_DTS_AUPR_HDR;
                    }
                    else if(DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_16 || DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_14 )
                    {
                        DTSParserParams_p->DTSParserState = ES_DTS_NEXT_TBD;
                    }
                    else if  ( DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_24 )
                    {
                        DTSParserParams_p->DTSParserState = ES_DTS_NEXT_TBD_1;
                        // TBD : No Streams available.
                    }
                    else if ( DTSParserParams_p->BitStreamType & DTS_BITSTREAM_HDMI_64 )
                    {
                        DTSParserParams_p->DTSParserState = ES_DTS_NEXT_TBD_2;
                        // HDMI stream
                        // TBD : No Streams available.
                    }   // Bit stream type HDMI.
                    else
                    {
                        DTSParserParams_p->DTSParserState = ES_DTS_CHECK_SYNC_CASE2;
                    }
                }
                else
                {
                    DTSParserParams_p->TempBuff.ReadPos -= 3;
                    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                    DTSParserParams_p->TempBuff.Size--;
                }
                DTSParserParams_p->SubStreamIndex = 4;
                DTSParserParams_p->nSubStreamSize = 0;
            }
            break;

        case ES_DTS_AUPR_HDR:

            if(DTSParserParams_p->TempBuff.Size)
            {
                DTSParserParams_p->nByteSkip -= DTSParserParams_p->TempBuff.Size;
                DTSParserParams_p->TempBuff.Size = 0;
            }
            if(DTSParserParams_p->nByteSkip)
            {
                DTSParserParams_p->nByteSkip--;
                offset++;
            }
            else
            {
                DTSParserParams_p->DTSParserState = ES_DTS_AUPR_HDR1;
            }
            break;

        case ES_DTS_AUPR_HDR1:

            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {
                DTSParserParams_p->AppData_p->FirstByteEncSamples = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                DTSParserParams_p->AppData_p->Last4ByteEncSamples = DTS_READ_NEXT_WORD32(DTSParserParams_p);
                DTSParserParams_p->TempBuff.ReadPos += 6;
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                DTSParserParams_p->AppData_p->DelayLossLess = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                DTSParserParams_p->AppData_p->DelayLossLess = (DTSParserParams_p->AppData_p->DelayLossLess << 8);
                DTSParserParams_p->AppData_p->DelayLossLess += DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                DTSParserParams_p->TempBuff.Size -= 9;
                DTSParserParams_p->AppData_p->ChangeSet = TRUE;
                DTSParserParams_p->OpBlk_p->AppData_p = DTSParserParams_p->AppData_p;
                DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;

                DTSParserParams_p->FirstByteEncSamples = DTSParserParams_p->AppData_p->FirstByteEncSamples;
                DTSParserParams_p->Last4ByteEncSamples = DTSParserParams_p->AppData_p->Last4ByteEncSamples;
                DTSParserParams_p->DelayLossLess = DTSParserParams_p->AppData_p->DelayLossLess;

            }
            break;
        case ES_DTS_NEXT_TBD:

            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {

                DTSParserParams_p->CoreBytes         = 0;
                DTSParserParams_p->InSync            = FALSE;
                DTSParserParams_p->SyncWord          = (DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_16) ?
                                                          DTS_SYNCWORD_CORE_16 : DTS_SYNCWORD_CORE_14;
                DTSParserParams_p->Current_Write_Ptr1 = (U8 *)DTSParserParams_p->OpBlk_p->MemoryStart;
                // Store sync word in the Frame buffer.
                for(i = 0; i < 4; i++)
                {
                    *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                    DTSParserParams_p->TempBuff.Size--;
                }
                DTSParserParams_p->CoreBytes += 4;

                //TBD for sampling freq extract

                if(DTSParserParams_p->BitStreamType == DTS_BITSTREAM_LOSSY_14)
                {
                    samp_freq_code = (DTSParserParams_p->TempBuff.Buff[(DTSParserParams_p->TempBuff.ReadPos + 5)&0xF] & 0xF);
                }
                else if(DTSParserParams_p->BitStreamType == DTS_BITSTREAM_LOSSY_16)
                {
                    samp_freq_code = (DTSParserParams_p->TempBuff.Buff[(DTSParserParams_p->TempBuff.ReadPos + 4)&0xF] & 0x3C) >> 2;
                }
                else
                {
                    //TBD
                }

                if(DTSFrequency[samp_freq_code] == 0)
                {
                    STTBX_Print((" ES_DTS Parser Invalid frequency restart sync search \n"));
                    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                }
                else
                {
                    if(DTSParserParams_p->Frequency != DTSFrequency[samp_freq_code])
                    {
                        DTSParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                        DTSParserParams_p->OpBlk_p->Data[FREQ_OFFSET]    = DTSFrequency[samp_freq_code];
                        DTSParserParams_p->Frequency = DTSFrequency[samp_freq_code];
                        Error=STAudEVT_Notify(DTSParserParams_p->EvtHandle,DTSParserParams_p->EventIDFrequencyChange, &DTSParserParams_p->Frequency, sizeof(DTSParserParams_p->Frequency), DTSParserParams_p->ObjectID);

                    }

                }
                // Start LookAhead !!
                DTSParserParams_p->DTSParserState         = ES_DTS_NEXT1_TBD;
            }
            break;

        case ES_DTS_NEXT1_TBD:
            // Start LookAhead !!
            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {
                nWord = DTS_READ_NEXT_WORD32(DTSParserParams_p);

                if ( nWord == DTSParserParams_p->SyncWord )
                {
                    // If another lossy sync word is encountered CASE 1.
                    if (DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_16)
                    {
                        nSyncExt = (DTSParserParams_p->TempBuff.Buff[((DTSParserParams_p->TempBuff.ReadPos + 4)&0xF)]) >> 2;                      // Extract sync extension bits
                        if ( nSyncExt == DTS_SYNCWORD_CORE_16EXT )  // Found sync extension
                        {
                            STTBX_Print((" ES_DTS Parser Next Core sync found \n"));
                            DTSParserParams_p->DTSParserState = ES_DTS_NEXT2_TBD_CASE1_SYNC_LOCK;
                            DTSParserParams_p->InSync         = TRUE;
                            DTSParserParams_p->SubStream = FALSE;
                        }
                        else
                        {
                            // Alias sync word, add to buffer & continue searching.
                            *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                            // Add the current one byte to Core Frame.
                            DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                            DTSParserParams_p->TempBuff.Size--;
                            DTSParserParams_p->CoreBytes++;
                            if( DTSParserParams_p->CoreBytes > 16*1024)
                            {
                                STTBX_Print((" ES_DTS Parser Core_Stream  Size is too large erstart sync search \n"));
                                DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                            }
                        }
                    }
                    if (DTSParserParams_p->BitStreamType & DTS_BITSTREAM_LOSSY_14 )
                    {
                        nSyncExt = ((((DTSParserParams_p->TempBuff.Buff[((DTSParserParams_p->TempBuff.ReadPos + 4)&0xF)]) << 8)|
                                   (DTSParserParams_p->TempBuff.Buff[((DTSParserParams_p->TempBuff.ReadPos + 5)&0xF)]))>>4);                      // Extract sync extension bits                      // Extract sync extension bits
                        if ( nSyncExt == DTS_SYNCWORD_CORE_14EXT )  // Found sync extension
                        {
                            STTBX_Print((" ES_DTS Parser Next Core sync found \n"));
                            DTSParserParams_p->DTSParserState = ES_DTS_NEXT2_TBD_CASE1_SYNC_LOCK;
                            DTSParserParams_p->InSync         = TRUE;
                            DTSParserParams_p->SubStream = FALSE;
                        }
                        else
                        {
                            // Alias sync word, add to buffer & continue searching.
                            *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                            // Add the current one byte to Core Frame.
                            DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                            DTSParserParams_p->TempBuff.Size--;
                            DTSParserParams_p->CoreBytes++;
                            if( DTSParserParams_p->CoreBytes > 16*1024)
                            {
                                STTBX_Print((" ES_DTS Parser Core_Stream  Size is too large erstart sync search \n"));
                                DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                            }
                        }
                    }

                }
                else if ( nWord == DTS_SYNCWORD_SUBSTREAM)
                {
                    // If substream sync word is encountered after the Core Sync CASE 2.
                    // Fill the 0-3 null bytes between the core and substream for 32 bit alignment
                    for(i=0;i<(DTSParserParams_p->CoreBytes & 3);i++)
                    {
                        *DTSParserParams_p->Current_Write_Ptr1++ = 0;
                    }
                    DTSParserParams_p->CoreBytes += i;
                    DTSParserParams_p->SubStream = TRUE;
                    DTSParserParams_p->DTSParserState = ES_DTS_CHECK_SYNC_CASE2;
                }
                else
                {

                    *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                    // Add the current one byte to Core Frame.
                    DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                    DTSParserParams_p->TempBuff.Size--;
                    DTSParserParams_p->CoreBytes++;
                    if( DTSParserParams_p->CoreBytes > 16*1024)
                    {
                        STTBX_Print((" ES_DTS Parser Core_Stream  Size is too large erstart sync search \n"));
                        DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                    }
                }
            }
            break;

        case ES_DTS_NEXT2_TBD_CASE1_SYNC_LOCK:
            DTSParserParams_p->FrameSize = DTSParserParams_p->CoreBytes;

            DTSParserParams_p->DTSParserState = ES_DTS_PUT_OUTPUT;
            break;


        case ES_DTS_IN_SYNC_CASE1:
            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {
                nWord = DTS_READ_NEXT_WORD32(DTSParserParams_p); // check for sync work
                if ( nWord == DTSParserParams_p->SyncWord )
                {
                    // Store sync word and temp Buff data in the Frame buffer.
                    DTSParserParams_p->NoBytesCopied = 0;
                    TempBuffSize = DTSParserParams_p->TempBuff.Size;
                    for(i = 0; i < TempBuffSize; i++)
                    {
                        *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                        DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                        DTSParserParams_p->TempBuff.Size--;
                        DTSParserParams_p->NoBytesCopied++;
                    }
                    DTSParserParams_p->FrameSize = DTSParserParams_p->CoreBytes;
                    DTSParserParams_p->DTSParserState = ES_DTS_WRITE_IN_FRAME_BUFFER;

                    DTSParserParams_p->CheckNextSubStreamSync = DTSParserParams_p->SubStream;
                    DTSParserParams_p->SubStreamIndex = 4;
                    DTSParserParams_p->nSubStreamSize = 0;

                }
                else
                {
                    STTBX_Print((" ES_DTS Parser Sync fail for CASE 1 re sync\n"));
                    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                }
            }
            break;

        case ES_DTS_CHECK_SYNC_CASE2:
            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->DTSParserState_Current = DTSParserParams_p->DTSParserState;
                DTSParserParams_p->DTSParserState         = ES_DTS_FILL_TEMP_BUFF;
            }
            else
            {

                DTSParserParams_p->SubStreamIndex_last = DTSParserParams_p->SubStreamIndex;
                bResult = ReadSubStreamHeader(DTSParserParams_p);
                if(bResult == TRUE)
                {
                    if((DTSParserParams_p->SubStreamIndex > DTSParserParams_p->SubStreamIndex_last)||(DTSParserParams_p->SubStreamIndex_last == 4))
                    {
                        // Store sync word and temp Buff data in the Frame buffer.
                        DTSParserParams_p->NoBytesCopied = 0;
                        SizeToWrite = (DTSParserParams_p->SubStreamSize > DTSParserParams_p->TempBuff.Size) ?
                                        DTSParserParams_p->TempBuff.Size:DTSParserParams_p->SubStreamSize;

                        for(i = 0; i < SizeToWrite; i++)
                        {
                            *DTSParserParams_p->Current_Write_Ptr1++ = DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.ReadPos++];
                            DTSParserParams_p->TempBuff.ReadPos = DTSParserParams_p->TempBuff.ReadPos & 0xF;
                            DTSParserParams_p->TempBuff.Size--;
                            DTSParserParams_p->NoBytesCopied++;
                        }
                        DTSParserParams_p->FrameSize = DTSParserParams_p->SubStreamSize;
                        DTSParserParams_p->nSubStreamSize += DTSParserParams_p->SubStreamSize;
                        DTSParserParams_p->DTSParserState = ES_DTS_WRITE_IN_FRAME_BUFFER;
                        DTSParserParams_p->CheckNextSubStreamSync  = TRUE;
                    }
                    else
                    {   DTSParserParams_p->FrameSize = DTSParserParams_p->nSubStreamSize;
                        DTSParserParams_p->nSubStreamSize = 0;
                        DTSParserParams_p->CheckNextSubStreamSync  = FALSE;
                        DTSParserParams_p->DTSParserState = ES_DTS_PUT_OUTPUT;
                    }
                }
                else if(DTSParserParams_p->BitStreamType == DTS_BITSTREAM_SUBSTREAM_32)
                {
                    STTBX_Print((" ES_DTS Parser Sync fail for CASE 2 re sync\n"));
                    DTSParserParams_p->DTSParserState = ES_DTS_SYNC_0;
                }
                else
                {
                    DTSParserParams_p->FrameSize = DTSParserParams_p->CoreBytes + DTSParserParams_p->nSubStreamSize;
                    DTSParserParams_p->CheckNextSubStreamSync  = FALSE;
                    DTSParserParams_p->DTSParserState = ES_DTS_PUT_OUTPUT;
                }
            }
            break;

        case ES_DTS_WRITE_IN_FRAME_BUFFER:
            if((Size-offset+DTSParserParams_p->NoBytesCopied)>=DTSParserParams_p->FrameSize)
            {
                /* 1 or more frame */
                memcpy(DTSParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(DTSParserParams_p->FrameSize - DTSParserParams_p->NoBytesCopied));
                DTSParserParams_p->Current_Write_Ptr1 += (DTSParserParams_p->FrameSize - DTSParserParams_p->NoBytesCopied);
                offset =  offset-DTSParserParams_p->NoBytesCopied+DTSParserParams_p->FrameSize;
                DTSParserParams_p->NoBytesCopied = 0;
                DTSParserParams_p->DTSParserState = ES_DTS_PUT_OUTPUT;
            }
            else
            {
                /* less than a frame */
                DTSParserParams_p->NoBytesRemainingInFrame = DTSParserParams_p->FrameSize  - (Size-offset+DTSParserParams_p->NoBytesCopied);
                memcpy(DTSParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                DTSParserParams_p->Current_Write_Ptr1 += (Size - offset);
                offset =  Size;
                DTSParserParams_p->NoBytesCopied = 0;
                DTSParserParams_p->DTSParserState = ES_DTS_PARTIAL_FRAME_WRITE_IN_FRAME_BUFFER;
            }
            break;

        case ES_DTS_PARTIAL_FRAME_WRITE_IN_FRAME_BUFFER:
            if(Size-offset>=DTSParserParams_p->NoBytesRemainingInFrame)
            {
                /* full frame */
                memcpy(DTSParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),DTSParserParams_p->NoBytesRemainingInFrame);
                DTSParserParams_p->Current_Write_Ptr1 += DTSParserParams_p->NoBytesRemainingInFrame;
                offset =  offset+DTSParserParams_p->NoBytesRemainingInFrame;
                DTSParserParams_p->DTSParserState = ES_DTS_PUT_OUTPUT;

            }
            else
            {
                /* again less than a frame */
                DTSParserParams_p->NoBytesRemainingInFrame -= (Size-offset);
                memcpy(DTSParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),Size-offset);
                DTSParserParams_p->Current_Write_Ptr1 += Size-offset;
                offset =  Size;
                DTSParserParams_p->DTSParserState = ES_DTS_PARTIAL_FRAME_WRITE_IN_FRAME_BUFFER;
            }
            break;

        case ES_DTS_PUT_OUTPUT:
            if(DTSParserParams_p->AppData_p->CoreSize!=DTSParserParams_p->CoreBytes)
            {
                DTSParserParams_p->AppData_p->ChangeSet = TRUE;
                DTSParserParams_p->AppData_p->CoreSize = DTSParserParams_p->CoreBytes;
                DTSParserParams_p->AppData_p->FirstByteEncSamples = DTSParserParams_p->FirstByteEncSamples;
                DTSParserParams_p->AppData_p->Last4ByteEncSamples = DTSParserParams_p->Last4ByteEncSamples;
                DTSParserParams_p->AppData_p->DelayLossLess = DTSParserParams_p->DelayLossLess;
                DTSParserParams_p->OpBlk_p->AppData_p = DTSParserParams_p->AppData_p;
            }
            if(DTSParserParams_p->CheckNextSubStreamSync)
            {
                DTSParserParams_p->DTSParserState = ES_DTS_CHECK_SYNC_CASE2;
            }
            else
            {
                DTSParserParams_p->SubStreamIndex = 4;
                DTSParserParams_p->nSubStreamSize = 0;
                DTSParserParams_p->OpBlk_p->Size = DTSParserParams_p->FrameSize;
                if(DTSParserParams_p->Get_Block == TRUE)
                {
                    if(DTSParserParams_p->OpBlk_p->Flags & PTS_VALID)
                    {
                        /* Get the PTS in the correct structure */
                        NotifyPTS.Low  = DTSParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                        NotifyPTS.High = DTSParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                    }
                    else
                    {
                        NotifyPTS.Low  = 0;
                        NotifyPTS.High = 0;
                    }

                    NotifyPTS.Interpolated  = FALSE;

        /*          if((FRAMESIZE != DTSParserParams_p->FrameSize)||CORESIZE != DTSParserParams_p->CoreBytes);
                    {
                        FRAMESIZE = DTSParserParams_p->FrameSize;
                        CORESIZE  = DTSParserParams_p->CoreBytes;
                        printf("FRAMESIZE[%d]  =%d  CORESIZE  =%d\n",counter,FRAMESIZE,CORESIZE);
                    }

                    counter++;  */
                    DTSParserParams_p->OpBlk_p->AppData_p = DTSParserParams_p->AppData_p;
                    Error = MemPool_PutOpBlk(DTSParserParams_p->OpBufHandle,(U32 *)&DTSParserParams_p->OpBlk_p);

                    if(Error != ST_NO_ERROR)
                    {
                        STTBX_Print((" ES_DTS Error in Memory Put_Block \n"));
                        return Error;
                    }
                    else
                    {
                        Error = STAudEVT_Notify(DTSParserParams_p->EvtHandle,DTSParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), DTSParserParams_p->ObjectID);
                        DTSParserParams_p->Put_Block = TRUE;
                        DTSParserParams_p->Get_Block = FALSE;
                    }
                }

                DTSParserParams_p->FrameComplete = TRUE;

                //TBD for other cases:
                if(DTSParserParams_p->BitStreamType != DTS_BITSTREAM_SUBSTREAM_32)
                {
                    DTSParserParams_p->DTSParserState = ES_DTS_IN_SYNC_CASE1;
                }
                else
                {
                    DTSParserParams_p->DTSParserState = ES_DTS_CHECK_SYNC_CASE2;
                }

            }

            break;

        case ES_DTS_FILL_TEMP_BUFF:
            if(DTSParserParams_p->TempBuff.Size<16)
            {
                DTSParserParams_p->TempBuff.Buff[DTSParserParams_p->TempBuff.WritePos++] = value;
                DTSParserParams_p->TempBuff.WritePos = (DTSParserParams_p->TempBuff.WritePos & 0xF);
                DTSParserParams_p->TempBuff.Size++;
                offset++;
            }
            else
            {
                DTSParserParams_p->DTSParserState = DTSParserParams_p->DTSParserState_Current;
            }
            break;
        default:
                break;

        }
    }
    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_DTS_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p)
{

    ES_DTS_ParserLocalParams_t *DTSParserLocalParams_p = (ES_DTS_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, DTSParserLocalParams_p->AppData_p);
        STOS_MemoryDeallocate(CPUPartition_p, DTSParserLocalParams_p);
    }

    return ST_NO_ERROR;

}

#endif

