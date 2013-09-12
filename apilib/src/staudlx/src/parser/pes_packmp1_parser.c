/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : PES_PACKMP1_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#include "pes_packmp1_parser.h"
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
Name         : PES_PACKMP1_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : ComParserInit_t   *Init_p     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t PES_PACKMP1_Parser_Init(ComParserInit_t *Init_p, PESParser_Handle_t *Handle)

{

    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;
    /*Create All structure required for the all
    Parser*/

    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print((" PES_PACKMP1_ Parser Init Entry \n "));

    /*Init the Required Data Strcuture for the Parser  */
    PES_PACKMP1_ParserParams_p = (PES_PACKMP1_ParserData_t *)STOS_MemoryAllocate(Init_p->Memory_Partition, sizeof(PES_PACKMP1_ParserData_t));

    if(PES_PACKMP1_ParserParams_p==NULL)
    {
        STTBX_Print((" PES_PACKMP1_ Parser Init No Memory \n "));
        return ST_ERROR_NO_MEMORY;
    }
    memset(PES_PACKMP1_ParserParams_p,0,sizeof(PES_PACKMP1_ParserData_t));

    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit=0;
    PES_PACKMP1_ParserParams_p->PTS_DIVISION = 1; //FOR DVB
    PES_PACKMP1_ParserParams_p->StreamID=0;
    PES_PACKMP1_ParserParams_p->Speed = 100;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT
    PES_PACKMP1_ParserParams_p->CLKRV_HandleForSynchro   = Init_p->CLKRV_HandleForSynchro;
#endif
    *Handle=(PESParser_Handle_t)PES_PACKMP1_ParserParams_p;

    STTBX_Print((" PES_PACKMP1_ Parser Init Exit SuccessFull \n "));
    return Error;
}

ST_ErrorCode_t PES_PACKMP1_ParserGetParsingFuction(PESParsingFunction_t * PESParsingFunc_p)
{
    *PESParsingFunc_p = (PESParsingFunction_t)PES_PACKMP1_Parse_Frame;
    return ST_NO_ERROR;
}

/* Set the Stream Type and Contain to Decoder */
ST_ErrorCode_t PES_PACKMP1_Parser_SetStreamType(PESParser_Handle_t const Handle, STAUD_StreamType_t  NewStream,
                                       STAUD_StreamContent_t StreamContents)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;


    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t *)Handle;

    if(PES_PACKMP1_ParserParams_p->StreamType!=NewStream)
    {
        /* De allocate the memory basically Linearized frame memory Area*/
        /* and Allocate for the New parser Stream Type */
    }
    PES_PACKMP1_ParserParams_p->StreamType=NewStream;

    PES_PACKMP1_ParserParams_p->StreamContent=StreamContents;

    return ST_NO_ERROR;

}

/* Set BroadcastProfile */
ST_ErrorCode_t PES_PACKMP1_Parser_SetBroadcastProfile(PESParser_Handle_t const Handle, STAUD_BroadcastProfile_t BroadcastProfile)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;

    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t *)Handle;

    if(BroadcastProfile==STAUD_BROADCAST_DVB)
    {
        PES_PACKMP1_ParserParams_p->PTS_DIVISION = 1;
    }
    else if(BroadcastProfile==STAUD_BROADCAST_DIRECTV)
    {
        PES_PACKMP1_ParserParams_p->PTS_DIVISION = 300;
    }
    else
    {
        PES_PACKMP1_ParserParams_p->PTS_DIVISION = 1;
    }

    return ST_NO_ERROR;

}

/* Set the Stream ID and Contain to Decoder */
ST_ErrorCode_t PES_PACKMP1_Parser_SetStreamID(PESParser_Handle_t const Handle, U32  NewStreamID)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;

    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t *)Handle;

    PES_PACKMP1_ParserParams_p->StreamID= (U8)NewStreamID;

    return ST_NO_ERROR;

}

/*Set the speed of playback*/
ST_ErrorCode_t PES_PACKMP1_SetSpeed(PESParser_Handle_t const Handle, S32 Speed)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p = (PES_PACKMP1_ParserData_t *)Handle;

    PES_PACKMP1_ParserParams_p->Speed = Speed;

    return ST_NO_ERROR;

}
#ifndef STAUD_REMOVE_CLKRV_SUPPORT

/*Set the handle for synchro*/
ST_ErrorCode_t PES_PACKMP1_SetClkSynchro(PESParser_Handle_t const Handle, STCLKRV_Handle_t Clksource)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p = (PES_PACKMP1_ParserData_t *)Handle;

    PES_PACKMP1_ParserParams_p->CLKRV_HandleForSynchro = Clksource;

    return ST_NO_ERROR;
}
#endif

/*Enable/Disable AVSync*/
ST_ErrorCode_t PES_PACKMP1_AVSyncCmd(PESParser_Handle_t const Handle, BOOL AVSyncEnable)
{
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p = (PES_PACKMP1_ParserData_t *)Handle;

    PES_PACKMP1_ParserParams_p->AVSyncEnable = AVSyncEnable;

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : PES_PACKMP1_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_PACKMP1_Parser_Stop(PESParser_Handle_t const Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;
    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t *)Handle;


    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit=0;

    return Error;
}

/************************************************************************************
Name         : PES_PACKMP1_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t PES_PACKMP1_Parser_Start(PESParser_Handle_t const Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;
    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t *)Handle;

    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSDTSFlags=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS=0;
    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit=0;

    return Error;
}


/************************************************************************************
Name         : PES_PACKMP1_Parse_Frame()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *No_Bytes_Used With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t PES_PACKMP1_Parse_Frame(PESParser_Handle_t const Handle, void *MemoryStart, U32 Size,U32 *No_Bytes_Used, ParsingFunction_t ES_ParsingFunction,ParserHandle_t ES_Parser_Handle)
{

    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p;
        U8 *pos;
    U32 offset;
    U8 value;
    ST_ErrorCode_t Error;


    /*Type cast the Handle */

    PES_PACKMP1_ParserParams_p=(PES_PACKMP1_ParserData_t*)Handle;

    offset=*No_Bytes_Used;

    pos=(U8 *)MemoryStart;

    //STTBX_Print((" PES_PACKMP1_ Parser Enrty \n"));


    while(offset<Size)
    {
        if(PES_PACKMP1_ParserParams_p->StreamType==STAUD_STREAM_TYPE_ES)
        {
            /* If Stream Type is ES then pass to the
            Frame Analyzer of asked type */

            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_ES_STREAM;

            PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES=(Size-offset);
        }

        value=pos[offset];

        switch(PES_PACKMP1_ParserParams_p->ParserState)
        {
        case PES_PACKMP1_SYNC_0_00:
            //STTBX_Print((" PES_PACKMP1_ Parser State PES Sync 0 \n "));
            if(value==0x00)
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_1_00;
            }
            offset++;
            break;

        case PES_PACKMP1_SYNC_1_00:
            //STTBX_Print((" PES_PACKMP1_ Parser State PES Sync 1 \n"));
            if(value==0x00)
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_2_01;
                offset++;
            }
            else
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
            }
            break;

        case PES_PACKMP1_SYNC_2_01:
            //STTBX_Print((" PES_PACKMP1_ Parser State PES Sync 2 \n "));
            if(value==0x01)
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_3_STREAMID;
                offset++;
            }
            else if(value==0x00)
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_1_00;
            }
            else
            {
                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
            }
            break;

        case PES_PACKMP1_SYNC_3_STREAMID:

            if(PES_PACKMP1_ParserParams_p->StreamID!=0)
            {
                // Check if stream ID is set else check for default
                if(PES_PACKMP1_ParserParams_p->StreamID!=value)
                {

                    //STTBX_Print(("MPEG Invalid Stream ID\n"));
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                }
                else
                {
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PACKET_LENGTH_1;
                }
            }
            else
            {
                if((value & 0xE0)==0xC0)
                {
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PACKET_LENGTH_1;

                }
                else
                {
                    //STTBX_Print(("PES PACKMP1 Invalid Stream ID\n"));
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                }


            }
            offset++;
            break;

        case PES_PACKMP1_PACKET_LENGTH_1:
            //STTBX_Print((" PES_PACKMP1_ Parser State PES_PACKMP1_ pack len1=0x%x \n ",value));
            PES_PACKMP1_ParserParams_p->PES_PACKMP1_PackLength1=value;
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PACKET_LENGTH_2;
            offset++;
            break;
        case PES_PACKMP1_PACKET_LENGTH_2:
            //STTBX_Print((" PES_PACKMP1_ Parser State PES pack len2=0x%x \n ",value));
            PES_PACKMP1_ParserParams_p->PES_PACKMP1_PackLength2=value;
            PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength=(PES_PACKMP1_ParserParams_p->PES_PACKMP1_PackLength1<<8)+PES_PACKMP1_ParserParams_p->PES_PACKMP1_PackLength2;
            PES_PACKMP1_ParserParams_p->PesHeaderPos = PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength;
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_STUFFING_BYTE;
            offset++;
            break;

        case PES_PACKMP1_STUFFING_BYTE:

            if(value ==0xFF)
            {
                PES_PACKMP1_ParserParams_p->PesHeaderPos -= 1;
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_STUFFING_BYTE;
                if((PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength - PES_PACKMP1_ParserParams_p->PesHeaderPos)> 16)
                {
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                }
                offset++;
            }
            else
            {
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_STD_BUFFER_0;
            }
            break;


        case PES_PACKMP1_STD_BUFFER_0:

            if((value & 0xC0)  == 0x40) //01xx xxxx
            {
                PES_PACKMP1_ParserParams_p->PesHeaderPos -= 1;
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_STD_BUFFER_1;
                offset++;
            }
            else
            {
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_PTS;
            }
            break;

        case PES_PACKMP1_STD_BUFFER_1:
            PES_PACKMP1_ParserParams_p->PesHeaderPos -= 1;
            PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_PTS;
            offset++;
            break;

        case PES_PACKMP1_PTS:


            if((value & 0xF0)  == 0x20) //0010 xxxx only PTS
            {
                PES_PACKMP1_ParserParams_p->PesHeaderPos -= 5;
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_PTS_1;
                PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader = 0;

            }
            else if((value & 0xF0)  == 0x30) //0011 xxxx only PTS and DTS
            {
                PES_PACKMP1_ParserParams_p->PesHeaderPos -= 10;
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_PTS_1;
                PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader = 5;
            }
            else if((value & 0xFF)  == 0x0F) // 0000 1111 no PTS DTS
            {
                PES_PACKMP1_ParserParams_p->PesHeaderPos -= 1;
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_SKIPPING_HEADER;
                PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader = 0;
                offset++;
            }
            else
            {
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_SYNC_0_00;
            }

            break;


        case PES_PACKMP1_PTS_1:
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS 1 =0x%x ", value));
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PTS_2;
            PES_PACKMP1_ParserParams_p->PTS[0]=value;
            offset++;
            break;

        case PES_PACKMP1_PTS_2:
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS 2 =0x%x ", value));
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PTS_3;
            PES_PACKMP1_ParserParams_p->PTS[1]=value;
            offset++;
            break;

        case PES_PACKMP1_PTS_3:
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS 3 =0x%x ", value));
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PTS_4;
            PES_PACKMP1_ParserParams_p->PTS[2]=value;
            offset++;
            break;

        case PES_PACKMP1_PTS_4:
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS 4 =0x%x ", value));
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_PTS_5;
            PES_PACKMP1_ParserParams_p->PTS[3]=value;
            offset++;
            break;

        case PES_PACKMP1_PTS_5:
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS 5 =0x%x ", value));
            PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SKIPPING_HEADER;
            PES_PACKMP1_ParserParams_p->PTS[4]=value;
            PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit = (PES_PACKMP1_ParserParams_p->PTS[0]&0x8)?1:0;
            PES_PACKMP1_ParserParams_p->CurrentPTS.PTS = ((U32)((PES_PACKMP1_ParserParams_p->PTS[0]>>1) & 0x07) << 30) |
                              ((U32)( PES_PACKMP1_ParserParams_p->PTS[1]             ) << 22) |
                              ((U32)((PES_PACKMP1_ParserParams_p->PTS[2] >> 1) & 0x7f) << 15) |
                              ((U32)( PES_PACKMP1_ParserParams_p->PTS[3]             ) <<  7) |
                              ((U32)((PES_PACKMP1_ParserParams_p->PTS[4] >> 1) & 0x7f)      );
            PES_PACKMP1_ParserParams_p->CurrentPTS.PTSValidFlag=TRUE;
            if(PES_PACKMP1_ParserParams_p->PTS_DIVISION == 300)
            {
                if(PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit)
                {
                    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit = 0;
                    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS = ((PES_PACKMP1_ParserParams_p->CurrentPTS.PTS >> 1) | 0x80000000)/150;
                }
                else
                {
                    PES_PACKMP1_ParserParams_p->CurrentPTS.PTS = PES_PACKMP1_ParserParams_p->CurrentPTS.PTS/300;
                }
            }
            //STTBX_Print(("PES_PACKMP1_ Parser State PTS =0x%x ", PES_PACKMP1_ParserParams_p->CurrentPTS.PTS));

            if(PES_PACKMP1_ParserParams_p->Speed != 100)
            {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                STCLKRV_ExtendedSTC_t tempPCR;
                ST_ErrorCode_t Error1;
                Error1 = STCLKRV_GetExtendedSTC(PES_PACKMP1_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                STTBX_Print(("PACKMP1[%d]:PTS,%u,STC,%u\n",PES_PACKMP1_ParserParams_p->Speed,PES_PACKMP1_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                if(Error1 == ST_NO_ERROR)
                {
                    STCLKRV_ExtendedSTC_t PTS;
                    PTS.BaseValue = PES_PACKMP1_ParserParams_p->CurrentPTS.PTS;
                    PTS.BaseBit32 = (PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);
                    if(EPTS_LATER_THAN(PTS,tempPCR))
                    {
                        U32 PTSSTCDiff = EPTS_DIFF(PTS,tempPCR);
                        U32 Diffms = PTS_TO_MILLISECOND(PTSSTCDiff);
                        if( Diffms > 500)
                        {
                            Diffms = 500;
                        }

                        AUD_TaskDelayMs(Diffms);

                        STTBX_Print(("PACKMP1:Delay %d, %d\n",PTS_TO_MILLISECOND(PTSSTCDiff),Diffms));
                        /*Drop the whole PES*/
                    }
                }
                else
                {
                    /*Drop the whole PES*/
                }
#endif
                /*Drop the whole PES Here*/
                if(PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength > (U32)(PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketHeaderLength + 3))
                {
                    PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength -2-1-PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketHeaderLength+
                                                                                PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader;
                    STTBX_Print(("PACKMP1[%d]:Drop %d bytes\n",PES_PACKMP1_ParserParams_p->Speed,PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SKIPPING_PES;
                }
                else
                {
                    STTBX_Print(("\n PES_PACKMP1_ Stream Error-- Size Error"));
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                }
            }
            else
            {
                if(PES_PACKMP1_ParserParams_p->AVSyncEnable)
                {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
                    STCLKRV_ExtendedSTC_t tempPCR;
                    ST_ErrorCode_t Error1;
                    Error1 = STCLKRV_GetExtendedSTC(PES_PACKMP1_ParserParams_p->CLKRV_HandleForSynchro, &tempPCR);
                    STTBX_Print(("PACKMP1[%d]:PTS,%u,STC,%u\n",PES_PACKMP1_ParserParams_p->Speed,PES_PACKMP1_ParserParams_p->CurrentPTS.PTS,tempPCR.BaseValue));
                    if(Error1 == ST_NO_ERROR)
                    {
                        STCLKRV_ExtendedSTC_t PTS;
                        PTS.BaseValue = PES_PACKMP1_ParserParams_p->CurrentPTS.PTS;
                        PTS.BaseBit32 = (PES_PACKMP1_ParserParams_p->CurrentPTS.PTS33Bit & 0x1);

                        if(EPTS_LATER_THAN(tempPCR,PTS))
                        {
                            /*Drop the whole PES*/
                            if(PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength > (U32)(PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketHeaderLength + 3))
                            {
                                PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES = PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketLength -2-1-PES_PACKMP1_ParserParams_p->PES_PACKMP1_PacketHeaderLength+
                                                                                            PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader;
                                STTBX_Print(("PACKMP1[%d]:Drop %d bytes\n",PES_PACKMP1_ParserParams_p->Speed,PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES));

                                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SKIPPING_PES;
                            }
                            else
                            {
                                STTBX_Print(("\n PES_PACKMP1_ Stream Error-- Size Error"));
                                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                            }
                        }
                    }
#endif
                }//if(PES_PACKMP1_ParserParams_p->AVSyncEnable)
            }

            offset++;
            break;

        case PES_PACKMP1_SKIPPING_HEADER:
            if(PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader)
            {
                PES_PACKMP1_ParserParams_p->NumberBytesSkipInPESHeader--;
                offset++;
            }
            else
            {
                /* NumberOfBytesRemainingForCurrentPES ==> These are the Bytes which is to be delivered to the
                specific Parser for parsing */
                PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES=PES_PACKMP1_ParserParams_p->PesHeaderPos;

                PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_ES_STREAM;

            }
            break;

        case PES_PACKMP1_SKIPPING_PES:
            if(PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                if((Size-offset)>=PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
                {

                    /*drop all bytes */
                    STTBX_Print(("PES Dropped:%d",PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES));
                    offset  +=  PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES;

                    PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES = 0;


                }
                else
                {
                    PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES-= (Size-offset);
                    STTBX_Print(("PES Dropped:%d",(Size-offset)));
                    offset = Size ;

                }
            }
            else
            {
                /*Go for pes parsing*/
                PES_PACKMP1_ParserParams_p->ParserState = PES_PACKMP1_SYNC_0_00;
                PES_PACKMP1_ParserParams_p->CurrentPTS.PTSValidFlag=FALSE;

            }
            break;

        case PES_PACKMP1_ES_STREAM:


            //STTBX_Print(("Delivering the ES Stream \n"));
            if((Size-offset)>=PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES)
            {
                /* Assuming all are the ES Bytes */
                /* The Specific parser Must return after completing one frame or extended header if there*/
                /* Further PES_PACKMP1_ parser will detect the next packet */
                U32 SizeOfESBytes=0;

                PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES=PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES;
                SizeOfESBytes=offset+PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                /*Send all bytes to the specific Parser */

                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,SizeOfESBytes,&offset,&PES_PACKMP1_ParserParams_p->CurrentPTS,&PES_PACKMP1_ParserParams_p->Private_Info);

                /* to do Error Handling */
                if(Error == ST_NO_ERROR)
                {
                    PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES=0;
                    PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES=0;
                    PES_PACKMP1_ParserParams_p->ParserState=PES_PACKMP1_SYNC_0_00;
                }
                else
                {
                    if(SizeOfESBytes != (offset+PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES))
                    {
                        PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES -=(offset - (SizeOfESBytes - PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES));
                    }
                    *No_Bytes_Used=offset;
                    return Error;
                }
            }
            else
            {
                PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES=(Size-offset);


                Error=ES_ParsingFunction(ES_Parser_Handle,MemoryStart,Size,&offset,&PES_PACKMP1_ParserParams_p->CurrentPTS,&PES_PACKMP1_ParserParams_p->Private_Info);

                if(Error == ST_NO_ERROR)
                {

                    PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES;
                }
                else
                {
                    if(PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES !=(Size-offset))
                    {
                        PES_PACKMP1_ParserParams_p->NumberOfBytesRemainingForCurrentPES-=(PES_PACKMP1_ParserParams_p->NumberOfBytesESBytesInCurrentPES - (Size-offset));
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


ST_ErrorCode_t PES_PACKMP1_Parser_Term(PESParser_Handle_t const Handle,ST_Partition_t   *CPUPartition_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    PES_PACKMP1_ParserData_t *PES_PACKMP1_ParserParams_p = (PES_PACKMP1_ParserData_t *)Handle;

    STTBX_Print(("Terminate PES_PACKMP1_ Parser \n"));

    if(Handle)
    {
        STOS_MemoryDeallocate(CPUPartition_p, PES_PACKMP1_ParserParams_p);
    }
    return Error;
}



