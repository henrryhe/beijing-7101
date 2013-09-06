/*****************************************************************************
File Name   : dslib.h

Description : API Interfaces for the DS Smartcard library.

Copyright (C) 1999 STMicroelectronics

Reference   :

DS Library Reference Manual, De La Rue Card Systems
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DSLIB_H
#define __DSLIB_H

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Driver constants, required by the driver model */
#define DS_DRIVER_ID               50
#define DS_DRIVER_BASE             (DS_DRIVER_ID << 16)

/* Error constants */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Smartcard handle */
typedef U32 DS_Handle_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t DS_Certify(const DS_Handle_t Handle, const U16 Offset, const U8 Data[8],
                          U16 Status[2]);

ST_ErrorCode_t DS_ChangeCode(const DS_Handle_t Handle, const U16 CodeNum,
                             const U8 OldCode[8], U8 NewCode[8],
                             U8 Status[2]);

ST_ErrorCode_t DS_Close(const DS_Handle_t Handle);

ST_ErrorCode_t DS_CreateBinary(const DS_Handle_t Handle, const U16 Length,
                               const U8 Data[255], U8 Status[2]);

ST_ErrorCode_t DS_CreateFile(const DS_Handle_t Handle, const U8 Data[15],
                             U8 Status[2]);

ST_ErrorCode_t DS_GenerateKey(const DS_Handle_t Handle, U8 Data[8],
                              U8 Status[2]);

ST_ErrorCode_t DS_GenerateTSC(const DS_Handle_t Handle, U8 Data[8],
                              U8 Status[2]);

ST_ErrorCode_t DS_GetResponse(const DS_Handle_t Handle, const U16 Length, U8 Data[23],
                              U8 Status[2]);

ST_Revision_t DS_GetRevision(void);

ST_ErrorCode_t DS_Invalidate(const DS_Handle_t Handle, U8 Status[2]);

ST_ErrorCode_t DS_Open(const DS_Handle_t SmartCardHandle,
                       DS_Handle_t *NewHandle_p);

ST_ErrorCode_t DS_ReadBinary(const DS_Handle_t Handle, const U16 Offset, U16 Length,
                             U8 Data[255], U8 Status[2]);

ST_ErrorCode_t DS_ReadDirectory(const DS_Handle_t Handle, U8 Data[16],
                                U8 Status[2]);

ST_ErrorCode_t DS_Rehabilitate(const DS_Handle_t Handle, U8 Status[2]);

ST_ErrorCode_t DS_Select(const DS_Handle_t Handle, const U8 Identity[2],
                         U8 Status[2]);

ST_ErrorCode_t DS_UnblockCode(const DS_Handle_t Handle, const U16 CodeNum,
                              const U8 UnblockCode[8], const U8 NewCode[8],
                              U8 Status[2]);

ST_ErrorCode_t DS_UpdateBinary(const DS_Handle_t Handle, const U16 Offset,
                               const U16 Length, const U8 Data[255],
                               U8 Status[2]);

ST_ErrorCode_t DS_VerifyCode(const DS_Handle_t Handle, const U16 CodeNum,
                             const U8 SecretCode[8], U8 Status[2]);

#endif /* __DSLIB_H */

/* End of dslib.h */
