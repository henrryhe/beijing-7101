/*****************************************************************************

File name: blit.c

Description: blit setup

COPYRIGHT (C) 2004 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include <stdio.h>
#include "blt_wrapper.h"
#include "blt_drawing.h"
#include "blt_engine.h"



/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define SECONDE_WAIT  0.5

/* Private Function prototypes -------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
char SnakeDemoTag[] = "SNAKE_DEMO_TAG";
STBLIT_Handle_t Handle;
STBLIT_BlitContext_t    Context;
STOS_Clock_t Timeout;
STEVT_Handle_t          EvtHandle;
semaphore_t* SnakeDemo_BlitCompletionTimeoutSemaphore;

 /*-----------------------------------------------------------------------------
 * Function : DemoBlitCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void DemoBlitCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    char* TaskTag = *((char**)EventData_p);

    if (strcmp(TaskTag, SnakeDemoTag) == 0)
    {
		STOS_SemaphoreSignal(SnakeDemo_BlitCompletionTimeoutSemaphore);
	}
}

/*-------------------------------------------------------------------------
 * Function : DemoBlit_WaitForBlitterComplite
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t DemoBlit_WaitForBlitterComplite()
{
    STOS_Clock_t time;

    time = STOS_time_plus(STOS_time_now(), TICKS_PER_MILLI_SECOND * 5);

    return STOS_SemaphoreWaitTimeOut(SnakeDemo_BlitCompletionTimeoutSemaphore,&time);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_Setup()
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_Setup(ST_Partition_t *SystemPartition_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                              ST_DeviceName_t EvtDeviceName, ST_DeviceName_t IntTMRDeviceName, STEVT_SubscriberID_t ExternSubscriberID)
{
    BOOL Err;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
	ST_DeviceName_t         DeviceName = "Blitter\0";

    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;

	if (SnakeDemoParameters.InitMode == 0)
    {
		/* ------------ Blit Device Init ------------ */
    	InitParams.CPUPartition_p                       = SystemPartition_p;
    	InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    	InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
    	InitParams.AVMEMPartition                       = AVMEMPartition;
    	InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    	InitParams.MaxHandles                           = 1;

    	InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
	#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_7109) || defined (ST_5188)|| defined (ST_5525)\
 	|| defined (ST_5107)
    	InitParams.SingleBlitNodeMaxNumber              = 500;
	#else
    	InitParams.SingleBlitNodeMaxNumber              = 30;
	#endif
    	InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    	InitParams.JobBlitNodeMaxNumber                 = 30;
    	InitParams.JobBlitBufferUserAllocated = FALSE;
    	InitParams.JobBlitMaxNumber       = 30;
    	InitParams.JobBufferUserAllocated               = FALSE;
    	InitParams.JobMaxNumber                         = 2;
    	InitParams.WorkBufferUserAllocated              = FALSE;
    	InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    	strcpy(InitParams.EventHandlerName,EvtDeviceName);


#if !defined(STBLIT_EMULATOR) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710) || defined (ST_7100) || defined (ST_5301)\
 || defined (ST_7109) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)
    	InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    	InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    	InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    	strcpy(InitParams.BlitInterruptEventName,IntTMRDeviceName);
#endif
#endif

#ifdef ST_OSLINUX
	    layer_init();
#endif

		/* Init DeviceName */
		strcpy(DeviceName, STBLIT_DEVICE_NAME);

		/* ------------ Blit Init ------------ */
		Err = STBLIT_Init(DeviceName,&InitParams);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Print (("Err Init : %d\n",Err));
    	    return (TRUE);
    	}

    	/* ------------ Open Event handler -------------- */
    	Err = STEVT_Open(EvtDeviceName, &EvtOpenParams, &EvtHandle);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Print (("Err Event Open : %d\n",Err));
    	    return (TRUE);
    	}

    	/* ------------ Subscribe to Blit Completed event ---------------- */
    	EvtSubscribeParams.NotifyCallback     = DemoBlitCompletedHandler;
    	EvtSubscribeParams.SubscriberData_p   = NULL;
    	Err = STEVT_SubscribeDeviceEvent(EvtHandle,DeviceName,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Print (("Err Subscribe Blit completion : %d\n",Err));
    	    return (TRUE);
    	}

    	/* ------------ Get Subscriber ID ------------ */
    	STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
	}
	else  /* (SnakeDemoParameters.InitMode == 1) */
	{
		/* Init DeviceName */
		strcpy(DeviceName, SnakeDemoParameters.DeviceName);

		SubscriberID = ExternSubscriberID;

	} /* (SnakeDemoParameters.InitMode == 0) */

	/* ------------ Blit Open ------------ */
    Err = STBLIT_Open(DeviceName,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
    	STTBX_Print (("Err Open : %d\n",Err));
    	return (TRUE);
    }


    /* ------------ Initialize global semaphores ------------ */
    SnakeDemo_BlitCompletionTimeoutSemaphore = STOS_SemaphoreCreateFifoTimeOut(NULL,0);


	/* ------------ Set context ------------ */
	Context.EventSubscriberID   = SubscriberID;
    Context.UserTag_p           = SnakeDemoTag;

    return (Err);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_Term()
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_Term(void)
{
    ST_ErrorCode_t Err;
    STBLIT_TermParams_t     TermParams;


    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Event Close : %d\n",Err));
        return (Err);
    }

	/* --------------- Close Blit Handle ----------*/
	Err = STBLIT_Close(Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Close : %d\n",Err));
        return (Err);
    }

	if (SnakeDemoParameters.InitMode == 0)
    {
		/* ------------ Blit Term ------------ */
    	TermParams.ForceTerminate = TRUE;
    	Err = STBLIT_Term(STBLIT_DEVICE_NAME,&TermParams);
    	if (Err != ST_NO_ERROR)
    	{
    	    STTBX_Print(("Err Term : %d\n",Err));
    	    return (Err);
    	}

#ifdef ST_OSLINUX
	    layer_term();
#endif

	}


    return (Err);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_FillRectangle
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_FillRectangle(STGXOBJ_Bitmap_t *DstBitmap,int x, int y, int w, int h, STGXOBJ_Color_t Color)
{
    ST_ErrorCode_t Err;
    STGXOBJ_Rectangle_t     Rectangle;
    /*STGXOBJ_Color_t         Color;*/
    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    /*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
    /*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
    /*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = SnakeDemoParameters.Alpha;
    Context.EnableClipRectangle     = TRUE;
    Context.ClipRectangle           = Rectangle;
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    /*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Set Color Type ------------ */
    /*--------------set Rectangle params----------*/
    Rectangle.PositionX = x ;
    Rectangle.PositionY = y ;
    Rectangle.Width=w;
    Rectangle.Height=h;

    /*Set ClipRectangle */
    Context.ClipRectangle           = Rectangle;

    /*-------------------------Blit---------------------------------------*/
    Err = STBLIT_FillRectangle(Handle,&(*DstBitmap),&Rectangle,&Color,&Context);

    if (WaitForBlitCompletetion == 1) /* Synchrone mode */
    {
        /*Waiting for notification*/
        if ( DemoBlit_WaitForBlitterComplite() != ST_NO_ERROR )
        {
            STTBX_Print(("Error------------------------->:%s\n", Err));
            return (Err);
        }
    }

    else                            /* Asynchrone mode */
    {
        while ( Err == STBLIT_ERROR_MAX_SINGLE_BLIT_NODE  )
        {
            Timeout = STOS_time_plus(STOS_time_now(), (U32)(TICKS_MILLI_SECOND * SECONDE_WAIT));
            STOS_TaskDelayUntil(Timeout);
            Err = STBLIT_FillRectangle(Handle,&(*DstBitmap),&Rectangle,&Color,&Context);
        }
    }

    return(Err);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_CopyRectangle
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_CopyRectangle(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int src_x,int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h)
{
    ST_ErrorCode_t Err;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;
    STGXOBJ_Palette_t       Palette;
    STOS_Clock_t            Timeout;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    /* Context.ColorKey              = dummy; */
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    /*    Context.MaskWord                = 0xFFFFFF00;*/
    Context.EnableMaskBitmap        = FALSE;
    /*    Context.MaskBitmap_p            = NULL;*/
    Context.EnableColorCorrection   = FALSE;
    /*    Context.ColorCorrectionTable_p  = NULL;*/
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = SnakeDemoParameters.Alpha;
    Context.EnableClipRectangle     = FALSE;
    /*Context.ClipRectangle           = Rectangle;*/
    Context.WriteInsideClipRectangle = TRUE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    /*    Context.JobHandle               = JobHandle[0];*/
    Context.NotifyBlitCompletion    = TRUE;

   /*--------------set Rectangle params----------*/
    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SrcBitmap;
    Src.Rectangle.PositionX  = src_x;
    Src.Rectangle.PositionY  = src_y;
    Src.Rectangle.Width      = src_w;
    Src.Rectangle.Height     = src_h;
    Src.Palette_p            = &Palette;

     /* ------------ Set destination ------------ */
    Dst.Bitmap_p             = DstBitmap;
    Dst.Rectangle.PositionX  = dst_x;
    Dst.Rectangle.PositionY  = dst_y;
    Dst.Rectangle.Width      = dst_w ;
    Dst.Rectangle.Height     = dst_h ;
    Dst.Palette_p            = NULL;

    Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );

    if (WaitForBlitCompletetion == 1) /* Synchrone mode */
    {
        /*Waiting for notification*/
        if ( DemoBlit_WaitForBlitterComplite() == FALSE )
        {
            STTBX_Print(("Error------------------------->:%s\n", Err));
            return (Err);
        }
    }
    else                      /* Asynchrone mode */
    {
        while ( Err == STBLIT_ERROR_MAX_SINGLE_BLIT_NODE  )
        {
            Timeout = STOS_time_plus(STOS_time_now(), (U32)(TICKS_MILLI_SECOND * SECONDE_WAIT));
            STOS_TaskDelayUntil(Timeout);
            Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        }
    }
   return (Err);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_ReseizeBitmap
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_ReseizeBitmap(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int dst_x,int dst_y,int dst_w,int dst_h)
{
    ST_ErrorCode_t Err;

    Err = DemoBlit_CopyRectangle(SrcBitmap,DstBitmap,0,0,SrcBitmap.Width,SrcBitmap.Height,dst_x,dst_y,dst_w,dst_h);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (Err);
    }
    return(Err);
}

 /*-----------------------------------------------------------------------------
 * Function : DemoBlit_ReseizeRectangle
 * Input    : None
 * Output   : None
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t DemoBlit_ReseizeRectangle(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int src_x,int src_y, int src_w,int src_h,int dst_x,int dst_y,int dst_w,int dst_h)
{
    ST_ErrorCode_t Err;

    Err = DemoBlit_CopyRectangle(SrcBitmap, DstBitmap, src_x, src_y, src_h, src_w, dst_x, dst_y, dst_w, dst_h);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (Err);
    }
    return(Err);
}
