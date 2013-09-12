/**********************************************************************************
 *   Filename       : es_aac_parser.c                                             *
 *   Start          : 19.01.2006                                                  *
 *   By             : Rahul Bansal                                                *
 *   Contact        : rahul.bansal@st.com                                         *
 *   Description    : The file contains the parser for ADIF, MP4 file and RAW AAC *
 *                    and its APIs that will be exported to the Modules.          *
 **********************************************************************************
*/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_AAC
//#define  STTBX_PRINT
#include "stos.h"
#include "es_aac_parser.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_evt.h"

/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */
#define AAC_BLOCK_SIZE 4096//1024
/* ----------------------------------------------------------------------------
--------------------------------------------------------------------------- */



/************************************************************************************
Name         : ES_AAC_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
           ES_AAC_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_AAC_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AAC_ParserLocalParams_t * AACParserParams_p;
    AACParserParams_p = STOS_MemoryAllocate(Init_p->Memory_Partition,sizeof(ES_AAC_ParserLocalParams_t));
    if(AACParserParams_p==NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    memset(AACParserParams_p,0,sizeof(ES_AAC_ParserLocalParams_t));
    /* Init the Local Parameters */

    AACParserParams_p->AACParserState = ES_AAC_FRAME;


    AACParserParams_p->FrameSize = AAC_BLOCK_SIZE;
    AACParserParams_p->Frequency = 0xFFFFFFFF;

    AACParserParams_p->Skip      = 0;
    AACParserParams_p->Pause     = 0;
    AACParserParams_p->OpBlk_p   = NULL;

    AACParserParams_p->OpBufHandle = Init_p->OpBufHandle;

    AACParserParams_p->EvtHandle             = Init_p->EvtHandle;
    AACParserParams_p->ObjectID              = Init_p->ObjectID;
    AACParserParams_p->EventIDNewFrame       = Init_p->EventIDNewFrame;
    AACParserParams_p->EventIDFrequencyChange= Init_p->EventIDFrequencyChange;

    *handle=(ParserHandle_t )AACParserParams_p;

    /* to init O/P pointers */
    AACParserParams_p->Blk_Out = 0;
    return (Error);
}

ST_ErrorCode_t ES_AAC_ParserGetParsingFunction( ParsingFunction_t * ParsingFunc_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p)
{

    *ParsingFunc_p      = (ParsingFunction_t)ES_AAC_Parse_Frame;
    *GetFreqFunction_p= (GetFreqFunction_t)ES_AAC_Get_Freq;
    *GetInfoFunction_p= (GetInfoFunction_t)ES_AAC_Get_Info;

    return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_AAC_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AAC_Parser_Stop(ParserHandle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AAC_ParserLocalParams_t *AACParserParams_p;
    AACParserParams_p=(ES_AAC_ParserLocalParams_t *)Handle;

    if(AACParserParams_p->OpBlk_p)
    {
        AACParserParams_p->OpBlk_p->Size = AACParserParams_p->NoBytesCopied;
        AACParserParams_p->OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(AACParserParams_p->OpBufHandle,(U32 *)&AACParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            STTBX_Print((" ES_AAC Error in Memory Put_Block \n"));
        }
        AACParserParams_p->OpBlk_p = NULL;
    }

    return Error;
}

/************************************************************************************
Name         : ES_AAC_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_AAC_Parser_Start(ParserHandle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    ES_AAC_ParserLocalParams_t *AACParserParams_p;
    AACParserParams_p=(ES_AAC_ParserLocalParams_t *)Handle;

    AACParserParams_p->AACParserState    = ES_AAC_FRAME;
    AACParserParams_p->FrameSize         = AAC_BLOCK_SIZE;

    AACParserParams_p->Frequency         = 0xFFFFFFFF;

    AACParserParams_p->Skip              = 0;
    AACParserParams_p->Pause             = 0;
    AACParserParams_p->Blk_Out           = 0;
    AACParserParams_p->OpBlk_p           = NULL;

    return Error;
}


/* Get Sampling Frequency from Parser */

ST_ErrorCode_t ES_AAC_Get_Freq(ParserHandle_t  Handle,U32 *SamplingFrequency_p)
{

/*  ES_AAC_ParserLocalParams_t *AACParserParams_p;
    AACParserParams_p=(ES_AAC_ParserLocalParams_t *)Handle;

    *SamplingFrequency_p=0;*/

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/* Get GetStreamInfo from Parser */

ST_ErrorCode_t ES_AAC_Get_Info(ParserHandle_t  Handle,STAUD_StreamInfo_t * StreamInfo)
{
/*  ES_AAC_ParserLocalParams_t *AACParserParams_p;
    AACParserParams_p=(ES_AAC_ParserLocalParams_t *)Handle;*/

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}
/* handle EOF from Parser */

ST_ErrorCode_t ES_AAC_Handle_EOF(ParserHandle_t  Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ES_AAC_ParserLocalParams_t *AACParserParams_p = (ES_AAC_ParserLocalParams_t *)Handle;

    if (AACParserParams_p->OpBlk_p == NULL)
    {
        Error = MemPool_GetOpBlk(AACParserParams_p->OpBufHandle, (U32 *)&AACParserParams_p->OpBlk_p);
        if(Error != ST_NO_ERROR)
        {
            return STAUD_MEMORY_GET_ERROR;
        }
        else
        {
            AACParserParams_p->Current_Write_Ptr1 = (U8 *)AACParserParams_p->OpBlk_p->MemoryStart;
            AACParserParams_p->OpBlk_p->Flags = 0;

            //mark the alignment tag
            AACParserParams_p->OpBlk_p->Flags   |= DATAFORMAT_VALID;
            AACParserParams_p->OpBlk_p->Data[DATAFORMAT_OFFSET]  = BE;
            AACParserParams_p->NoBytesCopied = 0;

        }

    }

    memset(AACParserParams_p->Current_Write_Ptr1,0x00,(AACParserParams_p->FrameSize - AACParserParams_p->NoBytesCopied));

    AACParserParams_p->OpBlk_p->Size = AACParserParams_p->NoBytesCopied;
    AACParserParams_p->OpBlk_p->Flags  |= EOF_VALID;

    Error = MemPool_PutOpBlk(AACParserParams_p->OpBufHandle,(U32 *)&AACParserParams_p->OpBlk_p);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print((" ES_AAC Error in Memory Put_Block \n"));
    }
    else
    {
        AACParserParams_p->OpBlk_p = NULL;
    }

    return ST_NO_ERROR;
}


/************************************************************************************
Name         : Parse_AAC_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
************************************************************************************/
ST_ErrorCode_t ES_AAC_Parse_Frame(ParserHandle_t const Handle,void *MemoryStart, U32 Size,U32 *No_Bytes_Used,PTS_t * CurrentPTS_p, Private_Info_t * Private_Info_p)
{
    U8 *pos;
    U32 offset;
    ST_ErrorCode_t Error;
    ES_AAC_ParserLocalParams_t *AACParserParams_p;
    STAUD_PTS_t NotifyPTS;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    //STTBX_Print((" ES_AAC Parser Enrty \n"));

    AACParserParams_p=(ES_AAC_ParserLocalParams_t *)Handle;

    while(offset<Size)
    {

        if(AACParserParams_p->OpBlk_p == NULL)
        {
            Error = MemPool_GetOpBlk(AACParserParams_p->OpBufHandle, (U32 *)&AACParserParams_p->OpBlk_p);
            if(Error != ST_NO_ERROR)
            {
                return STAUD_MEMORY_GET_ERROR;
            }
            else
            {
                if(AACParserParams_p->Pause)
                {
                    AUD_TaskDelayMs(AACParserParams_p->Pause);
                    AACParserParams_p->Pause = 0;
                }
            }

            AACParserParams_p->Current_Write_Ptr1 = (U8 *)AACParserParams_p->OpBlk_p->MemoryStart;
            AACParserParams_p->OpBlk_p->Flags = 0;
            if(CurrentPTS_p->PTSValidFlag == TRUE)
            {
                AACParserParams_p->OpBlk_p->Flags   |= PTS_VALID;

                AACParserParams_p->OpBlk_p->Data[PTS_OFFSET]     = CurrentPTS_p->PTS;
                AACParserParams_p->OpBlk_p->Data[PTS33_OFFSET]   = CurrentPTS_p->PTS33Bit;
            }
            CurrentPTS_p->PTSValidFlag      = FALSE;
            if(Private_Info_p->Private_Data[2] == 1) // FADE PAN VALID FLAG
            {
                AACParserParams_p->OpBlk_p->Flags   |= FADE_VALID;
                AACParserParams_p->OpBlk_p->Flags   |= PAN_VALID;
                AACParserParams_p->OpBlk_p->Data[FADE_OFFSET]    = (Private_Info_p->Private_Data[5]<<8)|(Private_Info_p->Private_Data[0]); // FADE Value
                AACParserParams_p->OpBlk_p->Data[PAN_OFFSET] = (Private_Info_p->Private_Data[6]<<8)|(Private_Info_p->Private_Data[1]); // PAN Value
            }

            //mark the alignment tag
            AACParserParams_p->OpBlk_p->Flags   |= DATAFORMAT_VALID;
            AACParserParams_p->OpBlk_p->Data[DATAFORMAT_OFFSET]  = BE;
            AACParserParams_p->NoBytesCopied = 0;

        }

        switch(AACParserParams_p->AACParserState)
        {

        case ES_AAC_FRAME:
            if((Size-offset)>=(AACParserParams_p->FrameSize - AACParserParams_p->NoBytesCopied))
            {
                /* 1 or more frame */
                memcpy(AACParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(AACParserParams_p->FrameSize - AACParserParams_p->NoBytesCopied));
                AACParserParams_p->OpBlk_p->Size = AACParserParams_p->FrameSize;

                if(AACParserParams_p->OpBlk_p->Flags & PTS_VALID)
                {
                    /* Get the PTS in the correct structure */
                    NotifyPTS.Low  = AACParserParams_p->OpBlk_p->Data[PTS_OFFSET];
                    NotifyPTS.High = AACParserParams_p->OpBlk_p->Data[PTS33_OFFSET];
                }
                else
                {
                    NotifyPTS.Low  = 0;
                    NotifyPTS.High = 0;
                }

                NotifyPTS.Interpolated  = FALSE;

                Error = MemPool_PutOpBlk(AACParserParams_p->OpBufHandle,(U32 *)&AACParserParams_p->OpBlk_p);
                if(Error != ST_NO_ERROR)
                {
                    STTBX_Print((" ES_AAC Error in Memory Put_Block \n"));
                    return Error;
                }
                else
                {
                    AACParserParams_p->OpBlk_p = NULL;
                    Error = STAudEVT_Notify(AACParserParams_p->EvtHandle,AACParserParams_p->EventIDNewFrame, &NotifyPTS, sizeof(NotifyPTS), AACParserParams_p->ObjectID);
                    STTBX_Print((" AAC Deli:%d\n",AACParserParams_p->Blk_Out));

                    AACParserParams_p->Blk_Out++;
                }

                offset += (AACParserParams_p->FrameSize - AACParserParams_p->NoBytesCopied);
            }
            else
            {
                /* less than a frame */

                memcpy(AACParserParams_p->Current_Write_Ptr1,((char *)MemoryStart+offset),(Size - offset));
                AACParserParams_p->Current_Write_Ptr1 += (Size - offset);
                AACParserParams_p->NoBytesCopied += (Size - offset);

                offset =  Size;

                AACParserParams_p->AACParserState = ES_AAC_FRAME;
            }
            break;

        default:
            break;
        }

        *No_Bytes_Used=offset;

    }
    return ST_NO_ERROR;
}

ST_ErrorCode_t ES_AAC_Parser_Term(ParserHandle_t *const handle, ST_Partition_t  *CPUPartition_p)
{

    ES_AAC_ParserLocalParams_t *AACParserParams_p = (ES_AAC_ParserLocalParams_t *)handle;

    if(handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, AACParserParams_p);
    }

    return ST_NO_ERROR;

}

ST_ErrorCode_t  ES_AAC_GetSynchroUnit(ParserHandle_t  Handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
}
#endif //#ifdef STAUD_INSTALL_AAC
