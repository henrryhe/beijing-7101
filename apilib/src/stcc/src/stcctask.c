/*******************************************************************************

File name   : stcctask.c

Description : STCC task manager
    -stcc_ProcessCaption
    -stcc_VSyncCallBack
    -stcc_UserDataCallBack

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
25 June 2001       Created                                       Michel Bruant
08 Jan  2002       Change a semaphore type to be 'timeout'           HSdLM
 *                 Add a protection to CaptionTaskBody
15 May  2002       Put parsing execution under cc task context   Michel Bruant
*******************************************************************************/

/* Defines ------------------------------------------------------------------ */

/*#define TRACE_CCTASK */

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stcctask.h"
#include "stcc.h"
#include "stccinit.h"
#include "parsers.h"
#include "ccslots.h"
#include "stos.h"

#ifdef TRACE_CCTASK
#include "../tests/src/trace.h"
#else
#define TraceBuffer(x)
#endif

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif


/* Extern function proto ---------------------------------------------------- */

#ifdef CLOSED_CAPTION_TEST_MODE
/* this function is defined in ./tests/src/decoder.c */
/* It is linked only in the test/debug application   */
extern void ProcessNewCC( U8* Data);
#endif

/* Private Variables (static)------------------------------------------------ */


/* Constants ---------------------------------------------------------------- */

#define CC_ENTRIES_NEEDED   4

/* Private functions -------------------------------------------------------- */

static void TaskBodyPresentation(stcc_Device_t * const Device_p);
static void TaskBodyParser(stcc_Device_t * const Device_p);

/*------------------------------------------------------------------------------
Name: stcc_UserDataCallBack
Purpose: This function is called when the STVID_USER_DATA_EVT occurs
Notes:
------------------------------------------------------------------------------
*/
void stcc_UserDataCallBack(STEVT_CallReason_t CC_Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            void* EventData,
                            void *SubscriberData_p)
{
    stcc_DeviceData_t * DeviceData_p;
    U32             i;
    BOOL            FoundCopy;

    UNUSED_PARAMETER(CC_Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);


    DeviceData_p = ((stcc_Device_t *)SubscriberData_p)->DeviceData_p;
    if(!DeviceData_p->CCDecodeStarted)
    {
        return;
    }

    STCC_DebugStats.NbUserDataReceived ++;

    FoundCopy = FALSE;
    i = 0;
    while((!FoundCopy) && (i < NB_COPIES))
    {
       if( DeviceData_p->Copies[i].Used ==  STCC_EMPTY)
       {
          DeviceData_p->Copies[i].Used    = STCC_IN_FILL;
          DeviceData_p->Copies[i].Length  = ((STVID_UserData_t*)
                                                EventData)->Length;
          DeviceData_p->Copies[i].PTS     = ((STVID_UserData_t*)EventData)->PTS;
          if(DeviceData_p->Copies[i].Length > COPY_SIZE)
          {
            CC_Trace("Closed caption : Lost User data, copy buff too small !!");
            DeviceData_p->Copies[i].Length  = COPY_SIZE;
          }
          memcpy(DeviceData_p->Copies[i].Data,
                 ((STVID_UserData_t*)EventData)->Buff_p,
                 DeviceData_p->Copies[i].Length);

          /*((char*)DeviceData_p->Copies[i].Data)[DeviceData_p->Copies[i].Length] = 0;*/
/*          printk("%s\r\n", (char*) DeviceData_p->Copies[i].Data);*/
          DeviceData_p->Copies[i].Used    = STCC_FILLED;
          FoundCopy         = TRUE;
       }
       else
       {
           i++;
       }
    }/* end while */
    if(!FoundCopy)
    {
        TraceBuffer(("CC : Lost User data, not enought copies !!\r\n"));
        for(i=0; i<NB_COPIES; i++)
        {
            DeviceData_p->Copies[i].Used      = STCC_EMPTY;
            DeviceData_p->Copies[i].Length    = 0;
        }
        return;
    }
    DeviceData_p->SchedByUserData = TRUE;

#if defined(ST_OSLINUX)
    semaphore_signal((DeviceData_p->Task.SemProcessCC_p));
#else
    if(semaphore_wait_timeout((DeviceData_p->Task.SemProcessCC_p),
                TIMEOUT_IMMEDIATE) == -1)
    {
        semaphore_signal((DeviceData_p->Task.SemProcessCC_p));
    }
#endif
}

/*------------------------------------------------------------------------------
Name:    stcc_VSyncCallBack
Purpose: This function is called when the STVTG_VSYNC_EVT occurs
Notes:
------------------------------------------------------------------------------*/
void stcc_VSyncCallBack(STEVT_CallReason_t CC_Reason,
                        const ST_DeviceName_t RegistrantName,
                        STEVT_EventConstant_t Event,
                        void* EventData,
                        void *SubscriberData_p)
{
    stcc_DeviceData_t * DeviceData_p;

    UNUSED_PARAMETER(CC_Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData);


    DeviceData_p = ((stcc_Device_t *)SubscriberData_p)->DeviceData_p;
    /* Is Task Running ? */
    if(!DeviceData_p->Task.IsRunning)
    {
        return;
    }

    /* Check if there is Closed Caption data to process */
    /*--------------------------------------------------*/
    if (DeviceData_p->NbUsedSlot >= CC_ENTRIES_NEEDED)
    {
        /* Inform CC Task */
        DeviceData_p->SchedByVsync = TRUE;

#if defined(ST_OSLINUX)
        semaphore_signal((DeviceData_p->Task.SemProcessCC_p));
#else
        if(semaphore_wait_timeout((DeviceData_p->Task.SemProcessCC_p),
                TIMEOUT_IMMEDIATE) == -1)
        {
            semaphore_signal((DeviceData_p->Task.SemProcessCC_p));
        }
#endif


    }/* end if nb slots */
}
/***************************************************************
Name : stcc_ProcessCaption
Description : This function is a task.
Parameters : Device_p
Return : none
****************************************************************/
void stcc_ProcessCaption(stcc_Device_t * const Device_p)

{
    stcc_DeviceData_t * DeviceData_p;

STOS_TaskEnter(NULL);

    DeviceData_p = Device_p->DeviceData_p;
    while (!DeviceData_p->Task.ToBeDeleted)
    {
        /* wait for a sched signal */
             semaphore_wait_timeout((DeviceData_p->Task.SemProcessCC_p),
                TIMEOUT_INFINITY);
 if (!DeviceData_p->Task.ToBeDeleted)
        {

    semaphore_wait((Device_p->DeviceData_p->SemAccess_p));

            if(Device_p->DeviceData_p->SchedByUserData)
            {
                /* parse the user data */
                Device_p->DeviceData_p->SchedByUserData = FALSE;
                TaskBodyParser(Device_p);
            }

            if(Device_p->DeviceData_p->SchedByVsync)
            {
                /* present the closed caption data */
                Device_p->DeviceData_p->SchedByVsync = FALSE;
                TaskBodyPresentation(Device_p);
            }

    semaphore_signal((Device_p->DeviceData_p->SemAccess_p));
        }

}
     /* loop until to be deleted */

STOS_TaskExit(NULL);
}
/*------------------------------------------------------------------------------
Name:    TaskBodyPresentation
Purpose:
Notes : at least CC_ENTRIES_NEEDED slots are linked in the list
------------------------------------------------------------------------------
*/
static void TaskBodyPresentation(stcc_Device_t * const Device_p)
{
    stcc_DeviceData_t *     DeviceData_p;
    STVBI_DynamicParams_t   DynamicParams;
    STCC_Data_t             Data;
    stcc_Slot_t             Slot1;
    U32                     DataIndex;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    U8                      VBI_Data[2];
    BOOL                    InSamePTS;
    U32                     PTS;


    DeviceData_p    = Device_p->DeviceData_p;
    Slot1 = DeviceData_p->CaptionSlot[DeviceData_p->FirstSlot];
    PTS = Slot1.PTS.PTS;
    InSamePTS = TRUE;

    while((InSamePTS) && (DeviceData_p->NbUsedSlot > CC_ENTRIES_NEEDED))
    {

        DataIndex = 0;
        while(DataIndex < Slot1.DataLength)
        {


            /* if output->denc : Send to denc via VBI */
            /*----------------------------------------*/
            if((DeviceData_p->OutputMode & STCC_OUTPUT_MODE_DENC)
                                        == STCC_OUTPUT_MODE_DENC)
            {
                VBI_Data[0] = Slot1.CC_Data[DataIndex].Byte1;
                VBI_Data[1] = Slot1.CC_Data[DataIndex].Byte2;
                DynamicParams.VbiType = STVBI_VBI_TYPE_CLOSEDCAPTION;
                DynamicParams.Type.CC.LinesField1 = 21;
                DynamicParams.Type.CC.LinesField2 = 284;
                /* Determine which field(s) will be presented by denc/vbi */
                if(Slot1.CC_Data[DataIndex].Field == 0)
                {
                    DynamicParams.Type.CC.Mode = STVBI_CCMODE_FIELD1;
                    ErrorCode = STVBI_SetDynamicParams(DeviceData_p->VBIHandle,
                                                      &DynamicParams);
                    ErrorCode = STVBI_WriteData(DeviceData_p->VBIHandle,
                                                VBI_Data,2);
                    STCC_DebugStats.NBCaptionDataPresented ++;
                }
                else if(Slot1.CC_Data[DataIndex].Field == 1)
                {
                    DynamicParams.Type.CC.Mode = STVBI_CCMODE_FIELD2;
                    ErrorCode = STVBI_SetDynamicParams(DeviceData_p->VBIHandle,
                                                      &DynamicParams);
                    ErrorCode = STVBI_WriteData(DeviceData_p->VBIHandle,
                                                VBI_Data,2);
                    STCC_DebugStats.NBCaptionDataPresented ++;
                }

            }/* end if mode denc/vbi */

            /* if output->evt : Notify an event */
            /*----------------------------------*/
            if((DeviceData_p->OutputMode & STCC_OUTPUT_MODE_EVENT)
                                        == STCC_OUTPUT_MODE_EVENT)
            {
                Data.Values[0]  = Slot1.CC_Data[DataIndex].Byte1;
                Data.Values[1]  = Slot1.CC_Data[DataIndex].Byte2;
                Data.FormatMode = DeviceData_p->FormatMode;
                Data.Field      = Slot1.CC_Data[DataIndex].Field;
                ErrorCode       = STEVT_Notify(DeviceData_p->EVTHandle,
                                               DeviceData_p->Notify_id,
                                               &Data);

            } /* end if mode evt notification */

#ifdef CLOSED_CAPTION_TEST_MODE
            /* if output->uart : Soft decode engine (for debug/test only) */
            /*-------------------------------------------------------------*/
            if((DeviceData_p->OutputMode & STCC_OUTPUT_MODE_UART_DECODED)
                                        == STCC_OUTPUT_MODE_UART_DECODED)
            {
                if(Slot1.CC_Data[DataIndex].Field == 0)
                {
                    ProcessNewCC((U8*)(&Slot1.CC_Data[DataIndex].Byte1));
                }
            }/* end if mode uart decoded */
#endif
            DataIndex ++;
        } /* reloop with next bytes in the same slot */

        stcc_FreeSlot(Device_p);     /* free this slot, test a next one */
        Slot1 = DeviceData_p->CaptionSlot[DeviceData_p->FirstSlot];
        if(PTS != Slot1.PTS.PTS)
        {
            InSamePTS = FALSE; /* will exit */
        }
    }/* end while in same PTS */
}
/*------------------------------------------------------------------------------
Name: TaskBodyParser
Purpose:
Notes:
------------------------------------------------------------------------------
*/

void TaskBodyParser(stcc_Device_t * const Device_p)
{
    U32      i;

    /* Scan all the user data copies */
    for( i=0; i<NB_COPIES; i++)
    {
        if(Device_p->DeviceData_p->Copies[i].Used == STCC_FILLED)
        {
            Device_p->DeviceData_p->Copies[i].Used = STCC_IN_READ;
            /* Parse the user data */
            /*---------------------*/
            /* if none format is set, try to detect one */
            if(Device_p->DeviceData_p->FormatMode == STCC_FORMAT_MODE_DETECT)
            {
                Device_p->DeviceData_p->FormatMode = STCC_Detect(Device_p,i);
            }
            /* parse with dtvvid21 if req */
            if((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_DTVVID21)
                                                  == STCC_FORMAT_MODE_DTVVID21)
            {
                STCC_ParseDTVvid21(Device_p,i,TRUE /* flag real DirecTV */);
                STCC_DebugStats.NbUserDataParsed ++;
            }
            /* parse with dvb if req */
            if((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_DVB)
                                                  == STCC_FORMAT_MODE_DVB)
            {
                STCC_ParseDTVvid21(Device_p,i,FALSE /* flag real DirecTV */);
                STCC_DebugStats.NbUserDataParsed ++;
            }
            /* parse with eia708 if req */
            if((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_EIA708)
                                                  == STCC_FORMAT_MODE_EIA708)
            {
                STCC_ParseEIA708(Device_p,i);
                STCC_DebugStats.NbUserDataParsed ++;
            }
            /* parse with dvs157 if req (parser = eia608) */
            if(((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_DVS157)
                                                   == STCC_FORMAT_MODE_DVS157)
            ||((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_EIA608)
                                                  == STCC_FORMAT_MODE_EIA608))
            {
                STCC_ParseDVS157(Device_p,i);
                STCC_DebugStats.NbUserDataParsed ++;
            }
            /* parse with udata130 if req */
            if((Device_p->DeviceData_p->FormatMode & STCC_FORMAT_MODE_UDATA130)
                                                  == STCC_FORMAT_MODE_UDATA130)
            {
                STCC_ParseUdata130(Device_p,i);
                STCC_DebugStats.NbUserDataParsed ++;
            }

            Device_p->DeviceData_p->Copies[i].Used = STCC_EMPTY;

        }/* end if copy used */
    }/* end for all copies */
}
/*----------------------------------------------------------------------------*/


