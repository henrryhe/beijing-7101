/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : PES_DVDV_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#include "pes_dvdv_parser.h"
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
Name         : PES_DVDV_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : ComParserInit_t   *Init_p     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t PES_DVDV_Parser_Init(ComParserInit_t *Init_p, PESParser_Handle_t *Handle)

{

    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;
    /*Create All structure required for the all
    Parser*/

    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print((" PES_DVDV_ Parser Init Entry \n "));

    /*Init the Required Data Strcuture for the Parser  */
    PES_DVDV_ParserParams_p = (PES_DVDV_ParserData_t *)STOS_MemoryAllocate(Init_p->Memory_Partition, sizeof(PES_DVDV_ParserData_t));
    if(PES_DVDV_ParserParams_p==NULL)
    {
        STTBX_Print((" PES_DVDV_ Parser Init No Memory \n "));
        return ST_ERROR_NO_MEMORY;
    }
    memset(PES_DVDV_ParserParams_p,0,sizeof(PES_DVDV_ParserData_t));

    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit=0;
    PES_DVDV_ParserParams_p->PTS_DIVISION = 1; //FOR DVB
    PES_DVDV_ParserParams_p->MPEG = FALSE;
    PES_DVDV_ParserParams_p->StreamID=0;
    PES_DVDV_ParserParams_p->Speed = 100;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    PES_DVDV_ParserParams_p->CLKRV_HandleForSynchro  = Init_p->CLKRV_HandleForSynchro;
#endif

    *Handle=(PESParser_Handle_t )PES_DVDV_ParserParams_p;

    STTBX_Print((" PES_DVDV_ Parser Init Exit SuccessFull \n "));
    return Error;
}

ST_ErrorCode_t PES_DVDV_ParserGetParsingFuction(PESParsingFunction_t * PESParsingFunc_p)
{
    *PESParsingFunc_p = (PESParsingFunction_t)PES_DVDV_Parse_Frame;
    return ST_NO_ERROR;
}


/* Set the Stream Type and Contain to Decoder */
ST_ErrorCode_t PES_DVDV_Parser_SetStreamType(PESParser_Handle_t const Handle,STAUD_StreamType_t NewStream,
                                            STAUD_StreamContent_t StreamContents)

{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;


    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t *)Handle;

    if(PES_DVDV_ParserParams_p->StreamType!=NewStream)
    {
        /* De allocate the memory basically Linearized frame memory Area*/
        /* and Allocate for the New parser Stream Type */
    }
    PES_DVDV_ParserParams_p->StreamType=NewStream;

    PES_DVDV_ParserParams_p->StreamContent=StreamContents;

    return ST_NO_ERROR;

}

/* Set BroadcastProfile */
ST_ErrorCode_t PES_DVDV_Parser_SetBroadcastProfile(PESParser_Handle_t const Handle,STAUD_BroadcastProfile_t BroadcastProfile)

{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;

    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t *)Handle;

    if(BroadcastProfile==STAUD_BROADCAST_DVB)
    {
        PES_DVDV_ParserParams_p->PTS_DIVISION = 1;
    }
    else if(BroadcastProfile==STAUD_BROADCAST_DIRECTV)
    {
        PES_DVDV_ParserParams_p->PTS_DIVISION = 300;
    }
    else
    {
        PES_DVDV_ParserParams_p->PTS_DIVISION = 1;
    }

    return ST_NO_ERROR;

}

/* Set the Stream ID and Contain to Decoder */
ST_ErrorCode_t PES_DVDV_Parser_SetStreamID(PESParser_Handle_t const Handle, U32  NewStreamID)
{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;


    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->StreamID=(U8)NewStreamID;

    return ST_NO_ERROR;

}

/*Set the speed of playback*/
ST_ErrorCode_t PES_DVDV_SetSpeed(PESParser_Handle_t const Handle, S32 Speed)
{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p = (PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->Speed = Speed;

    return ST_NO_ERROR;

}
#ifndef STAUD_REMOVE_CLKRV_SUPPORT

/*Set the handle for synchro*/
ST_ErrorCode_t PES_DVDV_SetClkSynchro(PESParser_Handle_t const Handle, STCLKRV_Handle_t Clksource)
{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p = (PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->CLKRV_HandleForSynchro = Clksource;

    return ST_NO_ERROR;
}
#endif

/*Enable/Disable AVSync*/
ST_ErrorCode_t PES_DVDV_AVSyncCmd(PESParser_Handle_t const Handle, BOOL AVSyncEnable)
{
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p = (PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->AVSyncEnable = AVSyncEnable;

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : PES_DVDV_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_DVDV_Parser_Stop(PESParser_Handle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;
    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit = 0;

    return Error;
}


/************************************************************************************
Name         : PES_DVDV_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_DVDV_Parser_Start(PESParser_Handle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;
    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t *)Handle;

    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS=0;
    PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit=0;

    return Error;
}

/************************************************************************************
Name         : PES_DVDV_Parse_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t PES_DVDV_Parse_Frame(PESParser_Handle_t const Handle, void *MemoryStart, U32 Size,U32 *No_Bytes_Used, ParsingFunction_t ES_ParsingFunction,ParserHandle_t ES_Parser_Handle)
{

    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p;
        U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;

    /*Type cast the Handle */

    PES_DVDV_ParserParams_p=(PES_DVDV_ParserData_t*)Handle;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    //STTBX_Print((" PES_DVDV_ Parser Enrty \n"));


    while(offset<Size)
    {
        if(PES_DVDV_ParserParams_p->StreamType==STAUD_STREAM_TYPE_ES)
        {
            /* If Stream Type is ES then pass to the
            Frame Analyzer of asked type */

            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_ES_STREAM;

            PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES=(Size-offset);
        }

        value=pos[offset];

        switch(PES_DVDV_ParserParams_p->ParserState)
        {
        case PES_DVDV_SYNC_0_00:
            //  STTBX_Print((" PES_DVDV_ Parser State PES Sync 0 \n "));
            if(value==0x00)
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
            }
            offset++;
            break;

        case PES_DVDV_SYNC_1_00:
            //  STTBX_Print((" PES_DVDV_ Parser State PES Sync 1 \n"));
            if(value==0x00)
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_2_01;
                offset++;
            }
            else
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
            }
            break;

        case PES_DVDV_SYNC_2_01:
            //  STTBX_Print((" PES_DVDV_ Parser State PES Sync 2 \n "));
            if(value==0x01)
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_3_STREAMID;
                offset++;
            }
            else if(value==0x00)
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
            }
            else
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
            }
            break;

        case PES_DVDV_SYNC_3_STREAMID:

            if(PES_DVDV_ParserParams_p->StreamID!=0)
            {
                // Check if stream ID is set else check for default
                if(PES_DVDV_ParserParams_p->StreamID!=value)
                {
                    //STTBX_Print(("MPEG Invalid Stream ID\n"));
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                }
                else
                {
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_LENGTH_1;
                    if((PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)||(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2))
                    {
                        PES_DVDV_ParserParams_p->MPEG = TRUE;
                    }
                }
            }
            else
            {
                if((PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)||(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2))
                {
                    if(((value & 0xF8)  == 0xD0)||((value & 0xF8)  == 0xC0)) //MPEG
                    {
                        PES_DVDV_ParserParams_p->MPEG = TRUE;
                        PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_LENGTH_1;
                    }
                    else
                    {
                        PES_DVDV_ParserParams_p->MPEG = FALSE;
                        PES_DVDV_ParserParams_p->ParserState = PES_DVDV_SYNC_0_00; //error in stream type
                    }
                }
                else if((value & 0xFF) == 0xBD) //Private_stream_1
                {
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_LENGTH_1;
                }
                else
                {
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                }
            }
            offset++;
            break;


        case PES_DVDV_PACKET_LENGTH_1:
            //  STTBX_Print((" PES_DVDV_ Parser State PES_DVDV_ pack len1=0x%x \n ",value));
            PES_DVDV_ParserParams_p->PES_DVDV_PackLength1=value;
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_LENGTH_2;
            offset++;
            break;
        case PES_DVDV_PACKET_LENGTH_2:
            //  STTBX_Print((" PES_DVDV_ Parser State PES pack len2=0x%x \n ",value));
            PES_DVDV_ParserParams_p->PES_DVDV_PackLength2=value;
            PES_DVDV_ParserParams_p->PES_DVDV_PacketLength=(PES_DVDV_ParserParams_p->PES_DVDV_PackLength1<<8)+PES_DVDV_ParserParams_p->PES_DVDV_PackLength2;
            if(PES_DVDV_ParserParams_p->PES_DVDV_PacketLength  > 2034) //Max Packet Length
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                STTBX_Print((" PES_DVDV_ Parser ERROR in PAcket Length \n "));
            }
            else
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_RES1;
            }


            //STTBX_Print((" PES_DVDV_ Parser State PES pack len=0x%x \n ",PES_DVDV_ParserParams_p->PES_DVDV_PacketLength));
            offset++;
            break;

        case PES_DVDV_PACKET_RES1:
            {
                //STTBX_Print((" PES_DVDV_ Parser State PES res1=0x%x \n ",value));
                if((value & 0xCE) == 0x80) // 10xx 000x
                {
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PACKET_RES2;
                }
                else
                {
                    STTBX_Print(("\n PES_DVDV_ Stream Error"));
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
                }
                offset++;
                break;
            }

        case PES_DVDV_PACKET_RES2:
            {
                //  STTBX_Print((" PES_DVDV_ Parser State RES2 PTS DTS Flags=0x%x \n ",value));
                if((value & 0x3E) == 0x0) // xx00 000x
                {
                    PES_DVDV_ParserParams_p->CurrentPTS.PTSDTSFlags=((value & 0xC0)>>6);
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_HEADER_LENGTH;
                }
                else
                {
                    STTBX_Print(("\n PES_DVDV_ Stream Error"));
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
                }

                offset++;
            break;
            }

        case PES_DVDV_HEADER_LENGTH:
            //  STTBX_Print(("PES_DVDV_ Parser State Header Length =0x%x ", value));
            PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength=value;
            if(PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength>15) //Max header length == 15
            {
                STTBX_Print(("\n PES_DVDV_ Stream Error"));
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
            }
            else
            {
                if(PES_DVDV_ParserParams_p->CurrentPTS.PTSDTSFlags==0x2) //PTS only
                {
                    if(PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength<5)/* 5 Because of Fixed 5 for PTS always*/
                    {
                        STTBX_Print(("\n PES_DVDV_ Stream Error"));
                        PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
                    }
                    else
                    {
                        PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader=PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength-5; /* 5 Because of Fixed 5 for PTS always*/
                        PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PTS_1;
                    }
                }
                else
                {
                    PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader=PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength;
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SKIPPING_HEADER;
                }
            }

            offset++;
            break;


        case PES_DVDV_PTS_1:
            //  STTBX_Print(("PES_DVDV_ Parser State PTS 1 =0x%x ", value));
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PTS_2;
            PES_DVDV_ParserParams_p->PTS[0]=value;
            if((value & 0xF0) != 0x20)/* 0010xxxx*/
            {
                STTBX_Print(("\n PES_DVDV_ Stream Error"));
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_1_00;
            }
            offset++;
            break;

        case PES_DVDV_PTS_2:
            //  STTBX_Print(("PES_DVDV_ Parser State PTS 2 =0x%x ", value));
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PTS_3;
            PES_DVDV_ParserParams_p->PTS[1]=value;
            offset++;
            break;

        case PES_DVDV_PTS_3:
            //  STTBX_Print(("PES_DVDV_ Parser State PTS 3 =0x%x ", value));
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PTS_4;
            PES_DVDV_ParserParams_p->PTS[2]=value;
            offset++;
            break;

        case PES_DVDV_PTS_4:
            //  STTBX_Print(("PES_DVDV_ Parser State PTS 4 =0x%x ", value));
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_PTS_5;
            PES_DVDV_ParserParams_p->PTS[3]=value;
            offset++;
            break;

        case PES_DVDV_PTS_5:
            //  STTBX_Print(("PES_DVDV_ Parser State PTS 5 =0x%x ", value));
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SKIPPING_HEADER;
            PES_DVDV_ParserParams_p->PTS[4]=value;
            PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit = (PES_DVDV_ParserParams_p->PTS[0]&0x8)?1:0;
            PES_DVDV_ParserParams_p->CurrentPTS.PTS = ((U32)((PES_DVDV_ParserParams_p->PTS[0]>>1) & 0x07) << 30) |
                              ((U32)( PES_DVDV_ParserParams_p->PTS[1]             ) << 22) |
                              ((U32)((PES_DVDV_ParserParams_p->PTS[2] >> 1) & 0x7f) << 15) |
                              ((U32)( PES_DVDV_ParserParams_p->PTS[3]             ) <<  7) |
                              ((U32)((PES_DVDV_ParserParams_p->PTS[4] >> 1) & 0x7f)      );
            PES_DVDV_ParserParams_p->CurrentPTS.PTSValidFlag=TRUE;

            if(PES_DVDV_ParserParams_p->PTS_DIVISION == 300)
            {
                if(PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit)
                {
                    PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit = 0;
                    PES_DVDV_ParserParams_p->CurrentPTS.PTS = ((PES_DVDV_ParserParams_p->CurrentPTS.PTS >> 1) | 0x80000000)/150;
                }
                else
                {
                    PES_DVDV_ParserParams_p->CurrentPTS.PTS = PES_DVDV_ParserParams_p->CurrentPTS.PTS/300;
                }
            }
            //STTBX_Print(("PES_DVDV_ Parser State PTS =0x%x ", PES_DVDV_ParserParams_p->CurrentPTS.PTS));

            /*We only do this while the speed in not 100*/
            if(PES_DVDV_ParserParams_p->Speed != 100)
            {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                STCLKRV_ExtendedSTC_t tempPCR;
                ST_ErrorCode_t Error1;
                Error1 = STCLKRV_GetExtendedSTC(PES_DVDV_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                STTBX_Print(("DVDVP[%d]:PTS,%u,STC,%u\n",PES_DVDV_ParserParams_p->Speed,PES_DVDV_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                if(Error1 == ST_NO_ERROR)
                {
                    STCLKRV_ExtendedSTC_t PTS;
                    PTS.BaseValue = PES_DVDV_ParserParams_p->CurrentPTS.PTS;
                    PTS.BaseBit32 = (PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);

                    if(EPTS_LATER_THAN(PTS,tempPCR))
                    {
                        U32 PTSSTCDiff = EPTS_DIFF(PTS,tempPCR);
                        U32 Diffms = PTS_TO_MILLISECOND(PTSSTCDiff);
                        if( Diffms > 500)
                        {
                            Diffms = 500;
                        }

                        AUD_TaskDelayMs(Diffms);

                        STTBX_Print(("DVDVP:Delay %d, %d\n",PTS_TO_MILLISECOND(PTSSTCDiff),Diffms));
                        /*Drop the whole PES*/
                    }
                }
                else
                {
                    /*Drop the whole PES*/
                }
#endif

                /*Drop the whole PES Here*/
                if(PES_DVDV_ParserParams_p->PES_DVDV_PacketLength > (U32)(PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength + 3))
                {
                    PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_DVDV_ParserParams_p->PES_DVDV_PacketLength -2-1-PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength+
                                                                                PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader;
                    STTBX_Print(("DVDVP[%d]:Drop %d bytes\n",PES_DVDV_ParserParams_p->Speed,PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SKIPPING_PES;
                }
                else
                {
                    STTBX_Print(("\n PES_DVDV_ Stream Error-- Size Error"));
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                }
            }
            else
            {
                if(PES_DVDV_ParserParams_p->AVSyncEnable)
                {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                    STCLKRV_ExtendedSTC_t tempPCR;
                    ST_ErrorCode_t Error1;
                    Error1 = STCLKRV_GetExtendedSTC(PES_DVDV_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                    STTBX_Print(("DVDVP[%d]:PTS,%u,STC,%u\n",PES_DVDV_ParserParams_p->Speed,PES_DVDV_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                    if(Error1 == ST_NO_ERROR)
                    {
                        STCLKRV_ExtendedSTC_t PTS;
                        PTS.BaseValue = PES_DVDV_ParserParams_p->CurrentPTS.PTS;
                        PTS.BaseBit32 = (PES_DVDV_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);

                        if(EPTS_LATER_THAN(tempPCR,PTS))
                        {
                            /*Drop the whole PES*/
                            if(PES_DVDV_ParserParams_p->PES_DVDV_PacketLength > (U32)(PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength + 3))
                            {
                                PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_DVDV_ParserParams_p->PES_DVDV_PacketLength -2-1-PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength+
                                                                                            PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader;
                                STTBX_Print(("DVDVP[%d]:Drop %d bytes\n",PES_DVDV_ParserParams_p->Speed,PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SKIPPING_PES;
                            }
                            else
                            {
                                STTBX_Print(("\n PES_DVDV_ Stream Error-- Size Error"));
                                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                            }
                        }
                    }
#endif
                }//if(PES_DVDV_ParserParams_p->AVSyncEnable)
            }

            offset++;
            break;

        case PES_DVDV_SKIPPING_HEADER:
            if(PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader)
            {
                PES_DVDV_ParserParams_p->NumberBytesSkipInPESHeader--;
                offset++;
            }
            else
            {
                /* NumberOfBytesRemainingForCurrentPES ==> These are the Bytes which is to be delivered to the
                specific Parser for parsing */
                PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES=PES_DVDV_ParserParams_p->PES_DVDV_PacketLength-2-1-PES_DVDV_ParserParams_p->PES_DVDV_PacketHeaderLength;/* Error if this (-)*/
                /* PES Packet 00 00 01 Stream_ID Len1 Len2 Res1 Res2 HeaderLen H1 H2 .. ............ES Data
                                                 <- PES Packet Length ------------------------->
                                                                                <------HeaderLen---->
                                                ES Bytes = PES Len - HeaderLen- 2 (RES) -1 (HeaderLen field)*/

                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_ID;

            }
            break;

        case PES_DVDV_SKIPPING_PES:
            if(PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                if((Size-offset)>=PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
                {
                    /* Assuming all are the ES Bytes */
                    /* The Specific parser Must return after completing one frame or extended header if there*/
                    /* Further PES_DVDV_ parser will detect the next packet */

                    /*drop all bytes */
                    STTBX_Print(("PES Dropped:%d",PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES));
                    offset  +=  PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES;

                    PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES = 0;


                }
                else
                {
                    PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES-= (Size-offset);
                    STTBX_Print(("PES Dropped:%d",(Size-offset)));
                    offset = Size ;

                }
            }
            else
            {
                /*Go for pes parsing*/
                PES_DVDV_ParserParams_p->ParserState = PES_DVDV_SYNC_0_00;
                PES_DVDV_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;

            }
            break;
        case PES_DVDV_SEARCH_PRIVATE_ID:

            //  packet_info->PesHeaderPos  = packet_info->PacketLength;
            if(PES_DVDV_ParserParams_p->MPEG)
            {
                if((PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG1)||(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG2))
                {
                    PES_DVDV_ParserParams_p->ParserState = PES_DVDV_ES_STREAM;
                }
                else
                {
                    PES_DVDV_ParserParams_p->MPEG = FALSE;
                    PES_DVDV_ParserParams_p->ParserState = PES_DVDV_SYNC_0_00; //error in stream type
                }

            }
            else
            {
                if((((value & 0xF8) == 0xA0)&&(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_LPCM))
                || (((value & 0xF8) == 0x80)&&((PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_AC3)|| (PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_DDPLUS)))
                || (((value & 0xF8) == 0x88)&&(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_DTS )))

                {
                    PES_DVDV_ParserParams_p->ParserState = PES_DVDV_SEARCH_PRIVATE_DATA_0;
                }
                else
                {
                    PES_DVDV_ParserParams_p->ParserState = PES_DVDV_SYNC_0_00; //error in stream type
                }
                offset++;
            }
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_0:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_DATA_1;
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[0]=value;
            offset++;
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_1:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_DATA_2;
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[1]=value;
            offset++;
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_2:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_DATA_3;
            if(PES_DVDV_ParserParams_p->StreamContent != STAUD_STREAM_CONTENT_LPCM)
            {
                PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SET_PRIVATE_DATA;
            }
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[2]=value;
            PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=4; //4 Byte Private data
            offset++;
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_3:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_DATA_4;
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[3]=value;
            offset++;
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_4:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SEARCH_PRIVATE_DATA_5;
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[4]=value;
            offset++;
            break;

        case PES_DVDV_SEARCH_PRIVATE_DATA_5:
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SET_PRIVATE_DATA;
            PES_DVDV_ParserParams_p->Private_Info.Private_Data[5]=value;

            PES_DVDV_ParserParams_p->Private_Info.Private_LPCM_Data[0] = ((U32)( PES_DVDV_ParserParams_p->Private_Info.Private_Data[3] ) << 16) |
                              ((U32)( PES_DVDV_ParserParams_p->Private_Info.Private_Data[4] ) << 8) |
                              ((U32)( PES_DVDV_ParserParams_p->Private_Info.Private_Data[5] ));

            PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=3; // 4+3 Byte Private data FOR LPCM
            offset++;
            break;


        case PES_DVDV_SET_PRIVATE_DATA:
            PES_DVDV_ParserParams_p->Private_Info.FirstAccessUnitPtr = ((U16)( PES_DVDV_ParserParams_p->Private_Info.Private_Data[1] ) << 8) |
                              ((U16)( PES_DVDV_ParserParams_p->Private_Info.Private_Data[2] ));
            PES_DVDV_ParserParams_p->Private_Info.NbFrameHeaders = PES_DVDV_ParserParams_p->Private_Info.Private_Data[0];
            PES_DVDV_ParserParams_p->ParserState=PES_DVDV_ES_STREAM;
            break;

        case PES_DVDV_ES_STREAM:
            if(PES_DVDV_ParserParams_p->StreamContent == STAUD_STREAM_CONTENT_MPEG_AAC)
            {
                PES_DVDV_ParserParams_p->Private_Info.Private_Data[3] = 1; //FOR AAC
            }
            else
            {
                PES_DVDV_ParserParams_p->Private_Info.Private_Data[3] = 0; //FOR AAC
            }

            //STTBX_Print(("Delivering the ES Stream \n"));

            if((Size-offset)>=PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                /* Assuming all are the ES Bytes */
                /* The Specific parser Must return after completing one frame or extended header if there*/
                /* Further PES_DVDV_ parser will detect the next packet */
                U32 SizeOfESBytes=0;

                PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES=PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES;
                SizeOfESBytes=offset+PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                /*Send all bytes to the specific Parser */

                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,SizeOfESBytes,&offset,&PES_DVDV_ParserParams_p->CurrentPTS,&PES_DVDV_ParserParams_p->Private_Info);

                /* to do Error Handling */
                if(Error == ST_NO_ERROR)
                {
                    PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES=0;
                    PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES=0;
                    PES_DVDV_ParserParams_p->ParserState=PES_DVDV_SYNC_0_00;
                }
                else
                {
                    if(SizeOfESBytes != (offset+PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES))
                    {
                        PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=(offset - (SizeOfESBytes - PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES));
                    }
                    *No_Bytes_Used=offset;
                    return Error;
                }

            }
            else
            {
                PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES=(Size-offset);


                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,Size,&offset,&PES_DVDV_ParserParams_p->CurrentPTS,&PES_DVDV_ParserParams_p->Private_Info);

                if(Error == ST_NO_ERROR)
                {

                    PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                }
                else
                {
                    if(PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES !=(Size-offset))
                    {
                        PES_DVDV_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=(PES_DVDV_ParserParams_p->NumberOfBytesESBytesInCurrentPES - (Size-offset));
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

ST_ErrorCode_t PES_DVDV_Parser_Term(PESParser_Handle_t const Handle,ST_Partition_t  *CPUPartition_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_DVDV_ParserData_t *PES_DVDV_ParserParams_p = (PES_DVDV_ParserData_t *)Handle;

    STTBX_Print(("Terminate PES_DVDV_ Parser \n"));

    if(Handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, PES_DVDV_ParserParams_p);
    }

    return Error;
}



