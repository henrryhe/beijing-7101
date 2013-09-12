
/*---------------------------------------------------------------
File Name: ioreg.h

Description: 

Copyright (C) 1999-2001 STMicroelectronics

History:

   04/02/00        Code based on original implementation by CBB.

    date: 27-June-2001
version: 3.1.0
 author: GJP
comment: adapted from reg0299.h
    
    date: 2-April-2002
version: 3.4.0
 author: SD
comment: update to support new addressing modes (STTUNER_IOREG_Mode_t)
    
date: 17/01/2005
version: 5.0.0
 author: SD
comment: ADV1 support
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_IOREG_H
#define __STTUNER_IOREG_H

#define FIELD_TYPE_UNSIGNED 0
#define FIELD_TYPE_SIGNED   1


/* includes --------------------------------------------------------------- */

#include "stlite.h"
#include "ioarch.h"


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* default timeout (for all I/O (I2C) operations) */
#define IOREG_DEFAULT_TIMEOUT           100

#define LSB(X) ((X & 0xFF))
#define MSB(Y) ((Y>>8)& 0xFF)

U8  STTUNER_IOREG_GetDefVal(int index);

/* public types ------------------------------------------------------------ */

    typedef enum
    {
        IOREG_NO  = 0,
        IOREG_YES = 1
    } STTUNER_IOREG_Flag_t;

	/* access modes for fields and registers -- added for ADV1*/
	typedef enum
	{
	    IOREG_ACCESS_WR,  /* can be read and written */
	    IOREG_ACCESS_R,   /* only be read from */
	    IOREG_ACCESS_W,   /* only be written to */
	    IOREG_ACCESS_NON  /* cannot be read or written (guarded register, e.g. register skipped by ChipApplyDefaultValues() etc.) */
	}
	STTUNER_IOREG_Access_t;

	typedef enum
	{
		IOREG_NOT_POINTED=0,
		IOREG_POINTED
		
	}
	STTUNER_IOREG_Pointed_t;

    /* register and address width: 8bits x 8addresses, 16x16 or 32x32 */
    typedef enum
    {
        REGSIZE_8BITS,
        REGSIZE_16BITS,
        REGSIZE_24BITS,/* for ADV1 registers*/
        REGSIZE_32BITS
    } STTUNER_IOREG_RegisterSize_t;


    /* Addressing mode - 8 bit address, 16 bit address, no address (0) */
    typedef enum
    {
        IOREG_MODE_SUBADR_8,        /* <addr><reg8><data><data>...       */
        IOREG_MODE_SUBADR_16,       /* <addr><reg8><reg8>data><data>...  */    
    	IOREG_MODE_NOSUBADR,         /* <addr><data><data>...             */
    	IOREG_MODE_NO_R_SUBADR ,      /* ADV1 LLA Needs*/
    	IOREG_MODE_SUBADR_8_NS ,       /*STB4K needs*/
    	IOREG_MODE_SUBADR_16_POINTED , /*Used in 899*/
    	IOREG_MODE_SUBADR_8_NS_MICROTUNE /*used for microtuner*/
    } STTUNER_IOREG_Mode_t;

    typedef struct
    {
	    U32 Address;    /* Address	             */
            U32 ResetValue; /* Default (reset) value */
   	    U32 Value;      /* Current value         */
   	    STTUNER_IOREG_RegisterSize_t  Size;
	    STTUNER_IOREG_Access_t        Access;
	    STTUNER_IOREG_Pointed_t       Pointed;
	    U16 		          PointerRegAddr;		/* Pointer Adress*/
	    U32 			  BaseAddress;	/*Base Adress*/
	    
   	    
    } STTUNER_IOREG_Register_t;


    typedef struct
    {
        int Reg;  /* Register index	    */
        U32 Pos;  /* Bit position	    */
        U32 Bits; /* Bit width	        */
        U32 Type; /* Signed or unsigned */
        U32 Mask; /* Mask compute with width and position	*/
    } STTUNER_IOREG_Field_t;


 
    typedef struct
    {
        long                         RegExtClk;       /* external clock value */
        U32                          Timeout;         /* I/O timeout */
        ST_Partition_t              *MemoryPartition; /* memory for dynamic allocation  */
        U32                          Registers;       /* number of registers, e.g. 65  for stv0299 */
        U32                          Fields;          /* number of register fields, e.g. 113 for stv0299 */
        #ifdef  STTUNER_DEBUG_MODULE_IOREG
        STTUNER_IOREG_Register_t    *RegMap;          /* register map list */
        STTUNER_IOREG_Field_t       *FieldMap;        /* register field list */
        STTUNER_IOREG_Flag_t         RegTrigger;      /* trigger scope enable */
        #endif
        STTUNER_IOREG_RegisterSize_t RegSize;         /* width of registers in this list */
        STTUNER_IOREG_Mode_t		 Mode;            /* Addressing mode */	 
        U8                          *ByteStream;      /* storage area for 'ContigousRegisters' operations */
        ST_ErrorCode_t  Error; /* Added for Error handling in I2C  */
        U32				  WrStart;		  /* Id of the first writable register */	
	U32			          WrSize;           /* Number of writable registers */	
	U32			          RdStart;		  /* Id of the first readable register */
	U32			          RdSize;			  /* Number of readable registers */
        U32                              *DefVal;
    } STTUNER_IOREG_DeviceMap_t;
    
typedef struct
{
    BOOL                IsFree;     
    STTUNER_IODriver_t  IODriver;   
    STTUNER_IORoute_t   IORoute;     
    U32                 Address;     
    U32                 ExtDev;     
    STTUNER_IOREG_DeviceMap_t DeviceMap;    			
    IOARCH_ExtHandle_t       *hInstance; 
    STTUNER_IOARCH_RedirFn_t  TargetFunction;
    STTUNER_IOARCH_RepeaterFn_t RepeaterFn;
} IOARCH_HandleData_t;





/* functions --------------------------------------------------------------- */

    ST_ErrorCode_t STTUNER_IOREG_Open(STTUNER_IOREG_DeviceMap_t  *DeviceMap);
    ST_ErrorCode_t STTUNER_IOREG_Close(STTUNER_IOREG_DeviceMap_t *DeviceMap);
        
    ST_ErrorCode_t STTUNER_IOREG_Reset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U8* DefaultVal,U16 *Addressarray);

    ST_ErrorCode_t STTUNER_IOREG_SetRegister(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegAddress, U32 Value);
    U32            STTUNER_IOREG_GetRegister(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegAddress);

    ST_ErrorCode_t STTUNER_IOREG_SetContigousRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress,U8 *RegsVal, int Number);
    ST_ErrorCode_t STTUNER_IOREG_GetContigousRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, int Number,U8 *RegsVal);

    ST_ErrorCode_t STTUNER_IOREG_SetField(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldIndex, int Value);
    U8            STTUNER_IOREG_GetField(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32  FieldIndex);
    U32  STTUNER_IOREG_FieldExtractVal(U8 RegisterVal, int FieldIndex);
      
    U8 STTUNER_IOREG_GetFieldVal(STTUNER_IOREG_DeviceMap_t *DeviceMap,U32 FieldIndex,U8 * DataArr);
    void STTUNER_IOREG_SetFieldVal(STTUNER_IOREG_DeviceMap_t *DeviceMap,U32 FieldIndex,int Value,U8 *DataArr);
    ST_ErrorCode_t STTUNER_IOREG_SetContigousRegisters_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, U32 *RegsVal, int Number);
    ST_ErrorCode_t STTUNER_IOREG_GetContigousRegisters_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 FirstRegAddress, int Number,U32 *RegsVal);
    int	 STTUNER_IOREG_GetField_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 FieldMask,U32 FieldInfo);
    ST_ErrorCode_t STTUNER_IOREG_SetField_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldMask,U32 FieldInfo,int Value);
    ST_ErrorCode_t STTUNER_IOREG_Reset_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 * DefaultVal,U32 *Addressarray);
 void  STTUNER_IOREG_RegSetDefaultVal(STTUNER_IOREG_DeviceMap_t *DeviceMap, U8 *DefRegVal,U16 *RegAddress ,U32 MaxRegisterNum,int RegIndex,U8 Value);
    U32  STTUNER_IOREG_RegGetDefaultVal(STTUNER_IOREG_DeviceMap_t *DeviceMap, U8 *DefRegVal,U16 *RegAddress ,U32 MaxRegisterNum,int RegIndex);






#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_IOREG_H */

/* End of ioreg.h */







