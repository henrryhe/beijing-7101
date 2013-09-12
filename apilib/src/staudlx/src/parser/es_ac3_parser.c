/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : ES_AC3_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_AC3
//#define  STTBX_PRINT
#include "stos.h"
#include "es_ac3_parser.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */



static U32 AC3FrameSizes[2][20] =
    {
        {
             64 * 2,   80 * 2,   96 * 2,  112 * 2,
            128 * 2,  160 * 2,  192 * 2,  224 * 2,
            256 * 2,  320 * 2,  384 * 2,  448 * 2,
            512 * 2,  640 * 2,  768 * 2,  896 * 2,
            1024* 2, 1152 * 2, 1280 * 2,        0
        },
        {
             69 * 2,  87 * 2, 104 * 2, 121 * 2,
            139 * 2, 174 * 2, 208 * 2, 243 * 2,
            278 * 2, 348 * 2, 417 * 2, 487 * 2,
            557 * 2, 696 * 2, 835 * 2, 975 * 2,
            1114* 2,1253 * 2, 1393* 2,       0
        }
    };






static U32 AC3Frequency[] =
    {
        48000,  44100,  32000,  0
    };


/* ----------------------------------------------------------------------------
        488                 External Data Types
--------------------------------------------------------------------------- */


/*---------------------------------------------------------------------
        835             Private Functions
----------------------------------------------------------------------*/

int ddplus_parserGetBits(BOOL reset,U32 MemoryStart, int bitsNeeded, U32 * BytesRead)
{
    static unsigned int BitBuffer = 0;
    static unsigned int BitsLeft  = 0;
    int   data, temp;
    U8 * DataPtr = (U8 *)MemoryStart;

    if(reset)
    {
        BitBuffer = 0;
        BitsLeft  = 0;
    }


    if (BitsLeft < (unsigned int)bitsNeeded)
    {
        data       = BitBuffer >> ( 32 - bitsNeeded );
        bitsNeeded = (bitsNeeded - BitsLeft);
        data       = data <<(bitsNeeded);
        BitBuffer  = DataPtr[*BytesRead] << 24;
        (*BytesRead)++;
        BitBuffer  = BitBuffer | (DataPtr[*BytesRead] << 16);
        (*BytesRead)++;
        BitBuffer  = BitBuffer | (DataPtr[*BytesRead] << 8);
        (*BytesRead)++;
        BitBuffer  = BitBuffer | (DataPtr[*BytesRead]);
        (*BytesRead)++;

        BitsLeft   = 32;
        temp       = BitBuffer >> (32 - bitsNeeded);
        BitBuffer  = BitBuffer << bitsNeeded;
        data       = data | temp;
        BitsLeft  -= bitsNeeded;
    }
    else
    {
        data        = BitBuffer >> ( 32 - bitsNeeded);
        BitBuffer <<= (bitsNeeded);
        BitsLeft   -= bitsNeeded;
    }

    return data;
}

void    SearchConvSync(ES_AC3_ParserLocalParams_t * AC3ParserParams_p)
{

    int temp;
    U32 acmod;
    U32 blk, fscod,lfe,strmTp;
    U32 nblocks;
    U32 convSync = 0;
    U32 BytesRead = AC3ParserParams_p->Audio_FrameSize - AC3ParserParams_p->FrameSize + 6;
    U32 MemoryStart = AC3ParserParams_p->OpBlk_p->MemoryStart;

    strmTp = (AC3ParserParams_p->HDR[0] >> 6) & 0x3;
    acmod=(AC3ParserParams_p->HDR[2] >> 1) & 0x7;
    fscod = (AC3ParserParams_p->HDR[2] >> 6) & 0x3;
    lfe = (AC3ParserParams_p->HDR[2] ) & 0x1;
    temp = (AC3ParserParams_p->HDR[2] >> 4) & 0x3;
    nblocks   = (fscod == 3) ? 6 : ((temp == 3)  ? 6 : temp + 1);

    temp = ddplus_parserGetBits(1,MemoryStart, 3, &BytesRead) & 0x01; // compre present
    if (temp == 0x01)
    {
        ddplus_parserGetBits(0,MemoryStart, 8, &BytesRead); // compre
    }
    if (acmod == 0x0)
    {
        temp = ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead) & 0x01; // compre2 present
        if (temp == 0x01)
        {
            ddplus_parserGetBits(0,MemoryStart, 8, &BytesRead); // compre
        }
    }
    if(strmTp == 0x01)
    {
        temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // chanmap present
        if (temp == 0x01)
        {
            ddplus_parserGetBits(0,MemoryStart, 16, &BytesRead); // chanmap
        }
    }
    temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // mixmdate present

    if (temp == 0x01)
    {
        if (acmod > 2)
        {
            ddplus_parserGetBits(0,MemoryStart, 2, &BytesRead); // dmixmod
        }

        if  ((acmod & 0x01) && (acmod > 2))
        {
            ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead); // ltrtcmixlev & lorocmixlev
        }

        if  (acmod & 0x4)
        {
            ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead); // ltrtcmixlev & lorocmixlev
        }

        if (lfe)
        {
            temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // lfemixlevcode
            if (temp == 0x1)
            {
                ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
            }
        }
        if (strmTp == 0x0)
        {
            temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // pgmscale
            if (temp == 0x1)
            {
                ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead);
            }
            if (acmod == 0)
            {
                temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // pgmscale2
                if (temp == 0x1)
                {
                    ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead);
                }
            }
            temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // extpgmscale
            if (temp == 0x1)
            {
                ddplus_parserGetBits(0,MemoryStart, 6, &BytesRead);
            }

            temp = ddplus_parserGetBits(0,MemoryStart, 2, &BytesRead) & 0x03; // extpgmscale

            if (temp == 1)
            {
                ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
            }
            else if (temp == 2)
            {
                ddplus_parserGetBits(0,MemoryStart, 12, &BytesRead);
            }
            else if (temp == 3)
            {
                temp = ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
                ddplus_parserGetBits(0,MemoryStart, 8*(temp+2), &BytesRead);
            }

            if (acmod < 2)
            {
                temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // paninfoe
                if (temp == 0x1)
                {
                    ddplus_parserGetBits(0,MemoryStart, 14, &BytesRead);
                }

                if (acmod == 0)
                {
                    temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // paninfoe2
                    if (temp == 0x1)
                        ddplus_parserGetBits(0,MemoryStart, 14, &BytesRead);
                }
            }

            temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // frmmixcfginfoe

            if (temp == 0x1)
            {
                if (nblocks == 0)
                {
                    ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
                }

                for(blk=0;blk<nblocks;blk++)
                {
                    temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // paninfoe2
                    if (temp == 0x1)
                    {
                        ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
                    }
                }
            }
        }
    }

    temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // infometadatae

    if (temp == 0x1)
    {
        ddplus_parserGetBits(0,MemoryStart, 5, &BytesRead);
        if(acmod == 2)
        {
            ddplus_parserGetBits(0,MemoryStart, 4, &BytesRead);
        }
        if (acmod >= 6)
        {
            ddplus_parserGetBits(0,MemoryStart, 2, &BytesRead);
        }
        temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // audprodie
        if (temp == 0x1)
        {
            ddplus_parserGetBits(0,MemoryStart, 8, &BytesRead);
        }
        if (acmod == 0)
        {
            temp = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead) & 0x01; // audprodie2
            if (temp == 0x1)
            {
                ddplus_parserGetBits(0,MemoryStart, 8, &BytesRead);
            }
        }
        if (fscod < 0x3)
        {
            ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead);
        }
    }

    if ((strmTp == 0) && (nblocks != 6))
    {
        convSync = ddplus_parserGetBits(0,MemoryStart, 1, &BytesRead);
    }

    if(nblocks == 6)
    {
        convSync = 1;
    }

    if (convSync == 1)
    {
        AC3ParserParams_p->OutOfSync = FALSE;
        if(AC3ParserParams_p->DDPFirstSyncFrame == 1)
        {
            AC3ParserParams_p->SyncFoundOnFirstFrame=TRUE;
        }
        else
        {
            AC3ParserParams_p->SyncFoundOnFirstFrame=FALSE;
        }
    }

//          printf("ConvSync = %d \n", convSync);
//          printf("NumBlks  = %d \n", nblocks);
    }


/************************************************************************************
Name         : ES_AC3_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_AC3_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AC3_ParserLocalParams_t * AC3ParserParams_p;
    AC3ParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_AC3_ParserLocalParams_t));
    if(AC3ParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(AC3ParserParams_p,0,sizeof(ES_AC3_ParserLocalParams_t));
    /* Init the Local Parameters */

    AC3ParserParams_p->AC3ParserState         = ES_AC3_SYNC_0;
    AC3ParserParams_p->FrameComplete          = TRUE;
    AC3ParserParams_p->First_Frame            = TRUE;
    AC3ParserParams_p->Audio_FrameSize        = 0;
    AC3ParserParams_p->OutOfSync              = TRUE;
    AC3ParserParams_p->SyncFoundOnFirstFrame  = TRUE;
    AC3ParserParams_p->DDPFirstSyncFrame      = 0;
    AC3ParserParams_p->AC3OutOfSync           = FALSE;
    AC3ParserParams_p->NBSampleOut            = 0;
    AC3ParserParams_p->Frequency              = 0xFFFFFFFF;
    AC3ParserParams_p->BitRate                = 0;
    AC3ParserParams_p->LastStreamType         = I0;
    AC3ParserParams_p->DCV71                  = TRUE;
    //AC3ParserParams_p->DEC71                = TRUE;
    AC3ParserParams_p->DEC71                  = FALSE;//For DCV 7.1
    AC3ParserParams_p->CheckForD0             = FALSE;
    AC3ParserParams_p->PreParseHeader         = FALSE;
    AC3ParserParams_p->Get_Block              = FALSE;
    AC3ParserParams_p->Put_Block              = TRUE;
    AC3ParserParams_p->Skip                   = 0;
    AC3ParserParams_p->Pause                  = 0;
    AC3ParserParams_p->Reuse                  = FALSE;
    AC3ParserParams_p->OpBufHandle            = Init_p->OpBufHandle;
    AC3ParserParams_p->EvtHandle              = Init_p->EvtHandle;
    AC3ParserParams_p->ObjectID               = Init_p->ObjectID;
    AC3ParserParams_p->EventIDNewFrame        = Init_p->EventIDNewFrame;
    AC3ParserParams_p->EventIDFrequencyChange = Init_p->EventIDFrequencyChange;
    AC3ParserParams_p->EventIDNewRouting      = Init_p->EventIDNewRouting;

    AC3ParserParams_p->StartFramePTSValid = TRUE;

    AC3ParserParams_p->Bit_Rate_Code     = 0xFF;
    AC3ParserParams_p->Bit_Stream_Mode  = 0xFF;
    AC3ParserParams_p->Audio_Coding_Mode = 0xFF;

    *handle=(ParserHandle_t )AC3ParserParams_p;

    /* to init O/P pointers */
    AC3ParserParams_p->Blk_Out = 0;
    return (Error);
}

ST_ErrorCode_t ES_AC3_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_AC3_Parse_Frame;
    *GetFreqFunction_p  = (GetFreqFunction_t)ES_AC3_Get_Freq;
    *GetInfoFunction_p  = (GetInfoFunction_t)ES_AC3_Get_Info;

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_AC3_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AC3_ParserLocalParams_t *AC3ParserParams_p;
    AC3ParserParams_p=(ES_AC3_ParserLocalParams_t *)Handle;

    if(AC3ParserParams_p->Get_Block == TRUE)
    {
        AC3ParserParams_p->OpBlk_p->Size = AC3ParserParams_p->Audio_FrameSize ;
        AC3ParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(AC3ParserParams_p->OpBufHandle,(U32 *)&AC3ParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            AC3ParserParams_p->Put_Block = TRUE;
            AC3ParserParams_p->Get_Block = FALSE;
        }
    }

    AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
    AC3ParserParams_p->FrameComplete = TRUE;
    AC3ParserParams_p->First_Frame   = TRUE;
    AC3ParserParams_p->NBSampleOut = 0;
    AC3ParserParams_p->OutOfSync = TRUE;
    AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
    AC3ParserParams_p->DDPFirstSyncFrame = 0;
    AC3ParserParams_p->AC3OutOfSync = FALSE;
    AC3ParserParams_p->Blk_Out = 0;
    return Error;
}

/************************************************************************************
Name         : ES_AC3_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AC3_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AC3_ParserLocalParams_t *AC3ParserParams_p;
    AC3ParserParams_p=(ES_AC3_ParserLocalParams_t *)Handle;

    AC3ParserParams_p->AC3ParserState        = ES_AC3_SYNC_0;
    AC3ParserParams_p->Audio_FrameSize       = 0;
    AC3ParserParams_p->NBSampleOut           = 0;
    AC3ParserParams_p->Frequency             = 0xFFFFFFFF;
    AC3ParserParams_p->BitRate               = 0;
    AC3ParserParams_p->LastStreamType        = I0;
    AC3ParserParams_p->DCV71                 = TRUE;
    //AC3ParserParams_p->DEC71               = TRUE;
    AC3ParserParams_p->DEC71                 = FALSE;//For DCV 7.1
    AC3ParserParams_p->CheckForD0            = FALSE;
    AC3ParserParams_p->PreParseHeader        = FALSE;
    AC3ParserParams_p->FrameComplete         = TRUE;
    AC3ParserParams_p->First_Frame           = TRUE;
    AC3ParserParams_p->OutOfSync             = TRUE;
    AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
    AC3ParserParams_p->DDPFirstSyncFrame     = 0;
    AC3ParserParams_p->AC3OutOfSync          = FALSE;
    AC3ParserParams_p->Put_Block             = TRUE;
    AC3ParserParams_p->Get_Block             = FALSE;
    AC3ParserParams_p->Skip                  = 0;
    AC3ParserParams_p->Pause                 = 0;
    AC3ParserParams_p->Reuse                 = FALSE;
    AC3ParserParams_p->StartFramePTSValid    = TRUE;
    AC3ParserParams_p->Bit_Rate_Code         = 0xFF;
    AC3ParserParams_p->Bit_Stream_Mode       = 0xFF;
    AC3ParserParams_p->Audio_Coding_Mode     = 0xFF;
    AC3ParserParams_p->Blk_Out               = 0;

    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_AC3_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

    ES_AC3_ParserLocalParams_t *AC3ParserParams_p;
    AC3ParserParams_p=(ES_AC3_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=AC3ParserParams_p->Frequency;

    return ST_NO_ERROR;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_AC3_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
    ES_AC3_ParserLocalParams_t *AC3ParserParams_p;
    AC3ParserParams_p=(ES_AC3_ParserLocalParams_t *)Handle;

    StreamInfo->SamplingFrequency   = AC3ParserParams_p->Frequency;
    StreamInfo->Bit_Rate_Code       = AC3ParserParams_p->Bit_Rate_Code ;
    StreamInfo->Bit_Stream_Mode     = AC3ParserParams_p->Bit_Stream_Mode ;
    StreamInfo->Audio_Coding_Mode   = AC3ParserParams_p->Audio_Coding_Mode ;
    StreamInfo->CopyRight           = 0; /*Currently not supported*/
    return ST_NO_ERROR;
}
/* handle EOF from Parser */

ST_ErrorCode_t ES_AC3_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AC3_ParserLocalParams_t *AC3ParserParams_p = (ES_AC3_ParserLocalParams_t *)Handle;

    if(AC3ParserParams_p->Get_Block == TRUE)
    {
        memset(AC3ParserParams_p->Current_Write_Ptr1,0x00,AC3ParserParams_p->NoBytesRemainingInFrame);

        AC3ParserParams_p->OpBlk_p->Size    = AC3ParserParams_p->FrameSize;
        AC3ParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        AC3ParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        AC3ParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
        Error = MemPool_PutOpBlk(AC3ParserParams_p->OpBufHandle,(U32 *)&AC3ParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
        }
        else
        {
            AC3ParserParams_p->Put_Block = TRUE;
            AC3ParserParams_p->Get_Block = FALSE;
        }
    }
    else
    {
        Error = MemPool_GetOpBlk(AC3ParserParams_p->OpBufHandle, (U32 *)&AC3ParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            AC3ParserParams_p->Put_Block = FALSE;
            AC3ParserParams_p->Get_Block = TRUE;
            AC3ParserParams_p->OpBlk_p->Size = 1536;
            AC3ParserParams_p->OpBlk_p->Flags  |= EOF_VALID;
            memset((void*)AC3ParserParams_p->OpBlk_p->MemoryStart,0x00,AC3ParserParams_p->OpBlk_p->MaxSize);
            Error = MemPool_PutOpBlk(AC3ParserParams_p->OpBufHandle,(U32 *)&AC3ParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
            }
            else
            {
                AC3ParserParams_p->Put_Block = TRUE;
                AC3ParserParams_p->Get_Block = FALSE;
            }
        }
    }

    return ST_NO_ERROR;
}


/************************************************************************************
Name         : Parse_AC3_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t ES_AC3_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{
    U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    ES_AC3_ParserLocalParams_t *AC3ParserParams_p;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    //  STTBX_Print((" ES_AC3 Parser Enrty \n"));

    AC3ParserParams_p=(ES_AC3_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {

        if(AC3ParserParams_p->FrameComplete == TRUE)
        {
            if(AC3ParserParams_p->Put_Block == TRUE)
            {
                if(AC3ParserParams_p->Reuse == FALSE)
                {
                    Error = MemPool_GetOpBlk(AC3ParserParams_p->OpBufHandle, (U32 *)&AC3ParserParams_p->OpBlk_p);
                    if(Error != ST_NO_ERROR)
                    {
                        *No_Bytes_Used = offset;
                        return STAUD_MEMORY_GET_ERROR;
                    }
                    else
                    {
                        if(AC3ParserParams_p->Pause)
                        {
                            AUD_TaskDelayMs(AC3ParserParams_p->Pause);
                            AC3ParserParams_p->Pause = 0;
                        }
                    }
                }

                AC3ParserParams_p->Put_Block = FALSE;
                AC3ParserParams_p->Get_Block = TRUE;
                AC3ParserParams_p->Current_Write_Ptr1 = (U8 *)AC3ParserParams_p->OpBlk_p->MemoryStart;
                //memset(AC3ParserParams_p->Current_Write_Ptr1,0x00,AC3ParserParams_p->OpBlk_p->MaxSize);
                AC3ParserParams_p->OpBlk_p->Flags = 0;

                if(Private_Info_p->Private_Data[2] == 1) // FADE PAN VALID FLAG
                {
                    AC3ParserParams_p->OpBlk_p->Flags   |= FADE_VALID;
                    AC3ParserParams_p->OpBlk_p->Flags   |= PAN_VALID;
                    AC3ParserParams_p->OpBlk_p->Data[FADE_OFFSET]    = (Private_Info_p->Private_Data[5]<<8)|(Private_Info_p->Private_Data[0]); // FADE Value
                    AC3ParserParams_p->OpBlk_p->Data[PAN_OFFSET] = (Private_Info_p->Private_Data[6]<<8)|(Private_Info_p->Private_Data[1]); // PAN Value
                }

                //mark the alignment tag
                AC3ParserParams_p->OpBlk_p->Flags   |= DATAFORMAT_VALID;
                AC3ParserParams_p->OpBlk_p->Data[DATAFORMAT_OFFSET]  = BE;
            }

             // code below commented out. To be invalidated at start of frame
            //CurrentPTS_p->PTSValidFlag        = FALSE;
            AC3ParserParams_p->FrameComplete = FALSE;
            AC3ParserParams_p->Audio_FrameSize = 0;
            AC3ParserParams_p->NBSampleOut = 0;
            AC3ParserParams_p->OutOfSync = TRUE;
            AC3ParserParams_p->DDPFirstSyncFrame = 0;
            if(AC3ParserParams_p->SyncFoundOnFirstFrame == FALSE)
            {
                memcpy(AC3ParserParams_p->Current_Write_Ptr1,&(AC3ParserParams_p->Temp_Buffer),AC3ParserParams_p->FrameSize);
                AC3ParserParams_p->Current_Write_Ptr1 += AC3ParserParams_p->FrameSize;
                AC3ParserParams_p->Audio_FrameSize += AC3ParserParams_p->FrameSize;
                AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
                AC3ParserParams_p->NBSampleOut = AC3ParserParams_p->NBSampleOut_Current;
            }
        }
        value=pos[offset];

        switch(AC3ParserParams_p->AC3ParserState)
        {
        case ES_AC3_SYNC_0:
            if (value == 0x0b)
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_1;
                AC3ParserParams_p->NoBytesCopied = 1;

                if(CurrentPTS_p->PTSValidFlag)
                {
                    AC3ParserParams_p->StartFramePTS = CurrentPTS_p->PTS;
                    AC3ParserParams_p->StartFramePTS33 = CurrentPTS_p->PTS33Bit;
                    AC3ParserParams_p->StartFramePTSValid = TRUE;
                    CurrentPTS_p->PTSValidFlag = FALSE;
                }

            }
            else
            {
                AC3ParserParams_p->NoBytesCopied = 0;
            }

            offset++;
            break;

        case ES_AC3_SYNC_1:
            if (value == 0x77)
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_HDR_0;
                AC3ParserParams_p->NoBytesCopied++;
                offset++;
                *AC3ParserParams_p->Current_Write_Ptr1++ = 0x0b;
                *AC3ParserParams_p->Current_Write_Ptr1++ = 0x77;
            }
            else
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
                //AC3ParserParams_p->Current_Write_Ptr1 = (U8 *)AC3ParserParams_p->OpBlk_p->MemoryStart;
            }
            break;

        case ES_AC3_HDR_0:
            AC3ParserParams_p->HDR[0]=value;
            AC3ParserParams_p->AC3ParserState = ES_AC3_HDR_1;
            offset++;
            AC3ParserParams_p->NoBytesCopied++;
            *AC3ParserParams_p->Current_Write_Ptr1++ = value;
            break;

        case ES_AC3_HDR_1:
            AC3ParserParams_p->HDR[1]=value;
            AC3ParserParams_p->AC3ParserState = ES_AC3_HDR_2;
            offset++;
            AC3ParserParams_p->NoBytesCopied++;
            *AC3ParserParams_p->Current_Write_Ptr1++ = value;
            break;

        case ES_AC3_HDR_2:
            AC3ParserParams_p->HDR[2]=value;
            AC3ParserParams_p->AC3ParserState = ES_AC3_HDR_3;
            offset++;
            AC3ParserParams_p->NoBytesCopied++;
            *AC3ParserParams_p->Current_Write_Ptr1++ = value;
            break;

        case ES_AC3_HDR_3:
            AC3ParserParams_p->HDR[3]=value;
            AC3ParserParams_p->AC3ParserState = ES_AC3_CHECK_HDR;
            offset++;
            AC3ParserParams_p->NoBytesCopied++;
            *AC3ParserParams_p->Current_Write_Ptr1++ = value;
            break;

        case ES_AC3_CHECK_HDR:
            if((((AC3ParserParams_p->HDR[3] >> 3) & 0x1F) >= 11) && (((AC3ParserParams_p->HDR[3] >> 3) & 0x1F) <= 16))
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_PLUS_FSCOD;
                AC3ParserParams_p->DDPFrame = TRUE;
            }
            else if (((AC3ParserParams_p->HDR[3] >> 3) & 0x1F) <= 8)
            {
                AC3ParserParams_p->DDPFrame = FALSE;
                if(AC3ParserParams_p->NBSampleOut == 0)
                {
                    AC3ParserParams_p->AC3ParserState = ES_AC3_FSCOD;
                }
                else
                {
                    AC3ParserParams_p->AC3ParserState = ES_AC3_FRAME;
                    AC3ParserParams_p->OutOfSync      = TRUE;
                    AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
                    AC3ParserParams_p->DDPFirstSyncFrame = 0;
                    AC3ParserParams_p->AC3OutOfSync   = TRUE;
                    AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                    AC3ParserParams_p->NoBytesCopied = 0;
                    AC3ParserParams_p->FrameSize = 0;
                }

            }
            else
            {
                // Invalid BSID just copy the header as a frame and go back to sync
                AC3ParserParams_p->AC3ParserState = ES_AC3_FRAME;
                AC3ParserParams_p->OutOfSync      = TRUE;
                AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
                AC3ParserParams_p->DDPFirstSyncFrame = 0;
                AC3ParserParams_p->DDPFrame = FALSE;
                AC3ParserParams_p->Audio_FrameSize += AC3ParserParams_p->NoBytesCopied;
                AC3ParserParams_p->NoBytesCopied = 0;
                AC3ParserParams_p->FrameSize = 0;

                // AC3ParserParams_p->FrameSize = AC3ParserParams_p->NoBytesCopied;
                // AC3ParserParams_p->DDPFrame = FALSE;
                // AC3ParserParams_p->NBSampleOut = 1536;
                // AC3ParserParams_p->NBSampleOut_Current = 1536;
                // AC3ParserParams_p->Audio_FrameSize = AC3ParserParams_p->FrameSize;
                // AC3ParserParams_p->AC3ParserState =ES_AC3_FRAME;
            }
            break;

        case ES_AC3_FSCOD_FOR_SYNC:
            AC3ParserParams_p->AC3OutOfSync  = FALSE;
            AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
            *AC3ParserParams_p->Current_Write_Ptr1++ = 0x0b;
            *AC3ParserParams_p->Current_Write_Ptr1++ = 0x77;
            *AC3ParserParams_p->Current_Write_Ptr1++ = AC3ParserParams_p->HDR[0];
            *AC3ParserParams_p->Current_Write_Ptr1++ = AC3ParserParams_p->HDR[1];
            *AC3ParserParams_p->Current_Write_Ptr1++ = AC3ParserParams_p->HDR[2];
            *AC3ParserParams_p->Current_Write_Ptr1++ = AC3ParserParams_p->HDR[3];
            AC3ParserParams_p->NoBytesCopied = 6;
            AC3ParserParams_p->AC3ParserState = ES_AC3_CHECK_HDR;
            break;

        case ES_AC3_FSCOD:
            if((AC3ParserParams_p->HDR[2] & 0x3f) > 37 || (AC3ParserParams_p->HDR[2] & 0xc0) == 0xc0 ) // MAX fscode 100101
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
                AC3ParserParams_p->Current_Write_Ptr1 = (U8 *)AC3ParserParams_p->OpBlk_p->MemoryStart;

            }
            else
            {
                U8 samp_freq_code = (AC3ParserParams_p->HDR[2] & 0xc0) >> 6;
                U8 frame_size_scode = (AC3ParserParams_p->HDR[2] & 0x3f)>>1;
                U8 index1 = samp_freq_code & 0x1;
                if(samp_freq_code==3)
                {
                    AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
                    AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                    STTBX_Print((" ES_AC3 ERROR in Sampling frequency code \n"));
                    break;
                }
                if(AC3ParserParams_p->CheckForD0)
                {
                    AC3ParserParams_p->CheckForD0 = FALSE;
                    AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                    AC3ParserParams_p->NoBytesCopied = 0;
                    AC3ParserParams_p->FrameSize = 0;
                    AC3ParserParams_p->AC3ParserState = ES_AC3_FRAME;
                    AC3ParserParams_p->PreParseHeader = TRUE;
                    break;
                }

                AC3ParserParams_p->Bit_Rate_Code     = (((AC3ParserParams_p->HDR[2] & 0x01)<<5)|frame_size_scode);
                AC3ParserParams_p->Bit_Stream_Mode   = (AC3ParserParams_p->HDR[3] & 0x07);

                /*report change in audio coding mode*/
                {
                    U8 ACMode = ((value >> 5) & 0x07);

                    if(AC3ParserParams_p->Audio_Coding_Mode != ACMode)
                    {
                        AC3ParserParams_p->Audio_Coding_Mode = ACMode;
                        /*Notify routing event*/
                        Error=STAudEVT_Notify(AC3ParserParams_p->EvtHandle,AC3ParserParams_p->EventIDNewRouting, &AC3ParserParams_p->Audio_Coding_Mode, sizeof(AC3ParserParams_p->Audio_Coding_Mode), AC3ParserParams_p->ObjectID);
                    }

                }
                //AC3ParserParams_p->Audio_Coding_Mode = ((value >> 5) & 0x07);

                if(AC3ParserParams_p->Frequency != AC3Frequency[samp_freq_code])
                {
                    AC3ParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                    AC3ParserParams_p->Frequency = AC3Frequency[samp_freq_code];
                    AC3ParserParams_p->OpBlk_p->Data[FREQ_OFFSET] = AC3ParserParams_p->Frequency;

                    Error=STAudEVT_Notify(AC3ParserParams_p->EvtHandle,AC3ParserParams_p->EventIDFrequencyChange, &AC3ParserParams_p->Frequency, sizeof(AC3ParserParams_p->Frequency), AC3ParserParams_p->ObjectID);

                }

                if(AC3ParserParams_p->StartFramePTSValid)
                {
                    AC3ParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                    AC3ParserParams_p->OpBlk_p->Data[PTS_OFFSET]      = AC3ParserParams_p->StartFramePTS;
                    AC3ParserParams_p->OpBlk_p->Data[PTS33_OFFSET]    = AC3ParserParams_p->StartFramePTS33;
                    AC3ParserParams_p->StartFramePTSValid = FALSE;
                }

                AC3ParserParams_p->FrameSize = AC3FrameSizes[index1][frame_size_scode];

                if(samp_freq_code == 1)
                {
                    AC3ParserParams_p->FrameSize = AC3ParserParams_p->FrameSize + 2*(AC3ParserParams_p->HDR[2] & 0x1);
                }
                else if(samp_freq_code == 2)
                {
                    AC3ParserParams_p->FrameSize = AC3ParserParams_p->FrameSize + (AC3ParserParams_p->FrameSize >> 1);
                }
                AC3ParserParams_p->NBSampleOut = 1536;
                AC3ParserParams_p->NBSampleOut_Current = 1536;
                AC3ParserParams_p->Audio_FrameSize = AC3ParserParams_p->FrameSize;
                AC3ParserParams_p->LastStreamType = I0;
                if(AC3ParserParams_p->NBSampleOut)
                {
                    AC3ParserParams_p->BitRate = ((AC3ParserParams_p->FrameSize * AC3ParserParams_p->Frequency)/(AC3ParserParams_p->NBSampleOut * 1000));
                }
                if(AC3ParserParams_p->DCV71)
                {
                    AC3ParserParams_p->CheckForD0 = TRUE;
                }

                AC3ParserParams_p->NoBytesRemainingInFrame = AC3ParserParams_p->FrameSize- AC3ParserParams_p->NoBytesCopied;
                AC3ParserParams_p->AC3ParserState =ES_AC3_FRAME;

            }
            break;

        case ES_AC3_PLUS_FSCOD:
            {
                U8  fscod;
                U32 NbSamples;
                U32 Frequency;
                U8  strmtyp;
                U8  substreamid;

                strmtyp      = AC3ParserParams_p->HDR[0] >> 6 ;
                substreamid  = (AC3ParserParams_p->HDR[0]&0x38) >> 3;

                {
                    U8 ACMode = (AC3ParserParams_p->HDR[2] >> 1) & 0x7;

                    if(AC3ParserParams_p->Audio_Coding_Mode != ACMode)
                    {
                        AC3ParserParams_p->Audio_Coding_Mode = ACMode;
                        /*Notify routing event*/
                        Error=STAudEVT_Notify(AC3ParserParams_p->EvtHandle,AC3ParserParams_p->EventIDNewRouting, &AC3ParserParams_p->Audio_Coding_Mode, sizeof(AC3ParserParams_p->Audio_Coding_Mode), AC3ParserParams_p->ObjectID);
                    }

                }
                //AC3ParserParams_p->Audio_Coding_Mode = (AC3ParserParams_p->HDR[2] >> 1) & 0x7;

                if(!AC3ParserParams_p->DCV71)
                {
                    if(strmtyp || substreamid)
                    {
                        //AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
                        AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                        AC3ParserParams_p->AC3ParserState = ES_AC3_SKIP;
                        AC3ParserParams_p->FrameSize = ((((AC3ParserParams_p->HDR[0])&0x7) << 8 ) + AC3ParserParams_p->HDR[1]);
                        AC3ParserParams_p->FrameSize += 1; /* Length will be from 1 to 2048 (2 byte)*/
                        AC3ParserParams_p->FrameSize = AC3ParserParams_p->FrameSize << 1;
                        AC3ParserParams_p->ByteToSkip = AC3ParserParams_p->FrameSize - AC3ParserParams_p->NoBytesCopied;
                        STTBX_Print((" ES_AC3 ERROR in STREAM_TYPE OR ID Only Type 0 and ID 0 supported \n"));
                        break;
                    }

                }
                else
                {
                    if((strmtyp == 3) || (substreamid) ||((AC3ParserParams_p->LastStreamType == D0)&&(strmtyp == 1)))
                    {
                        AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                        AC3ParserParams_p->AC3ParserState = ES_AC3_SKIP;
                        AC3ParserParams_p->FrameSize = ((((AC3ParserParams_p->HDR[0])&0x7) << 8 ) + AC3ParserParams_p->HDR[1]);
                        AC3ParserParams_p->FrameSize += 1; /* Length will be from 1 to 2048 (2 byte)*/
                        AC3ParserParams_p->FrameSize = AC3ParserParams_p->FrameSize << 1;
                        AC3ParserParams_p->ByteToSkip = AC3ParserParams_p->FrameSize - AC3ParserParams_p->NoBytesCopied;
                        AC3ParserParams_p->LastStreamType = D0;
                        STTBX_Print((" ES_AC3 ERROR in STREAM_TYPE OR ID Only Type 0 and ID 0 supported \n"));
                        break;
                    }

                }

                AC3ParserParams_p->LastStreamType = strmtyp;
                if((AC3ParserParams_p->LastStreamType != D0) && (AC3ParserParams_p->CheckForD0 == TRUE))
                {
                    AC3ParserParams_p->CheckForD0 = FALSE;
                    AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                    AC3ParserParams_p->NoBytesCopied = 0;
                    AC3ParserParams_p->FrameSize = 0;
                    AC3ParserParams_p->AC3ParserState = ES_AC3_FRAME;
                    AC3ParserParams_p->PreParseHeader = TRUE;
                    break;
                }

                AC3ParserParams_p->FrameSize = ((((AC3ParserParams_p->HDR[0])&0x7) << 8 ) + AC3ParserParams_p->HDR[1]);
                AC3ParserParams_p->FrameSize += 1; /* Length will be from 1 to 2048 (2 byte)*/
                AC3ParserParams_p->FrameSize = AC3ParserParams_p->FrameSize << 1;
                if(AC3ParserParams_p->LastStreamType != D0)
                {
                    fscod = (AC3ParserParams_p->HDR[2] >> 6) & 0x3;
                    NbSamples = ((AC3ParserParams_p->HDR[2] >> 4) & 0x3);
                    //STTBX_Print((" Nblock for frame %d = %d\n",AC3ParserParams_p->Blk_Out,NbSamples));
                    if (fscod == 3)
                    {
                        Frequency = AC3Frequency[NbSamples] >> 1;
                        NbSamples = 1536;
                    }
                    else
                    {
                        Frequency = AC3Frequency[fscod];
                        NbSamples = (NbSamples == 3)? 1536 : 256 * (NbSamples + 1);
                    }
                    AC3ParserParams_p->NBSampleOut += NbSamples;
                    if(AC3ParserParams_p->DEC71 == TRUE)
                    {
                        AC3ParserParams_p->NBSampleOut = 1536;//Do not accumulate frame for conv synch: deliver each dd+ frame.
                    }

                    if(AC3ParserParams_p->DCV71 && (AC3ParserParams_p->NBSampleOut==1536))
                    {
                        AC3ParserParams_p->CheckForD0 = TRUE;
                    }
                    AC3ParserParams_p->NBSampleOut_Current = NbSamples;
                    if(AC3ParserParams_p->NBSampleOut > 1536)
                    {
                        AC3ParserParams_p->NBSampleOut -= NbSamples;
                        AC3ParserParams_p->AC3OutOfSync   = TRUE;
                        AC3ParserParams_p->SyncFoundOnFirstFrame = TRUE;
                        AC3ParserParams_p->OutOfSync = TRUE;
                        AC3ParserParams_p->Current_Write_Ptr1-=AC3ParserParams_p->NoBytesCopied;
                        AC3ParserParams_p->NoBytesCopied = 0;
                        AC3ParserParams_p->FrameSize = 0;
                    }
                    /*For ddplus/HDMI the frequency must be four time*/
                    Frequency = Frequency * 4;
                    if(AC3ParserParams_p->Frequency != Frequency)
                    {
                        AC3ParserParams_p->OpBlk_p->Flags   |= FREQ_VALID;
                        AC3ParserParams_p->OpBlk_p->Data[FREQ_OFFSET]    = Frequency;
                        AC3ParserParams_p->Frequency = Frequency;
                        Error=STAudEVT_Notify(AC3ParserParams_p->EvtHandle,AC3ParserParams_p->EventIDFrequencyChange, &AC3ParserParams_p->Frequency, sizeof(AC3ParserParams_p->Frequency), AC3ParserParams_p->ObjectID);
                    }
                    AC3ParserParams_p->Frequency = Frequency;
                }
                else
                {
                    AC3ParserParams_p->CheckForD0 = FALSE;
                }

                if(AC3ParserParams_p->StartFramePTSValid)
                {
                    AC3ParserParams_p->OpBlk_p->Flags   |= PTS_VALID;
                    AC3ParserParams_p->OpBlk_p->Data[PTS_OFFSET]      = AC3ParserParams_p->StartFramePTS;
                    AC3ParserParams_p->OpBlk_p->Data[PTS33_OFFSET]    = AC3ParserParams_p->StartFramePTS33;
                    AC3ParserParams_p->StartFramePTSValid = FALSE;
                }

                AC3ParserParams_p->Audio_FrameSize += AC3ParserParams_p->FrameSize;
                if(AC3ParserParams_p->NBSampleOut)
                {
                    AC3ParserParams_p->BitRate = ((AC3ParserParams_p->FrameSize * AC3ParserParams_p->Frequency)/(AC3ParserParams_p->NBSampleOut * 1000));
                }

                AC3ParserParams_p->NoBytesRemainingInFrame = AC3ParserParams_p->FrameSize- AC3ParserParams_p->NoBytesCopied;
                AC3ParserParams_p->AC3ParserState =ES_AC3_FRAME;
            }
            break;

        case ES_AC3_SKIP:
            if(AC3ParserParams_p->ByteToSkip)
            {
                AC3ParserParams_p->ByteToSkip--;
                offset++;
            }
            else
            {
                AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
            }
            break;
        case ES_AC3_FRAME:
            if((Size-offset)>=AC3ParserParams_p->NoBytesRemainingInFrame)
            {
                /* 1 or more frame */
                memcpy(AC3ParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),AC3ParserParams_p->NoBytesRemainingInFrame);

                AC3ParserParams_p->OutOfSync = TRUE;
                if((AC3ParserParams_p->OutOfSync == TRUE)&&(AC3ParserParams_p->AC3OutOfSync == FALSE)&&(AC3ParserParams_p->DDPFrame == TRUE)&&(AC3ParserParams_p->LastStreamType == I0)&&(AC3ParserParams_p->FrameSize != 0))
                {
                    AC3ParserParams_p->DDPFirstSyncFrame++;
                    SearchConvSync(AC3ParserParams_p);
                }
                if(((AC3ParserParams_p->NBSampleOut == 1536)&&(AC3ParserParams_p->CheckForD0 == FALSE))||(AC3ParserParams_p->FrameSize==0)||((AC3ParserParams_p->OutOfSync == FALSE)&&(AC3ParserParams_p->SyncFoundOnFirstFrame == FALSE)))
                {
                    AC3ParserParams_p->CheckForD0 = FALSE;
                    AC3ParserParams_p->FrameComplete = TRUE;
                    if(AC3ParserParams_p->SyncFoundOnFirstFrame == FALSE)
                    {
                        AC3ParserParams_p->Current_Write_Ptr1 -= AC3ParserParams_p->NoBytesCopied;
                        memcpy(&(AC3ParserParams_p->Temp_Buffer),AC3ParserParams_p->Current_Write_Ptr1,AC3ParserParams_p->FrameSize);
                        AC3ParserParams_p->Audio_FrameSize -= AC3ParserParams_p->FrameSize;
                    }

                    AC3ParserParams_p->OpBlk_p->Size = AC3ParserParams_p->Audio_FrameSize;
                    if(AC3ParserParams_p->Get_Block == TRUE)
                    {
                        if(AC3ParserParams_p->OpBlk_p->Flags & PTS_VALID)
                        {
                            /* Get the PTS in the correct structure */
                            NotifyPTS.Low  = AC3ParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                            NotifyPTS.High = AC3ParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                        }
                        else
                        {
                            NotifyPTS.Low  = 0;
                            NotifyPTS.High = 0;
                        }

                        NotifyPTS.Interpolated  = FALSE;

                        if(AC3ParserParams_p->Skip == 0)
                        {
                            Error = MemPool_PutOpBlk(AC3ParserParams_p->OpBufHandle,(U32 *)&AC3ParserParams_p->OpBlk_p);
                            if(Error != ST_NO_ERROR)
                            {
                                STTBX_Print((" ES_AC3 Error in Memory Put_Block \n"));
                                return Error;
                            }
                            else
                            {
                                Error = STAudEVT_Notify(AC3ParserParams_p->EvtHandle,AC3ParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), AC3ParserParams_p->ObjectID);
                                STTBX_Print((" AC3 Deli:%d\n",AC3ParserParams_p->Blk_Out));
                                AC3ParserParams_p->Blk_Out++;
                                //printf("Size of frame[%d] = %d\n",AC3ParserParams_p->Blk_Out,AC3ParserParams_p->OpBlk_p->Size);
                                AC3ParserParams_p->Reuse = FALSE;
                                AC3ParserParams_p->Put_Block = TRUE;
                                AC3ParserParams_p->Get_Block = FALSE;
                            }
                        }
                        else
                        {
                            AC3ParserParams_p->Skip --;
                            AC3ParserParams_p->Reuse = TRUE;
                            AC3ParserParams_p->Put_Block = TRUE;
                            AC3ParserParams_p->Get_Block = FALSE;
                        }
                    }
                }
                else
                {
                    AC3ParserParams_p->Current_Write_Ptr1 += AC3ParserParams_p->NoBytesRemainingInFrame;
                }

                offset +=  AC3ParserParams_p->NoBytesRemainingInFrame;
                AC3ParserParams_p->NoBytesRemainingInFrame = 0;

                if((AC3ParserParams_p->AC3OutOfSync == FALSE)&&(AC3ParserParams_p->PreParseHeader == FALSE))
                {
                    AC3ParserParams_p->AC3ParserState = ES_AC3_SYNC_0;
                    AC3ParserParams_p->NoBytesCopied = 0;
                }
                else
                {
                    AC3ParserParams_p->PreParseHeader = FALSE;
                    AC3ParserParams_p->AC3ParserState = ES_AC3_FSCOD_FOR_SYNC;
                }

            }
            else
            {
                /* less than a frame */
                U32 SizeToCopy = (Size - offset);
                AC3ParserParams_p->FrameComplete = FALSE;

                memcpy(AC3ParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),SizeToCopy);
                AC3ParserParams_p->Current_Write_Ptr1 += SizeToCopy;
                AC3ParserParams_p->NoBytesRemainingInFrame -= SizeToCopy;
                offset =  Size;
                AC3ParserParams_p->NoBytesCopied = 0;

                AC3ParserParams_p->AC3ParserState = ES_AC3_FRAME;//ES_AC3_PARTIAL_FRAME;
            }
            break;
         default:
            break;
        }
    }
    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_AC3_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p)
{

    ES_AC3_ParserLocalParams_t *AC3ParserParams_p = (ES_AC3_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, AC3ParserParams_p);
    }

    return ST_NO_ERROR;

}

ST_ErrorCode_t  ES_AC3_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{
    ES_AC3_ParserLocalParams_t *AC3ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(AC3ParserLocalParams_p->Frequency)
    {
        SynchroUnit_p->SkipPrecision = ((1536 * 1000)/AC3ParserLocalParams_p->Frequency);
        SynchroUnit_p->PausePrecision = 1;
    }
    else
    {
        Error = STAUD_ERROR_DECODER_PREPARING;
    }

    return Error;
}

U32 ES_AC3_GetRemainTime(ParserHandle_t  Handle)
{
    ES_AC3_ParserLocalParams_t *AC3ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Handle;
    U32 Duration = 0;

    if(AC3ParserLocalParams_p)
    {
        switch (AC3ParserLocalParams_p->AC3ParserState)
        {
        case ES_AC3_FRAME:
        case ES_AC3_PARTIAL_FRAME:
            if(AC3ParserLocalParams_p->BitRate)
            {
                Duration = AC3ParserLocalParams_p->NoBytesRemainingInFrame/AC3ParserLocalParams_p->BitRate;
            }
            break;
        default:
            break;
        }
    }

    return Duration;
}
#endif //#ifdef STAUD_INSTALL_AC3


