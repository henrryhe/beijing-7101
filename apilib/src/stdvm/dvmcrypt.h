/*****************************************************************************

File Name  : dvmcrypt.h

Description: Encrypt and Decrypt support functions

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DVMCRYPT_H
#define __DVMCRYPT_H

/* Includes --------------------------------------------------------------- */
#include "dvmint.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */
extern  U8                     *STDVMi_CryptBufferAligned;

/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t STDVMi_CryptInit(void);
ST_ErrorCode_t STDVMi_CryptTerm(void);
ST_ErrorCode_t STDVMi_Decrypt(STDVMi_Handle_t *FileHandle_p, void *SrcBuffer_p, void *DstBuffer_p, U32 Size);
ST_ErrorCode_t STDVMi_Encrypt(STDVMi_Handle_t *FileHandle_p, void *SrcBuffer_p, void *DstBuffer_p, U32 Size);

#endif /*__DVMCRYPT_H*/

/* EOF --------------------------------------------------------------------- */

