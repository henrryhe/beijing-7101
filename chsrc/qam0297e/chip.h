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


/* PC (IA) requirements */
#if defined(HOST_PC)
 #include "stddefs.h"    /* Standard definitions */
#endif


#ifdef __cplusplus
 extern "C"
 {
#endif                  /* __cplusplus */


#define FIELD_TYPE_UNSIGNED 0
#define FIELD_TYPE_SIGNED   1


#define LSB(X) ((X & 0xFF))
#define MSB(Y) ((Y>>8)& 0xFF)


/* Access routines */
ST_ErrorCode_t ChipSetOneRegister( int RegId, unsigned char Value);
U8             ChipGetOneRegister( U32 RegAddress);

ST_ErrorCode_t ChipSetRegisters(U32 FirstRegAddress,U8 *RegsVal,int Number);
ST_ErrorCode_t ChipGetRegisters(U32 FirstRegAddress,int Number, U8 *RegsVal);

ST_ErrorCode_t ChipSetField( U32 FieldIndex, int Value);
U8             ChipGetField( U32 FieldIndex);


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

