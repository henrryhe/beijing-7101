/*****************************************************************************

File Name  : dvmfs.h

Description: Filesystem Callback functions for STPRM

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DVMFS_H
#define __DVMFS_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* File system callback functions passed to STPRM */
U32 STDVMi_FileOpen     (STPRM_Handle_t PRM_Handle, U8 *FileName,   STPRM_FSflags_t  Flags, S64 FileSizeInPackets);
U32 STDVMi_FileRead     (STPRM_Handle_t PRM_Handle, U32 FileHandle, U8  *Buffer, U32 SizeInPackets);
U32 STDVMi_FileWrite    (STPRM_Handle_t PRM_Handle, U32 FileHandle, U8  *Buffer, U32 SizeInPackets);
U32 STDVMi_FileSeek     (STPRM_Handle_t PRM_Handle, U32 FileHandle, S64  PacketOffset, STPRM_FSflags_t Flags);
U32 STDVMi_FileClose    (STPRM_Handle_t PRM_Handle, U32 FileHandle);


/* STDVM internal functions */
ST_ErrorCode_t STDVMi_UpdateStreamInfoToDisk(STDVMi_Handle_t *Handle_p);
ST_ErrorCode_t STDVMi_UpdateStreamChangePIDs(STDVMi_Handle_t *Handle_p, U32 NumberOfPids, STDVM_StreamData_t *Pids_p);
ST_ErrorCode_t STDVMi_ReadProgramInfo(STDVMi_Handle_t *Handle_p);
ST_ErrorCode_t STDVMi_SetStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p);
ST_ErrorCode_t STDVMi_GetStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_StreamInfo_t *StreamInfo_p);
ST_ErrorCode_t STDVMi_RemoveStreamInfo(STDVMi_Handle_t *Handle_p, char *StreamName);
ST_ErrorCode_t STDVMi_RemoveStream (STDVMi_Handle_t *Handle_p, char *StreamName);
ST_ErrorCode_t STDVMi_CopyStream(STDVMi_Handle_t   *Handle_p,
                                 char              *OldStreamName,
                                 U32                StartTimeInMs,
                                 U32                EndTimeInMs,
                                 char              *NewStreamName);
ST_ErrorCode_t STDVMi_CropStream(STDVMi_Handle_t   *Handle_p,
                                 char              *StreamName,
                                 U32                StartTimeInMs,
                                 U32                EndTimeInMs);
ST_ErrorCode_t STDVMi_SwitchCircularToLinear(STDVMi_Handle_t *FileHandle_p);
ST_ErrorCode_t STDVMi_CheckKey(STDVMi_Handle_t *Handle_p, char *StreamName, STDVM_Key_t Key);

#endif /*__DVMFS_H*/

/* EOF --------------------------------------------------------------------- */

