/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

File name   : vid_blit.c
Description : Definition of blit extra commands (utilities)
 *
Note        : All functions return TRUE if error for Testtool compatibility
 *
Date          Modification                                      Initials
----          ------------                                      --------
17 Feb 2005   Creation                                             MH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "testcfg.h"
#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */
#ifdef ST_OS21
#include "os21debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#endif /* ST_OS24 */
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "clevt.h"
#include "startup.h"
#include "stblit.h"
#include "clavmem.h"
#ifdef USE_CLKRV
#include "stclkrv.h"
#include "clclkrv.h"
#else
#include "stcommon.h" /* for ST_GetClocksPerSecond() */
#endif /* USE_CLKRV */
#include "vid_blit.h"
#include "vid_util.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Global variables ------------------------------------------------ */
static char VID_Msg[1024];                             /* text for trace */
static U32 TicksPerOneSecond;
/* --- Externals ------------------------------------------------------- */


extern STVID_PictureInfos_t  PictInfos;
STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;

#ifdef ST_OS21
    semaphore_t* BlitCompletionTimeout_p;
#endif
#ifdef ST_OS20
    semaphore_t BlitCompletionTimeout;
#endif

/*-------------------------------------------------------------------------
 * Function : AllocBitmapBuffer
 *            Allocates bitmap buffer
 * Input    : STGXOBJ_BitmapAllocParams
 * Parameter:
 *
 * Output   :
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t AllocBitmapBuffer (STGXOBJ_Bitmap_t* Bitmap_p , void* SharedMemoryBaseAddress_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                        STAVMEM_BlockHandle_t*  BlockHandle1_p,
                        STGXOBJ_BitmapAllocParams_t* Params1_p, STAVMEM_BlockHandle_t*  BlockHandle2_p,
                        STGXOBJ_BitmapAllocParams_t* Params2_p)
{

    ST_ErrorCode_t              Err;
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    STAVMEM_MemoryRange_t       RangeArea[2];
    U8                          NbForbiddenRange;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;

    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        /* Data1 related */
        NbForbiddenRange = 1;
        if (VirtualMapping.VirtualWindowOffset > 0)
        {
            RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
            RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                                (U32)(VirtualMapping.VirtualWindowOffset) - 1);
        }
        else /*  VirtualWindowOffset = 0 */
        {
            RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
            RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
        }

        if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
            VirtualMapping.VirtualSize)
        {
            RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                                (U32)(VirtualMapping.VirtualWindowOffset) +
                                                (U32)(VirtualMapping.VirtualWindowSize));
            RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                    (U32)(VirtualMapping.VirtualSize) - 1);

            NbForbiddenRange= 2;
        }
    }
    AllocBlockParams.PartitionHandle          = AVMEMPartition;
    AllocBlockParams.Size                     = Params1_p->AllocBlockParams.Size;
    AllocBlockParams.Alignment                = Params1_p->AllocBlockParams.Alignment;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,BlockHandle1_p);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }
    STAVMEM_GetBlockAddress(*BlockHandle1_p,(void**)&(Bitmap_p->Data1_p));

    Bitmap_p->Pitch =  Params1_p->Pitch;
    Bitmap_p->Size1  =  Params1_p->AllocBlockParams.Size;
    Bitmap_p->Offset = 0;
    Bitmap_p->BigNotLittle = 0;

    return(Err);
}  /* end of AllocBitmapBuffer */

#if defined (ST_5528)
/*-------------------------------------------------------------------------
 * Function : VID_ConvertVideoToGraphic
 *            convert bitmap : PictInfos.BitmapParams into graphic bitmap
 * Input    : PictInfos
 * Parameter:
 *
 * Output   :  RET_VAL1  = &TargetBitmap
 * Return   : FALSE if no error
 * ----------------------------------------------------------------------*/
static BOOL VID_ConvertVideoToGraphic (parse_t *pars_p, char *result_sym_p)
{

    ST_ErrorCode_t          Err;
    ST_DeviceName_t         Name="Blitter\0";

    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         BlitHandle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Palette_t       Palette;

    STAVMEM_BlockHandle_t  BlockHandle1;
    STAVMEM_BlockHandle_t  BlockHandle2;
    STGXOBJ_BitmapAllocParams_t Params1;
    STGXOBJ_BitmapAllocParams_t Params2;
    osclock_t                 time;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
#if defined (ST_5528)
    InitParams.DeviceType                           = STBLIT_DEVICE_TYPE_GAMMA_5528;
#endif
    InitParams.BaseAddress_p                        = (void*)(BLITTER_BASE_ADDRESS);
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
    InitParams.SharedMemoryBaseAddress_p            = (void*)SDRAM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
    InitParams.SingleBlitNodeMaxNumber              = 30;

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;

    InitParams.WorkBufferUserAllocated              = FALSE;
    InitParams.WorkBufferSize             = (40 * 16);
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

    InitParams.BlitInterruptLevel   = 0 ;
    InitParams.BlitInterruptNumber  = 0;

    Err = STBLIT_Init(Name,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(Name,&OpenParams,&BlitHandle);
    if (Err != ST_NO_ERROR)
    {
        TermParams.ForceTerminate = TRUE;
        STBLIT_Term(Name,&TermParams);
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Open : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Initialize global semaphores ------------ */
    #ifdef ST_OS21
        BlitCompletionTimeout_p=semaphore_create_fifo(0);
    #endif
    #ifdef ST_OS20
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
    #endif

    /* ------------ Set source ------------ */
    SourceBitmap.Data1_p = NULL;
    TargetBitmap.Data1_p = NULL;
    Palette.Data_p = NULL;
    SourceBitmap = PictInfos.BitmapParams;

    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = &Palette;

    /* ------------ Set Dst ------------ */
    TargetBitmap.ColorType              = STGXOBJ_COLOR_TYPE_RGB565;
    TargetBitmap.BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    TargetBitmap.PreMultipliedColor     = FALSE;
    TargetBitmap.ColorSpaceConversion   = STGXOBJ_ITU_R_BT709;
    TargetBitmap.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
    TargetBitmap.Width                  = SourceBitmap.Width;
    TargetBitmap.Height                 = SourceBitmap.Height;

    /* Alloc Destination */
    Err = STBLIT_GetBitmapAllocParams(BlitHandle,&TargetBitmap,&Params1,&Params2);
    if (Err != ST_NO_ERROR)
    {
        #ifdef ST_OS21
            semaphore_delete(BlitCompletionTimeout_p);
        #endif
        #ifdef ST_OS20
            semaphore_delete(&BlitCompletionTimeout);
        #endif
        STBLIT_Close(BlitHandle) ;
        TermParams.ForceTerminate = TRUE;
        STBLIT_Term(Name,&TermParams);
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Get Bitmap Alloc Params : %d\n",Err));
        return (TRUE);
    }
    Err = AllocBitmapBuffer (&TargetBitmap,(void*)SDRAM_BASE_ADDRESS, AvmemPartitionHandle[0],
                       &BlockHandle1,&Params1, &BlockHandle2,&Params2);
    if (Err != ST_NO_ERROR)
    {
        #ifdef ST_OS21
            semaphore_delete(BlitCompletionTimeout_p);
        #endif
        #ifdef ST_OS20
            semaphore_delete(&BlitCompletionTimeout);
        #endif
        STBLIT_Close(BlitHandle) ;
        TermParams.ForceTerminate = TRUE;
        STBLIT_Term(Name,&TermParams);
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Allocate Bitmap : %d\n",Err));
        return (TRUE);
    }

    Dst.Bitmap_p              = &TargetBitmap;
    Dst.Rectangle.PositionX   = 0;
    Dst.Rectangle.PositionY   = 0;
    Dst.Rectangle.Width       = TargetBitmap.Width;
    Dst.Rectangle.Height      = TargetBitmap.Height;
    Dst.Palette_p             = NULL;

    /* ------------ Set Context ------------ */
    Context.ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                 = STBLIT_ALU_COPY;
    Context.EnableMaskWord          = FALSE;
    Context.EnableMaskBitmap        = FALSE;
    Context.EnableColorCorrection   = FALSE;
    Context.Trigger.EnableTrigger   = FALSE;
    Context.GlobalAlpha             = 50;
    Context.EnableClipRectangle     = FALSE;
    Context.EnableFlickerFilter     = FALSE;
    Context.JobHandle               = STBLIT_NO_JOB_HANDLE;
    Context.NotifyBlitCompletion    = TRUE;

    /* ------------ Blit ------------ */

    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Start Conversion RGB565 -> ARGB8888\n"));
    Err = STBLIT_Blit(BlitHandle,NULL,&Src,&Dst,&Context );
    if (Err != ST_NO_ERROR)
    {
        #ifdef ST_OS21
            semaphore_delete(BlitCompletionTimeout_p);
        #endif
        #ifdef ST_OS20
            semaphore_delete(&BlitCompletionTimeout);
        #endif
        STBLIT_Close(BlitHandle) ;
        TermParams.ForceTerminate = TRUE;
        STBLIT_Term(Name,&TermParams);
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
        return (TRUE);
    }
#ifndef STBLIT_EMULATOR
    time = time_plus(time_now(), 15625*5);
    #ifdef ST_OS21
        if(semaphore_wait_timeout(BlitCompletionTimeout_p,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
		}
    #endif
    #ifdef ST_OS20
		if(semaphore_wait_timeout(&BlitCompletionTimeout,&time)!=0)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
		}
    #endif
#endif /* STBLIT_EMULATOR */


     /* Delete semaphore  */
#ifdef ST_OS21
    semaphore_delete(BlitCompletionTimeout_p);
#endif
#ifdef ST_OS20
    semaphore_delete(&BlitCompletionTimeout);
#endif

    /* ------------ Blit Term ------------ */
    Err = STBLIT_Close(BlitHandle) ;
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Close : %d\n",Err));
        return (TRUE);
    }
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(Name,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Term : %d\n",Err));
        return (TRUE);
    }
    sprintf(VID_Msg, "VID_ConvertVideoToGraphic: Bitmap 0x(%x) : converted to graphic at address 0x%0x.\r\n", (int)(&SourceBitmap), (int)(&TargetBitmap));
    STTBX_Print((VID_Msg));

    STTST_AssignInteger("RET_VAL1",(int)(&TargetBitmap), TRUE);
    STTST_AssignInteger("RET_VAL2",(int)(&SourceBitmap), TRUE);
    return(FALSE);

}/* end of VID_ConvertVideoToGraphic */

#endif /* #if defined (ST_5528) */

/*-------------------------------------------------------------------------
 * Function : VID_BlitCmd
 *            Definition of the register commands
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VID_BlitCmd (void)
{
    BOOL RetErr;

    TicksPerOneSecond = ST_GetClocksPerSecond();

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "VID_VidToGraphic",  VID_ConvertVideoToGraphic, "Convert video bitmap into graphic bitmap");
    if ( !RetErr )
    {
        sprintf( VID_Msg, "VID_BlitCmd() \t: ok\n");
    }
    else
    {
        sprintf( VID_Msg, "VID_BlitCmd() \t: failed !\n" );
    }
    STTST_Print(( VID_Msg ));

    return(!RetErr);

} /* end VID_RegisterBlit */


/* end of vid_blit.c */
