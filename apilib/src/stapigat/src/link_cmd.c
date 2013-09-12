/************************************************************************

File name   : link_cmd.c

Description : Commands for PTI

COPYRIGHT (C) STMicroelectronics 2002

Date          Modification                                    Initials
----          ------------                                    --------
12 Mar 1999   Creation                                        FQ
08 Sep 1999   Change STTBX_Output() by STTBX_Print()          FQ
30 Sep 1999   Move init. PTI buffer in PTI_InitComponent()    FQ
04 Oct 1999   Use stlite.h instead of os20.h                  FQ
12 Jan 2001   Add STPTI commands                              FQ
29 Aug 2001   Move the PTI calls in the pti_ocmd.c file       AN
03 Jun 2002   Remove the PTI calls and keep only LINK calls   AN
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#if defined (USE_PTI)
#include "stddefs.h"
#include "stdevice.h"

#include "sttbx.h"
#include "testtool.h"
#include "clclkrv.h"
#include "api_cmd.h"

#include "stclkrv.h"
#include "pti.h"

/*###########################################################################*/
/*############################## DEFINITION #################################*/
/*###########################################################################*/


/* --- Constants ---------------------------------------------------------- */
/* #define BOTH */ /* sets up with an illegal usage setting up in both fines */
/* #define  BACK_BUFFERED */ /* comment this out for direct to cdfifo operation */

#define STPTI_MAX_DEVICES     1

#define AUDIO_BUFFER_SIZE  8192
#define VIDEO_BUFFER_SIZE  8192


/* --- Global variables --------------------------------------------------- */

static S16 CurrentVideoFifoNb = 1;

static BOOL SendingVideo[STPTI_MAX_DEVICES];
static BOOL SendingAudio = FALSE;
static BOOL SendingPCR = FALSE;

static pid_t PID;

static char PTI_Msg[200]; /* for trace */

slot_t VSlot;
slot_t ASlot;

#pragma ST_device (device_byte_t)
     typedef volatile unsigned char device_byte_t;

/* Private Function prototypes ---------------------------------------------- */

/*##########################################################################*/
/*######################### INTERNAL FUNCTIONS #############################*/
/*##########################################################################*/
/*****************************************************************************
Name        : PTI_Init
Description : Initialise the LINK driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Init(void)
{
    BOOL RetOK;

    BOOL RetErr;
    unsigned int Size;
    unsigned char *Buffer;
    unsigned int PTI_driver_code_version;
    unsigned int PTI_tc_code_version;
    unsigned int PTI_variant;

    pti_version( &PTI_driver_code_version, &PTI_tc_code_version, &PTI_variant );
    STTBX_Print(( "PTI      : version=0x%X tc=0x%X variant=0x%X ",
             PTI_driver_code_version, PTI_tc_code_version, PTI_variant));

    RetOK = TRUE;

    /* --- Driver initialization --- */

    RetErr = pti_init(associate_descramblers_with_slots);
    if (RetErr)
    {
        STTBX_Print(("init. error 0x%x !\n", RetErr));
        RetOK = FALSE;
    }
    if ( RetOK )
    {
        STTBX_Print(("Initialized\n"));
    }

    /* --- Connection opening --- */
    if (RetOK)
    {
        RetErr = pti_allocate_dynamic_slot(&VSlot);
        if ( RetErr )
        {
            STTBX_Print(("pti_allocate_dynamic_slot() failed for video !\n"));
            RetOK = FALSE;
        }
    }

    /* --- Video buffers opening --- */
    if (RetOK)
    {
        RetErr = pti_malloc_buffer(VIDEO_BUFFER_SIZE, &Size, &Buffer);
        if ( RetErr )
        {
            STTBX_Print(("pti_malloc_buffer() failed for video !\n"));
            RetOK = FALSE;
        }

        RetErr = pti_set_buffer(VSlot, stream_type_video, Buffer, 0, NULL, NULL, NULL , no_flags);
        if ( RetErr )
        {
            STTBX_Print(("pti_set_buffer() failed  for video !\n"));
            RetOK = FALSE;
        }

        RetErr = pti_malloc_buffer(AUDIO_BUFFER_SIZE, &Size, &Buffer);
        if ( RetErr )
        {
            STTBX_Print(("pti_malloc_buffer() failed for audio !"));
            RetOK = FALSE;
        }

        RetErr = pti_allocate_dynamic_slot(&ASlot);
        if ( RetErr )
        {
            STTBX_Print(("pti_allocate_dynamic_slot() failed for audio !\n"));
            RetOK = FALSE;
        }

        RetErr = pti_set_buffer(ASlot, stream_type_audio, Buffer, 0, NULL, NULL, NULL , no_flags);
        if ( RetErr )
        {
            STTBX_Print(("pti_set_buffer() failed for audio !"));
            RetOK = FALSE;
        }

    }

    if (!RetOK)
    {
        STTBX_Print(("PTI_Init() failed !\n"));
    }

    return(RetOK);
} /* end of PTI_Init() */


/*****************************************************************************
Name        : PTI_Term
Description : Terminate the LINK driver
Parameters  : None
Assumptions :
Limitations : Dummy function, link driver has no Termination function in its API
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Term(void)
{
    BOOL RetOK;

    RetOK = TRUE;
    return(RetOK);
} /* end of PTI_Term() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStop
 *            Stop sending current audio program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static boolean PTI_AudioStop(void)
{
    BOOL RetErr;

    /* Stop previous audio DMA's */
    RetErr = pti_audio_dma_state(discard_data);
    RetErr |= pti_audio_dma_reset();
    SendingAudio = FALSE;

    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStart
 *            Select/Start sending an audio program
 * Input    : audio PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTI_AudioStart(pid_t PID)
{
    BOOL Err, RetErr;

    RetErr = FALSE;
    if (SendingAudio)
    {
        RetErr = PTI_AudioStop();
    }
    /* allocate PES channels */
    Err = pti_flush_stream(ASlot);
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_flush_stream() failed \n"));
        RetErr = TRUE;
    }
    /* change video program */
    Err = pti_set_pid(ASlot, PID);
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_set_pid() failed \n"));
        RetErr = TRUE;
    }

    Err = pti_flush_stream(ASlot);
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_flush_stream() failed \n"));
        RetErr = TRUE;
    }
    Err = pti_resume();
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_resume() failed \n"));
        RetErr = TRUE;
    }
    Err = pti_audio_dma_state(transfer_data);
    if ( Err )
    {
        STTBX_Print(("PTI_AudioStart() : pti_audio_dma_state() failed \n"));
        RetErr = TRUE;
    }

    SendingAudio = TRUE;

    return(RetErr);
}

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStop
 *            Stop sending current video program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStop(void)
{
    BOOL RetErr;

    RetErr = pti_video_dma_state(discard_data);
    RetErr |= pti_video_dma_reset();
    if ( RetErr )
    {
        STTBX_Print(("PTI_VideoStop() : pti reset failed !\n"));
    }
    else
    {
        STTBX_Print(("PTI_VideoStop() : pti reset done \n"));
    }
    SendingVideo[CurrentVideoFifoNb-1] = FALSE;

    return(RetErr);
} /* end of PTI_VideoStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStart
 *            Select/Start sending a video program
 * Input    : video PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStart(pid_t PID)
{
    BOOL RetErr;
    BOOL Err;

    RetErr = FALSE;

    /* --- Stop sending data --- */
    if (SendingVideo[CurrentVideoFifoNb-1])
    {
        RetErr = PTI_VideoStop();
    }

    /* Stop data arriving at the IIF (avoid FIFO overflow) */
    Err = pti_pause();
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_pause() failed !\n", PID));
        RetErr = TRUE;
    }

    Err = pti_flush_stream(VSlot);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_flush_stream() failed ! error=%d\n", PID, Err));
        RetErr = TRUE;
    }

    /* --- Change video program --- */
    Err = pti_set_pid(VSlot, PID);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_set_pid() failed ! error=%d\n", PID, Err));
        RetErr = TRUE;
    }

    /* --- Restart incoming data --- */
    Err = pti_flush_stream(VSlot);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_flush_stream() failed \n", PID));
        RetErr = TRUE;
    }

    Err = pti_resume();
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_resume() failed \n", PID));
        RetErr = TRUE;
    }

    Err = pti_video_dma_state(transfer_data);
    if ( Err )
    {
        STTBX_Print(("PTI_VideoStart(pid=%d) : pti_video_dma_state() failed \n", PID));
        RetErr = TRUE;
    }

    SendingVideo[CurrentVideoFifoNb-1] = TRUE;

    return(RetErr);

} /*  end of PTI_VideoStart() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStop
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_PCRStop(void)
{
    BOOL RetErr;
    RetErr = pti_clear_pcr_pid();
    SendingPCR = FALSE;

    return(RetErr);
} /* end of PTI_PCRStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStart
 *            Select the PCR
 * Input    : PCR PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_PCRStart(pid_t PID)
{
    BOOL RetErr;

    RetErr = FALSE;
    if (SendingPCR)
    {
        RetErr = PTI_PCRStop();
    }

    if (RetErr)
    {
        STTBX_Print(("PTI_PCRStop() failed in PTI_PCRStart() !\n"));
    }
    else
    {
        RetErr = pti_set_pcr_pid(PID);
        if (RetErr)
        {
            STTBX_Print(("pti_set_pcr_pid(pid=%d) failed in PTI_PCRStart() !\n", PID));
        }
        if (!RetErr)
        {

            SendingPCR = TRUE;
        }
    }

    return(RetErr);

} /* end of PTI_PCRStart() */


/*#######################################################################*/
/*########################### PTI COMMANDS ##############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : PTI_VideoPid
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_VideoPID( parse_t *pars_p, char *result_sym_p )
{
    /* pid_t PID; */
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
        RetErr = PTI_VideoStart(PID);
        sprintf(PTI_Msg, "PTI_VideoPID() : pid=0x%X : err=%d\n", PID, RetErr);
        STTBX_Print((PTI_Msg));
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_VideoPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioPID
 *            Start sending the specified audio program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_AudioPID(parse_t *pars_p, char *result_sym_p)
{
    pid_t PID;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
        RetErr = PTI_AudioStart(PID);
        sprintf(PTI_Msg, "PTI_AudioPID() : pid=0x%X : err=%d\n", PID, RetErr);
        STTBX_Print((PTI_Msg));
    }
    return(API_EnableError ? RetErr : FALSE );

 } /* end of PTI_AudioPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRPID
 *            Start the selected PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_PCRPID(parse_t *pars_p, char *result_sym_p)
{
    STCLKRV_ExtendedSTC_t   ExtendedSTC;
    pid_t                   PID;
    BOOL                    RetErr;
    ST_ErrorCode_t          ErrorCode;
    STCLKRV_Handle_t        ClkrvHdl;
    STCLKRV_OpenParams_t    stclkrv_OpenParams;

    /* get PCR PID */
    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
    }
    else
    {
        pti_pause();
         /* stop data arriving at the IIF (avoid FIFO overflow) */
        RetErr = PTI_PCRStart(PID);
        if (RetErr)
        {
            STTBX_Print(("PTI_PCRPID() : PTI_PCRStart(pid=%d) failed !\n", PID));
        }
        else
        {
            RetErr = pti_resume();
            if (RetErr)
            {
                STTBX_Print(("PTI_PCRPID() : pti_resume() failed !\n"));
            }
            else
            {
                /* delay some time so that we can get a successful GetDecodeClock() because PCR was invalidated after change */
                task_delay(20000);/* before : 2000 --> err getdecodeclk() (FQ Jan 2000)*/
                ErrorCode = STCLKRV_Open(STCLKRV_DEVICE_NAME, &stclkrv_OpenParams, &ClkrvHdl);
                if ( ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Print(("PTI_PCRPID() : STCLKRV_Open(devname=%s) failed. Err=0x%x !\n",
                                STCLKRV_DEVICE_NAME, ErrorCode));
                    RetErr = TRUE;
                    ClkrvHdl=0;
                }
                else
                {
                    ErrorCode = STCLKRV_GetExtendedSTC(ClkrvHdl, &ExtendedSTC);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("PTI_PCRPID() : STCLKRV_GetExtendedSTC() failed (err=0x%x) !\n", ErrorCode));
                    }
                    else
                    {
                        STTBX_Print(("PTI_PCRPID() : PID=%d STC=0x%x,%x\n",
                                PID, ExtendedSTC.BaseBit32, ExtendedSTC.BaseValue));
                    }
                    STCLKRV_Close(ClkrvHdl);
                }
            }
        }
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_PCRPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStopPID
 *            Stop the audio program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;

    RetErr = FALSE;
    if (SendingAudio)
    {
        RetErr = PTI_AudioStop();
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_AudioStopPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStopPID
 *            Stop the video program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;

    RetErr = FALSE;
    if (SendingVideo[CurrentVideoFifoNb-1])
    {
        RetErr = PTI_VideoStop();
    }
    return(API_EnableError ? RetErr : FALSE );
} /* end of PTI_VideoStopPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_MacroInit
 *            Register the PTI commands
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static BOOL PTI_InitCommand (void)
{
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "PTI_AudStartPID", PTI_AudioPID,
                                     "<PID> Start sending audio PID to audio decoder");
    RetErr |= STTST_RegisterCommand( "PTI_AudStopPID",  PTI_AudioStopPID,
                                     "Stop sending audio to audio decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidStartPID", PTI_VideoPID,
                                     "<PID> Start sending video PID to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidStopPID",  PTI_VideoStopPID,
                                     "Stop sending video to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_PCRPID",      PTI_PCRPID,
                                     "<PID> Start sending data PCR PID");

    return(!RetErr);

} /* end of PTI_MacroInit() */



/*##########################################################################*/
/*########################### EXPORTED FUNCTIONS ###########################*/
/*##########################################################################*/

 /*-------------------------------------------------------------------------
 * Function : PTI_InitComponent
 *            Initialization of PTI and its slots
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL PTI_InitComponent(void)
{
    BOOL RetOk;
    U8 i;

    CurrentVideoFifoNb = 1;
    for (i = 0; i < STPTI_MAX_DEVICES; i++)
    {
        SendingVideo[i] = FALSE; /* init table */
    }

    RetOk = PTI_Init();

    return(RetOk);

} /* end of PTI_InitComponent() */

 /*-------------------------------------------------------------------------
 * Function : PTI_TermComponent
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
void PTI_TermComponent(void)
{
    BOOL RetOk;

    RetOk = PTI_Term();

} /* end of PTI_TermComponent() */

 /*-------------------------------------------------------------------------
 * Function : PTI_RegisterCmd
 *            Register the commands
 * Input    :
 * Output   :
 * Return   : TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/

BOOL PTI_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = PTI_InitCommand();
    if (!RetOk)
    {
        STTBX_Print(( "PTI_RegisterCmd() \t: failed ! (LINK)\n"));
    }
    else
    {
        STTBX_Print(( "PTI_RegisterCmd() \t: ok (LINK)\n"));
    }

    return(RetOk);
} /* end of PTI_RegisterCmd() */

#endif /* USE_PTI */
/* End of link_cmd.c */
