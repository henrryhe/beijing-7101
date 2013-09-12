/*******************************************************************************

File name   : settings.c

Description : STCC settings manager
    -STCC_Start
    -STCC_Stop
    -STCC_SetFormatMode
    -STCC_GetFormatMode
    -STCC_SetOutputMode
    -STCC_GetOutputMode
    -STCC_SetDTVService

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
25 July 2001       Created                                          Michel Bruant
02 Jan  2002       Add 'const' in arguments and null pointer check. HSdLM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stcc.h"
#include "stccinit.h"
#include "ccslots.h"

/*
--------------------------------------------------------------------------------
Start stcc driver
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Start(const STCC_Handle_t Handle)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;
    STVBI_DynamicParams_t DynamicParams;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;
    if(Device_p->DeviceData_p->CCDecodeStarted)
    {
        CC_Defense(STCC_ERROR_DECODER_RUNNING);
        return(STCC_ERROR_DECODER_RUNNING);
    }
    /* VBI configuration */
    DynamicParams.VbiType = STVBI_VBI_TYPE_CLOSEDCAPTION;
    DynamicParams.Type.CC.LinesField1 = 21;
    DynamicParams.Type.CC.LinesField2 = 284;
    DynamicParams.Type.CC.Mode        = STVBI_CCMODE_BOTH;
    ErrCode = STVBI_SetDynamicParams(Device_p->DeviceData_p->VBIHandle,
                                    &DynamicParams);
    if(ErrCode != ST_NO_ERROR)
    {
        CC_Defense(STCC_ERROR_VBI_ACCESS);
        return(STCC_ERROR_VBI_ACCESS);
    }
    STVBI_Enable(Device_p->DeviceData_p->VBIHandle);
    /* don't test the return code */

    /* Start Closed Caption task */
    Device_p->DeviceData_p->CCDecodeStarted = TRUE;

    return(ErrCode);
}

/*
--------------------------------------------------------------------------------
Stop stcc driver
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_Stop(const STCC_Handle_t Handle)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;
    U32 i,j;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;
    if(!Device_p->DeviceData_p->CCDecodeStarted)
    {
        CC_Defense(STCC_ERROR_DECODER_STOPPED);
        return(STCC_ERROR_DECODER_STOPPED);
    }
    /* Stop Closed Caption userdata callback */
    Device_p->DeviceData_p->CCDecodeStarted = FALSE;
    /* Clear the screen */
    stcc_SlotsRecover(Device_p);
    semaphore_wait((Device_p->DeviceData_p->SemAccess_p));
    for(j = 0; j < NB_SLOT; j++)
    {
        i = stcc_GetSlotIndex(Device_p);
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[0].Byte1 = 0x94;
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[0].Byte2 = 0x2c;
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[0].Field = 0;
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[1].Byte1 = 0x94;
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[1].Byte2 = 0x2c;
        Device_p->DeviceData_p->CaptionSlot[i].CC_Data[1].Field = 0;
        Device_p->DeviceData_p->CaptionSlot[i].DataLength       = 2;
        Device_p->DeviceData_p->CaptionSlot[i].PTS.PTS          = j;
        Device_p->DeviceData_p->CaptionSlot[i].PTS.PTS33        = 0;
        stcc_InsertSlot(Device_p,i);
    }
    semaphore_signal((Device_p->DeviceData_p->SemAccess_p));

    return(ErrCode);
}

/*
--------------------------------------------------------------------------------
Set the Format Mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_SetFormatMode( const STCC_Handle_t Handle,
                                   const STCC_FormatMode_t FormatMode)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;

    Device_p->DeviceData_p->FormatMode = FormatMode;

    return(ErrCode);
}
/*
--------------------------------------------------------------------------------
Get the Format Mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_GetFormatMode( const STCC_Handle_t Handle,
                                   STCC_FormatMode_t * const FormatMode_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (FormatMode_p == NULL)
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;

    * FormatMode_p = Device_p->DeviceData_p->FormatMode;

    return(ErrCode);
}
/*
--------------------------------------------------------------------------------
Set the Output Mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_SetOutputMode( const STCC_Handle_t Handle,
                                   const STCC_OutputMode_t OutputMode)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;

    Device_p->DeviceData_p->OutputMode = OutputMode;

    return(ErrCode);
}
/*
--------------------------------------------------------------------------------
Get the Format Mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STCC_GetOutputMode( const STCC_Handle_t Handle,
                                   STCC_OutputMode_t * const OutputMode_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stcc_Device_t * Device_p;

    if (!IsValidHandle(Handle))
    {
        CC_Defense(ST_ERROR_INVALID_HANDLE);
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (OutputMode_p == NULL)
    {
        CC_Defense(ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    Device_p = ((stcc_Unit_t *) Handle)->Device_p;

    * OutputMode_p = Device_p->DeviceData_p->OutputMode;

    return(ErrCode);
}
/*----------------------------------------------------------------------------*/
