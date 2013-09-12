/*******************************************************************************

File name   : vid_ctcm.c

Description : Common things commands source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
27 Jan 2003        Optimisation and secure interrupt lock            HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>
#endif

#include "stddefs.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */

#include "stvid.h"
#include "vid_ctcm.h"


/* Private Types ------------------------------------------------------------ */

typedef const struct
{
    U32 SizeOfParams;
} ControllerCommand_t;


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private function prototypes ---------------------------------------------- */


/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : vidcom_PictureInfosToPictureParams
Description : Translate picture infos to old structure picture params
Parameters  : pointers to structures to translate from/to
Assumptions : Both pointer are not NULL
Limitations : none
Returns     : Nothing
*******************************************************************************/
void vidcom_PictureInfosToPictureParams(const STVID_PictureInfos_t * const PictureInfos_p, STVID_PictureParams_t * const Params_p, const S32 TemporalReference)
{
    switch (PictureInfos_p->BitmapParams.BitmapType)
    {
        case STGXOBJ_BITMAP_TYPE_MB :
        default :
            Params_p->CodingMode = STVID_CODING_MODE_MB;
            break;
    }
    switch (PictureInfos_p->BitmapParams.ColorType)
    {
        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_422 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422 :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV422;
            break;

        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_444 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444 :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV444;
            break;

        case STGXOBJ_COLOR_TYPE_SIGNED_YCBCR888_420 :
        case STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420 :
        default :
            Params_p->ColorType = STVID_COLOR_TYPE_YUV420;
            break;
    }
    Params_p->FrameRate     = PictureInfos_p->VideoParams.FrameRate;
    Params_p->Data          = PictureInfos_p->BitmapParams.Data1_p;
    Params_p->Size          = PictureInfos_p->BitmapParams.Size1 + PictureInfos_p->BitmapParams.Size2;
    Params_p->Height        = PictureInfos_p->BitmapParams.Height;
    Params_p->Width         = PictureInfos_p->BitmapParams.Width;
    switch (PictureInfos_p->BitmapParams.AspectRatio)
    {
        case STGXOBJ_ASPECT_RATIO_16TO9 :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
            break;

        case STGXOBJ_ASPECT_RATIO_221TO1 :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
            break;

        case STGXOBJ_ASPECT_RATIO_SQUARE :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
            break;

        case STGXOBJ_ASPECT_RATIO_4TO3 :
        default :
            Params_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;
    }
    switch (PictureInfos_p->VideoParams.ScanType)
    {
        case STGXOBJ_PROGRESSIVE_SCAN :
            Params_p->ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
            break;

        case STGXOBJ_INTERLACED_SCAN :
        default :
            Params_p->ScanType = STVID_SCAN_TYPE_INTERLACED;
            break;
    }
    Params_p->MPEGFrame     = PictureInfos_p->VideoParams.MPEGFrame;
    Params_p->TopFieldFirst = PictureInfos_p->VideoParams.TopFieldFirst;
    Params_p->TimeCode      = PictureInfos_p->VideoParams.TimeCode;
    Params_p->PTS           = PictureInfos_p->VideoParams.PTS;

    /* Private data */
    Params_p->pChromaOffset = (U32) PictureInfos_p->BitmapParams.Data2_p - (U32) PictureInfos_p->BitmapParams.Data1_p;
    Params_p->pTemporalReference = TemporalReference;

} /* End of PictureInfosToPictureParams() function */

/*******************************************************************************
Name        : vidcom_IncrementTimeCode
Description : Increment TimeCode depending of FrameRate
Parameters  : TimeCode to Increment, FrameRate, DropFrameFlag: As in GroupOfPictures if available, FALSE otherwise
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void vidcom_IncrementTimeCode(STVID_TimeCode_t * const TimeCode_p, const U32 FrameRate, BOOL DropFrameFlag)
{
    /* MaxFrames is the number of frames per second, depending on FrameRate.
     It is done +60 before dividing by 1000 in order to round 29970 or 59940 values */
    U8 MaxFrames = (FrameRate + 60) / 1000; /* Should be 30 in NTSC, 25 in PAL !!! */

    if ((TimeCode_p == NULL) || (MaxFrames == 0))
    {
        /* Bad pointer or frame rate 0 */
        return;
    }

    /* Compute depending on temporal_reference */
    TimeCode_p->Frames ++;
    while (TimeCode_p->Frames >= MaxFrames)
    {
        TimeCode_p->Frames -= MaxFrames;
        TimeCode_p->Seconds ++;
    }
    while (TimeCode_p->Seconds >= 60)
    {
        TimeCode_p->Seconds -= 60;
        TimeCode_p->Minutes ++;
    }
    while (TimeCode_p->Minutes >= 60)
    {
        TimeCode_p->Minutes -= 60;
        TimeCode_p->Hours ++;
    }
    if ((DropFrameFlag) && /* drop_frame_flag is set */
        ((TimeCode_p->Minutes % 10) != 0) /* All minutes but not 0, 10, ... 50 */
        )
    {
        /* drop_frame_flag (ISO/IEC 13818-2: 1995 (E)) :                     */
        /* It may be set to '1' only if the frame rate is 29,97Hz. If it is  */
        /* '0' then pictures are counted assuming rounding to the nearest    */
        /* integral number of pictures per second, for example 29,97Hz would */
        /* be rounded to and counted as 30Hz. If it is '1' then picture      */
        /* numbers 0 and 1 at the start of each minute, except minutes 0,    */
        /* 10, 20, 30, 40, 50 are omitted from the count.                    */
        if (TimeCode_p->Frames < 2)
        {
            TimeCode_p->Frames = 2;
        }
    }
    while (TimeCode_p->Hours >= 24)
    {
        TimeCode_p->Hours -= 24;
    }

} /* end of vidcom_IncrementTimeCode */

/*******************************************************************************
Name        : vidcom_PopCommand
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t vidcom_PopCommand(CommandsBuffer_t * const Buffer_p, U8 * const Data_p)
{
    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access to BeginPointer_p and UsedSize */
    interrupt_lock();

    /* Check that buffer is not empty. */
    if (Buffer_p->UsedSize != 0)
    {
        /* Read character */
        *Data_p = *(Buffer_p->BeginPointer_p);

        if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
        {
            Buffer_p->BeginPointer_p = Buffer_p->Data_p;
        }
        else
        {
            (Buffer_p->BeginPointer_p)++;
        }

        /* Update size */
        (Buffer_p->UsedSize)--;
    }
    else
    {
        /* Un-protect access to BeginPointer_p and UsedSize */
        interrupt_unlock();
        /* Can't pop command: buffer empty. So go out after interrupt_unlock() */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    interrupt_unlock();

    return(ST_NO_ERROR);
} /* End of vidcom_PopCommand() function */


/*******************************************************************************
Name        : vidcom_PopCommand32
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t vidcom_PopCommand32(CommandsBuffer32_t * const Buffer_p, U32 * const Data_p)
{
    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access to BeginPointer_p and UsedSize */
    interrupt_lock();

    /* Check that buffer is not empty. */
    if (Buffer_p->UsedSize != 0)
    {
        /* Read character */
        *Data_p = *(Buffer_p->BeginPointer_p);

        if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
        {
            Buffer_p->BeginPointer_p = Buffer_p->Data_p;
        }
        else
        {
            (Buffer_p->BeginPointer_p)++;
        }

        /* Update size */
        (Buffer_p->UsedSize)--;
    }
    else
    {
        /* Un-protect access to BeginPointer_p and UsedSize */
        interrupt_unlock();
        /* Can't pop command: buffer empty. So go out after interrupt_unlock() */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    interrupt_unlock();

    return(ST_NO_ERROR);
} /* End of vidcom_PopCommand32() function */


/*******************************************************************************
Name        : vidcom_PushCommand
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void vidcom_PushCommand(CommandsBuffer_t * const Buffer_p, const U8 Data)
{
    /* Protect access to BeginPointer_p and UsedSize */
    interrupt_lock();

    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    interrupt_unlock();

    /* Can't push command: buffer full */
    return;
} /* End of vidcom_PushCommand() function */


/*******************************************************************************
Name        : vidcom_PushCommand32
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void vidcom_PushCommand32(CommandsBuffer32_t * const Buffer_p, const U32 Data)
{
    /* Protect access to BeginPointer_p and UsedSize */
    interrupt_lock();

    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    interrupt_unlock();

    /* Can't push command: buffer full */
    return;
} /* End of vidcom_PushCommand32() function */


/*******************************************************************************
Name        : vidcom_PushCommand32DoOrValueIfFull
Description : Push a command into a circular buffer of commands, but never loose commands, OR value with previous value if full !
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void vidcom_PushCommand32DoOrValueIfFull(CommandsBuffer32_t * const Buffer_p, const U32 Data)
{
    /* Protect access to BeginPointer_p and UsedSize */
    interrupt_lock();

    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }
    else
    {
        /* Can't push command, buffer full: OR value */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize - 1) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - 1 - Buffer_p->TotalSize) |= Data;
        }
        else if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize - 1) < (Buffer_p->Data_p))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - 1 + Buffer_p->TotalSize) |= Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - 1) |= Data;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    interrupt_unlock();

    /* Can't push command: buffer full */
    return;
} /* End of vidcom_PushCommand32DoOrValueIfFull() function */

/*******************************************************************************
Name        : vidcom_FillBitmapWithDefaults
Description :
Parameters  : IN  : BitmapDefaultFillParams_p.
              OUT : Bitmap_p
Assumptions : Bitmap_p and BitmapDefaultFillParams_p are not NULL
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
void vidcom_FillBitmapWithDefaults(const BitmapDefaultFillParams_t * const BitmapDefaultFillParams_p, STGXOBJ_Bitmap_t * const Bitmap_p)
{
    /* Common values for all the devices */
    Bitmap_p->PreMultipliedColor = FALSE;
    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    Bitmap_p->Width = 0;
    Bitmap_p->Height = 0;
    Bitmap_p->Pitch = 0;
    Bitmap_p->Offset = 0;
    Bitmap_p->Data1_p = NULL;
    Bitmap_p->Size1 = 0;
    Bitmap_p->Data2_p = NULL;
    Bitmap_p->Size2 = 0;
    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;

    /* Specific values */
    if ((BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT) ||
        (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7710_DIGITAL_INPUT) ||
        (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7100_DIGITAL_INPUT) ||
        (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7109_DIGITAL_INPUT) ||
        (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7200_DIGITAL_INPUT))
    {
        Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
        Bitmap_p->BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM;
        Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    }
    else
    {
        if ( (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7015_DIGITAL_INPUT) ||
                (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_7020_DIGITAL_INPUT) )
        {
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
            Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
        }
        else /* device type is one of the MPEG types */
        {
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
            /* MPEG: ColorSpaceConversion depends on the profile */
            switch (BitmapDefaultFillParams_p->BrdCstProfile)
            {
#ifdef STVID_DIRECTV
                case STVID_BROADCAST_DIRECTV : /* DirecTV super-sets MPEG-2 norm */
                    Bitmap_p->ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                    break;
#endif /* STVID_DIRECTV */
                case STVID_BROADCAST_DVB : /* DVB super-sets MPEG-2 norm */
                    /* In DVB default value depends on the MPEG sequence profile and on the frame rate, but here it is
                    too complicated to get all the information, so choose a default that may not be the best
                    (supposing SD not HD) but that will anyway be updated at the first picture displayed, so ok */
                    switch (BitmapDefaultFillParams_p->VTGFrameRate)
                    {
                        case 25000 : /* 25 Hz */
                        case 50000 :
                            Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
                            break;

                        default : /* 30 Hz and others */
                            Bitmap_p->ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                            break;
                    }
                    break;

                case STVID_BROADCAST_ATSC : /* ATSC follows MPEG-2 norm */
                case STVID_BROADCAST_ARIB : /* ARIB follows MPEG-2 norm */
                default :   /* Default in MPEG-2 norm */
                    /* ColorSpaceConversion set to STGXOBJ_ITU_R_BT709 because default value in MPEG norm */
                    Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
                    break;
            }
        }
        Bitmap_p->BitmapType = STGXOBJ_BITMAP_TYPE_MB;
        if(  (BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)
           ||(BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG)
           ||(BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG)
           ||(BitmapDefaultFillParams_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG))
        {
            /* patch color space for blitter-display . It supports only Itu-601 */
            Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
        }
        Bitmap_p->BigNotLittle = FALSE;
    }
} /* End of vidcom_FillBitmapWithDefaults() function */


/*******************************************************************************
Name        : vidcom_ComparePictureID
Description : Compares two PictureID_t
Parameters  : 2 pointers on PictureIDs to compare
Assumptions : None
Limitations :
Returns     : <0 if first is smaller, 0 if equal, >0 if first is greater (as ANSI C memcmp and strcmp functions).
*******************************************************************************/
S32 vidcom_ComparePictureID(VIDCOM_PictureID_t *PictureID1_p, VIDCOM_PictureID_t *PictureID2_p)
{
    S32 Result;

    if(PictureID1_p->IDExtension < PictureID2_p->IDExtension)
    {
        Result = -1;
    }
    else if(PictureID1_p->IDExtension > PictureID2_p->IDExtension)
    {
        Result = 1;
    }
    else
    {   /* MSBs are equal, compare LSBs */
        if(PictureID1_p->ID < PictureID2_p->ID)
        {
            Result = -1;
        }
        else if(PictureID1_p->ID > PictureID2_p->ID)
        {
            Result = 1;
        }
        else
        {
            Result = 0;
        }
    }
    return(Result);
}


/*******************************************************************************
Name        : vidcom_NotifyUserData
Description : Notify User Data event
Parameters  : EventHandle, EventId, User data pointer
Assumptions : None
Limitations :
Returns     : None
*******************************************************************************/
void vidcom_NotifyUserData(STEVT_Handle_t EventsHandle, STEVT_EventID_t EventId, STVID_UserData_t * UserData_p)
{
#ifdef ST_OSLINUX
    STVID_UserData_t    * FlattenedUserData_p;
    U32                   StructAlignedSize = ((sizeof(STVID_UserData_t) + 3)/4)*4;         /* Flattened data are aligned on U32 */

    /* Allocates area for STVID_UserData_t + user data content flattened after structure, flattened data are aligned on U32 */
    if ((FlattenedUserData_p = (STVID_UserData_t *)STOS_MemoryAllocate(NULL, StructAlignedSize + UserData_p->Length)) != NULL)
    {
        /* Copy structure content */
        STOS_memcpy(FlattenedUserData_p, UserData_p, sizeof(STVID_UserData_t));
        /* Flatten data after structure content (aligned on U32) */
        STOS_memcpy((void *)((U32)FlattenedUserData_p + StructAlignedSize), UserData_p->Buff_p, UserData_p->Length);

        /* Use Linux special function STEVT_NotifyWithSize() */
        STEVT_NotifyWithSize(EventsHandle, EventId, STEVT_EVENT_DATA_TYPE_CAST FlattenedUserData_p, StructAlignedSize + UserData_p->Length);
        STOS_MemoryDeallocate(NULL, FlattenedUserData_p);
    }
#else
    STEVT_Notify(EventsHandle, EventId, STEVT_EVENT_DATA_TYPE_CAST UserData_p);
#endif
}


/* Private functions -------------------------------------------------------- */

/* End of vid_ctcm.c */
