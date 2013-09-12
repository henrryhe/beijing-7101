/*****************************************************************************
File Name   : dslib.c

Description : Library for DS Smartcards.

Copyright (C) 1999 STMicroelectronics

Reference   :

DS Library Reference Manual, De La Rue Card Systems
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>            /* C libs */
#include <string.h>

#include "stlite.h"           /* System includes */

#include "stsmart.h"          /* STAPI includes */

#include "dslib.h"            /* Local includes */

/* Private types/constants ------------------------------------------------ */

#define DS_CMD_LEN      5
#define DS_CLA          0x00

/* Private variables ------------------------------------------------------ */

/* Test harness revision number */
static ST_Revision_t Revision = "1.0.0";

/* Private function prototypes -------------------------------------------- */

/* Function definitions --------------------------------------------------- */

ST_Revision_t DS_GetRevision(void)
{
    return Revision;
} /* DS_GetRevision() */

ST_ErrorCode_t DS_Certify(const DS_Handle_t Handle, const U16 Offset, const U8 Data[8],
                          U16 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberRead, NumberWritten;
    U8 Cmd[] =
    {
        DS_CLA,                         /* CLA */
        0xF4,                           /* INS */
        0x00,                           /* P1 (offset MSB) */
        0x00,                           /* P2 (offset LSB)*/
        0x08                            /* P3 */
    };

    Cmd[2] = (Offset >> 8) & 0x00FF;
    Cmd[3] = Offset & 0x00FF;

    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN, &NumberWritten,
                             (U8 *)Data, 0, &NumberRead, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_ChangeCode(const DS_Handle_t Handle, const U16 CodeNum,
                             const U8 OldCode[8], U8 NewCode[8],
                             U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_CreateBinary(const DS_Handle_t Handle, const U16 Length,
                               const U8 Data[255], U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_CreateFile(const DS_Handle_t Handle, const U8 Data[15],
                             U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_GenerateKey(const DS_Handle_t Handle, U8 Data[8],
                              U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_GenerateTSC(const DS_Handle_t Handle, U8 Data[8],
                              U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_GetResponse(const DS_Handle_t Handle, const U16 Length, U8 Data[23],
                              U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberRead, NumberWritten;
    U8 Cmd[DS_CMD_LEN] =
    {
        DS_CLA,                         /* CLA */
        0xC0,                           /* INS */
        0x00,                           /* P1 */
        0x00,                           /* P2 */
        0x00                            /* P3 (bytes returned) */
    };

    Cmd[4] = Length;

    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN, &NumberWritten,
                             Data, 0, &NumberRead, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_Invalidate(const DS_Handle_t Handle, U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_ReadBinary(const DS_Handle_t Handle, const U16 Offset, U16 Length,
                             U8 Data[255], U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberRead, NumberWritten;
    U8 Cmd[] =
    {
        DS_CLA,                         /* CLA */
        0xB0,                           /* INS */
        0x00,                           /* P1 (offset MSB) */
        0x00,                           /* P2 (offset LSB) */
        0x00                            /* P3 (data bytes to follow) */
    };

    Cmd[2] = (Offset >> 8) & 0x00FF;
    Cmd[3] = Offset & 0x00FF;
    Cmd[4] = Length;

    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN, &NumberWritten,
                             Data, 0, &NumberRead, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_ReadDirectory(const DS_Handle_t Handle, U8 Data[16],
                                U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberRead, NumberWritten;
    U8 Cmd[DS_CMD_LEN] =
    {
        DS_CLA,                         /* CLA */
        0xB8,                           /* INS */
        0x00,                           /* P1 */
        0x00,                           /* P2 */
        0x10                            /* P3 (data bytes to follow) */
    };

    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN, &NumberWritten,
                             Data, 0, &NumberRead, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_Rehabilitate(const DS_Handle_t Handle, U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_Select(const DS_Handle_t Handle, const U8 Identity[2],
                         U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberRead, NumberWritten;
    U8 Cmd[DS_CMD_LEN+2] =
    {
        DS_CLA,                         /* CLA */
        0xA4,                           /* INS */
        0x00,                           /* P1 */
        0x00,                           /* P2 */
        0x02                            /* P3 (data bytes to follow) */
    };

    Cmd[5] = Identity[0];
    Cmd[6] = Identity[1];

    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN+2, &NumberWritten,
                             NULL, 0, &NumberRead, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_UnblockCode(const DS_Handle_t Handle, const U16 CodeNum,
                              const U8 UnblockCode[8], const U8 NewCode[8],
                              U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_UpdateBinary(const DS_Handle_t Handle, const U16 Offset,
                               const U16 Length, const U8 Data[255],
                               U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STSMART_Status_t CardStatus;
    U32 NumberWritten;
    U8 Cmd[260] =
    {
        DS_CLA,                         /* CLA */
        0xD6,                           /* INS */
        0x00,                           /* P1 (offset MSB) */
        0x00,                           /* P2 (offset LSB) */
        0x00,                           /* P3 (data bytes to follow) */
    };

    Cmd[2] = (Offset >> 8) & 0x00FF;
    Cmd[3] = Offset & 0x00FF;
    Cmd[4] = Length;
    memcpy(&Cmd[5], Data, Length);
    Error = STSMART_Transfer(Handle, Cmd, DS_CMD_LEN+Length, &NumberWritten,
                             NULL, 0, NULL, &CardStatus);

    Status[0] = CardStatus.StatusBlock.T0.PB[0];
    Status[1] = CardStatus.StatusBlock.T0.PB[1];

    return Error;
}

ST_ErrorCode_t DS_VerifyCode(const DS_Handle_t Handle, const U16 CodeNum,
                             const U8 SecretCode[8], U8 Status[2])
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

ST_ErrorCode_t DS_Open(const STSMART_Handle_t SmartCardHandle,
                       DS_Handle_t *NewHandle_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    *NewHandle_p = SmartCardHandle;

    return Error;
}

ST_ErrorCode_t DS_Close(const DS_Handle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    return Error;
}

/* End of module */
