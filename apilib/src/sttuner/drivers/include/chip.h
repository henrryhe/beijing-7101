/* -------------------------------------------------------------------------
File Name: chip.h

Description: Present a register based interface to hardware connected on an I2C bus.

Copyright (C) 1999-2001 STMicroelectronics

History:
   date: 10-October-2001
version: 1.0.0
 author: SA
comment: STAPIfied by GP

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef H_CHIP
#define H_CHIP


/* includes ---------------------------------------------------------------- */

/* STAPI (ST20) requirements */

 #include "stddefs.h"    /* Standard definitions */
 #include "ioarch.h"
 #include "ioreg.h"


/* PC (IA) requirements */
#if defined(HOST_PC)
 #include "stddefs.h"    /* Standard definitions */
#endif


#ifdef __cplusplus
 extern "C"
 {
#endif                  /* __cplusplus */


/* Access routines */
ST_ErrorCode_t ChipSetOneRegister(IOARCH_Handle_t hChip, int RegId, unsigned char Value);
U8             ChipGetOneRegister(IOARCH_Handle_t hChip, U32 RegAddress);

ST_ErrorCode_t ChipSetRegisters(IOARCH_Handle_t hChip,U32 FirstRegAddress,U8 *RegsVal,int Number);
ST_ErrorCode_t ChipGetRegisters(IOARCH_Handle_t hChip,U32 FirstRegAddress,int Number, U8 *RegsVal);

ST_ErrorCode_t ChipSetField(IOARCH_Handle_t hChip, U32 FieldIndex, int Value);
U8             ChipGetField(IOARCH_Handle_t hChip, U32 FieldIndex);

ST_ErrorCode_t ChipOpen(IOARCH_Handle_t *hChip,STTUNER_IOARCH_OpenParams_t  *OpenParams);
ST_ErrorCode_t ChipInit(void);
ST_ErrorCode_t ChipClose(IOARCH_Handle_t hChip);
ST_ErrorCode_t ChipTerm(void);

void ChipSetFieldImage(U32 FieldIndex,int Value,U8 *DataArr);
U8 ChipGetFieldImage(U32 FieldIndex,U8* DataArr);
U32  ChipFieldExtractVal(U8 RegisterVal, int FieldIndex);

U16 ChipGetRegAddress(U32 FieldId);
int ChipGetFieldMask(U32 FieldId);
int ChipGetFieldSign(U32 FieldId);
int ChipGetFieldPosition(U8 Mask);
int ChipGetFieldBits(int mask, int Position);

/* IO repeater/passthru function format */





#ifdef __cplusplus
 }
#endif                  /* __cplusplus */

#endif          /* H_CHIP */

/* End of chip.h */

