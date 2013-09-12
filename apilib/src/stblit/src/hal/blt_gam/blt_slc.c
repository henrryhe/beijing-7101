/*******************************************************************************

File name   : blt_slc.c

Description : blitter slice source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
09 March 2001        Created                                           TM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <string.h>
#endif
#include "stddefs.h"
#include "stblit.h"
#include "stgxobj.h"
#include "blt_init.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "blt_com.h"
#include "blt_mem.h"
#include "blt_pool.h"
#include "blt_ctl.h"
#include "blt_job.h"
#include "blt_fe.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#define STBLIT_INCREMENT_NB_BITS_PRECISION  10
#define STBLIT_TAP_NUMBER_FILTER            5

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t  GetSliceNumber (STBLIT_SliceRectangleBuffer_t* Buffer_p, BOOL HorNotVer);
static ST_ErrorCode_t  GetSlicingInfo (STBLIT_SliceRectangleBuffer_t* Buffer_p, BOOL HorNotVer);

static ST_ErrorCode_t OneInputBlitSliceRectangle(stblit_Device_t*           Device_p,
                                                STBLIT_SliceData_t*         Src_p,
                                                STBLIT_SliceData_t*         Dst_p,
                                                STBLIT_SliceRectangle_t*    SliceRectangle_p,
                                                STBLIT_BlitContext_t*       BlitContext_p);

static ST_ErrorCode_t TwoInputBlitSliceRectangle(stblit_Device_t*           Device_p,
                                                 STBLIT_SliceData_t*        Src1_p,
                                                 STBLIT_SliceData_t*        Src2_p,
                                                 STBLIT_SliceData_t*        Dst_p,
                                                 STBLIT_SliceRectangle_t*   SliceRectangle_p,
                                                 STBLIT_BlitContext_t*      BlitContext_p);



/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        :  GetSliceNumber
Description :
Parameters  :
Assumptions : Non NULL and meaningfull Buffer_p ! (no test done)
              Fixed destination slice
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t  GetSliceNumber (STBLIT_SliceRectangleBuffer_t* Buffer_p, BOOL HorNotVer)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
    U32                         Increment = 0;
    U32                         Counter;
    U32                         Temp;
    U32                         NoMoreSlice = FALSE;
    U32                         ScalingFactor;
    U32                         DstSliceSize;
    U32                         DstFullSize;
    U32                         SrcFullSize;
    U32                         SrcSliceSize;
    U32                         DstSliceStart, DstSliceStop, SrcSliceStart, SrcSliceStop;
    U32                         ClipStart, ClipStop;
    U32                         NbDstSteps;
    U32                         SlicesNumber = 1; /* At least one slice */

    if (HorNotVer == TRUE) /* Horizontal slicing */
    {
        DstSliceSize = Buffer_p->FixedRectangle.Width;
        DstFullSize  = Buffer_p->FullSizeDstRectangle.Width;
        SrcFullSize  = Buffer_p->FullSizeSrcRectangle.Width;
    }
    else /* Vertical slicing */
    {
        DstSliceSize = Buffer_p->FixedRectangle.Height;
        DstFullSize  = Buffer_p->FullSizeDstRectangle.Height;
        SrcFullSize  = Buffer_p->FullSizeSrcRectangle.Height;
    }

    /* Scaling factor calculation */
    ScalingFactor =  ((SrcFullSize - 1) << STBLIT_INCREMENT_NB_BITS_PRECISION) / (DstFullSize - 1);
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ScalingFactor = %d \n\n",ScalingFactor));*/


    /* First Slice Dst and Src */
    DstSliceStart   = 0;
    DstSliceStop    = DstSliceStart + DstSliceSize - 1;
    NbDstSteps      = DstSliceStop;
    /* Counter for last element in source needed by destination slice last element */
    Counter         = (NbDstSteps  * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
    SrcSliceSize    = Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION;
/*    Increment       = Counter - (SrcSliceSize << STBLIT_INCREMENT_NB_BITS_PRECISION);*/  /* It is the increment for last pixel in
                                                                                            * first slice ! useless */
    Increment       = 0; /* The first slice start always with initial offset to 0 */
    SrcSliceStart   = 0;
    SrcSliceStop    = SrcSliceStart + SrcSliceSize - 1;
    ClipStart       = DstSliceStart;
    ClipStop        = DstSliceStop;


    /* Other Slices */
    while (NoMoreSlice == FALSE)
    {
        SlicesNumber++;

        /* Next destination slice first element */
        NbDstSteps = NbDstSteps + 1;

        /* Index of first element in source needed by next destination slice first element */
        SrcSliceStart   = ((NbDstSteps * ScalingFactor) >> STBLIT_INCREMENT_NB_BITS_PRECISION) + 3 - STBLIT_TAP_NUMBER_FILTER;

        /* Counter for last element in source needed by next destination slice first element */
        Counter         = (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        DstSliceStart   = DstSliceStop + 1;
        ClipStart       = DstSliceStart ;
        Temp            = Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION;
        while ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) > (Temp - 2))
        {
            DstSliceStart--;
            NbDstSteps--;
            Counter =  (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        }
        Increment = Counter - ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) << STBLIT_INCREMENT_NB_BITS_PRECISION);
        DstSliceStop  = DstSliceStart + DstSliceSize - 1;

        if (DstSliceStop >= (DstFullSize - 1))
        {
            DstSliceStop     = DstFullSize - 1;
            NoMoreSlice = TRUE;
        }

        NbDstSteps = DstSliceStop;

        /* Counter for last element in source needed by destination slice last element */
        Counter       = (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        SrcSliceSize  = ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) - 1) - SrcSliceStart + 1;
        SrcSliceStop  = SrcSliceStart + SrcSliceSize - 1;

        if (SrcSliceStop > (SrcFullSize - 1))
        {
            SrcSliceStop   = SrcFullSize - 1;
            SrcSliceSize   = SrcSliceStop - SrcSliceStart + 1;
        }

        ClipStop   = DstSliceStop;

    }

    /* Update slice number field */
    if (HorNotVer == TRUE) /* Horizontal slicing */
    {
        Buffer_p->HorizontalSlicingNumber = SlicesNumber;
    }
    else /* Vertical slicing */
    {
        Buffer_p->VerticalSlicingNumber = SlicesNumber;
    }


    return(Err);
}



/*******************************************************************************
Name        :  GetSlicingInfo
Description :
Parameters  :
Assumptions : Non NULL and meaningfull Buffer_p ! (no test done)
              Fixed destination slice
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t  GetSlicingInfo (STBLIT_SliceRectangleBuffer_t* Buffer_p, BOOL HorNotVer)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
    U32                         Increment = 0;
    U32                         Counter;
    U32                         Temp;
    U32                         NoMoreSlice = FALSE;
    U32                         ScalingFactor;
    U32                         DstSliceSize;
    U32                         DstFullSize;
    U32                         SrcFullSize;
    U32                         SrcSliceSize;
    U32                         DstSliceStart, DstSliceStop, SrcSliceStart, SrcSliceStop;
    U32                         ClipStart, ClipStop;
    U32                         NbDstSteps;
    STBLIT_SliceRectangle_t*    SliceRectangle_p;
    STBLIT_SliceRectangle_t*    StartAddress;
    U32                         i;

    if (HorNotVer == TRUE) /* Horizontal slicing */
    {
        DstSliceSize = Buffer_p->FixedRectangle.Width;
        DstFullSize  = Buffer_p->FullSizeDstRectangle.Width;
        SrcFullSize  = Buffer_p->FullSizeSrcRectangle.Width;
    }
    else /* Vertical slicing */
    {
        DstSliceSize = Buffer_p->FixedRectangle.Height;
        DstFullSize  = Buffer_p->FullSizeDstRectangle.Height;
        SrcFullSize  = Buffer_p->FullSizeSrcRectangle.Height;
    }

    /* Scaling factor calculation */
    ScalingFactor =  ((SrcFullSize - 1) << STBLIT_INCREMENT_NB_BITS_PRECISION) / (DstFullSize - 1);
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ScalingFactor = %d \n\n",ScalingFactor));*/


    /* First Slice Dst and Src */
    DstSliceStart   = 0;
    DstSliceStop    = DstSliceStart + DstSliceSize - 1;
    NbDstSteps      = DstSliceStop;
    /* Counter for last element in source needed by destination slice last element */
    Counter         = (NbDstSteps  * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
    SrcSliceSize    = Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION;
/*    Increment       = Counter - (SrcSliceSize << STBLIT_INCREMENT_NB_BITS_PRECISION);*/  /* It is the increment for last pixel in
                                                                                            * first slice ! useless */
    Increment       = 0; /* The first slice start always with initial offset to 0 */
    SrcSliceStart   = 0;
    SrcSliceStop    = SrcSliceStart + SrcSliceSize - 1;
    ClipStart       = DstSliceStart;
    ClipStop        = DstSliceStop;


/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Counter = %d \n", Counter));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Increment = %d \n", Increment));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstSliceStart = %d \n", DstSliceStart));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstSliceStop = %d \n", DstSliceStop));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipStart = %d \n", ClipStart));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipStop = %d \n", ClipStop));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceStart = %d \n", SrcSliceStart));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceStop = %d \n", SrcSliceStop));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceSize = %d \n\n", SrcSliceSize));*/

    /* Update first slice rectangle */
    SliceRectangle_p = Buffer_p->SliceRectangleArray;

    if (HorNotVer == TRUE) /* Horizontal slicing */
    {
        for (i=0; i < Buffer_p->VerticalSlicingNumber; i++ )
        {
            SliceRectangle_p->SrcRectangle.PositionX   = SrcSliceStart +  Buffer_p->FullSizeSrcRectangle.PositionX;
            SliceRectangle_p->SrcRectangle.Width       = SrcSliceSize;
            SliceRectangle_p->DstRectangle.PositionX   = DstSliceStart  +  Buffer_p->FullSizeDstRectangle.PositionX;
            SliceRectangle_p->DstRectangle.Width       = DstSliceStop - DstSliceStart + 1;
            SliceRectangle_p->ClipRectangle.PositionX  = ClipStart   +  Buffer_p->FullSizeDstRectangle.PositionX;
            SliceRectangle_p->ClipRectangle.Width      = ClipStop - ClipStart + 1;
            SliceRectangle_p->Reserved1                = (U16)ScalingFactor;
            SliceRectangle_p->Reserved2                = (U16)Increment;

            SliceRectangle_p++;
        }
    }
    else  /* Vertical slicing */
    {
        for (i=0; i < Buffer_p->HorizontalSlicingNumber; i++ )
        {
            SliceRectangle_p->SrcRectangle.PositionY   = SrcSliceStart + Buffer_p->FullSizeSrcRectangle.PositionY;
            SliceRectangle_p->SrcRectangle.Height      = SrcSliceSize;
            SliceRectangle_p->DstRectangle.PositionY   = DstSliceStart + Buffer_p->FullSizeDstRectangle.PositionY;
            SliceRectangle_p->DstRectangle.Height      = DstSliceStop - DstSliceStart + 1;
            SliceRectangle_p->ClipRectangle.PositionY  = ClipStart + Buffer_p->FullSizeDstRectangle.PositionY;
            SliceRectangle_p->ClipRectangle.Height     = ClipStop - ClipStart + 1;
            SliceRectangle_p->Reserved3                = (U16)ScalingFactor;
            SliceRectangle_p->Reserved4                = (U16)Increment;

            SliceRectangle_p += Buffer_p->VerticalSlicingNumber;
        }
    }

    /* Update pointer to point to first array element */
    SliceRectangle_p = Buffer_p->SliceRectangleArray;

    /* Other Slices */
    while (NoMoreSlice == FALSE)
    {
        /* Next destination slice first element */
        NbDstSteps = NbDstSteps + 1;

        /* Index of first element in source needed by next destination slice first element */
        SrcSliceStart   = ((NbDstSteps * ScalingFactor) >> STBLIT_INCREMENT_NB_BITS_PRECISION) + 3 - STBLIT_TAP_NUMBER_FILTER;

        /* Counter for last element in source needed by next destination slice first element */
        Counter         = (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        DstSliceStart   = DstSliceStop + 1;
        ClipStart       = DstSliceStart ;
        Temp            = Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION;
        while ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) > (Temp - 2))
        {
            DstSliceStart--;
            NbDstSteps--;
            Counter =  (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        }
        Increment = Counter - ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) << STBLIT_INCREMENT_NB_BITS_PRECISION);
        DstSliceStop  = DstSliceStart + DstSliceSize - 1;

        if (DstSliceStop >= (DstFullSize - 1))
        {
            DstSliceStop     = DstFullSize - 1;
            NoMoreSlice = TRUE;
        }

        NbDstSteps = DstSliceStop;

        /* Counter for last element in source needed by destination slice last element */
        Counter       = (NbDstSteps * ScalingFactor) + (3 << STBLIT_INCREMENT_NB_BITS_PRECISION);
        SrcSliceSize  = ((Counter >> STBLIT_INCREMENT_NB_BITS_PRECISION) - 1) - SrcSliceStart + 1;
        SrcSliceStop  = SrcSliceStart + SrcSliceSize - 1;

        if (SrcSliceStop > (SrcFullSize - 1))
        {
            SrcSliceStop   = SrcFullSize - 1;
            SrcSliceSize   = SrcSliceStop - SrcSliceStart + 1;
        }

        ClipStop   = DstSliceStop;

/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Counter = %d \n", Counter));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Increment = %d \n", Increment));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstSliceStart = %d \n", DstSliceStart));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstSliceStop = %d \n", DstSliceStop));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipStart = %d \n", ClipStart));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipStop = %d \n", ClipStop));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceStart = %d \n", SrcSliceStart));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceStop = %d \n", SrcSliceStop));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcSliceSize = %d \n\n", SrcSliceSize));*/


        if (HorNotVer == TRUE) /* Horizontal slicing */
        {
            SliceRectangle_p += Buffer_p->VerticalSlicingNumber;
            StartAddress = SliceRectangle_p;

            for (i=0; i < Buffer_p->VerticalSlicingNumber; i++ )
            {
                SliceRectangle_p->SrcRectangle.PositionX   = SrcSliceStart +  Buffer_p->FullSizeSrcRectangle.PositionX;
                SliceRectangle_p->SrcRectangle.Width       = SrcSliceSize;
                SliceRectangle_p->DstRectangle.PositionX   = DstSliceStart +  Buffer_p->FullSizeDstRectangle.PositionX;
                SliceRectangle_p->DstRectangle.Width       = DstSliceStop - DstSliceStart + 1;
                SliceRectangle_p->ClipRectangle.PositionX  = ClipStart +  Buffer_p->FullSizeDstRectangle.PositionX;
                SliceRectangle_p->ClipRectangle.Width      = ClipStop - ClipStart + 1;
                SliceRectangle_p->Reserved1                = (U16)ScalingFactor;
                SliceRectangle_p->Reserved2                = (U16)Increment;

                SliceRectangle_p++;
            }

            SliceRectangle_p = StartAddress;

        }
        else
        {
/*            SliceRectangle_p = SliceRectangle_p++;*/
            SliceRectangle_p++;
            StartAddress = SliceRectangle_p;

            for (i=0; i < Buffer_p->HorizontalSlicingNumber; i++ )
            {
                SliceRectangle_p->SrcRectangle.PositionY   = SrcSliceStart +  Buffer_p->FullSizeSrcRectangle.PositionY;
                SliceRectangle_p->SrcRectangle.Height      = SrcSliceSize;
                SliceRectangle_p->DstRectangle.PositionY   = DstSliceStart +  Buffer_p->FullSizeDstRectangle.PositionY;
                SliceRectangle_p->DstRectangle.Height      = DstSliceStop - DstSliceStart + 1;
                SliceRectangle_p->ClipRectangle.PositionY  = ClipStart +  Buffer_p->FullSizeDstRectangle.PositionY;
                SliceRectangle_p->ClipRectangle.Height     = ClipStop - ClipStart + 1;
                SliceRectangle_p->Reserved3                = (U16)ScalingFactor;
                SliceRectangle_p->Reserved4                = (U16)Increment;

                SliceRectangle_p += Buffer_p->VerticalSlicingNumber;
            }

            SliceRectangle_p = StartAddress;
        }
    }

    return(Err);
}

/*
--------------------------------------------------------------------------------
API Get slice buffer function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetSliceRectangleBuffer(
    STBLIT_Handle_t                  Handle,
    STBLIT_SliceRectangleBuffer_t*   Buffer_p
)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
    stblit_Unit_t*              Unit_p      = (stblit_Unit_t*) Handle;
    STBLIT_SliceRectangle_t*    SliceRectangle_p;
    U32                         ScalingFactor;
    U32                         i;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check parameter */
    if (Buffer_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if ((Buffer_p->HorizontalSlicingNumber == 0) ||
        (Buffer_p->VerticalSlicingNumber == 0) ||
        (Buffer_p->SliceRectangleArray == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check the reference slice (don't care about its position) */
    if ((Buffer_p->FixedRectangle.Width == 0) ||
        (Buffer_p->FixedRectangle.Height == 0)||
        (Buffer_p->FixedRectangle.Width > STBLIT_MAX_WIDTH) ||   /* 12 bit HW constraints */
        (Buffer_p->FixedRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check full size rectangles */
    if ((Buffer_p->FullSizeSrcRectangle.PositionX < 0) ||
        (Buffer_p->FullSizeSrcRectangle.PositionY < 0) ||
        (Buffer_p->FullSizeSrcRectangle.Width == 0)    ||
        (Buffer_p->FullSizeSrcRectangle.Height == 0)   ||
        (Buffer_p->FullSizeSrcRectangle.Width > STBLIT_MAX_WIDTH)    ||  /* 12 bit HW constraints */
        (Buffer_p->FullSizeSrcRectangle.Height > STBLIT_MAX_HEIGHT)  ||  /* 12 bit HW constraints */
        (Buffer_p->FullSizeDstRectangle.PositionX < 0) ||
        (Buffer_p->FullSizeDstRectangle.PositionY < 0) ||
        (Buffer_p->FullSizeDstRectangle.Width == 0)    ||
        (Buffer_p->FullSizeDstRectangle.Height == 0)   ||
        (Buffer_p->FullSizeDstRectangle.Width > STBLIT_MAX_WIDTH)    || /* 12 bit HW constraints */
        (Buffer_p->FullSizeDstRectangle.Height > STBLIT_MAX_HEIGHT))     /* 12 bit HW constraints */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Support only horizontal fixed dst slice(first implementation)*/
    if (Buffer_p->FixedRectangleType != STBLIT_SLICE_FIXED_RECTANGLE_TYPE_DESTINATION)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if ((Buffer_p->FixedRectangle.Width > Buffer_p->FullSizeDstRectangle.Width ) ||
        (Buffer_p->FixedRectangle.Height > Buffer_p->FullSizeDstRectangle.Height))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (Buffer_p->VerticalSlicingNumber > 1)
    {
        GetSlicingInfo (Buffer_p, FALSE);
    }
    else  /* a unique vertical slice = Full Height */
    {
        SliceRectangle_p = Buffer_p->SliceRectangleArray;
        ScalingFactor =  ((Buffer_p->FullSizeSrcRectangle.Height - 1) << STBLIT_INCREMENT_NB_BITS_PRECISION) /
                          (Buffer_p->FullSizeDstRectangle.Height - 1);

        for (i=0; i < Buffer_p->HorizontalSlicingNumber; i++ )
        {
            SliceRectangle_p->SrcRectangle.PositionY   = Buffer_p->FullSizeSrcRectangle.PositionY;
            SliceRectangle_p->SrcRectangle.Height      = Buffer_p->FullSizeSrcRectangle.Height;
            SliceRectangle_p->DstRectangle.PositionY   = Buffer_p->FullSizeDstRectangle.PositionY;
            SliceRectangle_p->DstRectangle.Height      = Buffer_p->FullSizeDstRectangle.Height;
            SliceRectangle_p->ClipRectangle.PositionY  = Buffer_p->FullSizeDstRectangle.PositionY;
            SliceRectangle_p->ClipRectangle.Height     = Buffer_p->FullSizeDstRectangle.Height;
            SliceRectangle_p->Reserved3                = (U16)ScalingFactor;
            SliceRectangle_p->Reserved4                = 0;

            SliceRectangle_p++;
        }
    }

    if (Buffer_p->HorizontalSlicingNumber > 1)
    {
        GetSlicingInfo (Buffer_p, TRUE);
    }
    else /* a unique horizontal slice = Full width */
    {
        SliceRectangle_p = Buffer_p->SliceRectangleArray;
        ScalingFactor =  ((Buffer_p->FullSizeSrcRectangle.Width - 1) << STBLIT_INCREMENT_NB_BITS_PRECISION) /
                          (Buffer_p->FullSizeDstRectangle.Width - 1);

        for (i=0; i < Buffer_p->VerticalSlicingNumber; i++ )
        {
            SliceRectangle_p->SrcRectangle.PositionX   = Buffer_p->FullSizeSrcRectangle.PositionX;
            SliceRectangle_p->SrcRectangle.Width       = Buffer_p->FullSizeSrcRectangle.Width;
            SliceRectangle_p->DstRectangle.PositionX   = Buffer_p->FullSizeDstRectangle.PositionX;
            SliceRectangle_p->DstRectangle.Width       = Buffer_p->FullSizeDstRectangle.Width;
            SliceRectangle_p->ClipRectangle.PositionX  = Buffer_p->FullSizeDstRectangle.PositionX;
            SliceRectangle_p->ClipRectangle.Width      = Buffer_p->FullSizeDstRectangle.Width;
            SliceRectangle_p->Reserved1                = (U16)ScalingFactor;
            SliceRectangle_p->Reserved2                = 0;

            SliceRectangle_p++;
        }
    }


    /* Display buffer */
/*    SliceRectangle_p = Buffer_p->SliceRectangleArray;*/
/*    for (i= 0;i<(Buffer_p->HorizontalSlicingNumber * Buffer_p->VerticalSlicingNumber) ;i++ )*/
/*    {*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcRectangle.PositionX = %d \n", SliceRectangle_p->SrcRectangle.PositionX));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcRectangle.PositionY = %d \n", SliceRectangle_p->SrcRectangle.PositionY));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcRectangle.Width = %d \n", SliceRectangle_p->SrcRectangle.Width));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SrcRectangle.Height = %d \n", SliceRectangle_p->SrcRectangle.Height));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstRectangle.PositionX = %d \n", SliceRectangle_p->DstRectangle.PositionX));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstRectangle.PositionY = %d \n", SliceRectangle_p->DstRectangle.PositionY));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstRectangle.Width = %d \n", SliceRectangle_p->DstRectangle.Width));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "DstRectangle.Height = %d \n", SliceRectangle_p->DstRectangle.Height));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipRectangle.PositionX = %d \n", SliceRectangle_p->ClipRectangle.PositionX));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipRectangle.PositionY = %d \n", SliceRectangle_p->ClipRectangle.PositionY));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipRectangle.Width = %d \n", SliceRectangle_p->ClipRectangle.Width));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "ClipRectangle.Height = %d \n", SliceRectangle_p->ClipRectangle.Height));*/

/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Hor SF = %d \n", SliceRectangle_p->Reserved1));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Hor Increment = %d \n", SliceRectangle_p->Reserved2));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ver SF = %d \n", SliceRectangle_p->Reserved3));*/
/*        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ver Increment = %d \n\n", SliceRectangle_p->Reserved4));*/

/*        SliceRectangle_p++;*/
/*    }*/


    return(Err);
}

/*
--------------------------------------------------------------------------------
API Blit slice rectangle function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_BlitSliceRectangle(
    STBLIT_Handle_t           Handle,
    STBLIT_SliceData_t*       Src1_p,
    STBLIT_SliceData_t*       Src2_p,
    STBLIT_SliceData_t*       Dst_p,
    STBLIT_SliceRectangle_t*  SliceRectangle_p,
    STBLIT_BlitContext_t*     BlitContext_p
)
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;
    stblit_Device_t*        Device_p;
    stblit_Job_t*           Job_p;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = Unit_p->Device_p ;

    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check parameters */
    if ((BlitContext_p == NULL)                ||
        ((Src1_p == NULL) && (Src2_p == NULL)) ||
        (Dst_p == NULL) ||
        (SliceRectangle_p == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check Blit context  */
    if (BlitContext_p->JobHandle != STBLIT_NO_JOB_HANDLE)
    {
        Job_p = (stblit_Job_t*) BlitContext_p->JobHandle;

        /* Check Job Validity */
        if (Job_p->JobValidity != STBLIT_VALID_JOB)
        {
            return(STBLIT_ERROR_INVALID_JOB_HANDLE);
        }

        /* Check Handles mismatch */
        if (Job_p->Handle != Handle)
        {
            return(STBLIT_ERROR_JOB_HANDLE_MISMATCH);
        }

        /* Check if Job is closed */
        if (Job_p->Closed == TRUE)
        {
            /* Job already submitted. No change permitted */
            return(STBLIT_ERROR_JOB_CLOSED);
        }
    }

    /* Check Mask Size if any (Mask Size = Foreground src)          TBDone */

    if (Src2_p == NULL)   /* 1 source (src1) */
    {
        Err = OneInputBlitSliceRectangle(Device_p, Src1_p, Dst_p,SliceRectangle_p, BlitContext_p);
    }
    else if (Src1_p == NULL)   /* 1 source (src2) */
    {
        Err = OneInputBlitSliceRectangle(Device_p, Src2_p, Dst_p,SliceRectangle_p, BlitContext_p);
    }
    else   /* 2 sources  */
    {
        Err = TwoInputBlitSliceRectangle(Device_p, Src1_p, Src2_p,  Dst_p,SliceRectangle_p, BlitContext_p);
    }

    return(Err);
}

/*
--------------------------------------------------------------------------------
API Blit Get slice rectangle number function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetSliceRectangleNumber(
    STBLIT_Handle_t                  Handle,
    STBLIT_SliceRectangleBuffer_t*   Buffer_p
)
{
    ST_ErrorCode_t          Err = ST_NO_ERROR;
    stblit_Unit_t*          Unit_p      = (stblit_Unit_t*) Handle;

    /* Check Handle validity */
    if (Handle == (STBLIT_Handle_t)NULL)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Unit_p->UnitValidity != STBLIT_VALID_UNIT)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Check parameter */
    if (Buffer_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check the reference slice (don't care about its position) */
    if ((Buffer_p->FixedRectangle.Width == 0) ||
        (Buffer_p->FixedRectangle.Height == 0)||
        (Buffer_p->FixedRectangle.Width > STBLIT_MAX_WIDTH) ||   /* 12 bit HW constraints */
        (Buffer_p->FixedRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check full size rectangles */
    if ((Buffer_p->FullSizeSrcRectangle.PositionX < 0) ||
        (Buffer_p->FullSizeSrcRectangle.PositionY < 0) ||
        (Buffer_p->FullSizeSrcRectangle.Width == 0)    ||
        (Buffer_p->FullSizeSrcRectangle.Height == 0)   ||
        (Buffer_p->FullSizeSrcRectangle.Width > STBLIT_MAX_WIDTH)    ||  /* 12 bit HW constraints */
        (Buffer_p->FullSizeSrcRectangle.Height > STBLIT_MAX_HEIGHT)  ||  /* 12 bit HW constraints */
        (Buffer_p->FullSizeDstRectangle.PositionX < 0) ||
        (Buffer_p->FullSizeDstRectangle.PositionY < 0) ||
        (Buffer_p->FullSizeDstRectangle.Width == 0)    ||
        (Buffer_p->FullSizeDstRectangle.Height == 0)   ||
        (Buffer_p->FullSizeDstRectangle.Width > STBLIT_MAX_WIDTH)    || /* 12 bit HW constraints */
        (Buffer_p->FullSizeDstRectangle.Height > STBLIT_MAX_HEIGHT))     /* 12 bit HW constraints */
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Only Slice with fixed destination supported */
    if (Buffer_p->FixedRectangleType != STBLIT_SLICE_FIXED_RECTANGLE_TYPE_DESTINATION)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }


    if (Buffer_p->FixedRectangle.Width != Buffer_p->FullSizeDstRectangle.Width )
    {
        GetSliceNumber (Buffer_p, TRUE);
    }
    else
    {
        Buffer_p->HorizontalSlicingNumber = 1;
    }
    if (Buffer_p->FixedRectangle.Height != Buffer_p->FullSizeDstRectangle.Height)
    {
        GetSliceNumber (Buffer_p, FALSE);
    }
    else
    {
        Buffer_p->VerticalSlicingNumber = 1;
    }

/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Hor Slice Number = %d \n", Buffer_p->HorizontalSlicingNumber));*/
/*    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Ver Slice Number = %d \n", Buffer_p->VerticalSlicingNumber));*/

    return(Err);
}

/*******************************************************************************
Name        : OneInputBlitSliceRectangle
Description : 1 input / 1 output blit slice operation
Parameters  :
Assumptions : All parameters not null
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t OneInputBlitSliceRectangle(stblit_Device_t*           Device_p,
                                                STBLIT_SliceData_t*         Src_p,
                                                STBLIT_SliceData_t*         Dst_p,
                                                STBLIT_SliceRectangle_t*    SliceRectangle_p,
                                                STBLIT_BlitContext_t*       BlitContext_p)
 {
    ST_ErrorCode_t          Err             = ST_NO_ERROR;
    STGXOBJ_ColorType_t     DstColorType    = Dst_p->Bitmap_p->ColorType;
    STGXOBJ_ColorType_t     SrcColorType    = Src_p->Bitmap_p->ColorType;
    U8                      SrcIndex ,DstIndex, TmpIndex;
    U16                     Code;
    U16                     CodeMask = 0;
    STBLIT_SliceData_t      Src1;
    STBLIT_SliceData_t*     Src1_p          = &Src1;
    STBLIT_SliceData_t*     Src2_p          = Src_p;
    STBLIT_SliceData_t*     MaskSrc1_p      = Src1_p;
    STBLIT_SliceData_t*     MaskSrc2_p      = Src2_p;
    STBLIT_SliceData_t      Mask;
    STBLIT_SliceData_t      SrcTmp;
    STBLIT_SliceData_t      DstTmp;
    STGXOBJ_Palette_t       MaskPalette;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    STGXOBJ_Bitmap_t        WorkBitmap;
    STGXOBJ_ColorType_t     TmpColorType;
    U8                      TmpNbBitPerPixel;
    U32                     TmpBitWidth;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_CommonField_t                FirstNodeCommonField ;
    stblit_CommonField_t                LastNodeCommonField;
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
    BOOL                    IsVResizeOrFF = FALSE;
#endif

    /* MB not supported for slice blit */
    if ((Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) ||
        (Src_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB))
    {
        /*  not supported */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Dst settings */
    if ((SliceRectangle_p->DstRectangle.Width == 0)    ||
        (SliceRectangle_p->DstRectangle.Height == 0)   ||
        (SliceRectangle_p->DstRectangle.Width > STBLIT_MAX_WIDTH) ||  /* 12 bit HW constraints */
        (SliceRectangle_p->DstRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Dst_p->UseSliceRectanglePosition == TRUE)
    {
        if ((SliceRectangle_p->DstRectangle.PositionX < 0) ||
            (SliceRectangle_p->DstRectangle.PositionY < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((SliceRectangle_p->DstRectangle.PositionX + SliceRectangle_p->DstRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.PositionY + SliceRectangle_p->DstRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else /* Rectangle position ignored : Start in (O,O) */
    {
        if (((SliceRectangle_p->DstRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Dst_p->Bitmap_p->ColorType, SliceRectangle_p->DstRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif


    /* Check Src settings */
    if ((SliceRectangle_p->SrcRectangle.Width == 0)    ||
        (SliceRectangle_p->SrcRectangle.Height == 0)   ||
        (SliceRectangle_p->SrcRectangle.Width > STBLIT_MAX_WIDTH) ||  /* 12 bit HW constraints */
        (SliceRectangle_p->SrcRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Src_p->UseSliceRectanglePosition == TRUE)
    {
        if ((SliceRectangle_p->SrcRectangle.PositionX < 0) ||
            (SliceRectangle_p->SrcRectangle.PositionY < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((SliceRectangle_p->SrcRectangle.PositionX + SliceRectangle_p->SrcRectangle.Width -1) > (Src_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->SrcRectangle.PositionY + SliceRectangle_p->SrcRectangle.Height -1) > (Src_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else
    {
        if (((SliceRectangle_p->SrcRectangle.Width -1) > (Src_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->SrcRectangle.Height -1) > (Src_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Src_p->Bitmap_p->ColorType, SliceRectangle_p->SrcRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
#ifdef WA_HORIZONTAL_POSITION_ODD
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SliceRectangle_p->SrcRectangle.PositionX % 2) == 0) &&
        ((Src_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    ||
         (Src_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)  ||
         (Src_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)    ||
         (Src_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)) &&
        ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE))                                    &&
        (SliceRectangle_p->SrcRectangle.Width >= 128 ))
    {
        /* If PositionX is modified, it impacts the overall slice management. Dst slice to ve recalculated also!*/
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

#ifdef WA_GNBvd06658
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (SliceRectangle_p->SrcRectangle.Width >= 128 ))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SliceRectangle_p->SrcRectangle.Width >= 128 ))
        {
            IsVResizeOrFF = TRUE;
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
      if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
           (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SliceRectangle_p->SrcRectangle.Width != SliceRectangle_p->DstRectangle.Width) &&
          (SliceRectangle_p->SrcRectangle.Width >= 128)  &&
          ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
           (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)))
      {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);;
      }
#endif
#ifdef WA_GNBvd13838
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE)) &&
        (SliceRectangle_p->SrcRectangle.Width != SliceRectangle_p->DstRectangle.Width) &&
        (SliceRectangle_p->SrcRectangle.Width >= 128)  &&
        (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1))
      {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);;
      }
#endif


    /* Get src and Dst indexes */
    stblit_GetIndex(SrcColorType, &SrcIndex,Src_p->Bitmap_p->ColorSpaceConversion);
    stblit_GetIndex(DstColorType, &DstIndex,Dst_p->Bitmap_p->ColorSpaceConversion);


    /* Default values for Src1 : => Dst */
    Src1.UseSliceRectanglePosition  = Dst_p->UseSliceRectanglePosition;
    Src1.Bitmap_p                   = Dst_p->Bitmap_p;
    Src1.Palette_p                  = Dst_p->Palette_p;

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* If Err = ST_NO_ERROR, CodeMask is always supported. No need to check the last bit */
        if (stblit_GetTmpIndex(SrcIndex,&TmpIndex,&TmpColorType, &TmpNbBitPerPixel) != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        CodeMask = stblit_TableSingleSrc1[SrcIndex][TmpIndex];
        /* Code Mask does not have any CLUT operation enabled because TableSingleSrc1 does not set anything on Src2 h/w flow.
         * => CodeMask 2nd bit is always 0 (Clut disable).
         * However in case of Alpha4 mask, a color expansion has to be done in Src2 h/w flow in order to make it Alpha8, i.e a
         * CLUT expansion operation has to be enabled.
         * => We just set Clut expansion code in CodeMask  */
        if (BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
        {
            CodeMask |= 0x2;
        }
        SrcIndex = TmpIndex;
    }

    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
    {
        /* Blend mode */
        Code = stblit_TableBlend[SrcIndex][DstIndex];
    }
    else if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT)) &&
             (BlitContext_p->EnableMaskBitmap == FALSE)&&
             (BlitContext_p->EnableMaskWord == FALSE) &&
             (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Single Src2 */
        Code   = stblit_TableSingleSrc2[SrcIndex][DstIndex];
        Src1_p = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Single Src1 */
        Code   = stblit_TableSingleSrc1[DstIndex][DstIndex];   /* Src1 == Dst  */
        Src2_p = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_CLEAR)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_SET))
    {
        /* Only Dst is checked */
        /* Code = TableSet[DstIndex]; */
        Code = 0;
    }
    else /* (All other ROP) Or (COPY/COPY_INVERT with mask)*/
    {
        /* ROP */
        Code = stblit_TableROP[SrcIndex][DstIndex];
    }


    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Initialize CommonField values */
    memset(&FirstNodeCommonField, 0, sizeof(stblit_CommonField_t));
    memset(&LastNodeCommonField, 0, sizeof(stblit_CommonField_t));

    if (BlitContext_p->EnableMaskBitmap == FALSE)
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if ((IsVResizeOrFF == TRUE)  &&
            (Src1_p != NULL))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p, Src1_p, Src2_p, Dst_p, SliceRectangle_p, BlitContext_p, Code,
                                                        STBLIT_NO_MASK, &NodeFirstHandle, &OperationCfg, &FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        NodeLastHandle  = NodeFirstHandle;
        NumberNodes     = 1;

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
    }
    else   /* 2 passes for mask */
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (IsVResizeOrFF == TRUE)  /* Src1 is always non NULL in Mask mode (concerns 2nd pass)*/
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        /* Set MaskPalette : Palette used in case of Alpha4 to Alpha8 conversion */
        MaskPalette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
        MaskPalette.Data_p      = (void*) Device_p->Alpha4To8Palette_p;
/*        MaskPalette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;*/
/*        MaskPalette.ColorDepth  = ;    */

        /* Set Mask source */
        Mask.Bitmap_p                   = BlitContext_p->MaskBitmap_p;
        Mask.UseSliceRectanglePosition  = MaskSrc2_p->UseSliceRectanglePosition;
        Mask.Palette_p                  = &MaskPalette;

        /* Set Work Bitmap fields : TBD (Pitch, Alignment)!!!!*/
        WorkBitmap.ColorType            = TmpColorType;
        WorkBitmap.BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        WorkBitmap.PreMultipliedColor   = FALSE ;                                 /* TBD (Src2 params?)*/
        WorkBitmap.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;                    /* TBD (Src2 params?)*/
        WorkBitmap.AspectRatio          = STGXOBJ_ASPECT_RATIO_SQUARE;            /* TBD (Src2 params?)*/
        /* Use Mask dimensions */
        WorkBitmap.Width                = SliceRectangle_p->SrcRectangle.Width;
        WorkBitmap.Height               = SliceRectangle_p->SrcRectangle.Height;
        TmpBitWidth                     = WorkBitmap.Width * TmpNbBitPerPixel;
        if ((TmpBitWidth % 8) != 0)
        {
            WorkBitmap.Pitch            = (TmpBitWidth / 8) + 1;
        }
        else
        {
            WorkBitmap.Pitch            = TmpBitWidth / 8;
        }
        WorkBitmap.Data1_p              = BlitContext_p->WorkBuffer_p;
        WorkBitmap.Offset               = 0;

        /* Temporary dst setting */
        DstTmp.Bitmap_p                   = &WorkBitmap;
        DstTmp.Palette_p                  = MaskSrc2_p->Palette_p;
        DstTmp.UseSliceRectanglePosition  = MaskSrc2_p->UseSliceRectanglePosition;

        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p, MaskSrc2_p, &Mask, &DstTmp, SliceRectangle_p,BlitContext_p, CodeMask,
                                                        STBLIT_MASK_FIRST_PASS, &NodeFirstHandle, &OperationCfg, &FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }


        SrcTmp.Bitmap_p                     = DstTmp.Bitmap_p;
        SrcTmp.Palette_p                    = DstTmp.Palette_p;
        SrcTmp.UseSliceRectanglePosition    = DstTmp.UseSliceRectanglePosition;

        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p, MaskSrc1_p, &SrcTmp, Dst_p, SliceRectangle_p, BlitContext_p, Code,
                                                        STBLIT_MASK_SECOND_PASS, &NodeLastHandle, &OperationCfg, &LastNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Release First node which has been sucessfully created previously */
            STOS_SemaphoreWait(Device_p->AccessControl);
            stblit_ReleaseNode(Device_p,NodeFirstHandle);
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }
        NumberNodes = 2;

        /* Link Two nodes together */
        stblit_Connect2Nodes(Device_p, (stblit_Node_t*)NodeFirstHandle,(stblit_Node_t*)NodeLastHandle);

        /* Remove Blit completion interruption for the first node :
         * In this case both nodes are considered as atomic. Never interruption between.
         * HAS TO BE DONE BEFORE WORKAROUNDS IT if any !!!
         * At this stage, the only possible IT is for blit notification
         * Note that there is no lock IT, unlock IT, submission IT, workaround IT nor job notification IT  , yet at this stage !! */
        ((stblit_Node_t*)NodeFirstHandle)->ITOpcode &= (U32)~STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        FirstNodeCommonField.INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));

#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))

        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on first node */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

            ((stblit_Node_t*)NodeLastHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on last node */
            LastNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeLastHandle ,&LastNodeCommonField);
    }

#ifdef STBLIT_TEST_FRONTEND
    /* Simulator dump */
    /* NodeDumpFile((U32*)NodeFirstHandle , sizeof(stblit_Node_t), "DumpFirstNode");
    NodeDumpFile((U32*)NodeLastHandle , sizeof(stblit_Node_t), "DumpLastNode");  */

    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit*/
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end.*/
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);
 }


/*******************************************************************************
Name        : TwoInputBlitSliceRectangle
Description : 2 input / 1 output blit slice operation
Parameters  :
Assumptions :  All parameters not null
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t TwoInputBlitSliceRectangle(stblit_Device_t*           Device_p,
                                                 STBLIT_SliceData_t*        Src1_p,
                                                 STBLIT_SliceData_t*        Src2_p,
                                                 STBLIT_SliceData_t*        Dst_p,
                                                 STBLIT_SliceRectangle_t*   SliceRectangle_p,
                                                 STBLIT_BlitContext_t*      BlitContext_p)
 {
    ST_ErrorCode_t          Err             = ST_NO_ERROR;
    STGXOBJ_ColorType_t     DstColorType    = Dst_p->Bitmap_p->ColorType;
    STGXOBJ_ColorType_t     Src1ColorType   = Src1_p->Bitmap_p->ColorType;
    STGXOBJ_ColorType_t     Src2ColorType   = Src2_p->Bitmap_p->ColorType;
    U8                      Src2Index, DstIndex, TmpIndex;
    U16                     Code;
    U16                     CodeMask = 0;
    STBLIT_SliceData_t*     MaskSrc1_p      = Src1_p;
    STBLIT_SliceData_t*     MaskSrc2_p      = Src2_p;
    STBLIT_SliceData_t      Mask;
    STBLIT_SliceData_t      SrcTmp;
    STBLIT_SliceData_t      DstTmp;
    STGXOBJ_Palette_t       MaskPalette;
    U8                      NumberNodes;
    stblit_NodeHandle_t     NodeFirstHandle, NodeLastHandle;
    STGXOBJ_Bitmap_t        WorkBitmap;
    STGXOBJ_ColorType_t     TmpColorType;
    U8                      TmpNbBitPerPixel;
    U32                     TmpBitWidth;
    stblit_OperationConfiguration_t     OperationCfg;
    stblit_CommonField_t                FirstNodeCommonField;
    stblit_CommonField_t                LastNodeCommonField ;
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
    BOOL                    IsVResizeOrFF = FALSE;
#endif

    /* MB not supported for slices */
    if ((Dst_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) ||
       (Src1_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB) ||
       (Src2_p->Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB))
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Check Src1/Dst settings (both have got the same slice rectangle)*/
    if ((SliceRectangle_p->DstRectangle.Width == 0)    ||
        (SliceRectangle_p->DstRectangle.Height == 0)   ||
        (SliceRectangle_p->DstRectangle.Width > STBLIT_MAX_WIDTH) ||  /* 12 bit HW constraints */
        (SliceRectangle_p->DstRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Dst_p->UseSliceRectanglePosition == TRUE)
    {
        if ((SliceRectangle_p->DstRectangle.PositionX < 0) ||
            (SliceRectangle_p->DstRectangle.PositionY < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((SliceRectangle_p->DstRectangle.PositionX + SliceRectangle_p->DstRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.PositionY + SliceRectangle_p->DstRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else /* Rectangle position ignored : Start in (O,O) */
    {
        if (((SliceRectangle_p->DstRectangle.Width -1) > (Dst_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.Height -1) > (Dst_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }

    if (Src1_p->UseSliceRectanglePosition == TRUE)
    {
        if ((SliceRectangle_p->DstRectangle.PositionX < 0) ||
            (SliceRectangle_p->DstRectangle.PositionY < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((SliceRectangle_p->DstRectangle.PositionX + SliceRectangle_p->DstRectangle.Width -1) > (Src1_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.PositionY + SliceRectangle_p->DstRectangle.Height -1) > (Src1_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else /* Rectangle position ignored : Start in (O,O) */
    {
        if (((SliceRectangle_p->DstRectangle.Width -1) > (Src1_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->DstRectangle.Height -1) > (Src1_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Src1_p->Bitmap_p->ColorType, SliceRectangle_p->DstRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    Err = stblit_WA_CheckWidthByteLimitation(Dst_p->Bitmap_p->ColorType, SliceRectangle_p->DstRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

    /* Check Src2 settings */
    if ((SliceRectangle_p->SrcRectangle.Width == 0)    ||
        (SliceRectangle_p->SrcRectangle.Height == 0)   ||
        (SliceRectangle_p->SrcRectangle.Width > STBLIT_MAX_WIDTH) ||   /* 12 bit HW constraints */
        (SliceRectangle_p->SrcRectangle.Height > STBLIT_MAX_HEIGHT))   /* 12 bit HW constraints */
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (Src2_p->UseSliceRectanglePosition == TRUE)
    {
        if ((SliceRectangle_p->SrcRectangle.PositionX < 0) ||
            (SliceRectangle_p->SrcRectangle.PositionY < 0))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        if (((SliceRectangle_p->SrcRectangle.PositionX + SliceRectangle_p->SrcRectangle.Width -1) > (Src2_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->SrcRectangle.PositionY + SliceRectangle_p->SrcRectangle.Height -1) > (Src2_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
    else
    {
        if (((SliceRectangle_p->SrcRectangle.Width -1) > (Src2_p->Bitmap_p->Width - 1)) ||
            ((SliceRectangle_p->SrcRectangle.Height -1) > (Src2_p->Bitmap_p->Height - 1)))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }
#ifdef WA_WIDTH_8176_BYTES_LIMITATION
    Err = stblit_WA_CheckWidthByteLimitation(Src2_p->Bitmap_p->ColorType, SliceRectangle_p->SrcRectangle.Width);
    if (Err != ST_NO_ERROR)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif
#ifdef WA_HORIZONTAL_POSITION_ODD
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SliceRectangle_p->SrcRectangle.PositionX % 2) == 0) &&
        ((Src2_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422)    ||
         (Src2_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)  ||
         (Src2_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420)    ||
         (Src2_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420)) &&
        ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE))                              &&
        (SliceRectangle_p->SrcRectangle.Width >= 128 ))
    {
        /* If PositionX is modified, it impacts the overall slice management. Dst slice to ve recalculated also!*/
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
#endif

#ifdef WA_GNBvd06658
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
            ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
             (BlitContext_p->EnableFlickerFilter == TRUE)) &&
            (BlitContext_p->EnableClipRectangle == TRUE)&&
            (SliceRectangle_p->SrcRectangle.Width >= 128 ))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
             (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
          ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
           (BlitContext_p->EnableFlickerFilter == TRUE)) &&
          (SliceRectangle_p->SrcRectangle.Width >= 128 ))
        {
            IsVResizeOrFF = TRUE;
        }
#endif
#ifdef WA_VRESIZE_OR_FLICKER_AND_YCRCB_TARGET
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE)) &&
        (SliceRectangle_p->SrcRectangle.Width != SliceRectangle_p->DstRectangle.Width) &&
        (SliceRectangle_p->SrcRectangle.Width >= 128) &&
        ((Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422) ||
         (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422)))
    {
          return(ST_ERROR_FEATURE_NOT_SUPPORTED);;
    }
#endif
#ifdef WA_GNBvd13838
    if (((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
         (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)) &&
        ((SliceRectangle_p->SrcRectangle.Height != SliceRectangle_p->DstRectangle.Height) ||
         (BlitContext_p->EnableFlickerFilter == TRUE)) &&
        (SliceRectangle_p->SrcRectangle.Width != SliceRectangle_p->DstRectangle.Width) &&
        (SliceRectangle_p->SrcRectangle.Width >= 128) &&
        (Dst_p->Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_CLUT1))
    {
          return(ST_ERROR_FEATURE_NOT_SUPPORTED);;
    }
#endif

    /* Get Src and Dst indesxes */
    stblit_GetIndex(Src2ColorType, &Src2Index,Src2_p->Bitmap_p->ColorSpaceConversion);
    stblit_GetIndex(DstColorType, &DstIndex,Dst_p->Bitmap_p->ColorSpaceConversion );

    /* The driver only support Src1 Color Format = Dst Color format */
    if (Src1ColorType != DstColorType)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    if (BlitContext_p->EnableMaskBitmap == TRUE)
    {
        /* If Err = ST_NO_ERROR, CodeMask is always supported. No need to check the last bit */
        if (stblit_GetTmpIndex(Src2Index,&TmpIndex,&TmpColorType, &TmpNbBitPerPixel) != ST_NO_ERROR)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
        CodeMask = stblit_TableSingleSrc1[Src2Index][TmpIndex]; /* Src2 foreground placed on Src1 HW pipe */
        /* Code Mask does not have any CLUT operation enabled because TableSingleSrc1 does not set anything on Src2 h/w flow.
         * => CodeMask 2nd bit is always 0 (Clut disable).
         * However in case of Alpha4 mask, a color expansion has to be done in Src2 h/w flow in order to make it Alpha8, i.e a
         * CLUT expansion operation has to be enabled.
         * => We just set Clut expansion code in CodeMask  */
        if (BlitContext_p->MaskBitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ALPHA4)
        {
            CodeMask |= 0x2;
        }
        Src2Index = TmpIndex;
    }

    if (BlitContext_p->AluMode == STBLIT_ALU_ALPHA_BLEND)
    {
        /* Blend mode */
         Code = stblit_TableBlend[Src2Index][DstIndex];
    }
    else if (((BlitContext_p->AluMode == STBLIT_ALU_COPY) || (BlitContext_p->AluMode == STBLIT_ALU_COPY_INVERT)) &&
              (BlitContext_p->EnableMaskBitmap == FALSE)&&
              (BlitContext_p->EnableMaskWord == FALSE)&&
              (BlitContext_p->ColorKeyCopyMode != STBLIT_COLOR_KEY_MODE_DST))
    {
        /* Single Src2 */
        Code    = stblit_TableSingleSrc2[Src2Index][DstIndex];
        Src1_p  = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_NOOP)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_INVERT))
    {
        /* Single Src1 */
        Code    = stblit_TableSingleSrc1[DstIndex][DstIndex];  /* Src1 Color format == Dst Color Format
                                                                * because same color format constraint for the moment! */
        Src2_p  = NULL;
    }
    else if ((BlitContext_p->AluMode == STBLIT_ALU_CLEAR)  ||
             (BlitContext_p->AluMode == STBLIT_ALU_SET))
    {
        /* Only Dst is checked */
        /* Code = TableSet[DstIndex]; */
        Code = 0;
    }
    else /* (All other ROP) Or (COPY/COPY_INVERT with mask)*/
    {
        /* ROP */
        Code = stblit_TableROP[Src2Index][DstIndex];
    }
    if (((Code >> 15) == 1) ||
        ((((Code >> 14) & 0x1) == 1) && (BlitContext_p->EnableColorCorrection == FALSE)))
    {
        /* Conversion implies multipass or not supported by hardware */
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* Initialize CommonField values */
    memset(&FirstNodeCommonField, 0, sizeof(stblit_CommonField_t));
    memset(&LastNodeCommonField, 0, sizeof(stblit_CommonField_t));

    if (BlitContext_p->EnableMaskBitmap == FALSE)
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if ((IsVResizeOrFF == TRUE)  &&
            (Src1_p != NULL))
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif
        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p,Src1_p, Src2_p, Dst_p, SliceRectangle_p, BlitContext_p, Code,
                                      STBLIT_NO_MASK, &NodeFirstHandle, &OperationCfg,&FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }
#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        NodeLastHandle  = NodeFirstHandle;
        NumberNodes     = 1;

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
    }
    else
    {
#ifdef WA_VRESIZE_OR_FLICKER_AND_SRC1
        if (IsVResizeOrFF == TRUE)  /* Src1 is always non NULL in Mask mode (concerns 2nd pass) */
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
#endif

        /* Set MaskPalette : Palette used in case of Alpha4 to Alpha8 conversion */
        MaskPalette.ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
        MaskPalette.Data_p      = (void*) Device_p->Alpha4To8Palette_p;
/*        MaskPalette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;*/
/*        MaskPalette.ColorDepth  = ;    */

        /* Set Mask Source */
        Mask.Bitmap_p                  = BlitContext_p->MaskBitmap_p;
        Mask.UseSliceRectanglePosition = MaskSrc2_p->UseSliceRectanglePosition;
        Mask.Palette_p                 = &MaskPalette;

        /* Set Work Bitmap fields : TBD (Pitch, Alignment)!!!!*/
        WorkBitmap.ColorType            = TmpColorType;
        WorkBitmap.BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        WorkBitmap.PreMultipliedColor   = FALSE ;                                 /* TBD (Src2 params?)*/
        WorkBitmap.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;                    /* TBD (Src2 params?)*/
        WorkBitmap.AspectRatio          = STGXOBJ_ASPECT_RATIO_SQUARE;            /* TBD (Src2 params?)*/
        /* Use Mask dimensions */
        WorkBitmap.Width                = SliceRectangle_p->SrcRectangle.Width;
        WorkBitmap.Height               = SliceRectangle_p->SrcRectangle.Height;
        TmpBitWidth                     = WorkBitmap.Width * TmpNbBitPerPixel;
        if ((TmpBitWidth % 8) != 0)
        {
            WorkBitmap.Pitch            = (TmpBitWidth / 8) + 1;
        }
        else
        {
            WorkBitmap.Pitch            = TmpBitWidth / 8;
        }
        WorkBitmap.Data1_p               = BlitContext_p->WorkBuffer_p;
        WorkBitmap.Offset               = 0;

        /* Temporary dst setting */
        DstTmp.Bitmap_p             = &WorkBitmap;
        DstTmp.Palette_p            = MaskSrc2_p->Palette_p;
        DstTmp.UseSliceRectanglePosition  = MaskSrc2_p->UseSliceRectanglePosition;

        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p, MaskSrc2_p, &Mask, &DstTmp, SliceRectangle_p, BlitContext_p, CodeMask,
                                      STBLIT_MASK_FIRST_PASS, &NodeFirstHandle, &OperationCfg, &FirstNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            return(Err);
        }

        SrcTmp.Bitmap_p                   = DstTmp.Bitmap_p;
        SrcTmp.Palette_p                  = DstTmp.Palette_p;
        SrcTmp.UseSliceRectanglePosition  = MaskSrc2_p->UseSliceRectanglePosition;

        Err = stblit_OnePassBlitSliceRectangleOperation(Device_p, MaskSrc1_p, &SrcTmp, Dst_p,SliceRectangle_p,BlitContext_p, Code,
                                      STBLIT_MASK_SECOND_PASS, &NodeLastHandle, &OperationCfg, &LastNodeCommonField);
        if (Err != ST_NO_ERROR)
        {
            /* Release First node which has been sucessfully created previously */
            STOS_SemaphoreWait(Device_p->AccessControl);
            stblit_ReleaseNode(Device_p, NodeFirstHandle);
            STOS_SemaphoreSignal(Device_p->AccessControl);

            return(Err);
        }
        NumberNodes = 2;

        /* Link Two nodes together */
        stblit_Connect2Nodes(Device_p, (stblit_Node_t*)NodeFirstHandle,(stblit_Node_t*)NodeLastHandle);

        /* Remove Blit completion interruption for the first node :
         * In this case both nodes are considered as atomic. Never interruption between.
         * HAS TO BE DONE BEFORE WORKAROUNDS IT if any !!!
         * At this stage, the only possible IT is for blit notification
         * Note that there is no lock IT, unlock IT, submission IT, workaround IT nor job notification IT  , yet at this stage !! */
        ((stblit_Node_t*)NodeFirstHandle)->ITOpcode &= (U32)~STBLIT_IT_OPCODE_BLIT_COMPLETION_MASK;
        FirstNodeCommonField.INS &= ((U32)(~((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED) << STBLIT_INS_ENABLE_IRQ_SHIFT)));

#ifdef WA_SOFT_RESET_EACH_NODE
        if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100) ||
            (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
        {
            ((stblit_Node_t*)NodeFirstHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on first node */
            FirstNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);

            ((stblit_Node_t*)NodeLastHandle)->ITOpcode |= STBLIT_IT_OPCODE_SW_WORKAROUND_MASK;
            /* Set Blit completion IT on last node */
            LastNodeCommonField.INS |= ((U32)(STBLIT_INS_ENABLE_IRQ_COMPLETED & STBLIT_INS_ENABLE_IRQ_MASK) << STBLIT_INS_ENABLE_IRQ_SHIFT);
        }
#endif

        /* Update CommonField in node*/
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeFirstHandle ,&FirstNodeCommonField);
        stblit_WriteCommonFieldInNode((stblit_Node_t*)NodeLastHandle ,&LastNodeCommonField);
    }

#ifdef STBLIT_TEST_FRONTEND
    /* Simulator dump */
    /* NodeDumpFile((U32*)NodeFirstHandle , sizeof(stblit_Node_t), "DumpFirstNode");
    NodeDumpFile((U32*)NodeLastHandle , sizeof(stblit_Node_t), "DumpLastNode"); */
    STBLIT_FirstNodeAddress = (U32)NodeFirstHandle;
#endif

    /* Add Node list into Job if needed or submit to back-end single blit */
    if (BlitContext_p->JobHandle == STBLIT_NO_JOB_HANDLE) /* Single Blit -> Automatical submission to the back-end. */
    {
#ifndef STBLIT_TEST_FRONTEND
        stblit_PostSubmitMessage(Device_p,(void*)NodeFirstHandle, (void*)NodeLastHandle, FALSE, FALSE);
#endif
    }
    else   /* Job Blit */
    {
        stblit_AddNodeListInJob(Device_p,BlitContext_p->JobHandle,NodeFirstHandle,NodeLastHandle, NumberNodes);
        stblit_AddJBlitInJob (Device_p, BlitContext_p->JobHandle, NodeFirstHandle, NodeLastHandle, NumberNodes, &OperationCfg);
    }

    return(Err);
 }



/* End of blt_slc.c */

