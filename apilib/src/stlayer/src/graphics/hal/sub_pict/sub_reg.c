/*******************************************************************************

File name   : Sub_reg.c

Description : register source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2000        Created                                           TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sub_reg.h"
#include "sub_spu.h"
#include "stgxobj.h"
#include "stsys.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* correction computed on 5516 / PAL / NTSC */
/* good to be checked on all 55xx */
#ifdef HW_5517
#define SUB_PAL_HORIZONTAL_START  (0)
#define SUB_PAL_VERTICAL_START    (-2)
#define SUB_NTSC_HORIZONTAL_START (-4)
#define SUB_NTSC_VERTICAL_START   (-2)
#else
#define SUB_PAL_HORIZONTAL_START  (+1)
#define SUB_PAL_VERTICAL_START    (-2)
#define SUB_NTSC_HORIZONTAL_START (-3)
#define SUB_NTSC_VERTICAL_START   (-2)
#endif

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : layersub_SetPalette
Description :
Parameters  :
Assumptions :  Non NULL Device_p
               Device_p->ActiveViwPort != LAYERSUB_NO_VIEWPORT_HANDLE
               Palette is  ARGB888. ITU-601 used for YCrCB conversion
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetPalette(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U32             i;
    U8*             Component_p;
    U8              Y,Cb, Cr;
    U8              R, G, B;

    Component_p = (U8*)STAVMEM_VirtualToCPU((void*)(((layersub_ViewPort_t*)(Device_p->ActiveViewPort))->Palette.Data_p),
                                           &(Device_p->VirtualMapping));

    /* reset CLUT Fifo */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), LAYERSUB_SPD_CTL2_RC);
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL2_OFFSET), 0);

    for (i = 0; i < 4; i++)     /* Don't care of other possible palette entries (4th->16th) */
    {

        R    = (*(Component_p + (4 * i) + 2)) & 0xFF;
        G    = (*(Component_p + (4 * i) + 1)) & 0xFF;
        B    = (*(Component_p + (4 * i))) & 0xFF;
        ((layersub_ViewPort_t*)(Device_p->ActiveViewPort))->Alpha[i] = (*(Component_p + (4 * i) + 3)) & 0xFF;

        /* Set Color */
        Y  = (((U32)( 263*(S32)R + 516*(S32)G + 100*(S32)B) / 1024) + 16 ) & 0xFF;
        Cb = (((U32)(-152*(S32)R - 298*(S32)G + 450*(S32)B) / 1024) + 128) & 0xFF;
        Cr = (((U32)( 450*(S32)R - 377*(S32)G - 73 *(S32)B) / 1024) + 128) & 0xFF;


        /* Y component */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_LUT_OFFSET),
                           Y & LAYERSUB_SPD_LUT_MASK);

#if defined (HW_5512) || defined (HW_5510) || defined (HW_5514) || defined (HW_5516)\
 || defined (HW_5517)
        /* Cb component */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_LUT_OFFSET),
                           Cb & LAYERSUB_SPD_LUT_MASK);

        /* Cr component */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_LUT_OFFSET),
                           Cr & LAYERSUB_SPD_LUT_MASK);
#else  /* HW_5508, HW_5518 */
        /* Cr component */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_LUT_OFFSET),
                           Cr & LAYERSUB_SPD_LUT_MASK);

        /* Cb component */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_LUT_OFFSET),
                           Cb & LAYERSUB_SPD_LUT_MASK);
#endif
    }

    /* Update Contrast field in DCSQ */
    layersub_UpdateContrast(Device_p,(layersub_ViewPort_t*)(Device_p->ActiveViewPort));

    return(Err);
}

/*******************************************************************************
Name        : layersub_SetDisplayArea
Description :
Parameters  :
Assumptions : Non NULL Device_p
              Device_p->ActiveViwPort != LAYERSUB_NO_VIEWPORT_HANDLE
              Start positions are > 0  , End Position ?? (cursor point top left corner)
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetDisplayArea(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t          Err         = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*)Device_p->ActiveViewPort;

    /* Set SXDO */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SXDO_LESS_BIT_OFFSET),
                        0 & LAYERSUB_SPD_SXDO_LESS_BIT_MASK);
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SXDO_MOST_BIT_OFFSET),
                        0 & LAYERSUB_SPD_SXDO_MOST_BIT_MASK);

    /* Set SXD1 */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SXD1_LESS_BIT_OFFSET),
                        (U32)(ViewPort_p->OutputRectangle.Width - 1)
                              & LAYERSUB_SPD_SXD1_LESS_BIT_MASK);
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SXD1_MOST_BIT_OFFSET),
                        ((U32)(ViewPort_p->OutputRectangle.Width - 1) >> 8)
                               & LAYERSUB_SPD_SXD1_MOST_BIT_MASK);


   /* Set SYDO */
   STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SYDO_LESS_BIT_OFFSET),
                        0 & LAYERSUB_SPD_SYDO_LESS_BIT_MASK);
   STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SYDO_MOST_BIT_OFFSET),
                        0 & LAYERSUB_SPD_SYDO_MOST_BIT_MASK);

   /* Set SYD1 */
   STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SYD1_LESS_BIT_OFFSET),
                      (U32)(ViewPort_p->OutputRectangle.Height - 1)
                            & LAYERSUB_SPD_SYD1_LESS_BIT_MASK);
   STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_SYD1_MOST_BIT_OFFSET),
                      ((U32)(ViewPort_p->OutputRectangle.Height - 1) >> 8)
                            & LAYERSUB_SPD_SYD1_MOST_BIT_MASK);


    return(Err);
}

/*******************************************************************************
Name        : layersub_SetXDOYDO
Description :
Parameters  :
Assumptions :  Non NULL Device_p
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetXDOYDO(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    layersub_ViewPort_t*    ViewPort_p  = (layersub_ViewPort_t*)Device_p->ActiveViewPort;


    /* Set XDO */
    switch(Device_p->FrameRate)
    {
        case 25000: /*PAL*/
        case 50000: /*PAL*/
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_XD0_MOST_BIT_OFFSET),
                        ((U32)(Device_p->XStart
                               + Device_p->XOffset
                               + ViewPort_p->OutputRectangle.PositionX
                               + SUB_PAL_HORIZONTAL_START) >> 8)
                               & LAYERSUB_SPD_XD0_MOST_BIT_MASK);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_XDO_LESS_BIT_OFFSET),
                        (U32)(Device_p->XStart
                              + Device_p->XOffset
                              + ViewPort_p->OutputRectangle.PositionX
                              + SUB_PAL_HORIZONTAL_START)
                              & LAYERSUB_SPD_XDO_LESS_BIT_MASK);
        break;

        default: /* use ntsc ofset */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_XD0_MOST_BIT_OFFSET),
                        ((U32)(Device_p->XStart
                               + Device_p->XOffset
                               + ViewPort_p->OutputRectangle.PositionX
                               + SUB_NTSC_HORIZONTAL_START) >> 8)
                               & LAYERSUB_SPD_XD0_MOST_BIT_MASK);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_XDO_LESS_BIT_OFFSET),
                        (U32)(Device_p->XStart
                              + Device_p->XOffset
                              + ViewPort_p->OutputRectangle.PositionX
                              + SUB_NTSC_HORIZONTAL_START)
                              & LAYERSUB_SPD_XDO_LESS_BIT_MASK);
        break;
    }
   /* Set YDO */
    switch(Device_p->FrameRate)
    {
        case 25000: /*PAL*/
        case 50000: /*PAL*/
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_YDO_MOST_BIT_OFFSET),
                        ((U32)(Device_p->YStart
                               + Device_p->YOffset
                               + ViewPort_p->OutputRectangle.PositionY
                               + SUB_PAL_VERTICAL_START) >> 8)
                               & LAYERSUB_SPD_YDO_MOST_BIT_MASK);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_YDO_LESS_BIT_OFFSET),
                        (U32)(Device_p->YStart
                              + Device_p->YOffset
                              + ViewPort_p->OutputRectangle.PositionY
                              + SUB_PAL_VERTICAL_START)
                              & LAYERSUB_SPD_YDO_LESS_BIT_MASK);
        break;

        default: /* use ntsc offset */
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_YDO_MOST_BIT_OFFSET),
                        ((U32)(Device_p->YStart
                               + Device_p->YOffset
                               + ViewPort_p->OutputRectangle.PositionY
                               + SUB_NTSC_VERTICAL_START) >> 8)
                               & LAYERSUB_SPD_YDO_MOST_BIT_MASK);
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_YDO_LESS_BIT_OFFSET),
                        (U32)(Device_p->YStart
                              + Device_p->YOffset
                              + ViewPort_p->OutputRectangle.PositionY
                              + SUB_NTSC_VERTICAL_START)
                              & LAYERSUB_SPD_YDO_LESS_BIT_MASK);
        break;
    }


    return(Err);
}



/*******************************************************************************
Name        : layersub_SetBufferPointer
Description :
Parameters  :
Assumptions : Non NULL Device_p
              Device_p->ActiveViwPort != LAYERSUB_NO_VIEWPORT_HANDLE
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_SetBufferPointer(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t          Err                 = ST_NO_ERROR;
    U32                     ReadPointer;
    U32                     WritePointer;
    U8*                     Address_p;
    layersub_ViewPort_t*    ViewPort_p          = (layersub_ViewPort_t*)Device_p->ActiveViewPort;

    /* Translate from Virtual to CPU mapping */
    Address_p = (U8*)STAVMEM_VirtualToCPU((void*)ViewPort_p->Header_p,&(Device_p->VirtualMapping));

    /* Address Offset in unit of 64bit relative to SDRAM base address
     * In spec, it is supposed to be absolut address but upper bit are troncated so it's an offset!*/
    ReadPointer  = (U32)(Address_p) / 8;
    WritePointer = ReadPointer ;

    interrupt_lock();

    /* Set SPRead */
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPREAD_OFFSET),
                       (ReadPointer >> 16) & LAYERSUB_VID_SPREAD_FIRST_CYCLE_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPREAD_OFFSET),
                       (ReadPointer >> 8) & LAYERSUB_VID_SPREAD_SECOND_CYCLE_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPREAD_OFFSET),
                        ReadPointer & LAYERSUB_VID_SPREAD_THIRD_CYCLE_MASK);

    /* Set SPWrite */
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPWRITE_OFFSET),
                       (WritePointer >> 16) & LAYERSUB_VID_SPWRITE_FIRST_CYCLE_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPWRITE_OFFSET),
                       (WritePointer >> 8) & LAYERSUB_VID_SPWRITE_SECOND_CYCLE_MASK);
    STSYS_WriteRegDev8((void*)(LAYERSUB_VID_BASE_ADDRESS + LAYERSUB_VID_SPWRITE_OFFSET),
                        WritePointer & LAYERSUB_VID_SPWRITE_THIRD_CYCLE_MASK);

    interrupt_unlock();

    return(Err);
}


/*******************************************************************************
Name        : layersub_StartSPU
Description :
Parameters  :
Assumptions :  Non NULL Device_p
               Device_p->ActiveViwPort != LAYERSUB_NO_VIEWPORT_HANDLE
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_StartSPU(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;
    U8              Value;

    /* Set Palette register */
    layersub_SetPalette(Device_p);

    /* Set SubPicture Display area registers */
    layersub_SetDisplayArea(Device_p);

    /* Set Offsets registers */
    layersub_SetXDOYDO(Device_p);

    /* Set Video Registers */
    layersub_SetBufferPointer(Device_p);

    /* Write Sub Picture Control register 1
     * Start it and enable display */
    Value =  LAYERSUB_SPD_CTL1_B |  LAYERSUB_SPD_CTL1_V  |  LAYERSUB_SPD_CTL1_D | LAYERSUB_SPD_CTL1_S;
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL1_OFFSET),Value & LAYERSUB_SPD_CTL1_MASK);


    return(Err);
}

/*******************************************************************************
Name        : layersub_StopSPU
Description :
Parameters  :
Assumptions :  Non NULL Device_p
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_StopSPU(layersub_Device_t* Device_p)
{
    ST_ErrorCode_t  Err = ST_NO_ERROR;

    /* Clear Sub Picture Control register 1 */
    STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL1_OFFSET),0);

    return(Err);
}

/*******************************************************************************
Name        : layersub_ReceiveEvtVSync
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void layersub_ReceiveEvtVSync (STEVT_CallReason_t      Reason,
                               const ST_DeviceName_t  RegistrantName,
                               STEVT_EventConstant_t  Event,
                               const void*            EventData_p,
                               const void*            SubscriberData_p)
{
    layersub_Device_t* Device_p;

    Device_p = (layersub_Device_t*)SubscriberData_p;

    if ((Device_p->SwapActiveSPU) || (Device_p->TaskTerminate))
    {
        semaphore_signal(Device_p->ProcessEvtVSyncSemaphore_p);
    }
}

/******************************************************************************
Name        : layersub_ProcessEvtVSync
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/

void layersub_ProcessEvtVSync (void* data_p)
{
    layersub_Device_t*    Device_p;
    U8              Value;

    /* Get Device_t info */
    Device_p = (layersub_Device_t*) data_p;

    while(TRUE)
    {
        semaphore_wait(Device_p->ProcessEvtVSyncSemaphore_p);

        Device_p->SwapActiveSPU = FALSE;

        if (Device_p->TaskTerminate)
        {
            break;
        }

        /* Process Update of registers */

        /* Set Palette register */
        layersub_SetPalette(Device_p);

        /* Set SubPicture Display area registers */
        layersub_SetDisplayArea(Device_p);

        /* Set Offsets registers */
        layersub_SetXDOYDO(Device_p);

        /* Set Video Registers */
        layersub_SetBufferPointer(Device_p);

        /* Write Sub Picture Control register 1
        * Start it and enable display */
        Value =  LAYERSUB_SPD_CTL1_B |  LAYERSUB_SPD_CTL1_V  |  LAYERSUB_SPD_CTL1_D | LAYERSUB_SPD_CTL1_S;
        STSYS_WriteRegDev8((void*)((U32)Device_p->BaseAddress_p + LAYERSUB_SPD_CTL1_OFFSET),Value & LAYERSUB_SPD_CTL1_MASK);
    }
}

/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of Sub_reg.c */
