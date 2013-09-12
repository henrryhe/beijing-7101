/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : PES_MP2_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#include "pes_mp2_parser.h"
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif


/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
                        External Data Types
--------------------------------------------------------------------------- */

/*---------------------------------------------------------------------
                    Private Functions
----------------------------------------------------------------------*/


/************************************************************************************
Name         : PES_MP2_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : ComParserInit_t   *Init_p     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t PES_MP2_Parser_Init(ComParserInit_t *Init_p, PESParser_Handle_t *Handle)

{

    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;
    /*Create All structure required for the all
    Parser*/

    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print((" PES_MP2_ Parser Init Entry \n "));

    /*Init the Required Data Strcuture for the Parser  */
    PES_MP2_ParserParams_p = (PES_MP2_ParserData_t *)STOS_MemoryAllocate(Init_p->Memory_Partition, sizeof(PES_MP2_ParserData_t));

    if(PES_MP2_ParserParams_p==NULL)
    {
        STTBX_Print((" PES_MP2_ Parser Init No Memory \n "));
        return ST_ERROR_NO_MEMORY;
    }
    memset(PES_MP2_ParserParams_p,0,sizeof(PES_MP2_ParserData_t));


    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
    PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags   = 0;
    PES_MP2_ParserParams_p->CurrentPTS.PTSValidFlag  = FALSE;
    PES_MP2_ParserParams_p->CurrentPTS.PTS           = 0;
    PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit      = 0;
    PES_MP2_ParserParams_p->PTS_DIVISION             = 1; //FOR DVB
    PES_MP2_ParserParams_p->StreamID                 = 0;
    PES_MP2_ParserParams_p->Speed                    = 100;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    PES_MP2_ParserParams_p->CLKRV_HandleForSynchro   = Init_p->CLKRV_HandleForSynchro;
#endif

    *Handle=(PESParser_Handle_t)PES_MP2_ParserParams_p;

    STTBX_Print((" PES_MP2_ Parser Init Exit SuccessFull \n "));
    return Error;
}

ST_ErrorCode_t PES_MP2_ParserGetParsingFuction(PESParsingFunction_t* PESParsingFunc)
{
    *PESParsingFunc     =   (PESParsingFunction_t)PES_MP2_Parse_Frame;
    return ST_NO_ERROR;
}

/* Set the Stream Type and Contain to Decoder */
ST_ErrorCode_t PES_MP2_Parser_SetStreamType(PESParser_Handle_t const Handle,STAUD_StreamType_t  NewStream,
                                      STAUD_StreamContent_t  StreamContents)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;


    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t *)Handle;

    if(PES_MP2_ParserParams_p->StreamType!=NewStream)
    {
        /* De allocate the memory basically Linearized frame memory Area*/
        /* and Allocate for the New parser Stream Type */
    }
    PES_MP2_ParserParams_p->StreamType=NewStream;

    PES_MP2_ParserParams_p->StreamContent=StreamContents;

    return ST_NO_ERROR;

}

/* Set BroadcastProfile */
ST_ErrorCode_t PES_MP2_Parser_SetBroadcastProfile(PESParser_Handle_t const Handle,STAUD_BroadcastProfile_t BroadcastProfile)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;

    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t *)Handle;

    if(BroadcastProfile==STAUD_BROADCAST_DVB)
    {
        PES_MP2_ParserParams_p->PTS_DIVISION = 1;
    }
    else if(BroadcastProfile==STAUD_BROADCAST_DIRECTV)
    {
        PES_MP2_ParserParams_p->PTS_DIVISION = 300;
    }
    else
    {
        PES_MP2_ParserParams_p->PTS_DIVISION = 1;
        //FEATURE_NOT_SUPPORTED;
    }

    return ST_NO_ERROR;

}

/* Set the Stream ID and Contain to Decoder */
ST_ErrorCode_t PES_MP2_Parser_SetStreamID(PESParser_Handle_t const Handle, U32  NewStreamID)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;


    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->StreamID=(U8)NewStreamID;

    return ST_NO_ERROR;

}

/*Set the speed of playback*/
ST_ErrorCode_t PES_MP2_SetSpeed(PESParser_Handle_t const Handle, S32 Speed)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p = (PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->Speed = Speed;

    return ST_NO_ERROR;

}
#ifndef STAUD_REMOVE_CLKRV_SUPPORT

/*Set the handle for synchro*/
ST_ErrorCode_t PES_MP2_SetClkSynchro(PESParser_Handle_t const Handle, STCLKRV_Handle_t Clksource)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p = (PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->CLKRV_HandleForSynchro = Clksource;

    return ST_NO_ERROR;
}
#endif

/*Enable/Disable AVSync*/
ST_ErrorCode_t PES_MP2_AVSyncCmd(PESParser_Handle_t const Handle, BOOL AVSyncEnable)
{
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p = (PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->AVSyncEnable = AVSyncEnable;

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : PES_MP2_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_MP2_Parser_Stop(PESParser_Handle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;
    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
    PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_MP2_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_MP2_ParserParams_p->CurrentPTS.PTS=0;
    PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit=0;

    return Error;
}

/************************************************************************************
Name         : PES_MP2_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_MP2_Parser_Start(PESParser_Handle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;
    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t *)Handle;

    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
    PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_MP2_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_MP2_ParserParams_p->CurrentPTS.PTS=0;
    PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit=0;

    return Error;
}


/************************************************************************************
Name         : PES_MP2_Parse_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t PES_MP2_Parse_Frame(PESParser_Handle_t const Handle, void *MemoryStart, U32 Size,U32 *No_Bytes_Used, ParsingFunction_t ES_ParsingFunction,ParserHandle_t ES_Parser_Handle)
{

    PES_MP2_ParserData_t *PES_MP2_ParserParams_p;
        U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;
    U8 private_data_count;

    /*Type cast the Handle */

    PES_MP2_ParserParams_p=(PES_MP2_ParserData_t*)Handle;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    private_data_count=0;
//  STTBX_Print((" PES_MP2_ Parser Enrty \n"));


    while(offset<Size)
    {
        if(PES_MP2_ParserParams_p->StreamType==STAUD_STREAM_TYPE_ES)
        {
            /* If Stream Type is ES then pass to the
            Frame Analyzer of asked type */

            PES_MP2_ParserParams_p->ParserState=PES_MP2_ES_STREAM;

            PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES=(Size-offset);

        }

        value=pos[offset];

        switch(PES_MP2_ParserParams_p->ParserState)
        {
        case PES_MP2_SYNC_0_00:
        //  STTBX_Print((" PES_MP2_ Parser State PES Sync 0 \n "));
            if(value==0x00)
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
            }
            offset++;
            break;

        case PES_MP2_SYNC_1_00:
        //  STTBX_Print((" PES_MP2_ Parser State PES Sync 1 \n"));
            if(value==0x00)
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_2_01;
                offset++;
            }
            else
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
            }
            break;

        case PES_MP2_SYNC_2_01:
        //  STTBX_Print((" PES_MP2_ Parser State PES Sync 2 \n "));
            if(value==0x01)
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_3_STREAMID;
                offset++;
            }
            else if(value==0x00)
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
            }
            else
            {
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
            }
            break;

        case PES_MP2_SYNC_3_STREAMID:

            if(PES_MP2_ParserParams_p->StreamID!=0)
            {
                // Check if stream ID is set else check for default
                if(PES_MP2_ParserParams_p->StreamID!=value)
                {

                    //STTBX_Print(("MPEG Invalid Stream ID\n"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                }
                else
                {
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_LENGTH_1;
                }
            }
            else
            {
                if(((value & 0xE0)==0xC0) || (value==0xBD))
                {
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_LENGTH_1;

                }
                else if(((PES_MP2_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_WMA)||(PES_MP2_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_WMAPROLSL))&&((value==0x70)||(value==0x73))) // p or s
                {
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_LENGTH_1;
                }

                else
                {
                    //STTBX_Print(("PES MP2 Invalid Stream ID\n"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                }


            }
            offset++;
            break;

        case PES_MP2_PACKET_LENGTH_1:
           //STTBX_Print((" PES_MP2_ Parser State PES_MP2_ pack len1=0x%x \n ",value));
            PES_MP2_ParserParams_p->PES_MP2_PackLength1=value;
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_LENGTH_2;
            offset++;
            break;
        case PES_MP2_PACKET_LENGTH_2:
            //STTBX_Print((" PES_MP2_ Parser State PES pack len2=0x%x \n ",value));
            PES_MP2_ParserParams_p->PES_MP2_PackLength2=value;
            PES_MP2_ParserParams_p->PES_MP2_PacketLength=(PES_MP2_ParserParams_p->PES_MP2_PackLength1<<8)+PES_MP2_ParserParams_p->PES_MP2_PackLength2;
            //STTBX_Print((" PES_MP2_ Parser State PES pack len=0x%x \n ",PES_MP2_ParserParams_p->PES_MP2_PacketLength));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_RES1;
            offset++;
            break;

        case PES_MP2_PACKET_RES1:
            {
                //STTBX_Print((" PES_MP2_ Parser State PES res1=0x%x \n ",value));
                if((value & 0xC0) == 0x80)
                {
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_PACKET_RES2;
                }
                else
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
                }
                offset++;
                break;
            }

        case PES_MP2_PACKET_RES2:
            {
                //STTBX_Print((" PES_MP2_ Parser State RES2 PTS DTS Flags=0x%x \n ",value));
                PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags=((value & 0xC0)>>6);
                PES_MP2_ParserParams_p->RES2FLAG = value;
                PES_MP2_ParserParams_p->ParserState=PES_MP2_HEADER_LENGTH;
                offset++;
            break;
            }

        case PES_MP2_HEADER_LENGTH:
            //  STTBX_Print(("PES_MP2_ Parser State Header Length =0x%x ", value));
            PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength=value;
            if(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength)
            {
                if(PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags==0x2) //PTS only
                {
                    if(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength<5)/* 5 Because of Fixed 5 for PTS always*/
                    {
                        STTBX_Print(("\n PES_MP2_ Stream Error"));
                        PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;

                    }
                    else
                    {
                        PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength-5; /* 5 Because of Fixed 5 for PTS always*/
                        PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_1;
                    }
                }
                else if (PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags==0x3) //PTS & DTS
                {
                    if(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength<10)/* 10 Because of Fixed 10 for DTS+PTS always*/
                    {
                        STTBX_Print(("\n PES_MP2_ Stream Error"));
                        PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;

                    }
                    else
                    {
                        PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength-5; /* 5 Because of Fixed 5 for PTS always*/
                        PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_1;
                    }

                }
                else
                {
                    PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength;
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_HEADER_EXT;
                }


            }

            else
            {
                /* There is 0 byte header so ES data will start now */
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=0;
                PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_HEADER;
            }

            offset++;
            break;


        case PES_MP2_PTS_1:
            //STTBX_Print(("PES_MP2_ Parser State PTS 1 =0x%x ", value));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_2;
            PES_MP2_ParserParams_p->PTS[0]=value;
            if(PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags==0x2) //PTS only
            {
                if((value & 0xF0) != 0x20)/* 0010xxxx*/
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
                }
            }
            else //PTS & DTS
            {
                if((value & 0xF0) != 0x30)/* 0011xxxx*/
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
                }
            }
            offset++;
            break;
        case PES_MP2_PTS_2:
            //STTBX_Print(("PES_MP2_ Parser State PTS 2 =0x%x ", value));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_3;
            PES_MP2_ParserParams_p->PTS[1]=value;
            offset++;
            break;
        case PES_MP2_PTS_3:
            //STTBX_Print(("PES_MP2_ Parser State PTS 3 =0x%x ", value));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_4;
            PES_MP2_ParserParams_p->PTS[2]=value;
            offset++;
            break;
        case PES_MP2_PTS_4:
            //STTBX_Print(("PES_MP2_ Parser State PTS 4 =0x%x ", value));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_PTS_5;
            PES_MP2_ParserParams_p->PTS[3]=value;
            offset++;
            break;
        case PES_MP2_PTS_5:
            //STTBX_Print(("PES_MP2_ Parser State PTS 5 =0x%x ", value));
            PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_HEADER;
            PES_MP2_ParserParams_p->PTS[4]=value;
            PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit = (PES_MP2_ParserParams_p->PTS[0]&0x8)?1:0;
            PES_MP2_ParserParams_p->CurrentPTS.PTS = ((U32)((PES_MP2_ParserParams_p->PTS[0]>>1) & 0x07) << 30) |
                              ((U32)( PES_MP2_ParserParams_p->PTS[1]             ) << 22) |
                              ((U32)((PES_MP2_ParserParams_p->PTS[2] >> 1) & 0x7f) << 15) |
                              ((U32)( PES_MP2_ParserParams_p->PTS[3]             ) <<  7) |
                              ((U32)((PES_MP2_ParserParams_p->PTS[4] >> 1) & 0x7f)      );
            PES_MP2_ParserParams_p->CurrentPTS.PTSValidFlag=TRUE;
            if(PES_MP2_ParserParams_p->PTS_DIVISION == 300)
            {
                if(PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit)
                {
                    PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit = 0;
                    PES_MP2_ParserParams_p->CurrentPTS.PTS = ((PES_MP2_ParserParams_p->CurrentPTS.PTS >> 1) | 0x80000000)/150;
                }
                else
                {
                    PES_MP2_ParserParams_p->CurrentPTS.PTS = PES_MP2_ParserParams_p->CurrentPTS.PTS/300;
                }
            }
            //STTBX_Print(("PES_MP2_ Parser State PTS =0x%x ", PES_MP2_ParserParams_p->CurrentPTS.PTS));
            /* Got the PTS Raise An Event to the Application*/
            if((PES_MP2_ParserParams_p->StreamType == STAUD_STREAM_TYPE_PES_AD) && (PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader))
            {
                PES_MP2_ParserParams_p->ParserState =PES_MP2_SKIPPING_HEADER_EXT;
            }
            else
            {
                PES_MP2_ParserParams_p->ParserState =PES_MP2_SKIPPING_HEADER;
            }


            if(PES_MP2_ParserParams_p->Speed != 100)
            {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                STCLKRV_ExtendedSTC_t tempPCR;
                ST_ErrorCode_t Error1;
                Error1 = STCLKRV_GetExtendedSTC(PES_MP2_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                STTBX_Print(("MP2P[%d]:PTS,%u,STC,%u\n",PES_MP2_ParserParams_p->Speed,PES_MP2_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                if(Error1 == ST_NO_ERROR)
                {
                    STCLKRV_ExtendedSTC_t PTS;
                    PTS.BaseValue = PES_MP2_ParserParams_p->CurrentPTS.PTS;
                    PTS.BaseBit32 = (PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);

                    if(EPTS_LATER_THAN(PTS,tempPCR))
                    {
                        U32 PTSSTCDiff = EPTS_DIFF(PTS,tempPCR);
                        U32 Diffms = PTS_TO_MILLISECOND(PTSSTCDiff);
                        if( Diffms > 500)
                        {
                            Diffms = 500;
                        }

                        AUD_TaskDelayMs(Diffms);

                        STTBX_Print(("MP2P:Delay %d, %d\n",PTS_TO_MILLISECOND(PTSSTCDiff),Diffms));
                        /*Drop the whole PES*/
                    }
                }
                else
                {
                    /*Drop the whole PES*/
                }
#endif

                /*Drop the whole PES Here*/
                if(PES_MP2_ParserParams_p->PES_MP2_PacketLength > (U32)(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength + 3))
                {
                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_MP2_ParserParams_p->PES_MP2_PacketLength -2-1-PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength+
                                                                                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader;
                    STTBX_Print(("MP2P[%d]:Drop %d bytes\n",PES_MP2_ParserParams_p->Speed,PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_PES;
                }
                else
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error-- Size Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                }
            }
            else
            {

                if(PES_MP2_ParserParams_p->AVSyncEnable)
                {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                    STCLKRV_ExtendedSTC_t tempPCR;
                    ST_ErrorCode_t Error1;
                    Error1 = STCLKRV_GetExtendedSTC(PES_MP2_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                    STTBX_Print(("MP2P[%d]:PTS,%u,STC,%u\n",PES_MP2_ParserParams_p->Speed,PES_MP2_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                    if(Error1 == ST_NO_ERROR)
                    {
                        STCLKRV_ExtendedSTC_t PTS;
                        PTS.BaseValue = PES_MP2_ParserParams_p->CurrentPTS.PTS;
                        PTS.BaseBit32 = (PES_MP2_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);

                        if(EPTS_LATER_THAN(tempPCR,PTS))
                        {
                            /*Drop the whole PES*/
                            if(PES_MP2_ParserParams_p->PES_MP2_PacketLength > (U32)(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength + 3))
                            {
                                PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_MP2_ParserParams_p->PES_MP2_PacketLength -2-1-PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength+
                                                                                            PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader;
                                STTBX_Print(("MP2P[%d]:Drop %d bytes\n",PES_MP2_ParserParams_p->Speed,PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                                PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_PES;
                            }
                            else
                            {
                                STTBX_Print(("\n PES_MP2_ Stream Error-- Size Error"));
                                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                            }
                        }
                    }
#endif
                }//if(PES_MP2_ParserParams_p->AVSyncEnable)
            }

            offset++;
            break;

        case PES_MP2_SKIPPING_HEADER_EXT:
            {

                PES_MP2_ParserParams_p->Skip = 0;

                if(PES_MP2_ParserParams_p->CurrentPTS.PTSDTSFlags==0x3) //DTS Flag check (5 Byte)
                {
                    PES_MP2_ParserParams_p->Skip += 5;
                }

                if((PES_MP2_ParserParams_p->RES2FLAG & 0x20) == 0x20)//ESCR Flag check (6 Byte)
                {
                    PES_MP2_ParserParams_p->Skip +=6;
                }

                if((PES_MP2_ParserParams_p->RES2FLAG & 0x10) == 0x10)//ES Rate Flag check (3 Byte)
                {
                    PES_MP2_ParserParams_p->Skip +=3;
                }

                if((PES_MP2_ParserParams_p->RES2FLAG & 0x08) == 0x08)//DSM Trick mode check (1 Byte)
                {
                    PES_MP2_ParserParams_p->Skip +=1;
                }

                if((PES_MP2_ParserParams_p->RES2FLAG & 0x04) == 0x04)//Additional copy Flag check (1 Byte)
                {
                    PES_MP2_ParserParams_p->Skip +=1;
                }

                if((PES_MP2_ParserParams_p->RES2FLAG & 0x02) == 0x02)//PES CRC Flag check (2 Byte)
                {
                    PES_MP2_ParserParams_p->Skip +=2;
                }

                //update the remaining header to skip
                if(PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader <  PES_MP2_ParserParams_p->Skip)
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
                }
                else
                {
                    PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader -= PES_MP2_ParserParams_p->Skip;
                    PES_MP2_ParserParams_p->ParserState =PES_MP2_SKIP_EXT;
                }
            }
            break;

        case PES_MP2_SKIP_EXT:
            {
                if(PES_MP2_ParserParams_p->Skip)
                {
                    PES_MP2_ParserParams_p->Skip--;
                    offset++;
                }
                else
                {

                    PES_MP2_ParserParams_p->ParserState = PES_MP2_PACKET_EXT;
                }
            }
            break;

        case PES_MP2_PACKET_EXT:
            {
                if(((PES_MP2_ParserParams_p->RES2FLAG & 0x01) == 0x01)
                    && PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader)//PES Extension Flag check
                {
                    if((value & 0x80) == 0x80) // PES private data flag
                    {
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_PARSE_AD_LENGTH;
                    }
                    else
                    {
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[0] = 0x00; //FADE Value
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[1] = 0x00; //PAN Value
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[5] = 0xFF; //Erro Fade Value
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[6] = 0xFF; //Error PAN Value
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[2] = 1;    // Valid FLAG
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_SKIPPING_HEADER;
                    }
                    offset++;
                    PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--;
                }
                else
                {
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[0] = 0x00; //FADE Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[1] = 0x00; //PAN Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[5] = 0xFF; //Erro Fade Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[6] = 0xFF; //Error PAN Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[2] = 1;    // Valid FLAG
                    PES_MP2_ParserParams_p->ParserState = PES_MP2_SKIPPING_HEADER;
                }
            }
            break;

        case PES_MP2_PARSE_AD_LENGTH:
            {
                if((value == 0xF8)&&(PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader >= 16)) // 1111(reserved) 1000(ADLength)
                {
                    PES_MP2_ParserParams_p->Skip = 6; //text+revision
                    PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader -= PES_MP2_ParserParams_p->Skip;
                    PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--; // skip first byte of AD descriptor
                    offset++;
                    PES_MP2_ParserParams_p->ParserState = PES_MP2_SKIP_AD_TEXT_N_TAG;
                }
                else
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error"));
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[0] = 0x00; //FADE Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[1] = 0x00; //PAN Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[5] = 0xFF; //Erro Fade Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[6] = 0xFF; //Error PAN Value
                    PES_MP2_ParserParams_p->Private_Info.Private_Data[2] = 1;    // Valid FLAG
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SKIPPING_HEADER;
                }
            }
            break;


        case PES_MP2_SKIP_AD_TEXT_N_TAG:
            {
                if(PES_MP2_ParserParams_p->Skip)
                {
                    PES_MP2_ParserParams_p->Skip--;
                    offset++;
                }
                else
                {
                    PES_MP2_ParserParams_p->ParserState = PES_MP2_PARSE_AD_FADE;
                }
            }
            break;

        case PES_MP2_PARSE_AD_FADE:
            {
                PES_MP2_ParserParams_p->Private_Info.Private_Data[0] = value; //FADE Value
                PES_MP2_ParserParams_p->Private_Info.Private_Data[5] = 0x0; //Erro Fade Value
                PES_MP2_ParserParams_p->Private_Info.Private_Data[2] = 1;    // Valid FLAG
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--;
                PES_MP2_ParserParams_p->ParserState = PES_MP2_PARSE_AD_PAN;
                offset++;
            }
            break;

        case PES_MP2_PARSE_AD_PAN:
            {
                PES_MP2_ParserParams_p->Private_Info.Private_Data[1] = value; //PAN Value
                PES_MP2_ParserParams_p->Private_Info.Private_Data[6] = 0x0; //Error PAN Value
                PES_MP2_ParserParams_p->Private_Info.Private_Data[2] = 1;    // Valid FLAG
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--;
                PES_MP2_ParserParams_p->ParserState = PES_MP2_SKIPPING_HEADER;
                offset++;
            }
            break;


        case PES_MP2_SKIPPING_HEADER:
            if(PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader)
            {
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--;
                offset++;
            }
            else
            {
                /* NumberOfBytesRemainingForCurrentPES ==> These are the Bytes which is to be delivered to the
                specific Parser for parsing */

                if(PES_MP2_ParserParams_p->PES_MP2_PacketLength>(U32)(PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength+3))
                {
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_ES_STREAM;

                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES=PES_MP2_ParserParams_p->PES_MP2_PacketLength-2-1-PES_MP2_ParserParams_p->PES_MP2_PacketHeaderLength;/* Error if this (-)*/
                    /* PES Packet 00 00 01 Stream_ID Len1 Len2 Res1 Res2 HeaderLen H1 H2 .. ............ES Data
                                                 <- PES Packet Length ------------------------->
                                                                                <------HeaderLen---->
                                                ES Bytes = PES Len - HeaderLen- 2 (RES) -1 (HeaderLen field)*/

                    switch(PES_MP2_ParserParams_p->StreamContent)
                    {
                    case STAUD_STREAM_CONTENT_WMA:
                    case STAUD_STREAM_CONTENT_WMAPROLSL:
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[4] = (PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES>>8); //Size of WMA data block
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[5] = (PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES&0xFF); //Size of WMA data block
                        break;

                    case STAUD_STREAM_CONTENT_PCM:
                    case STAUD_STREAM_CONTENT_WAVE:
                        PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_ID; /* PCM Private header */
                        break;
                    default:
                        break;
                    }

                }
                else
                {
                    STTBX_Print(("\n PES_MP2_ Stream Error-- Size Error"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                }

            }
            break;

        case PES_MP2_SKIPPING_PES:
            if(PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                if((Size-offset)>=PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
                {

                    /*drop all bytes */
                    offset  +=  PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES;

                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES = 0;


                }
                else
                {
                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES-= (Size-offset);
                    offset = Size ;

                }
            }
            else
            {
                /*Go for pes parsing*/
                PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00;
                PES_MP2_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
            }
            break;
        case PES_SEARCH_PRIVATE_ID:
                switch(value)
                {
                case 0xA3: // PCM sub_stream_id  = 0xA3

                    if(PES_MP2_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_PCM)
                        PES_MP2_ParserParams_p->ParserState = PES_SEARCH_PRIVATE_DATA_0;
                    else
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00; //error in stream type
                    break;

                case 0xA4: // WAVE substream_id = 0xA4
                    if(PES_MP2_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_WAVE)
                        PES_MP2_ParserParams_p->ParserState = PES_SEARCH_PRIVATE_DATA_0;
                    else
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00; //error in stream type
                    break;

                default:
                    PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00; //error in stream type
                    break;
                }
                offset++;
                break;
        case PES_SEARCH_PRIVATE_DATA_0:
            if(PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES>=value)
            {
                switch(PES_MP2_ParserParams_p->StreamContent)
                {
                case STAUD_STREAM_CONTENT_PCM:
                    PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_1;
                    switch (value)
                    {
                    case 0: PES_MP2_ParserParams_p->ParserState=PES_SKIPPING_PRIVATE_HEADER;
                    case 9: PES_MP2_ParserParams_p->Private_Info.Private_Data[0]=value; //Private_header_length
                        PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=value;
                    break;
                    default:
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00; //invalid length
                    break;

                    }
                    break;
                case STAUD_STREAM_CONTENT_WAVE:
                    PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_1;
                    switch (value)
                    {
                    case 0: PES_MP2_ParserParams_p->ParserState=PES_SKIPPING_PRIVATE_HEADER;
                    case 4: PES_MP2_ParserParams_p->Private_Info.Private_Data[0]=value; //Private_header_length
                        PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader=value;
                    break;
                    default:
                        PES_MP2_ParserParams_p->ParserState = PES_MP2_SYNC_0_00; //invalid length

                    }
                break;

                default:
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                break;

                }
            }
            else
            {
                /* insufficient number of bytes in PES packet */
                STTBX_Print(("PES_SEARCH_PRIVATE_ID:Error Case  \n"));

                PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
            }
            private_data_count++;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_1:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_2;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[1]=value;//first_access_unit_pointer
            offset++;
            STTBX_Print(("PES_SEARCH_PRIVATE_DATA_1 \n"));
            break;

        case PES_SEARCH_PRIVATE_DATA_2:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_3;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[2]=value;//first_access_unit_pointer
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_3:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_4;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[3]=value;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_4:
            switch(PES_MP2_ParserParams_p->StreamContent)
            {
            case STAUD_STREAM_CONTENT_PCM:
                PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_5;
                PES_MP2_ParserParams_p->Private_Info.Private_Data[4]=value;
                break;
            case STAUD_STREAM_CONTENT_WAVE:
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader -= 4;
                PES_MP2_ParserParams_p->ParserState=PES_SKIPPING_PRIVATE_HEADER;
                PES_MP2_ParserParams_p->Private_Info.Private_Data[4]=value;
                break;
            default:
                break;
            }
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_5:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_6;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[5]=value;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_6:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_7;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[6]=value;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_7:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_8;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[7]=value;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_8:
            PES_MP2_ParserParams_p->ParserState=PES_SEARCH_PRIVATE_DATA_9;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[8]=value;
            offset++;
            break;
        case PES_SEARCH_PRIVATE_DATA_9:

            PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader -= 9;
            PES_MP2_ParserParams_p->ParserState=PES_SKIPPING_PRIVATE_HEADER;
            PES_MP2_ParserParams_p->Private_Info.Private_Data[9]=value;
            offset++;
            break;

        case PES_SKIPPING_PRIVATE_HEADER:
            if(PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader>0)
            {
                PES_MP2_ParserParams_p->NumberBytesSkipInPESHeader--;
                offset++;
            }
            else
            {
                PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=
                    (PES_MP2_ParserParams_p->Private_Info.Private_Data[0]+2);
                        /* NumberOfBytesRemainingForCurrentPES-=Private_header_length + 1(private header length field) + 1(substream id) */

                if((PES_MP2_ParserParams_p->Private_Info.Private_Data[0])!=0) /* if private header length is not 0 */
                {
                    PES_MP2_ParserParams_p->Private_Info.FirstAccessUnitPtr =
                                ((U16)( PES_MP2_ParserParams_p->Private_Info.Private_Data[1] ) << 8) |
                              ((U16)( PES_MP2_ParserParams_p->Private_Info.Private_Data[2] ));

                    switch(PES_MP2_ParserParams_p->StreamContent)
                    {
                    case STAUD_STREAM_CONTENT_PCM:

                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[0] =
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[3] ) << 24) |
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[4] ) << 16) |
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[5] ) << 8)  |
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[6] )); /* audio sample frequency */

                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[1] =
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[7] ) << 16) |
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[8] ) << 8)  |
                          ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[9] )); /* QWL+No.Audio Channel+Flags+reserved*/
                        break;
                    case STAUD_STREAM_CONTENT_WAVE:
                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[0] =
                            ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[3] ) << 8) |
                            ((U32)( PES_MP2_ParserParams_p->Private_Info.Private_Data[4] )); /* Compression code */
                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[1] = 0;
                        break;
                    default:
                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[0] = 0;
                        PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[1] = 0;
                    break;

                 }

                }
                else
                {
                    int i;

                    for(i=0;i<10;i++)
                    {
                        PES_MP2_ParserParams_p->Private_Info.Private_Data[i]=0;
                    }

                    PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[0] = 0;
                    PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[1] = 0;

                }
                PES_MP2_ParserParams_p->ParserState=PES_MP2_ES_STREAM; /* move for ES parsing */

                if((PES_MP2_ParserParams_p->StreamContent==STAUD_STREAM_CONTENT_WAVE) &&
                    (PES_MP2_ParserParams_p->Private_Info.Private_LPCM_Data[0] != 0x01)) /*if stream compressed then error*/
                {
                    STTBX_Print(("\n PES_MP2_ Compressed WAVE Stream not supported \n"));
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_1_00;
                }

            }
            break;

        case PES_MP2_ES_STREAM:
            if(PES_MP2_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)
            {
                PES_MP2_ParserParams_p->Private_Info.Private_Data[3] = 1; //FOR AAC
            }
            else
            {
                PES_MP2_ParserParams_p->Private_Info.Private_Data[3] = 0; //FOR AAC
            }


    //      STTBX_Print(("Delivering the ES Stream \n"));
            if((Size-offset)>=PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                /* Assuming all are the ES Bytes */
                /* The Specific parser Must return after completing one frame or extended header if there*/
                /* Further PES_MP2_ parser will detect the next packet */
                U32 SizeOfESBytes=0;

                PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES=PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES;
                SizeOfESBytes=offset+PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                /*Send all bytes to the specific Parser */

                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,SizeOfESBytes,&offset,&PES_MP2_ParserParams_p->CurrentPTS,&PES_MP2_ParserParams_p->Private_Info);

                /* to do Error Handling */
                if(Error == ST_NO_ERROR)
                {
                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES=0;
                    PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES=0;
                    PES_MP2_ParserParams_p->ParserState=PES_MP2_SYNC_0_00;
                }
                else
                {
                    if(SizeOfESBytes != (offset+PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES))
                    {
                        PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=(offset - (SizeOfESBytes - PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES));
                    }
                    *No_Bytes_Used=offset;
                    return Error;

                }

            }
            else
            {

                PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES=(Size-offset);

                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,Size,&offset,&PES_MP2_ParserParams_p->CurrentPTS,&PES_MP2_ParserParams_p->Private_Info);

                if(Error == ST_NO_ERROR)
                {

                    PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                }
                else
                {
                    if(PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES !=(Size-offset))
                    {
                        PES_MP2_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=(PES_MP2_ParserParams_p->NumberOfBytesESBytesInCurrentPES - (Size-offset));
                    }
                    *No_Bytes_Used=offset;
                    return Error;
                }
            }

            break;

        default:
            break;

        }
    }

    *No_Bytes_Used=offset;
    return ST_NO_ERROR;
}


ST_ErrorCode_t PES_MP2_Parser_Term(PESParser_Handle_t const Handle,ST_Partition_t   *CPUPartition_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_MP2_ParserData_t *PES_MP2_ParserParams_p = (PES_MP2_ParserData_t *)Handle;

    STTBX_Print(("Terminate PES_MP2_ Parser \n"));

    if(Handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, PES_MP2_ParserParams_p);
    }
    return Error;
}



