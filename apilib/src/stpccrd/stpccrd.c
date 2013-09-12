/*****************************************************************************
File Name   : stpccrd.c

Description : API Interface  for the STPCCRD driver.

History     : Support for the STV0700 and STV0701 is added
              STPCCRD_CheckCIS() was corrected for the ErrorCode Return
              STPCCRD_CIReset() is improved

              Ported to 7100/OS21,5100,7710 & 5105

Copyright (C) 2005 STMicroelectronics

Reference  : ST API Definition "STPCCRD Driver API" STB-API-332
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
/* Standard Library Includes */
#if defined(ST_OS20) || defined(ST_OS21)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if defined( ST_OSLINUX )
#include "linux/kthread.h"
#else
#include "stlite.h"
#include "sttbx.h"
#endif

/* STAPI Includes */
#include "stcommon.h"
#include "stdevice.h"

#if !defined( ST_OSLINUX )
#include "sti2c.h"
#if defined(ST_7710) || defined(ST_7100) || defined(ST_5100) || defined(ST_5301) || defined(ST_5105) \
|| defined(ST_7109) || defined(ST_5107)
#include "stsys.h"
#endif
#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#include "stevt.h"
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

/* Private Includes */
#include "stpccrd.h"
#include <pccrd_hal.h>

/* Private Macros */
#define MALLOC(x)   memory_allocate(InitParams_p->DriverPartition_p,x)
#define FREE_MEM(x) memory_deallocate(InitParams_p->DriverPartition_p,x)

/* Global variable : Pointer to the Driver Instace Linked List */
static PCCRD_ControlBlock_t  *g_PCCRDControlHead = NULL;

/* Private Constants ------------------------------------------------------ */
#define DEVICE_CODE_MSK     0xF0          /* top nibble holds Device Code */
#define I2C_BUS_XS_TIMEOUT  5             /* bus access timeout in ms units */
#define OPEN_HANDLE_WID     8             /* Open bits width */
#define CI_RESET_DELAY      ST_GetClocksPerSecond()/100 /* 10msec */

/* Driver revision number */
static const ST_Revision_t stpccrd_DriverRev = "STPCCRD-REL_1.0.2";

/* Private Variables ------------------------------------------------------ */
static semaphore_t *g_PCCRDAtomicSem_p = NULL;

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
static volatile U8 *DataReg_Mod_A       = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_A_REG_IO_BASE_ADDRESS);
static volatile U8 *Status_Reg_Mod_A    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_A_REG_IO_BASE_ADDRESS+1);
static volatile U8 *Command_Reg_Mod_A   = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_A_REG_IO_BASE_ADDRESS+1);
static volatile U8 *Size_Reg_0_Mod_A    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_A_REG_IO_BASE_ADDRESS+2);
static volatile U8 *Size_Reg_1_Mod_A    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_A_REG_IO_BASE_ADDRESS+3);
static volatile U8 *DataReg_Mod_B       = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_B_OFFSET+DVB_CI_A_REG_IO_BASE_ADDRESS);
static volatile U8 *Status_Reg_Mod_B    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_B_OFFSET+DVB_CI_A_REG_IO_BASE_ADDRESS+1);
static volatile U8 *Command_Reg_Mod_B   = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_B_OFFSET+DVB_CI_A_REG_IO_BASE_ADDRESS+1);
static volatile U8 *Size_Reg_0_Mod_B    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_B_OFFSET+DVB_CI_A_REG_IO_BASE_ADDRESS+2);
static volatile U8 *Size_Reg_1_Mod_B    = (volatile U8*)(BANK_BASE_ADDRESS+DVB_CI_B_OFFSET+DVB_CI_A_REG_IO_BASE_ADDRESS+3);
#endif

/* Private Function prototypes -------------------------------------------- */
static BOOL IsHandleInvalid( STPCCRD_Handle_t       Handle,
                             PCCRD_ControlBlock_t   **Construct_p,
                             U32                    *OpenIndex_p );

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)

BOOL ReadByte( U8* Source_p, U8* Buffer_p);

BOOL ReadByte( U8* Source_p, U8* Buffer_p)
{

    /* This is done to avoid 4byte read by lbinc instruction by ST20C1 core */
    U8 status;
    U8** Src_address = &Source_p;

    interrupt_lock(); /* ensure not descheduled between ldnl and lbinc */

    __asm
    {
    ldl Src_address;
    ldnl 0;
    lbinc;
    st status;
    }
    *Buffer_p =  status;

    interrupt_unlock();

    return TRUE;
}

#endif

/****************************************************************************
Name         : IsHandleInvalid()

Description  : Multi-purpose procedure to check Handle validity, plus return
               the address of its data structure element and the Open Index.

Parameters   : Handle     Identifies the PCCRD device,
               *Construct Pointer to ControlBlock data structure,
               OpenIndex  Index of the OpenBlock structure.

Return Value : TRUE  if Handle is invalid
               FALSE if Handle appears to be legal
****************************************************************************/

BOOL IsHandleInvalid( STPCCRD_Handle_t        Handle,
                      PCCRD_ControlBlock_t    **Construct_p,
                      U32                     *OpenIndex_p )
{
    PCCRD_ControlBlock_t    *ControlBlkPtr_p;
    U32                     OpenIndex;

    for ( ControlBlkPtr_p = g_PCCRDControlHead; ControlBlkPtr_p != NULL;
         ControlBlkPtr_p =(PCCRD_ControlBlock_t*)ControlBlkPtr_p->Next_p)
    {
        for (OpenIndex=0; OpenIndex < ControlBlkPtr_p->MaxHandle; OpenIndex++)
        {
            /* Check for the valid Handle */
            if ((ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].Handle) == Handle )
            {
                /* Check if the Device is closed */
                if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceOpen == FALSE )
                {
                    return (TRUE);                        /* Invalid Handle */
                }

                *Construct_p = ControlBlkPtr_p;         /* Write-back location */
                *OpenIndex_p = OpenIndex;               /* Write back OpenIndex */
                return (FALSE);                         /* Indicate valid Handle */
            }
        }
    }

    return (TRUE);   /* Invalid BaseHandle */

}/* IsHandleInvalid */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED

/******************************************************************************
Name        : RegisterEvents

Description : Called from STI2C_Init() when driver has to operate in slave
              mode. Opens the specified Event Handler & registers the
              events which can be notified in slave mode.

Parameters  : EvtHandlerName   Name of the Event Handler,
              *ControlBlkPtr_p Pointer to ControlBlock data structure

Return Value: STPCCRD_ERROR_EVT : Error related to STEVT
              ST_NO_ERROR       : No Error
******************************************************************************/

static ST_ErrorCode_t RegisterEvents (const ST_DeviceName_t  EvtHandlerName,
                                      PCCRD_ControlBlock_t   *ControlBlkPtr_p)
{
    STEVT_OpenParams_t   EVTOpenParams;
    EVTOpenParams.dummy = 0;

    /* Open the specified event handler */
    if (STEVT_Open (EvtHandlerName, &EVTOpenParams,&ControlBlkPtr_p->EVT_Handle) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    /* Register the Events Exported with STEVT driver */
    if (STEVT_RegisterDeviceEvent( ControlBlkPtr_p->EVT_Handle, ControlBlkPtr_p->DeviceName,
                                   STPCCRD_EVT_MOD_A_INSERTED,&ControlBlkPtr_p->Mod_A_InsertedEvtId) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_RegisterDeviceEvent( ControlBlkPtr_p->EVT_Handle, ControlBlkPtr_p->DeviceName,
                                   STPCCRD_EVT_MOD_B_INSERTED,&ControlBlkPtr_p->Mod_B_InsertedEvtId) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_RegisterDeviceEvent( ControlBlkPtr_p->EVT_Handle, ControlBlkPtr_p->DeviceName,
                                   STPCCRD_EVT_MOD_A_REMOVED,&ControlBlkPtr_p->Mod_A_RemovedEvtId) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_RegisterDeviceEvent( ControlBlkPtr_p->EVT_Handle, ControlBlkPtr_p->DeviceName,
                                   STPCCRD_EVT_MOD_B_REMOVED,&ControlBlkPtr_p->Mod_B_RemovedEvtId) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    return (ST_NO_ERROR);

}/* RegisterEvents */

/******************************************************************************
Name         : STPCCRD_EventNotifyTask
Description  : This task is created only when Hot Plug is Enabled.
               This task waits on a semaphore which is signalled on receving Interrupt
               then checks for the cause of Interrupt and Notify the Relevant Events

Parameters   : *Param_p Pointer to ControlBlock data structure

Return Value : void

See Also     : STPCCRD_Init()
******************************************************************************/

static void STPCCRD_EventNotifyTask (void  *Param_p)
{
    PCCRD_ControlBlock_t  *ControlBlkPtr_p;
    ST_ErrorCode_t        ErrorCode;

    void (* NotifyFunction)(STPCCRD_Handle_t Handle,
                            STPCCRD_EventType_t Event,
                            ST_ErrorCode_t ErrorCode);

#if defined(ST_5516) || defined(ST_5517)

    U16   I2CPIOState;
    U16   HandleCount;

#elif  defined(ST_7100) || defined(ST_5100) || defined(ST_5301) || defined(ST_7109)

    U16   State;
    U16   I2CPIOState;
    U16   HandleCount;

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)

    U8    ISRRegVal = 0;

#endif /* (ST_5516) || (ST_5517) || (ST_5514) || (ST_5518)*/

    ControlBlkPtr_p = (PCCRD_ControlBlock_t*)Param_p;

    /* While loop to wait for CardDetectSemaphore to be signaled */
    while (ControlBlkPtr_p->WaitingToQuit == FALSE)
    {
        STOS_SemaphoreWait(ControlBlkPtr_p->CardDetectSemaphore_p);
        if (ControlBlkPtr_p->WaitingToQuit == TRUE)
        {
            break;
        }

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)

    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);

        /* switch For Card A */
        switch (I2CPIOState & (DVB_CI_CD1_A | DVB_CI_CD2_A))
        {
            case 0 :/* Card A  inserted */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus == MOD_ABSENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                             * the handle of the open client is always passed back to enable the
                             * caller to determine who the event is for.*/

                             NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                             if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen == TRUE))
                             {
                                  NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                  STPCCRD_EVT_MOD_A_INSERTED,ST_NO_ERROR);
                             }
                        }/* End of For loop */
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_A_INSERTED:/* Card A Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                 NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_A_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
        }/* End of switch for Card B */


        /* switch For Card B */
        switch (I2CPIOState & (DVB_CI_CD1_B | DVB_CI_CD2_B))
        {
            case 0 :/* Card B  inserted */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_ABSENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ( NotifyFunction != NULL &&
                                ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE)
                            {
                                NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_B_INSERTED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_B_INSERTED :/* Card B  Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && ( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                               STPCCRD_EVT_MOD_B_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
        }/* End of switch for Card B */

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)

        /* prevent task from re-entrancy during Interrupt Status Read operation */
        STOS_TaskLock();
        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,INTSTA,&ISRRegVal);
        STOS_TaskUnLock();

        /* Checking for Card A */
        if (ISRRegVal & STV0700_INTSTA_DETA)
        {
            /* Mod A inserted */
            if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus==MOD_ABSENT)
            {
                if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                {
                    NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].NotifyFunction;
                    if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen==TRUE))
                    {
                        NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].Handle,
                                       STPCCRD_EVT_MOD_A_INSERTED,ST_NO_ERROR);
                    }
                }
                else
                {
                    if ((ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen==TRUE))
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_InsertedEvtId,NULL);
                    }
                }

                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus = MOD_PRESENT;
            }
            else
            {
                /* Mod A extracted */
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].NotifyFunction;
                        if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen==TRUE))
                        {
                            NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].Handle,
                                           STPCCRD_EVT_MOD_A_REMOVED,ST_NO_ERROR);
                        }
                    }
                    else
                    {
                        if ((ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen==TRUE))
                        {
                            STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_RemovedEvtId,NULL);
                        }
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus = MOD_ABSENT;
                }
            }
        }/*For Card A*/
#ifndef STV0701 /* Check for Card B only if STV0700 is present */

        if (ISRRegVal & STV0700_INTSTA_DETB)
        {
            /* Mod B inserted */
            if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_ABSENT)
            {
                if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                {
                    NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].NotifyFunction;
                    if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen==TRUE))
                    {
                        NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].Handle,
                                       STPCCRD_EVT_MOD_B_INSERTED,ST_NO_ERROR);
                    }

                }
                else
                {
                    if ((ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen==TRUE))
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_InsertedEvtId,NULL);
                    }
                }

                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus = MOD_PRESENT;
            }
            else
            {
                /* Mod B extracted */
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_PRESENT)
                {

                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].NotifyFunction;
                        if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen==TRUE))
                        {
                            NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].Handle,
                                           STPCCRD_EVT_MOD_B_REMOVED,ST_NO_ERROR);
                        }
                    }
                    else
                    {
                        if ((ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen==TRUE))
                        {
                            STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_RemovedEvtId,NULL);
                        }
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus = MOD_ABSENT;
                }
            }
        }/* For Card B */
#endif /* STV0701 */

#elif defined(ST_7710) || defined(ST_5105) || defined(ST_5107)
    U8 State;
    ReadByte((U8*)EPLD_EPM3256A_ADDR, &State);

    /* switch For Card A */
    switch (State & (DVB_CI_CD1_A | DVB_CI_CD2_A))
    {
          case 0 :/* Card A  inserted */
          {
              if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus == MOD_ABSENT)
              {
                  if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                  {
                       /* The event is required for all connected clients */
                       for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                       {
                            /* Notify this client of the event, if notify routine is not NULL
                             * the handle of the open client is always passed back to enable the
                             * caller to determine who the event is for.*/

                             NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                             if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen == TRUE))
                             {
                                  NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                  STPCCRD_EVT_MOD_A_INSERTED,ST_NO_ERROR);
                             }
                        }/* End of For loop */
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_A_INSERTED:/* Card A Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                 NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_A_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
        }/* End of switch for Card B */

        ReadByte((U8*)EPLD_EPM3256A_ADDR, &State);

        /* switch For Card B */
        switch (State & (DVB_CI_CD1_B | DVB_CI_CD2_B))
        {
            case 0 :/* Card B  inserted */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus == MOD_ABSENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ( NotifyFunction != NULL &&
                                ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE)
                            {
                                NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_B_INSERTED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_B_INSERTED :/* Card B  Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && ( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                               STPCCRD_EVT_MOD_B_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
    }/* End of switch for Card B */

#elif defined(ST_5301) || defined(ST_5100)

    State = STSYS_ReadRegDev8((void*)EPLD_EPM3256A_ADDR);

    /* switch For Card A */
    switch (*EPLDState & (DVB_CI_CD1_A | DVB_CI_CD2_A))
    {
          case 0 :/* Card A  inserted */
          {
              if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus == MOD_ABSENT)
              {
                  if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                  {
                       /* The event is required for all connected clients */
                       for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                       {
                            /* Notify this client of the event, if notify routine is not NULL
                             * the handle of the open client is always passed back to enable the
                             * caller to determine who the event is for.*/

                             NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                             if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen == TRUE))
                             {
                                  NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                  STPCCRD_EVT_MOD_A_INSERTED,ST_NO_ERROR);
                             }
                        }/* End of For loop */
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_A_INSERTED:/* Card A Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && (ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                 NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_A_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_A_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
        }/* End of switch for Card B */


        /* switch For Card B */
        switch (State & (DVB_CI_CD1_B | DVB_CI_CD2_B))
        {
            case 0 :/* Card B  inserted */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus == MOD_ABSENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ( NotifyFunction != NULL &&
                                ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE)
                            {
                                NotifyFunction( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                                STPCCRD_EVT_MOD_B_INSERTED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_InsertedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_PRESENT;
                }
                break;
            }

            case PCCRD_CARD_B_INSERTED :/* Card B  Removed */
            {
                if (ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus==MOD_PRESENT)
                {
                    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                    {
                        /* The event is required for all connected clients */
                        for (HandleCount = 0; HandleCount < ControlBlkPtr_p->MaxHandle; HandleCount++)
                        {
                            /* Notify this client of the event, if notify routine is not NULL
                            * the handle of the open client is always passed back to enable the
                            * caller to determine who the event is for.
                            */
                            NotifyFunction = ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].NotifyFunction;
                            if ((NotifyFunction != NULL) && ( ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen==TRUE))
                            {
                                NotifyFunction(ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].Handle,
                                               STPCCRD_EVT_MOD_B_REMOVED,ST_NO_ERROR);
                            }
                        }
                    }
                    else
                    {
                        STEVT_Notify(ControlBlkPtr_p->EVT_Handle,ControlBlkPtr_p->Mod_B_RemovedEvtId,NULL);
                    }

                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].ModPresenceStatus=MOD_ABSENT;
                }
                break;
            }

            default:
                break; /* default case */
        }/* End of switch for Card B */

#endif   /* (ST_5514 || ST_5518 )*/
}/* End of While loop */

}/* STPCCRD_EventNotifyTask */

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

/* API Functions ----------------------------------------------------------- */

/****************************************************************************
Name         : STPCCRD_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
 ****************************************************************************/

ST_Revision_t STPCCRD_GetRevision( void )
{
    return( stpccrd_DriverRev );

}/* STPCCRD_GetRevision */

/****************************************************************************
Name         : STPCCRD_Init()

Description  : Generates a linked list element for each instance called,
               into which the caller's InitParams_p are copied, plus a few
               other vital parameters.
               Initalize the Hardware by calling PCCRD_HardInit()

Parameters   : DeviceName    Name of the PCCRD device,
               *InitParams_p Pointer to Initparams data structure

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_NO_MEMORY              Insufficient memory free
               ST_ERROR_ALREADY_INITIALISED    Device already initialized
               ST_ERROR_BAD_PARAMETER          Error in parameters passed
               STPCCRD_ERROR_I2C               Error in opening I2C Handle
               Following Errors In case Hot Plug In is Enabled
               STPCCRD_ERROR_EVT               Error in opening EVT or registering
                                               the Events
               STPCCRD_ERROR_TASK_FAILURE      Error While creating Task

See Also     : STPCCRD_InitParams_t
               STPCCRD_Term()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Init( const ST_DeviceName_t          DeviceName,
                             const STPCCRD_InitParams_t     *InitParams_p)
{
    PCCRD_ControlBlock_t     *NextCtrlElem_p, *CrtElem_p = NULL; /* Search list pointers */
    PCCRD_ControlBlock_t     *ControlBlkPtr_p;           		 /* Element for this Init */
    ST_ErrorCode_t            ErrorCode = ST_NO_ERROR;
    U32                       HandleCount;

#if !defined(ST_OSLINUX)
#if !defined(ST_7710)  && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
    STI2C_OpenParams_t       I2COpenParams;
#endif
#endif/*ST_OSLINUX*/

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    ST_ErrorCode_t error = ST_NO_ERROR;
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    /* Perform necessary validity checks */
    if ( ( DeviceName == NULL )                      ||    /* NULL address for Name? */
        ( DeviceName[0] == '\0' )                    ||    /* NUL Name string? */
        ( strlen( DeviceName ) >= ST_MAX_DEVICE_NAME )||    /* Name string too long? */
        /*( InitParams_p->BaseAddress_p != NULL)         || */ /* NULL base address */
        ( InitParams_p == NULL ) )                         /* NULL address for InitParams_p? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)
    if ( (InitParams_p->I2CDeviceName[0] == '\0') ||
        ((InitParams_p->DeviceCode & DEVICE_CODE_MSK) != PCF8575_I2C_ADDESS))
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

#endif

#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    if ( (InitParams_p->I2CDeviceName[0] == '\0') ||
        ((InitParams_p->DeviceCode != STV0700_I2C_ADDESS_1) &&
         (InitParams_p->DeviceCode != STV0700_I2C_ADDESS_2) &&
         (InitParams_p->DeviceCode != STV0700_I2C_ADDESS_3) &&
         (InitParams_p->DeviceCode != STV0700_I2C_ADDESS_4)))
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
#elif defined(ST_7710) || defined(ST_5301) || defined(ST_5100) || defined(ST_5105) \
|| defined(ST_5107)
    if ((InitParams_p->DeviceCode != EPLD_EPM3256A_ADDR) ||
        (InitParams_p->DeviceCode != EPLD_EPM3256B_ADDR))
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif

#if !defined(ST_5528)
    if ((InitParams_p->MaxHandle < STPCCRD_MIN_OPEN_DEVICE )  ||   /* Invalid MaxHandle? */
        (InitParams_p->MaxHandle > STPCCRD_MAX_OPEN_DEVICE )  ||   /* Invalid MaxHandle? */
        (InitParams_p->DriverPartition_p == NULL))                 /* Driver partition omitted? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif
    /*
    The following section is protected from re-entrancy whilst critical
    elements within the g_PCCRDControlHead linked list are both read and
    written.
    */

    /* Check the Linked list if this device has been already Initalised. */
    if (g_PCCRDControlHead != NULL)
    {
        NextCtrlElem_p = g_PCCRDControlHead;
        do
        {
            CrtElem_p = NextCtrlElem_p;
            if (!strcmp(CrtElem_p->DeviceName,DeviceName))
            {
                /* This device has already been opened  */
                return( ST_ERROR_ALREADY_INITIALIZED );
            }

            NextCtrlElem_p = (PCCRD_ControlBlock_t*)CrtElem_p->Next_p;

        }while (CrtElem_p->Next_p != NULL);
    }

    /* Atomic semaphore creation */
    STOS_TaskLock();                                        /* Prevent re-entrancy */
    if (g_PCCRDControlHead == NULL)
    {
       g_PCCRDAtomicSem_p = STOS_SemaphoreCreateFifo(NULL,1); /* Initialize semaphore */

    }
    STOS_TaskUnlock();                                      /* Re-enable schedules etc */

    /* Semaphore creation */
    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );               /* Atomic sequence start */

    if ((ControlBlkPtr_p = ( PCCRD_ControlBlock_t* )MALLOC(sizeof(PCCRD_ControlBlock_t))) == NULL)
    {
        return ( ST_ERROR_NO_MEMORY );
    }

    /* Insert the current element in the list */
    if (g_PCCRDControlHead == NULL)
    {
        g_PCCRDControlHead = ControlBlkPtr_p;
    }
    else
    {
        CrtElem_p->Next_p = (void*)ControlBlkPtr_p;
    }

    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );              /* End of critical region */

    /* Common initialisations  */
    ControlBlkPtr_p->Next_p = NULL;
    ControlBlkPtr_p->BaseHandle = ( STPCCRD_Handle_t )( (U32)ControlBlkPtr_p << OPEN_HANDLE_WID );
    strcpy( ControlBlkPtr_p->DeviceName, DeviceName );
    ControlBlkPtr_p->NumberOpen = 0;                       /* No Open Handles yet */
    ControlBlkPtr_p->DeviceLock = FALSE;

    /* Memory allocation for the open block of each module */
    ControlBlkPtr_p->OpenBlkPtr_p =(PCCRD_OpenBlock_t*)MALLOC(InitParams_p->MaxHandle * sizeof(PCCRD_OpenBlock_t));

    if ((ControlBlkPtr_p->OpenBlkPtr_p) == NULL)
    {
        FREE_MEM(ControlBlkPtr_p);
        return(ST_ERROR_NO_MEMORY);
    }

    for ( HandleCount = 0;HandleCount < InitParams_p->MaxHandle; HandleCount++ )
    {
        ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceOpen  = FALSE;
        ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].DeviceLock  = FALSE; /*For Multi Access to same Module*/
        ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].AccessState = PCCRD_ATTRIB_ACCESS;

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        ControlBlkPtr_p->OpenBlkPtr_p[HandleCount].ModPresenceStatus = MOD_ABSENT;
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
    }

    /* Copy Device details from caller */
    ControlBlkPtr_p->DriverPartition_p = InitParams_p->DriverPartition_p;
    ControlBlkPtr_p->MaxHandle         = InitParams_p->MaxHandle;

    /* Initialise the TS Processing state */
    ControlBlkPtr_p->MOD_A_TS_PROCESSING_ENABLED = FALSE;
    ControlBlkPtr_p->MOD_B_TS_PROCESSING_ENABLED = FALSE;

#ifdef ST_OSLINUX
    /* Remap base address under Linux */
    ControlBlkPtr_p->PccrdEPLDMappingWidth = STPCCRD_EPLD_MAPPING_WIDTH;
    ControlBlkPtr_p->MappingEPLDBaseAddress = (unsigned long *)STLINUX_MapRegion((void *)(EPLD_BANK_BASE_ADDRESS),
                                                              ControlBlkPtr_p->PccrdEPLDMappingWidth ,"PCCRDEPLDMEM" );
    if (ControlBlkPtr_p->MappingEPLDBaseAddress == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }

    ControlBlkPtr_p->PccrdDVBCIMappingWidth = STPCCRD_DVBCI_MAPPING_WIDTH;
    ControlBlkPtr_p->MappingDVBCIBaseAddress = (unsigned long *)STLINUX_MapRegion((void *)(BANK_BASE_ADDRESS |  0x400000),
                                                                ControlBlkPtr_p->PccrdDVBCIMappingWidth ,"PCCRDDVBCIMEM" );
    if (ControlBlkPtr_p->MappingDVBCIBaseAddress == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }

    ControlBlkPtr_p->PccrdEMICFGMappingWidth = STPCCRD_EMICFG_MAPPING_WIDTH;
    ControlBlkPtr_p->MappingEMICFGBaseAddress = (unsigned long *)STLINUX_MapRegion((void *)(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET),
                                                                ControlBlkPtr_p->PccrdEMICFGMappingWidth ,"PCCRDEMICFGMEM" );
    if (ControlBlkPtr_p->MappingEMICFGBaseAddress == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
#endif

    /* Open I2C before controller initialization */
#if defined(ST_5516) || defined(ST_5517) || defined(ST_5514) || defined(ST_5518) || defined(ST_7100) \
|| defined(ST_5528) || defined(ST_7109)

#if !defined(ST_OSLINUX)
    I2COpenParams.I2cAddress       = InitParams_p->DeviceCode;
    I2COpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
    I2COpenParams.BusAccessTimeOut = I2C_BUS_XS_TIMEOUT;

    if ((ErrorCode = STI2C_Open(InitParams_p->I2CDeviceName,&I2COpenParams,
            &ControlBlkPtr_p->I2C_Handle )) != ST_NO_ERROR)
    {
        FREE_MEM(ControlBlkPtr_p->OpenBlkPtr_p);
        FREE_MEM(ControlBlkPtr_p);
        return(STPCCRD_ERROR_I2C);
    }
#else

   ControlBlkPtr_p->I2C_Message.addr = InitParams_p->DeviceCode >> 1;
   if((ControlBlkPtr_p->I2C_Message.addr)!= 0)
   {
       ControlBlkPtr_p->Adapter_299 =(struct i2c_adapter*)i2c_get_adapter(0);
       if(ControlBlkPtr_p->Adapter_299 == NULL)
       {
             return ST_ERROR_OPEN_HANDLE;
       }
   }
   else
   {
       return(STPCCRD_ERROR_I2C);
   }
#endif/*ST_OSLINUX*/
#endif

    /* Open Connection to Event */
    /* Open the event handler attached to this instance */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED

    ControlBlkPtr_p->InterruptNumber = InitParams_p->InterruptNumber;
    ControlBlkPtr_p->InterruptLevel  = InitParams_p->InterruptLevel;

    if (InitParams_p->EvtHandlerName[0] != '\0')
    {
        if ((ErrorCode = RegisterEvents (InitParams_p->EvtHandlerName, ControlBlkPtr_p))!=ST_NO_ERROR)
        {
            FREE_MEM(ControlBlkPtr_p->OpenBlkPtr_p);
            FREE_MEM(ControlBlkPtr_p);
            return(ErrorCode);
        }
    }
    else
    {
        /* STEVT is not used so make the EVT Handle as NULL */
        ControlBlkPtr_p->EVT_Handle = 0;
    }
    /* Create A Event Manager Task  */
    ControlBlkPtr_p->CardDetectSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
    ControlBlkPtr_p->WaitingToQuit = FALSE;

    error =  STOS_TaskCreate((void (*)(void*))STPCCRD_EventNotifyTask,
                                       ControlBlkPtr_p,
                                       ControlBlkPtr_p->TaskStack,
                                       20*PCCRD_TASK_STACK_SIZE,
                                       &(ControlBlkPtr_p->task),
                                       ControlBlkPtr_p->TaskStack,
                                       &ControlBlkPtr_p->task,
                                       &(ControlBlkPtr_p->tdesc),
                                       MAX_USER_PRIORITY,
                                       "STPCCRD_EventNotifyTask",
                                        (task_flags_t)0);

    if(error != ST_NO_ERROR)
    {
        FREE_MEM(ControlBlkPtr_p->OpenBlkPtr_p);
        FREE_MEM(ControlBlkPtr_p);
        return(STPCCRD_ERROR_TASK_FAILURE);
    }
    else
    {
          printk("entering task event notfy created");
    }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    /* Hardware Initialization */
    if ((ErrorCode = PCCRD_HardInit(ControlBlkPtr_p)) != ST_NO_ERROR)
    {
        FREE_MEM(ControlBlkPtr_p->OpenBlkPtr_p);
        FREE_MEM(ControlBlkPtr_p);
        return(ErrorCode);
    }
    return( ST_NO_ERROR );

}/* STPCCRD_Init */

/****************************************************************************
Name         : STPCCRD_Term()

Description  : Terminates an instance of the PCCRD Interface, optionally
               closing any Open Handles if forced, including I2C Handle.

Parameters   : DeviceName  Specifies name of PCCRD device,
               *TermParams Holds the ForceTerminate flag

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_BAD_PARAMETER       Invalid parameter(s)
               ST_ERROR_UNKNOWN_DEVICE      Init not called
               ST_ERROR_OPEN_HANDLE         Close not called
               Following Errors In case Hot Plug In is Enabled
               ST_ERROR_INTERRUPT_UNINSTALL Error while uninstalling Inerrupts
               STPCCRD_ERROR_TASK_FAILURE   Error While terminating Task

See Also     : STPCCRD_Init()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Term( const ST_DeviceName_t          DeviceName,
                             const STPCCRD_TermParams_t     *TermParams_p)
{
    PCCRD_ControlBlock_t  *ControlBlkPtr_p, *PrevElem_p = NULL;

    ST_ErrorCode_t        ErrorCode = ST_NO_ERROR;

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    task_t                *Task_p;
    clock_t               Timeout;
    ST_ErrorCode_t        error = ST_NO_ERROR;
#endif

    /* Perform parameter validity checks */
    if ( ( DeviceName   == NULL ) ||                /* NULL Name ptr */
        ( TermParams_p == NULL ))                   /* NULL structure ptr */
    {
         return( ST_ERROR_BAD_PARAMETER );
    }

    if ( g_PCCRDControlHead == NULL )               /* No Inits outstanding? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );          /* Device not exists */
    }
    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );           /* Start of atomic region */

    /* Loop for all the device instances in the linked list */
    for ( ControlBlkPtr_p = g_PCCRDControlHead;ControlBlkPtr_p != NULL;
         ControlBlkPtr_p  =(PCCRD_ControlBlock_t*)ControlBlkPtr_p->Next_p)
    {
        if ( strcmp( DeviceName,ControlBlkPtr_p->DeviceName )== 0)
        {
            break;
        }
        PrevElem_p = ControlBlkPtr_p;               /* Keeping the previous element in the list */
    }

    if ( ControlBlkPtr_p == NULL )                   /* No DeivceName match found */
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );      /* End atomic before return */
        return( ST_ERROR_UNKNOWN_DEVICE );
    }

    if ( ControlBlkPtr_p->NumberOpen > 0 )           /* Any Handles Open */
    {
        if (!TermParams_p->ForceTerminate )          /* Unforced Closure */
        {
            STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );  /* End atomic before return */
            return(ST_ERROR_OPEN_HANDLE );          /* User needs to call Close(s) */
        }
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    /*
     * Uninstall the interrupt handler for this port. Only disable
     * interrupt if no other handlers connected on the same level.
     * Note this needs to be MT-safe.
     */
    STOS_InterruptLock();

#if defined(ST_OSLINUX)
    free_irq(ControlBlkPtr_p->InterruptNumber, (void *)&ControlBlkPtr_p);
#else
    if (interrupt_uninstall( ControlBlkPtr_p->InterruptNumber,
                             ControlBlkPtr_p->InterruptLevel) != 0)
    {
        ErrorCode = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif/*ST_OSLINUX*/

#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable (ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif

    STOS_InterruptUnLock();       /* End of interrupt uninstallation */

    /* Make sure that WaitingToQuit is done */
    ControlBlkPtr_p->WaitingToQuit = TRUE;

    STOS_SemaphoreSignal(ControlBlkPtr_p->CardDetectSemaphore_p);

    /* Give some time to EventNotify Task before quiting */
    Timeout = time_plus(time_now(),ST_GetClocksPerSecond() * PCCRD_TIME_DELAY);

    if(error == ST_NO_ERROR)
    {
        STOS_TaskDelete(&ControlBlkPtr_p->task,
                         ControlBlkPtr_p->TaskStack,
                        &ControlBlkPtr_p->task,
                         ControlBlkPtr_p->TaskStack);
    }
    else
    {
         ErrorCode = STPCCRD_ERROR_TASK_FAILURE;
    }

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

#if defined(ST_5516) || defined(ST_5517) || defined(ST_5514) || defined(ST_5518) || defined(ST_7100) \
|| defined(ST_5528) || defined(ST_7109)
#if !defined(ST_OSLINUX)
    if ((ErrorCode = STI2C_Close(ControlBlkPtr_p->I2C_Handle )) != ST_NO_ERROR)
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );  /* End atomic before return */
        return( ErrorCode );                     /* Memory not freed */
    }
#else
    ControlBlkPtr_p->I2C_Message.addr = 0;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );

#endif/*ST_OSLINUX*/
#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    if (PCCRD_UsingEventHandler(ControlBlkPtr_p))
    {
        if ((ErrorCode = STEVT_Close(ControlBlkPtr_p->EVT_Handle )) != ST_NO_ERROR)
        {
            STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );   /* End atomic before return */
            return( ErrorCode );                     /* Memory not freed */
        }
    }
    /* Deallocate the semaphores */
    STOS_SemaphoreDelete(NULL,ControlBlkPtr_p->CardDetectSemaphore_p );
#endif

    /* Update the Devices Linked list */
    if ( ControlBlkPtr_p == g_PCCRDControlHead )
    {
        /* Shift the global Header pointer if its the first instance */
        g_PCCRDControlHead =(PCCRD_ControlBlock_t*)ControlBlkPtr_p->Next_p;
    }
    else
    {
        /* Take the previous Instance refrence to update the list */
        PrevElem_p->Next_p = ControlBlkPtr_p->Next_p;
    }
#if defined(ST_OSLINUX)
    STLINUX_UnmapRegion((void *)ControlBlkPtr_p->MappingEPLDBaseAddress ,ControlBlkPtr_p->PccrdEPLDMappingWidth);
    STLINUX_UnmapRegion((void *)ControlBlkPtr_p->MappingDVBCIBaseAddress , ControlBlkPtr_p->PccrdDVBCIMappingWidth);
    STLINUX_UnmapRegion((void *)ControlBlkPtr_p->MappingEMICFGBaseAddress ,ControlBlkPtr_p->PccrdEMICFGMappingWidth);
#endif
    memory_deallocate(ControlBlkPtr_p->DriverPartition_p,ControlBlkPtr_p->OpenBlkPtr_p);
    memory_deallocate(ControlBlkPtr_p->DriverPartition_p,ControlBlkPtr_p);
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );              /* End of atomic region */

    /* For the last device, the Atomic semaphore must be deleted */
    if ( g_PCCRDControlHead == NULL )                    /* No Inits outstanding */
    {
         STOS_SemaphoreDelete(NULL,g_PCCRDAtomicSem_p);
    }

    return( ErrorCode );

}/* STPCCRD_Term */

/****************************************************************************
Name         : STPCCRD_Open()

Description  : Searches the linked list of Init instances for a string
               match with the Device name. The address of the matching
               entry forms part of the Handle, the remaining part is the
               Open index.

Parameters   : DeviceName  Specifies name of PCCRD device,
               *OpenParams Pointer to the openparams,
               *Handle_p   Pointer to Handle value to be written

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_UNKNOWN_DEVICE     Named Device not intialised
               ST_ERROR_BAD_PARAMETER      Bad parameter detected
               ST_ERROR_NO_FREE_HANDLES    MaxHandle exceeded

See Also     : STPCCRD_Close()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Open( const ST_DeviceName_t      DeviceName,
                             const STPCCRD_OpenParams_t *OpenParams_p,
                             STPCCRD_Handle_t           *Handle_p )
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Perform parameter validity checks*/
    if (( DeviceName   == NULL )   ||                /* NULL Name pointer */
        ( Handle_p     == NULL ))                    /* NULL Handle pointer */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Look for the Device in the global Linked list*/
    for ( ControlBlkPtr_p = g_PCCRDControlHead;ControlBlkPtr_p != NULL;
         ControlBlkPtr_p =(PCCRD_ControlBlock_t*)ControlBlkPtr_p->Next_p )
    {
        if ( strcmp( DeviceName,ControlBlkPtr_p->DeviceName ) == 0 )
        {
            break;
        }
    }

    if ( ControlBlkPtr_p == NULL )                          /* No Name match found? */
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }

#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
#ifdef STV0701
    if (OpenParams_p->ModName != STPCCRD_MOD_A)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif
#endif

    /* Now that the Init instance is determined, complete the validity checks */
    if (ControlBlkPtr_p->NumberOpen >= ControlBlkPtr_p->MaxHandle) /* All Open slots in use */
    {
        return( ST_ERROR_NO_FREE_HANDLES );
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
    {
        /* If STEVT is not used Callback must be given */
        if (OpenParams_p->NotifyFunction == NULL)
        {
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    /* Locate a free MemSegment slot, protecting the structure in case of re-entrancy. */
    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                      /* Atomic sequence start */
    switch (OpenParams_p->ModName)
    {
        case STPCCRD_MOD_A:
        {
            if ( ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen == FALSE )
            {
                ControlBlkPtr_p->NumberOpen++;                 /* Increment Handles Open */
                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].DeviceOpen = TRUE;
                *Handle_p =(U32)ControlBlkPtr_p->OpenBlkPtr_p;     /* Write back Handle_p to caller */
                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].Handle=*Handle_p;
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
                if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                {
                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].NotifyFunction =OpenParams_p->NotifyFunction;
                }
                else
                {
                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].NotifyFunction=NULL;
                }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
            }
            else
            {
                ErrorCode= ST_ERROR_NO_FREE_HANDLES;
            }

            break;
        }

        case STPCCRD_MOD_B:
        {
            if ( ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen == FALSE )
            {
                ControlBlkPtr_p->NumberOpen++;                 /* Increment Handles Open */
                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].DeviceOpen = TRUE;
                *Handle_p = (U32)((ControlBlkPtr_p->OpenBlkPtr_p) + 1);   /* Write back Handle_p to caller */
                ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].Handle=*Handle_p;
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
                if (!PCCRD_UsingEventHandler(ControlBlkPtr_p))
                {
                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].NotifyFunction =OpenParams_p->NotifyFunction;
                }
                else
                {
                    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].NotifyFunction=NULL;
                }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
            }
            else
            {
                ErrorCode= ST_ERROR_NO_FREE_HANDLES;
            }

            break;
        }

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
    }/*Switch Ends*/

    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                  /* End of critical region */
    return(ErrorCode);

}/* STPCCRD_Open */

/****************************************************************************
Name         : STPCCRD_Close()

Description  : Flags the specified PCCRD device segment as closed.

Parameters   : Handle Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_INVALID_HANDLE     Handle value is not Correct
               ST_ERROR_DEVICE_BUSY        Device is busy in serving other client
               ST_NO_ERROR                 No errors occurred

See Also     : STPCCRD_Open()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Close( STPCCRD_Handle_t Handle )
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    U32                         OpenIndex;                 /* Open Handle number */

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Ensure area to be accessed doesn't overflow the segment */
    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                      /* Atomic sequence start */

    /* Check if Device in use by another caller */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Free the Open slot */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceOpen = FALSE;

    /* Make Handle Value as NULL */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].Handle = 0;

    /* Decrement Handles Open */
    ControlBlkPtr_p->NumberOpen--;

    /* Atomic sequence end */
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );

    return( ErrorCode );

}/* STPCCRD_Close */

/****************************************************************************
Name         : STPCCRD_ModCheck()

Description  : Checks For the Presence of Module

Parameters   : Handle Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value is not correct
               STPCCRD_ERROR_NO_MODULE     Required Module is not present
               STPCCRD_ERROR_I2C_READ      Error in reading the I2C Interface
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_ModCheck(STPCCRD_Handle_t Handle)
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode;
    U32                         OpenIndex;     /* Open Handle number */

    /* Perform parameter validity checks Handle passed invalid? */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    /* In case of HotPlugIn Support at driver level Information about presence is maintained
       internally */

    switch (OpenIndex)
    {
        case STPCCRD_MOD_A:
        case STPCCRD_MOD_B:
        {
            if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ModPresenceStatus==MOD_PRESENT)
            {
                ErrorCode = ST_NO_ERROR;
            }
            else
            {
                ErrorCode = STPCCRD_ERROR_NO_MODULE;
            }
            break;
        }

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        break;
    }
#else

#if defined(ST_5516) || defined(ST_5517)
    {
        U16  I2CPIOState;

        /* If HotPlugin is not enabled then API reads the CD#(Card Detect) pins status */
        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }

        /* Checking CD# pins for Card A or B */
        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
            {

                if (( I2CPIOState & (DVB_CI_CD1_A | DVB_CI_CD2_A)) == 0 )
                {
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }

                break;

            }

            case STPCCRD_MOD_B:
            {

                if ((I2CPIOState & (DVB_CI_CD1_B | DVB_CI_CD2_B)) == 0 )
                {
                    ErrorCode=  ST_NO_ERROR;
                }
                else
                {
                    ErrorCode= STPCCRD_ERROR_NO_MODULE;
                }

                break;

            }
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
        }/* End of Switch */
    }

#elif defined(ST_7100) || defined(ST_7109)
    {

        U8   CardRegister;
        U32  CardRegAddress;

#if !defined(ST_OSLINUX)
        /* If HotPlugin is not enabled then API reads the CD#(Card Detect) pins status */
        CardRegAddress =  CARD_REG; /* Read the Card Reg of the EPLD for Card detect */
        CardRegister   = *(U8*)CardRegAddress;
#else
        CardRegAddress = (U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG;
        CardRegister   = *(U8*)CardRegAddress;
#endif

        /* Checking CD# pins for Card A or B */
        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
            {
                if (( CardRegister & (DVB_CI_CD1_A | DVB_CI_CD2_A)) == 0 )
                {
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }

                break;
            }

            case STPCCRD_MOD_B:
            {
                if (( CardRegister & (DVB_CI_CD1_B | DVB_CI_CD2_B)) == 0 )
                {
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }

                break;
            }

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;

            break;
        }/* End of Switch */
    }
#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U8        ControllerRegValue;
        U32       I2CAddressOffset;

        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
            {
                I2CAddressOffset = MODA;
                break;
            }

#if !defined(ST_5528)
            case STPCCRD_MOD_B:
            {
                I2CAddressOffset = MODB;
                break;
            }
#endif
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                return (ErrorCode);

        }/* End of Switch */

        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2CAddressOffset,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            if (ControllerRegValue & STV0700_MOD_DET)
            {
                ErrorCode = ST_NO_ERROR;
            }
            else
            {
                ErrorCode = STPCCRD_ERROR_NO_MODULE;
            }
        }
    }

#elif defined(ST_7710) || defined(ST_5105) || defined(ST_5107)
    {
        U8 State;
        /* Checking CD# pins for Card A or B */
        ReadByte((U8*)EPLD_EPM3256A_ADDR, &State);
        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:

                if ((State & (DVB_CI_CD1_A | DVB_CI_CD2_A)) == 0 )
                {
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }
                break;

            case STPCCRD_MOD_B:

#if !defined(ST_5105) && !defined(ST_5107)
                if ((State & (DVB_CI_CD1_B | DVB_CI_CD2_B)) == 0 )
                {
                    ErrorCode =  ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;

                }
#endif
                break;
        }
    }
#elif defined(ST_5100) || defined(ST_5301)
    {

        /* Checking CD# pins for Card A or B */
        U16  State;

        switch (OpenIndex)
        {

            State = STSYS_ReadRegDev8((void*)EPLD_EPM3256A_ADDR);
            case STPCCRD_MOD_A:

                if (( State & (DVB_CI_CD1_A | DVB_CI_CD2_A)) == 0 )
                {
                    ErrorCode = ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }
                break;

            case STPCCRD_MOD_B:


                if ((State & (DVB_CI_CD1_B | DVB_CI_CD2_B)) == 0 )
                {
                    ErrorCode =  ST_NO_ERROR;
                }
                else
                {
                    ErrorCode = STPCCRD_ERROR_NO_MODULE;
                }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
        }/* End of Switch */
    }
#endif
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    return (ErrorCode);

}/* STPCCRD_ModCheck */

/****************************************************************************
Name         : STPCCRD_ModReset()

Description  : Performs a Hard reset

Parameters   : Handle Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value is not correct
               ST_ERROR_DEVICE_BUSY        Device is busy
               STPCCRD_ERROR_I2C_WRITE     Error in writing the I2C Interface
               STPCCRD_ERROR_I2C_READ      Error in reading the I2C Interface
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_ModReset(STPCCRD_Handle_t Handle)
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    U32                         OpenIndex = 0;                /* Open Handle number */

    /* Perform parameter validity checks */
    if (IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ))
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                     /* Atomic sequence start */

    /* Checking if the device is busy */
    if (ControlBlkPtr_p->DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );               /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable (ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

#if defined(ST_5516) || defined(ST_5517)
    {
        U16  I2CPIOState;

        switch ( OpenIndex)
        {
            case STPCCRD_MOD_A : /* Resetting Card A */

                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState |(U16)DVB_CI_RESET_A;
                    /*I2CPIOState = I2CPIOState |(U16)(DVB_CI_CD_A_B);*/
                    ErrorCode   = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        break;
                    }
                    task_delay(PCCRD_TIME_DELAY);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        I2CPIOState = I2CPIOState & ~((U16)DVB_CI_RESET_A);
                        /*I2CPIOState = I2CPIOState |(U16)(DVB_CI_CD_A_B);*/
                        ErrorCode   = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                    }
                }
                break;

            case STPCCRD_MOD_B : /* Resetting Card B */

                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState |(U16)DVB_CI_RESET_B;
                    /*I2CPIOState = I2CPIOState |(U16)(DVB_CI_CD_A_B);*/
                    ErrorCode   = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        break;
                    }
                    task_delay(PCCRD_TIME_DELAY);
                    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        I2CPIOState = I2CPIOState & ~((U16)DVB_CI_RESET_B);
                        /*I2CPIOState = I2CPIOState|(U16)(DVB_CI_CD_A_B);*/
                        ErrorCode   = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                    }
                }

                break;

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }

#elif defined(ST_7100) || defined(ST_7109)
    {
        U8   CardRegister;
        clock_t t1;
        U8 EPLDRegister;
        BOOL RESET_LOW = FALSE,RESET_HIGH=FALSE;

        switch ( OpenIndex)
        {
            case STPCCRD_MOD_A : /* Resetting Card A */

#if !defined(ST_OSLINUX)
                CardRegister = *(U8*)CARD_REG;
                CardRegister |= DVB_CI_RESET_A;
                *(U8*)CARD_REG = CardRegister;
#else
                CardRegister = *(U8*)(ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG);
                CardRegister |= DVB_CI_RESET_A;
                *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG) = CardRegister;
#endif/*ST_OSLINUX*/

                task_delay(ST_GetClocksPerSecond()/100000);   /* 10usec delay */

#if !defined(ST_OSLINUX)
                EPLDRegister = *(U8*)INTR_REG;
#else
                EPLDRegister =  *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + INTR_REG);
#endif/*ST_OSLINUX*/

                if ((EPLDRegister & 0x01) == 0x01)
                {
                    RESET_LOW = TRUE;
                }
                CardRegister &= ~(DVB_CI_RESET_A);
#if !defined(ST_OSLINUX)
                *(U8*)CARD_REG = CardRegister;
#else
                *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG) = CardRegister;
#endif/*ST_OSLINUX*/

                t1 = time_now();
                do
                {

#if !defined(ST_OSLINUX)
                    EPLDRegister = *(U8*)INTR_REG;
#else
                    EPLDRegister =  *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + INTR_REG);
#endif/*ST_OSLINUX*/
                    if ((EPLDRegister & 0x01) == 0x00)
                    {
                        RESET_HIGH = TRUE;
                        break;
                    }
                }
                while( time_minus(time_now(),t1) <= 5*ST_GetClocksPerSecond());    /*max 5sec */

                break;

            case STPCCRD_MOD_B : /* Resetting Card B */

#if !defined(ST_OSLINUX)
                CardRegister = *(U8*)CARD_REG;
                CardRegister |= DVB_CI_RESET_B;
                *(U8*)CARD_REG = CardRegister;
#else
                CardRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG);
                CardRegister |= DVB_CI_RESET_B;
                *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG) = CardRegister;
#endif/*ST_OSLINUX*/

                task_delay(ST_GetClocksPerSecond()/100000);   /* 10usec delay */

#if !defined(ST_OSLINUX)
                EPLDRegister = *(U8*)INTR_REG;
#else
                EPLDRegister =  *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + INTR_REG);
#endif/*ST_OSLINUX*/
                if ((EPLDRegister & 0x02) == 0x02)
                {
                    RESET_LOW = TRUE;
                }

                CardRegister &= ~(DVB_CI_RESET_B);

#if !defined(ST_OSLINUX)
                *(U8*)CARD_REG = CardRegister;
#else
                *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_REG) = CardRegister;
#endif/*ST_OSLINUX*/

                t1 = time_now();
                do
                {

#if !defined(ST_OSLINUX)
                EPLDRegister = *(U8*)INTR_REG;
#else
                EPLDRegister =  *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + INTR_REG);
#endif/*ST_OSLINUX*/
                    if ((EPLDRegister & 0x02) == 0x00)
                    {
                        RESET_HIGH = TRUE;
                        break;
                    }
                }
                while( time_minus(time_now(),t1) <= 5*ST_GetClocksPerSecond());    /*max 5sec */
                break;

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }

       /* if (RESET_LOW == FALSE || RESET_HIGH == FALSE)
        {
            ErrorCode = ST_ERROR_TIMEOUT;
        }*/


    }
#elif defined(ST_5514) || defined(ST_5518)
    {
        U8       ControllerRegValue = 0;
        U32      I2CAddressOffset   = 0;

        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
                I2CAddressOffset = MODA;
                break;

            case STPCCRD_MOD_B:
                I2CAddressOffset = MODB;
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                ControlBlkPtr_p->DeviceLock = FALSE;
                return (ErrorCode);
        }

        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2CAddressOffset,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            ControllerRegValue = ControllerRegValue | STV0700_MOD_RST;
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2CAddressOffset,(U16)ControllerRegValue);
        }

        /* Unset the reset bit of Module Control register */
        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2CAddressOffset,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            ControllerRegValue = ControllerRegValue & (~STV0700_MOD_RST);
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2CAddressOffset,(U16)ControllerRegValue);
        }
    }
#elif defined(ST_5528)
    {
        U8       ControllerRegValue = 0;
        U32      I2CAddressOffset   = 0;

        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
                I2CAddressOffset = CICNTRL;
                break;

            case STPCCRD_MOD_B:
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                ControlBlkPtr_p->DeviceLock = FALSE;
                return (ErrorCode);
        }

        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2CAddressOffset,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            ControllerRegValue = ControllerRegValue | STV0700_MOD_RST;
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2CAddressOffset,(U16)ControllerRegValue);
        }
    }

#elif defined(ST_5100) || defined(ST_5301)

    {
        U8  RegValue = 0;
        switch ( OpenIndex)
        {
            case STPCCRD_MOD_A : /* Resetting Card A */

                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x01 );
                RegValue = STSYS_ReadRegDev8((void*)(EPLD_DVBCI_RESET_REG ));
                if (( RegValue & 0x01) != 0x01)
                {
                    ErrorCode = STPCCRD_ERROR_MOD_RESET;
                }
                else
                {
                    ErrorCode = ST_NO_ERROR;
                }
                task_delay(CI_RESET_DELAY);
                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x00 );
                break;

            case STPCCRD_MOD_B : /* Resetting Card B */

                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ), 0x02 );
                RegValue = STSYS_ReadRegDev8((void*)(EPLD_DVBCI_RESET_REG ));
                if ( (RegValue & 0x02) != 0x02)
                {
                    ErrorCode = STPCCRD_ERROR_MOD_RESET;
                }
                else
                {
                    ErrorCode = ST_NO_ERROR;
                }
                task_delay(CI_RESET_DELAY);
                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x00 );
                break;

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }

    }

#elif defined(ST_5105) || defined(ST_7710) || defined(ST_5107)

    {
        U8 RegValue = 0;

        switch ( OpenIndex)
        {
            case STPCCRD_MOD_A : /* Resetting Card A */

                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x01 );
            ReadByte((U8*)EPLD_DVBCI_RESET_REG,&RegValue);
                if (( RegValue & 0x01) != 0x01)
                {
                    ErrorCode = STPCCRD_ERROR_MOD_RESET;
                }
                else
                {
                    ErrorCode = ST_NO_ERROR;
                }
                task_delay(CI_RESET_DELAY);
                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x00 );
                break;

            case STPCCRD_MOD_B : /* Resetting Card B */

                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ), 0x02 );
                ReadByte((U8*)EPLD_DVBCI_RESET_REG,&RegValue);
                if ( (RegValue & 0x02) != 0x02)
                {
                    ErrorCode = STPCCRD_ERROR_MOD_RESET;
                }
                else
                {
                    ErrorCode = ST_NO_ERROR;
                }
                task_delay(CI_RESET_DELAY);
                STSYS_WriteRegDev8((void*)(EPLD_DVBCI_RESET_REG ),  0x00 );
                break;

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }

    }

#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif



    /* Free the device */
    ControlBlkPtr_p->DeviceLock = FALSE;
    return (ErrorCode);

}/* STPCCRD_ModReset */

/****************************************************************************
Name         : STPCCRD_ModPowerOn

Description  : Performs Power Setting

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)
               Voltage  Specifies the voltage

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value not correct
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               STPCCRD_ERROR_I2C_WRITE     Error in writing the I2C Interface
               STPCCRD_ERROR_I2C_READ      Error in reading the I2C Interface

See Also     : STPCCRD_ModPowerOff()
               STPCCRD_PowerMode_t
****************************************************************************/

ST_ErrorCode_t STPCCRD_ModPowerOn( STPCCRD_Handle_t     Handle,
                                   STPCCRD_PowerMode_t  Voltage )
{
    PCCRD_ControlBlock_t    *ControlBlkPtr_p;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    U32                     OpenIndex = 0;                /* Open Handle number */

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                 /* Atomic sequence start */

    /* Checks if the device in use */
    if ( ControlBlkPtr_p->DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );           /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );               /* Atomic sequence end */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable (ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

#if defined(ST_5516) || defined(ST_5517)
    {
        U16    I2CPIOState;
        switch (Voltage)/* Check the voltage */
        {
            case STPCCRD_POWER_3300 :
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    if (OpenIndex == STPCCRD_MOD_A)
                    {
                        I2CPIOState = I2CPIOState & ~(DVB_CI_VCC5V_NOTVCC3V_A);
                        I2CPIOState = I2CPIOState |DVB_CI_CD_A_B;
                    }
                    else
                    {
                        if (OpenIndex == STPCCRD_MOD_B)
                        {
                            I2CPIOState = I2CPIOState & ~(DVB_CI_VCC5V_NOTVCC3V_B);
                            I2CPIOState = I2CPIOState |DVB_CI_CD_A_B;
                        }
                        else
                        {
                            ErrorCode= ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                }

                break;

            case STPCCRD_POWER_5000 :

                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    if (OpenIndex == STPCCRD_MOD_A)
                    {
                        I2CPIOState = I2CPIOState |DVB_CI_VCC5V_NOTVCC3V_A;
                        I2CPIOState = I2CPIOState |DVB_CI_CD_A_B;
                    }
                    else
                    {
                        if (OpenIndex == STPCCRD_MOD_B)
                        {
                            I2CPIOState = I2CPIOState |DVB_CI_VCC5V_NOTVCC3V_B;
                            I2CPIOState = I2CPIOState |DVB_CI_CD_A_B;
                        }
                        else
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch ends */
        if (ErrorCode == ST_NO_ERROR) *(U8*)CARD_PWR_REG = CardPwrRegister;
    }

#elif defined(ST_7100) || defined(ST_7109)
    {

        U8   CardPwrRegister;
        switch (Voltage)/* Check the voltage */
        {
            case STPCCRD_POWER_3300 :

                    if (OpenIndex == STPCCRD_MOD_A)
                    {
#if !defined(ST_OSLINUX)
                        CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
                        CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/
                        CardPwrRegister &= ~(DVB_CI_VCC5V_NOTVCC3V_A);
                        CardPwrRegister |= DVB_CI_VCC3V_NOTVCC5V_A;
                    }
                    else if (OpenIndex == STPCCRD_MOD_B)
                    {
#if !defined(ST_OSLINUX)
                        CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
                        CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/
                        CardPwrRegister &= ~(DVB_CI_VCC5V_NOTVCC3V_B);
                        CardPwrRegister |= DVB_CI_VCC3V_NOTVCC5V_B;
                    }
                    else
                    {
                        ErrorCode= ST_ERROR_BAD_PARAMETER;
                    }

                break;

            case STPCCRD_POWER_5000 :

                    if (OpenIndex == STPCCRD_MOD_A)
                    {
#if !defined(ST_OSLINUX)
                        CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
                        CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/
                        CardPwrRegister &= ~(DVB_CI_VCC3V_NOTVCC5V_A);
                        CardPwrRegister |= DVB_CI_VCC5V_NOTVCC3V_A;
                    }
                    else if (OpenIndex == STPCCRD_MOD_B)
                    {
#if !defined(ST_OSLINUX)
                        CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
                        CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/
                        CardPwrRegister &= ~(DVB_CI_VCC3V_NOTVCC5V_B);
                        CardPwrRegister |= DVB_CI_VCC5V_NOTVCC3V_B;
                    }
                    else
                    {
                        ErrorCode= ST_ERROR_BAD_PARAMETER;
                    }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch ends */
        if (ErrorCode == ST_NO_ERROR)
        {
#if !defined(ST_OSLINUX)
            *(U8*)CARD_PWR_REG = CardPwrRegister;
#else
            *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG) = CardPwrRegister;
#endif/*ST_OSLINUX*/

       }
    }
#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U8  ControllerRegValue = 0;
        switch (Voltage)
        {
            case STPCCRD_POWER_3300 :
            case STPCCRD_POWER_5000 :
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,PCR,&ControllerRegValue);
                if ( ErrorCode == ST_NO_ERROR )
                {
                    ControllerRegValue = (ControllerRegValue | STV0700_CARD_POWER_ON);
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)ControllerRegValue);
                }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch ends */
    }
#elif defined(ST_5100) || defined(ST_5301)
    {

        U8 Volt_Test = 0;
        switch (Voltage) /* Check the voltage */
        {
            case STPCCRD_POWER_3300 :

                /*ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);*/
                if (OpenIndex == STPCCRD_MOD_A)
                {
                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256A_ADDR & DVB_CI_VCC3V_NOTVCC5V_A), DVB_CI_VCC3V_NOTVCC5V_A );
                    Volt_Test = (U8)STSYS_ReadRegDev8((void*)EPLD_EPM3256A_ADDR);
                    if ( (Volt_Test & DVB_CI_VCC3V_NOTVCC5V_A) != DVB_CI_VCC3V_NOTVCC5V_A )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256A_ADDR,  DVB_CI_VCC3V_NOTVCC5V_A );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }
                }
                else if (OpenIndex == STPCCRD_MOD_B)
                {

                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR & DVB_CI_VCC3V_NOTVCC5V_B), DVB_CI_VCC3V_NOTVCC5V_B );
                    Volt_Test = (U8)STSYS_ReadRegDev8((void*)EPLD_EPM3256B_ADDR);
                    if ( (Volt_Test & DVB_CI_VCC3V_NOTVCC5V_B) != DVB_CI_VCC3V_NOTVCC5V_B )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256B_ADDR, DVB_CI_VCC3V_NOTVCC5V_B );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }
                }
                break;

            case STPCCRD_POWER_5000 :
                if ( OpenIndex == STPCCRD_MOD_A )
                {
                    /* Card A */
                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256A_ADDR & DVB_CI_VCC5V_NOTVCC3V_A), DVB_CI_VCC5V_NOTVCC3V_A );
                    Volt_Test = (U8)STSYS_ReadRegDev8((void*)EPLD_EPM3256A_ADDR);
                    if ( (Volt_Test & DVB_CI_VCC5V_NOTVCC3V_A) != DVB_CI_VCC5V_NOTVCC3V_A )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256A_ADDR, DVB_CI_VCC5V_NOTVCC3V_A );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }

                }
                else if ( OpenIndex == STPCCRD_MOD_B )
                {
                     /* Card B */
                     STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR & DVB_CI_VCC5V_NOTVCC3V_B), DVB_CI_VCC5V_NOTVCC3V_B );
                     Volt_Test = (U8)STSYS_ReadRegDev8((void*)EPLD_EPM3256B_ADDR);
                     if ( (Volt_Test & DVB_CI_VCC5V_NOTVCC3V_B) != DVB_CI_VCC5V_NOTVCC3V_B )
                     {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256B_ADDR, DVB_CI_VCC5V_NOTVCC3V_B );
                     }
                     else
                     {
                        ErrorCode = FALSE;
                     }

                }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch ends */
    }

#elif defined(ST_7710) || defined(ST_5105) || defined(ST_5107)
    {

        U8 Volt_Test = 0;
        switch (Voltage) /* Check the voltage */
        {
            case STPCCRD_POWER_3300 :

                if (OpenIndex == STPCCRD_MOD_A)
                {
                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256A_ADDR & DVB_CI_VCC3V_NOTVCC5V_A), DVB_CI_VCC3V_NOTVCC5V_A );
                    ReadByte((U8*)EPLD_EPM3256A_ADDR,&Volt_Test);
                    if ( (Volt_Test & DVB_CI_VCC3V_NOTVCC5V_A) != DVB_CI_VCC3V_NOTVCC5V_A )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256A_ADDR,  DVB_CI_VCC3V_NOTVCC5V_A );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }
                }
#if !defined(ST_5105) && !defined(ST_5107)
                else if (OpenIndex == STPCCRD_MOD_B)
                {

                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR & DVB_CI_VCC3V_NOTVCC5V_B), DVB_CI_VCC3V_NOTVCC5V_B );
                    ReadByte((U8*)EPLD_EPM3256A_ADDR,&Volt_Test);
                    if ( (Volt_Test & DVB_CI_VCC3V_NOTVCC5V_B) != DVB_CI_VCC3V_NOTVCC5V_B )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256B_ADDR, DVB_CI_VCC3V_NOTVCC5V_B );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }
                }
#endif
                break;

            case STPCCRD_POWER_5000 :
                if ( OpenIndex == STPCCRD_MOD_A )
                {
                    /* Card A */
                    STSYS_WriteRegDev8((void*)(EPLD_EPM3256A_ADDR & DVB_CI_VCC5V_NOTVCC3V_A), DVB_CI_VCC5V_NOTVCC3V_A );
                    ReadByte((U8*)EPLD_EPM3256A_ADDR,&Volt_Test);
                    if ( (Volt_Test & DVB_CI_VCC5V_NOTVCC3V_A) != DVB_CI_VCC5V_NOTVCC3V_A )
                    {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256A_ADDR, DVB_CI_VCC5V_NOTVCC3V_A );
                    }
                    else
                    {
                        ErrorCode = FALSE;
                    }

                }
#if !defined(ST_5105) && !defined(ST_5107)
                else if ( OpenIndex == STPCCRD_MOD_B )
                {
                     /* Card B */
                     STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR & DVB_CI_VCC5V_NOTVCC3V_B), DVB_CI_VCC5V_NOTVCC3V_B );
                     ReadByte((U8*)EPLD_EPM3256A_ADDR,&Volt_Test);
                     if ( (Volt_Test & DVB_CI_VCC5V_NOTVCC3V_B) != DVB_CI_VCC5V_NOTVCC3V_B )
                     {
                        STSYS_WriteRegDev8((void*)EPLD_EPM3256B_ADDR, DVB_CI_VCC5V_NOTVCC3V_B );
                     }
                     else
                     {
                        ErrorCode = FALSE;
                     }

                }
#endif
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch ends */
    }

#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif

   /* Free the Device */
   ControlBlkPtr_p->DeviceLock = FALSE;

   return (ErrorCode);

}/* STPCCRD_ModPowerOn */

/****************************************************************************
Name         : STPCCRD_ModPowerOff

Description  : Performs Power Setting

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value not correct
               STPCCRD_ERROR_CANT_POWEROFF In case of 5516/17 no Power off possible

See Also     : STPCCRD_ModPowerOn()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_ModPowerOff( STPCCRD_Handle_t Handle )
{
    PCCRD_ControlBlock_t    *ControlBlkPtr_p;
    U32                     OpenIndex=0;                  /* Open Handle number */
    ST_ErrorCode_t          ErrorCode=ST_NO_ERROR;

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable (ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */


#if defined(ST_5516) || defined(ST_5517) || defined(ST_7710) || defined(ST_5100) || defined(ST_5301) \
|| defined(ST_5105) || defined(ST_5107)
    ErrorCode = STPCCRD_ERROR_CANT_POWEROFF;


#elif defined(ST_7100) || defined(ST_7109)
    {

        U8   CardPwrRegister;

        if (OpenIndex == STPCCRD_MOD_A)
        {

#if !defined(ST_OSLINUX)
            CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
            CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/

            CardPwrRegister &= ~(DVB_CI_VCC5V_NOTVCC3V_A | DVB_CI_VCC3V_NOTVCC5V_A);
        }
        else if (OpenIndex == STPCCRD_MOD_B)
        {
#if !defined(ST_OSLINUX)
            CardPwrRegister = *(U8*)CARD_PWR_REG;
#else
            CardPwrRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG);
#endif/*ST_OSLINUX*/
            CardPwrRegister &= ~(DVB_CI_VCC5V_NOTVCC3V_B | DVB_CI_VCC3V_NOTVCC5V_B);
        }
        else
        {
            ErrorCode= ST_ERROR_BAD_PARAMETER;
        }

        if (ErrorCode == ST_NO_ERROR)
        {
#if !defined(ST_OSLINUX)
            *(U8*)CARD_PWR_REG = CardPwrRegister;
#else
            *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_PWR_REG) = CardPwrRegister;
#endif/*ST_OSLINUX*/
        }
    }

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U8  ControllerRegValue;
        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,PCR,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            ControllerRegValue = ControllerRegValue & (~STV0700_CARD_POWER_ON);
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)ControllerRegValue);
        }
    }
#endif /* ST_5516 || ST_5517 || ST_5514 || ST_5518 */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif

    return (ErrorCode);

}/* STPCCRD_ModPowerOff */

/****************************************************************************
Name         : STPCCRD_CIReset

Description  : Performs a Command Interface Reset

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value not correct
               ST_ERROR_DEVICE_BUSY        Device is currently Busy
               STPCCRD_ERROR_CI_RESET      Error In resetting the Command Interface

 ****************************************************************************/

ST_ErrorCode_t STPCCRD_CIReset( STPCCRD_Handle_t Handle)
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode;
    U32                         OpenIndex;                  /* Open Handle number */
#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    U32                         CommandInterfaceBaseAddress = 0;/* Address of IO Base register */
    U8                          *CIRegBaseAddress_p;
#endif
    U8                          StatusRegValue;
    U16                         RetryCount;

    ErrorCode = ST_NO_ERROR;

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE);
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );       /* Atomic sequence start */

    /* Check if the device in use by another caller */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );  /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY);
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

    /* Check and switch to IO access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_IO_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex, PCCRD_IO_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
            return ( ErrorCode );
        }
    }

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
    switch(OpenIndex)
    {
        case STPCCRD_MOD_A:

            /* Reset the Command Interface */
            *Command_Reg_Mod_A = PCCRD_SETRS;

            task_delay(CI_RESET_DELAY);

            /* Make the Reset bit low */
            *Command_Reg_Mod_A = PCCRD_RESET_COMMAND;

            /* Get the Status register value */
            task_delay(CI_RESET_DELAY);

            ReadByte((U8*)Status_Reg_Mod_A,&StatusRegValue);

            /* Verify and retry to set the FR bit */
            for (RetryCount = 0; RetryCount < PCCRD_MAX_CI_RESET_RETRY; RetryCount++)
            {
                if (!( StatusRegValue & PCCRD_FR))
                {
                    task_delay(CI_RESET_DELAY);
                    ReadByte((U8*)Status_Reg_Mod_A,&StatusRegValue);
                }
                else
                {
                    break;
                 }

            }
            break;

        case STPCCRD_MOD_B:

            /* Reset the Command Interface */
            *Command_Reg_Mod_B = PCCRD_SETRS;

            task_delay(CI_RESET_DELAY);

            /* Make the Reset bit low */
            *Command_Reg_Mod_B = PCCRD_RESET_COMMAND;

            /* Get the Status register value */
            task_delay(CI_RESET_DELAY);

            ReadByte((U8*)Status_Reg_Mod_B,&StatusRegValue);

            /* Verify and retry to set the FR bit */
            for (RetryCount = 0; RetryCount < PCCRD_MAX_CI_RESET_RETRY; RetryCount++)
            {
                if (!( StatusRegValue & PCCRD_FR))
                {
                    task_delay(CI_RESET_DELAY);
                    ReadByte((U8*)Status_Reg_Mod_B,&StatusRegValue);
                }
                else
                {
                    break;
                 }

            }
            break;

       }

#else

    /* Address of Command Interface Registers */
#if !defined(ST_OSLINUX)
    CommandInterfaceBaseAddress = BANK_BASE_ADDRESS;/*ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress
    includes BANK_BASE_ADDRESS*/
#endif
    CommandInterfaceBaseAddress += ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress;
    CIRegBaseAddress_p          = (U8*)CommandInterfaceBaseAddress;

    /* Reset the Command Interface */
    *(CIRegBaseAddress_p + PCCRD_CMDSTATUS)  = PCCRD_SETRS;

    task_delay(CI_RESET_DELAY);

    /* Make the Reset bit low */
    *(CIRegBaseAddress_p + PCCRD_CMDSTATUS) = PCCRD_RESET_COMMAND;

    /* Get the Status register value */
    task_delay(CI_RESET_DELAY);

    StatusRegValue = *(CIRegBaseAddress_p + PCCRD_CMDSTATUS);

    /* Verify and retry to set the FR bit */
    for (RetryCount = 0; RetryCount < PCCRD_MAX_CI_RESET_RETRY; RetryCount++)
    {
        if (!( StatusRegValue & PCCRD_FR))
        {
            task_delay(CI_RESET_DELAY);
            StatusRegValue = *(CIRegBaseAddress_p + PCCRD_CMDSTATUS);
        }
        else
        {
            break;
        }

    }

#endif

    if (RetryCount == PCCRD_MAX_CI_RESET_RETRY)
    {
        ErrorCode = STPCCRD_ERROR_CI_RESET;
    }

    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return ( ErrorCode );

}/* STPCCRD_CIReset */

/****************************************************************************
Name         : STPCCRD_GetStatus()

Description  : Check for the status of the bit specified.

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)
               Status   Specifies the Status bit of the Status Register
               *Value_p Pointer to value of bit

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value is not correct
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)

See Also     : STPCCRD_SetStatus()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_GetStatus ( STPCCRD_Handle_t       Handle,
                                   STPCCRD_Status_t       Status,
                                   U8                     *Value_p)
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode;
    U32                         OpenIndex;                  /* Open Handle number */
    U8                          StatusRegValue;
#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    volatile U8                 *CIRegBaseAddress_p;
    U32                          CommandInterfaceBaseAddress = 0;
#endif

    ErrorCode = ST_NO_ERROR;

    /* Perform Handle validity checks Handle passed invalid */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p);                      /* Atomic sequence start */

    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;

    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

    /* Check and switch to IO access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_IO_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex, PCCRD_IO_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock= FALSE;
            return ( ErrorCode );
        }
    }

    /* Getting the Command Interface Address */
#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
    switch(OpenIndex)
    {
        case STPCCRD_MOD_A:
            ReadByte((U8*)Status_Reg_Mod_A,&StatusRegValue);
            task_delay( ST_GetClocksPerSecond()/1000);
            break;

         case STPCCRD_MOD_B:
            ReadByte((U8*)Status_Reg_Mod_B,&StatusRegValue);
            task_delay( ST_GetClocksPerSecond()/1000);
            break;
    }

#else

#if !defined(ST_OSLINUX)
    CommandInterfaceBaseAddress = BANK_BASE_ADDRESS;
    /*Mapped ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress
    includes BANK_BASE_ADDRESS*/
#endif
    CommandInterfaceBaseAddress += ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress;
    CIRegBaseAddress_p          = (volatile U8*)CommandInterfaceBaseAddress;

    task_delay( ST_GetClocksPerSecond()/1000);  /* 1msec delay */
    StatusRegValue              = *(CIRegBaseAddress_p + PCCRD_CMDSTATUS);
    task_delay( ST_GetClocksPerSecond()/1000);  /* 1msec delay */

#endif

    *Value_p       = BIT_VALUE_LOW;

    switch (Status) /* Checking for the status bit */
    {
        case STPCCRD_CI_DATA_AVAILABLE:
            if (StatusRegValue & PCCRD_DA)
            {
                *Value_p = BIT_VALUE_HIGH;
            }
            break;

        case STPCCRD_CI_FREE:
            if (StatusRegValue & PCCRD_FR)
            {
                *Value_p = BIT_VALUE_HIGH;
            }
            break;

        case STPCCRD_CI_READ_ERROR:
            if (StatusRegValue & PCCRD_RE)
            {
                *Value_p = BIT_VALUE_HIGH;
            }
            break;

        case STPCCRD_CI_WRITE_ERROR:
            if (StatusRegValue & PCCRD_WE)
            {
                *Value_p = BIT_VALUE_HIGH;
            }
            break;

        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;

    }/* Switch ends */

    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return (ErrorCode);

}/* STPCCRD_GetStatus */

/****************************************************************************
Name         : STPCCRD_SetStatus()

Description  : Set the command bit specified of the Command/Status Register.

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)
               Command  Specifies the Command bit of the Command Register
               Value    Value of bit (high or low)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value is not correct
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)

See Also     : STPCCRD_GetStatus()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_SetStatus ( STPCCRD_Handle_t       Handle,
                                   STPCCRD_Command_t      Command,
                                   U8                     Value)
{
    PCCRD_ControlBlock_t    *ControlBlkPtr_p;
    ST_ErrorCode_t          ErrorCode;
    U32                     OpenIndex;                  /* Open Handle number */

#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    volatile U8             *CIRegBaseAddress_p;
    U32                     CommandInterfaceBaseAddress = 0;
#endif

    ErrorCode = ST_NO_ERROR;

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                      /* Atomic sequence start */

    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

    /* Check and switch to IO access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_IO_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex, PCCRD_IO_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock= FALSE;
            return ( ErrorCode );
        }
    }

    /* Getting the Command Interface Address */
#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)

    switch(OpenIndex)
    {
        case STPCCRD_MOD_A:

            if ( Value == BIT_VALUE_HIGH ) /* If the Command bit is to be set high */
            {
                switch(Command)/* Checking for the Command bit */
                {
                    case STPCCRD_CI_RESET:
                    *Command_Reg_Mod_A    = PCCRD_SETRS;
                    break;

                    case STPCCRD_CI_SIZE_READ:
                    *Command_Reg_Mod_A    = PCCRD_SETSR;
                    break;

                    case STPCCRD_CI_SIZE_WRITE:
                    *Command_Reg_Mod_A    = PCCRD_SETSW;
                    break;

                    case STPCCRD_CI_HOST_CONTROL:
                    *Command_Reg_Mod_A    = PCCRD_SETHC;
                    break;

                    default:
                    ErrorCode= ST_ERROR_BAD_PARAMETER;
                }/* Switch Ends */
            }
            else
            {
                if ( Value == BIT_VALUE_LOW )/* If the Command bit is to be set low */
                {
                     switch(Command)/* Checking for the Command bit */
                     {
                          case STPCCRD_CI_RESET:
                          case STPCCRD_CI_SIZE_READ:
                          case STPCCRD_CI_SIZE_WRITE:
                          case STPCCRD_CI_HOST_CONTROL:
                             *Command_Reg_Mod_A  = PCCRD_RESET_COMMAND;
                              break;
                          default:
                              ErrorCode = ST_ERROR_BAD_PARAMETER;
                     }/* Switch Ends */
                }
                else
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

        case STPCCRD_MOD_B:

            if ( Value == BIT_VALUE_HIGH ) /* If the Command bit is to be set high */
            {
                switch(Command)/* Checking for the Command bit */
                {
                    case STPCCRD_CI_RESET:
                    *Command_Reg_Mod_B    = PCCRD_SETRS;
                    break;

                    case STPCCRD_CI_SIZE_READ:
                    *Command_Reg_Mod_B    = PCCRD_SETSR;
                    break;

                    case STPCCRD_CI_SIZE_WRITE:
                    *Command_Reg_Mod_B    = PCCRD_SETSW;
                    break;

                    case STPCCRD_CI_HOST_CONTROL:
                    *Command_Reg_Mod_B    = PCCRD_SETHC;
                    break;

                    default:
                    ErrorCode= ST_ERROR_BAD_PARAMETER;
                }/* Switch Ends */
            }
            else
            {
                if ( Value == BIT_VALUE_LOW )/* If the Command bit is to be set low */
                {
                     switch(Command)/* Checking for the Command bit */
                     {
                          case STPCCRD_CI_RESET:
                          case STPCCRD_CI_SIZE_READ:
                          case STPCCRD_CI_SIZE_WRITE:
                          case STPCCRD_CI_HOST_CONTROL:
                             *Command_Reg_Mod_B  = PCCRD_RESET_COMMAND;
                              break;
                          default:
                              ErrorCode = ST_ERROR_BAD_PARAMETER;
                     }/* Switch Ends */
                }
                else
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;
      }
#else

#if !defined(ST_OSLINUX)
    CommandInterfaceBaseAddress = BANK_BASE_ADDRESS;
#endif
    CommandInterfaceBaseAddress += ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress;
    CIRegBaseAddress_p          = (volatile U8*)CommandInterfaceBaseAddress;

    if ( Value == BIT_VALUE_HIGH ) /* If the Command bit is to be set high */
    {
        switch(Command)/* Checking for the Command bit */
        {
            case STPCCRD_CI_RESET:
                *(CIRegBaseAddress_p+ PCCRD_CMDSTATUS) = PCCRD_SETRS;
                break;

            case STPCCRD_CI_SIZE_READ:
                *(CIRegBaseAddress_p+ PCCRD_CMDSTATUS) = PCCRD_SETSR;
                break;

            case STPCCRD_CI_SIZE_WRITE:
                *(CIRegBaseAddress_p+ PCCRD_CMDSTATUS) = PCCRD_SETSW;
                break;

            case STPCCRD_CI_HOST_CONTROL:
                *(CIRegBaseAddress_p+ PCCRD_CMDSTATUS) = PCCRD_SETHC;
                break;

            default:
                ErrorCode= ST_ERROR_BAD_PARAMETER;
        }/* Switch Ends */
    }
    else
    {
        if ( Value == BIT_VALUE_LOW )/* If the Command bit is to be set low */
        {
            switch(Command)/* Checking for the Command bit */
            {
                case STPCCRD_CI_RESET:
                case STPCCRD_CI_SIZE_READ:
                case STPCCRD_CI_SIZE_WRITE:
                case STPCCRD_CI_HOST_CONTROL:
                    *(CIRegBaseAddress_p + PCCRD_CMDSTATUS) = PCCRD_RESET_COMMAND;
                    break;
                default:
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
            }/* Switch Ends */
        }
        else
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

#endif

    /* Free the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return (ErrorCode);

}/* STPCCRD_SetStatus */

/****************************************************************************
Name         : STPCCRD_Read()

Description  : Performs a Command Interface Read

Parameters   : Handle          Identifies connection with PCCRD Slot(A or B)
               *Buffer_p       Pointer to buffer data to be written,
               NumberToRead    Number of bytes to read,
               *NumberReadOK_p Pointer to the number of bytes read.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_INVALID_HANDLE       Handle value not Correct
               ST_ERROR_BAD_PARAMETER        Invalid parameter(s)
               ST_ERROR_DEVICE_BUSY          Device is currently busy
               STPCCRD_ERROR_BADREAD         Failure reading device

See Also     : STPCCRD_Write()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Read( STPCCRD_Handle_t   Handle,
                             U8                 *Buffer_p,
                             U16                *NumberReadOK_p )
{
    PCCRD_ControlBlock_t    *ControlBlkPtr_p;
    ST_ErrorCode_t          ErrorCode;
    U32                     OpenIndex;             /* Open Handle number */
    U32                     NumberReadOK;
#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    U32                     NumberToRead;
#else
    U8                      NumberToRead;
    U8                      NumberToRead2;
#endif
    U32                     MaxSize;
    U32                     BufferCount;
    U8                      StatusRegVal;
    U8                      StatusRE,StatusDA;

#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    U32                     CommandInterfaceBaseAddress = 0;
    volatile U8             *CIRegBaseAddress_p;   /* Base of Interface register */
#endif


    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Check if the pointers are Initialised */
    if ( (Buffer_p == NULL) || (NumberReadOK_p == NULL))
    {
        return ( ST_ERROR_BAD_PARAMETER );
    }

    MaxSize   = *NumberReadOK_p;               /* Max size */
    ErrorCode = ST_NO_ERROR;

    /* Initialising the variables with 0*/
    *NumberReadOK_p = 0;
    NumberReadOK = 0;
    NumberToRead = 0;

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                      /* Atomic sequence start */

    /* Check if the device is busy */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

    /* Check and switch to IO access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_IO_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex, PCCRD_IO_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
            return ( ErrorCode );
        }
    }

    /* Getting the Command Interface Base address */
#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
    switch( OpenIndex )
    {
        case STPCCRD_MOD_A:
            ReadByte((U8*)Status_Reg_Mod_A,&StatusRegVal);
            task_delay( ST_GetClocksPerSecond());
            break;

        case STPCCRD_MOD_B:
            ReadByte((U8*)Status_Reg_Mod_B,&StatusRegVal);
            task_delay( ST_GetClocksPerSecond());
            break;
    }

#else

#if !defined(ST_OSLINUX)
    CommandInterfaceBaseAddress = BANK_BASE_ADDRESS;
#endif
    CommandInterfaceBaseAddress += ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress;
    CIRegBaseAddress_p          =  (volatile U8*)CommandInterfaceBaseAddress;
    StatusRegVal                =  CIRegBaseAddress_p[PCCRD_CMDSTATUS];

#endif

    if ( StatusRegVal & PCCRD_DA )
    {

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
        switch( OpenIndex )
        {
            case STPCCRD_MOD_A:
                ReadByte((U8*)Size_Reg_0_Mod_A,&NumberToRead);
                task_delay( ST_GetClocksPerSecond()/100); /* need settling time */

                ReadByte((U8*)Size_Reg_1_Mod_A,&NumberToRead2);
                NumberToRead |= NumberToRead << SHIFT_EIGHT_BIT;
                task_delay( ST_GetClocksPerSecond()/100);
                break;

            case STPCCRD_MOD_B:
                ReadByte((U8*)Size_Reg_0_Mod_B,&NumberToRead);
                task_delay( ST_GetClocksPerSecond()/100); /* need settling time */
                ReadByte((U8*)Size_Reg_1_Mod_B,&NumberToRead2);

                NumberToRead |= NumberToRead << SHIFT_EIGHT_BIT;
                task_delay( ST_GetClocksPerSecond()/100);
                break;
        }

#else
        NumberToRead =  CIRegBaseAddress_p[PCCRD_SIZE_LS];
        NumberToRead |= (CIRegBaseAddress_p[PCCRD_SIZE_MS] << SHIFT_EIGHT_BIT);
#endif
        /* Control size acceptable */
        if ( ( MaxSize > 0 ) && ( NumberToRead > MaxSize ))
        {
            NumberReadOK = 0;
            ErrorCode    = STPCCRD_ERROR_BADREAD;
        }
        else
        {
            for ( BufferCount = 0 ; BufferCount < NumberToRead ; BufferCount++ )
            {

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
                switch( OpenIndex )
                {
                    case STPCCRD_MOD_A:
                        ReadByte((U8*)DataReg_Mod_A,&Buffer_p[BufferCount] );
                        break;
                    case STPCCRD_MOD_B:
                        ReadByte((U8*)DataReg_Mod_B,&Buffer_p[BufferCount] );
                        break;
                }
#else
                Buffer_p[BufferCount] = CIRegBaseAddress_p[PCCRD_DATA];

#endif

                NumberReadOK++;

            }/* For loop end */

#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
            switch( OpenIndex )
            {
                case STPCCRD_MOD_A:
                    /* Check the DA and RE status bits */
                    ReadByte((U8*)Status_Reg_Mod_A,&StatusDA );
                    ReadByte((U8*)Status_Reg_Mod_A,&StatusRE );
                    break;

                case STPCCRD_MOD_B:
                    /* Check the DA and RE status bits */
                    ReadByte((U8*)Status_Reg_Mod_B,&StatusDA );
                    ReadByte((U8*)Status_Reg_Mod_B,&StatusRE );
                    break;
            }

            StatusDA &= PCCRD_DA;
            StatusRE &= PCCRD_RE;
#else

            /* Check the DA and RE status bits */
            StatusDA = CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_DA;
            StatusRE = CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_RE;
#endif

            /* Before 1st byte read  DA = 1 & RE=0 */
            /* From 1st to last read DA = 0 & RE=1 */
            /* After last Read DA=0 & RE=0 otherwise error occurs */
            /*if ( ((StatusRE == 0) && (BufferCount < (NumberToRead - 1))) ||
                   ((StatusRE == PCCRD_RE) && (BufferCount ==(NumberToRead - 1))) ||
                    (StatusDA == PCCRD_DA))
            {
                  ErrorCode = STPCCRD_ERROR_BADREAD;
                  break;
            }*/
        }

    }
    else
    {
        NumberReadOK = NumberToRead;                      /* Report total number read */
        ErrorCode    = STPCCRD_ERROR_BADREAD;
    }

    *NumberReadOK_p = NumberReadOK;

    /* Free the Device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return( ErrorCode );

}/* STPCCRD_Read */

/****************************************************************************
Name         : STPCCRD_Write()

Description  : Performs a Command Interface Write

Parameters   : Handle             Identifies connection with PCCRD Slot(A or B)
               *Buffer_p          Pointer to buffer data to be written,
               NumberToWrite      Number of bytes to write,
               *NumberWrittenOK_p Pointer to the number of bytes written.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value is not correct
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_DEVICE_BUSY        Device is currently busy
               STPCCRD_ERROR_BADWRITE      Error during write
               ST_ERROR_TIMEOUT            Write operation timeout

See Also     : STPCCRD_Read()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_Write( STPCCRD_Handle_t    Handle,
                              U8                  *Buffer_p,
                              U16                 NumberToWrite,
                              U16                 *NumberWrittenOK_p)
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode;
    U32                         OpenIndex;            /* Open Handle number */
    U32                         NumberWrittenOK;
    U32                         BufferCount;
    U32                         NumberOfRetry;
    U8                          StatusWE;

#if !defined(ST_5105) && !defined(ST_7710) && !defined(ST_5107)
    U32                        CommandInterfaceBaseAddress = 0;
    volatile U8                 *CIRegBaseAddress_p;   /* Base of Interface register */
#endif

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Checking if the pointer arguments are NULL */
    if ( (Buffer_p == NULL) ||(NumberWrittenOK_p == NULL) )
    {
        return( ST_ERROR_BAD_PARAMETER);
    }

    *NumberWrittenOK_p = NumberWrittenOK = 0;         /* No bytes read so far */
    if ( NumberToWrite == 0 )                          /* No bytes to write */
    {
        return( ST_NO_ERROR );                        /* Nothing to do, OK */
    }

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                      /* Atomic sequence start */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock= TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );                    /* Atomic sequence end */

    /* Check and switch to IO access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_IO_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex, PCCRD_IO_ACCESS);

        if (ErrorCode != ST_NO_ERROR)
        {
             ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
             return ( ErrorCode );
        }
    }


#if defined(ST_5105) || defined(ST_7710) || defined(ST_5107)
    /* To avoid the problem in C106 core with lbinc instruction */
    switch( OpenIndex )
    {
        case STPCCRD_MOD_A:
            /* If some data are avalaible, Device is busy */
            if (*Status_Reg_Mod_A & PCCRD_DA)
            {
                 ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
                 return ( ST_ERROR_DEVICE_BUSY );
            }

            /* The HOST takes control before the transfer */
            *Command_Reg_Mod_A  = PCCRD_SETHC;

            /* If the module is free to accept data from the host, device can start the writing */
            if (! (*Status_Reg_Mod_A & PCCRD_FR))
            {
                /* Try again */
                /* If the module is free to accept data from the host, device can start the writing */
                if (! (*Status_Reg_Mod_A & PCCRD_FR))
                {
                    /* The module is busy */
                    *Command_Reg_Mod_A = PCCRD_RESETHC;  /* Return control to the module */
                    *NumberWrittenOK_p = 0 ;
                    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
                    return ( ST_ERROR_DEVICE_BUSY );
                }

            }

            /* The host writes the number of bytes he wants to send to the module */
            *Size_Reg_0_Mod_A  = (U8)(NumberToWrite & LOWER_BYTE);
            *Size_Reg_1_Mod_A  = (U8)(NumberToWrite >> SHIFT_EIGHT_BIT);

            ReadByte((U8*)Status_Reg_Mod_A,&StatusWE);
            StatusWE &= PCCRD_WE;

            /* writting the first byte unless only one byte to read */
            NumberOfRetry = 0;
            NumberWrittenOK = 0;

            while ( (NumberToWrite > 1) && (NumberWrittenOK == 0) && (NumberOfRetry < PCCRD_MAX_CI_WRITE_RETRY) )
            {
                *DataReg_Mod_A   = Buffer_p[0];
                ReadByte((U8*)Status_Reg_Mod_A,&StatusWE);
                StatusWE &= PCCRD_WE;
                if ( StatusWE == PCCRD_WE )
                {
                    NumberWrittenOK ++;
                }
                NumberOfRetry++;
            }

            if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
            {
                *NumberWrittenOK_p = NumberWrittenOK;

                /* Return control to the module */
                *Command_Reg_Mod_A = PCCRD_RESETHC;
                ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
                return ( STPCCRD_ERROR_BADWRITE );
            }

            for (BufferCount = 1 ; BufferCount < NumberToWrite - 1 ; BufferCount++)
            {
                *DataReg_Mod_A   = Buffer_p[BufferCount];
                StatusWE = (*Status_Reg_Mod_A & PCCRD_WE);

                /* If there is some writing errors, we stop the transfer */
                if ( StatusWE == 0 )
                {
                    *NumberWrittenOK_p = NumberWrittenOK;

                    /* Return control to the module */
                    *Command_Reg_Mod_A  = PCCRD_RESETHC;
                    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
                    return ( STPCCRD_ERROR_BADWRITE );
                }
                NumberWrittenOK ++;
            }

            NumberOfRetry = 0;
            do  /* Write the last byte till WE bit is low */
            {

                *DataReg_Mod_A   = Buffer_p[NumberToWrite-1];
                StatusWE = (*Status_Reg_Mod_A & PCCRD_WE);
            } while (( StatusWE == PCCRD_WE) && (++NumberOfRetry<PCCRD_MAX_CI_WRITE_RETRY));

            if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
            {
                *NumberWrittenOK_p = NumberWrittenOK;

                /* Return control to the module */
                *Command_Reg_Mod_A  = PCCRD_RESETHC;

                ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
                return ( STPCCRD_ERROR_BADWRITE );
            }

            NumberWrittenOK++;
            *NumberWrittenOK_p = NumberWrittenOK;

            /* Return control to the module */
            *Command_Reg_Mod_A  = PCCRD_RESETHC;
            break;

       case STPCCRD_MOD_B:
            /* If some data are avalaible, Device is busy */
            if (*Status_Reg_Mod_B & PCCRD_DA)
            {
                 ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
                 return ( ST_ERROR_DEVICE_BUSY );
            }

            /* The HOST takes control before the transfer */
            *Command_Reg_Mod_B  = PCCRD_SETHC;

            /* If the module is free to accept data from the host, device can start the writing */
            if (! (*Status_Reg_Mod_B & PCCRD_FR))
            {
                /* Try again */
                /* If the module is free to accept data from the host, device can start the writing */
                if (! (*Status_Reg_Mod_B & PCCRD_FR))
                {
                    /* The module is busy */
                    *Command_Reg_Mod_B = PCCRD_RESETHC;  /* Return control to the module */
                    *NumberWrittenOK_p = 0 ;
                    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
                    return ( ST_ERROR_DEVICE_BUSY );
                }

            }

            /* The host writes the number of bytes he wants to send to the module */
            *Size_Reg_0_Mod_B  = (U8)(NumberToWrite & LOWER_BYTE);
            *Size_Reg_1_Mod_B  = (U8)(NumberToWrite >> SHIFT_EIGHT_BIT);

            ReadByte((U8*)Status_Reg_Mod_B,&StatusWE);
            StatusWE &= PCCRD_WE;

            /* writting the first byte unless only one byte to read */
            NumberOfRetry = 0;
            NumberWrittenOK = 0;

            while ( (NumberToWrite > 1) && (NumberWrittenOK == 0) && (NumberOfRetry < PCCRD_MAX_CI_WRITE_RETRY) )
            {
                *DataReg_Mod_B   = Buffer_p[0];
                ReadByte((U8*)Status_Reg_Mod_B,&StatusWE);
                StatusWE &= PCCRD_WE;
                if ( StatusWE == PCCRD_WE )
                {
                    NumberWrittenOK ++;
                }
                NumberOfRetry++;
            }

            if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
            {
                *NumberWrittenOK_p = NumberWrittenOK;

                /* Return control to the module */
                *Command_Reg_Mod_B = PCCRD_RESETHC;
                ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
                return ( STPCCRD_ERROR_BADWRITE );
            }

            for (BufferCount = 1 ; BufferCount < NumberToWrite - 1 ; BufferCount++)
            {
                *DataReg_Mod_B   = Buffer_p[BufferCount];
                StatusWE = (*Status_Reg_Mod_B & PCCRD_WE);

                /* If there is some writing errors, we stop the transfer */
                if ( StatusWE == 0 )
                {
                    *NumberWrittenOK_p = NumberWrittenOK;

                    /* Return control to the module */
                    *Command_Reg_Mod_B  = PCCRD_RESETHC;
                    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
                    return ( STPCCRD_ERROR_BADWRITE );
                }
                NumberWrittenOK ++;
        }

        NumberOfRetry = 0;
        do  /* Write the last byte till WE bit is low */
        {

            *DataReg_Mod_B   = Buffer_p[NumberToWrite-1];
            StatusWE = (*Status_Reg_Mod_B & PCCRD_WE);
        } while (( StatusWE == PCCRD_WE) && (++NumberOfRetry<PCCRD_MAX_CI_WRITE_RETRY));

        if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
        {
            *NumberWrittenOK_p = NumberWrittenOK;

            /* Return control to the module */
            *Command_Reg_Mod_B  = PCCRD_RESETHC;

            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
            return ( STPCCRD_ERROR_BADWRITE );
        }

        NumberWrittenOK++;
        *NumberWrittenOK_p = NumberWrittenOK;

        /* Return control to the module */
       *Command_Reg_Mod_B  = PCCRD_RESETHC;
       break;

   }

#else  /* if !defined 5105 */

    /* Address of Command Interface Registers */
#if !defined(ST_OSLINUX)
    CommandInterfaceBaseAddress = BANK_BASE_ADDRESS;
#endif
    CommandInterfaceBaseAddress += ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].CommandBaseAddress;
    CIRegBaseAddress_p          =  (volatile U8*)CommandInterfaceBaseAddress;

    /* If some data are avalaible, Device is busy */
    if (CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_DA)
    {
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
        return ( ST_ERROR_DEVICE_BUSY );
    }

    /* The Host takes control before the transfer */
    CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_SETHC;

    /* If the module is free to accept data from the host, device can start the writing */
    if (! (CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_FR))
    {

        /* The module is busy */
        CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_RESETHC;  /* Return control to the module */
        *NumberWrittenOK_p = 0 ;
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
        return ( ST_ERROR_DEVICE_BUSY );

    }

    /* The host writes the number of bytes he wants to send to the module */
    CIRegBaseAddress_p[PCCRD_SIZE_LS] = (U8)(NumberToWrite & LOWER_BYTE);
    CIRegBaseAddress_p[PCCRD_SIZE_MS] = (U8)(NumberToWrite >> SHIFT_EIGHT_BIT);

    StatusWE = (CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_WE);

    /* writting the first byte unless only one byte to read */
    NumberOfRetry = 0;
    NumberWrittenOK = 0;

    while ( (NumberToWrite > 1) && (NumberWrittenOK == 0) && (NumberOfRetry < PCCRD_MAX_CI_WRITE_RETRY) )
    {
        CIRegBaseAddress_p[PCCRD_DATA] = Buffer_p[0];
        StatusWE = CIRegBaseAddress_p[PCCRD_CMDSTATUS];
        StatusWE = StatusWE & PCCRD_WE;
        if ( StatusWE == PCCRD_WE )
        {
            NumberWrittenOK ++;
        }
        NumberOfRetry++;
    }

    if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
    {
        *NumberWrittenOK_p = NumberWrittenOK;

        /* Return control to the module */
        CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_RESETHC;
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
        return ( STPCCRD_ERROR_BADWRITE );
    }

    for (BufferCount = 1 ; BufferCount < NumberToWrite - 1 ; BufferCount++)
    {
        CIRegBaseAddress_p[PCCRD_DATA] = Buffer_p[BufferCount];
        StatusWE = (CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_WE);

        /* If there is some writing errors, we stop the transfer */
        if ( StatusWE == 0 )
        {
            *NumberWrittenOK_p = NumberWrittenOK;
            /* Return control to the module */
            CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_RESETHC;
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock =FALSE;
            return ( STPCCRD_ERROR_BADWRITE );
        }
        NumberWrittenOK ++;
    }

    NumberOfRetry = 0;
    do  /* Write the last byte till WE bit is low */
    {

        CIRegBaseAddress_p[PCCRD_DATA] = Buffer_p[NumberToWrite-1];
        StatusWE = (CIRegBaseAddress_p[PCCRD_CMDSTATUS] & PCCRD_WE);

    } while (( StatusWE == PCCRD_WE)
        && (++NumberOfRetry<PCCRD_MAX_CI_WRITE_RETRY));

    if (NumberOfRetry == PCCRD_MAX_CI_WRITE_RETRY)
    {
        *NumberWrittenOK_p = NumberWrittenOK;
        /* Return control to the module */
        CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_RESETHC;
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
        return ( STPCCRD_ERROR_BADWRITE );
    }

    NumberWrittenOK++;
    *NumberWrittenOK_p = NumberWrittenOK;

    /* Return control to the module */
    CIRegBaseAddress_p[PCCRD_CMDSTATUS] = PCCRD_RESETHC;

#endif

    /*Free the Device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return (ST_NO_ERROR);

}/* STPCCRD_Write */

/****************************************************************************
Name         : STPCCRD_ControlTS

Description  : Enables or Disables the Transport Stream Interface of the Module

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)
               Control  Bypass control mode

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle value not correct
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               STPCCRD_ERROR_NO_MODULE     Module is not present(In Hot PlugIn case)
               ST_ERROR_DEVICE_BUSY        Device is currently Busy
               STPCCRD_ERROR_I2C_WRITE     Error in writing the I2C Interface
               STPCCRD_ERROR_I2C_READ      Error in reading the I2C Interface

*****************************************************************************/

ST_ErrorCode_t STPCCRD_ControlTS( STPCCRD_Handle_t Handle,STPCCRD_TSBypassMode_t Control)
{
    PCCRD_ControlBlock_t     *ControlBlkPtr_p;
    ST_ErrorCode_t           ErrorCode;
    U32                      OpenIndex;                  /* Open Handle number */

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle, &ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    /* Check for the presence of Module Only In case of Hot Plugin Support */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ModPresenceStatus == MOD_ABSENT)
    {
        return(STPCCRD_ERROR_NO_MODULE);
    }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                    /* Atomic sequence start */

    /* Check for device in use */
    if (ControlBlkPtr_p->DeviceLock)
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );              /* Atomic sequence end */
        return(ST_ERROR_DEVICE_BUSY);
    }

    /* Lock the device */
    ControlBlkPtr_p->DeviceLock = TRUE;
    STOS_SemaphoreSignal(g_PCCRDAtomicSem_p);                    /* Atomic sequence end */

    if (Control == STPCCRD_ENABLE_TS_PROCESSING)
    {
        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:

#if defined(STV0701) || defined(ST_5105) || defined(ST_5107) || defined(STPCCRD_USE_ONE_MODULE)
                ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_A_ONLY);
#else
                if (ControlBlkPtr_p->MOD_B_TS_PROCESSING_ENABLED == TRUE)
                {
                    /* Do the TS setting for Both Card A and B Situation */
                    ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_A_AND_B);
                }
                else
                {
                    /* Do the TS setting for Card A only Situation */
                    ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_A_ONLY);
                }
#endif
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControlBlkPtr_p->MOD_A_TS_PROCESSING_ENABLED = TRUE;
                }
                break;

            case STPCCRD_MOD_B :
                if (ControlBlkPtr_p->MOD_A_TS_PROCESSING_ENABLED == TRUE)
                {
                    /* Do the TS setting for Both Card A and B Situation */
                    ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_A_AND_B);
                }
                else
                {
                    /* Do the TS setting for Card B only Situation */
                    ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_B_ONLY);
                }
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControlBlkPtr_p->MOD_B_TS_PROCESSING_ENABLED = TRUE;
                }
                break;

            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* Switch Ends */
    }
    else
    {
        if (Control == STPCCRD_DISABLE_TS_PROCESSING)
        {
            switch (OpenIndex)
            {
                case STPCCRD_MOD_A :
#if defined(STV0701) || defined(ST_5105) || defined(ST_5107) || defined(STPCCRD_USE_ONE_MODULE)
                    ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,NO_PCCARD);
#else
                    if (ControlBlkPtr_p->MOD_B_TS_PROCESSING_ENABLED == FALSE)
                    {
                        /* Do the TS setting for No Card Situation */
                        ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,NO_PCCARD);
                    }
                    else
                    {
                        /* Do the TS setting for Card B Only Situation */
                        ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_B_ONLY);
                    }
#endif
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControlBlkPtr_p->MOD_A_TS_PROCESSING_ENABLED=FALSE;
                    }
                    break;

                case STPCCRD_MOD_B :
                    if (ControlBlkPtr_p->MOD_A_TS_PROCESSING_ENABLED == FALSE)
                    {
                        /* Do the TS setting for No Card Situation */
                        ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,NO_PCCARD);
                    }
                    else
                    {
                        /* Do the TS setting for Card A Only Situation */
                        ErrorCode = PCCRD_ControlTS(ControlBlkPtr_p,PCCARD_A_ONLY);
                    }
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControlBlkPtr_p->MOD_B_TS_PROCESSING_ENABLED = FALSE;
                    }
                    break;


                default:
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }/* Switch Ends */
        }
        else
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Free the Device */
    ControlBlkPtr_p->DeviceLock = FALSE;

    return ( ErrorCode );

}/* STPCCRD_ControlTS */

/****************************************************************************
Name         : STPCCRD_CheckCIS()

Description  : Analyse the CIS of the DVB CI Module to check if it is
               compliant with EN50221 specification or not.
               Also extracts out the Information about the COR Register

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               STPCCRD_ERROR_NO_MODULE       Module is not present(Only In case
                                             if Hot PlugIn is Enabled)
               ST_ERROR_DEVICE_BUSY          Device is busy in serving other client
               ST_ERROR_INVALID_HANDLE       Handle value is not Correct
               STPCCRD_ERROR_CIS_READ        Either CIS is not properly read or DVB CI is not
                                             EN50221 compliant

See Also     : STPCCRD_WriteCOR()

*****************************************************************************/

ST_ErrorCode_t STPCCRD_CheckCIS ( STPCCRD_Handle_t Handle )
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    U32                         OpenIndex;                  /* Open Handle number */
    ST_ErrorCode_t              ErrorCode;
    PCCRD_Tupple_t              CurrentTupple;
    U16                         OffsetCurrentTupple;
    U8                          SubtTuppleTag, SubtTuppleLen,FunctionEntry ;
    U8*                         SubtTuppleValue_p;
    U8                          TpceIf = 0;
    U8                          Tpcefs = 0;
    U8                          PowerDescriptionBits;
    U8                          CORValue = 0;
    U32                         StatusCheckCIS = 0;
    U32                         RetryCount;

    /* Variable related to the PC Card Specification Vol 2 */
    U8                          TPCC_SZ, TPCC_LAST, TPCC_RMSK, TPCC_RFSZ, TPCC_RMSZ, TPCC_RASZ;
    U32                         TPCC_RADR = 0;
    U8*                         TPCE_SUB_TUPPLE_p;
    U8                          TPCE_INDX, TPCE_INTFACE, TPCE_DEFAULT;
    U8                          TPCE_PD_SPSB;   /* Structure parameter selection byte */
    U8                          TPCE_PD_SPD;    /* Structure parameter definition */
    U8                          TPCE_PD_SPDX;   /* Extension structure parameter definition */
    U8                          *TPCE_TD_p;
    U8                          TPCE_IO_RANGE;
    U8                          TPCE_IO_BM;     /* I/O bus mapping */
    U8                          TPCE_IO_AL;     /* I/O address line */
    U8                          TPCE_IO_RDB;    /* I/O Range descriptor byte */
    U8                          TPCE_IO_SOL;    /* Size of Length I/O address range*/
    U8                          TPCE_IO_SOA;    /* Size of Address I/O address range*/
    U8                          TPCE_IO_NAR;    /* Number of I:O address range */
    U8                          *TPCE_IO_RDFFCI_p;
    U8                          *TPCE_IO_p;
    U8                          *TPCE_IR_p;
    U8                          *TPCE_MS_p;
    U32                         TPCE_IO_BLOCK_START;
    U32                         TPCE_IO_BLOCK_SIZE;
    U8                          *TPCE_PD_p;

    /* Check for Invalid Handle  */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    /*Check for the presence of Module */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ModPresenceStatus==MOD_ABSENT)
    {
        return(STPCCRD_ERROR_NO_MODULE);
    }
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );      /* Atomic sequence start */

    /* Check if the device in use  */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );  /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;
    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );

#if defined(ST_5105) || defined(ST_5107)

    task_delay( ST_GetClocksPerSecond());

#endif


    /* Check and switch to Attribute access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_ATTRIB_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p, OpenIndex,PCCRD_ATTRIB_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock= FALSE;
            return ( ErrorCode );
        }
    }

    OffsetCurrentTupple = 0;
    for (RetryCount = 0; RetryCount < PCCRD_MAX_CHECK_CIS_RETRY;RetryCount++)
    {

        task_delay( ST_GetClocksPerSecond()/100);

        ErrorCode = PCCRD_ReadTupple(&(ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex]), OffsetCurrentTupple, &CurrentTupple);
        if (ErrorCode == ST_NO_ERROR)
        {
            break;
        }
    }

    /* Looping till we get the last Tupple or any error */
    while ( (CurrentTupple.TuppleTag   != CISTPL_END)
          && (CurrentTupple.TuppleLink != 0)
          && (StatusCheckCIS != STATUS_CIS_CHECKED))
    {
        switch (CurrentTupple.TuppleTag) /* Check for the tupple tag */
        {
            case CISTPL_DEVICE_OA :break;

            case CISTPL_DEVICE_OC :break;

            case CISTPL_VERS_1 :
            {
                if ( (CurrentTupple.TuppleData[IND_TPLLV1_MINOR] != VAL_TPLLV1_MINOR )
                   ||(CurrentTupple.TuppleData[IND_TPLLV1_MAJOR]!= VAL_TPLLV1_MAJOR ))
                {
                    STTBX_Print(("Invalid tupple CISTPL_VERS_1(%02X) LINK=%d\n",
                                CurrentTupple.TuppleTag,CurrentTupple.TuppleLink));
                    break;
                }
                StatusCheckCIS |= STATUS_CIS_VERS_1_CHECKED;

                CurrentTupple.TuppleData[CurrentTupple.TuppleLink]  = 0;
                break;
            } /* end of case CISTPL_VERS_1 */

            case CISTPL_MANFID :
            {
                if ( (CurrentTupple.TuppleLink != SZ_TPLLMANFID))
                {
                    STTBX_Print(("Invalid tupple CISTPL_MANFID(%02X) LINK=%d\n",
                                CurrentTupple.TuppleTag,CurrentTupple.TuppleLink));
                    break;
                }
                StatusCheckCIS |= STATUS_CIS_MANFID_CHECKED;
                break;
            }
            case CISTPL_CONFIG :
            {
                if ( (CurrentTupple.TuppleLink < 5 ))
                {
                    STTBX_Print(("Invalid tupple CISTPL_CONFIG(%02X) LINK=%d\n",
                                CurrentTupple.TuppleTag,CurrentTupple.TuppleLink));
                    break;
                }
                TPCC_SZ     = CurrentTupple.TuppleData[0];
                TPCC_LAST   = CurrentTupple.TuppleData[1];
                TPCC_RFSZ   = TPCC_SZ & MASK_TPCC_RFSZ;
                TPCC_RMSZ   = TPCC_SZ & MASK_TPCC_RMSZ;
                TPCC_RASZ   = TPCC_SZ & MASK_TPCC_RASZ;

                switch ( TPCC_RASZ )
                {
                    case TPCC_RADR_1_BYTE :
                    {
                        TPCC_RADR = CurrentTupple.TuppleData[2];
                        break;
                    }
                    case TPCC_RADR_2_BYTE :
                    {
                        TPCC_RADR = CurrentTupple.TuppleData[3];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[2];
                        break;
                    }

                    case TPCC_RADR_3_BYTE :
                    {
                        TPCC_RADR = CurrentTupple.TuppleData[4];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[3];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[2];
                        break;
                    }

                    case TPCC_RADR_4_BYTE :
                    {
                        TPCC_RADR = CurrentTupple.TuppleData[5];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[4];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[3];
                        TPCC_RADR <<= SHIFT_EIGHT_BIT;
                        TPCC_RADR |= CurrentTupple.TuppleData[2];
                        break;
                    }
                } /* end switch TPCC_RASZ */
                if ( TPCC_RADR > MAX_COR_BASE_ADDRESS)
                {
                    STTBX_Print(("Invalid tupple CISTPL_CONFIG(%02X) LINK=%d\n TPCC_RADR[%4X] > 0x0FFE\n",
                                CurrentTupple.TuppleTag,CurrentTupple.TuppleLink,TPCC_RADR));
                    break;
                }
                else
                {
                    STTBX_Print(("Valid tupple CISTPL_CONFIG TPCC_RADR %x \n",TPCC_RADR));
                }
                if ( TPCC_RMSZ != 0 )
                {
                    STTBX_Print(("Invalid tupple CISTPL_CONFIG(%02X) LINK=%d\n More than 1 Configuration register[%d]\n",
                                CurrentTupple.TuppleTag,CurrentTupple.TuppleLink,TPCC_RMSZ+1));
                    break;
                }

                TPCC_RMSK         = CurrentTupple.TuppleData[TPCC_RASZ+3];
                SubtTuppleTag     = CurrentTupple.TuppleData[TPCC_RASZ+4];
                SubtTuppleLen     = CurrentTupple.TuppleData[TPCC_RASZ+5];
                SubtTuppleValue_p = CurrentTupple.TuppleData + TPCC_RASZ + 6;

                FunctionEntry = SubtTuppleTag;
                STTBX_Print(("FunctionEntry = %02X\n", FunctionEntry));

                if ( ((strcmp(STCF_LIB, (ST_String_t)SubtTuppleValue_p+ 2 ))==0) &&
                        (SubtTuppleValue_p[1]== STCI_IFN_HIGH) && (SubtTuppleValue_p[0] == STCI_IFN_LOW) )
                {
                    STTBX_Print(("TPCC_LAST = %02X STCI_IFN=%02X%02X\n%s\n",
                                TPCC_LAST, SubtTuppleValue_p[1], SubtTuppleValue_p [0], SubtTuppleValue_p+ 2 ));
                    StatusCheckCIS |= STATUS_CIS_CONFIG_CHECKED;

                }
                break;
            }

            case CISTPL_CFTABLE_ENTRY :
            {
                TPCE_INDX   = CurrentTupple.TuppleData[ 0 ];
                TPCE_INTFACE= TPCE_INDX & MASK_7_BIT;
                TPCE_DEFAULT= TPCE_INDX & MASK_6_BIT;
                TPCE_INDX   = TPCE_INDX & MASK_TPCE_INDX;

                if ( TPCE_INTFACE )
                {
                    TpceIf = CurrentTupple.TuppleData[ 1 ];
                    Tpcefs = CurrentTupple.TuppleData[ 2 ];
                    TPCE_PD_p = CurrentTupple.TuppleData +3 ;
                }
                else
                {
                    Tpcefs = CurrentTupple.TuppleData[ 1 ];
                    TPCE_PD_p = CurrentTupple.TuppleData +2 ;
                }

                STTBX_Print(("\tField Selection %x \n",Tpcefs ));
                PowerDescriptionBits =  Tpcefs & (MASK_1_BIT |MASK_0_BIT);

                /* Check power description */
                while( PowerDescriptionBits > 0 )
                {

                    STTBX_Print(("\tpower description %02X, nb=%d \n", *TPCE_PD_p, PowerDescriptionBits));
                    for ( TPCE_PD_SPSB = *TPCE_PD_p++; TPCE_PD_SPSB> 0; TPCE_PD_SPSB>>= 1 )
                    {
                        if ( TPCE_PD_SPSB & MASK_0_BIT )
                        {
                            TPCE_PD_SPD = *TPCE_PD_p++;
                            STTBX_Print(("\t\tpower description field %02X\n", TPCE_PD_SPD ));
                            while ( *TPCE_PD_p & MASK_7_BIT )
                            {
                                TPCE_PD_SPDX = *TPCE_PD_p++;
                            }
                        }
                    }
                    PowerDescriptionBits--;
                }

                TPCE_TD_p = TPCE_PD_p; /* on n est plus sur un power descriptor mais sur un time descriptor */
                TPCE_PD_p = NULL;

                /* timing */
                if ( Tpcefs & MASK_2_BIT )
                {
                    switch ( (*TPCE_TD_p) & MASK_RW_SCALE )
                    {
                            case NOT_READY_WAIT: /* neither ready scale nor wait scale definition */
                                TPCE_IO_p = TPCE_TD_p + 1;
                                break;
                            case WAIT_SCALE_0 :
                            case WAIT_SCALE_1 :
                            case WAIT_SCALE_2 : /* only wait scale definition */
                                TPCE_IO_p = TPCE_TD_p + 2;
                                break;

                            case READY_SCALE_0 :
                            case READY_SCALE_1 :
                            case READY_SCALE_2 :
                            case READY_SCALE_3 :
                            case READY_SCALE_4 :
                            case READY_SCALE_5 :
                            case READY_SCALE_6: /* only ready scale definition */
                                TPCE_IO_p = TPCE_TD_p + 2;
                                break;
                            default :
                                TPCE_IO_p = TPCE_TD_p + 3;
                                break;
                    }
                }
                else
                {
                    TPCE_IO_p = TPCE_TD_p;
                    STTBX_Print(("\t Timing Descriptor : Not There\n"));
                }
                /* I/O space */
                if ( Tpcefs & MASK_3_BIT  )
                {
                    TPCE_IR_p     = TPCE_IO_p + 1;
                    TPCE_IO_RANGE = (*TPCE_IO_p)& MASK_7_BIT;
                    TPCE_IO_BM    = ((*TPCE_IO_p)& (MASK_5_BIT|MASK_6_BIT)) >> 5; /* bus mapping */
                    TPCE_IO_AL    = (*TPCE_IO_p)& MASK_IO_ADDLINES;

                    if ( TPCE_IO_RANGE )
                    {
                        int TpceLoop;
                        TPCE_IO_RDB = TPCE_IO_p[1];               /* I/O Range descriptor byte */
                        TPCE_IO_SOL = (TPCE_IO_RDB & (MASK_7_BIT|MASK_6_BIT)) >> 6; /* Size of Length I/O address range*/
                        TPCE_IO_SOA = (TPCE_IO_RDB & (MASK_5_BIT|MASK_4_BIT)) >> 4; /* Size of Address I/O address range*/
                        TPCE_IO_NAR = (TPCE_IO_RDB & (MASK_3_BIT|MASK_2_BIT|MASK_1_BIT|MASK_0_BIT)) +1;  /* Number of I:O address range */

                        TPCE_IO_RDFFCI_p = TPCE_IO_p + 2;
                        for ( TpceLoop = 0 ; TpceLoop < TPCE_IO_NAR; TpceLoop++)
                        {

                            TPCE_IO_BLOCK_START = TPCE_IO_RDFFCI_p[(TPCE_IO_SOL+TPCE_IO_SOA)*TpceLoop];
                            if ( TPCE_IO_SOA > 1 )
                            {
                                TPCE_IO_BLOCK_START |= ( (U32)TPCE_IO_RDFFCI_p[(TPCE_IO_SOL+TPCE_IO_SOA)*TpceLoop+1])<<SHIFT_EIGHT_BIT;
                            }
                            TPCE_IO_BLOCK_SIZE = TPCE_IO_RDFFCI_p[(TPCE_IO_SOL+TPCE_IO_SOA)*TpceLoop+ TPCE_IO_SOA] +1;

                        }
                        TPCE_IR_p =  TPCE_IO_RDFFCI_p + (TPCE_IO_SOL+TPCE_IO_SOA)*TPCE_IO_NAR ;
                    }
                    else
                    {
                        TPCE_IR_p = TPCE_IO_p+1;
                    }

                }
                else
                {
                    TPCE_IR_p = TPCE_IO_p;
                    TPCE_IO_p = NULL;
                    STTBX_Print(("\tNot I/O space descriptor, Tpcefs = %02X\n",Tpcefs));
                }

                /* IRQ */
                if ( Tpcefs & MASK_4_BIT  )
                {
                    TPCE_MS_p = TPCE_IR_p+1;
                    STTBX_Print(("IRQ description is present\n"));

                }
                else
                {
                    STTBX_Print(("IRQ description is not present\n"));
                    TPCE_MS_p = TPCE_IR_p;
                }

                /* Memory space */
                switch ( (Tpcefs &( MASK_6_BIT | MASK_6_BIT) )>>5 )
                {
                    case 0 :
                        TPCE_SUB_TUPPLE_p = TPCE_MS_p;
                        TPCE_MS_p = NULL;
                        break;

                    case 1 :
                        TPCE_SUB_TUPPLE_p = TPCE_MS_p + 1;
                        break;

                    case 2 :
                        TPCE_SUB_TUPPLE_p = TPCE_MS_p + 2;
                        break;

                    case 3 :
                        TPCE_SUB_TUPPLE_p = TPCE_MS_p + 3;
                        break;
                }


                if ((TpceIf == TPCE_IF)&& (*TPCE_IO_p == TPCE_IO))
                {
                    CORValue = TPCE_INDX;

                    STTBX_Print(("Function COR(%02X) at(0x%x) for entry %s\n", CORValue,TPCC_RADR,TPCE_SUB_TUPPLE_p+2 ));

                    StatusCheckCIS |= STATUS_CIS_CFTENTRY_CHECKED;

                }
                else
                {
                    STTBX_Print(("Other Function COR(%02X) for entry %s\n", TPCE_INDX, TPCE_SUB_TUPPLE_p+2 ));
                }
                break;

            }

            default :break;

        }   /* end switch */

        OffsetCurrentTupple += 2 + CurrentTupple.TuppleLink;
        ErrorCode = PCCRD_ReadTupple(&(ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex]), OffsetCurrentTupple, &CurrentTupple);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
            return (ErrorCode);
        }

    } /* end while */

    if (StatusCheckCIS != STATUS_CIS_CHECKED)
    {
        STTBX_Print(("STPCCRD_CheckCIS Not Completed 0x%x\n",StatusCheckCIS));
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].COR_Address        = 0;
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ConfigurationIndex = 0;
        ErrorCode = STPCCRD_ERROR_CIS_READ;
    }
    else
    {
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].COR_Address = TPCC_RADR;
        ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ConfigurationIndex = CORValue;
        ErrorCode = ST_NO_ERROR;
    }

    /* Free the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return ( ErrorCode );

} /* STPCCRD_CheckCIS */

/****************************************************************************
Name         : STPCCCRD_WriteCOR()

Description  : Performs a write on Attribute Memory

Parameters   : Handle Identifies connection with PCCRD Slot(A or B)

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               ST_ERROR_INVALID_HANDLE     Handle Value is not correct
               ST_ERROR_DEVICE_BUSY        Device is currently busy

See Also     : STPCCRD_CheckCIS()
 ****************************************************************************/

ST_ErrorCode_t STPCCRD_WriteCOR( STPCCRD_Handle_t Handle )
{
    PCCRD_ControlBlock_t        *ControlBlkPtr_p;
    ST_ErrorCode_t              ErrorCode;
    U32                         OpenIndex;            /* Open Handle number */
    U8                          ConfigIndex;
    U32                         MemoryBaseAddress=0;
    U8                          *CORMemAddress_p;

    /* Perform Handle validity check */
    if ( IsHandleInvalid( Handle,&ControlBlkPtr_p, &OpenIndex ) )
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Ensure area to be accessed doesn't overflow the segment */
    STOS_SemaphoreWait( g_PCCRDAtomicSem_p );                /* Atomic sequence start */

    /* Check if the Device in use by another caller */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock )
    {
        STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );          /* Atomic sequence end */
        return( ST_ERROR_DEVICE_BUSY );
    }

    /* Lock the device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = TRUE;

    STOS_SemaphoreSignal( g_PCCRDAtomicSem_p );              /* Atomic sequence end */

    /* Check and switch to Memory access mode */
    if (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState != PCCRD_ATTRIB_ACCESS)
    {
        ErrorCode = PCCRD_ChangeAccessMode( ControlBlkPtr_p,OpenIndex,PCCRD_ATTRIB_ACCESS);
        if (ErrorCode != ST_NO_ERROR)
        {
            ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;
            return ( ErrorCode );
        }
    }

    /* Getting Configuration Option Register Address */
#if !defined(ST_OSLINUX)
    MemoryBaseAddress =  BANK_BASE_ADDRESS;
#endif

    MemoryBaseAddress += (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AttributeBaseAddress);

#if defined(ST_7100) || defined(ST_7109)   /* Shift 2 times for A2->A0 */
    MemoryBaseAddress += (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].COR_Address<<2);
#else
    MemoryBaseAddress += (ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].COR_Address);
#endif

    CORMemAddress_p   = (U8*)MemoryBaseAddress;
    ConfigIndex       = ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].ConfigurationIndex;

    /* Writing the Config Index vlaue to the COR */
    *CORMemAddress_p = ConfigIndex;

    /*  Free the Device */
    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].DeviceLock = FALSE;

    return( ST_NO_ERROR );

}/* STPCCRD_WriteCOR */
/* End of File */
