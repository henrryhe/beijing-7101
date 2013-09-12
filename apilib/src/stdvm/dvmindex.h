/*****************************************************************************

File Name  : dvmindex.h

Description: Callback functions for STPRM header

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DVMINDEX_H
#define __DVMINDEX_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STDVMi_RecordCallbackStart (STPRM_Handle_t PRM_Handle);
ST_ErrorCode_t STDVMi_RecordCallbackPacket(STPRM_Handle_t PRM_Handle, U32 PacketTime, S64 PacketPosition);
ST_ErrorCode_t STDVMi_RecordCallbackStop  (STPRM_Handle_t PRM_Handle);
ST_ErrorCode_t STDVMi_RecordCallbackGetTime(STPRM_Handle_t  PRM_Handle,
					                        U32            *StartTimeInMs,
					                        U32            *EndTimeInMs);

ST_ErrorCode_t STDVMi_PlayCallbackStart   (STPRM_Handle_t PRM_Handle);
ST_ErrorCode_t STDVMi_PlayCallbackPacket  ( STPRM_Handle_t  PRM_Handle,
                                            S64             PacketPosition,
                                            U32            *NbPacketsToInject,
                                            U32            *FirstPacketTimeInMs,
                                            U32            *TimeDurationInMs);
ST_ErrorCode_t STDVMi_PlayCallbackJump    ( STPRM_Handle_t  PRM_Handle,
                                            U32             PacketTime,
                                            S32             JumpInMs,
                                            S64            *NewPacketPosition);
ST_ErrorCode_t STDVMi_PlayCallbackGetTime ( STPRM_Handle_t  PRM_Handle,
                                            S64            *CurrentPacketPosition,
                                            U32            *StartTimeInMs,
                                            U32            *CurrentTimeInMs,
                                            U32            *EndTimeInMs);
ST_ErrorCode_t STDVMi_PlayCallbackStop    (STPRM_Handle_t PRM_Handle);

/* STDVM internal functions */
ST_ErrorCode_t STDVMi_GetStreamTime(STDVMi_Handle_t *Handle_p, S64 Position, U32 *TimeInMs_p);
ST_ErrorCode_t STDVMi_GetStreamPosition(STDVMi_Handle_t *Handle_p, U32 TimeInMs, U64 *Position_p);
ST_ErrorCode_t STDVMi_GetTimeAndSize(STDVMi_Handle_t *Handle_p, U32 *StartTimeInMs_p, U32 *DurationInMs_p, S64 *NbOfPackets_p);
U32            STDVMi_GetStreamRateBpmS(STDVMi_Handle_t *Handle_p);
ST_ErrorCode_t STDVMi_CopyIndexFileAndGetStreamPosition(STDVMi_Handle_t    *OldFileHandle_p,
                                                        STDVMi_Handle_t    *NewFileHandle_p,
                                                        U32                 StartTimeInMs,
                                                        U32                 EndTimeInMs,
                                                        U64                *StartPosition_p,
                                                        U64                *EndPosition_p);

#endif /*__DVMINDEX_H*/

/* EOF --------------------------------------------------------------------- */

