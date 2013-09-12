/************************************************************************
COPYRIGHT STMicroelectronics (C) 2005
Source file name : Com_Parser.c
Owner : ST ACC Noida
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#include "stos.h"
#include "com_parser.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#include "stdio.h"
#include "string.h"
#endif
#include "aud_pes_es_parser.h"
#include "pes_mp2_parser.h"
#ifndef ST_51XX
#include "pes_packmp1_parser.h"
#include "pes_dvdv_parser.h"
#include "pes_dvda_parser.h"
#include "es_ac3_parser.h"
#else
#include "es_ac3_parser_amphion.h"
#endif
#include "es_mpeg_parser.h"
#include "es_cdda_parser.h"
#include "es_lpcma_parser.h"
#include "es_lpcmv_parser.h"
#include "es_mlp_parser.h"
#include "es_dts_parser.h"
#include "es_wmapes_parser.h"
#include "es_wave_parser.h"
#include "es_aiff_parser.h"
#include "es_aac_parser.h"

/* ----------------------------------------------------------------------------
                               Private Types
---------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
                        External Data Types
--------------------------------------------------------------------------- */

/*---------------------------------------------------------------------
                    Private Functions
----------------------------------------------------------------------*/
ST_ErrorCode_t Com_Parser_Start(PESES_ParserControl_t *ControlBlock_p);
ST_ErrorCode_t Com_Parse_Frame(PESES_ParserControl_t *ControlBlock_p);
ST_ErrorCode_t Com_Parser_Stop(PESES_ParserControl_t *ControlBlock_p);
ST_ErrorCode_t Com_Parser_Term(PESES_ParserControl_t *ControlBlock_p);

/* ----------------------------------------------------------------------------
                               External Confidentail functions
---------------------------------------------------------------------------- */

#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
extern ST_ErrorCode_t ES_WMA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

extern ST_ErrorCode_t ES_WMA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t   *CPUPartition_p);

extern ST_ErrorCode_t ES_WMA_ParserGetParsingFunction(  ParsingFunction_t   * ParsingFucntion_p,
                                                GetFreqFunction_t * GetFreqFunction_p,
                                                GetInfoFunction_t * GetInfoFunction_p);
extern ST_ErrorCode_t ES_WMA_Parser_Stop(ParserHandle_t const Handle);
extern ST_ErrorCode_t ES_WMA_Parser_Start(ParserHandle_t const Handle);

extern ST_ErrorCode_t ES_WMA_Handle_EOF(ParserHandle_t  Handle);
extern  ST_ErrorCode_t ES_WMA_SetStreamID(ParserHandle_t  Handle, U8 WMAStreamNumber);
#endif

/************************************************************************************
Name         : Com_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : PESParserInit_t   *Init_p     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t Com_Parser_Init(ComParserInit_t *Init_p, ComParser_Handle_t *Handle)

{
    Com_ParserData_t *Com_ParserLocalParams_p;

    /*Create All structure required for the all
    Parser*/

    ST_ErrorCode_t Error = ST_NO_ERROR;

    STTBX_Print((" ComParser Init Entry \n "));
    Com_ParserLocalParams_p = (Com_ParserData_t *)memory_allocate (Init_p->Memory_Partition, sizeof(Com_ParserData_t));

    if(Com_ParserLocalParams_p==NULL)
    {
        STTBX_Print((" Com_ Parser Init No Memory \n "));
        return ST_ERROR_NO_MEMORY;
    }
    memset(Com_ParserLocalParams_p,0,sizeof(Com_ParserData_t));
    /* Init all the pes parsers here and store their handles */
    {
        Error |= PES_MP2_Parser_Init(Init_p, &Com_ParserLocalParams_p->PESParserHandle[PES_MP2]);
#ifndef ST_51XX
        Error |= PES_PACKMP1_Parser_Init(Init_p, &Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1]);
        Error |= PES_DVDV_Parser_Init(Init_p, &Com_ParserLocalParams_p->PESParserHandle[PES_DVDV]);
        Error |= PES_DVDA_Parser_Init(Init_p, &Com_ParserLocalParams_p->PESParserHandle[PES_DVDA]);
#endif
    }
    /* Init all the es parsers here and store their handles */

    {
#ifdef STAUD_INSTALL_MPEG
        Error |= ES_MPEG_Parser_Init( Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_MPEG]);
#endif

#ifdef STAUD_INSTALL_AC3
        Error |= ES_AC3_Parser_Init(  Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_AC3]);
#endif

#ifdef STAUD_INSTALL_LPCMV
        Error |= ES_LPCMV_Parser_Init(Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_LPCMV]);
#endif

#ifdef STAUD_INSTALL_LPCMA
        Error |= ES_LPCMA_Parser_Init(Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_LPCMA]);
#endif

#ifdef STAUD_INSTALL_MLP
        Error |= ES_MLP_Parser_Init(Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_MLP]);
#endif

#ifdef STAUD_INSTALL_DTS
        Error |= ES_DTS_Parser_Init(  Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_DTS]);
#endif

#ifdef STAUD_INSTALL_CDDA
        Error |= ES_CDDA_Parser_Init( Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_CDDA]);
#endif

#ifdef STAUD_INSTALL_WAVE
        Error |= ES_Wave_Parser_Init( Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_WAVE]);
#endif

#ifdef STAUD_INSTALL_AIFF
        Error |= ES_AIFF_Parser_Init( Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_AIFF]);
#endif

#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
        Error |= ES_WMA_Parser_Init(  Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_WMA]);
        Error |= ES_WMAPES_Parser_Init(  Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_WMA_PES]);
#endif

#ifdef STAUD_INSTALL_AAC
        Error |= ES_AAC_Parser_Init(  Init_p, &Com_ParserLocalParams_p->ESParserHandle[ES_AAC]);
#endif

    }

    if(Error!=ST_NO_ERROR)
    {
        //If error in the Init of any PES, ES Parsers then terminate to deallocate the memory memory
        return ST_ERROR_NO_MEMORY;
    }
/* Get Parsing Functions for all the pes parsers */

    Error |= PES_MP2_ParserGetParsingFuction(&Com_ParserLocalParams_p->PESParseFunction[PES_MP2]);
#ifndef ST_51XX
    Error |= PES_PACKMP1_ParserGetParsingFuction(&Com_ParserLocalParams_p->PESParseFunction[PES_PACKMP1]);
    Error |= PES_DVDV_ParserGetParsingFuction(&Com_ParserLocalParams_p->PESParseFunction[PES_DVDV]);

    Error |= PES_DVDA_ParserGetParsingFuction(&Com_ParserLocalParams_p->PESParseFunction[PES_DVDA]);
#endif

/* Get Parsing Functions for all the es parsers */
#ifdef STAUD_INSTALL_MPEG
    Error |= ES_MPEG_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_MPEG],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_MPEG],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_MPEG]);
#endif

#ifdef STAUD_INSTALL_AC3
    Error |= ES_AC3_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_AC3],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_AC3],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_AC3]);
#endif

#ifdef STAUD_INSTALL_LPCMV
    Error |= ES_LPCMV_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_LPCMV],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_LPCMV],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_LPCMV]);
#endif

#ifdef STAUD_INSTALL_LPCMA
    Error |= ES_LPCMA_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_LPCMA],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_LPCMA],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_LPCMA]);
#endif

#ifdef STAUD_INSTALL_MLP
    Error |= ES_MLP_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_MLP],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_MLP],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_MLP]);
#endif

#ifdef STAUD_INSTALL_DTS
    Error |= ES_DTS_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_DTS],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_DTS],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_DTS]);
#endif

#ifdef STAUD_INSTALL_CDDA
    Error |= ES_CDDA_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_CDDA],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_CDDA],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_CDDA]);
#endif

#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
    Error |= ES_WMA_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_WMA],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_WMA],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_WMA]);

    Error |= ES_WMAPES_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_WMA_PES],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_WMA_PES],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_WMA_PES]);
#endif

#if defined(STAUD_INSTALL_AAC)
    Error |= ES_AAC_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_AAC],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_AAC],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_AAC]);
#endif

#ifdef STAUD_INSTALL_WAVE
    Error |= ES_Wave_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_WAVE],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_WAVE],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_WAVE]);
#endif

#ifdef STAUD_INSTALL_AIFF
    Error |= ES_AIFF_ParserGetParsingFunction(
                &Com_ParserLocalParams_p->ESParseFunction[ES_AIFF],
                &Com_ParserLocalParams_p->ESGetFreqFunction[ES_AIFF],
                &Com_ParserLocalParams_p->ESGetInfoFunction[ES_AIFF]);
#endif


    if(Error!=ST_NO_ERROR)
    {
        // PES, ES Parser never return any error for GetParsing functions
    }


    Init_p->Handle=(ComParser_Handle_t *)Com_ParserLocalParams_p;

    *Handle = (ComParser_Handle_t)Init_p->Handle;

    STTBX_Print((" Com_Parser Init Exit SuccessFull \n "));
    return ST_NO_ERROR;
}

/* Set the Stream Type and Contain to Decoder */
ST_ErrorCode_t Com_Parser_SetStreamType(ComParser_Handle_t  Handle,STAUD_StreamType_t StreamType,
    STAUD_StreamContent_t StreamContents)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Com_ParserLocalParams_p->StreamType=StreamType;

    Com_ParserLocalParams_p->StreamContent=StreamContents;

    Com_ParserLocalParams_p->CurPESParseFunction = NULL;
    switch(StreamType)
    {
#ifndef ST_51XX
    case STAUD_STREAM_TYPE_MPEG1_PACKET:
        Com_ParserLocalParams_p->CurPESParser = PES_PACKMP1;
        Com_ParserLocalParams_p->PESStartFunction = (PESStartFunction_t)PES_PACKMP1_Parser_Start;
        Com_ParserLocalParams_p->PESStopFunction = (PESStopFunction_t)PES_PACKMP1_Parser_Stop;
        PES_PACKMP1_Parser_SetStreamType(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1], StreamType, StreamContents);
        break;
#endif
    case STAUD_STREAM_TYPE_ES:
    case STAUD_STREAM_TYPE_PES:
    case STAUD_STREAM_TYPE_PES_AD:
        Com_ParserLocalParams_p->CurPESParser = PES_MP2;
        Com_ParserLocalParams_p->PESStartFunction = (PESStartFunction_t)PES_MP2_Parser_Start;
        Com_ParserLocalParams_p->PESStopFunction = (PESStopFunction_t)PES_MP2_Parser_Stop;
        PES_MP2_Parser_SetStreamType(Com_ParserLocalParams_p->PESParserHandle[PES_MP2], StreamType, StreamContents);
        break;
    case STAUD_STREAM_TYPE_PES_DVD:
#ifndef ST_51XX
        Com_ParserLocalParams_p->CurPESParser = PES_DVDV;
        Com_ParserLocalParams_p->PESStartFunction = (PESStartFunction_t)PES_DVDV_Parser_Start;
        Com_ParserLocalParams_p->PESStopFunction = (PESStopFunction_t)PES_DVDV_Parser_Stop;
        PES_DVDV_Parser_SetStreamType(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV], StreamType, StreamContents);
        break;
    case STAUD_STREAM_TYPE_PES_DVD_AUDIO:
        Com_ParserLocalParams_p->CurPESParser = PES_DVDA;
        Com_ParserLocalParams_p->PESStartFunction = (PESStartFunction_t)PES_DVDA_Parser_Start;
        Com_ParserLocalParams_p->PESStopFunction = (PESStopFunction_t)PES_DVDA_Parser_Stop;
        PES_DVDA_Parser_SetStreamType(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA], StreamType, StreamContents);
        break;
#endif
    default:
        Com_ParserLocalParams_p->CurPESParser = MAX_NUM_PES_PARSER;
        Com_ParserLocalParams_p->PESStartFunction = NULL;
        Com_ParserLocalParams_p->PESStopFunction = NULL;
        Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    }

    switch(StreamContents)
    {
    case STAUD_STREAM_CONTENT_MPEG1:
    case STAUD_STREAM_CONTENT_MPEG2:
    case STAUD_STREAM_CONTENT_MP3:
    case STAUD_STREAM_CONTENT_MPEG_AAC:
#ifdef STAUD_INSTALL_MPEG
        Com_ParserLocalParams_p->CurESParser        = ES_MPEG;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_MPEG_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_MPEG_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_MPEG_Handle_EOF;

        Error |= ES_MPEG_SetStreamContent(Com_ParserLocalParams_p->ESParserHandle[ES_MPEG],StreamContents);
#endif
        break;
    case STAUD_STREAM_CONTENT_AC3:
    case STAUD_STREAM_CONTENT_DDPLUS:
#ifdef STAUD_INSTALL_AC3
        Com_ParserLocalParams_p->CurESParser        = ES_AC3;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_AC3_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_AC3_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_AC3_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_LPCM:
#ifdef STAUD_INSTALL_LPCMV
        Com_ParserLocalParams_p->CurESParser        = ES_LPCMV;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_LPCMV_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_LPCMV_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_LPCMV_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_LPCM_DVDA:
#ifdef STAUD_INSTALL_LPCMA
        Com_ParserLocalParams_p->CurESParser        = ES_LPCMA;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_LPCMA_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_LPCMA_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_LPCMA_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_MLP:
#ifdef STAUD_INSTALL_MLP
        Com_ParserLocalParams_p->CurESParser        = ES_MLP;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_MLP_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_MLP_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_MLP_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_DTS:
#ifdef STAUD_INSTALL_DTS
        Com_ParserLocalParams_p->CurESParser        = ES_DTS;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_DTS_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_DTS_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_DTS_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_CDDA:
    case STAUD_STREAM_CONTENT_PCM:
#ifdef STAUD_INSTALL_CDDA
        Com_ParserLocalParams_p->CurESParser        = ES_CDDA;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_CDDA_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_CDDA_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_CDDA_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_WMA:
    case STAUD_STREAM_CONTENT_WMAPROLSL:
#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
        if(Com_ParserLocalParams_p->StreamType == STAUD_STREAM_TYPE_ES)
        {
            Com_ParserLocalParams_p->CurESParser        = ES_WMA;
            Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_WMA_Parser_Start;
            Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_WMA_Parser_Stop;
            Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_WMA_Handle_EOF;
        }
        else
        {
            Com_ParserLocalParams_p->CurESParser        = ES_WMA_PES;
            Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_WMAPES_Parser_Start;
            Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_WMAPES_Parser_Stop;
            Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_WMAPES_Handle_EOF;
        }
#endif
        break;

    case STAUD_STREAM_CONTENT_WAVE:
#ifdef STAUD_INSTALL_WAVE
        Com_ParserLocalParams_p->CurESParser        = ES_WAVE;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_Wave_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_Wave_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_Wave_Handle_EOF;
#endif
        break;

    case STAUD_STREAM_CONTENT_AIFF:
#ifdef STAUD_INSTALL_AIFF
        Com_ParserLocalParams_p->CurESParser        = ES_AIFF;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_AIFF_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_AIFF_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_AIFF_Handle_EOF;
#endif
        break;
    case STAUD_STREAM_CONTENT_ADIF:
    case STAUD_STREAM_CONTENT_MP4_FILE:
    case STAUD_STREAM_CONTENT_RAW_AAC:
#if defined(STAUD_INSTALL_AAC)
        Com_ParserLocalParams_p->CurESParser        = ES_AAC;
        Com_ParserLocalParams_p->ESStartFunction    = (ESStartFunction_t)ES_AAC_Parser_Start;
        Com_ParserLocalParams_p->ESStopFunction     = (ESStopFunction_t)ES_AAC_Parser_Stop;
        Com_ParserLocalParams_p->ESEOFFunction  = (ESEOFFunction_t)ES_AAC_Handle_EOF;
#endif
        break;
    default:
        Com_ParserLocalParams_p->CurESParser        = MAX_NUM_PARSER;
        Com_ParserLocalParams_p->ESStartFunction    = NULL;
        Com_ParserLocalParams_p->ESStopFunction     = NULL;
        Com_ParserLocalParams_p->ESEOFFunction  = NULL;
        Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    }

    return Error;
}

/* Set the BroadcastProfile */
ST_ErrorCode_t Com_Parser_SetBroadcastProfile(ComParser_Handle_t  Handle,STAUD_BroadcastProfile_t BroadcastProfile)
{
    Com_ParserData_t *Com_ParserLocalParams_p =(Com_ParserData_t *)Handle;

    PES_MP2_Parser_SetBroadcastProfile(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],         BroadcastProfile);
#ifndef ST_51XX
    PES_PACKMP1_Parser_SetBroadcastProfile(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1], BroadcastProfile);
    PES_DVDV_Parser_SetBroadcastProfile(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],       BroadcastProfile);
    PES_DVDA_Parser_SetBroadcastProfile(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],       BroadcastProfile);
#endif
    return ST_NO_ERROR;
}

/* Set the Stream ID and Contain to Decoder */
ST_ErrorCode_t Com_Parser_SetStreamID(ComParser_Handle_t  Handle,U32 StreamID)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error |= PES_MP2_Parser_SetStreamID(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],         StreamID);
#ifndef ST_51XX
    Error |= PES_PACKMP1_Parser_SetStreamID(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1], StreamID);
    Error |= PES_DVDV_Parser_SetStreamID(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],       StreamID);
    Error |= PES_DVDA_Parser_SetStreamID(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],       StreamID);
#endif
    return Error;
}


/* Set the PCM Params for CDDA decoder Decoder */
ST_ErrorCode_t Com_Parser_SetPCMParams(ComParser_Handle_t  Handle,U32 Frequency, U32 SampleSize, U32 Nch)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STAUD_INSTALL_CDDA
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    U8 es_parser_function = 0;

    es_parser_function = ES_CDDA; // only required for the CDDA decoder
    Error = ES_CDDA_Set_Pcm((Com_ParserLocalParams_p->ESParserHandle[es_parser_function]),Frequency,SampleSize,Nch);

#else

    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#endif

    return Error;
}


/* Get Sampling Frequency from ES Parser */

ST_ErrorCode_t Com_Parser_GetFreq(ComParser_Handle_t  Handle,U32 *SamplingFrequency_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
    case STAUD_STREAM_CONTENT_NULL:
    case STAUD_STREAM_CONTENT_HE_AAC:
    case STAUD_STREAM_CONTENT_CDDA_DTS:
    case STAUD_STREAM_CONTENT_DV:
    case STAUD_STREAM_CONTENT_BEEP_TONE:
    case STAUD_STREAM_CONTENT_PINK_NOISE:
        Error |=    STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    default:
        if(es_parser < MAX_NUM_PARSER)
        {
            if(Com_ParserLocalParams_p->ESGetFreqFunction[es_parser])
            Error |= Com_ParserLocalParams_p->ESGetFreqFunction[es_parser]
                        ((Com_ParserLocalParams_p->ESParserHandle[es_parser]),SamplingFrequency_p);
            else
            Error |= ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
        else
        {
            Error |=    STAUD_ERROR_INVALID_STREAM_TYPE;
        }
        break;
    }

    if(Error != ST_NO_ERROR)
    {
        // ES Parser never return any error
    }

    return ST_NO_ERROR;
}


/* Get GetStreamInfo from ES Parser */

ST_ErrorCode_t Com_Parser_GetInfo(ComParser_Handle_t  Handle,STAUD_StreamInfo_t * StreamInfo_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
    case STAUD_STREAM_CONTENT_NULL:
    case STAUD_STREAM_CONTENT_HE_AAC:
    case STAUD_STREAM_CONTENT_CDDA_DTS:
    case STAUD_STREAM_CONTENT_DV:
    case STAUD_STREAM_CONTENT_BEEP_TONE:
    case STAUD_STREAM_CONTENT_PINK_NOISE:
        Error |=    STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    default:
        if(es_parser < MAX_NUM_PARSER)
        {
            if(Com_ParserLocalParams_p->ESGetInfoFunction[es_parser])
            Error |= Com_ParserLocalParams_p->ESGetInfoFunction[es_parser]
                ((Com_ParserLocalParams_p->ESParserHandle[es_parser]),StreamInfo_p);
            else
                Error |=ST_ERROR_FEATURE_NOT_SUPPORTED;
        }
        else
        {
            Error |=    STAUD_ERROR_INVALID_STREAM_TYPE;
        }
        break;
    }

    return ST_NO_ERROR;
}

/************************************************************************************
Name         : Com_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : Com_ParserData_t  Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t Com_Parser_Stop(PESES_ParserControl_t *ControlBlock_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)ControlBlock_p->ComParser_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 pes_parser      = Com_ParserLocalParams_p->CurPESParser;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    if((Com_ParserLocalParams_p->PESStopFunction) && (pes_parser <  MAX_NUM_PES_PARSER))
    {
        Error |= Com_ParserLocalParams_p->PESStopFunction(Com_ParserLocalParams_p->PESParserHandle[pes_parser]);
    }
    else
    {
        Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
    }

    switch(Com_ParserLocalParams_p->StreamContent)
    {
    case STAUD_STREAM_CONTENT_BEEP_TONE:
    case STAUD_STREAM_CONTENT_PINK_NOISE:
        /* Since we don't need a parser for  pink noise and beep tone, return no error.
        This will cause Peses task to deshedule and other task will get time to execute */
        break;
    default:
        if((Com_ParserLocalParams_p->ESStopFunction) && (es_parser < MAX_NUM_PARSER))
        {
            Error |= Com_ParserLocalParams_p->ESStopFunction(Com_ParserLocalParams_p->ESParserHandle[es_parser]);
        }
        else
        {
            Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
        }
        break;
    }

    return Error;
}


/************************************************************************************
Name         : Com_Parser_Start()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t Com_Parser_Start(PESES_ParserControl_t *ControlBlock_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p =(Com_ParserData_t *)ControlBlock_p->ComParser_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 pes_parser      = Com_ParserLocalParams_p->CurPESParser;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    Com_ParserLocalParams_p->CurPESParseFunction = NULL;

    switch(Com_ParserLocalParams_p->StreamType)
    {
    case STAUD_STREAM_TYPE_MPEG1_PACKET:
        switch(Com_ParserLocalParams_p->StreamContent)
        {
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:

#ifdef STAUD_INSTALL_MPEG
            Com_ParserLocalParams_p->CurPESParseFunction =
                                     Com_ParserLocalParams_p->PESParseFunction[PES_PACKMP1];
#endif
        default:
            break;
        }

        break;
    case STAUD_STREAM_TYPE_ES:
    case STAUD_STREAM_TYPE_PES:
    case STAUD_STREAM_TYPE_PES_AD:
        switch(Com_ParserLocalParams_p->StreamContent)
        {
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MPEG_AAC:
        /* for ES+ (ES -> PES) injection support for MP3 for all OS */
        case STAUD_STREAM_CONTENT_MP3:
        case STAUD_STREAM_CONTENT_AC3:
        case STAUD_STREAM_CONTENT_ADIF:
        case STAUD_STREAM_CONTENT_MP4_FILE:
        case STAUD_STREAM_CONTENT_RAW_AAC:
        case STAUD_STREAM_CONTENT_WMA:
        case STAUD_STREAM_CONTENT_WMAPROLSL:
        case STAUD_STREAM_CONTENT_PCM:
        case STAUD_STREAM_CONTENT_DDPLUS:
        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_LPCM_DVDA:
        case STAUD_STREAM_CONTENT_MLP:
        case STAUD_STREAM_CONTENT_DTS:
        case STAUD_STREAM_CONTENT_AIFF:
        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_WAVE:

            Com_ParserLocalParams_p->CurPESParseFunction =
                                     Com_ParserLocalParams_p->PESParseFunction[PES_MP2];

            break;
        default:
            break;
        }

        break;
#ifndef ST_51XX
    case STAUD_STREAM_TYPE_PES_DVD:
        switch(Com_ParserLocalParams_p->StreamContent)
        {
        case STAUD_STREAM_CONTENT_AC3:
        case STAUD_STREAM_CONTENT_DDPLUS:
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MPEG_AAC:
        case STAUD_STREAM_CONTENT_MP3:
        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_LPCM_DVDA:
        case STAUD_STREAM_CONTENT_MLP:
        case STAUD_STREAM_CONTENT_DTS:
        case STAUD_STREAM_CONTENT_WAVE:
        case STAUD_STREAM_CONTENT_AIFF:
        case STAUD_STREAM_CONTENT_CDDA:
        case STAUD_STREAM_CONTENT_PCM:
        case STAUD_STREAM_CONTENT_WMA:
        case STAUD_STREAM_CONTENT_WMAPROLSL:
        case STAUD_STREAM_CONTENT_ADIF:
        case STAUD_STREAM_CONTENT_MP4_FILE:
        case STAUD_STREAM_CONTENT_RAW_AAC:

            Com_ParserLocalParams_p->CurPESParseFunction = Com_ParserLocalParams_p->PESParseFunction[PES_DVDV];

            break;
        default:
            break;

        }

        if(((es_parser == ES_LPCMA)||(es_parser == ES_MLP))&&
                                     (Com_ParserLocalParams_p->StreamType ==  STAUD_STREAM_TYPE_PES_DVD))
        {
            Com_ParserLocalParams_p->CurPESParseFunction = NULL;
        }
        break;
    case STAUD_STREAM_TYPE_PES_DVD_AUDIO:
        switch(Com_ParserLocalParams_p->StreamContent)
        {
        case STAUD_STREAM_CONTENT_LPCM:
        case STAUD_STREAM_CONTENT_MLP:
            Com_ParserLocalParams_p->CurPESParseFunction = Com_ParserLocalParams_p->PESParseFunction[PES_DVDA];
            break;
        default:
            break;
        }
        break;
#endif
    default:
        break;
    }

    if((Com_ParserLocalParams_p->PESStartFunction) && (pes_parser <  MAX_NUM_PES_PARSER))
    {
        Error |= Com_ParserLocalParams_p->PESStartFunction(Com_ParserLocalParams_p->PESParserHandle[pes_parser]);
    }
    else
    {
        Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
    }

    switch(Com_ParserLocalParams_p->StreamContent)
    {
    case STAUD_STREAM_CONTENT_BEEP_TONE:
    case STAUD_STREAM_CONTENT_PINK_NOISE:
        /* Since we don't need a parser for  pink noise and beep tone, return no error.
        This will cause Peses task to deshedule and other task will get time to execute */
        break;
    default:
        if((Com_ParserLocalParams_p->ESStartFunction)&&(es_parser <  MAX_NUM_PARSER))
        {
            Error |= Com_ParserLocalParams_p->ESStartFunction(Com_ParserLocalParams_p->ESParserHandle[es_parser]);
        }
        else
        {
            Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
        }
        break;
    }

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
ST_ErrorCode_t Com_Parse_Frame(PESES_ParserControl_t *ControlBlock_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t*)ControlBlock_p->ComParser_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    void *MemoryStart   = (void *)ControlBlock_p->PESBlock.StartPointer;
    U32 Size                = ControlBlock_p->PESBlock.ValidSize;
    U32 *No_Bytes_Used  = &ControlBlock_p->BytesUsed;
    U16 pes_parser      = Com_ParserLocalParams_p->CurPESParser;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
    case STAUD_STREAM_CONTENT_BEEP_TONE:
    case STAUD_STREAM_CONTENT_PINK_NOISE:
        /* Since we don't need a parser for  pink noise and beep tone, return no error.
        This will cause Peses task to deshedule and other task will get time to execute */
        break;
    default:
        /*check for valid combinations do it here? or at set streamid*/
        if(Com_ParserLocalParams_p->CurPESParseFunction && (pes_parser < MAX_NUM_PES_PARSER) && (es_parser < MAX_NUM_PARSER))
        {
                if(Com_ParserLocalParams_p->ESParseFunction[es_parser])
                {
                    Error |= Com_ParserLocalParams_p->CurPESParseFunction((Com_ParserLocalParams_p->PESParserHandle[pes_parser]),
                    MemoryStart,Size,No_Bytes_Used,Com_ParserLocalParams_p->ESParseFunction[es_parser],
                    Com_ParserLocalParams_p->ESParserHandle[es_parser]);
                }
                else
                {
                    Error |= ST_ERROR_FEATURE_NOT_SUPPORTED;
                }

        }
        else
        {
            Error |= STAUD_ERROR_INVALID_STREAM_TYPE;
        }
        break;
    }

    return Error;
}

ST_ErrorCode_t Com_Parser_Term(PESES_ParserControl_t *ControlBlock_p)
{
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)ControlBlock_p->ComParser_p;
    ST_Partition_t  *CPUPartition_p = ControlBlock_p->CPUPartition_p;
    ST_ErrorCode_t Error                = ST_NO_ERROR;

    /* terminate all the pes and es parsers here */
    if(Com_ParserLocalParams_p)
    {
        Error |= PES_MP2_Parser_Term(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],CPUPartition_p);
#ifndef ST_51XX
        Error |= PES_PACKMP1_Parser_Term(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1],CPUPartition_p);
        Error |= PES_DVDV_Parser_Term(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],CPUPartition_p);
        Error |= PES_DVDA_Parser_Term(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_MPEG
        Error |= ES_MPEG_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_MPEG],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_AC3
        Error |= ES_AC3_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AC3],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_LPCMV
        Error |= ES_LPCMV_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_LPCMV],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_LPCMA
        Error |= ES_LPCMA_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_LPCMA],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_MLP
        Error |= ES_MLP_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_MLP],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_DTS
        Error |= ES_DTS_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_DTS],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_CDDA
        Error |= ES_CDDA_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_CDDA],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_WAVE
        Error |= ES_Wave_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_WAVE],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_AIFF
        Error |= ES_AIFF_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AIFF],CPUPartition_p);
#endif


#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
        Error |= ES_WMA_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_WMA],CPUPartition_p);
        Error |= ES_WMAPES_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_WMA_PES],CPUPartition_p);
#endif

#ifdef STAUD_INSTALL_AAC
        Error |= ES_AAC_Parser_Term((ParserHandle_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AAC],CPUPartition_p);
#endif

        if(Error == ST_NO_ERROR)
        {
            /*All PES and ES Parsers Deallocation were sucessfull deallocate the comparser*/
            memory_deallocate(CPUPartition_p, Com_ParserLocalParams_p);
        }
    }

    return Error;
}

/************************************************************************************
Name         : Com_Parser_HandleEOF()

Description  : This function handles the event of and EOF

Parameters   : Com_ParserData_t  Handle

Return Value :
    ST_NO_ERROR
************************************************************************************/
ST_ErrorCode_t Com_Parser_HandleEOF(ComParser_Handle_t  Handle)
{
    Com_ParserData_t *Com_ParserLocalParams_p =(Com_ParserData_t*)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 es_parser           = Com_ParserLocalParams_p->CurESParser;

    if(Com_ParserLocalParams_p->ESEOFFunction)
    {
        Error |= Com_ParserLocalParams_p->ESEOFFunction(Com_ParserLocalParams_p->ESParserHandle[es_parser]);
    }
    else
    {
        Error |=    STAUD_ERROR_INVALID_STREAM_TYPE;
    }


    return Error;
}

/************************************************************************************
Name         : Com_Parser_SetWMAStreamID()

Description  : This function set stream ID of WMA stream to be played

Parameters   : Com_ParserData_t  Handle
               WMAStreamNumber stream ID number

Return Value :
    ST_NO_ERROR
************************************************************************************/
ST_ErrorCode_t Com_Parser_SetWMAStreamID(ComParser_Handle_t  Handle, U8 WMAStreamNumber)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t*)Handle;
    Error = ES_WMA_SetStreamID(Com_ParserLocalParams_p->ESParserHandle[ES_WMA],WMAStreamNumber);
#endif
    return Error;
}


/************************************************************************************
Name         : Com_Parser_SetSpeed()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t Com_Parser_SetSpeed(ComParser_Handle_t  Handle,S32 Speed)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    /* Start all the parsers here */
    {
        Error |= PES_MP2_SetSpeed(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],Speed);
#ifndef ST_51XX
        Error |= PES_PACKMP1_SetSpeed(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1],Speed);
        Error |= PES_DVDV_SetSpeed(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],Speed);
        Error |= PES_DVDA_SetSpeed(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],Speed);
#endif
    }

    return Error;
}
/************************************************************************************
Name         : Com_Parser_SetClkSynchro()

Description  : Sets the clock recovery source.

Parameters   : Com_ParserData_t  Handle
               STCLKRV_Handle_t Clksource

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
ST_ErrorCode_t Com_Parser_SetClkSynchro(ComParser_Handle_t  Handle,STCLKRV_Handle_t Clksource)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    {
        Error |= PES_MP2_SetClkSynchro(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],Clksource);
#ifndef ST_51XX
        Error |= PES_PACKMP1_SetClkSynchro(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1],Clksource);
        Error |= PES_DVDV_SetClkSynchro(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],Clksource);
        Error |= PES_DVDA_SetClkSynchro(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],Clksource);
#endif
    }

    return Error;
}
#endif

/************************************************************************************
Name         : Com_Parser_AVSyncCmd()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle
                  BOOL AVSyncEnable

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/
ST_ErrorCode_t Com_Parser_AVSyncCmd(ComParser_Handle_t  Handle,BOOL AVSyncEnable)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    {
        Error |= PES_MP2_AVSyncCmd(Com_ParserLocalParams_p->PESParserHandle[PES_MP2],AVSyncEnable);
#ifndef ST_51XX
        Error |= PES_PACKMP1_AVSyncCmd(Com_ParserLocalParams_p->PESParserHandle[PES_PACKMP1],AVSyncEnable);
        Error |= PES_DVDV_AVSyncCmd(Com_ParserLocalParams_p->PESParserHandle[PES_DVDV],AVSyncEnable);
        Error |= PES_DVDA_AVSyncCmd(Com_ParserLocalParams_p->PESParserHandle[PES_DVDA],AVSyncEnable);
#endif
    }

    return Error;
}

/************************************************************************************
Name         : Com_Parser_GetSynchroUnit()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle
                  SynchroUnit_p Synchro units

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/
ST_ErrorCode_t Com_Parser_GetSynchroUnit(ComParser_Handle_t Handle,STAUD_SynchroUnit_t *SynchroUnit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
#ifdef STAUD_INSTALL_AC3
    case STAUD_STREAM_CONTENT_AC3:
    case STAUD_STREAM_CONTENT_DDPLUS:
        Error = ES_AC3_GetSynchroUnit(Com_ParserLocalParams_p->ESParserHandle[ES_AC3],SynchroUnit_p);
        break;
#endif
#ifdef STAUD_INSTALL_MPEG
    case STAUD_STREAM_CONTENT_MPEG1:
    case STAUD_STREAM_CONTENT_MPEG2:
    case STAUD_STREAM_CONTENT_MPEG_AAC:
    case STAUD_STREAM_CONTENT_MP3:
        Error = ES_MPEG_GetSynchroUnit(Com_ParserLocalParams_p->ESParserHandle[ES_MPEG],SynchroUnit_p);
        break;
#endif
#ifdef STAUD_INSTALL_AAC
    case STAUD_STREAM_CONTENT_ADIF:
    case STAUD_STREAM_CONTENT_MP4_FILE:
    case STAUD_STREAM_CONTENT_RAW_AAC:
        Error = ES_AAC_GetSynchroUnit(Com_ParserLocalParams_p->ESParserHandle[ES_AAC], SynchroUnit_p);
        break;
#endif

    default:

        Error = STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    }

    return Error;
}

/************************************************************************************
Name         : Com_Parser_SkipSynchro()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle
                  Delay skip units

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/
ST_ErrorCode_t Com_Parser_SkipSynchro(ComParser_Handle_t Handle, U32 Delay)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
#ifdef STAUD_INSTALL_AC3
    case STAUD_STREAM_CONTENT_AC3:
    case STAUD_STREAM_CONTENT_DDPLUS:
        {
            ES_AC3_ParserLocalParams_t *AC3_ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AC3];

            if((AC3_ParserLocalParams_p->Skip == 0) && (AC3_ParserLocalParams_p->Pause == 0))
            {
                /*apply only if pause is already 0*/
                AC3_ParserLocalParams_p->Skip = Delay;
            }
            else
            {
                Error = STAUD_ERROR_DECODER_PAUSING;
            }
        }
        break;
#endif
#ifdef STAUD_INSTALL_MPEG
    case STAUD_STREAM_CONTENT_MPEG1:
    case STAUD_STREAM_CONTENT_MPEG2:
    case STAUD_STREAM_CONTENT_MPEG_AAC:
    case STAUD_STREAM_CONTENT_MP3:
        {
            ES_MPEG_ParserLocalParams_t *MPEG_ParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)Com_ParserLocalParams_p->ESParserHandle[ES_MPEG];

            if((MPEG_ParserLocalParams_p->Skip == 0) && (MPEG_ParserLocalParams_p->Pause == 0))
            {
                /*apply only if pause is already 0*/
                MPEG_ParserLocalParams_p->Skip = Delay;
            }
            else
            {
                Error = STAUD_ERROR_DECODER_PAUSING;
            }
        }
        break;
#endif

    default:

        Error = STAUD_ERROR_INVALID_STREAM_TYPE;
        break;
    }

    return Error;
}
/************************************************************************************
Name         : Com_Parser_PauseSynchro()

Description  : Starts the Parser.

Parameters   : Com_ParserData_t  Handle
                   Delay pause units

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/
ST_ErrorCode_t Com_Parser_PauseSynchro(ComParser_Handle_t Handle, U32 Delay)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;

    switch(Com_ParserLocalParams_p->StreamContent)
    {
#ifdef STAUD_INSTALL_AC3
    case STAUD_STREAM_CONTENT_AC3:
    case STAUD_STREAM_CONTENT_DDPLUS:
        {
            ES_AC3_ParserLocalParams_t *AC3_ParserLocalParams_p = (ES_AC3_ParserLocalParams_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AC3];

            if((AC3_ParserLocalParams_p->Pause == 0) && (AC3_ParserLocalParams_p->Skip == 0))
            {
                /*apply only if pause is already 0*/
                AC3_ParserLocalParams_p->Pause = Delay;
            }
            else
            {
                Error = STAUD_ERROR_DECODER_PAUSING;
            }
        }
        break;
#endif
#ifdef STAUD_INSTALL_MPEG
    case STAUD_STREAM_CONTENT_MPEG1:
    case STAUD_STREAM_CONTENT_MPEG2:
    case STAUD_STREAM_CONTENT_MPEG_AAC:
    case STAUD_STREAM_CONTENT_MP3:
        {
            ES_MPEG_ParserLocalParams_t *MPEG_ParserLocalParams_p = (ES_MPEG_ParserLocalParams_t *)Com_ParserLocalParams_p->ESParserHandle[ES_MPEG];

            if((MPEG_ParserLocalParams_p->Pause == 0) && (MPEG_ParserLocalParams_p->Skip == 0))
            {
                /*apply only if pause is already 0*/
                MPEG_ParserLocalParams_p->Pause = Delay;
            }
            else
            {
                Error = STAUD_ERROR_DECODER_PAUSING;
            }
        }
        break;
#endif
#ifdef STAUD_INSTALL_AAC
    case STAUD_STREAM_CONTENT_ADIF:
    case STAUD_STREAM_CONTENT_MP4_FILE:
    case STAUD_STREAM_CONTENT_RAW_AAC:
        {
            ES_AAC_ParserLocalParams_t *AAC_ParserLocalParams_p = (ES_AAC_ParserLocalParams_t *)Com_ParserLocalParams_p->ESParserHandle[ES_AAC];

            if((AAC_ParserLocalParams_p->Pause == 0) && (AAC_ParserLocalParams_p->Skip == 0))
            {
                /*apply only if pause is already 0*/
                AAC_ParserLocalParams_p->Pause = Delay;
            }
            else
            {
                Error = STAUD_ERROR_DECODER_PAUSING;
            }
        }
        break;
#endif

    default:

        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        break;
    }

    return Error;

}

U32 Com_Parser_GetRemainTime(ComParser_Handle_t Handle)
{
    U32 Duration = 0;
    Com_ParserData_t *Com_ParserLocalParams_p = (Com_ParserData_t *)Handle;
    U16 es_parser   = Com_ParserLocalParams_p->CurESParser;

    if(es_parser < MAX_NUM_PARSER)
    {
        switch(Com_ParserLocalParams_p->StreamContent)
        {
#ifdef STAUD_INSTALL_AC3
        case STAUD_STREAM_CONTENT_AC3:
        case STAUD_STREAM_CONTENT_DDPLUS:
            Duration = ES_AC3_GetRemainTime(Com_ParserLocalParams_p->ESParserHandle[es_parser]);
            break;
#endif
#ifdef STAUD_INSTALL_MPEG
        case STAUD_STREAM_CONTENT_MPEG1:
        case STAUD_STREAM_CONTENT_MPEG2:
        case STAUD_STREAM_CONTENT_MPEG_AAC:
        case STAUD_STREAM_CONTENT_MP3:
            Duration = ES_MPEG_GetRemainTime(Com_ParserLocalParams_p->ESParserHandle[es_parser]);
            break;
#endif
        default:
            break;
        }
    }

    return Duration;

}

