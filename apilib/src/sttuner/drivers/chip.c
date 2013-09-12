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

/* STAPI */
#include "sttuner.h"
/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "ioarch.h"     /* STTUNER driver */
#include "ioreg.h"
#include "chip.h"

#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))
#define REPEATER_ON    1
#define REPEATER_OFF   0
#define WAITFORLOCK    1

/* ST20 boot address (FLASH memory/MPX boot) */
#define BAD_RAM_ADDRESS (0x7ffffffe)

extern IOARCH_HandleData_t  IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];




/* ----------------------------------------------------------------------------
Name: ChipSetOneRegister()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetOneRegister(IOARCH_Handle_t hChip,int FirstReg, unsigned char Value)
{
   #ifdef STTUNER_DEBUG_MODULE_CHIP
   const char*      identity    = "STTUNER chip.c ChipSetOneRegister()";
   #endif
   ST_ErrorCode_t   Error       = ST_NO_ERROR;
   U32              Pool[2];
   U32              hInstance   = 0;
   unsigned char    Buffer[3];

   if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Invalid handle passed\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].IsFree == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Handle not used\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Pool[0] = IOARCH_Handle[hChip - 1].ExtDev;  /*Retrieve corresponding demod i2c handle*/
      Pool[1] = IOARCH_Handle[hChip].ExtDev;      /*tuner i2c handle*/
      Error = STI2C_LockMultiple(Pool, 2, WAITFORLOCK);
      if (Error != ST_NO_ERROR)
      {
         #ifdef STTUNER_DEBUG_MODULE_CHIP
         STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
         #endif
         return(Error);
      }

      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance,REPEATER_ON);  /*Repeater function invocation*/
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
   }
   
   switch (IOARCH_Handle[hChip].DeviceMap.Mode)
   {
   
      case IOREG_MODE_SUBADR_16:
         Buffer[0] = MSB(FirstReg);   
         Buffer[1] = LSB(FirstReg);
         Buffer[2] = (U8) (Value);
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 3, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);                                  
         break;
      
      case IOREG_MODE_SUBADR_8:
      case IOREG_MODE_SUBADR_8_NS:
      case IOREG_MODE_NO_R_SUBADR:
      case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
         IOARCH_Handle[hChip].DeviceMap.ByteStream[(U8) (FirstReg & 0xFF)] = (U8) (Value);  /*DeviceMap Update*/
         Buffer[0] = (U8) (FirstReg & 0xFF);
         Buffer[1] = (U8) (Value);
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 2, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);                                  
         break;

      case IOREG_MODE_NOSUBADR:
         Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
         break;
      default:
      	break;
   }      

   IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
      return(Error);
   }
   

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      return(Error); 
      }
   }


      #ifdef STTUNER_DEBUG_MODULE_CHIP
      if  (IOARCH_Handle[hChip].DeviceMap.Mode == IOREG_MODE_SUBADR_16)
      	{
      STTBX_Print(("%s Write Success  One byte %x in address %x%x\n", identity, Buffer[2],Buffer[0],Buffer[1]));
       }
      else
      	{
      STTBX_Print(("%s Write Success  One byte %x in address %x\n", identity, Buffer[1],Buffer[0]));
      }
      #endif


   return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipSetRegisters()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetRegisters(IOARCH_Handle_t hChip,U32 FirstRegAddress,U8 *RegsVal,int Number)
{
   ST_ErrorCode_t   Error       = ST_NO_ERROR;
   U8               index;
   unsigned char    Buffer[30] =
   {
      0
   };
   U32              hInstance   = 0;
   U32              Pool[2];


   #ifdef STTUNER_DEBUG_MODULE_CHIP
   const char*      identity    = "STTUNER chip.c ChipSetRegisters()";
   #endif

   if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Invalid handle passed\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].IsFree == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Handle not used\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
 
      Pool[0] = IOARCH_Handle[hChip - 1].ExtDev;   /*Retrieve corresponding demod i2c handle*/
      Pool[1] = IOARCH_Handle[hChip].ExtDev;       /*tuner i2c handle*/
      Error = STI2C_LockMultiple(Pool, 2, WAITFORLOCK);
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
      #endif
      if (Error != ST_NO_ERROR)
      {
         return(Error);
      }
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_ON);
        if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
   }
  

   switch (IOARCH_Handle[hChip].DeviceMap.Mode)
   {
       case IOREG_MODE_SUBADR_16:
         Buffer[0] = MSB(FirstRegAddress);   
         Buffer[1] = LSB(FirstRegAddress);
         for (index = 0; index < Number; index++)
         {
            Buffer[index + 2] = RegsVal[index];
         }      
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,(Number+2), IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize); 
         break;



      case IOREG_MODE_SUBADR_8:
      case IOREG_MODE_NO_R_SUBADR:
         Buffer[0] = LSB(FirstRegAddress);  

         for (index = 0; index < Number; index++)
         {
            Buffer[index + 1] = RegsVal[index];
            IOARCH_Handle[hChip].DeviceMap.ByteStream[LSB(FirstRegAddress)] = RegsVal[index];
            FirstRegAddress++;
         }      
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,(Number+1), IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize); 

         break;
        
      case IOREG_MODE_NOSUBADR:
      
         for (index = 0; index < Number; index++)
         {
            Buffer[index] = RegsVal[index];
            IOARCH_Handle[hChip].DeviceMap.ByteStream[index] = RegsVal[index];
            FirstRegAddress++;
         }      
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, Number, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize); 
         break;
         
        case IOREG_MODE_SUBADR_8_NS:
        	Buffer[0] = LSB(FirstRegAddress);  

         for (index = 0; index < Number; index++)
         {
            Buffer[index + 1] = RegsVal[index];
            IOARCH_Handle[hChip].DeviceMap.ByteStream[LSB(FirstRegAddress)] = RegsVal[index];
            FirstRegAddress++;
         }      
         Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,(Number+1), IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize); 

         break;
  	
        case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
          Buffer[0] = LSB(FirstRegAddress);  

         for (index = 0; index < Number; index++)
         {
            Buffer[index + 1] = RegsVal[index];
            IOARCH_Handle[hChip].DeviceMap.ByteStream[LSB(FirstRegAddress)] = RegsVal[index];
            FirstRegAddress++;
         }      
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,(Number+1), IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize); 
          break;

          default:
      	break;
   }

   IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
      return(Error);
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      return(Error);
      }
   }
     #ifdef STTUNER_DEBUG_MODULE_CHIP
      if  (IOARCH_Handle[hChip].DeviceMap.Mode==IOREG_MODE_SUBADR_16) 
      {   
      STTBX_Print(("%s Write Success.  Following %d bytes starting from address %x%x\n", identity,Number,Buffer[0],Buffer[1]));
      for (index = 0; index < Number; index++)
         {
            STTBX_Print(("%x\n",Buffer[index+2]));
         }
      }
      else
      {
      STTBX_Print(("%s Write Success.  Following %d bytes starting from address %x\n", identity,Number,Buffer[0]));
      for (index = 0; index < Number; index++)
         {
            STTBX_Print(("%x\n",Buffer[index+1]));
         }
      }
      #endif

   return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipSetField()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipSetField(IOARCH_Handle_t hChip, U32 FieldIndex, int Value)
{
   ST_ErrorCode_t   Error       = ST_NO_ERROR;
   U16              RegAddress;
   U8                Value_DeviceMap;
   unsigned char    Buffer[3];
   U32              Pool[2];
   U32              hInstance   = 0;
   int  Mask;
   int  Sign;
   int  Bits;
   int  Pos;


   #ifdef STTUNER_DEBUG_MODULE_CHIP
   const char*      identity    = "STTUNER chip.c ChipSetField()";
   #endif

   if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Invalid handle passed\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].IsFree == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Handle not used\n", identity));
      #endif
      return(ST_ERROR_INVALID_HANDLE);
   }

   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Pool[0] = IOARCH_Handle[hChip - 1].ExtDev;  /*Retrieve corresponding demod i2c handle*/
      Pool[1] = IOARCH_Handle[hChip].ExtDev;      /*tuner i2c handle*/
      Error = STI2C_LockMultiple(Pool, 2, WAITFORLOCK);
      if (Error != ST_NO_ERROR)
      {
         #ifdef STTUNER_DEBUG_MODULE_CHIP
         STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
         #endif
         return(Error);
      }

      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance,REPEATER_ON);  /*Repeater function invocation*/
        if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }

   }

 
   RegAddress = ChipGetRegAddress(FieldIndex);
   Sign = ChipGetFieldSign(FieldIndex);
   Mask = ChipGetFieldMask(FieldIndex);
   Pos = ChipGetFieldPosition(Mask);
   Bits = ChipGetFieldBits(Mask, Pos);

   if (IOARCH_Handle[hChip].DeviceMap.Mode == IOREG_MODE_SUBADR_16)
   {
   Value_DeviceMap = ChipGetOneRegister(hChip,RegAddress);
   }
   else
   {
   Value_DeviceMap = IOARCH_Handle[hChip].DeviceMap.ByteStream[RegAddress];
   }
   

   if (Sign == FIELD_TYPE_SIGNED)
   {
      Value = (Value > 0) ? Value : Value + (1 << Bits);    /*  compute signed fieldval */
   }

   Value = Mask & (Value << Pos);   /*  Shift and mask value */
   Value_DeviceMap = (Value_DeviceMap & (~Mask)) + Value;

   switch (IOARCH_Handle[hChip].DeviceMap.Mode)
   {
  
      case IOREG_MODE_SUBADR_16:
         Buffer[0] = MSB(RegAddress);   
         Buffer[1] = LSB(RegAddress);
         Buffer[2] = (U8) (Value_DeviceMap);
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 3, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);                              
         break;

      case IOREG_MODE_SUBADR_8:
      case IOREG_MODE_SUBADR_8_NS:
      case IOREG_MODE_NO_R_SUBADR:
      case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
         IOARCH_Handle[hChip].DeviceMap.ByteStream[(U8) (RegAddress & 0xFF)] = (U8) (Value_DeviceMap);
         Buffer[0] = (U8) (RegAddress & 0xFF);
         Buffer[1] = (U8) (Value_DeviceMap);
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 2, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);                              
         break;

      case IOREG_MODE_NOSUBADR:
         Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
         break; 
         
       default:
      	  break;
   }

  IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
      return(Error);
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      return(Error);
      }
   }
    #ifdef STTUNER_DEBUG_MODULE_CHIP
      if  (IOARCH_Handle[hChip].DeviceMap.Mode==IOREG_MODE_SUBADR_16)
      	{
      STTBX_Print(("%s Write Success  One byte %x in address %x%x\n", identity, Buffer[2],Buffer[0],Buffer[1]));
      }
      else
      	{
      STTBX_Print(("%s Write Success  One byte %x in address %x\n", identity, Buffer[1],Buffer[0]));
     }
      #endif
   return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipGetOneRegister()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetOneRegister(IOARCH_Handle_t hChip, U32 RegAddress)
{
   ST_ErrorCode_t   Error       = ST_NO_ERROR;
   U8               Buffer[2]; 
   U32              hInstance   = 0;
   U32              Pool[2];


   #ifdef STTUNER_DEBUG_MODULE_CHIP
   const char*      identity    = "STTUNER chip.c ChipGetOneRegister()";
   #endif

   if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Invalid handle passed\n", identity));
      #endif

   }


   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }


   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }
   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Pool[0] = IOARCH_Handle[hChip - 1].ExtDev;  /*Retrieve corresponding demod i2c handle*/
      Pool[1] = IOARCH_Handle[hChip].ExtDev;      /*tuner i2c handle*/
      Error = STI2C_LockMultiple(Pool, 2, WAITFORLOCK);
      if (Error != ST_NO_ERROR)
      {
         #ifdef STTUNER_DEBUG_MODULE_CHIP
         STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
         #endif
        
      }

      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance,REPEATER_ON);  /*Repeater function invocation*/
        if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      }
      
   }

   switch (IOARCH_Handle[hChip].DeviceMap.Mode)
   {

      case IOREG_MODE_SUBADR_16:
         if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_DIRECT)
         {
            Buffer[0] = MSB(RegAddress);  
            Buffer[1] = LSB(RegAddress);
            Error =  STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 2, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
            break;
         }
      case IOREG_MODE_SUBADR_8:
         if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_DIRECT)
         {
            Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */

            Error =  STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);

            if (Error == ST_NO_ERROR)
            {
               IOARCH_Handle[hChip].DeviceMap.ByteStream[(U8) (RegAddress & 0xFF)] = Buffer[0]; 
            }
            break;
         }
         
      case IOREG_MODE_NOSUBADR:
      case IOREG_MODE_NO_R_SUBADR:
         Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);

         if (Error == ST_NO_ERROR)
            {
               IOARCH_Handle[hChip].DeviceMap.ByteStream[(U8) (RegAddress & 0xFF)] = Buffer[0];
            }

         break;
         
      case IOREG_MODE_SUBADR_8_NS:
	   case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
	  	    Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */

            Error =  STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
         break;
	  	  
         
          default:
      	break;
   }


  IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
      
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      
      }
   }
    #ifdef STTUNER_DEBUG_MODULE_CHIP
      if  (IOARCH_Handle[hChip].DeviceMap.Mode==IOREG_MODE_SUBADR_16)
      	{
      STTBX_Print(("%s Read Success  One byte %x from address %x\n", identity, Buffer[0],RegAddress));
    }
      else
      	{
      STTBX_Print(("%s Read Success  One byte %x from address %x\n", identity, Buffer[0],RegAddress));
    }
      #endif
   
   return(Buffer[0]);
}
/* ----------------------------------------------------------------------------
Name: ChipGetRegisters()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t ChipGetRegisters(IOARCH_Handle_t hChip,U32 FirstRegAddress,int Number,U8 *RegsVal)
{
  U8 index,BufIndex;
  U8 Buffer[2];
  ST_ErrorCode_t Error = ST_NO_ERROR;
  U32 hInstance = 0;
  U32 Pool[2];
  
    #ifdef STTUNER_DEBUG_MODULE_CHIP
    const char *identity = "STTUNER chip.c ChipGetRegisters()";
    #endif
    
    if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
    {
    #ifdef STTUNER_DEBUG_MODULE_CHIP
       STTBX_Print(("%s Failed Invalid handle passed\n", identity));
    #endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    if (IOARCH_Handle[hChip].IsFree == TRUE)
    {
    #ifdef STTUNER_DEBUG_MODULE_CHIP
        STTBX_Print(("%s Failed Handle not used\n", identity));
    #endif
        return(ST_ERROR_INVALID_HANDLE);
    }
 

           
   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
    hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }
    
    if(IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
        {
           Pool[0] = IOARCH_Handle[hChip-1].ExtDev;
           Pool[1] = IOARCH_Handle[hChip].ExtDev;
           Error= STI2C_LockMultiple(Pool,2,WAITFORLOCK);
            if (Error != ST_NO_ERROR)
             {
               #ifdef STTUNER_DEBUG_MODULE_CHIP
               STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
               #endif
              return(Error);
             }
            Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance,REPEATER_ON);
            if (Error != ST_NO_ERROR)
            {
             #ifdef STTUNER_DEBUG_MODULE_CHIP
          STTBX_Print(("%s Repeater function failed\n", identity));
           #endif
           return(Error);
           }
        }

   switch(IOARCH_Handle[hChip].DeviceMap.Mode)
     {
        case IOREG_MODE_SUBADR_16:
        Buffer[0] = MSB(FirstRegAddress);   
        Buffer[1] = LSB(FirstRegAddress);
        Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,2, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.WrSize);
	    Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), RegsVal,Number, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.RdSize);
        break;
        
	     case IOREG_MODE_SUBADR_8:
	     Buffer[0]=LSB(FirstRegAddress);
         Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,1, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.WrSize);
	     Error = STI2C_Read (I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), RegsVal,Number, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.RdSize);
	     for(index = 0; index < Number; index++)
         {
	     IOARCH_Handle[hChip].DeviceMap.ByteStream[LSB(FirstRegAddress)]=RegsVal[index];
	    FirstRegAddress++;
	     }
        break;
       
        case IOREG_MODE_NOSUBADR:
        case IOREG_MODE_NO_R_SUBADR:
            BufIndex = 0;
            Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), IOARCH_Handle[hChip].DeviceMap.ByteStream,Number, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
            if(Error == ST_NO_ERROR)
            {
                for(index = FirstRegAddress; index < (FirstRegAddress + Number); index++)   /* read back into local copy of register map */
                {
                  RegsVal[index]  = IOARCH_Handle[hChip].DeviceMap.ByteStream[BufIndex];      	
                  BufIndex++;
                }
            }
         break;
         
        case IOREG_MODE_SUBADR_8_NS:
        case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
         Buffer[0]=LSB(FirstRegAddress);
         Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer,1, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.WrSize);
	     Error = STI2C_Read (I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), RegsVal,Number, IOARCH_Handle[hChip].DeviceMap.Timeout,&IOARCH_Handle[hChip].DeviceMap.RdSize);
         break;
        	
        	
 	
 	     default:
      	  break;
            
    }
      
   IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
      return(Error);
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      return(Error);
      }
   }
   #ifdef STTUNER_DEBUG_MODULE_CHIP 
      STTBX_Print(("%s Read Success.  Following %d bytes starting from address %x\n", identity,Number,FirstRegAddress));
      for (index = 0; index < Number; index++)
         {
            STTBX_Print(("%x\n",RegsVal[index]));
         }
    #endif
   
    return(Error);
}
/* ----------------------------------------------------------------------------
Name: ChipGetField()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
U8 ChipGetField(IOARCH_Handle_t hChip, U32 FieldIndex)
{
   #ifdef STTUNER_DEBUG_MODULE_CHIP
   const char*      identity    = "STTUNER chip.c ChipSetOneRegister()";
   #endif
   U32              Pool[2];
   U32              hInstance   = 0;
   U16              RegAddress;
   int              Mask;
   int              Sign;
   int              Bits;
   int              Pos;
   U8               Buffer[2];
   ST_ErrorCode_t   Error       = ST_NO_ERROR;
   if (hChip >= STTUNER_IOARCH_MAX_HANDLES)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Invalid handle passed\n", identity));
      #endif

   }

   if (IOARCH_Handle[hChip].IsFree == TRUE)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Failed Handle not used\n", identity));
      #endif

   }

   if (IOARCH_Handle[hChip].hInstance != NULL)
   {
      hInstance = *IOARCH_Handle[hChip].hInstance; /* if REPEATER or PASSTHRU retrieve instance of fn() to call */
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Pool[0] = IOARCH_Handle[hChip - 1].ExtDev;  /*Retrieve corresponding demod i2c handle*/
      Pool[1] = IOARCH_Handle[hChip].ExtDev;      /*tuner i2c handle*/
      Error = STI2C_LockMultiple(Pool, 2, WAITFORLOCK);
      if (Error != ST_NO_ERROR)
      {
         #ifdef STTUNER_DEBUG_MODULE_CHIP
         STTBX_Print(("%s STI2C_LockMultiple() failed\n", identity));
         #endif

      }

      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance,REPEATER_ON);  /*Repeater function invocation*/
        if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      return(Error);
      }

   }

   RegAddress = ChipGetRegAddress(FieldIndex);
   Sign = ChipGetFieldSign(FieldIndex);
   Mask = ChipGetFieldMask(FieldIndex);
   Pos = ChipGetFieldPosition(Mask);
   Bits = ChipGetFieldBits(Mask, Pos);




   switch (IOARCH_Handle[hChip].DeviceMap.Mode)
   {

      case IOREG_MODE_SUBADR_16:
        if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_DIRECT)
         {
             Buffer[0] = MSB(RegAddress);   
             Buffer[1] = LSB(RegAddress);
            Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 2, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
         }
      break;
      case IOREG_MODE_SUBADR_8:

            Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */

            Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
            IOARCH_Handle[hChip].DeviceMap.ByteStream[RegAddress] = Buffer[0];
      break;
      
        case IOREG_MODE_SUBADR_8_NS:
        case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
            Buffer[0] = LSB(RegAddress);    /* 8 bits sub addresses */
            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.WrSize);
            Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);  
        break;
      
       
      case IOREG_MODE_NOSUBADR:
      case IOREG_MODE_NO_R_SUBADR:
         Error = STI2C_Read(I2C_HANDLE(&IOARCH_Handle[hChip].ExtDev), Buffer, 1, IOARCH_Handle[hChip].DeviceMap.Timeout, &IOARCH_Handle[hChip].DeviceMap.RdSize);
         break;
         
          default:
      	break;
   
   }


   Buffer[0] = ((Buffer[0] & Mask) >> Pos);

   if ((Sign == FIELD_TYPE_SIGNED) && (Buffer[0] & (1 << (Bits - 1))))
   {
      Buffer[0] = (Buffer[0] - (1 << Bits)); /* Compute signed values */
   } 
   IOARCH_Handle[hChip].DeviceMap.Error |= Error; /* GNBvd17801->Update error field of devicemap */   
   if (Error != ST_NO_ERROR)
   {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s I/O Failed error = %d\n", identity, Error));
      #endif
   }

   if (IOARCH_Handle[hChip].IORoute == STTUNER_IO_REPEATER)
   {
      Error = (*IOARCH_Handle[hChip].RepeaterFn)(hInstance, REPEATER_OFF);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s Repeater function failed\n", identity));
      #endif
      }
      Error = STI2C_UnlockMultiple(Pool, 2);
      if (Error != ST_NO_ERROR)
      {
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      STTBX_Print(("%s STI2C_UnlockMultiple() failed\n", identity));
      #endif
      }
   }
      #ifdef STTUNER_DEBUG_MODULE_CHIP
      if  (IOARCH_Handle[hChip].DeviceMap.Mode==IOREG_MODE_SUBADR_16)
      	{
      STTBX_Print(("%s Read Success  One byte %x from address %x\n", identity, Buffer[0],RegAddress));
       }
      else
      	{
      STTBX_Print(("%s Read Success  One byte %x from address %x\n", identity, Buffer[0],RegAddress));
      }
      #endif
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


































