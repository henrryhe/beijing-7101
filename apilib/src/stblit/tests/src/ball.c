/*****************************************************************************/
/*                           DEMO.C - Demo in GX1                             */
/*                                                                            */
/* Author : Marc CHAPPELLIER                                                  */
/* Ported to ST20TP3/7015 with STAPI by Xavier Degeneve                       */
/*                                                                            */
/******************************************************************************/

/* WARNING: This code is pre-alpha ! It is not well coded/documented.         */

#if !defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
/* Header includes */
/* --------------- */
#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef REMOVE /* Removed by XD */
#include "newtypes.h"
#include "st40gx1.h"
#endif /* REMOVE */
#include "stavmem.h"
#include "ball.h"
#include "config.h"
#include "clevt.h"
#ifndef STBLIT_EMULATOR
#include "clintmr.h"
#endif

#ifdef REMOVE /* Removed by XD */
#include "gam_hal_go.h"
#include "gam_hal_util.h"
#endif /* REMOVE */

/*  #define TRACE_UART */
#ifdef TRACE_UART
#include "trace.h"
#define BLIT_Trace(a,b) trace((a),(b))
#define BLIT_Trace3(a,b,c) trace((a),(b),(c))
#else
#define BLIT_Trace(a,b)
#define BLIT_Trace3(a,b,c)
#endif

#define BLIT_LOOP_STRESS_ON FALSE
#define BLIT_LOOP_STRESS 5

#define   SECOND 50
/*  #define   SECOND 90 */
/*  #define SECOND (ST_GetClockSpeed()*10) */ /* XD */


/* Definition of vector ball type */
/* ------------------------------ */
typedef struct
{
 U32 ball;
 S32 x;
 S32 y;
 S32 z;
} ball_object;

/* Object in vector ball */
/* --------------------- */
#include "ball_objects.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ST_OS21
    #include "os21debug.h"
    #include <sys/stat.h>
    #include <fcntl.h>
#endif /*End of ST_OS21  */

#ifdef ST_OS20
	#include <debug.h>
#endif /*End of ST_OS20*/

#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"

#include "stcommon.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "testtool.h"
#include "api_test.h"
#include "startup.h"
#include "stblit.h"
#include "stsys.h"

#if defined (ST_7015)
#define BLITTER_INT_EVENT ST7015_BLITTER_INT
#elif defined (ST_7020)
#define BLITTER_INT_EVENT ST7020_BLITTER_INT
#elif defined (ST_GX1)
#define BLITTER_INT_EVENT 0
#elif defined (ST_5528)
#define BLITTER_INT_EVENT ST5528_BLITTER_INTERRUPT
#elif defined (ST_7710)
#define BLITTER_INT_EVENT ST7710_BLT_REG_INTERRUPT
#elif defined (ST_7100)
#define BLITTER_INT_EVENT  ST7100_BLITTER_INTERRUPT
#endif

/* Global variables */
/* ---------------- */
static BOOL BALL_Running = FALSE;
static BOOL BALL_First_Run = TRUE;
/* BALL_Demo task Id used to wait its end                           */
static task_t*  Task_p = (task_t*)NULL;

#ifdef ST_OSLINUX
    static pthread_t            Task;
    static pthread_attr_t       BALL_TaskAttribute;
#endif
#ifdef ST_OS21
	static task_t*   Task;
#endif
#ifdef ST_OS20
	static task_t   Task;
	static tdesc_t  TaskDesc;
#endif

static int      StackTask[50 * 1024/ sizeof(int)];


static STBLIT_BlitContext_t   BlitContext_Fill;
static STGXOBJ_Color_t        Color;
static STGXOBJ_Rectangle_t    DisplayRect;

static STBLIT_Source_t        BlitSource;
static STBLIT_Destination_t   BlitDestination;
static STBLIT_BlitContext_t   BlitContext_Copy;

static STBLIT_Handle_t BlitHdl;

static STGXOBJ_Bitmap_t BallsBitmap;
static STGXOBJ_Bitmap_t *BallsBitmap_p;

static STGXOBJ_Bitmap_t *DisplayBitmap_p;  /* Current displayed bitmap */
static STGXOBJ_Bitmap_t *DrawBitmap_p;     /* Current drawn bitmap     */
static STGXOBJ_Bitmap_t DisplayBitmap_tmp1;
static STGXOBJ_Bitmap_t DisplayBitmap_tmp2;
static STGXOBJ_Bitmap_t DisplayBitmap_tmp3;
static STGXOBJ_Bitmap_t *DisplayBitmap_tmp1_p;
static STGXOBJ_Bitmap_t *DisplayBitmap_tmp2_p;
static STGXOBJ_Bitmap_t *DisplayBitmap_tmp3_p;
static STEVT_SubscriberID_t            SubscriberID;



static BOOL                   no_first_time;
static ball_object            balls[100];
static S32                    ball_vx[100],ball_vy[100],ball_vz[100]; /* Vector move from previous obj */
static S32                    ball_dx[100],ball_dy[100],ball_dz[100]; /* Diff between curent and target */
static BOOL                   event_vector_ball_enabled;
static BOOL                   event_vector_ball_disabled;

static S32 cosinus[512];
static S32 sinus[512];

static S32  lAngleX,lAngleY,lAngleZ,lAngleXInc,lAngleYYInc,lAngleZInc,lAngleYInc,lAngleYY;
static U32  i;
static U16  xpos,ypos,hsize,vsize;
static U32  demo_time = 0;
static S32  lGainX,lGainY,lGainXInc,lGainYInc;
static S32  x,y,z;

static BOOL BALL_SD_activated = FALSE;

extern U8                           BlitterRegister[];
extern STAVMEM_PartitionHandle_t    AvmemPartitionHandle[];
extern ST_Partition_t*              SystemPartition_p;

static semaphore_t* BallsBlitCompletionTimeout;
static semaphore_t* BallsJobCompletionTimeout;

#ifdef ST_OSLINUX
extern MapParams_t   BlitterMap;
#define LINUX_BLITTER_BASE_ADDRESS    BlitterMap.MappedBaseAddress
#endif


#ifndef STBLIT_EMULATOR
static void ResetEngine(void)
{
#if defined(ST_GX1)

#else
    void *Address;
/*    STSYS_WriteRegDev32LE((void*)(0x6000AA00), 1);*/
/*    STSYS_WriteRegDev32LE((void*)(0x6000AA00), 0);*/
#ifdef ST_OSLINUX
    Address = LINUX_BLITTER_BASE_ADDRESS;
#else
    Address = CURRENT_BASE_ADDRESS;
#endif
    STSYS_WriteRegDev32LE(Address, 1);
    STSYS_WriteRegDev32LE(Address, 0);

#endif
}
#endif

extern ST_ErrorCode_t ConvertGammaToBitmap ( char*,U8 , STGXOBJ_Bitmap_t*, STGXOBJ_Palette_t* );
extern ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p , void* SharedMemoryBaseAddress_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                        STAVMEM_BlockHandle_t*  BlockHandle1_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p, STAVMEM_BlockHandle_t*  BlockHandle2_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p);

/*-----------------------------------------------------------------------------
 * Function : BallsBlitCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BallsBlitCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    STOS_SemaphoreSignal(BallsBlitCompletionTimeout);
}

/*-----------------------------------------------------------------------------
 * Function : BallsJobCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BallsJobCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    STOS_SemaphoreSignal(BallsJobCompletionTimeout);
}





/******************************************************************************/
/*                                  DEMO_INIT                                 */
/*                                                                            */
/* This function is the main entry point of the demo.                         */
/*                                                                            */
/* INPUT   : None                                                             */
/* OUTPUT  : None                                                             */
/* RETURN  : None                                                             */
/*                                                                            */
/******************************************************************************/

static BOOL demo_init()
{
    ST_ErrorCode_t                  ErrCode = ST_NO_ERROR;
    ST_DeviceName_t                 Name="Blitter\0";
    STBLIT_InitParams_t             InitParams;
    STBLIT_OpenParams_t             BlitOpenP;
    STGXOBJ_Palette_t               Palette;
    STAVMEM_BlockHandle_t           BlockHandle1, BlockHandle2, BlockHandle3;
    STAVMEM_BlockHandle_t           BlockHandle4, BlockHandle5, BlockHandle6;
    STGXOBJ_BitmapAllocParams_t     Params1;
    STGXOBJ_BitmapAllocParams_t     Params2;
    STEVT_OpenParams_t              EvtOpenParams;
    STEVT_Handle_t                  EvtHandle;
    STEVT_DeviceSubscribeParams_t   EvtSubscribeParams;


    memset((void *)&BlitOpenP, 0, sizeof(STBLIT_OpenParams_t));

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                 = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p              = CURRENT_BASE_ADDRESS;
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;
    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 400;
    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 400;
    InitParams.JobBlitBufferUserAllocated           = FALSE;
    InitParams.JobBlitMaxNumber                     = 400;
    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 400;
    InitParams.WorkBufferUserAllocated              = FALSE;
    InitParams.WorkBufferSize                       = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);
#ifndef STBLIT_EMULATOR
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    InitParams.BlitInterruptLevel   = 0 ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT ;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif

    ErrCode = STBLIT_Init(Name,&InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err Init : %d\n",ErrCode));
        return (TRUE);
    }

    /* Open STBLIT                                                             */
    if (ErrCode == ST_NO_ERROR)
    {
        /* BlitOpenP is dummy                                                  */
        ErrCode = STBLIT_Open(Name, &BlitOpenP, &(BlitHdl));
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_Open()=%d\n",ErrCode));
            return(TRUE);
        }
    }

    /* Initialize global semaphores */
    BallsBlitCompletionTimeout = STOS_SemaphoreCreateFifoTimeOut(NULL, 0 );
    BallsJobCompletionTimeout = STOS_SemaphoreCreateFifoTimeOut(NULL, 0 );

    /* Open Event handler */
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",ErrCode));
        return(TRUE);
    }

    /* Subscribe to Blit Completed event */
    EvtSubscribeParams.NotifyCallback     = BallsBlitCompletedHandler;
/*    EvtSubscribeParams.RegisterCallback   = NULL;
    EvtSubscribeParams.UnregisterCallback = NULL;*/
    EvtSubscribeParams.SubscriberData_p   = NULL;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle,Name,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Blit completion : %d\n",ErrCode));
        return(TRUE);
    }
    EvtSubscribeParams.NotifyCallback   = BallsJobCompletedHandler;
    ErrCode = STEVT_SubscribeDeviceEvent(EvtHandle, Name,STBLIT_JOB_COMPLETED_EVT,&EvtSubscribeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Subscribe Job completion : %d",ErrCode));
        return (TRUE);
    }

    /* Get Subscriber ID */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);

    /* Load balls */
    BallsBitmap.Data1_p = NULL;
    ErrCode = ConvertGammaToBitmap("../../balls.gam",0,&BallsBitmap,&Palette);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Err Convert Gamma to Bitmap : %d\n",ErrCode));
        return(TRUE);
    }
    BallsBitmap_p = &BallsBitmap;


    /* Set DisplayBitmap_tmp1,2 and 3 */
    DisplayBitmap_tmp1.ColorType              = STGXOBJ_COLOR_TYPE_ARGB1555;
    DisplayBitmap_tmp1.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    DisplayBitmap_tmp1.PreMultipliedColor     = FALSE;
    DisplayBitmap_tmp1.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    DisplayBitmap_tmp1.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    DisplayBitmap_tmp1.Width                  = 800 ;
    DisplayBitmap_tmp1.Height                 = 600;

    DisplayBitmap_tmp2      = DisplayBitmap_tmp1;
    DisplayBitmap_tmp3      = DisplayBitmap_tmp1;


    ErrCode = STBLIT_GetBitmapAllocParams(BlitHdl,&DisplayBitmap_tmp1,&Params1,&Params2);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",ErrCode));
        return(TRUE);
    }

    /* Alloc Bitmaps */
    AllocBitmapBuffer (&DisplayBitmap_tmp1,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
    AllocBitmapBuffer (&DisplayBitmap_tmp2,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle3,&Params1, &BlockHandle4,&Params2);
    AllocBitmapBuffer (&DisplayBitmap_tmp3,(void*)TEST_SHARED_MEM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle5,&Params1, &BlockHandle6,&Params2);

    /* Copy Pointer to fast access (swap)                    */
    DisplayBitmap_tmp1_p = &DisplayBitmap_tmp1;
    DisplayBitmap_tmp2_p = &DisplayBitmap_tmp2;
    DisplayBitmap_tmp3_p = &DisplayBitmap_tmp3;


    /* Create sin/cos tables */
    /* --------------------- */
    for (x=0;x<512;x++)
    {
        cosinus[x] = (S32)(256*cos((double)x * 3.14159265 / 256));
        sinus[x]   = (S32)(256*sin((double)x * 3.14159265 / 256));
    }

    /* Remove filter, consider that blitter do it correctly */

    /* Prepare blitcontext */
    if (ErrCode == ST_NO_ERROR)
    {
        /* Config the context for blitting operation                 */
        memset((void *)&BlitContext_Fill,0,sizeof(STBLIT_BlitContext_t));

        BlitContext_Fill.Trigger.EnableTrigger    = FALSE;
        BlitContext_Fill.JobHandle                = STBLIT_NO_JOB_HANDLE;
        BlitContext_Fill.WriteInsideClipRectangle = TRUE;
        BlitContext_Fill.EnableClipRectangle      = FALSE;
        BlitContext_Fill.NotifyBlitCompletion     = TRUE;
        BlitContext_Fill.AluMode                  = STBLIT_ALU_COPY;
        BlitContext_Fill.EnableMaskWord           = FALSE;
        BlitContext_Fill.MaskWord                 = 0xFFFFFFFF;
        BlitContext_Fill.GlobalAlpha              = 128;
        BlitContext_Fill.EventSubscriberID        = SubscriberID;

        memset((void *)&BlitContext_Copy,0,sizeof(STBLIT_BlitContext_t));

        BlitContext_Copy.ColorKeyCopyMode         = STBLIT_COLOR_KEY_MODE_SRC; /* Or SRC/DST ??? */
        BlitContext_Copy.ColorKey.Type            = STGXOBJ_COLOR_KEY_TYPE_RGB888;
        BlitContext_Copy.ColorKey.Value.RGB888.RMin    = 0;
        BlitContext_Copy.ColorKey.Value.RGB888.RMax    = 0;
        BlitContext_Copy.ColorKey.Value.RGB888.ROut    = FALSE;
        BlitContext_Copy.ColorKey.Value.RGB888.REnable = TRUE;
        BlitContext_Copy.ColorKey.Value.RGB888.GMin    = 0xFC;
        BlitContext_Copy.ColorKey.Value.RGB888.GMax    = 0xFC;
        BlitContext_Copy.ColorKey.Value.RGB888.GOut    = FALSE;
        BlitContext_Copy.ColorKey.Value.RGB888.GEnable = TRUE;
        BlitContext_Copy.ColorKey.Value.RGB888.BMin    = 0;
        BlitContext_Copy.ColorKey.Value.RGB888.BMax    = 0;
        BlitContext_Copy.ColorKey.Value.RGB888.BOut    = FALSE;
        BlitContext_Copy.ColorKey.Value.RGB888.BEnable = TRUE;

        BlitContext_Copy.Trigger.EnableTrigger    = FALSE;
        BlitContext_Copy.JobHandle                = STBLIT_NO_JOB_HANDLE;
        BlitContext_Copy.WriteInsideClipRectangle = TRUE;
        BlitContext_Copy.EnableClipRectangle      = FALSE;
        BlitContext_Copy.NotifyBlitCompletion     = FALSE; /* Only Job based */
        BlitContext_Copy.AluMode                  = STBLIT_ALU_ALPHA_BLEND;
        BlitContext_Copy.EnableMaskWord           = FALSE;
        BlitContext_Copy.MaskWord                 = 0xFFFFFFFF;
        BlitContext_Copy.GlobalAlpha              = 128;
        BlitContext_Copy.EventSubscriberID        = SubscriberID;

        /* Color used for filling the Layer bitmap                   */
        Color.Type                 = STGXOBJ_COLOR_TYPE_ARGB1555;
        Color.Value.ARGB1555.Alpha = 0;
        Color.Value.ARGB1555.R     = 0;
        Color.Value.ARGB1555.G     = 0;
        Color.Value.ARGB1555.B     = 40;

        DisplayRect.PositionX = 0;
        DisplayRect.PositionY = 0;
        DisplayRect.Width     = DisplayBitmap_tmp1_p->Width;
        DisplayRect.Height    = DisplayBitmap_tmp1_p->Height;

        /* Blit source and destination structure                     */
        BlitSource.Type          = STBLIT_SOURCE_TYPE_BITMAP;
        BlitSource.Data.Bitmap_p = BallsBitmap_p;
        BlitSource.Rectangle.PositionX = 0;
        BlitSource.Rectangle.PositionY = 0;
        BlitSource.Rectangle.Width     = 0; /* To update */
        BlitSource.Rectangle.Height    = 0; /* To update */
        BlitSource.Palette_p     = NULL;

        BlitDestination.Bitmap_p  = NULL; /* Will be set later */
        BlitDestination.Rectangle = DisplayRect;
        BlitDestination.Palette_p = NULL;

    }

    /* Return no errors */
    /* ---------------- */
    return(TRUE);
} /* demo_init () */

/******************************************************************************/
/*                                  DEMO_START                                */
/*                                                                            */
/* This function is used to start the demo.                                   */
/*                                                                            */
/* INPUT   : None                                                             */
/* OUTPUT  : None                                                             */
/* RETURN  : None                                                             */
/*                                                                            */
/******************************************************************************/

static void demo_start()
{
    U32 i;

    /* Set all dynamic variables */
    /* ------------------------- */
    lAngleX  = 30;
    lAngleY  = 40;
    lAngleZ  = 0;
    lAngleYY  = 0;
    lGainX   = 60;
    lGainY   = 0;
    lGainXInc  = 0;
    lGainYInc  = 0;
    lAngleYYInc  = 5;
    lAngleXInc = 0;
    lAngleYInc = 3;
    lAngleZInc  = 0;

    /* Initialize vector ball parameters */
    /* --------------------------------- */
    for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
    {
        ball_vx[i] = 0;
        ball_vy[i] = 0;
        ball_vz[i] = 0;
    }
    ball_obj_current = 0;
    ball_morph_start = 0;
    ball_morph_stop  = 0;
    ball_time_zero   = 0;

    /* Set all booleans */
    /* ---------------- */
    no_first_time           = FALSE;
    event_vector_ball_enabled      = FALSE;
    event_vector_ball_disabled      = FALSE;

} /* demo_start () */


static int tri_z(const void *ball_1,const void *ball_2)
{
    ball_object *ball_a = (ball_object *)ball_1;
    ball_object *ball_b = (ball_object *)ball_2;
    if (ball_a->z == ball_b->z) return(0);
    if (ball_a->z > ball_b->z)  return(-1);
    return(1);
}

/* Use to config */
static U32 dFactor = 1;
static U32 xFactor = 0;
static U32 yFactor = 0;
static U32 hFactorM = 3;
static U32 hFactorD = 2;
static U32 vFactorM = 3;
static U32 vFactorD = 2;
static U32 hFactorZoom = 2000;
static U32 vFactorZoom = 2000;

/*=====================================================================
  Name:        manage_vector_balls
  Description: All computation fro rotating balls.
  Arguments:   No.
  Returns  :   Error Code.
==================================================================== */
/*  void manage_vector_balls(U8 field) */
static ST_ErrorCode_t manage_vector_balls( )
{
    ST_ErrorCode_t       ErrCode;
    U32                  local_time;
    STBLIT_JobParams_t   BlitJobParams;
    STBLIT_JobHandle_t   JobHdl;
#ifndef STBLIT_EMULATOR
    BOOL                    TimeOut = FALSE;
    STOS_Clock_t            time;
#endif

    ErrCode = ST_NO_ERROR;

#define JOB TRUE
#define BLIT_JOBNOTIFY TRUE

    /* Clear  plane */
    /* ============ */
    /* Not into the Job */
    /*  BLIT_Trace ( "BALL : %s\r\n" , "FillRectangle" );*/
    ErrCode = STBLIT_FillRectangle(BlitHdl, DrawBitmap_p,
                                   &DisplayRect, &Color, &BlitContext_Fill);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_FillRectangle()=%d\n",ErrCode));
        return(TRUE);
    }
    else
    {
        #ifndef STBLIT_EMULATOR
            /*  Wait for Blit to be completed */
            time = STOS_time_plus(STOS_time_now(), 15625*5);
            if(STOS_SemaphoreWaitTimeOut(BallsBlitCompletionTimeout,&time)!=0)
                    {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT FILL"));
                    ResetEngine();
                    TimeOut = TRUE;
                    }
            else
                {
                    /* BLIT_Trace ( "BALL : %s\r\n" , "FillRectangle OK" );*/
                }
        #else
            /*  Wait for Blit to be completed */
            STOS_SemaphoreWaitTimeOut(BallsBlitCompletionTimeout, TIMEOUT_INFINITY);
            /*        BLIT_Trace ( "BALL : %s\r\n" , "FillRectangle OK" );*/
        #endif

    }


    /* If this is the first time, set time zero */
    /* ---------------------------------------- */
    if (ball_time_zero == 0) ball_time_zero = demo_time;
    local_time = demo_time-ball_time_zero;

    /* Morphing on vector balls */
    /* ------------------------ */
    if (local_time == ball_events[ball_obj_current].time)
    {
        /* Its time to change current object */
        ball_obj_current++;
        if (ball_obj_current == (sizeof(ball_events)/sizeof(tball_evts))) ball_obj_current=0;
        ball_morph_start = local_time;
        ball_morph_stop  = local_time+5*SECOND;
        for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
        {
            ball_dx[i] = ball_events[ball_obj_current].objet[i].x - ball_current[i].x;
            ball_dy[i] = ball_events[ball_obj_current].objet[i].y - ball_current[i].y;
            ball_dz[i] = ball_events[ball_obj_current].objet[i].z - ball_current[i].z;
        }
    }
    if ((local_time>ball_morph_start) && (local_time<ball_morph_stop))
    {
        /* Morphing zone */
        for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
        {
            ball_vx[i] += ball_dx[i];
            ball_vy[i] += ball_dy[i];
            ball_vz[i] += ball_dz[i];
        }
    }
    if (local_time == ball_morph_stop)
    {
        /* Update current balls positions after morphing */
        for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
        {
            ball_current[i].x += ball_vx[i]/(5*SECOND);
            ball_current[i].y += ball_vy[i]/(5*SECOND);
            ball_current[i].z += ball_vz[i]/(5*SECOND);
            ball_vx[i] = ball_vy[i] = ball_vz[i] = 0; /* Prepare next morph */
        }
    }

    /* Move vector balls */
    /* ----------------- */
    for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
    {
        x=dFactor*(ball_current[i].x + (ball_vx[i]/(5*SECOND)));
        y=dFactor*(ball_current[i].y + (ball_vy[i]/(5*SECOND)));
        y += (lGainY*sinus[(lAngleYY+((i*10)/9)+i*60)&0x1FF])/256;
        z=dFactor*(ball_current[i].z + (ball_vz[i]/(5*SECOND)));
        balls[i].ball = ball_current[i].ball;

        /* Rotation autour de l'axe X */
        balls[i].y=(y*cosinus[lAngleX&0x1FF]-z*sinus[lAngleX&0x1FF])/256; /* Sinus table size is 0x200=512 */
        balls[i].z=(y*sinus[lAngleX&0x1FF]+z*cosinus[lAngleX&0x1FF])/256;
        y=balls[i].y;
        z=balls[i].z;
        /* Rotation autour de l'axe Y */
        balls[i].x=(x*cosinus[lAngleY&0x1FF]-z*sinus[lAngleY&0x1FF])/256;
        balls[i].z=(x*sinus[lAngleY&0x1FF]+z*cosinus[lAngleY&0x1FF])/256;
        x=balls[i].x;
        /* Rotation autour de l'axe Z */
        balls[i].x=(x*cosinus[lAngleZ&0x1FF]-y*sinus[lAngleZ&0x1FF])/256;
        balls[i].y=(x*sinus[lAngleZ&0x1FF]+y*cosinus[lAngleZ&0x1FF])/256;
    }

    /* Change dynamic parameters */
    /* ------------------------- */
    if (local_time>(3*SECOND))
    {
        if ((local_time&0x1FF) == 0)  lGainY += 2;
        if (lGainY >= 30) lGainY = 30;
    }
    if (local_time>5*SECOND)
    {
        if ((local_time&0x1FF) == 0)
        {
            lAngleXInc  = rand()&3;
            lAngleYInc  = (rand()&6)+1;
            lAngleZInc  = rand()&1;
            lAngleYYInc = rand()&10;
        }
    }

    lAngleX += lAngleXInc;
    lAngleY += lAngleYInc;
    lAngleZ += lAngleZInc;
    lAngleYY+= lAngleYYInc;

    /* Sort the balls */
    /* -------------- */
    qsort((void *)balls,sizeof(ball_current)/sizeof(ball_object),sizeof(ball_object),tri_z);

    /* Blit, update to blit into the good bitmap */
    BlitDestination.Bitmap_p  = DrawBitmap_p;

    /* Create a Job                                                            */
    if (ErrCode == ST_NO_ERROR)
    {
        memset((void *)&BlitJobParams, 0, sizeof(STBLIT_JobParams_t));

        BlitJobParams.NotifyJobCompletion = TRUE;
        BlitJobParams.EventSubscriberID   = SubscriberID;

/*        BLIT_Trace ( "BALL : %s\r\n" , "CreateJob" );*/

        ErrCode = STBLIT_CreateJob(BlitHdl, &BlitJobParams, &JobHdl);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_CreateJob()=%d\n",ErrCode));
        }
/*        BLIT_Trace ( "BALL : %s\r\n" , "CreateJob OK" );*/
    }

    /* Update Job handle for further blit                                      */
    BlitContext_Copy.JobHandle = JobHdl;


    /* Update vector balls */
    /* ------------------- */
    for (i=0;i<sizeof(ball_current)/sizeof(ball_object);i++)
    {
        S32 tmp;
        tmp  = ((720/2) + (256*balls[i].x)/(256+balls[i].z)+xFactor);
        xpos = (tmp < 0 ? 0 : (U16)tmp);
        tmp  = ((400/2+50) + (256*balls[i].y)/(256+balls[i].z)+yFactor);
        ypos = (tmp < 0 ? 0 : (U16)tmp);
        tmp   = (50*100+hFactorZoom)/(256+balls[i].z);
        hsize = (tmp < 0 ? 0 : (U16)tmp);
        tmp   = (50*100+vFactorZoom)/(256+balls[i].z);
        vsize = (tmp < 0 ? 0 : (U16)tmp);


        if (event_vector_ball_enabled == TRUE)
        {
            BlitSource.Rectangle.PositionX = BallsBitmap_p->Height*balls[i].ball;
            BlitSource.Rectangle.PositionY = 0;
            /* BitmapSource has all balls stored horizontally, copy just one */
            BlitSource.Rectangle.Width     = BallsBitmap_p->Height;
            BlitSource.Rectangle.Height    = BallsBitmap_p->Height;
            BlitDestination.Rectangle.PositionX = xpos;
            BlitDestination.Rectangle.PositionY = ypos;
            BlitDestination.Rectangle.Width     = hsize/hFactorD*hFactorM;
            BlitDestination.Rectangle.Height    = vsize/vFactorD*vFactorM;

/*            BLIT_Trace3 ( "BALL : %s Ball %d\r\n" , "BL0IT", i );*/
            ErrCode = STBLIT_Blit(BlitHdl, &BlitSource, NULL,
                        &BlitDestination, &BlitContext_Copy);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_Bit(%d)=%d\n",i,ErrCode));
            }
/*            BLIT_Trace ( "BALL : %s\r\n" , "Blit OK" );*/
/*            if (i== 10)*/
/*            {*/
/*                break;*/
/*            }*/

        }

    }

    /* Submit Job                                                          */
    /* Even if there is an error, BLIT what we can blit                    */
/*    BLIT_Trace ( "BALL : %s\r\n" , "SubmitJobBack" );*/

#if (BLIT_LOOP_STRESS_ON==TRUE)
    {
        U32 index;
        for(index=0;index<BLIT_LOOP_STRESS;index ++)
        {
#endif /* (BLIT_LOOP_STRESS_ON==TRUE) */

    ErrCode = STBLIT_SubmitJobBack(BlitHdl, JobHdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_SubmitJobBack()=%d",
                      ErrCode));
    }


#if (BLIT_LOOP_STRESS_ON==TRUE)
        }
    }

#endif /* (BLIT_LOOP_STRESS_ON==TRUE) */

/*    BLIT_Trace ( "BALL : %s\r\n" , "SubmitJobBack OK" );*/

/*    BLIT_Trace ( "BALL : %s\r\n" , "DeleteJob" );*/
    ErrCode =  STBLIT_DeleteJob(BlitHdl, JobHdl);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STBLIT_DeleteJob()=%d",
                      ErrCode));
    }
/*    BLIT_Trace ( "BALL : %s\r\n" , "SubmitJobBack OK" );*/

    return ErrCode;

} /* manage_vector_balls () */

/*=====================================================================
  Name:        BALL_Task
  Description: Demo of rotating BALLs
  Arguments:   Standard testtool parameters.
  Returns  :   Nothing, this is a task.
==================================================================== */
static void BALL_Task( void *Param_p )
{
/*    STLAYER_ViewPortSource_t VPSource;*/
    ST_ErrorCode_t           ErrCode;
    BOOL                     Toggle;
#ifndef STBLIT_EMULATOR
    BOOL                    TimeOut = FALSE;
    STOS_Clock_t            time;
#endif


    ErrCode = ST_NO_ERROR;
    Toggle  = FALSE;
/*    memset((void *)&VPSource, 0, sizeof(STLAYER_ViewPortSource_t));*/

/*    VPSource.SourceType    = STLAYER_GRAPHIC_BITMAP;*/
/*    VPSource.Data.BitMap_p = NULL;*/
/*    VPSource.Palette_p     = NULL;*/

    DisplayBitmap_p = DisplayBitmap_tmp1_p;
    DrawBitmap_p    = DisplayBitmap_tmp1_p;

    while(BALL_Running == TRUE)
    {
        /* Wait VSYNC */
        if (Toggle == FALSE)
        {
/*            VTG_WaitVsync(VTG_MAIN, STVTG_BOTTOM);*/
            Toggle = TRUE;
        }
        else
        {
/*            VTG_WaitVsync(VTG_MAIN, STVTG_TOP);*/
            Toggle = FALSE;
        }

        /* Draw the previous displayed */
        if ( DisplayBitmap_p == DisplayBitmap_tmp1_p )
        {
            DrawBitmap_p    = DisplayBitmap_tmp3_p; /* Draw 1 now */
            DisplayBitmap_p = DisplayBitmap_tmp2_p; /* Display 2 next VSYNC 1 current*/
        }
        else if ( DisplayBitmap_p == DisplayBitmap_tmp2_p )
        {
            DrawBitmap_p    = DisplayBitmap_tmp1_p; /* Draw 1 now */
            DisplayBitmap_p = DisplayBitmap_tmp3_p; /* Display 3 next VSYNC 2 current*/
        }
        else
        {
            DrawBitmap_p    = DisplayBitmap_tmp2_p; /* Draw 1 now */
            DisplayBitmap_p = DisplayBitmap_tmp1_p; /* Display 1 next VSYNC 3 current*/
        }

        /* Select next image to display                  */
/*        VPSource.Data.BitMap_p = DisplayBitmap_p;*/
        ErrCode = manage_vector_balls();
/*        STLAYER_SetViewPortSource (LAYER_GET_VP_HANDLE(LAYER_GFX1,0), &VPSource);*/
/*        if (BALL_SD_activated == TRUE)*/
/*        {*/
/*            STLAYER_SetViewPortSource (LAYER_GET_VP_HANDLE(LAYER_GFX2,0), &VPSource);*/
/*        }*/
        if (ErrCode == ST_NO_ERROR)
        {
            #ifndef STBLIT_EMULATOR
                /*  Wait for Blit to be completed */
                time = STOS_time_plus(STOS_time_now(), 15625*5);
                if(STOS_SemaphoreWaitTimeOut(BallsJobCompletionTimeout,&time)!=0)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT JOB"));
                    ResetEngine();
                    TimeOut = TRUE;
                }
            #else
                /*  Wait for Blit to be completed */
                STOS_SemaphoreWaitTimeOut(BallsJobCompletionTimeout, TIMEOUT_INFINITY);
            #endif

        }

        demo_time++;
    }

} /* BALL_Task() */

/*---------------------------------------------------------------------
  Name:        BALL_Start
  Description: Run a BALL function
  Arguments:
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL BALL_Start (void)
{
    BOOL RetErr;

    RetErr = FALSE;

    if (BALL_First_Run == TRUE)
    {
        demo_init();
        demo_start();
        event_vector_ball_enabled = TRUE;
        BALL_First_Run = FALSE;
    }

    BALL_Running = TRUE;

    /* Create task for the demo                                      */
    if  (RetErr == FALSE)
    {
/*        BALL_Tid = task_create( BALL_Task, 0L, 10240, STAPIDEMO7015_TASKPRIO_DEMO_BALL,*/
/*                                "STDM_BallDemo", 0 );*/

    #ifdef ST_OSLINUX
        pthread_attr_init(&BALL_TaskAttribute);
        if (pthread_create(&Task, &BALL_TaskAttribute, (void*) BALL_Task, (void*)NULL))
        {
            RetErr = TRUE;
        }
        else
        {
            printf("STBLIT - BALL : created layer task.\n");
        }
    #endif
    #ifdef ST_OS21
		task_create((void(*)(void*))BALL_Task,
            (void*)NULL,
            50 * 1024,
            1,
            "BallTask",0);
    #endif
    #ifdef ST_OS20
       /* Create thread */
        task_init ((void(*)(void*))BALL_Task,
            (void*)NULL,
            StackTask, 50 * 1024,
            &Task, &TaskDesc,
            1,
            "BallTask",0);

        Task_p= &Task;
    #endif

    }

    return RetErr;
} /* BALL_Start () */


/*---------------------------------------------------------------------
  Name:        BALL_AddSD
  Description: Add SD !
  Arguments:
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL BALL_AddSD (void)
{
    BOOL RetErr;

    RetErr = FALSE;

    if (BALL_Running == FALSE)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Need to start if before !"));
        return FALSE;
    }

    BALL_SD_activated = TRUE;

    return RetErr;
} /* BALL_AddSD () */

/*---------------------------------------------------------------------
  Name:        BALL_Stop
  Description: Stop BALL function
  Arguments:
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL BALL_Stop (void)
{
    BOOL RetErr;
    task_t *TaskList[1];

    RetErr = FALSE;

    BALL_Running = FALSE;

    /* Wait for task to finish before deleting                           */
    if (Task_p != NULL )
    {
        TaskList[0] = (task_t*)&Task;
        #ifdef ST_OSLINUX
            /* kill thread */
            if (pthread_cancel(Task))
            {
                printf("BLIT: unable to kill task\n");
                RetErr = TRUE;
            }
            else
            {
                pthread_attr_destroy(&BALL_TaskAttribute);
                printf("BLIT: killing intmr task.\n");
            }
        #else
            task_wait((task_t**)&TaskList,1,TIMEOUT_INFINITY);
            #if defined(ST_OS21) || defined(ST_OSWINCE)
                task_delete(Task);
            #endif
            #ifdef ST_OS20
                task_delete(&Task);
            #endif
        #endif
        Task_p= (task_t*)NULL;
    }

    STTBX_Print(("BALL stoped\n"));

    return RetErr;
} /* BALL_Stop () */


/*---------------------------------------------------------------------
  Name:        TTBALL_Start
  Description: Call BALL_Test subtask
  Arguments:   Standard testtool parameters.
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL TTBALL_Start ( parse_t *pars_p, char *result_sym_p )
{
    return BALL_Start();
} /* TTBALL_Start () */


/*---------------------------------------------------------------------
  Name:        TTBALL_Stop
  Description: Stop BALL
  Arguments:   Standard testtool parameters.
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL TTBALL_Stop ( parse_t *pars_p, char *result_sym_p )
{
    return BALL_Stop();
} /* TTBALL_Stop () */

/*---------------------------------------------------------------------
  Name:        TTBALL_AddSD
  Description: Call BALL_AddSD
  Arguments:   Standard testtool parameters.
  Returns  :   TRUE if fails, FALSE if terminates successfully.
-------------------------------------------------------------------- */
BOOL TTBALL_AddSD ( parse_t *pars_p, char *result_sym_p )
{
    return BALL_AddSD();
} /* TTBALL_AddSD () */

#endif


/* EOF */
