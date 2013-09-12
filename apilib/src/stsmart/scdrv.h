/*****************************************************************************
File Name   : scdrv.h

Description : Internal driver routines.

Copyright (C) 2006 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCDRV_H
#define __SCDRV_H

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t SMART_ProcessATR(SMART_ControlBlock_t *Smart_p);
ST_ErrorCode_t SMART_ProcessProcedureBytes(SMART_ControlBlock_t *Smart_p);
ST_ErrorCode_t SMART_InitiatePTSCommand(SMART_ControlBlock_t *Smart_p,
                                               U8 *PtsBytes_p);
ST_ErrorCode_t SMART_InitiateATR(SMART_ControlBlock_t *Smart_p, BOOL ActiveHigh);
void SMART_Deactivate(SMART_ControlBlock_t *Smart_p);
void SMART_CompleteOperation(SMART_ControlBlock_t *Smart_p,
                                    SMART_Operation_t *Operation_p,
                                    ST_ErrorCode_t ErrorCode);
void SMART_ClearStatus(SMART_ControlBlock_t *Smart_p);
void SMART_SetETU(SMART_ControlBlock_t *Smart_p);
U8 SMART_PIOBitFromBitMask(U8 BitMask);
ST_ErrorCode_t SMART_SetParams(SMART_ControlBlock_t *Smart_p);
/* Figure out clock frequency from etu and baudrate */
void SMART_SetSCClock(SMART_ControlBlock_t *Smart_p,
                      U32 Etu,
                      U32 BitRate,
                      U32 *ActualClk_p);
/* Just sets the clock, no interpretation */
void SMART_SetClockFrequency(SMART_ControlBlock_t *Smart_p,
                             U32 RequestedClk,
                             U32 *ActualClk_p);
ST_ErrorCode_t SMART_CheckTransfer(SMART_ControlBlock_t *Smart_p,
                                   U8 *Buffer_p);

/* Internal task */
void SMART_EventManager(message_queue_t *MessageQueue_p);

/* Callbacks */
void SMART_IoHandler(ST_ErrorCode_t ErrorCode,
                     void *);
void SMART_DetectHandler(STPIO_Handle_t Handle,
                         STPIO_BitMask_t BitMask);
#endif /* __SCDRV_H */

/* End of scdrv.h */
