/************************************************************************
COPYRIGHT (C) STMicroelectronics 2000

File name   : vid_util.c
Description : Definition of video extra commands (utilities)
 *            in order to test the features of STVID API 3.0.0 or later
 *            with Testtool
 *
Note        : All functions return TRUE if error for Testtool compatibility
 *
Date          Modification                                      Initials
----          ------------                                      --------
14 Aug 2000   Creation (adaptation of previous vid_test.c)         FQ


************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#ifdef ST_OS20
#include <ostime.h>
#endif
#ifdef ST_OS21
#include "stlite.h"
#endif
#include "stvid.h"
/*#include "staud.h"*/
#include "stddefs.h"
#include "stdevice.h"
#ifdef ST_OS20
#include "debug.h"
#endif
#ifdef ST_OS21
#include "os21debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#else
#include "pti.h"
#endif
/* workaround for link 2.0.1 */
#ifdef DVD_TRANSPORT_LINK
#include "ptilink.h"
#endif
#if defined(ST_7015)
#include "sti7015.h"
#include "mb295.h"
#endif
#if defined(ST_7020)
#include "sti7020.h"
#include "mb295.h"
#endif
#include "stavmem.h"
#include "stsys.h"
#include "stevt.h"
#include "stclkrv.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

#if defined(ST_7015) || defined(ST_7020)
/* #define STPTI_MAX_DEVICES     5 --> error 0x04 on STPTI2 at init */
#define STPTI_MAX_DEVICES     1
#else
#define STPTI_MAX_DEVICES     1
#endif

#define NB_MAX_OF_COPIED_PICTURES  2         /* pictures in memory for show/hide tests */

#define NB_OF_VIDEO_EVENTS         18
#define NB_MAX_OF_STACKED_EVENTS   60

#ifdef DVD_TRANSPORT_STPTI
typedef unsigned int pid_t; /* defined as int for pti/link, but U16 for spti */
#endif

/* --- Global variables ------------------------------------------------ */

STAVMEM_BlockHandle_t   API_DestBufferHnd;   /* handle for avmem */

static STAVMEM_BlockHandle_t  CopiedPictureHandle[NB_MAX_OF_COPIED_PICTURES];

static char   VID_Msg[800];                  /* text for trace */
static void  *VID_FileBuffer = NULL ;        /* address of stream buffer */
static int    VID_NbBytes;                   /* size of stream buffer */
static task_t *VIDTid;                       /* for injection in memory */
static int    InjectLoop;                    /* for injection in memory */
static BOOL VID_MemInjectStarted = FALSE;    /* for injection in memory */

typedef struct {
    void *                              StartAddress_p;
    U32                                 BufferSize;
    U32                                 LastFilledTransferSize;
    STAVMEM_BlockHandle_t               AvmemBlockHandle;
} VIDTRICK_BackwardBuffer_t;

VIDTRICK_BackwardBuffer_t BufferA;

/* --- Externals ------------------------------------------------------- */

extern STVID_Handle_t          VidDecHndl0;         /* handle for video */
extern STVID_ViewPortHandle_t  ViewPortHndl0;       /* handle for viewport */
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;
extern S16  NbOfCopiedPictures;
extern STVID_PictureParams_t CopiedPictureParams[NB_MAX_OF_COPIED_PICTURES];

#if 0
extern void VideoIntPrintStats(void);
extern void VideoIntResetStats(void);
#endif

extern BOOL PTI_VideoStart(pid_t);
extern BOOL PTI_VideoStopPID(void) ;
extern boolean PTI_AudioStopPID(void);
extern BOOL PTI_PCRStop(void);
extern boolean PTI_PCRStart(pid_t);
extern BOOL SetEncodingMode(const U8 EncodingMode);

/*#######################################################################*/
/*########################## EXTRA COMMANDS #############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_ClearStatus
 * ----------------------------------------------------------------------*/
static BOOL VID_ClearStatus( parse_t *pars_p, char *result_sym_p )
{
#if 0
    VideoIntResetStats();
#endif
    return ( FALSE );

} /* end of VID_ClearStatus() */

/*-------------------------------------------------------------------------
 * Function : VID_GetStatus
 *            Get video decoder status
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetStatus( parse_t *pars_p, char *result_sym_p )
{

#if 0
    VideoIntPrintStats();
#endif
    return ( FALSE );

} /* end of VID_GetStatus() */


/*-------------------------------------------------------------------------
 * Function : DoVideoInject
 *            Function is called by subtask
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoVideoInject( void *Loop_p )
{

#ifdef DVD_TRANSPORT_STPTI
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;
    STPTI_DMAParams_t       STDMA_Params;
#if defined(ST_7015) || defined(ST_7020)
    U8 *Ptr;
    U32 BBL, BitBufferSize, BitBufferThreshold, PacketSize, InjectedPacketSize;

#if defined(ST_7015)
    STDMA_Params.Destination = (STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO_OFFSET); /* XD change to new define */
#else
    STDMA_Params.Destination = (STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO_OFFSET);
#endif
    STDMA_Params.Holdoff = 1;
    STDMA_Params.WriteLength = 4;

#if defined (mb295)
    /* Bug in STPTI with mb295, WA: Configure PTI address ! */
    STSYS_WriteRegDev32LE(PTI_BASE_ADDRESS + 0x1060, STDMA_Params.Destination);
#endif
    STTBX_Print(("BBG=0x%x BBS=0x%x", STSYS_ReadRegDev32LE(0x60000240), STSYS_ReadRegDev32LE(0x60000244)));
    BitBufferSize = (STSYS_ReadRegDev32LE(0x60000244) & 0x03FFFF00) - (STSYS_ReadRegDev32LE(0x60000240) & 0x03FFFF00) + 256;
    PacketSize = ((BitBufferSize / 8)) & (~3);
    BitBufferThreshold = BitBufferSize - PacketSize - 512;  /* 512 to avoid bit buffer overflow */
    BitBufferThreshold -= PacketSize; /* Reduce again ! */
    STSYS_WriteRegDev32LE(0x60000278, 0x2);    /* Disable PBO */
    STSYS_WriteRegDev32LE(0x60000248, BitBufferThreshold);   /* Change Threshold */
    STTBX_Print(("BitBufferSize=%u bytes\n",BitBufferSize));
    STTBX_Print(("BitBufferThreshold=%u bytes\n",BitBufferThreshold));
    STTBX_Print(("PacketSize=%u bytes\n",PacketSize));

    for ( ; (InjectLoop > 0) && (ErrCode == ST_NO_ERROR); InjectLoop -- )
    {
        Ptr = (U8 *) VID_FileBuffer;
        for ( Ptr = (U8 *) VID_FileBuffer; Ptr < ((U8 *) VID_FileBuffer + VID_NbBytes); )
        {
            BBL = STSYS_ReadRegDev32LE(0x6000024c) & 0x03FFFF00;
            if (BBL < BitBufferThreshold)
            {
                InjectedPacketSize = (U32) VID_FileBuffer - (U32) Ptr + VID_NbBytes;
                if (InjectedPacketSize > PacketSize)
                {
                    InjectedPacketSize = PacketSize;
                }
#if 1
                ErrCode = STPTI_UserDataWrite((U8 *) Ptr, (U32)InjectedPacketSize, &STDMA_Params);
                ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);  /* Wait before use */
                Ptr += InjectedPacketSize;
#else
                /* Testing manually */
                BBL = (U32) Ptr + InjectedPacketSize;
                while (Ptr < (U8 *) BBL)
                {
#if defined(ST_7015)
                    STSYS_WriteRegDev32LE(STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO_OFFSET, *((U32 *) Ptr) );
#else
                    STSYS_WriteRegDev32LE(STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO_OFFSET, *((U32 *) Ptr) );
#endif
                    Ptr += 4;
                }
#endif
            }
            else
            {
                task_reschedule();
            }
        }
    }

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error in DoVideoInject()\n"));
    }

#else
	VID_MemInjectStarted = TRUE;

#if defined(ST_7015) || defined(ST_7020)
    STDMA_Params.Destination = VIDEO_CD_FIFO1;
#else
    STDMA_Params.Destination = VIDEO_CD_FIFO;
#endif
    STDMA_Params.Holdoff = 1;
    STDMA_Params.WriteLength = 1;

    for ( ; (InjectLoop > 0) && (ErrCode == ST_NO_ERROR); InjectLoop -- )
    {
        ErrCode = STPTI_UserDataWrite((U8 *)VID_FileBuffer, VID_NbBytes, &STDMA_Params);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STPTI_UserDataWrite(): error=%d\n", ErrCode));
        }
        else
        {
            ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_UserDataSynchronize() : error=%d\n", ErrCode));
            }
        }
    }
#endif
#else /* link or pti */
	VID_MemInjectStarted = TRUE;

    pti_video_dma_state( discard_data );
    pti_video_dma_reset();
    #ifdef DVD_TRANSPORT_LINK
    pti_video_dma_open() ;
    #endif
    for ( ; InjectLoop > 0; InjectLoop -- )
    {
        pti_video_dma_inject( (unsigned char*)VID_FileBuffer, (int) VID_NbBytes);
        pti_video_dma_synchronize();
    }
    #ifdef DVD_TRANSPORT_LINK
    pti_video_dma_close() ;
    #endif
#endif
} /* end of DoVideoInject() */

/*-------------------------------------------------------------------------
 * Function : VID_KillTask
 *            Kill Video Inject task
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VID_KillTask( parse_t *pars_p, char *Result )
{
    task_t *TaskList[1];

    /* Reset global variable and wait for task to finish before deleting */
    InjectLoop =  0;
    if ( VIDTid != NULL )
    {
        TaskList[0] = VIDTid;
        if ( task_wait( TaskList, 1, TIMEOUT_INFINITY ) == -1 )
        {
            STTBX_Print(("Error: Timeout task_wait\n"));
        }
        STTBX_Print(("Video Task finished\n"));
        if ( task_delete( VIDTid ) != 0 )
        {
            STTBX_Print(("Error: Task Delete (VID)\n"));
        }
		VID_MemInjectStarted = FALSE;
        VIDTid = NULL;
    }
    return(FALSE);
} /* end of VID_KillTask() */

/*-------------------------------------------------------------------------
 * Function : VID_MemInject
 *            Inject Video in Memory to DMA
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_MemInject( parse_t *pars_p, char *Result )
{
    void *Loop_p = &InjectLoop;

	if ( VID_MemInjectStarted == TRUE )
	{
        STTBX_Print(("Task 'VideoInject' not deleted : run VID_KILL before\n"));
        return(TRUE);
	}
    if ( VID_FileBuffer == NULL )
    {
        STTBX_Print(("No Video in memory\n"));
        return(TRUE);
    }

    STTST_GetInteger( pars_p, 1, (S32 *)&(InjectLoop) );
#ifdef ST_OS20
    if (( VIDTid = task_create( DoVideoInject, Loop_p, 4096, 0, "VideoInject", 0 )) == NULL )
#endif
#ifdef ST_OS21
    if (( VIDTid = task_create( DoVideoInject, Loop_p, 16384, 0, "VideoInject", 0 )) == NULL )
#endif
    {
        STTBX_Print(("Unable to create video task\n"));
    }
    else
    {
        STTBX_Print(("Playing video %d times\n", InjectLoop ));
    }
    return(FALSE);

} /* end of VID_MemInject() */


/*-------------------------------------------------------------------------
 * Function : VID_Load
 *            Load a mpeg file in memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VID_Load(parse_t *pars_p, char *result_sym_p )
{
  extern ST_Partition_t *SystemPartition;

  char FileName[80];
  static char *AllocatedBuffer = NULL;
  BOOL RetErr;
  long int FileDescriptor, FileSize;
#ifdef ST_OS21
    struct stat FileStatus;
#endif /* ST_OS21 */

	/* get file name */
	memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      STTST_TagCurrentLine( pars_p, "expected FileName" );
    }
	else
	{
        if (AllocatedBuffer != NULL)
        {
            memory_deallocate(SystemPartition, AllocatedBuffer);
            AllocatedBuffer = NULL;
        }
	    /* Get memory to store file data */
#ifdef ST_OS20
        FileDescriptor = debugopen(FileName, "rb" );
#endif
#ifdef ST_OS21
        FileDescriptor  = open(FileName, O_RDONLY | O_BINARY  );
#endif /* ST_OS21 */
        if (FileDescriptor< 0 )
        {
            sprintf(VID_Msg, "unable to open %s ...\n", FileName);
   			STTBX_Print((VID_Msg));
            return ( FALSE );
        }

        STTBX_Print(("loading...\n"));
#ifdef ST_OS20
        FileSize = debugfilesize(FileDescriptor);
#endif
#ifdef ST_OS21
    fstat(FileDescriptor, &FileStatus);
    FileSize = FileStatus.st_size;
#endif /* ST_OS21 */


        /* ---  round up "size" to a multiple of 16 bytes, and malloc it --- */
        while ( FileSize > 0 && AllocatedBuffer == NULL )
        {
            AllocatedBuffer = (char *)memory_allocate(SystemPartition, (U32)((FileSize + 15) & (~15)) );
            if (AllocatedBuffer == NULL)
            {
                FileSize-=200000; /* file too big : truncate the size */
                sprintf(VID_Msg, "Not enough memory for file loading : try to truncate to %ld \n",FileSize);
                STTBX_Print((VID_Msg));
                if ( FileSize < 0 )
                {
                    return(API_EnableError ? TRUE : FALSE );
                }
            }
        }

        /* --- Setup the alligned buffer, load file and return --- */
        VID_FileBuffer = (void *)(((unsigned int)AllocatedBuffer + 15) & (~15));
        VID_NbBytes = (int)FileSize;
        debugread(FileDescriptor, VID_FileBuffer, (size_t) FileSize);
        debugclose(FileDescriptor);

        sprintf(VID_Msg, "file [%s] loaded : size %ld at address %x \n",
                      FileName, FileSize, (int)VID_FileBuffer);
		STTBX_Print((VID_Msg));

	} /* end param ok */

	return(API_EnableError ? RetErr : FALSE );

} /* end of VID_Load() */


/*-------------------------------------------------------------------------
 * Function : VIDDecodeFromMemory
 *            copy a stream from buffer to video bit buffer
 *             ( all the data in one time,
 *               or 10% by 10% of the data,
 *			     or 100 bytes by 100 bytes )
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VIDDecodeFromMemory(parse_t *pars_p, char *result_sym_p)
{
    #define VIDEO_FLUSH_SIZE  128      /* The video input CD fifo is 128 bytes */
    unsigned char video_flush[VIDEO_FLUSH_SIZE + 4];
    unsigned char *BuffPt;
    unsigned int i, j;
    S32 LVar;
    BOOL RetErr;
    int NbOfDecodedBytes;
    int DecodeSize;
    int DecodeMode;
#ifdef ST_OS20
    int DecodeDelay;
#endif
#ifdef ST_OS21
    osclock_t DecodeDelay;
#endif
    int NbMaxOfLoop;
    int NbOfLoop;
#ifdef DVD_TRANSPORT_STPTI
    STPTI_DMAParams_t       STDMA_Params;
    ST_ErrorCode_t          ErrCode;
#endif
#ifdef ST_7015 || defined(ST_7020)
    S32 FifoNb;
#ifdef DVD_TRANSPORT_STPTI
    U8 *Ptr;
    U32 BBL, BitBufferSize, BitBufferThreshold, PacketSize, InjectedPacketSize;
#endif
#endif

	/* get mode (0=decode all, 1=decode next packet) */
	RetErr = STTST_GetInteger( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
   		STTST_TagCurrentLine( pars_p, "expected mode (0=all data,1=10% by 10%, 100=100bytes by 100 bytes)" );
		return(API_EnableError ? TRUE : FALSE );
	}
	else
	{
		DecodeMode =(int)LVar;
	}
	/* get delay (only with mode 1 and 100, else null) */
	RetErr = STTST_GetInteger( pars_p, 0, &LVar );
	if ( RetErr == TRUE )
	{
   		STTST_TagCurrentLine( pars_p, "expected delay (1000, 2000, ...)" );
		return(API_EnableError ? TRUE : FALSE );
	}
	else
	{
		DecodeDelay =(int)LVar;
	}
#ifdef ST_7015 || defined(ST_7020)
    RetErr = STTST_GetInteger( pars_p, 1, &FifoNb);
    if ( RetErr == TRUE || (FifoNb < 1 && FifoNb > STPTI_MAX_DEVICES) )
    {
        sprintf(VID_Msg, "expected Fifo number (1 to %d)", STPTI_MAX_DEVICES);
        STTST_TagCurrentLine( pars_p, VID_Msg);
    }
#endif
    /* --- Prepare data --- */

  	/* the decoder needs to reach a start code in order to become idle after a
	 * picture decode; we insert here a picture start code 00 00 01 00 */
	i = 0;
	video_flush[i++] = 0xFF; /* 00 */
 	video_flush[i++] = 0xFF; /* 00 */
    video_flush[i++] = 0xFF; /* 01 */
    video_flush[i++] = 0xFF; /* 00 */
    /* the input decoder FIFO is of VIDEO_FLUSH_SIZE bytes, in order to
    * be sure that all data sent will be decoded we need to flush all that FIFO
    * into the bit buffer.
    * To do this we will send VIDEO_FLUSH_SIZE extra bytes at the end of any data
    * sending, just to make sure all relevant data are flushed.	*/
	for (j = i; j < VIDEO_FLUSH_SIZE; j++)
	{
    	video_flush[j] = 0xFF;
	}

    NbOfLoop = 0;
    NbMaxOfLoop = 1;
    NbOfDecodedBytes = 0;
    DecodeSize = VID_NbBytes; /* all data */
    if ( DecodeMode == 1 )
    {
        NbMaxOfLoop = 10;
        DecodeSize = VID_NbBytes / 10; /* 10% of the data */
    }
    if ( DecodeMode == 100 )
    {
		NbMaxOfLoop = VID_NbBytes / 100;
		if ( VID_NbBytes % 100 > 0 )
		{
			NbMaxOfLoop++;
		}
        DecodeSize = 100; /* 100 bytes */
    }

    /* --- Send data --- */

    while ( NbOfLoop < NbMaxOfLoop )
    {
	    if ( NbOfLoop+1 == NbMaxOfLoop )
	    {
            DecodeSize = VID_NbBytes - NbOfDecodedBytes;  /* exact number of remaining bytes */
    	}
	    BuffPt = (unsigned char *)VID_FileBuffer + NbOfDecodedBytes;

	    /* Start copy from VID_FileBuffer to video bit buffer */

        /* Caution (13 Feb 2001): traces needs a lot of CPU time, a VSync may
           be skipped, so event test gives unexpected results ;
           if necessary I will add later a display option to this command */
        /*FQsprintf(VID_Msg, "Transfer of %d bytes from %x address to video bit buffer...\n", DecodeSize, (int)BuffPt );
   			STTBX_Print((VID_Msg));*/

	    /*SendingVideo = TRUE;*/
#ifdef DVD_TRANSPORT_STPTI
#if defined(ST_7015) || defined(ST_7020)

#if defined(ST_7015)
    STDMA_Params.Destination = (STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO_OFFSET); /* XD change to new define */
#else
    STDMA_Params.Destination = (STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO_OFFSET); /* XD change to new define */
#endif
    STDMA_Params.Holdoff = 1;
    STDMA_Params.WriteLength = 4;

#if defined (mb295)
    /* Bug in STPTI with mb295, WA: Configure PTI address ! */
    STSYS_WriteRegDev32LE(PTI_BASE_ADDRESS + 0x1060, STDMA_Params.Destination);
#endif
    BitBufferSize = (STSYS_ReadRegDev32LE(0x60000244) & 0x03FFFF00) - (STSYS_ReadRegDev32LE(0x60000240) & 0x03FFFF00) + 256;
    PacketSize = ((BitBufferSize / 8)) & (~3);
    BitBufferThreshold = BitBufferSize - PacketSize - 512;  /* 512 to avoid bit buffer overflow */
    BitBufferThreshold -= PacketSize; /* Reduce again ! */
    STSYS_WriteRegDev32LE(0x60000278, 0x2);    /* Disable PBO */
    STSYS_WriteRegDev32LE(0x60000248, BitBufferThreshold);   /* Change Threshold */
    STTBX_Print(("BitBufferSize=%u bytes\n",BitBufferSize));
    STTBX_Print(("BitBufferThreshold=%u bytes\n",BitBufferThreshold));
    STTBX_Print(("PacketSize=%u bytes\n",PacketSize));

    Ptr = (U8 *) VID_FileBuffer;
    for ( Ptr = (U8 *) VID_FileBuffer; Ptr < ((U8 *) VID_FileBuffer + VID_NbBytes); )
    {
        BBL = STSYS_ReadRegDev32LE(0x6000024c) & 0x03FFFF00;
        if (BBL < BitBufferThreshold)
        {
            InjectedPacketSize = (U32) VID_FileBuffer - (U32) Ptr + VID_NbBytes;
            if (InjectedPacketSize > PacketSize)
            {
                InjectedPacketSize = PacketSize;
            }
#if 1
            ErrCode = STPTI_UserDataWrite((U8 *)Ptr, (U32)InjectedPacketSize, &STDMA_Params);
            ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);  /* Wait before use */
            Ptr += InjectedPacketSize;
#else
            i = (U32) Ptr + InjectedPacketSize;
            while (Ptr < (U8 *) i)
            {
#if defined(ST_7015)
                STSYS_WriteRegDev32LE(STI7015_BASE_ADDRESS + ST7015_VIDEO_CD_FIFO_OFFSET, *((U32 *) Ptr) );
#else
                STSYS_WriteRegDev32LE(STI7020_BASE_ADDRESS + ST7020_VIDEO_CD_FIFO_OFFSET, *((U32 *) Ptr) );
#endif
                Ptr += 4;
            }
#endif
        }
        else
        {
            task_reschedule();
        }
    }

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Error in DoVideoInject()\n"));
    }
#else
        /* device N should send the PES on FIFO N through slot N */
        #if defined (ST_7015) || defined (ST_7020)
        STDMA_Params.Destination = VIDEO_CD_FIFO1;
        if ( FifoNb==2 )
            STDMA_Params.Destination = VIDEO_CD_FIFO2;
        if ( FifoNb==3 )
            STDMA_Params.Destination = VIDEO_CD_FIFO3;
        if ( FifoNb==4 )
            STDMA_Params.Destination = VIDEO_CD_FIFO4;
        if ( FifoNb==5 )
            STDMA_Params.Destination = VIDEO_CD_FIFO5;
        #else
        STDMA_Params.Destination = VIDEO_CD_FIFO;
        #endif
        STDMA_Params.Holdoff = 1;
        STDMA_Params.WriteLength = 1;
        ErrCode = STPTI_UserDataWrite((U8 *)BuffPt, (U32)DecodeSize, &STDMA_Params);
        if ( ErrCode != ST_NO_ERROR )
        {
	        NbOfDecodedBytes += DecodeSize ;
        }
        /*FQSTTBX_Print(("STPTI_UserDataWrite() : ErrCode=%x, %d bytes bufferized\n",
			          ErrCode, NbOfDecodedBytes ));*/

        ErrCode = STPTI_UserDataSynchronize(&STDMA_Params);
        /*FQ STTBX_Print(("STPTI_UserDataSynchronize() : RetErr=%x \n", ErrCode));*/
#endif
#else /* link or pti */
        #ifdef DVD_TRANSPORT_LINK
        pti_video_dma_open() ;
        #endif
        RetErr = pti_video_dma_inject(BuffPt, DecodeSize);
        if ( RetErr != FALSE )
        {
	        NbOfDecodedBytes += DecodeSize ;
        }
        STTBX_Print(("pti_video_dma_inject() : RetErr=%x, %d bytes bufferized\n",
			    RetErr, NbOfDecodedBytes ));

        RetErr = pti_video_dma_synchronize(); /* not blocking after error */
        STTBX_Print(("pti_video_dma_synchronize() : RetErr=%x \n", RetErr));
        #ifdef DVD_TRANSPORT_LINK
        pti_video_dma_close() ;
        #endif
#endif

	    /* Flush after copy of the complete picture */
	    if ( NbOfDecodedBytes>=VID_NbBytes )
	    {
            /***
		    RetErr = pti_video_dma_inject(video_flush, VIDEO_FLUSH_SIZE);
	        sprintf(VID_Msg, "pti_video_dma_inject(video_flush) : RetErr=%x \n", RetErr);
            STTBX_Print((VID_Msg));

	  	    RetErr= pti_video_dma_synchronize();
	        sprintf(VID_Msg, "pti_video_dma_synchronize() : RetErr=%x \n", RetErr);
            STTBX_Print((VID_Msg));
            ***/
            STTBX_Print(("All the picture is copied\n"));
		    NbOfDecodedBytes = 0; /* reset count */
	    }

	    /* Now wait for the display to be finished : wait one PAL frame */
	    task_delay(40000 / 64);

	    task_delay(DecodeDelay);
        NbOfLoop++;
    } /* end while */


    return(FALSE);

} /* end of VIDDecodeFromMemory() */


/*-------------------------------------------------------------------------
 * Function : CopyVideoBuffer
 *            copy a picture from video buffer to another video buffer
 *            (buffer allocation for picture saving)
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL CopyVideoBuffer(parse_t *pars_p, char *result_sym_p)
{
/***
    BOOL RetErr;
    void *VID_BufferSrcAddr;
    void *VID_BufferDestAddr;
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    void *ForbidBorder[1];

	RetErr = FALSE;
***/
	/* get source address of video buffer (from picture param.) */
	/* allocate a destination video buffer in Mpeg stack */

/***
    if (STAVMEM_GetBlockAddress(AvmemPartitionHandle, VID_PictParams.MemoryHandle, &VID_BufferSrcAddr) != ST_NO_ERROR)
    {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address in CopyVideoBuffer()"));
    }

    AvmemAllocParams.Size = VID_PictParams.Size;
    AvmemAllocParams.Alignment = 0;
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    * HG: check this! *
    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 1;
    AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
    ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x200000);
    if (STAVMEM_AllocBlock(AvmemPartitionHandle, &AvmemAllocParams, &API_DestBufferHnd) != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error allocating in CopyVideoBuffer()"));
        RetErr = TRUE;
    }
	else
	{
***/
		/* get destination buffer address */
		/* copy video buffer from source to destination */

/***
        if (STAVMEM_GetBlockAddress(AvmemPartitionHandle, API_DestBufferHnd, &VID_BufferDestAddr) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error getting address in CopyVideoBuffer()"));
        }


        if (STAVMEM_CopyBlock1D(AvmemPartitionHandle, VID_BufferSrcAddr, VID_BufferDestAddr, VID_PictParams.Size) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error copying in CopyVideoBuffer()"));
        }
    }
***/
	return(FALSE);

} /* end of CopyVideoBuffer() */

/*-------------------------------------------------------------------------
 * Function : FreeVideoBuffer
 *            free the allocated video buffer
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL FreeVideoBuffer(parse_t *pars_p, char *result_sym_p)
{
    STAVMEM_FreeBlockParams_t AvmemFreeParams;

    AvmemFreeParams.PartitionHandle = AvmemPartitionHandle;
    if (STAVMEM_FreeBlock(&AvmemFreeParams, &API_DestBufferHnd) != ST_NO_ERROR)
    {
        /* Error handling */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error freeing in FreeVideoBuffer()"));
    }

	return(FALSE);

} /* end of FreeVideoBuffer() */



/*-------------------------------------------------------------------------
 * Function : Vid_Copy
 *            Copy last displayed picture into memory
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Copy(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    STAVMEM_FreeBlockParams_t AvmemFreeParams;
    STVID_PictureParams_t SourcePictureParams, DestPictureParams;
    STAVMEM_BlockHandle_t  DestPictureHandle;
    void *ForbidBorder[3];
    void *Address_p;
    char Param[80];
    ST_ErrorCode_t Error;

    /* Free memory ? */
    Param[0] = '\0';
    RetErr = STTST_GetString( pars_p, "", Param, sizeof(Param) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Param (\"FREE\" to free pictures)" );
    }

    if ( strcmp(Param,"FREE")==0 || strcmp(Param,"free")==0 )
    {
        while (NbOfCopiedPictures > 0 )
        {
            if (CopiedPictureHandle[NbOfCopiedPictures-1] != STAVMEM_INVALID_BLOCK_HANDLE)
            {
                AvmemFreeParams.PartitionHandle = AvmemPartitionHandle;
                if (STAVMEM_FreeBlock(&AvmemFreeParams, &(CopiedPictureHandle[NbOfCopiedPictures-1])) != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error freeing in VID_Copy()"));
                }

                CopiedPictureHandle[NbOfCopiedPictures-1] = STAVMEM_INVALID_BLOCK_HANDLE;
            }
            NbOfCopiedPictures--;
            STTBX_Print( ("VID_Copy() : memory for pictures is now free\n"));
        }
        return(FALSE);
    } /* end if free required */

    if ( NbOfCopiedPictures >= NB_MAX_OF_COPIED_PICTURES )
    {
        STTBX_Print( ("VID_Copy() : too many pictures in memory !\n"));
        return(API_EnableError ? TRUE : FALSE );
    }
    /*CopiedPictureHandle[NbOfCopiedPictures] = STAVMEM_INVALID_BLOCK_HANDLE;*/

    /* Get source picture info */

    Error = STVID_GetPictureParams(VidDecHndl0, STVID_PICTURE_DISPLAYED, &SourcePictureParams);
    if (Error != ST_NO_ERROR)
    {
        sprintf(VID_Msg, "VID_Copy() : error STVID_GetPictureParams %d\n", Error);
        STTBX_Print((VID_Msg));
    }
    else
    {
        /* Get Size and alignment */
        Error = STVID_GetPictureAllocParams(VidDecHndl0, &SourcePictureParams, &AvmemAllocParams);
        if (Error != ST_NO_ERROR)
        {
            sprintf( VID_Msg, "VID_Copy() : error STVID_GetPictureAllocParams %d\n", Error);
            STTBX_Print((VID_Msg));
        }
        else
        {
            /* Memory allocation */
            AvmemAllocParams.PartitionHandle = AvmemPartitionHandle;
            AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
            /* HG: check this! */
            AvmemAllocParams.NumberOfForbiddenRanges = 0;
            AvmemAllocParams.ForbiddenRangeArray_p = NULL;
            AvmemAllocParams.NumberOfForbiddenBorders = 3;
            AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
            ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x200000);
            ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
            ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0x600000);
            if (STAVMEM_AllocBlock(&AvmemAllocParams, &(DestPictureHandle)) != ST_NO_ERROR)
            {
                /* Error handling */
                sprintf(VID_Msg, "VID_Copy() : error STAVMEM_AllocBlock, size %d\n", AvmemAllocParams.Size);
                STTBX_Print((VID_Msg));
            }
            else
            {
                if (STAVMEM_GetBlockAddress(DestPictureHandle, &Address_p) != ST_NO_ERROR)
                {
                    /* Error handling */
                    STTBX_Print(("Error getting address in VID_Copy()"));
                }

                /* Store source picture in memory */
                DestPictureParams.Data = Address_p;
                Error = STVID_DuplicatePicture(&SourcePictureParams, &DestPictureParams);
	            if (Error != ST_NO_ERROR)
                {
                    sprintf(VID_Msg, "VID_Copy() : STVID_DuplicatePicture error %d\n", Error);
                    STTBX_Print((VID_Msg));
                }
  	            else
                {
                    STTBX_Print(("VID_Copy() : done\n"));
                    CopiedPictureParams[NbOfCopiedPictures] = DestPictureParams;
                    CopiedPictureHandle[NbOfCopiedPictures] = DestPictureHandle;
                    NbOfCopiedPictures++;
                    RetErr =  FALSE;
                }
            }
        }
    }

    return(API_EnableError ? RetErr : FALSE );
} /* end of VID_Copy() */

/*-------------------------------------------------------------------------
 * Function : VID_StorePicture
 *            Store a copied picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_StorePicture( parse_t *pars_p, char *result_sym_p )
{
STVID_PictureParams_t PictParams;
BOOL RetErr;
short PictureNo;
long int FilePict;
char FileName[80];


    if ( NbOfCopiedPictures == 0 )
    {
        STTST_TagCurrentLine( pars_p, "VID_StorePicture() : no picture to store placed in memory !" );
        return(API_EnableError ? TRUE : FALSE );
    }

    /* get picture number */
    RetErr = STTST_GetInteger( pars_p, NbOfCopiedPictures, (S32 *)&PictureNo );
    if ( RetErr == TRUE )
    {
        sprintf( VID_Msg, "expected Picture No. (max is %d)", NbOfCopiedPictures );
        STTST_TagCurrentLine( pars_p, VID_Msg );
    }

	memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "../scripts/pictyuv", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      STTST_TagCurrentLine( pars_p, "expected FileName" );
    }


    if ( PictureNo > NbOfCopiedPictures )
    {
        PictureNo = NbOfCopiedPictures ;
    }
    if ( PictureNo <= 0 )
    {
        PictureNo = 1 ;
    }

    PictParams = CopiedPictureParams[PictureNo-1];
#ifdef ST_OS20
    FilePict = debugopen( FileName,"w+b" );
#endif
#ifdef ST_OS21
    FilePict  = open(FileName, O_WRONLY | O_BINARY  );
#endif /* ST_OS21 */

    if (FilePict == -1)
	{
        STTBX_Print(("Open failed : the state of the file must not be Read-Only.\n"));
		return (FALSE);
	}
    debugwrite( FilePict,&PictParams,sizeof(STVID_PictureParams_t)) ;
    debugwrite( FilePict,PictParams.Data,PictParams.Size) ;

    /* Close the files */
    debugclose(FilePict);


    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_StorePicture() */

/*-------------------------------------------------------------------------
 * Function : VID_RestorePicture
 *            Restore a stored picture structure
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_RestorePicture( parse_t *pars_p, char *result_sym_p )
{
STVID_PictureParams_t PictParams;
BOOL RetErr;
long int FilePict, FileSize;
char FileName[80];
STAVMEM_AllocBlockParams_t AvmemAllocParams;
STAVMEM_BlockHandle_t  PictHandle;
void *ForbidBorder[3];
void *Address_p;
ST_ErrorCode_t Error;
#ifdef ST_OS21
    struct stat FileStatus;
#endif /* ST_OS21 */

    if ( NbOfCopiedPictures >= NB_MAX_OF_COPIED_PICTURES )
    {
        STTBX_Print( ("VID_Restore : too many pictures in memory !\n"));
        return(API_EnableError ? TRUE : FALSE );
    }


	memset( FileName, 0, sizeof(FileName));
    RetErr = STTST_GetString( pars_p, "../scripts/pictyuv", FileName, sizeof(FileName) );
    if ( RetErr == TRUE )
    {
      STTST_TagCurrentLine( pars_p, "expected FileName" );
    }
#ifdef ST_OS20
    FilePict = debugopen( FileName,"r+b" );
#endif
#ifdef ST_OS21
    FilePict  = open(FileName, O_RDONLY | O_BINARY  );
#endif /* ST_OS21 */

    if (FilePict == -1)
	{
        STTBX_Print(("Open failed : the state of the file must not be Read-Only.\n"));
		return (FALSE);
	}

#ifdef ST_OS20
    FileSize = debugfilesize(FilePict);
#endif
#ifdef ST_OS21
    fstat(FilePict, &FileStatus);
    FileSize = FileStatus.st_size;
#endif /* ST_OS21 */

    debugread( FilePict,&PictParams,sizeof(STVID_PictureParams_t)) ;

    /* Get Size and alignment */
    Error = STVID_GetPictureAllocParams(VidDecHndl0, &PictParams, &AvmemAllocParams);
    if (Error != ST_NO_ERROR)
    {
        sprintf( VID_Msg, "VID_Copy() : error STVID_GetPictureAllocParams %d\n", Error);
        STTBX_Print((VID_Msg));
    }
    else
    {
        /* Memory allocation */
        AvmemAllocParams.PartitionHandle = AvmemPartitionHandle;
        AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AvmemAllocParams.NumberOfForbiddenRanges = 0;
        AvmemAllocParams.ForbiddenRangeArray_p = NULL;
        AvmemAllocParams.NumberOfForbiddenBorders = 3;
        AvmemAllocParams.ForbiddenBorderArray_p = &ForbidBorder[0];
        ForbidBorder[0] = (void *) (SDRAM_BASE_ADDRESS + 0x200000);
        ForbidBorder[1] = (void *) (SDRAM_BASE_ADDRESS + 0x400000);
        ForbidBorder[2] = (void *) (SDRAM_BASE_ADDRESS + 0x600000);
        if (STAVMEM_AllocBlock(&AvmemAllocParams, &(PictHandle)) != ST_NO_ERROR)
        {
            /* Error handling */
            sprintf(VID_Msg, "VID_Restore : error STAVMEM_AllocBlock, size %d\n", AvmemAllocParams.Size);
            STTBX_Print((VID_Msg));
        }
        else
        {
            if (STAVMEM_GetBlockAddress(PictHandle, &Address_p) != ST_NO_ERROR)
            {
                /* Error handling */
                STTBX_Print(("Error getting address in VID_Copy()"));
            }
            else
            {
                /* Store source picture in memory */
                PictParams.Data = Address_p;

                STTBX_Print(("VID_Restore : done\n"));
                CopiedPictureParams[NbOfCopiedPictures] = PictParams;
                CopiedPictureHandle[NbOfCopiedPictures] = PictHandle;
                NbOfCopiedPictures++;
                debugread( FilePict, PictParams.Data, PictParams.Size) ;

                RetErr =  FALSE;
            }
        }
    }

    /* Close the files */
    debugclose(FilePict);


    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_RestorePicture() */


/*-------------------------------------------------------------------------
 * Function : Vid_InteractiveIO
 *            Interactive test of STVID_SetIOWindows()
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Vid_InteractiveIO(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr =  TRUE ;
    BOOL SetIO = FALSE;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    S32 Delta=1;
    char WinType='i';
    char Hit;

    STTBX_Print(("Run STVID_SetIOWindows(ix,iy,iw,ih,ox,oy,ow,oh) interactivly\nPress x/y/w/h to set x/y/w/h ; +/- to increase/decrease ; i/o to select input/output\nPress Q or Enter to quit\n"));
    /* get current windows parameters */
    RetErr = STVID_GetIOWindows( ViewPortHndl0, &InputWinPositionX, &InputWinPositionY,
                                  &InputWinWidth, &InputWinHeight,
                                  &OutputWinPositionX, &OutputWinPositionY,
                                  &OutputWinWidth, &OutputWinHeight );
    Hit=0;
    while ( Hit!='q' && Hit!='Q' && Hit!='\r')
    {
        STTBX_Print(("Press a key (X,Y,W,H,I,O,-,+) or Q to Quit\n"));
        STTBX_InputChar(&Hit);
        /*STTBX_InputChar(STTBX_INPUT_DEVICE_NORMAL, &Hit);*/
        switch(Hit)
        {
            case 'x':
            case 'X':
                if (WinType=='i')
                    InputWinPositionX+=Delta;
                else
                    OutputWinPositionX+=Delta;
                SetIO= TRUE;
                break;
            case 'y':
            case 'Y':
                if (WinType=='i')
                    InputWinPositionY+=Delta;
                else
                    OutputWinPositionY+=Delta;
                SetIO= TRUE;
                break;
            case 'w':
            case 'W':
                if (WinType=='i')
                    InputWinWidth+=Delta;
                else
                    OutputWinWidth+=Delta;
                SetIO= TRUE;
                break;
            case 'h':
            case 'H':
                if (WinType=='i')
                    InputWinHeight+=Delta;
                else
                    OutputWinHeight+=Delta;
                SetIO= TRUE;
                break;
            case 'i':
            case 'I':
                WinType='i';
                sprintf(VID_Msg, "Now input window will change %d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case 'o':
            case 'O':
                WinType='o';
                sprintf(VID_Msg, "Now output window will change %d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case '-':
                Delta--;
                sprintf(VID_Msg, "Increase/decrease value=%d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            case '+':
                Delta++;
                sprintf(VID_Msg, "Increase/decrease value=%d\n",Delta);
                STTBX_Print((VID_Msg));
                break;
            default:
                break;
        } /* end switch */
        if ( SetIO == TRUE )
        {
            RetErr = STVID_SetIOWindows( ViewPortHndl0,
                                      InputWinPositionX, InputWinPositionY,
                                      InputWinWidth, InputWinHeight,
                                      OutputWinPositionX, OutputWinPositionY,
                                      OutputWinWidth, OutputWinHeight );
            sprintf( VID_Msg, "STVID_SetIOWindows(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", VidDecHndl0,
                     InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                     OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
            STTBX_Print((VID_Msg));
        }
        SetIO= FALSE;

    }
    return(FALSE);

} /* end of Vid_InteractiveIO() */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_InitCommand2
 *            Definition of the register commands
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VID_InitCommand2 (void)
{
    BOOL RetErr;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "VID_CLEARSTATUS", VID_ClearStatus, "Clear video decoder status");
    RetErr |= STTST_RegisterCommand( "VID_COPY",     VID_Copy,      "Copy last displayed picture into memory\n\t\t\"FREE\" Free the pictures in memory");
	RetErr |= STTST_RegisterCommand( "VID_DECODE",   VIDDecodeFromMemory, "Copy buffer into video bit buffer");
    RetErr |= STTST_RegisterCommand( "VID_INJECT",   VID_MemInject, "<loop> DMA Inject Video in memory (parallel task)");
	RetErr |= STTST_RegisterCommand( "VID_IO",       Vid_InteractiveIO,  "Interactive SetIoWindow");
    RetErr |= STTST_RegisterCommand( "VID_KILL",     VID_KillTask, "Kill Video Inject Task");
	RetErr |= STTST_RegisterCommand( "VID_LOAD",     VID_Load,      "Load a file in memory");
    RetErr |= STTST_RegisterCommand( "VID_RESTORE",  VID_RestorePicture, "Restore a picture param previously stored");
    RetErr |= STTST_RegisterCommand( "VID_STATUS",   VID_GetStatus, "Get video decoder status");
    RetErr |= STTST_RegisterCommand( "VID_STORE",    VID_StorePicture, "Store the picture param of a copied picture");

    if ( RetErr == TRUE )
    {
        STTBX_Print(( "VID_InitCommand2() \t: failed !\n" ));
    }
    else
    {
        STTBX_Print(( "VID_InitCommand2() \t: ok\n" ));
    }

    return( RetErr );

} /* end VID_InitCommand2 */


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL VID_RegisterCmd2()
{
    BOOL RetErr;

    RetErr = VID_InitCommand2(); /* stvid commands */

    return(RetErr ? FALSE : TRUE);

} /* end VID2_CmdStart */


/*#######################################################################*/

