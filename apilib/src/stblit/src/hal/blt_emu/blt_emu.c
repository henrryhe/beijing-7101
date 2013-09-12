 /*******************************************************************************

 File name   : blt_emu.c

 Description : Gamma emulator source file

 COPYRIGHT (C) STMicroelectronics 2000.

 Date               Modification                                     Name
 ----               ------------                                     ----
 28 Nov 2000        Created                                           TM
 *******************************************************************************/

 /* Private preliminary definitions (internal use only) ---------------------- */


 /* Includes ----------------------------------------------------------------- */

 #include "stddefs.h"
 #include "blt_emu.h"
 #include "gam_emul_global.h"
 #include "gam_emul_blitter_core.h"
 #include "bltswcfg.h"
 #include "stsys.h"
 #include "blt_be.h"
 #include "sttbx.h"
 #include "stavmem.h"

 #include <stdio.h>
 #include "blt_com.h"
#if !defined(ST_OS21) && !defined(ST_OSWINCE)
	#include "ostime.h"
 #endif

 /* Private Types ------------------------------------------------------------ */


 /* Private Constants -------------------------------------------------------- */

 /* Private Variables (static)------------------------------------------------ */


 /* Global Variables --------------------------------------------------------- */


 /* Private Macros ----------------------------------------------------------- */


 /* Private Function prototypes ---------------------------------------------- */
 static ST_ErrorCode_t DirectCopy(gam_emul_BlitterParameters *Blt_p);
 static ST_ErrorCode_t DirectFill(gam_emul_BlitterParameters *Blt_p);

 /* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stblit_EmulatorInit
Description : Initialize gamma emulator related stuff
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_EmulatorInit(stblit_Device_t * Device_p)
{
    ST_ErrorCode_t   Err = ST_NO_ERROR;

    /* Semaphore related */

    Device_p->EmulatorStartSemaphore= STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->EmulatorStartISRSemaphore= STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->EmulatorStopISRSemaphore= STOS_SemaphoreCreateFifo(NULL,0);

    Device_p->EmulatorSuspendSemaphore= STOS_SemaphoreCreateFifoTimeOut(NULL,0);

    /* Emulator Task related */
    #if defined(ST_OS21) || defined(ST_OSWINCE)
		/* Interrupt Service routine (High Priority Process)*/
		Device_p->EmulatorISRTask = task_create ((void(*)(void*))stblit_EmulatorISRProcess,
				(void*)Device_p,
				STBLIT_EMULATOR_ISR_TASK_STACK_SIZE,
				255,
				"STBLIT_EmulatorISRTask",0);/*task_flags_high_priority_process =1 */
		Device_p->EmulatorTask= task_create ((void(*)(void*))stblit_EmulatorProcess,
				(void*)Device_p,
				STBLIT_EMULATOR_TASK_STACK_SIZE,
				STBLIT_EMULATOR_TASK_STACK_PRIORITY,
				"STBLIT_EmulatorHWTask",0);

    #endif
    #ifdef ST_OS20
		task_init ((void(*)(void*))stblit_EmulatorProcess,
				(void*)Device_p,
				Device_p->StackEmulatorTask, STBLIT_EMULATOR_TASK_STACK_SIZE,
				&(Device_p->EmulatorTask), &(Device_p->EmulatorTaskDesc),
				STBLIT_EMULATOR_TASK_STACK_PRIORITY,
				"STBLIT_EmulatorHWTask",0);

		/* Interrupt Service routine (High Priority Process)*/
		task_init ((void(*)(void*))stblit_EmulatorISRProcess,
				(void*)Device_p,
				Device_p->StackEmulatorISRTask, STBLIT_EMULATOR_ISR_TASK_STACK_SIZE,
				&(Device_p->EmulatorISRTask), &(Device_p->EmulatorISRTaskDesc),
				0,
				"STBLIT_EmulatorISRTask",task_flags_high_priority_process);
	#endif
    return(Err);
}

/*******************************************************************************
Name        : stblit_EmulatorTerm
Description : Initialize gamma emulator related stuff
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_EmulatorTerm(stblit_Device_t * Device_p)
{
    ST_ErrorCode_t   Err = ST_NO_ERROR;
    task_t* EmulatorTaskList[1];

    /* In order to reach the TaskTerminate  test */
    Device_p->TaskTerminate = TRUE;  /* Probably redondant */

    /* Delete ISR task */
    STOS_SemaphoreSignal(Device_p->EmulatorStartISRSemaphore); /* Make sure the thread not waiting */

#if defined(ST_OS21) || defined(ST_OSWINCE)
    EmulatorTaskList[0] = (task_t*)Device_p->EmulatorISRTask;
#endif
#ifdef ST_OS20
    EmulatorTaskList[0] = (task_t*)&Device_p->EmulatorISRTask;
#endif

#ifndef ST_GX1 /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
    task_wait((task_t**)&EmulatorTaskList,1,TIMEOUT_INFINITY);
#endif
    #if defined(ST_OS21) || defined(ST_OSWINCE)
		task_delete(Device_p->EmulatorISRTask);
    #endif
    #ifdef ST_OS20
		task_delete(&Device_p->EmulatorISRTask);
	#endif

    /* Delete main Emulator task */
    STOS_SemaphoreSignal(Device_p->EmulatorStartSemaphore);    /* Make sure the thread is not waiting */
    STOS_SemaphoreSignal(Device_p->EmulatorStopISRSemaphore);  /* Make sure the thread not waiting */

#if defined(ST_OS21) || defined(ST_OSWINCE)
    EmulatorTaskList[0] = (task_t*)Device_p->EmulatorTask;
#endif
#ifdef ST_OS20
    EmulatorTaskList[0] = (task_t*)&Device_p->EmulatorTask;
#endif
#ifndef ST_GX1 /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
    task_wait((task_t**)&EmulatorTaskList,1,TIMEOUT_INFINITY);
#endif
    #if defined(ST_OS21) || defined(ST_OSWINCE)
		task_delete(Device_p->EmulatorTask);
    #endif
    #ifdef ST_OS20
		task_delete(&Device_p->EmulatorTask);
	#endif


    return(Err);
}

/*******************************************************************************
Name        : FormatInputToOutput
Description : Emulate the output formatter
Parameters  : Take as input the internal h/w color representation (32 bit bus)
Assumptions :  All parameters checks done at upper level.
               When RGB expansion, MSB filled with 0 mode only!
Limitations :  Only implemented for 4:4:4 formats
Returns     :
*******************************************************************************/
static ST_ErrorCode_t FormatInputToOutput (U32* ValueIn_p, U32* ValueOut_p,gam_emul_BlitterParameters* Blt_p)
 {
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    U32                     Alpha;

    *ValueOut_p = 0;

     switch (Blt_p->T_format)
    {
        case gam_emul_TYPE_ARGB8888:
        case gam_emul_TYPE_RGB888:
        case gam_emul_TYPE_YCbCr888:
        case gam_emul_TYPE_CLUT8:
        case gam_emul_TYPE_BYTE :
            *ValueOut_p = *ValueIn_p;
            break;
        case gam_emul_TYPE_RGB565:
            *ValueOut_p = (U32)(((*ValueIn_p & 0xF8)     >> 3)  |
                                ((*ValueIn_p & 0xFC00)   >> 5)  |
                                ((*ValueIn_p & 0xF80000) >> 8));
            break;
        case gam_emul_TYPE_ARGB8565:
            *ValueOut_p = (U32)(((*ValueIn_p & 0xF8)       >> 3)  |
                                ((*ValueIn_p & 0xFC00)     >> 5)  |
                                ((*ValueIn_p & 0xF80000)   >> 8)  |
                                ((*ValueIn_p & 0xFF000000) >> 8));
            break;
        case gam_emul_TYPE_ARGB4444:
            if ((*ValueIn_p & 0xFF000000) == 0)
            {
                Alpha = 0;
            }
            else if (((*ValueIn_p & 0xFF000000) >> 24) == 128)
            {
                Alpha = 0xF;
            }
            else
            {
                Alpha = (U32)((*ValueIn_p & 0x78000000) >> 27);
            }
            *ValueOut_p = (U32)(((*ValueIn_p & 0xF0)       >> 4)  |
                                ((*ValueIn_p & 0xF000)     >> 8)  |
                                ((*ValueIn_p & 0xF00000)   >> 12)  |
                                 (Alpha << 12));
            break;
        case gam_emul_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((*ValueIn_p & 0xF8)       >> 3)  |
                                ((*ValueIn_p & 0xF800)     >> 6)  |
                                ((*ValueIn_p & 0xF80000)   >> 9)  |
                                ((*ValueIn_p & 0x80000000) >> 16));
            break;
        case gam_emul_TYPE_A1 :
            *ValueOut_p = (U32)((*ValueIn_p & 0x80000000) >> 31);
            break;
        case gam_emul_TYPE_A8 :
            *ValueOut_p = (U32)((*ValueIn_p & 0xFF000000) >> 24);
            break;
        case gam_emul_TYPE_ACLUT44:
            if ((*ValueIn_p & 0xFF000000) == 0)
            {
                Alpha = 0;
            }
            else if (((*ValueIn_p & 0xFF000000) >> 24) == 128)
            {
                Alpha = 0xF;
            }
            else
            {
                Alpha = (U32)((*ValueIn_p & 0x78000000) >> 27);
            }
            *ValueOut_p = (U32)((*ValueIn_p & 0xF)  |
                                (Alpha << 4));
            break;
        case gam_emul_TYPE_ACLUT88:
            *ValueOut_p = (U32)(( *ValueIn_p & 0xFF)  |
                                ((*ValueIn_p & 0xFF000000) >> 16));
            break;
        case gam_emul_TYPE_CLUT1:
            *ValueOut_p = *ValueIn_p & 0x1;
            break;
        case gam_emul_TYPE_CLUT2:
            *ValueOut_p = *ValueIn_p & 0x3;
            break;
        case gam_emul_TYPE_CLUT4:
            *ValueOut_p = *ValueIn_p & 0xF;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }

/*******************************************************************************
Name        : FormatOutputToInput
Description : Emulate the reverse of output formatter
Parameters  : Take as input the bitmap color representation and generate the internal h/w bus format
Assumptions :  All parameters checks done at upper level.
               When RGB expansion, MSB filled with 0 mode only!
Limitations :  Only implemented for 4:4:4 formats
Returns     :
*******************************************************************************/
static ST_ErrorCode_t FormatOutputToInput (U32* ValueIn_p, U32* ValueOut_p,gam_emul_BlitterParameters* Blt_p)
 {
    ST_ErrorCode_t          Err     = ST_NO_ERROR;
    U32                     Alpha;

    *ValueOut_p = 0;

     switch (Blt_p->T_format)
    {
        case gam_emul_TYPE_ARGB8888:
        case gam_emul_TYPE_RGB888:
        case gam_emul_TYPE_YCbCr888:
        case gam_emul_TYPE_CLUT8:
        case gam_emul_TYPE_BYTE :
            *ValueOut_p = *ValueIn_p;
            break;
        case gam_emul_TYPE_RGB565:
            *ValueOut_p = (U32)(((*ValueIn_p & 0x1F)     << 3)  |
                                ((*ValueIn_p & 0x7E0)    << 5)  |
                                ((*ValueIn_p & 0xF800)   << 8));
            break;
        case gam_emul_TYPE_ARGB8565:
            *ValueOut_p = (U32)(((*ValueIn_p & 0x1F)       << 3)  |
                                ((*ValueIn_p & 0x7E0)      << 5)  |
                                ((*ValueIn_p & 0xF800)     << 8)  |
                                ((*ValueIn_p & 0xFF000000) << 8));
            break;
        case gam_emul_TYPE_ARGB4444:
            if ((*ValueIn_p & 0xF000) == 0)
            {
                Alpha = 0;
            }
            else if ((*ValueIn_p & 0xF000) == 0xF)
            {
                Alpha = 128;
            }
            else
            {
                Alpha = ((*ValueIn_p & 0xF000) >> 9) | 0x4;
            }
            *ValueOut_p = (U32)(((*ValueIn_p & 0xF)        << 4)  |
                                ((*ValueIn_p & 0xF0)       << 8)  |
                                ((*ValueIn_p & 0xF00)      << 12)  |
                                 (Alpha << 12));
            break;
        case gam_emul_TYPE_ARGB1555:
            *ValueOut_p = (U32)(((*ValueIn_p & 0x1F)       << 3)  |
                               ((*ValueIn_p & 0x3E0)      << 6)  |
                               ((*ValueIn_p & 0x7C00)     << 9)  |
                               ((*ValueIn_p & 0x8000)     << 16));
            break;
        case gam_emul_TYPE_A1 :
            *ValueOut_p = (U32)((*ValueIn_p & 0x1)  << 31);
            break;
        case gam_emul_TYPE_A8 :
            *ValueOut_p = (U32)((*ValueIn_p & 0xFF) << 24);
            break;
        case gam_emul_TYPE_ACLUT44:
            if ((*ValueIn_p & 0xF0) == 0)
            {
                Alpha = 0;
            }
            else if ((*ValueIn_p & 0xF0) == 0xF)
            {
                Alpha = 128;
            }
            else
            {
                Alpha = ((*ValueIn_p & 0xF0) >> 1) | 0x4;
            }
            *ValueOut_p = (U32)((*ValueIn_p & 0xF)  |
                                (Alpha << 20));
            break;
        case gam_emul_TYPE_ACLUT88:
            *ValueOut_p = (U32)(( *ValueIn_p & 0xFF)  |
                                ((*ValueIn_p & 0xFF00) << 16));
            break;
        case gam_emul_TYPE_CLUT1:
            *ValueOut_p = *ValueIn_p & 0x1;
            break;
        case gam_emul_TYPE_CLUT2:
            *ValueOut_p = *ValueIn_p & 0x3;
            break;
        case gam_emul_TYPE_CLUT4:
            *ValueOut_p = *ValueIn_p & 0xF;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    return(Err);
 }



/*******************************************************************************
Name        : DirectCopy
Description : Initialize gamma emulator related stuff
Parameters  :
Assumptions :  If STAVMEM used, Copy Direction HAS TO BE Down right !!
               Direct fill convern Src1 and Target only!
               Only WriteInsideClippingRectangle supported
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DirectCopy(gam_emul_BlitterParameters *Blt_p)
{
    ST_ErrorCode_t  Err                     = ST_NO_ERROR;
    U32             SrcAddress;
    U32             DstAddress;
    U32             Width;
    U32             DstXMin, DstXMax,DstYMin, DstYMax, PixWidth, PixHeight;
    U32             SrcXMin, SrcYMin, XOffset, YOffset;
    BOOL            CanBeOptimized          = FALSE;
    U32             T_XMax                  = Blt_p->T_x + Blt_p->S1_width - 1;
    U32             T_YMax                  = Blt_p->T_y + Blt_p->S1_height - 1;

    if (Blt_p->EnaRectangularClipping == 1)
    {
        /* Determine intersection beetwen clipping rectangle (if any) and dst rectangle */
         if ((Blt_p->ClipXMin > T_XMax)     ||
            (Blt_p->ClipYMin > T_YMax)     ||
            (Blt_p->T_x > Blt_p->ClipXMax) ||
            (Blt_p->T_y > Blt_p->ClipYMax))
        {
            /* No intersection between clipping rectangle and dst rectangle */
			return (Err);
        }
        else
        {
            if (Blt_p->ClipXMin >= Blt_p->T_x)
            {
                DstXMin  = Blt_p->ClipXMin ;
                if (Blt_p->ClipXMax <= T_XMax)
                {
                    DstXMax = Blt_p->ClipXMax;
                }
                else
                {
                    DstXMax = T_XMax;
                }
            }
            else  /* Blt_p->T_x >  Blt_p->ClipXMin */
            {
                DstXMin  = Blt_p->T_x ;
                if (T_XMax <= Blt_p->ClipXMax)
                {
                    DstXMax = T_XMax;
                }
                else
                {
                    DstXMax = Blt_p->ClipXMax;
                }
            }

            if (Blt_p->ClipYMin >= Blt_p->T_y)
            {
                DstYMin  = Blt_p->ClipYMin ;
                if (Blt_p->ClipYMax <= T_YMax)
                {
                    DstYMax = Blt_p->ClipYMax;
                }
                else
                {
                    DstYMax = T_YMax;
                }
            }
            else /* Blt_p->T_y >  Blt_p->ClipYMin */
            {
                DstYMin  = Blt_p->T_y ;
                if (T_YMax <= Blt_p->ClipYMax)
                {
                    DstYMax = T_YMax;
                }
                else
                {
                    DstYMax = Blt_p->ClipYMax;
                }
            }
        }
    }
    else
    {
        DstXMin = Blt_p->T_x;
        DstXMax = T_XMax;
        DstYMin = Blt_p->T_y;
        DstYMax = T_YMax;
    }
    PixWidth  = DstXMax - DstXMin + 1;
    PixHeight = DstYMax - DstYMin + 1;

    /* Position Offset in src/dst rectangles relatively to top-left pixel of src/dst rectangle */
    XOffset = DstXMin - Blt_p->T_x ;
    YOffset = DstYMin - Blt_p->T_y ;
    SrcXMin = Blt_p->S1_x + XOffset;
    SrcYMin = Blt_p->S1_y + YOffset;

	switch (Blt_p->T_format)
    {
        case gam_emul_TYPE_ARGB8888:
        case gam_emul_TYPE_RGB888:
        case gam_emul_TYPE_ARGB8565:
        case gam_emul_TYPE_RGB565:
        case gam_emul_TYPE_ARGB1555:
        case gam_emul_TYPE_ARGB4444:
        case gam_emul_TYPE_CLUT8:
        case gam_emul_TYPE_ACLUT44:
        case gam_emul_TYPE_ACLUT88:
        case gam_emul_TYPE_YCbCr888:
        case gam_emul_TYPE_A8:
        case gam_emul_TYPE_BYTE:
            SrcAddress =  (U32)(Blt_p->S1_ba + gam_emul_BLTMemory + (SrcYMin * Blt_p->S1_pitch) + (SrcXMin * (Blt_p->S1_bpp>>3)));
            DstAddress =  (U32)(Blt_p->T_ba  + gam_emul_BLTMemory + (DstYMin * Blt_p->T_pitch) + (DstXMin * (Blt_p->T_bpp>>3)));
            Width      =  (U32)(PixWidth) * (Blt_p->S1_bpp>>3);
            CanBeOptimized  = TRUE;
            break;
        case gam_emul_TYPE_YCbCr422R:
            /* Dst And Src must have same parity */
            if (((SrcXMin % 2) == 0)  && ((DstXMin % 2) == 0))
            {
                SrcAddress =  (U32)(Blt_p->S1_ba + gam_emul_BLTMemory + (SrcYMin * Blt_p->S1_pitch) + (SrcXMin * 2));
                DstAddress =  (U32)(Blt_p->T_ba + gam_emul_BLTMemory + (DstYMin * Blt_p->T_pitch) + (DstXMin * 2));
                if ((PixWidth % 2) == 0) /* even pixel width */
                {
                    Width = (U32)(PixWidth * 2);
                }
                else
                {
                    Width = (U32)((PixWidth - 1) * 2 + 3);
                }
                CanBeOptimized  = TRUE;
            }
            else if (((SrcXMin % 2) != 0)  && ((DstXMin % 2) != 0))
            {
                SrcAddress =  (U32)(Blt_p->S1_ba + gam_emul_BLTMemory + (SrcYMin * Blt_p->S1_pitch) + ((SrcXMin - 1) * 2) + 3);
                DstAddress =  (U32)(Blt_p->T_ba + gam_emul_BLTMemory + (DstYMin * Blt_p->T_pitch) + ((DstXMin - 1) * 2) + 3);
                if ((PixWidth % 2) == 0)  /* even pixel width */
                {
                    Width = (U32)(PixWidth * 2);
                }
                else
                {
                    Width = (U32)((PixWidth - 1) * 2 + 1);
                }
                CanBeOptimized  = TRUE;
            }
            break;
        case gam_emul_TYPE_CLUT4:
        case gam_emul_TYPE_CLUT2:
        case gam_emul_TYPE_CLUT1:
        case gam_emul_TYPE_A1:
        case gam_emul_TYPE_YCbCr422MB:
        case gam_emul_TYPE_YCbCr420MB:
            break;
        default:
            break;
    }

    if (CanBeOptimized == TRUE)
    {
        #if defined(ST_OS21) || defined(ST_OSWINCE)
			Err = STAVMEM_CopyBlock2D((void*)SrcAddress,Width,(U32)PixHeight,(U32)Blt_p->S1_pitch,
				                (void*)DstAddress,(U32)Blt_p->T_pitch);
        #endif
        #ifdef ST_OS20
			STAVMEM_CopyBlock2D((void*)SrcAddress,Width,(U32)PixHeight,(U32)Blt_p->S1_pitch,
				                (void*)DstAddress,(U32)Blt_p->T_pitch);
        #endif
    }
    else
    {
        gam_emul_DirectCopy(Blt_p);
    }

    return(Err);
}

/*******************************************************************************
Name        : DirectFill
Description :
Parameters  :
Assumptions : Only WriteInsideClippingRectangle supported
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t DirectFill(gam_emul_BlitterParameters *Blt_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    BOOL            CanBeOptimized = FALSE;
    U32             DstAddress;
    U32             Pattern;
    U32             DstWidth;
    U32             DstXMin, DstXMax,DstYMin, DstYMax, PixWidth, PixHeight;
    U32             T_XMax                  = Blt_p->T_x + Blt_p->S1_width - 1;
    U32             T_YMax                  = Blt_p->T_y + Blt_p->S1_height - 1;

    if (Blt_p->EnaRectangularClipping == 1)
    {
         /* Determine intersection beetwen clipping rectangle (if any) and dst rectangle */
        if ((Blt_p->ClipXMin > T_XMax)     ||
            (Blt_p->ClipYMin > T_YMax)     ||
            (Blt_p->T_x > Blt_p->ClipXMax) ||
            (Blt_p->T_y > Blt_p->ClipYMax))
        {
            /* No intersection between clipping rectangle and dst rectangle */
            return(Err);
        }
        else
        {
            if (Blt_p->ClipXMin >= Blt_p->T_x)
            {
                DstXMin  = Blt_p->ClipXMin ;
                if (Blt_p->ClipXMax <= T_XMax)
                {
                    DstXMax = Blt_p->ClipXMax;
                }
                else
                {
                    DstXMax = T_XMax;
                }
            }
            else  /* Blt_p->T_x >  Blt_p->ClipXMin */
            {
                DstXMin  = Blt_p->T_x ;
                if (T_XMax <= Blt_p->ClipXMax)
                {
                    DstXMax = T_XMax;
                }
                else
                {
                    DstXMax = Blt_p->ClipXMax;
                }
            }

            if (Blt_p->ClipYMin >= Blt_p->T_y)
            {
                DstYMin  = Blt_p->ClipYMin ;
                if (Blt_p->ClipYMax <= T_YMax)
                {
                    DstYMax = Blt_p->ClipYMax;
                }
                else
                {
                    DstYMax = T_YMax;
                }
            }
            else /* Blt_p->T_y >  Blt_p->ClipYMin */
            {
                DstYMin  = Blt_p->T_y ;
                if (T_YMax <= Blt_p->ClipYMax)
                {
                    DstYMax = T_YMax;
                }
                else
                {
                    DstYMax = Blt_p->ClipYMax;
                }
            }
        }
    }
    else
    {
        DstXMin = Blt_p->T_x;
        DstXMax = T_XMax;
        DstYMin = Blt_p->T_y;
        DstYMax = T_YMax;
    }
    PixWidth  = DstXMax - DstXMin + 1;
    PixHeight = DstYMax - DstYMin + 1;

    switch (Blt_p->T_format)
    {
        case gam_emul_TYPE_ARGB8888:
        case gam_emul_TYPE_RGB888:
        case gam_emul_TYPE_ARGB8565:
        case gam_emul_TYPE_YCbCr888:
        case gam_emul_TYPE_RGB565:
        case gam_emul_TYPE_ARGB1555:
        case gam_emul_TYPE_ARGB4444:
        case gam_emul_TYPE_ACLUT88:
        case gam_emul_TYPE_CLUT8:
        case gam_emul_TYPE_ACLUT44:
        case gam_emul_TYPE_A8:
        case gam_emul_TYPE_BYTE:
            CanBeOptimized = TRUE;
            break;
        case gam_emul_TYPE_YCbCr422R:
        case gam_emul_TYPE_CLUT4:
        case gam_emul_TYPE_CLUT2:
        case gam_emul_TYPE_CLUT1:
        case gam_emul_TYPE_A1:
        case gam_emul_TYPE_YCbCr422MB:
        case gam_emul_TYPE_YCbCr420MB:
            break;
        default:
            break;
    }

    if (CanBeOptimized == TRUE)
    {
        DstWidth    = PixWidth * (Blt_p->T_bpp>>3);
        DstAddress  =  (U32)(Blt_p->T_ba  + gam_emul_BLTMemory + (DstYMin * Blt_p->T_pitch) + DstXMin * (Blt_p->T_bpp>>3)); /* Always the case
                                                                                                           according to formats
                                                                                                           in previous switch */
        Pattern = (U32)Blt_p->S1_colorfill;



        STAVMEM_FillBlock2D((void*)&Pattern, (U32)(Blt_p->T_bpp>>3), 1, 1,(void*)DstAddress,
                            (U32)DstWidth,(U32)PixHeight,(U32)Blt_p->T_pitch );




    }
    else
    {
        gam_emul_DirectFill(Blt_p);
    }



    return(Err);
}

/*******************************************************************************
Name        : WriteColorLineWhenDstColorKey
Description :
Parameters  :
Assumptions : Color is on the same format as destination bitmap one
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t  WriteColorLineWhenDstColorKey(void* DstXMinAddress_p, U32 PixWidth, U32 Color, gam_emul_BlitterParameters *Blt_p)
{
	int r,EnableRKey,ROutRange;
	int g,EnableGKey,GOutRange;
	int b,EnableBKey,BOutRange;
    U32 DstAddress;
    U32 DstColor;
    U32 DstInternalColor;
    U32 x;
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    EnableRKey = Blt_p->CHK_RMode & 1;
    EnableGKey = Blt_p->CHK_GMode & 1;
    EnableBKey = Blt_p->CHK_BMode & 1;
    ROutRange = (Blt_p->CHK_RMode >> 1) & 1;
    GOutRange = (Blt_p->CHK_GMode >> 1) & 1;
    BOutRange = (Blt_p->CHK_BMode >> 1) & 1;


    for (x=0 ; x < PixWidth ; x++ )
    {
        /* Get Color from bitmap */

        DstAddress = (U32)DstXMinAddress_p + x * (Blt_p->T_bpp>>3);
        DstColor = 0;

        switch (Blt_p->T_bpp)
        {
            case 32 :
                DstColor = (U32)STSYS_ReadRegMem32LE((void*)DstAddress);
                break;

            case 24 :
                /* Can not use 24bit STSYS routine because address not always aligned on 32 bit */
                DstColor  = (U32)STSYS_ReadRegMem8((void*)DstAddress) & 0xFF;
                DstColor |= (U32)((STSYS_ReadRegMem8((void*)((U32)DstAddress + 1)) & 0xFF) << 8);
                DstColor |= (U32)((STSYS_ReadRegMem8((void*)((U32)DstAddress + 2)) & 0xFF) << 16);
                break;

            case 16 :
                DstColor = (U32)STSYS_ReadRegMem16LE((void*)DstAddress);
                break;

            case 8 :
                DstColor = (U32)STSYS_ReadRegMem8((void*)DstAddress);
                break;

            default :  /* Not supported */
                break;
        }

        /* Format Color to Internal h/w bus formatter */
        FormatOutputToInput(&DstColor,&DstInternalColor,Blt_p);

        r = (DstInternalColor >> 16) & 0xFF;
        g = (DstInternalColor >> 8) & 0xFF;
        b = DstInternalColor & 0xFF;
        if (((((g <  Blt_p->CHK_GMin) || (g >  Blt_p->CHK_GMax)) && (GOutRange==1) && (EnableGKey==1)) ||
              ((g >= Blt_p->CHK_GMin) && (g <= Blt_p->CHK_GMax)  && (GOutRange==0) && (EnableGKey==1)) ||
               (EnableGKey==0))
                                &&
            ((((b <  Blt_p->CHK_BMin) || (b >  Blt_p->CHK_BMax)) && (BOutRange==1) && (EnableBKey==1)) ||
              ((b >= Blt_p->CHK_BMin) && (b <= Blt_p->CHK_BMax) && (BOutRange==0) && (EnableBKey==1))  ||
               (EnableBKey==0))
                                &&
            ((((r <  Blt_p->CHK_RMin) || (r >  Blt_p->CHK_RMax)) && (ROutRange==1) && (EnableRKey==1)) ||
              ((r >= Blt_p->CHK_RMin) && (r <= Blt_p->CHK_RMax) && (ROutRange==0) && (EnableRKey==1))  ||
               (EnableRKey==0)))
        {
            /*  Write onto Target */
            switch (Blt_p->T_bpp)
            {
                case 32 :
                    STSYS_WriteRegMem32LE((void*)DstAddress,Color);
                    break;

                case 24 :
                    /* Can not use 24bit STSYS routine because address not always aligned on 32 bit */
                    STSYS_WriteRegMem8((void*)DstAddress,Color);
                    STSYS_WriteRegMem8((void*)((U32)DstAddress + 1),(Color >> 8 ) & 0xFF);
                    STSYS_WriteRegMem8((void*)((U32)DstAddress + 2),(Color >> 16) & 0xFF);
                    break;

                case 16 :
                    STSYS_WriteRegMem16LE((void*)DstAddress,Color);
                    break;

                case 8 :
                    STSYS_WriteRegMem8((void*)DstAddress,Color);
                    break;

                default :  /* Not supported */
                    break;
            }
        }
    }
    return(Err);
}


/*******************************************************************************
Name        : OptimizedDrawXYL
Description :
Parameters  :
Assumptions : Only WriteInsideClippingRectangle supported
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t OptimizedDrawXYL(gam_emul_BlitterParameters *Blt_p)
{
    ST_ErrorCode_t  Err             = ST_NO_ERROR;
    BOOL            CanBeOptimized  = FALSE;
    U32             DstAddress;
    U32             Pattern;
    U32             DstWidth;
    U32             DstXMin, DstXMax,DstYMin, DstYMax, PixWidth;
    U32             T_XMax, T_XMin,T_YMin, T_YMax;
    U32             Length;
    U32             Color;
    U32             i;

    switch (Blt_p->T_format)
    {
        case gam_emul_TYPE_ARGB8888:
        case gam_emul_TYPE_RGB888:
        case gam_emul_TYPE_ARGB8565:
        case gam_emul_TYPE_YCbCr888:
        case gam_emul_TYPE_RGB565:
        case gam_emul_TYPE_ARGB1555:
        case gam_emul_TYPE_ARGB4444:
        case gam_emul_TYPE_ACLUT88:
        case gam_emul_TYPE_CLUT8:
        case gam_emul_TYPE_ACLUT44:
        case gam_emul_TYPE_A8:
        case gam_emul_TYPE_BYTE:
            CanBeOptimized = TRUE;
            break;
        case gam_emul_TYPE_YCbCr422R:
        case gam_emul_TYPE_CLUT4:
        case gam_emul_TYPE_CLUT2:
        case gam_emul_TYPE_CLUT1:
        case gam_emul_TYPE_A1:
        case gam_emul_TYPE_YCbCr422MB:
        case gam_emul_TYPE_YCbCr420MB:
            break;
        default:
            break;
    }

    if (CanBeOptimized == TRUE)
    {

        for (i=0;i< Blt_p->XYL_buff_format; i++)
        {
            /* Fetch element in array */
            T_XMin = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i);
            T_YMin = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i + 4);

            switch (Blt_p->XYL_format & 0x3)
            {
                case 0 :  /* XY subinstruction format */
                    Color = (U32)(Blt_p->S2_colorfill);
                    Length = 1;
                    break;

                case 1 : /* XYL subinstruction format */
                    Color = (U32)(Blt_p->S2_colorfill);
                    Length = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i + 8);
                    break;

                case 2 : /* XYC subinstruction format */
                    Color  = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i + 12);
                    Length = 1;
                    break;

                case 3 : /* XYLC subinstruction format */
                    Color  = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i + 12);
                    Length = *(U32*)(Blt_p->XYL_xyp + gam_emul_BLTMemory + 16 * i + 8);
                    break;

                default :
                    break;
            }

            T_XMax = T_XMin + Length - 1;
            T_YMax = T_YMin;

            /* Clip if any */
            if (Blt_p->EnaRectangularClipping == 1)
            {
                /* Determine intersection beetwen clipping rectangle (if any) and dst rectangle */
                if ((Blt_p->ClipXMin > T_XMax)     ||
                    (Blt_p->ClipYMin > T_YMax)     ||
                    (T_XMin > Blt_p->ClipXMax)     ||
                    (T_YMin > Blt_p->ClipYMax))
                {
                    /* No intersection between clipping rectangle and dst rectangle
                     * go to next loop iteration */
                    continue;
                }
                else
                {
                    if (Blt_p->ClipXMin >= T_XMin)
                    {
                        DstXMin  = Blt_p->ClipXMin ;
                        if (Blt_p->ClipXMax <= T_XMax)
                        {
                            DstXMax = Blt_p->ClipXMax;
                        }
                        else
                        {
                            DstXMax = T_XMax;
                        }
                    }
                    else  /* T_XMin >  Blt_p->ClipXMin */
                    {
                        DstXMin  = T_XMin ;
                        if (T_XMax <= Blt_p->ClipXMax)
                        {
                            DstXMax = T_XMax;
                        }
                        else
                        {
                            DstXMax = Blt_p->ClipXMax;
                        }
                    }
                    if (Blt_p->ClipYMin >= T_YMin)
                    {
                        DstYMin  = Blt_p->ClipYMin ;
                        if (Blt_p->ClipYMax <= T_YMax)
                        {
                            DstYMax = Blt_p->ClipYMax;
                        }
                        else
                        {
                            DstYMax = T_YMax;
                        }
                    }
                    else /* T_YMin >  Blt_p->ClipYMin */
                    {
                        DstYMin  = T_YMin ;
                        if (T_YMax <= Blt_p->ClipYMax)
                        {
                            DstYMax = T_YMax;
                        }
                        else
                        {
                            DstYMax = Blt_p->ClipYMax;
                        }
                    }
                }
            }
            else
            {
                DstXMin = T_XMin;
                DstXMax = T_XMax;
                DstYMin = T_YMin;
                DstYMax = T_YMax;
            }

            PixWidth    = DstXMax - DstXMin + 1;
            DstWidth    = PixWidth * (Blt_p->T_bpp>>3);
            DstAddress  = (U32)(Blt_p->T_ba  + gam_emul_BLTMemory + (DstYMin * Blt_p->T_pitch) + DstXMin * (Blt_p->T_bpp>>3));

            FormatInputToOutput ((U32*)&Color, (U32*)&Pattern,Blt_p);

            if (Blt_p->EnaColorKey == 1)
            {
                /* Set Destination color key */
                WriteColorLineWhenDstColorKey((void*)DstAddress,PixWidth,Pattern, Blt_p);
            }
            else
            {
                STAVMEM_FillBlock1D((void*)&Pattern, (U32)(Blt_p->T_bpp>>3), (void*)DstAddress, (U32)DstWidth);
            }
        }
    }
    else
    {
        gam_emul_XYLBlitterOperation(Blt_p);
    }



    return(Err);
}


/*******************************************************************************
Name        : stblit_EmulatorProcessNode
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
U32 stblit_EmulatorProcessNode(stblit_Device_t* Device_p, U32 CpuNodeAddress, BOOL *EnaBlitCompletedIRQ_p)
{
    stblit_Node_t*              Node_p;
    U32                         DeviceNodeAddress;
    gam_emul_BlitterParameters  Blt;
	ST_ErrorCode_t Error;

    /* Get Node_p descriptor */
    Node_p = (stblit_Node_t*) CpuNodeAddress;

    /* We extract all the parameters, and get the next device node address */
    DeviceNodeAddress = gam_emul_ExtractBlitterParameters(CpuNodeAddress,&Blt);

   /* Instruction Analysis */
   switch (Blt.BLT_instruction)
   {
       case 0x04:   /* Scr1 Direct Copy mode */
       case 0x0C:
       case 0x14:
       case 0x1C:
           /*  For WriteOutsideClippingRectangle, rely on generic emulation function  */
           if ((Blt.EnaRectangularClipping == 1) && (Blt.InternalClip == 1))
           {
			    gam_emul_GeneralBlitterOperation(Device_p,&Blt);
		   }
           else
           {
		        DirectCopy(&Blt);
		   }
           break;
       case 0x07:   /* Scr1 Direct Fill mode */
       case 0x0F:
       case 0x17:
       case 0x1F:
            /*  For WriteOutsideClippingRectangle, rely on generic emulation function  */
           if ((Blt.EnaRectangularClipping == 1) && (Blt.InternalClip == 1))
           {
			   gam_emul_GeneralBlitterOperation(Device_p,&Blt);
		   }
           else
           {
			   Error = DirectFill(&Blt);
		   }
           break;
       case 0:  /* Both sources are disabled */
           break;
       default:
		   if (Blt.XYLModeEnable == 1)  /*XYL enabled*/
           {
               /* XYL standard */
               if (Blt.XYL_mode == 0)
               {
                   /* Fetch optimization info in Reserved field from Node_p */
                   if ((Node_p->Reserved & 0x1) == 1)  /* Optimization possible (Note : If clipping, only writeInside) */
                   {
                       OptimizedDrawXYL(&Blt);
                   }
                   else
                   {
                       gam_emul_XYLBlitterOperation(&Blt);
                   }
               }

           }
           else /* no XYL*/
           {
		       gam_emul_GeneralBlitterOperation(Device_p,&Blt);
		   }
           break;
   }

   if (Blt.EnaBlitCompletedIRQ == 1)
   {
       *EnaBlitCompletedIRQ_p = TRUE;
   }
   else
   {
       *EnaBlitCompletedIRQ_p = FALSE;
   }

   return(DeviceNodeAddress);
}

/*******************************************************************************
Name        : stblit_EmulatorProcess
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_EmulatorProcess (void* data_p)
{
    stblit_Device_t*    Device_p;
    U32                 DeviceNodeAddress;
    U32                 CpuNodeAddress;
    S32                 Value;
    BOOL                EnaBlitCompletedIRQ;

    Device_p = (stblit_Device_t*) data_p;

    /* Set Global for emulator
     * Attention : The offset PhysicalCPU-PhysicalDevice > 0  has to be positive
     * No problem with 55XX/70XX since Physical Device = 0 */
    gam_emul_BLTMemory = (char*) ((U32)Device_p->VirtualMapping.PhysicalAddressSeenFromCPU_p - (U32)Device_p->VirtualMapping.PhysicalAddressSeenFromDevice_p);

    while (TRUE)
    {
        STOS_SemaphoreWait(Device_p->EmulatorSuspendSemaphore, TIMEOUT_IMMEDIATE);(Device_p->EmulatorStartSemaphore);
        if (Device_p->TaskTerminate == TRUE)
        {
            break;
        }

        interrupt_lock();
        DeviceNodeAddress = STSYS_ReadRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET));
        CpuNodeAddress = (U32)stblit_DeviceToCpu((void*)DeviceNodeAddress,&(Device_p->VirtualMapping));

        interrupt_unlock();

        if (DeviceNodeAddress != 0)
        {

            while(DeviceNodeAddress != 0)
            {
                 interrupt_lock();
                /* Update Status1 registers */
                STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA1_OFFSET),DeviceNodeAddress) ;

                interrupt_unlock();

                /* Process Cpu Node and retrieve Next node device address + Blit completion IT info */
                DeviceNodeAddress = stblit_EmulatorProcessNode(Device_p, CpuNodeAddress,&EnaBlitCompletedIRQ);

                /* Get Next node Cpu address */
                CpuNodeAddress = (U32)stblit_DeviceToCpu((void*)DeviceNodeAddress, &(Device_p->VirtualMapping));

                /* Update Status3 registers */
                interrupt_lock();
                if (DeviceNodeAddress  == 0)
                {
                    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA3_OFFSET), BLT_STA3_COMPLETED | BLT_STA3_READY) ;
                }
                else
                {
                    STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_STA3_OFFSET), BLT_STA3_COMPLETED) ;

                }

                /* Update NIP (in real HW, this update is always done!) : Note that if there is an interrupt
                 * this value may be rewritten by the handler in case of node insertion. */
                STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_NIP_OFFSET),DeviceNodeAddress);



                interrupt_unlock();

                /* Look if interruption*/
                Value = STOS_SemaphoreWaitTimeOut(Device_p->EmulatorSuspendSemaphore, TIMEOUT_IMMEDIATE);
                if ((Value == 0) || (EnaBlitCompletedIRQ == TRUE))
                {
                    /* Call Interrupt service routine (HPP)*/
                    STOS_SemaphoreSignal(Device_p->EmulatorStartISRSemaphore);

                    /* Wait for ISR to be done */
                    STOS_SemaphoreWait(Device_p->EmulatorStopISRSemaphore);
                    break;
                }
            }
        }
    }
}

/*******************************************************************************
Name        : stblit_EmulatorISRProcess
Description : Interrupt Service routine emulated by an High Priority process
Parameters  :
Assumptions : High Priority Process (HPP)
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_EmulatorISRProcess (void* data_p)
{
    stblit_Device_t* Device_p;
    U32 Dummy = 0;
    ST_DeviceName_t DummyName;

    Device_p = (stblit_Device_t*) data_p;

    while(TRUE)
    {
        STOS_SemaphoreWait(Device_p->EmulatorStartISRSemaphore);
		if (Device_p->TaskTerminate == TRUE)
        {
			break;
        }
		interrupt_lock();
        stblit_BlitterInterruptEventHandler((STEVT_CallReason_t) Dummy, DummyName,(STEVT_EventConstant_t) Dummy,
                                       (void*)Dummy,(void*)Device_p);
        interrupt_unlock();
        STOS_SemaphoreSignal(Device_p->EmulatorStopISRSemaphore);
    }
}


 /* End of blt_emu.c */
