/* -------------------------------------------------------------------------
File Name: chip.c

Description: 

Copyright (C) 1999-2001 STMicroelectronics

History:
   date: 10-October-2001
version: 1.0.0
 author: GP
comment: 

   date: 21-June-2007
version: 4.9.0
 author: RV
comment: 

---------------------------------------------------------------------------- */
/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
  
#include <string.h>     /* for memcpy() */
#endif
#include "sttbx.h"


#include "stddef.h"     /* TOOLSET Standard definitions */
#include "stddefs.h"    /* STAPI Standard definitions */

/* local to sttuner */
#include "chip.h"
#include "ch_tuner_mid.h"


/* ----------------------------------------------------------------------------
Name: ChipSetOneRegister()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetOneRegister(int FirstReg, unsigned char Value)
{
	ST_ErrorCode_t   Error       = ST_NO_ERROR;
	unsigned char    Buffer[3];

   
	 Buffer[0] = (U8) (FirstReg & 0xFF);
	 Buffer[1] = (U8) (Value);
	 Error = ch_I2C_DemodReadAndWrite(CH_I2C_WRITE, Buffer, 2);

	return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipSetRegisters()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetRegisters(U32 FirstRegAddress,U8 *RegsVal,int Number)
{
	ST_ErrorCode_t   Error       = ST_NO_ERROR;
	U8               index;
	unsigned char    Buffer[250] =
	{
		0
	};


	Buffer[0] = LSB(FirstRegAddress);  

	for (index = 0; index < Number; index++)
	{
		Buffer[index + 1] = RegsVal[index];
	}    

	 Error = ch_I2C_DemodReadAndWrite(CH_I2C_WRITE, Buffer, (Number+1));

	return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipSetField()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetField(U32 FieldIndex, int Value)
{
	ST_ErrorCode_t   Error       = ST_NO_ERROR;
	U16              RegAddress;
	U8                Value_DeviceMap;
	unsigned char    Buffer[3];
	int  Mask;
	int  Sign;
	int  Bits;
	int  Pos;
 
	RegAddress = ChipGetRegAddress(FieldIndex);
	Sign = ChipGetFieldSign(FieldIndex);
	Mask = ChipGetFieldMask(FieldIndex);
	Pos = ChipGetFieldPosition(Mask);
	Bits = ChipGetFieldBits(Mask, Pos);

	Value_DeviceMap = ChipGetOneRegister(RegAddress);


	if (Sign == FIELD_TYPE_SIGNED)
	{
		Value = (Value > 0) ? Value : Value + (1 << Bits);    /*  compute signed fieldval */
	}

	Value = Mask & (Value << Pos);   /*  Shift and mask value */
	Value_DeviceMap = (Value_DeviceMap & (~Mask)) + Value;


	Buffer[0] = (U8) (RegAddress & 0xFF);
	Buffer[1] = (U8) (Value_DeviceMap);
	Error = ch_I2C_DemodReadAndWrite(CH_I2C_WRITE, Buffer, 2);

	return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipGetOneRegister()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetOneRegister(U32 RegAddress)
{
	ST_ErrorCode_t   Error       = ST_NO_ERROR;
	U8               Buffer[2]; 

	Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */

	Error |= ch_I2C_DemodReadAndWrite(CH_I2C_READ, Buffer, 1);

	return(Buffer[0]);
}
/* ----------------------------------------------------------------------------
Name: ChipGetRegisters()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipGetRegisters(U32 FirstRegAddress,int Number,U8 *RegsVal)
{
	U8 Buffer[2];
	ST_ErrorCode_t Error = ST_NO_ERROR;
  
	Buffer[0]=LSB(FirstRegAddress);
	Error |= ch_I2C_DemodReadAndWrite(CH_I2C_READ, RegsVal, Number);

	return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipGetField()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetField(U32 FieldIndex)
{
	U16              RegAddress;
	int              Mask;
	int              Sign;
	int              Bits;
	int              Pos;
	U8               Buffer[2];
	ST_ErrorCode_t   Error       = ST_NO_ERROR;



	RegAddress = ChipGetRegAddress(FieldIndex);
	Sign = ChipGetFieldSign(FieldIndex);
	Mask = ChipGetFieldMask(FieldIndex);
	Pos = ChipGetFieldPosition(Mask);
	Bits = ChipGetFieldBits(Mask, Pos);

	Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */

	Error |= ch_I2C_DemodReadAndWrite(CH_I2C_READ, Buffer, 1);

	Buffer[0] = ((Buffer[0] & Mask) >> Pos);

	if ((Sign == FIELD_TYPE_SIGNED) && (Buffer[0] & (1 << (Bits - 1))))
	{
		Buffer[0] = (Buffer[0] - (1 << Bits)); /* Compute signed values */
	} 
	
	return(Buffer[0]);
}

/* ----------------------------------------------------------------------------
Name: ChipSetFieldImage()

Description:
    Set the value of the field of register with the value store in  dataarray(this function is for tuner)
Parameters:
    mask containing address and field,pointer to data array,no of values to set
Return Value:
---------------------------------------------------------------------------- */
void ChipSetFieldImage(U32 FieldIndex,int Value,U8 *DataArr)
{
    U16 RegAddress;	
    int Mask;
    int Sign;
    int Bits;
    int Pos;
    	
	
			RegAddress= ChipGetRegAddress(FieldIndex);
			Sign = ChipGetFieldSign(FieldIndex);
			Mask = ChipGetFieldMask(FieldIndex);
			Pos = ChipGetFieldPosition(Mask);
			Bits = ChipGetFieldBits(Mask,Pos);
 if(Sign == FIELD_TYPE_SIGNED)
    {
        Value = (Value > 0 ) ? Value : Value + (1<<Bits);	/*	compute signed fieldval	*/
    }
    
    Value = Mask & (Value<<Pos);	/*	Shift and mask value */	
	
	
	DataArr[RegAddress] = (DataArr[RegAddress] & (~Mask))+ (U8)Value ;	
}

/* ----------------------------------------------------------------------------
Name: ChipGetFieldImage()

Description:
    Compute the value of the field of register and store the value  in  dataarray(this function is for tuner)
Parameters:
     mask containing address and field,pointer to data array,no of values to set
Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetFieldImage(U32 FieldIndex,U8* DataArr)
{
	U16 RegAddress;
	U8 value;
	int Mask;
	int Sign;
	int Bits;
	int Pos;
    
    			RegAddress= ChipGetRegAddress(FieldIndex);
    			Sign = ChipGetFieldSign(FieldIndex);
			Mask = ChipGetFieldMask(FieldIndex);
			Pos = ChipGetFieldPosition(Mask);
			Bits = ChipGetFieldBits(Mask,Pos);
			

    value= ((DataArr[RegAddress] & Mask )>>Pos);
     if((Sign == FIELD_TYPE_SIGNED)&&(value>=(((1<<Bits)-1) ) ) )
    {
        value =  value - (1 <<Bits);	/*	compute signed fieldval	*/
    }
    
  	
	return(value);	
}
/* ----------------------------------------------------------------------------
Name: ChipFieldExtractVal()

Description:
    Compute the value of the field with the 8 bit register value .
Parameters:
    index of the field
Return Value:
---------------------------------------------------------------------------- */
U32  ChipFieldExtractVal(U8 RegisterVal, int FieldIndex)
{

   U16 RegAddress;
	int Mask;
	int Sign;
	int Bits;
	int Pos;	
	U32 Value;
    
    Value= RegisterVal;
	RegAddress= ChipGetRegAddress(FieldIndex);
   Sign = ChipGetFieldSign(FieldIndex);
	Mask = ChipGetFieldMask(FieldIndex);
	Pos = ChipGetFieldPosition(Mask);
	Bits = ChipGetFieldBits(Mask,Pos);

     /*	Extract field */
    Value = (( Value & Mask) >> Pos);

    if( (Sign== FIELD_TYPE_SIGNED) && ( Value & (1 << (Bits-1)) ) )
    {
        Value = (Value - (1 << Bits)); /* Compute signed values */
    } 
   
    return(Value);
}


/*****************************************************
**FUNCTION	::	IOREG_GetRegAddress
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	mask
**RETURN	::	mask
*****************************************************/
U16 ChipGetRegAddress(U32 FieldId)
{
 U16 RegAddress;
 RegAddress = (FieldId>>16) & 0xFFFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return RegAddress;
}

/*****************************************************
**FUNCTION	::	ChipGetFieldMask
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	mask
**RETURN	::	mask
*****************************************************/
int ChipGetFieldMask(U32 FieldId)
{
 int mask;
 mask = FieldId & 0xFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return mask;
}

/*****************************************************
**FUNCTION	::	ChipGetFieldSign
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	sign
**RETURN	::	sign
*****************************************************/
int ChipGetFieldSign(U32 FieldId)
{
 int sign;
 sign = (FieldId>>8) & 0x01; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return sign;
}

/*****************************************************
**FUNCTION	::	ChipGetFieldPosition
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	position
**RETURN	::	position
*****************************************************/
int ChipGetFieldPosition(U8 Mask)
{
 int position=0, i=0;

 while((position == 0)&&(i < 8))
 {
 	position = (Mask >> i) & 0x01;
 	i++;
 }
  
 return (i-1);
}
/*****************************************************
**FUNCTION	::	IOREG_GetFieldBits
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	number of bits
**RETURN	::	number of bits
*****************************************************/
int ChipGetFieldBits(int mask, int Position)
{
 int bits,bit;
 int i =0;
 
 bits = mask >> Position;
 bit = bits ;
 while ((bit > 0)&&(i<8))
 {
 	i++;
 	bit = bits >> i;
 	
 }
 return i;
}


