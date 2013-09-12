/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.
 *
 *  Module      : stvid_core_proc.c
 *  Date        : 11th-07-2006
 *  Author      : Haithem ELLAFI
 *  Description : procfs implementation
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/proc_fs.h>

#include <linux/errno.h>   /* Defines standard error codes */

#include "stvid_core_proc.h"        /* Prototytpes for this compile unit */
#include "api.h"






/*** PROTOTYPES **************************************************************/
static int stvid_core_read_proc_STVID_Statistics( char *buf, char **start, off_t offset,
                                                  int count, int *eof, void *data );

static int stvid_core_read_proc_STVID_Status( char *buf, char **start, off_t offset,
                                                  int count, int *eof, void *data );


static int stvid_core_read_proc_STVID_GlobalInformation( char *buf, char **start, off_t offset,
                                                  int count, int *eof, void *data );





/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/


static struct proc_dir_entry *stvid_core_proc_root_dir = NULL; /* All proc files for this module will appear in here. */
static char   DirectoryName[30];

/*** METHODS ****************************************************************/
#ifdef STVID_DEBUG_GET_STATUS
/*-------------------------------------------------------------------------
 * Function : vid_GetScanType
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetScanType(U8 Index, char *String)
{
    switch (Index)
    {
        case 1 :
            strcpy(String, "STVID_SCAN_TYPE_PROGRESSIVE");
            break;
        case 2 :
            strcpy(String, "STVID_SCAN_TYPE_INTERLACED");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}

/*-------------------------------------------------------------------------
 * Function : vid_GetMPEGStandard
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetMPEGStandard(U8 Index, char *String)
{
    switch (Index)
    {
        case 1 :
            strcpy(String, "MPEG 1");
            break;
        case 2 :
            strcpy(String, "MPEG 2");
            break;
        case 4 :
            strcpy(String, "MPEG-4 Part 10 - H264");
            break;
        case 8 :
            strcpy(String, "VC1");
            break;
        case 16 :
            strcpy(String, "MPEG-4 Part 2 - DivX");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}

/*-------------------------------------------------------------------------
 * Function : vid_GetStateMachine
 *            Retreive ScanType string
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void vid_GetStateMachine(U8 Index, char *String)
{
    switch (Index)
    {
        case 0 :
            strcpy(String, "VIDEO_STATE_STOPPED");
            break;
        case 1 :
            strcpy(String, "VIDEO_STATE_RUNNING");
            break;
        case 2 :
            strcpy(String, "VIDEO_STATE_PAUSING");
            break;
        case 3 :
            strcpy(String, "VIDEO_STATE_FREEZING");
            break;
        default:
            strcpy(String, "UNKNOWN");
            break;
    } /* end switch */
}
#endif /* STVID_DEBUG_GET_STATUS */

/*=============================================================================

   stvid_core_init_proc_fs

   Initialise the proc file system
   Called from module initialisation routine.

   This function is called from  stvid_core_init_module() and is not reentrant.

  ===========================================================================*/
int stvid_core_init_proc_fs(char *Name)
{
	/* Store Directory Name */
	strcpy(DirectoryName,Name);

	/* Creation if not existing of /proc/STAPI directory */
	stvid_core_proc_root_dir = proc_mkdir(DirectoryName,NULL);
	if (stvid_core_proc_root_dir == NULL)
	{
	     goto fault_no_dir;
	}

	/* Creation of new entries */
	create_proc_read_entry("STVID_Statistics",
                           0444,
                           stvid_core_proc_root_dir,
                           stvid_core_read_proc_STVID_Statistics,
                           NULL);

	create_proc_read_entry("STVID_Status",
                           0444,
                           stvid_core_proc_root_dir,
                           stvid_core_read_proc_STVID_Status,
                           NULL);

	create_proc_read_entry("STVID_GlobalInformation",
                           0444,
                           stvid_core_proc_root_dir,
                           stvid_core_read_proc_STVID_GlobalInformation,
                           NULL);

    return 0;  /* If we get here then we have succeeded */

    /**************************************************************************/

    /*** Clean up on error ***/

 fault_no_dir:
    return (-1);
}

/*=============================================================================

   stvid_core_cleanup_proc_fs

   Remove the proc file system and and clean up.
   Called from module cleanup routine.

   This function is called from  stvid_core_cleanup_module() and is not reentrant.

  ===========================================================================*/
int stvid_core_cleanup_proc_fs(void)
{
    int err = 0;

    remove_proc_entry("STVID_Statistics",stvid_core_proc_root_dir);
	remove_proc_entry("STVID_Status",stvid_core_proc_root_dir);
	remove_proc_entry("STVID_GlobalInformation",stvid_core_proc_root_dir);
    remove_proc_entry(DirectoryName,NULL);

    return (err);
}


/*=============================================================================
   stvid_core_read_proc_STVID_Statistics
  ===========================================================================*/
int stvid_core_read_proc_STVID_Statistics( char *buf, char **start, off_t offset,
                                           int count, int *eof, void *data )
{
	int                len = 0;

#ifndef STVID_DEBUG_GET_STATISTICS
	len += sprintf( buf+len,"Statistics option not enabled\n");
#else /* !STVID_DEBUG_GET_STATISTICS */
	ST_ErrorCode_t     Error;
	STVID_Statistics_t Statistics;
	ST_DeviceName_t    DeviceNameArray[STVID_MAX_DEVICE];
	U32                NbDevices;
	int                i; /* Count */


	len += sprintf( buf+len,"=================\n");
	len += sprintf( buf+len,"STVID Statistics\n");
    len += sprintf( buf+len,"=================\n");

	/* Get all available devices */
	stvid_GetDeviceNameArray(DeviceNameArray, &NbDevices);

	if(NbDevices == 0)
	{
		/* No devices initialized */
		len += sprintf( buf+len,"No devices initialized\n");
	}
	else
	{
		for (i=0; i<NbDevices; i++)
		{

			len += sprintf( buf+len,"\nDeviceName : %s\n", DeviceNameArray[i]);
    		len += sprintf( buf+len,"-------------------------------\n");

			Error = STVID_GetStatistics(DeviceNameArray[i], &Statistics);
    		if( Error != ST_NO_ERROR ){
    		    len += sprintf( buf+len,"Failed STVID_GetStatistics(): 0x%x\n",Error);
    		}
			else
			{
    		    len += sprintf( buf+len,"\n ApiPbLiveResetWaitForFirstPictureDetected.....%d", Statistics.ApiPbLiveResetWaitForFirstPictureDetected );
    		    len += sprintf( buf+len,"\n ApiPbLiveResetWaitForFirstPictureDecoded......%d", Statistics.ApiPbLiveResetWaitForFirstPictureDecoded );
    		    len += sprintf( buf+len,"\n ApiPbLiveResetWaitForNextPicture..............%d", Statistics.ApiPbLiveResetWaitForNextPicture );
    		    len += sprintf( buf+len,"\n AvsyncSkippedFields...................%d", Statistics.AvsyncSkippedFields );
    		    len += sprintf( buf+len,"\n AvsyncRepeatedFields..................%d", Statistics.AvsyncRepeatedFields );
    		    len += sprintf( buf+len,"\n AvsyncMaxRepeatedFields...............%d", Statistics.AvsyncMaxRepeatedFields );
    		    len += sprintf( buf+len,"\n AvsyncFailedToSkipFields..............%d", Statistics.AvsyncFailedToSkipFields );
    		    len += sprintf( buf+len,"\n AvsyncExtendedSTCAvailable............%d", Statistics.AvsyncExtendedSTCAvailable );
    		    len += sprintf( buf+len,"\n AvsyncPictureWithNonInterpolatedPTS...%d", Statistics.AvsyncPictureWithNonInterpolatedPTS );
    		    len += sprintf( buf+len,"\n AvsyncPictureCheckedSynchronizedOk....%d", Statistics.AvsyncPictureCheckedSynchronizedOk );
    		    len += sprintf( buf+len,"\n AvsyncPTSInconsistency................%d", Statistics.AvsyncPTSInconsistency );
    		    len += sprintf( buf+len,"\n DecodeHardwareSoftReset...............%d", Statistics.DecodeHardwareSoftReset );
    		    len += sprintf( buf+len,"\n DecodeStartCodeFound..................%d", Statistics.DecodeStartCodeFound );
    		    len += sprintf( buf+len,"\n DecodeSequenceFound...................%d", Statistics.DecodeSequenceFound );
    		    len += sprintf( buf+len,"\n DecodeUserDataFound...................%d", Statistics.DecodeUserDataFound );
    		    len += sprintf( buf+len,"\n DecodePictureFound....................%d", Statistics.DecodePictureFound );
    		    len += sprintf( buf+len,"\n DecodePictureFoundMPEGFrameI..........%d", Statistics.DecodePictureFoundMPEGFrameI );
    		    len += sprintf( buf+len,"\n DecodePictureFoundMPEGFrameP..........%d", Statistics.DecodePictureFoundMPEGFrameP );
    		    len += sprintf( buf+len,"\n DecodePictureFoundMPEGFrameB..........%d", Statistics.DecodePictureFoundMPEGFrameB );
    		    len += sprintf( buf+len,"\n DecodePictureSkippedRequested.........%d", Statistics.DecodePictureSkippedRequested );
    		    len += sprintf( buf+len,"\n DecodePictureSkippedNotRequested......%d", Statistics.DecodePictureSkippedNotRequested );
    		    len += sprintf( buf+len,"\n DecodePictureDecodeLaunched...........%d", Statistics.DecodePictureDecodeLaunched );
    		    len += sprintf( buf+len,"\n DecodeStartConditionVbvDelay..........%d", Statistics.DecodeStartConditionVbvDelay );
    		    len += sprintf( buf+len,"\n DecodeStartConditionPtsTimeComparison.%d", Statistics.DecodeStartConditionPtsTimeComparison );
    		    len += sprintf( buf+len,"\n DecodeStartConditionVbvBufferSize.....%d", Statistics.DecodeStartConditionVbvBufferSize );
    		    len += sprintf( buf+len,"\n DecodeInterruptStartDecode............%d", Statistics.DecodeInterruptStartDecode );
    		    len += sprintf( buf+len,"\n DecodeInterruptPipelineIdle...........%d", Statistics.DecodeInterruptPipelineIdle );
    		    len += sprintf( buf+len,"\n DecodeInterruptDecoderIdle............%d", Statistics.DecodeInterruptDecoderIdle );
    		    len += sprintf( buf+len,"\n DecodeInterruptBitBufferEmpty.........%d", Statistics.DecodeInterruptBitBufferEmpty );
    		    len += sprintf( buf+len,"\n DecodeInterruptBitBufferFull..........%d", Statistics.DecodeInterruptBitBufferFull );
    		    len += sprintf( buf+len,"\n DecodePbStartCodeFoundInvalid.................%d", Statistics.DecodePbStartCodeFoundInvalid );
    		    len += sprintf( buf+len,"\n DecodePbStartCodeFoundVideoPES................%d", Statistics.DecodePbStartCodeFoundVideoPES );
    		    len += sprintf( buf+len,"\n DecodePbMaxNbInterruptSyntaxErrorPerPicture...%d",
    		                                                               Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture );
    		    len += sprintf( buf+len,"\n DecodePbInterruptSyntaxError..................%d", Statistics.DecodePbInterruptSyntaxError );
    		    len += sprintf( buf+len,"\n DecodePbInterruptDecodeOverflowError..........%d",
    		                                                                       Statistics.DecodePbInterruptDecodeOverflowError );
    		    len += sprintf( buf+len,"\n DecodePbInterruptDecodeUnderflowError.........%d", Statistics.DecodePbInterruptDecodeUnderflowError );
    		    len += sprintf( buf+len,"\n DecodePbDecodeTimeOutError....................%d", Statistics.DecodePbDecodeTimeOutError );
    		    len += sprintf( buf+len,"\n DecodePbInterruptMisalignmentError............%d", Statistics.DecodePbInterruptMisalignmentError );
    		    len += sprintf( buf+len,"\n DecodePbInterruptQueueOverflow................%d", Statistics.DecodePbInterruptQueueOverflow );
    		    len += sprintf( buf+len,"\n DecodePbHeaderFifoEmpty.......................%d", Statistics.DecodePbHeaderFifoEmpty );
    		    len += sprintf( buf+len,"\n DecodePbVbvSizeGreaterThanBitBuffer...........%d", Statistics.DecodePbVbvSizeGreaterThanBitBuffer );
    		    len += sprintf( buf+len,"\n DecodeMinBitBufferLevelReached........%d", Statistics.DecodeMinBitBufferLevelReached );
    		    len += sprintf( buf+len,"\n DecodeMaxBitBufferLevelReached........%d", Statistics.DecodeMaxBitBufferLevelReached );
    		    len += sprintf( buf+len,"\n DecodePbSequenceNotInMemProfileSkipped........%d", Statistics.DecodePbSequenceNotInMemProfileSkipped );
		#ifdef STVID_HARDWARE_ERROR_EVENT
    		    len += sprintf( buf+len,"\n DecodePbHardwareErrorMissingPID...............%d", Statistics.DecodePbHardwareErrorMissingPID );
    		    len += sprintf( buf+len,"\n DecodePbHardwareErrorSyntaxError..............%d", Statistics.DecodePbHardwareErrorSyntaxError );
    		    len += sprintf( buf+len,"\n DecodePbHardwareErrorTooSmallPicture..........%d", Statistics.DecodePbHardwareErrorTooSmallPicture );
		#endif /* STVID_HARDWARE_ERROR_EVENT */
    		    len += sprintf( buf+len,"\n DecodePbParserError..........%d", Statistics.DecodePbParserError );
    		    len += sprintf( buf+len,"\n DecodePbPreprocError..........%d", Statistics.DecodePbPreprocError );
    		    len += sprintf( buf+len,"\n DecodePbFirmwareError..........%d", Statistics.DecodePbFirmwareError );
    		    len += sprintf( buf+len,"\n DecodeGNBvd42696Error..........%d", Statistics.DecodeGNBvd42696Error );
    		    len += sprintf( buf+len,"\n DisplayPictureInsertedInQueue.........%d", Statistics.DisplayPictureInsertedInQueue );
    		    len += sprintf( buf+len,"\n DisplayPictureInsertedInQueueDecima...%d", Statistics.DisplayPictureInsertedInQueueDecimated );
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedByMain.........%d", Statistics.DisplayPictureDisplayedByMain);
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedByAux..........%d", Statistics.DisplayPictureDisplayedByAux);
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedBySec..........%d", Statistics.DisplayPictureDisplayedBySec);
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedDecimByMain....%d", Statistics.DisplayPictureDisplayedDecimatedByMain);
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedDecimByAux.....%d", Statistics.DisplayPictureDisplayedDecimatedByAux);
    		    len += sprintf( buf+len,"\n DisplayPictureDisplayedDecimBySec.....%d", Statistics.DisplayPictureDisplayedDecimatedBySec);
    		    len += sprintf( buf+len,"\n DisplayPbQueueLockedByLackOfPicture...........%d", Statistics.DisplayPbQueueLockedByLackOfPicture );
    		    len += sprintf( buf+len,"\n DisplayPbQueueOverflow........................%d", Statistics.DisplayPbQueueOverflow );
    		    len += sprintf( buf+len,"\n DisplayPbPictureTooLateRejectedByMain.........%d", Statistics.DisplayPbPictureTooLateRejectedByMain);
    		    len += sprintf( buf+len,"\n DisplayPbPictureTooLateRejectedByAux..........%d", Statistics.DisplayPbPictureTooLateRejectedByAux);
    		    len += sprintf( buf+len,"\n DisplayPbPictureTooLateRejectedBySec..........%d", Statistics.DisplayPbPictureTooLateRejectedBySec);
    		    len += sprintf( buf+len,"\n QueuePictureInsertedInQueue...........%d", Statistics.QueuePictureInsertedInQueue);
    		    len += sprintf( buf+len,"\n QueuePictureRemovedFromQueue..........%d", Statistics.QueuePictureRemovedFromQueue);
    		    len += sprintf( buf+len,"\n QueuePicturePushedToDisplay...........%d", Statistics.QueuePicturePushedToDisplay);
    		    len += sprintf( buf+len,"\n QueuePbPictureTooLateRejected.................%d", Statistics.QueuePbPictureTooLateRejected);
    		    len += sprintf( buf+len,"\n SpeedDisplayedIFramesNb...............%d", Statistics.SpeedDisplayIFramesNb);
    		    len += sprintf( buf+len,"\n SpeedDisplayedPFramesNb...............%d", Statistics.SpeedDisplayPFramesNb);
    		    len += sprintf( buf+len,"\n SpeedDisplayedBFramesNb...............%d", Statistics.SpeedDisplayBFramesNb);
    		    len += sprintf( buf+len,"\n SpeedMaxBitRate.......................%d", Statistics.MaxBitRate);
    		    len += sprintf( buf+len,"\n SpeedMinBitRate.......................%d", Statistics.MinBitRate);
    		    len += sprintf( buf+len,"\n SpeedLastBitRate......................%d", Statistics.LastBitRate);
    		    len += sprintf( buf+len,"\n MaxPositiveDriftRequested.............%d", Statistics.MaxPositiveDriftRequested);
				len += sprintf( buf+len,"\n MaxNegativeDriftRequested.............%d", Statistics.MaxNegativeDriftRequested);
    		    len += sprintf( buf+len,"\n SpeedDecodedIPicturesNb...............%d", Statistics.NbDecodedPicturesI);
    		    len += sprintf( buf+len,"\n SpeedDecodedPPicturesNb...............%d", Statistics.NbDecodedPicturesP);
    		    len += sprintf( buf+len,"\n SpeedDecodedBPicturesNb...............%d", Statistics.NbDecodedPicturesB);
    		    len += sprintf( buf+len,"\n SpeedSkipReturnNone...................%d", Statistics.SpeedSkipReturnNone);
    		    len += sprintf( buf+len,"\n SpeedRepeatReturnNone.................%d", Statistics.SpeedRepeatReturnNone);
    		    len += sprintf( buf+len,"\n");
			}
		}
	}
#endif /* STVID_DEBUG_GET_STATISTICS */

    *eof = 1;
    return len;
}

/*=============================================================================
   stvid_core_read_proc_STVID_Status
  ===========================================================================*/
int stvid_core_read_proc_STVID_Status( char *buf, char **start, off_t offset,
                                           int count, int *eof, void *data )
{
	int             len = 0;

#ifndef STVID_DEBUG_GET_STATUS
	len += sprintf( buf+len,"Status option not enabled\n");
#else /* !STVID_DEBUG_GET_STATUS */
	ST_ErrorCode_t  Error;
	STVID_Status_t  Status;
	char            TempString[30];
	ST_DeviceName_t DeviceNameArray[STVID_MAX_DEVICE];
	U32             NbDevices;
	int             i; /* Count */

	len += sprintf( buf+len,"============\n");
	len += sprintf( buf+len,"STVID Status\n");
    len += sprintf( buf+len,"============\n");

	/* Get all available devices */
	stvid_GetDeviceNameArray(DeviceNameArray, &NbDevices);

	if(NbDevices == 0)
	{
		/* No devices initialized */
		len += sprintf( buf+len,"No devices initialized\n");
	}
	else
	{
		for (i=0; i<NbDevices; i++)
		{
			len += sprintf( buf+len,"\nDeviceName : %s\n", DeviceNameArray[i]);
    		len += sprintf( buf+len,"-------------------------------\n");


			Error = STVID_GetStatus(DeviceNameArray[i], &Status);
    		if( Error != ST_NO_ERROR ){
    		    len += sprintf( buf+len,"Failed STVID_GetStatus(): 0x%x\n",Error);
    		}
			else
			{
				/* Allocated buffers */
				len += sprintf( buf+len, "\n BitBufferAddress_p....................0x%x", (U32)Status.BitBufferAddress_p );
				len += sprintf( buf+len, "\n BitBufferSize.........................%d", (U32)Status.BitBufferSize );
				len += sprintf( buf+len, "\n DataInputBufferAddress_p..............0x%x", (U32)Status.DataInputBufferAddress_p );
				len += sprintf( buf+len, "\n DataInputBufferSize...................%d", (U32)Status.DataInputBufferSize );
				len += sprintf( buf+len, "\n SCListBufferAddress_p.................0x%x", (U32)Status.SCListBufferAddress_p );
				len += sprintf( buf+len, "\n SCListBufferSize......................%d", (U32)Status.SCListBufferSize );
				len += sprintf( buf+len, "\n IntermediateBuffersAddress_p..........0x%x", (U32)Status.IntermediateBuffersAddress_p );
				len += sprintf( buf+len, "\n IntermediateBuffersSize...............%d", (U32)Status.IntermediateBuffersSize );
				len += sprintf( buf+len, "\n FdmaNodesSize.........................%d", (U32)Status.FdmaNodesSize );
				len += sprintf( buf+len, "\n InjectFlushBufferSize.................%d", (U32)Status.InjectFlushBufferSize );

				/* Memory profile related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n MemoryProfile.........................");
				len += sprintf( buf+len, "\n + MaxHeight...........................%d", Status.MemoryProfile.MaxHeight );
				len += sprintf( buf+len, "\n + MaxWidth............................%d", Status.MemoryProfile.MaxWidth );
				len += sprintf( buf+len, "\n + NbFrameStore........................%d", Status.MemoryProfile.NbFrameStore );

				/* SyncDelay related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n SyncDelay.............................%d", Status.SyncDelay );

				/* StateMachine related */
				len += sprintf( buf+len, "\n");
				vid_GetStateMachine(Status.StateMachine, TempString);
				len += sprintf( buf+len, "\n StateMachine..........................%s", TempString );

				/* LastStartParams related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n VideoAlreadyStarted...................%s", ((Status.VideoAlreadyStarted == TRUE) ? "TRUE" : "FALSE"));
				len += sprintf( buf+len, "\n LastStartParams.......................");
				len += sprintf( buf+len, "\n + RealTime............................%d", Status.LastStartParams.RealTime );
				len += sprintf( buf+len, "\n + UpdateDisplay.......................%d", Status.LastStartParams.UpdateDisplay );
				len += sprintf( buf+len, "\n + BrdCstProfile.......................%d", Status.LastStartParams.BrdCstProfile );
				len += sprintf( buf+len, "\n + StreamType..........................%d", Status.LastStartParams.StreamType );
				len += sprintf( buf+len, "\n + StreamID............................%d", Status.LastStartParams.StreamID );
				len += sprintf( buf+len, "\n + DecodeOnce..........................%d", Status.LastStartParams.DecodeOnce );

				/* LastFreezeParams related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n VideoAlreadyFreezed...................%s", ((Status.VideoAlreadyFreezed == TRUE) ? "TRUE" : "FALSE"));
				len += sprintf( buf+len, "\n LastFreezeParams......................");
				len += sprintf( buf+len, "\n + Mode................................%d", Status.LastFreezeParams.Mode );
				len += sprintf( buf+len, "\n + Field...............................%d", Status.LastFreezeParams.Field );

				/* ES, PES, SC Pointers related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n BitBufferReadPointer..................0x%x", (U32)Status.BitBufferReadPointer );
				len += sprintf( buf+len, "\n BitBufferWritePointer.................0x%x", (U32)Status.BitBufferWritePointer );
				len += sprintf( buf+len, "\n DataInputBufferReadPointer............0x%x", (U32)Status.DataInputBufferReadPointer );
				len += sprintf( buf+len, "\n DataInputBufferWritePointer...........0x%x", (U32)Status.DataInputBufferWritePointer );
				len += sprintf( buf+len, "\n SCListBufferReadPointer...............0x%x", (U32)Status.SCListBufferReadPointer );
				len += sprintf( buf+len, "\n SCListBufferWritePointer..............0x%x", (U32)Status.SCListBufferWritePointer );

				/* SequenceInfo related */
				len += sprintf( buf+len, "\n");
				len += sprintf( buf+len, "\n SequenceInfo..........................%x",(U32)Status.SequenceInfo_p);
				len += sprintf( buf+len, "\n + Height..............................%d", Status.SequenceInfo_p->Height );
				len += sprintf( buf+len, "\n + Width...............................%d", Status.SequenceInfo_p->Width );

				vid_GetScanType(Status.SequenceInfo_p->ScanType, TempString);
				len += sprintf( buf+len, "\n + ScanType............................%s", TempString );

				len += sprintf( buf+len, "\n + FrameRate...........................%d", Status.SequenceInfo_p->FrameRate );
				len += sprintf( buf+len, "\n + BitRate.............................%d", Status.SequenceInfo_p->BitRate );

				vid_GetMPEGStandard(Status.SequenceInfo_p->MPEGStandard, TempString);
				len += sprintf( buf+len, "\n + MPEGStandard........................%s", TempString );

				len += sprintf( buf+len, "\n + IsLowDelay..........................%d", Status.SequenceInfo_p->IsLowDelay );
				len += sprintf( buf+len, "\n + VBVBufferSize.......................%d", Status.SequenceInfo_p->VBVBufferSize );
				len += sprintf( buf+len, "\n + StreamID............................%d", Status.SequenceInfo_p->StreamID );
				len += sprintf( buf+len, "\n + ProfileAndLevelIndication...........%d", Status.SequenceInfo_p->ProfileAndLevelIndication );
				len += sprintf( buf+len, "\n + VideoFormat.........................%d", Status.SequenceInfo_p->VideoFormat );

				len += sprintf( buf+len, "\n");
			}
		}
	}
#endif /* STVID_DEBUG_GET_STATUS */

    *eof = 1;
    return len;
}


/*=============================================================================
   stvid_core_read_proc_STVID_GlobalInformation
  ===========================================================================*/
int stvid_core_read_proc_STVID_GlobalInformation( char *buf, char **start, off_t offset,
                                           int count, int *eof, void *data )
{
	int                len = 0;

#if !defined(STVID_DEBUG_GET_STATISTICS) || !defined(STVID_DEBUG_GET_STATUS)
	len += sprintf( buf+len,"Statistics or Status options not enabled\n");
#else /* defined STVID_DEBUG_GET_STATISTICS && defined STVID_DEBUG_GET_STATUS */
	ST_ErrorCode_t     Error;
	STVID_Statistics_t Statistics;
	STVID_Status_t     Status;
	U32                FrameBufferSize;
	char               TempString[30];
	ST_DeviceName_t    DeviceNameArray[STVID_MAX_DEVICE];
	U32                NbDevices;
	int                i; /* Count */

	len += sprintf( buf+len,"========================\n");
	len += sprintf( buf+len,"STVID Global Information\n");
    len += sprintf( buf+len,"========================\n");

	/* Get all available devices */
	stvid_GetDeviceNameArray(DeviceNameArray, &NbDevices);

	if(NbDevices == 0)
	{
		/* No devices initialized */
		len += sprintf( buf+len,"No devices initialized\n");
	}
	else
	{
		for (i=0; i<NbDevices; i++)
		{
			len += sprintf( buf+len,"\nDeviceName : %s\n", DeviceNameArray[i]);
    		len += sprintf( buf+len,"-------------------------------\n");

			Error = STVID_GetStatistics(DeviceNameArray[i], &Statistics);
    		if( Error != ST_NO_ERROR )
			{
    		    len += sprintf( buf+len,"Failed STVID_GetStatistics(): 0x%x\n",Error);
    		}
			else
			{
				len += sprintf( buf+len,"Decoder Errors Statistics : %d\n", (Statistics.DecodePbStartCodeFoundInvalid + Statistics.DecodePbStartCodeFoundVideoPES +
																			 Statistics.DecodePbMaxNbInterruptSyntaxErrorPerPicture + Statistics.DecodePbInterruptSyntaxError +
																			 Statistics.DecodePbInterruptDecodeOverflowError + Statistics.DecodePbInterruptDecodeUnderflowError +
																			 Statistics.DecodePbDecodeTimeOutError + Statistics.DecodePbInterruptMisalignmentError +
																			 Statistics.DecodePbInterruptQueueOverflow + Statistics.DecodePbHeaderFifoEmpty +
																			 Statistics.DecodePbVbvSizeGreaterThanBitBuffer));

				len += sprintf( buf+len,"Frames Discarded : %d\n", (Statistics.DecodePictureDecodeLaunched - Statistics.DecodeInterruptDecoderIdle + Statistics.DecodeHardwareSoftReset));
			}

			Error = STVID_GetStatus(DeviceNameArray[i], &Status);
    		if( Error != ST_NO_ERROR ){
    		    len += sprintf( buf+len,"Failed STVID_GetStatus(): 0x%x\n",Error);
    		}
			else
			{
				vid_GetMPEGStandard(Status.SequenceInfo_p->MPEGStandard, TempString);
				len += sprintf( buf+len,"Decode Stream Type : %s\n", TempString );
				len += sprintf( buf+len,"Stream Bit rate : %d\n", Status.SequenceInfo_p->BitRate);
				len += sprintf( buf+len,"Memory Allocated :\n");
				len += sprintf( buf+len,"+ BitBufferSize.........................%d\n", (U32)Status.BitBufferSize );
				len += sprintf( buf+len,"+ DataInputBufferSize...................%d\n", (U32)Status.DataInputBufferSize );
				len += sprintf( buf+len,"+ SCListBufferSize......................%d\n", (U32)Status.SCListBufferSize );
				len += sprintf( buf+len,"+ IntermediateBuffersSize...............%d\n", (U32)Status.IntermediateBuffersSize );
				len += sprintf( buf+len,"+ FdmaNodesSize.........................%d\n", (U32)Status.FdmaNodesSize );
				len += sprintf( buf+len,"+ InjectFlushBufferSize.................%d\n", (U32)Status.InjectFlushBufferSize );



				FrameBufferSize = (U32)((U32)Status.MemoryProfile.NbFrameStore * (U32)Status.MemoryProfile.MaxWidth * (U32)Status.MemoryProfile.MaxHeight);
				FrameBufferSize = (FrameBufferSize + (FrameBufferSize/2)); /* FrameBufferSize * 1.5 */

				len += sprintf( buf+len,"+ Frame Buffers.........................%d x (%d x %d) x 1.5 = %d\n", (U32)Status.MemoryProfile.NbFrameStore, (U32)Status.MemoryProfile.MaxWidth,
				                                                                          (U32)Status.MemoryProfile.MaxHeight, FrameBufferSize);
				len += sprintf( buf+len,"+ TOTAL (Bytes).........................%d\n", (FrameBufferSize + Status.BitBufferSize + Status.DataInputBufferSize  + Status.IntermediateBuffersSize + Status.FdmaNodesSize + Status.InjectFlushBufferSize));
			}

			len += sprintf( buf+len,"\n");
		}
	}
#endif /* STVID_DEBUG_GET_STATISTICS && STVID_DEBUG_GET_STATUS */

	*eof = 1;
    return len;
}

