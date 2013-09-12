/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : parse_mpeg.c
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifdef STAUD_INSTALL_HEAAC
//#define  STTBX_PRINT
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif

#include "parse_heaac.h"
#include "aud_pes_es_parser.h"

#define SEND_COMPLETE_LOAS_CHUNK 1

//static U32 FrameSent = 0;
static ST_ErrorCode_t DeliverLAOSMuxElement(PESES_ParserControl_t *Control_p);
/*#define PES_TO_ES_BY_FDMA */

/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
                        External Data Types
--------------------------------------------------------------------------- */

/*---------------------------------------------------------------------
                    Private Functions
----------------------------------------------------------------------*/
ST_ErrorCode_t HEAAC_Init(PESES_ParserControl_t *Control_p);
ST_ErrorCode_t HEAAC_Stop(PESES_ParserControl_t *Control_p);
ST_ErrorCode_t HEAAC_Start(PESES_ParserControl_t *Control_p);
ST_ErrorCode_t Parse_HEAAC(PESES_ParserControl_t *Control_p);
ST_ErrorCode_t GetStreamInfoHEAAC(PESES_ParserControl_t *Control_p, STAUD_StreamInfo_t * StreamInfo);

/************************************************************************************
Name         : HEAAC_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : PESES_ParserControl_t      PESES parser control block

Return Value :
    ST_NO_ERROR                     Success.

 ************************************************************************************/

ST_ErrorCode_t HEAAC_Init(PESES_ParserControl_t *Control_p)
{
    HEAACParserLocalParams_t *HEAACParams_p = &(Control_p->AudioParams.HEAACParams);

    HEAACParams_p->FirstTransfer     = FALSE;
    HEAACParams_p->CurretTransfer    = FALSE;
    HEAACParams_p->CurrentBlockInUse = FALSE;
    HEAACParams_p->LAOSFrameSize     = 0;
    HEAACParams_p->FrameDuration     = 0;
    HEAACParams_p->Frequency         = 0;

    HEAACParams_p->HEAACParserState  = DETECT_LAOS_SYNC0;
    HEAACParams_p->ParserState       = HEAACPES_SYNC_0_00;
    Control_p->PTDTSFlags            = 0;
    Control_p->PTSValidFlag          = FALSE;
    Control_p->PTS                   = 0;
    Control_p->SkipDurationInMs      = 0;


    return ST_NO_ERROR;

}

/************************************************************************************
Name         : HEAAC_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESES_ParserControl_t      PESES parser control block

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t HEAAC_Stop(PESES_ParserControl_t *Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 i;
    HEAACParserLocalParams_t *HEAACParams_p = &(Control_p->AudioParams.HEAACParams);

    for(i = Control_p->Blk_out; i<Control_p->Blk_in_Next;i++)
    {
        U32 Index = i % NUM_NODES_PARSER;
        Control_p->LinearQueue[Index].OpBlk_p->Size = HEAACParams_p->LAOSFrameSize;
        Control_p->LinearQueue[Index].OpBlk_p->Flags  &= ~PTS_VALID;
        Error = MemPool_PutOpBlk(Control_p->OpBufHandle, (U32 *)&Control_p->LinearQueue[Index].OpBlk_p);
    }

    for(i = 0;i < NUM_NODES_PARSER;i++)
    {
        Control_p->LinearQueue[i].Valid        = FALSE;
        Control_p->LinearQueue[i].OpBlk_p  = NULL;
    }

    Control_p->Blk_in      = 0;
    Control_p->Blk_in_Next = 0;
    Control_p->Blk_out     = 0;
    HEAACParams_p->FirstTransfer     = FALSE;
    HEAACParams_p->CurretTransfer    = FALSE;
    HEAACParams_p->CurrentBlockInUse = FALSE;
    HEAACParams_p->LAOSFrameSize     = 0;
    HEAACParams_p->FrameDuration     = 0;
    HEAACParams_p->Frequency         = 0;

    HEAACParams_p->HEAACParserState = DETECT_LAOS_SYNC0;
    HEAACParams_p->ParserState      = HEAACPES_SYNC_0_00;
    Control_p->PTDTSFlags           = 0;
    Control_p->PTSValidFlag         = FALSE;
    Control_p->PTS                  = 0;
    Control_p->SkipDurationInMs     = 0;


    return Error;
}

/************************************************************************************
Name         : HEAAC_Start()

Description  : Starts the Parser.

Parameters   : PESES_ParserControl_t      PESES parser control block

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t HEAAC_Start(PESES_ParserControl_t *Control_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    HEAACParserLocalParams_t *HEAACParams_p = &(Control_p->AudioParams.HEAACParams);

    Control_p->Blk_in      = 0;
    Control_p->Blk_in_Next = 0;
    Control_p->Blk_out     = 0;
    HEAACParams_p->FirstTransfer  = FALSE;
    HEAACParams_p->CurretTransfer = FALSE;
    HEAACParams_p->CurrentBlockInUse = FALSE;
    HEAACParams_p->LAOSFrameSize     = 0;
    HEAACParams_p->FrameDuration     = 0;
    HEAACParams_p->Frequency         = 0;

    HEAACParams_p->HEAACParserState = DETECT_LAOS_SYNC0;
    HEAACParams_p->ParserState      = HEAACPES_SYNC_0_00;
    Control_p->PTDTSFlags           = 0;
    Control_p->PTSValidFlag         = FALSE;
    Control_p->PTS                  = 0;
    Control_p->SkipDurationInMs     = 0;

    return Error;
}

/************************************************************************************
Name         : Parse_HEAAC()

Description  : This is entry function for the parsing  .

Parameters   : void *MemoryStart    Memory Block Start Address
                U32 Size  Size of Memory Block
                U32 *BytesUsed With in this block how many bytes were already used

Return Value :
    ST_NO_ERROR
    ST_SYNC_DETECTED   PES Header Sync word present in the given Data Block
    ST_NO_SYNC_WORD_FOUND No PES Header Sync word present in the given Data Block
************************************************************************************/
ST_ErrorCode_t Parse_HEAAC(PESES_ParserControl_t *Control_p)
{

    U8 *pos;
    U32 offset;
    U32 offsetES = 0;
    U8 value;
    ST_ErrorCode_t Error;
    HEAACParserLocalParams_t *HEAACParams_p = &(Control_p->AudioParams.HEAACParams);
    U32 Size = Control_p->PESBlock.ValidSize;
    offset   = Control_p->BytesUsed;
    pos      = (U8 *)Control_p->PESBlock.StartPointer;

    /* Deliver if Current packet is pending */
    //DeliverMPEGFrame(Control_p);
    //STTBX_Print(("State=%d,size=%d,offset=%d,pos=0x%x\n",HEAACParams_p->ParserState,Size,offset,pos));
    while(offset<Size)
    {
        value=pos[offset];
        if(Control_p->StreamType == STAUD_STREAM_TYPE_ES)
        {
            HEAACParams_p->ParserState = HEAACPES_ES_STREAM;
        }
        switch(HEAACParams_p->ParserState)
        {
            case HEAACPES_SYNC_0_00:
                if(value==0x00)
                    HEAACParams_p->ParserState=HEAACPES_SYNC_1_00;
                offset++;
                break;

            case HEAACPES_SYNC_1_00:
                if(value==0x00)
                    HEAACParams_p->ParserState =HEAACPES_SYNC_2_01;
                else
                    HEAACParams_p->ParserState =HEAACPES_SYNC_0_00;
                offset++;
                break;

            case HEAACPES_SYNC_2_01:
                if(value==0x01)
                {
                    HEAACParams_p->ParserState =HEAACPES_SYNC_3_STREAMID;
                    offset++;
                }
                else
                {
                    HEAACParams_p->ParserState =HEAACPES_SYNC_0_00;
                }
                break;

            case HEAACPES_SYNC_3_STREAMID:
                if(Control_p->StreamID!=0)
                {
                    // Check if stream ID is set else check for default
                    if(Control_p->StreamID!=value)
                    {

                        //STTBX_Print(("MPEG Invalid Stream ID\n"));
                        HEAACParams_p->ParserState = HEAACPES_SYNC_0_00;
                    }
                    else
                        HEAACParams_p->ParserState = HEAACPES_PACKET_LENGTH_1;

                }
                else
                {
                    // As per latest DVB standard ETSI TS 101 154 V1.7.1 HEAAC is no longer a "private stream id"

                    if((value & 0xE0) != 0xC0)
                    {
                        /*Not an NonAC3 frame*/
                        //STTBX_Print(("AC3 Invalid Stream ID\n"));
                        HEAACParams_p->ParserState = HEAACPES_SYNC_0_00;
                    }
                    else
                        HEAACParams_p->ParserState = HEAACPES_PACKET_LENGTH_1;
                }
                offset++;
                break;

            case HEAACPES_PACKET_LENGTH_1:
                HEAACParams_p->PESPackLength1=value;
                HEAACParams_p->ParserState =HEAACPES_PACKET_LENGTH_2;
                offset++;
                break;
            case HEAACPES_PACKET_LENGTH_2:
                HEAACParams_p->PESPackLength2=value;
                HEAACParams_p->PESPacketLength=(HEAACParams_p->PESPackLength1<<8)+HEAACParams_p->PESPackLength2;
                HEAACParams_p->ParserState =HEAACPES_PACKET_RES1;
                STTBX_Print(("PESPacketLength:, %d\n",HEAACParams_p->PESPacketLength));
                offset++;
                break;

            case HEAACPES_PACKET_RES1:
                HEAACParams_p->ParserState =HEAACPES_PACKET_RES2;
                offset++;
                break;

            case HEAACPES_PACKET_RES2:
                Control_p->PTDTSFlags=value;
                HEAACParams_p->ParserState =HEAACPES_PACKET_LENGTH;
                offset++;
                break;

            case HEAACPES_PACKET_LENGTH:
                HEAACParams_p->PESPacketHeaderLength=value;
                if(HEAACParams_p->PESPacketHeaderLength)
                {
                    if((Control_p->PTDTSFlags & 0x80) == 0x80)
                    {
                        if(HEAACParams_p->PESPacketHeaderLength < 5)
                        {
                            STTBX_Print(("\n PES Stream Error"));
                            HEAACParams_p->ParserState =HEAACPES_SYNC_0_00;

                        }
                        else
                        {
                            HEAACParams_p->ParserState =HEAACPES_PTS_1;
                            /*Subtracting 5 bytes for PTS values*/
                            HEAACParams_p->NumberBytesSkipInPESHeader=HEAACParams_p->PESPacketHeaderLength-5;
                        }
                    }
                    else
                    {
                        HEAACParams_p->NumberBytesSkipInPESHeader=HEAACParams_p->PESPacketHeaderLength;
                        HEAACParams_p->ParserState =HEAACPES_SKIPPING_HEADER;
                    }

                }
                else
                {
                    /* There is 0 byte header so ES data will start now */
                    HEAACParams_p->NumberBytesSkipInPESHeader=0;
                    HEAACParams_p->ParserState =HEAACPES_SKIPPING_HEADER;
                }
                STTBX_Print(("PESPacketHeaderLength:, %d\n",HEAACParams_p->PESPacketHeaderLength));
                offset++;
            break;


            case HEAACPES_PTS_1:
                HEAACParams_p->ParserState =HEAACPES_PTS_2;
                HEAACParams_p->PTS[0]=value;
                offset++;
                break;
            case HEAACPES_PTS_2:
                HEAACParams_p->ParserState =HEAACPES_PTS_3;
                HEAACParams_p->PTS[1]=value;
                offset++;
                break;
            case HEAACPES_PTS_3:
                HEAACParams_p->ParserState =HEAACPES_PTS_4;
                HEAACParams_p->PTS[2]=value;
                offset++;
                break;
            case HEAACPES_PTS_4:
                HEAACParams_p->ParserState =HEAACPES_PTS_5;
                HEAACParams_p->PTS[3]=value;
                offset++;
                break;
            case HEAACPES_PTS_5:
                HEAACParams_p->ParserState =HEAACPES_FIRST_VOBU;
                HEAACParams_p->PTS[4]=value;
                /* Add PTS 33 bit */
                Control_p->PTS33Bit = (HEAACParams_p->PTS[0]&0x8)?1:0;
                Control_p->PTS = ((U32)((HEAACParams_p->PTS[0]>>1) & 0x07) << 30) |
                                ((U32)( HEAACParams_p->PTS[1]             ) << 22) |
                                ((U32)((HEAACParams_p->PTS[2] >> 1) & 0x7f) << 15) |
                                ((U32)( HEAACParams_p->PTS[3]             ) <<  7) |
                                ((U32)((HEAACParams_p->PTS[4] >> 1) & 0x7f)      );
                Control_p->PTSValidFlag = TRUE;
                /* Got the PTS Raise An Event to the Application*/

                if(Control_p->Speed != 100)
                {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
/* STCLKRV_GetExtendedSTC not available on 7200*/
#if !defined (ST_7200)
                    STCLKRV_ExtendedSTC_t tempPCR;
                    ST_ErrorCode_t Error1;
                    Error1 = STCLKRV_GetExtendedSTC(Control_p->CLKRV_HandleForSynchro, &tempPCR);
                    STTBX_Print(("HEAAC[%d]:PTS,%u,STC,%u\n",Control_p->Speed,Control_p->PTS,tempPCR.BaseValue));
                    if(Error1 == ST_NO_ERROR)
                    {
                        STCLKRV_ExtendedSTC_t PTS;
                        PTS.BaseValue = Control_p->PTS;
                        PTS.BaseBit32 = (Control_p->PTS33Bit & 0x1);

                        if(EPTS_LATER_THAN(PTS,tempPCR))
                        {
                            U32 PTSSTCDiff = EPTS_DIFF(PTS,tempPCR);
                            U32 Diffms = PTS_TO_MILLISECOND(PTSSTCDiff);
                            if( Diffms > 500)
                            {
                                Diffms = 500;
                            }

                            AUD_TaskDelayMs(Diffms);

                            STTBX_Print(("HEAAC:Delay %d, %d\n",PTS_TO_MILLISECOND(PTSSTCDiff),Diffms));
                            /*Drop the whole PES*/
                        }
                    }
                    else
                    {
                        /*Drop the whole PES*/
                    }
#endif
#endif

                    /*Drop the whole PES Here*/
                    if(HEAACParams_p->PESPacketLength > (U32)(HEAACParams_p->PESPacketHeaderLength + 3))
                    {
                        HEAACParams_p->NumberOfBytesRemainingForCurrentPES = HEAACParams_p->PESPacketLength -2-1-HEAACParams_p->PESPacketHeaderLength+
                                                                                    HEAACParams_p->NumberBytesSkipInPESHeader;
                        STTBX_Print(("HEAAC[%d]:Drop %d bytes\n",Control_p->Speed,HEAACParams_p->NumberOfBytesRemainingForCurrentPES));

                        HEAACParams_p->ParserState=HEAACPES_SKIPPING_PES;
                    }
                    else
                    {
                        STTBX_Print(("\n HEAAC Stream Error-- Size Error"));
                      HEAACParams_p->ParserState=HEAACPES_SYNC_0_00;
                    }
                }
                else
                {

                    if(Control_p->AVSyncEnable)
                    {
#if !defined(STAUD_REMOVE_PTI_SUPPORT) && !defined(STAUD_REMOVE_CLKRV_SUPPORT)
/* STCLKRV_GetExtendedSTC not available*/
#if !defined (ST_7200)
                        STCLKRV_ExtendedSTC_t tempPCR;
                        ST_ErrorCode_t Error1;
                        Error1 = STCLKRV_GetExtendedSTC(Control_p->CLKRV_HandleForSynchro, &tempPCR);
                        STTBX_Print(("HEAAC[%d]:PTS,%u,STC,%u\n",Control_p->Speed,Control_p->PTS,tempPCR.BaseValue));
                        if(Error1 == ST_NO_ERROR)
                        {
                            STCLKRV_ExtendedSTC_t PTS;
                            PTS.BaseValue = Control_p->PTS;
                            PTS.BaseBit32 = (Control_p->PTS33Bit & 0x1);

                            if(EPTS_LATER_THAN(tempPCR,PTS) && (EPTS_DIFF(tempPCR,PTS) > PARSER_DROP_THRESHOLD))
                            {
                                /*Drop the whole PES*/
                                if(HEAACParams_p->PESPacketLength > (U32)(HEAACParams_p->PESPacketHeaderLength + 3))
                                {
                                    HEAACParams_p->NumberOfBytesRemainingForCurrentPES = HEAACParams_p->PESPacketLength -2-1-HEAACParams_p->PESPacketHeaderLength+
                                                                                                HEAACParams_p->NumberBytesSkipInPESHeader;
                                    STTBX_Print(("HEAAC[%d]:Drop %d bytes\n",Control_p->Speed,HEAACParams_p->NumberOfBytesRemainingForCurrentPES));

                                    HEAACParams_p->ParserState=HEAACPES_SKIPPING_PES;
                                }
                                else
                                {
                                    STTBX_Print(("\n HEAAC Stream Error-- Size Error"));
                                  HEAACParams_p->ParserState=HEAACPES_SYNC_0_00;
                                }
                            }
                        }
#endif
#endif
                    }//if(Control_p->AVSyncEnable)
                }

                offset++;
                break;

            case HEAACPES_FIRST_VOBU:
                HEAACParams_p->ParserState =HEAACPES_SKIPPING_HEADER;
                break;
            case HEAACPES_SKIPPING_HEADER:
                if(HEAACParams_p->NumberBytesSkipInPESHeader)
                {
                    HEAACParams_p->NumberBytesSkipInPESHeader--;
                    STTBX_Print(("HEAACPES_SKIPPING_HEADER:, %d\n",HEAACParams_p->NumberBytesSkipInPESHeader));
                    offset++;
                }
                else
                {
                    if(HEAACParams_p->PESPacketLength < ((U32)3 + HEAACParams_p->PESPacketHeaderLength))
                    {
                        STTBX_Print(("\n PES Stream Error-- Size Error"));
                        HEAACParams_p->ParserState =HEAACPES_SYNC_0_00;

                    }
                    else
                    {
                        HEAACParams_p->NumberOfBytesRemainingForCurrentPES=HEAACParams_p->PESPacketLength-2-1-HEAACParams_p->PESPacketHeaderLength;/* Error if this (-)*/
                        HEAACParams_p->ParserState =HEAACPES_ES_STREAM;
                    }


                }
                break;

            case HEAACPES_SKIPPING_PES:
                if(HEAACParams_p->NumberOfBytesRemainingForCurrentPES)
                {
                    if((Size-offset)>=HEAACParams_p->NumberOfBytesRemainingForCurrentPES)
                    {

                        /*drop all bytes */
                        offset  +=  HEAACParams_p->NumberOfBytesRemainingForCurrentPES;

                        HEAACParams_p->NumberOfBytesRemainingForCurrentPES = 0;
                    }
                    else
                    {
                        HEAACParams_p->NumberOfBytesRemainingForCurrentPES-= (Size-offset);
                        offset = Size ;
                    }
                }
                else
                {
                    /*Go for pes parsing*/
                    HEAACParams_p->ParserState = HEAACPES_SYNC_0_00;

                }
                break;

            case HEAACPES_ES_STREAM:
                if(Control_p->StreamType==STAUD_STREAM_TYPE_PES)
                {
                    if((Size-offset)>=HEAACParams_p->NumberOfBytesRemainingForCurrentPES)
                    {
                        HEAACParams_p->NumberOfBytesESBytesInCurrentPES = HEAACParams_p->NumberOfBytesRemainingForCurrentPES;

                        HEAACParams_p->NumberOfBytesRemainingForCurrentPES = 0;
                        HEAACParams_p->ParserState = HEAACPES_SYNC_0_00;
                    }
                    else
                    {
                        HEAACParams_p->NumberOfBytesESBytesInCurrentPES = (Size-offset);

                        if(HEAACParams_p->NumberOfBytesRemainingForCurrentPES < HEAACParams_p->NumberOfBytesESBytesInCurrentPES)

                        {
                            HEAACParams_p->ParserState =HEAACPES_SYNC_0_00;
                            offset=Size;
                            break;
                        }
                        else
                        {
                            HEAACParams_p->NumberOfBytesRemainingForCurrentPES -= HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                        }
                    }
                    offsetES=offset;
                    Control_p->ESValidBytes = HEAACParams_p->NumberOfBytesESBytesInCurrentPES+offset;
                /* Get the Free entry in the Current Linear Buffer */
                }
                else if(Control_p->StreamType == STAUD_STREAM_TYPE_ES)
                {
                    offsetES=offset;
                    Control_p->ESValidBytes                                                                    = Size;
                    Control_p->PTSValidFlag                                                                    = FALSE;
                    HEAACParams_p->NumberOfBytesRemainingForCurrentPES  = 0;
                    HEAACParams_p->NumberOfBytesESBytesInCurrentPES     = (Size - offset);
                }

                while(offsetES < Control_p->ESValidBytes)
                {
                    U32 Index = Control_p->Blk_in % NUM_NODES_PARSER;

                    if(HEAACParams_p->CurrentBlockInUse == FALSE)
                    {
                        Error = MemPool_GetOpBlk(Control_p->OpBufHandle,
                                                        (U32 *)&Control_p->LinearQueue[Index].OpBlk_p);
                        if(Error == ST_NO_ERROR)
                        {
                            //STTBX_Print(("MPEG_Parser:Block=%d\n",Control_p->Blk_in));
                            /*free memory block available for linearization*/
                            if(Control_p->Blk_in == 0)
                                STTBX_Print(("MPEG:Got=%d\n",Control_p->Blk_in));
                            Control_p->Blk_in_Next++;
                            //STTBX_Print(("MPEG:Get %d, %x\n",Control_p->Blk_in,&Control_p->LinearQueue[Index].OpBlk_p));
                            HEAACParams_p->CurrentBlockInUse = TRUE;
                            Control_p->LinearQueue[Index].OpBlk_p->Flags = 0;
                            Control_p->src = (U8 *)Control_p->LinearQueue[Index].OpBlk_p->MemoryStart;
                            memset(Control_p->src,0x00,Control_p->LinearQueue[Index].OpBlk_p->MaxSize);
                            if(Control_p->PTSValidFlag == TRUE)
                            {
                                Control_p->PTSValidFlag                                = FALSE;
                                Control_p->LinearQueue[Index].Valid                = FALSE;
                                Control_p->LinearQueue[Index].OpBlk_p->Flags  |= PTS_VALID;
                                Control_p->LinearQueue[Index].OpBlk_p->Data[PTS_OFFSET]    = Control_p->PTS;
                                Control_p->LinearQueue[Index].OpBlk_p->Data[PTS33_OFFSET] = Control_p->PTS33Bit;
                                //STTBX_Print(("PTS=%d,PCR=%d\n",Control_p->PTS/90,PCR/90));

                            }

                        }
                        else
                        {
                            /*No free block available */
                            /*SO sleep*/
                            //STTBX_Print(("MPEG_Parser:Block failure\n"));
                            if(Control_p->Blk_in == 0)
                                STTBX_Print(("MPEG:Fail=%d\n",Control_p->Blk_in));

                            HEAACParams_p->NumberOfBytesRemainingForCurrentPES += HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                            if(HEAACParams_p->NumberOfBytesRemainingForCurrentPES)
                            {
                                HEAACParams_p->ParserState = HEAACPES_ES_STREAM;
                            }

                            Control_p->BytesUsed = offset;
                            return Error;
                        }
                    }

                    value=pos[offsetES];
                    switch(HEAACParams_p->HEAACParserState)
                    {

                        case DETECT_LAOS_SYNC0:
                            if(value==0x56)
                            {
                                HEAACParams_p->HEAACParserState=DETECT_LAOS_SYNC1;
                                STTBX_Print(("LOAS:ES=%d\n",Control_p->Blk_in));
                                HEAACParams_p->HDR[0]=value;
                            }
                            HEAACParams_p->NumberOfBytesESBytesInCurrentPES--;
                            offsetES++;
                            offset++;
                            break;

                        case DETECT_LAOS_SYNC1:
                            if((value & 0xe0)== 0xe0)
                            {

                                HEAACParams_p->HEAACParserState=PARSE_LAOS_MUX_LENGTH;                                  *Control_p->src=value;
                                HEAACParams_p->HDR[1]=value;
                            }

                            HEAACParams_p->NumberOfBytesESBytesInCurrentPES--;
                            offsetES++;
                            offset++;
                            break;

                        case PARSE_LAOS_MUX_LENGTH:
                            {
                                HEAACParams_p->LAOSFrameSize = (U16)((HEAACParams_p->HDR[1] & 0x1F)<<8) + value;
                                HEAACParams_p->NoBytesToCopy = HEAACParams_p->LAOSFrameSize;
                                HEAACParams_p->HDR[2]=value;
                                HEAACParams_p->NumberOfBytesESBytesInCurrentPES--;
                                HEAACParams_p->HEAACParserState=LINEARIZE_LAOS_MUX_DATA;
#if SEND_COMPLETE_LOAS_CHUNK
                                memcpy(Control_p->src, (U8*)(HEAACParams_p->HDR), 3);
                                Control_p->src += 3;
#endif
                                offsetES++;
                                offset++;
                                break;
                            }


                        case LINEARIZE_LAOS_MUX_DATA:
                            if(HEAACParams_p->NoBytesToCopy>HEAACParams_p->NumberOfBytesESBytesInCurrentPES)
                            {
                                /* copy the number of bytes in the Buffer*/
#ifdef PES_TO_ES_BY_FDMA
                                Control_p->NodePESToES_p->Node.NumberBytes = HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                                Control_p->NodePESToES_p->Node.SourceAddress_p = (U8*)((U32)(pos + offsetES) & 0x1FFFFFFF);/* will change depending upon Offset  */
                                Control_p->NodePESToES_p->Node.DestinationAddress_p =(U8*)((U32)Control_p->src & 0x1FFFFFFF); /* will change depening upon ES Buffer*/
                                Control_p->NodePESToES_p->Node.Length = Control_p->NodePESToES_p->Node.NumberBytes;
                                Control_p->PESTOESTransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)((U32)(Control_p->PESTOESTransferParams.NodeAddress_p) & 0x1FFFFFFF);

                                Error=STFDMA_StartGenericTransfer(&Control_p->PESTOESTransferParams,&Control_p->PESTOESTransferID);
                                if(Error!=ST_NO_ERROR)
                                {
                                    return Error;
                                }
#else
                                memcpy(Control_p->src,(pos+offsetES),(U32)HEAACParams_p->NumberOfBytesESBytesInCurrentPES);
#endif
                                Control_p->src+=HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                                HEAACParams_p->NoBytesToCopy=HEAACParams_p->NoBytesToCopy-HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                                HEAACParams_p->NoBytesCopied+=HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                                offset+=HEAACParams_p->NumberOfBytesESBytesInCurrentPES;
                                HEAACParams_p->NumberOfBytesESBytesInCurrentPES=0;
                                offsetES=offset;
                            }
                            else
                            {
#ifdef PES_TO_ES_BY_FDMA

                                Control_p->NodePESToES_p->Node.NumberBytes = HEAACParams_p->NoBytesToCopy;
                                Control_p->NodePESToES_p->Node.SourceAddress_p = (U8* )((U32)(pos+offsetES) & 0x1FFFFFFF);/* will change depending upon Offset  */
                                Control_p->NodePESToES_p->Node.DestinationAddress_p =(U8* )((U32)Control_p->src & 0x1FFFFFFF); /* will change depening upon ES Buffer*/
                                Control_p->NodePESToES_p->Node.Length = Control_p->NodePESToES_p->Node.NumberBytes;
                                Control_p->PESTOESTransferParams.NodeAddress_p = (STFDMA_GenericNode_t *)((U32)Control_p->PESTOESTransferParams.NodeAddress_p & 0x1FFFFFFF);

                                Error=STFDMA_StartGenericTransfer(&Control_p->PESTOESTransferParams,&Control_p->PESTOESTransferID);
                                if(Error!=ST_NO_ERROR)
                                {
                                    return Error;
                                }
#else
                                memcpy(Control_p->src,(pos+offsetES),HEAACParams_p->NoBytesToCopy);
#endif

                                Control_p->src+=HEAACParams_p->NoBytesToCopy;
                                offset+=HEAACParams_p->NoBytesToCopy;
                                HEAACParams_p->NoBytesCopied+=HEAACParams_p->NoBytesToCopy;
                                if(HEAACParams_p->NoBytesCopied<2048)
                                {
                                    //memset(Control_p->src,0,(2048-HEAACParams_p->NoBytesCopied));
                                }
                                else
                                {
                                    HEAACParams_p->NoBytesCopied=2048;
                                }
                                HEAACParams_p->NumberOfBytesESBytesInCurrentPES-=HEAACParams_p->NoBytesToCopy;
                                HEAACParams_p->NoBytesToCopy=0;

                                //if(!(HEAACParams_p->FDMANodePrepared%1000))
                                //  STTBX_Print(("Prepared %d",HEAACParams_p->FDMANodePrepared));

                                Control_p->LinearQueue[Index].OpBlk_p->Size = HEAACParams_p->LAOSFrameSize;
#if SEND_COMPLETE_LOAS_CHUNK
                                Control_p->LinearQueue[Index].OpBlk_p->Size += 3;
#endif

                                if(Control_p->LinearQueue[Index].OpBlk_p->Size > 1152)
                                    STTBX_Print(("MPEG Size too large-->%d\n",Control_p->LinearQueue[Index].OpBlk_p->Size));

                                HEAACParams_p->HEAACParserState = DELIVER_LAOS_MUX_DATA;
                                Control_p->LinearQueue[Index].Valid = TRUE;
                                Control_p->Blk_in++;
                                HEAACParams_p->CurrentBlockInUse = FALSE;
                                HEAACParams_p->HEAACParserState = DELIVER_LAOS_MUX_DATA;
                                offsetES=offset;
                            }
                            break;


                        case DELIVER_LAOS_MUX_DATA:


                            HEAACParams_p->HEAACParserState = DETECT_LAOS_SYNC0;

                            //STTBX_Print(("2:DeliverMPEGFrame:\n"));
                            DeliverLAOSMuxElement(Control_p);
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
    }

    Control_p->BytesUsed = offset;
    return ST_NO_ERROR;
}





/************************************************************************************
Name         : DeliverLAOSMuxElement()

Description  : deliver to the memory manager.

Parameters   :
                Control_p controll block of the timer task
Return Value :
        Error delivery to the memory manager
 ************************************************************************************/
static ST_ErrorCode_t DeliverLAOSMuxElement(PESES_ParserControl_t *Control_p)
{
    U32 Index;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STAUD_PTS_t PTS;
    HEAACParserLocalParams_t *HEAACParams_p = &(Control_p->AudioParams.HEAACParams);

    Index = Control_p->Blk_out % NUM_NODES_PARSER;

    if(Control_p->LinearQueue[Index].Valid == TRUE)
    {
        if(HEAACParams_p->FirstTransfer==FALSE)
        {

            HEAACParams_p->FirstTransfer=TRUE;
        }

        {
            //STTBX_Print(("BlockNO=%d,size=%d,address=0x%x\n",Control_p->Blk_out,Control_p->LinearQueue[Index].OpBlk_p->Size,Control_p->LinearQueue[Index].OpBlk_p->MemoryStart));
            if(Control_p->LinearQueue[Index].OpBlk_p->Flags & PTS_VALID)
            {
                /* Get the PTS in the correct structure */
                PTS.Low  = Control_p->LinearQueue[Index].OpBlk_p->Data[PTS_OFFSET];
                PTS.High    = Control_p->LinearQueue[Index].OpBlk_p->Data[PTS33_OFFSET];
            }
            else
            {
                PTS.Low = 0;
                PTS.High    = 0;
            }

            PTS.Interpolated    = FALSE;


            Error = MemPool_PutOpBlk(Control_p->OpBufHandle,
                                            (U32 *)&Control_p->LinearQueue[Index].OpBlk_p);

            if(Error == ST_NO_ERROR)
            {
                Control_p->LinearQueue[Index].OpBlk_p = NULL;
                if(Control_p->Blk_out % 200 == 199)
                    STTBX_Print(("MPEG:Deli %d\n",Control_p->Blk_out));
                Control_p->Blk_out++;
                // Notify New Frame Event //
                Error = STAudEVT_Notify(Control_p->EvtHandle, Control_p->EventIDNewFrame, &PTS, sizeof(PTS), Control_p->ObjectID);
            }
            else
            {
                STTBX_Print(("MPEG:Deli Failure %x, %x\n",Control_p->Blk_out,&Control_p->LinearQueue[Index].OpBlk_p));
                Error = MemPool_PutOpBlk(Control_p->OpBufHandle,(U32 *)&Control_p->LinearQueue[Index].OpBlk_p);
            }
        }
    }
    return Error;
}



ST_ErrorCode_t GetStreamInfoHEAAC(PESES_ParserControl_t *Control_p, STAUD_StreamInfo_t * StreamInfo)
{
        // StreamInfo->AudioMode=STAUD_STEREO_MODE_STEREO; // Part of Decoder
        /*StreamInfo->BitRate=Control_p->StreamInfo.BitRate;
        StreamInfo->SamplingFrequency=Control_p->StreamInfo.SamplingFrequency;*/
        return ST_ERROR_FEATURE_NOT_SUPPORTED;

}

#endif /*#ifdef STAUD_INSTALL_HEAAC*/

