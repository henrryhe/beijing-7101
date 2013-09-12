/*******************************************************************************

File name   : vid_crcapi.c

Description :

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 08 Ju 2005        Created                                          MO
*******************************************************************************/

/*#define STTBX_Print*/

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdarg.h> /* va_start */
#include <string.h>
#include <stdlib.h>
#include "stddefs.h"
#endif

#include "api.h"
#include "vid_ctcm.h"
#include "stdevice.h"
#include "stavmem.h"

#include "sttbx.h"
#include "stsys.h"
#include "stpio.h"
#ifdef VALID_TOOLS
    #include "msglog.h"
#endif /* VALID_TOOLS */

/* Public functions --------------------------------------------------------- */

/* Private Defines ---------------------------------------------------------- */

/*-----------------7/8/2005 1:23PM------------------
   STVID_StartCheckCRC
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if parameter problem
              STVID_ERROR_MEMORY_ACCESS if memory copy fails
 * --------------------------------------------------*/
ST_ErrorCode_t STVID_CRCStartCheck(const STVID_Handle_t VideoHandle, STVID_CRCStartParams_t *StartParams_p)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCStartCheck(Device_p->DeviceData_p->CRCHandle, StartParams_p));
    }
} /* End of STVID_CRCStartCheck() function */

/*---------------------------------------------------
   STVID_IsCRCCheckRunning
Returns     : TRUE if a CRC check has been started
 * --------------------------------------------------*/
BOOL STVID_IsCRCCheckRunning(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_IsCRCCheckRunning(Device_p->DeviceData_p->CRCHandle));
    }
} /* End of VIDCRC_IsCRCCheckRunning() function */

/*---------------------------------------------------
   STVID_IsCRCModeField
Returns     : TRUE if CRC check should be done in field mode for this picture
              FALSE if CRC check should be done in frame mode
 * --------------------------------------------------*/
BOOL STVID_IsCRCModeField(const STVID_Handle_t VideoHandle,
                          STVID_PictureInfos_t *PictureInfos_p,
                          STVID_MPEGStandard_t MPEGStandard)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_IsCRCModeField(Device_p->DeviceData_p->CRCHandle, PictureInfos_p, MPEGStandard));
    }
} /* End of STVID_IsCRCModeField() function */

ST_ErrorCode_t STVID_CheckCRC(const STVID_Handle_t VideoHandle,
                                STVID_PictureInfos_t *PictureInfo,
                                U32 ExtendedPresentationOrderPictureIDExtension, U32 ExtendedPresentationOrderPictureID,
                                U32 DecodingOrderFrameID, U32 PictureNumber,
                                BOOL IsRepeatedLastPicture, BOOL IsRepeatedLastButOnePicture,
                                void *LumaAddress, void *ChromaAddress,
                                U32 LumaCRC, U32 ChromaCRC)
{
    VideoDevice_t * Device_p;
    VIDCOM_PictureID_t ExtendedPOPID;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;
        ExtendedPOPID.IDExtension = ExtendedPresentationOrderPictureIDExtension;
        ExtendedPOPID.ID = ExtendedPresentationOrderPictureID;

        return(VIDCRC_CheckCRC(Device_p->DeviceData_p->CRCHandle,
                               PictureInfo, ExtendedPOPID, DecodingOrderFrameID, PictureNumber,
                               IsRepeatedLastPicture, IsRepeatedLastButOnePicture,
                               LumaAddress, ChromaAddress,
                               LumaCRC, ChromaCRC));
    }
} /* End of STVID_CheckCRC() function */

ST_ErrorCode_t STVID_CRCGetCurrentResults(const STVID_Handle_t VideoHandle, STVID_CRCCheckResult_t *CRCCheckResult_p)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCGetCurrentResults(Device_p->DeviceData_p->CRCHandle, CRCCheckResult_p));
    }
} /* End of STVID_CRCGetCurrentResults() function */

ST_ErrorCode_t STVID_CRCStopCheck(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCStopCheck(Device_p->DeviceData_p->CRCHandle));
    }
} /* End of STVID_CRCStopCheck() function */

ST_ErrorCode_t STVID_CRCStartQueueing(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCStartQueueing(Device_p->DeviceData_p->CRCHandle));
    }
} /* End of STVID_CRCStartQueueing() function */

ST_ErrorCode_t STVID_CRCStopQueueing(const STVID_Handle_t VideoHandle)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCStopQueueing(Device_p->DeviceData_p->CRCHandle));
    }
} /* End of STVID_CRCStopQueueing() function */

ST_ErrorCode_t STVID_CRCGetQueue(const STVID_Handle_t VideoHandle, STVID_CRCReadMessages_t *ReadCRC_p)
{
    VideoDevice_t * Device_p;

    if (!(IsValidHandle(VideoHandle)))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

        return(VIDCRC_CRCGetQueue(Device_p->DeviceData_p->CRCHandle, ReadCRC_p));
    }
} /* End of STVID_CRCGetQueue() function */



/* End of vid_crcapi.c file */
