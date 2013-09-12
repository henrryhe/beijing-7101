

  
/* ----------------------------------------------------------------------------
File Name: newioreg.c

Description: 

     This module handles low-level driver functions for I2c etc.


Copyright (C) 1999-2001 STMicroelectronics

Revision History:
    04/02/00        Code based on original implementation by CBB.

    21/03/00        Received code modifications that eliminate compiler
                    warnings.

   date: 27-June-2001
version: 3.1.0
 author: GJP from work by LW/CBB
comment: adapted from reg0299.c

   date: 21-August-2001
version: 3.1.1
 author: GJP
comment: removed register and field name strings.

    date: 2-April-2002
version: 3.4.0
 author: SD
comment: update to support new addressing modes.

 date: 1-June-2006
version: 
 author: HS ,SD
comment: Simplification of the Code without using Regmap and Fieldmap.


Reference:
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
  
#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* IO access layer */

#include "ioreg.h"     /* header for this file */

#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))

	static U32 LastBaseAddress=0xffffffff;
	static U16 LastPointer=0xffff;/* local functions --------------------------------------------------------- */

extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];
/* local functions --------------------------------------------------------- */

U32 IOREG_FieldCreateMask(STTUNER_IOREG_Field_t *Field);



/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
    #ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_Open()";
    #endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* error checking */
    Error = STTUNER_Util_CheckPtrNull(DeviceMap);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail 'DeviceMap' not valid\n", identity));
#endif
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceMap->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail 'MemoryPartition' not valid\n", identity));
#endif
        return(Error);
    }


    if (DeviceMap->Registers < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Register count (%d)\n", identity, DeviceMap->Registers));
#endif
        return(ST_ERROR_BAD_PARAMETER);           
    }

    if (DeviceMap->Fields < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s WARNING: field count (%d)\n", identity, DeviceMap->Fields));
#endif
    }

    /* setup defaults */
    if (DeviceMap->Timeout == 0)
    {
        DeviceMap->Timeout = IOREG_DEFAULT_TIMEOUT;
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s Timeout set to default (%d)\n", identity, IOREG_DEFAULT_TIMEOUT));
#endif
    }
 
    
#ifdef  STTUNER_DEBUG_MODULE_IOREG
   /* scope trigger pin */
    DeviceMap->RegTrigger = IOREG_NO;
#endif

#ifdef  STTUNER_DEBUG_MODULE_IOREG
    /* allocate memory */
    DeviceMap->RegMap = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, DeviceMap->Registers, sizeof(STTUNER_IOREG_Register_t));
    if (DeviceMap->RegMap == NULL)  
    {
        #ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail memory allocation RegMap\n", identity));
        #endif    
        return(ST_ERROR_NO_MEMORY);           
    }

    DeviceMap->FieldMap = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, DeviceMap->Fields, sizeof(STTUNER_IOREG_Field_t));
    if (DeviceMap->FieldMap == NULL)  
    {
	#ifdef STTUNER_DEBUG_MODULE_IOREG
	        STTBX_Print(("%s fail memory allocation FieldMap\n", identity));
	#endif    
        	STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->RegMap);
	
	#ifdef STTUNER_DEBUG_MODULE_IOREG
	        STTBX_Print(("%s RegMap FREED\n", identity));
	#endif
        return(ST_ERROR_NO_MEMORY);           
    }
    
#endif

    switch(DeviceMap->Mode)
    {
        case IOREG_MODE_SUBADR_8:   /* nothing to do */
        case IOREG_MODE_SUBADR_16: 
        case IOREG_MODE_SUBADR_16_POINTED:       
        break;

        case IOREG_MODE_NOSUBADR:   /* allocate enough space to read/write the entire register map in one operation */
        case IOREG_MODE_NO_R_SUBADR:
        case IOREG_MODE_SUBADR_8_NS:
        case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
            DeviceMap->ByteStream = STOS_MemoryAllocateClear(DeviceMap->MemoryPartition, DeviceMap->Registers, sizeof(U8) );
            if (DeviceMap->ByteStream == NULL)  
            {
	#ifdef STTUNER_DEBUG_MODULE_IOREG
                STTBX_Print(("%s fail memory allocation ByteStream\n", identity));

                STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->RegMap);


                STTBX_Print(("%s RegMap FREED\n", identity));

                STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->FieldMap);
	#endif
	#ifdef STTUNER_DEBUG_MODULE_IOREG
                STTBX_Print(("%s FieldMap FREED\n", identity));
	#endif
                return(ST_ERROR_NO_MEMORY);           
            }
            else
            DeviceMap->Error = ST_NO_ERROR;  /* GNBvd17801->Added by for I2C error handling */
            
            break;

        default:
	#ifdef STTUNER_DEBUG_MODULE_IOREG
           	 STTBX_Print(("%s fail Mode of addressing error (8,16,none).\n", identity));
	#endif
        return(ST_ERROR_BAD_PARAMETER);           
    }

#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s opened ok\n", identity));
#endif    
    return(Error);           
 }



/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_Close(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR ;

    /* error checking */
    Error = STTUNER_Util_CheckPtrNull(DeviceMap);
    
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail 'DeviceMap' not valid\n", identity));
#endif
        return(Error);
    }
    
#ifdef  STTUNER_DEBUG_MODULE_IOREG
    /* deallocate memory */
    if (DeviceMap->RegMap != NULL)
    {
        STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->RegMap);
        STTBX_Print(("%s RegMap FREED\n", identity));
    }
    else
    {



        STTBX_Print(("%s RegMap not freed\n", identity));
    } 
#endif

#ifdef  STTUNER_DEBUG_MODULE_IOREG
    if (DeviceMap->FieldMap != NULL)
    {
        STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->FieldMap);

        STTBX_Print(("%s FieldMap FREED\n", identity));
    }
    else
    {


        STTBX_Print(("%s FieldMap not freed\n", identity));
    }
#endif

    switch(DeviceMap->Mode)
    {
        case IOREG_MODE_SUBADR_8:
        case IOREG_MODE_SUBADR_16:  /* nothing to do */
        case IOREG_MODE_SUBADR_16_POINTED:
                   break;

        case IOREG_MODE_NOSUBADR:
        case IOREG_MODE_NO_R_SUBADR:
        case IOREG_MODE_SUBADR_8_NS:
        case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
            if (DeviceMap->ByteStream != NULL)
            {
                STOS_MemoryDeallocate(DeviceMap->MemoryPartition, DeviceMap->ByteStream);
#ifdef STTUNER_DEBUG_MODULE_IOREG
                STTBX_Print(("%s ByteStream FREED\n", identity));
#endif
            }
            else
            {
#ifdef STTUNER_DEBUG_MODULE_IOREG
                STTBX_Print(("%s ByteStream not freed\n", identity));
#endif
            }
            break;

        default:
            break;
    }

   return(Error);           
}






































































































ST_ErrorCode_t STTUNER_IOREG_Reset_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 * DefaultVal,U32 *Addressarray)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 index;
	for (index = 0; index < DeviceMap->Registers; index++)
    {	
       Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, Addressarray[index],  DefaultVal[index]);
       if (Error != ST_NO_ERROR)
       {
			#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
            STTBX_Print(("\n%s fail Error=%d DeviceMap=0x%08x IOHandle=%u", identity, Error, (U32)DeviceMap, IOHandle));
			#endif
            DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
            return( Error );
        }   
    }
    DeviceMap->Error |= Error;
    return( Error );
}     

/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_Reset()

Description:
    Set Chip register values according Default Values stored in ResetValue.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_Reset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U8 * DefaultVal,U16 *Addressarray)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_Reset()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 *iobuffer;
    int index;
    U8 Bufindex = 0;


    switch(DeviceMap->Mode)
    {
        case IOREG_MODE_SUBADR_8:
        case IOREG_MODE_SUBADR_16: 
	 for (index = 0; index < DeviceMap->Registers; index++)
            {	
                 Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, Addressarray[index],  DefaultVal[index]);
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
                    STTBX_Print(("\n%s fail Error=%d DeviceMap=0x%08x IOHandle=%u", identity, Error, (U32)DeviceMap, IOHandle));
#endif
                    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
                    return( Error );
                }

            }   /* for(RegIndex) */
            break;
        case IOREG_MODE_NO_R_SUBADR:
             iobuffer = DeviceMap->ByteStream;
            for(index = DeviceMap->WrStart; index < (DeviceMap->WrStart+DeviceMap->Registers); index++)
             {
                  iobuffer[Bufindex] = DefaultVal[index];  
                  
  		  #ifdef  STTUNER_DEBUG_MODULE_IOREG
                  DeviceMap->RegMap[index].Value = DeviceMap->RegMap[index].ResetValue;    /* update local copy as well*/             
 		  #endif
                 ++Bufindex;
                }
         
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_WRITE, DeviceMap->WrStart, iobuffer, DeviceMap->Registers, DeviceMap->Timeout);
            break;
		
        case IOREG_MODE_SUBADR_8_NS:
        
            for (index = 0; index < DeviceMap->Registers; index++)
            {	
                  Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, Addressarray[index],  DefaultVal[index]);
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
                    STTBX_Print(("\n%s fail Error=%d DeviceMap=0x%08x IOHandle=%u", identity, Error, (U32)DeviceMap, IOHandle));
#endif
                    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
                    return( Error );
                }

            }   /* for(RegIndex) */
      
            break;
			
case IOREG_MODE_SUBADR_8_NS_MICROTUNE:		
        for (index = 0; index < DeviceMap->Registers; index++)
            {	
                 Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, Addressarray[index],  DefaultVal[index]);
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
                    STTBX_Print(("\n%s fail Error=%d DeviceMap=0x%08x IOHandle=%u", identity, Error, (U32)DeviceMap, IOHandle));
#endif
                    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
                    return( Error );
                }

            }   /* for(RegIndex) */
            break;
        case IOREG_MODE_NOSUBADR:
       
            iobuffer = DeviceMap->ByteStream;
            for(index = 0; index < DeviceMap->Registers; index++)
            {
                iobuffer[index] = (DefaultVal[index]&0xFF);
 #ifdef  STTUNER_DEBUG_MODULE_IOREG                 
                DeviceMap->RegMap[index].Value = DeviceMap->RegMap[index].ResetValue;   /* update local copy as well */
#endif
              }
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_WRITE, 0, iobuffer, DeviceMap->Registers, DeviceMap->Timeout);
            break;
            case IOREG_MODE_SUBADR_16_POINTED:       
        break;
    }

    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
            STTBX_Print(("%s fail Error=%d DeviceMap=0x%08x IOHandle=%u\n", identity, Error, (U32)DeviceMap, IOHandle));
#endif
    }

    return( Error );
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_SetRegister()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_SetRegister(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegAddress, U32 Value)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_SetRegister()";
    U32 value, regaddress;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;   
    unsigned char data[10],nbdata = 0;
    U32 Size;
	U8 RegsVal_U8;
    U32 RegsVal_U32;
   switch(DeviceMap->Mode)
	{
	   case IOREG_MODE_SUBADR_16_POINTED:
		RegsVal_U32=Value;
		    Error = STTUNER_IOREG_SetContigousRegisters_SizeU32(DeviceMap, IOHandle, RegAddress, &RegsVal_U32, 1);        
		
		break;			
	   case IOREG_MODE_SUBADR_16:
	  
  			data[nbdata++]=MSB(RegAddress);	/* 16 bits sub addresses */

			data[nbdata++]=LSB(RegAddress);	/* 8 bits sub addresses */
			
			data[nbdata++]= Value; /* first Byte Value*/
			
			Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
			#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
			#endif			
		break;
       case IOREG_MODE_SUBADR_8:
	   case IOREG_MODE_SUBADR_8_NS:
	   case IOREG_MODE_NO_R_SUBADR:	
	   case IOREG_MODE_SUBADR_8_NS_MICROTUNE :	       		
		       
		       RegsVal_U8 = Value & 0xFF;
		       Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_WRITE, RegAddress, &(RegsVal_U8), 1, DeviceMap->Timeout);								
		       
		       break;
	   case IOREG_MODE_NOSUBADR:
		       Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            	       break;
            	       
 }
    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
    if( Error != ST_NO_ERROR)
    {
    	 
#ifdef STTUNER_DEBUG_MODULE_IOREG
            STTBX_Print(("%s fail Error=%d DeviceMap=0x%08x IOHandle=%u\n", identity, Error, (U32)DeviceMap, IOHandle));
#endif
    }
    else
    {
 #ifdef  STTUNER_DEBUG_MODULE_IOREG    	
     DeviceMap->RegMap[RegIndex].Value = Value;   /* update local copy as well */
        value      = DeviceMap->RegMap[RegIndex].Value;
        regaddress = DeviceMap->RegMap[RegIndex].Address;
        STTBX_Print(("%s Address(0x%x %u)\tValue(0x%x %u)\n", identity, regaddress, regaddress, value, value ));
#endif
    }

    return(Error);
}	



/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_GetRegister()

Description:
    Get Register value.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
U32 STTUNER_IOREG_GetRegister(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 RegAddress)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_GetRegister()";
    U32 value, regaddress;
    
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ST_ErrorCode_t ErrorStore = ST_NO_ERROR;
    
    unsigned char data[10],nbdata = 0;
    U8 RegsVal_U8;
    int i =0;
    U32 Size, RegsVal_U32=0;


   switch(DeviceMap->Mode)
   {
	case IOREG_MODE_SUBADR_16_POINTED:
		STTUNER_IOREG_GetContigousRegisters_SizeU32(DeviceMap, IOHandle, RegAddress, 1, &RegsVal_U32);     
		
	break;
	case IOREG_MODE_SUBADR_16:
		data[nbdata++]=MSB(RegAddress);	/* 16 bits sub addresses */
		case IOREG_MODE_SUBADR_8:
		if(IOARCH_Handle[IOHandle].IORoute ==STTUNER_IO_DIRECT )
		{
			data[nbdata++]=LSB(RegAddress);	/* 8 bits sub addresses */
			/*only for inbuf and packet delin registers*/
			Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
			#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (Error == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	Error =ST_NO_ERROR;
            }
			#endif
			ErrorStore = STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data,1, DeviceMap->Timeout, &Size);
			#ifdef STTUNER_DEBUG_DTV_I2C_QUIET_MODE
            if (ErrorStore == STTUNER_ERROR_DTV_I2C_QUIET_MODE)
            {
            	ErrorStore =ST_NO_ERROR;
            }
			#endif
			Error |= ErrorStore;
			if(Error == ST_NO_ERROR)
	        {
				RegsVal_U8 =data [i] & 0xff;
				#ifdef  STTUNER_DEBUG_MODULE_IOREG
				DeviceMap->RegMap[RegIndex].Value=data[i];
				#endif	
	        }
	        else
	        {
	           	RegsVal_U8 =0xFF;
	           	#ifdef  STTUNER_DEBUG_MODULE_IOREG
	        	DeviceMap->RegMap[RegIndex].Value=0xFF;
	        	#endif
	        }	
	           
	     }
		 else
	     {
	        	Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_READ, RegAddress, &RegsVal_U8, 1, DeviceMap->Timeout);
	        	#ifdef  STTUNER_DEBUG_MODULE_IOREG
	        	Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_READ, DeviceMap->RegMap[RegIndex].Address, (U8 *)&DeviceMap->RegMap[RegIndex].Value, 1, DeviceMap->Timeout);
				#endif
	     }						    
		break;			
							
	  case IOREG_MODE_NOSUBADR:
	  case IOREG_MODE_NO_R_SUBADR:
		 Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_READ, 0, data, 1, DeviceMap->Timeout);
		 if(Error == ST_NO_ERROR)
	     { 
         	 RegsVal_U8=data[0] & 0xff;
         	 #ifdef  STTUNER_DEBUG_MODULE_IOREG
             DeviceMap->RegMap[RegIndex].Value=data[i];	
             #endif
	     }
         else
		 {
			RegsVal_U8 = 0xFF;
			#ifdef  STTUNER_DEBUG_MODULE_IOREG
			DeviceMap->RegMap[RegIndex].Value=0xFF;
			#endif
		 }	
		 break;
	 case IOREG_MODE_SUBADR_8_NS:
	 case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
		Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_READ_NS, (U16)RegAddress, (U8 *)&RegsVal_U8, 1, DeviceMap->Timeout);
		#ifdef  STTUNER_DEBUG_MODULE_IOREG
		Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_READ_NS, DeviceMap->RegMap[RegIndex].Address, (U8 *)&DeviceMap->RegMap[RegIndex].Value, 1, DeviceMap->Timeout);
		#endif
		break;
		
     		
     }
    	

    if( Error != ST_NO_ERROR)
    {
    	#ifdef  STTUNER_DEBUG_MODULE_IOREG
		#ifdef STTUNER_DEBUG_MODULE_IOREG
		    STTBX_Print(("%s fail Error=%d DeviceMap=0x%08x IOHandle=%u\n", identity, Error, (U32)DeviceMap, IOHandle));
		#endif  
		#endif  
    }
    else
    {
		#ifdef STTUNER_DEBUG_MODULE_IOREG
        value      = DeviceMap->RegMap[RegIndex].Value;
        regaddress = DeviceMap->RegMap[RegIndex].Address;
        STTBX_Print(("%s Address(0x%x %u)\tValue(0x%x %u)\n", identity, regaddress, regaddress, value, value ));
		#endif
    }
	
    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
    if(DeviceMap->Error != ST_NO_ERROR) 
    {
		#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Errror=%d\n", identity, Error));
		#endif
    }
    if(DeviceMap->Mode != IOREG_MODE_SUBADR_16_POINTED)
    {
    	RegsVal_U32 = RegsVal_U8 & 0xFF;
    }
    		
    return(RegsVal_U32);
}

ST_ErrorCode_t STTUNER_IOREG_SetContigousRegisters_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, U32 *RegsVal, int Number)
{
	ST_ErrorCode_t Error=ST_NO_ERROR;
	unsigned char data[100]={0},nbdata = 0;
	int i,j, pointed;
	U16 pointer,addressReg;
	U32 baseAddress,Size;
	
	if((FirstRegAddress & 0xFF00)==0xF300)
	pointer=0xf3fc;					/*DVBS2 demod*/
	else if((FirstRegAddress & 0xFF00)==0xFA00)
	pointer=0xfafc;					/*DVBS2 FEC*/
	else
	pointer =0;					/*Other*/
			
	baseAddress=(FirstRegAddress>>16)&0x00003FFF;
	addressReg=(FirstRegAddress & 0xFFFF);
	pointed=(FirstRegAddress>>31);
	
	if(pointed==IOREG_POINTED)
	{
		if((pointer!=LastPointer)||(baseAddress!=LastBaseAddress))
		{
			/* Write pointer register (4x8bits read access) */
			nbdata=0;
			data[nbdata++]=MSB(pointer);	/* 8 bits base addresses reg*/
			data[nbdata++]=LSB(pointer);	/* 16 bits base addresses reg*/

			for(i=0;i<4;i++) 
				data[nbdata++]=(baseAddress>>(8*i))&0xFF;		 /*Base adress value*/
		
			Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
			LastPointer=pointer;
			LastBaseAddress=baseAddress;
					
		}
		
		/* Write pointed register (4x8bits read access) */ 
		nbdata=0;
		data[nbdata++]=MSB(addressReg);	/* 16 bits sub addresses */ 
		data[nbdata++]=LSB(addressReg);	/* 8 bits sub addresses */  
		
		for(j=0;j<Number;j++)
			for(i=0;i<4;i++) 
				data[nbdata++]=(RegsVal[j]>>(8*i))&0xFF; /* first Byte Value*/
						
	}
	else
	{	
		nbdata = 0;
		data[nbdata++]=MSB(FirstRegAddress);	/* 16 bits sub addresses */
		data[nbdata++]=LSB(FirstRegAddress);	/* 8 bits sub addresses */
		for(i=0;i<Number;i++)								/* fill data buffer */ 
			data[nbdata++]=(RegsVal[i])&0xFF; /* first Byte Value*/
	}	
				
	Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
	return Error;
}


/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_SetContigousRegisters()

Description:
    Set Register value.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_SetContigousRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FirstRegAddress, U8 *RegsVal, int Number)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_SetContigousRegisters()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    int BufIndex=0;
    U32 index, Size ;
    U8 *iobuffer;
    unsigned char iobufferdemod[100]={0};
   
    if(Number < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail no registers to Set (%d)\n", identity, Number));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch(DeviceMap->Mode)
    {
    	BufIndex = 0;
    	
    	case IOREG_MODE_SUBADR_16:  	
    	
  			iobufferdemod[BufIndex++]=MSB(FirstRegAddress);	/* 16 bits sub addresses */
  			
  		case IOREG_MODE_SUBADR_8:
			iobufferdemod[BufIndex++]=LSB(FirstRegAddress);	/* 8 bits sub addresses */
			
			for(index = 0; index < Number; index++)
            {
				iobufferdemod[BufIndex++]= RegsVal[index];
			}
			
			Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), iobufferdemod, BufIndex, DeviceMap->Timeout, &Size);
	
            break;

       case IOREG_MODE_NO_R_SUBADR:
            iobuffer = DeviceMap->ByteStream;
            BufIndex = 0;
            for(index = FirstRegAddress; index < (FirstRegAddress+ Number); index++)
            {
            	iobuffer[BufIndex] = RegsVal[index];
            	#ifdef  STTUNER_DEBUG_MODULE_IOREG
                iobuffer[BufIndex] = (U8)(DeviceMap->RegMap[index].Value & 0xFF);
                #endif
                BufIndex++;
            }
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_WRITE, DeviceMap->WrStart, iobuffer, Number, DeviceMap->Timeout);
            break;
        
       case IOREG_MODE_SUBADR_8_NS:
	   
            iobuffer = DeviceMap->ByteStream;
                BufIndex = 0;
           for(index = FirstRegAddress; index < (FirstRegAddress + Number); index++)
            {
                 iobuffer[BufIndex] = (U8)(RegsVal[index] & 0xFF);
                 #ifdef  STTUNER_DEBUG_MODULE_IOREG
                iobuffer[BufIndex] = (U8)(DeviceMap->RegMap[index].Value & 0xFF);
	#endif
                 BufIndex++;
            }          
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_WRITE_NS, (U16)FirstRegAddress, iobuffer, Number, DeviceMap->Timeout);
		       
                        
            break;


	 case IOREG_MODE_SUBADR_8_NS_MICROTUNE:   /*this case is only for microtuner*/
	  for (index = FirstRegAddress; index <  (FirstRegAddress + Number); index++)
            {	
                 Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, index,  *RegsVal++);
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_IOREG_RESET
                    STTBX_Print(("\n%s fail Error=%d DeviceMap=0x%08x IOHandle=%u", identity, Error, (U32)DeviceMap, IOHandle));
#endif
                    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
                    return( Error );
                }

            }   /* for(RegIndex) */
            
            
            
            break;
		
     
       case IOREG_MODE_NOSUBADR:   /* Fill byte buffer from register array */
            iobuffer = DeviceMap->ByteStream;
            BufIndex = 0;
            
            for(index = (U16)FirstRegAddress; index < ((U16)FirstRegAddress + Number); index++)
            {
                iobuffer[BufIndex] = (U8)(RegsVal[index] & 0xFF);
                
#ifdef STTUNER_DEBUG_MODULE_IOREG
                STTBX_Print(("%s %d[%u:%u(%u)]\n", identity, index, *RegsVal, iobuffer[BufIndex], BufIndex));
#endif
                BufIndex++;
            }
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_WRITE, 0, iobuffer, Number, DeviceMap->Timeout);
          
            break;
       case IOREG_MODE_SUBADR_16_POINTED:       
            break;
    }

    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
    if(Error != ST_NO_ERROR) 
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Errror=%d\n", identity, Error));
#endif
    }
       
    return(DeviceMap->Error);
}

ST_ErrorCode_t STTUNER_IOREG_GetContigousRegisters_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 FirstRegAddress, int Number,U32 *RegsVal)
{
	ST_ErrorCode_t Error = ST_NO_ERROR;
	unsigned char data[100]={0},dummy[4],nbdata =0;
	U8 dum;
	int i,j=0, pointed;
	U16 pointer,addressReg;
	U32 baseAddress=0, Size;
		
	if((FirstRegAddress & 0xFF00)==0xF300)
		pointer=0xf3fc;					/*DVBS2 demod*/
	else if((FirstRegAddress & 0xFF00)==0xFA00)
		pointer=0xfafc;					/*DVBS2 FEC*/
	else
		pointer =0;					/*Other*/
					
	baseAddress=(FirstRegAddress>>16)&0x00003FFF;
	addressReg=(FirstRegAddress & 0xFFFF);
	pointed=(FirstRegAddress>>31);
			
	if(Number < 20)
	{
		if(pointed==IOREG_POINTED)
		{
			if((pointer!=LastPointer)||(baseAddress!=LastBaseAddress))
			{
				/* Write pointer register (4x8bits write access) */
				nbdata=0;
				data[nbdata++]=MSB(pointer);	/* 8 bits base addresses reg*/
				data[nbdata++]=LSB(pointer);	/* 16 bits base addresses reg*/

				for(i=0;i<4;i++)
					data[nbdata++]=(baseAddress>>(8*i))&0xFF;		 /*Base adress value*/
		
				Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
				LastPointer=pointer;
				LastBaseAddress=baseAddress;
			}
								
			#ifndef NO_DUMMY20ACCESS  
			/*trial for +20 dummy access */
			nbdata=0; 
			if((LSB(addressReg)&0x08)==0)
				dum=0x20;
			else
				dum=0;
			dummy[nbdata++]=MSB(addressReg);	/* 16 bits sub addresses */ 
			dummy[nbdata++]=dum;	/* 8 bits sub addresses */
						
			Error |= STI2C_Write(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), dummy, nbdata, DeviceMap->Timeout, &Size);
			Error |= STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), dummy,4, DeviceMap->Timeout, &Size);
				
			#endif 
			
			/* Read pointed register (4x8bits read access) */
			nbdata=0;
			data[nbdata++]=MSB(addressReg);	/* 16 bits sub addresses */ 
			data[nbdata++]=LSB(addressReg);	/* 8 bits sub addresses */  
			
			Error |= STI2C_Write(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
			Error |= STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data,4*Number, DeviceMap->Timeout, &Size);
				
		}
		else					 
		{
			nbdata=0;
			data[nbdata++]=MSB(addressReg);	/* for 16 bits sub addresses */
			data[nbdata++]=LSB(addressReg);  /* for 8 bits sub addresses	*/
						
			/*only for inbuf and packet delin registers*/
			Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data, nbdata, DeviceMap->Timeout, &Size);
			Error |= STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), data,Number, DeviceMap->Timeout, &Size);
			if((MSB(addressReg)==0xf6)||(MSB(addressReg)==0xf2))
			{
			     dummy[0]=MSB(addressReg);	/* 16 bits sub addresses */ 
			     dummy[1]=0xff;	/* 8 bits sub addresses */
			     Error |= STI2C_Write(I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), dummy, 2, DeviceMap->Timeout, &Size);
			     Error |= STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), dummy,1, DeviceMap->Timeout, &Size);					
			}	    
		}
		for(i=0;i<Number;i++)
		{
			if(Error == ST_NO_ERROR)
			{
				RegsVal[i]=data[j++];
				if(pointed==IOREG_POINTED)
				{
					RegsVal[i] += data[j++]<<8;
					RegsVal[i] += data[j++]<<16;
					RegsVal[i] += data[j++]<<24; 
				}
			}
			else
				RegsVal[i] = 0xFFFFFFFF; 
		}
	}
	else 
		Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
	
	return Error;
}

/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_GetContigousRegisters()

Description:
    Get Register value.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_GetContigousRegisters(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 FirstRegAddress, int Number,unsigned char *RegsVal)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_GetContigousRegisters()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int BufIndex=0;
    U16 index; 
    U32 Size;
    U8 *iobuffer;
    U8 Data[100] = {0}, i=0;
     unsigned char iobufferdemod[100]={0};
    
      /* Can only read 8 contigous registers */
    if((Number < 1) || (Number > 80) )
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail no registers to Get (%d)\n", identity, Number));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
  
  
   switch(DeviceMap->Mode)
     {
        








        case IOREG_MODE_SUBADR_16:
        case IOREG_MODE_SUBADR_16_POINTED:
        
        	iobufferdemod[BufIndex++]=MSB(FirstRegAddress);	/* 16 bits sub addresses */
	
	case IOREG_MODE_SUBADR_8:
	iobufferdemod[BufIndex++]=LSB(FirstRegAddress);	/* 8 bits sub addresses */
	
	/*only for inbuf and packet delin registers*/
	Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), iobufferdemod, BufIndex, DeviceMap->Timeout, &Size);
	Error = STI2C_Read (I2C_HANDLE(&IOARCH_Handle[IOHandle].ExtDev), Data,Number, DeviceMap->Timeout, &Size);
	if(Error == ST_NO_ERROR)
	{
		for(i=0;i<Number;++i)
	        RegsVal[i] = Data[i];
	}         	
	          
            break;

    
        case IOREG_MODE_NOSUBADR:
        case IOREG_MODE_NO_R_SUBADR:
            iobuffer = DeviceMap->ByteStream;
            BufIndex = 0;
            Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_READ, 0, iobuffer, Number, DeviceMap->Timeout);
            if(Error == ST_NO_ERROR)
            {
            	BufIndex = 0;
                for(index = FirstRegAddress; index < (FirstRegAddress + Number); index++)   /* read back into local copy of register map */
                {
                	RegsVal[index] = iobuffer[BufIndex];
                	#ifdef  STTUNER_DEBUG_MODULE_IOREG
                   	 DeviceMap->RegMap[index].Value = (U32)iobuffer[BufIndex];
                    	#endif

                	
#ifdef STTUNER_DEBUG_MODULE_IOREG
                    STTBX_Print(("%s %d[%u:%u(%u)]\n", identity, index, DeviceMap->RegMap[index].Value, iobuffer[BufIndex], BufIndex));
#endif
                    BufIndex++;
                }
            }
            break;
         case IOREG_MODE_SUBADR_8_NS:
         case IOREG_MODE_SUBADR_8_NS_MICROTUNE :
        /* This will read contigous register, by sending only first register address -> can read max. 8 register continously */
        Error = STTUNER_IOARCH_ReadWrite(IOHandle, STTUNER_IO_SA_READ_NS,(U16)FirstRegAddress, Data, Number, DeviceMap->Timeout);
            for(index = (U16)FirstRegAddress; index < ((U16)FirstRegAddress + Number); index++)
            {
            	RegsVal[i] = Data[i];
            	 #ifdef  STTUNER_DEBUG_MODULE_IOREG
                DeviceMap->RegMap[index].Value = Data[i];
                #endif
                i++;
            }
            break;
    }

    DeviceMap->Error |= Error; /* GNBvd17801->Update error field of devicemap */
    if(Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Errror=%d\n", identity, Error));
#endif
    }

    return(Error);
}
/*****************************************************
**FUNCTION	::	IOREG_GetRegAddress
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	mask
**RETURN	::	mask
*****************************************************/
U16 IOREG_GetRegAddress(U32 FieldId)
{
 U16 RegAddress;
 RegAddress = (FieldId>>16) & 0xFFFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return RegAddress;
}

/*****************************************************
**FUNCTION	::	IOREG_GetFieldMask
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	mask
**RETURN	::	mask
*****************************************************/
int IOREG_GetFieldMask(U32 FieldId)
{
 int mask;
 mask = FieldId & 0xFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return mask;
}

/*****************************************************
**FUNCTION	::	IOREG_GetFieldSign
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	sign
**RETURN	::	sign
*****************************************************/
int IOREG_GetFieldSign(U32 FieldId)
{
 int sign;
 sign = (FieldId>>8) & 0x01; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */ 
 return sign;
}

/*****************************************************
**FUNCTION	::	IOREG_GetFieldPosition
**ACTION	::	
**PARAMS IN	::	
**PARAMS OUT::	position
**RETURN	::	position
*****************************************************/
int IOREG_GetFieldPosition(U8 Mask)
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
int IOREG_GetFieldBits(int mask, int Position)
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

/*****************************************************
**FUNCTION	::	ChipGetFieldSign
**ACTION	::	get the sign of a field in the chip
**PARAMS IN	::	
**PARAMS OUT::	sign
**RETURN	::	sign
*****************************************************/
int IOREG_GetFieldSign_SizeU32(U32 FieldInfo)
{
	int sign;
	sign = (FieldInfo>>30) & 0x01; /*FieldInf is [reg pointed,sig][reg base address][reg address] --- 4 bytes */ 
	return sign;
}

/*****************************************************
**FUNCTION	::	ChipGetFieldPosition
**ACTION	::	get the position of a field in the chip
**PARAMS IN	::	
**PARAMS OUT::	position
**RETURN	::	position
*****************************************************/
int IOREG_GetFieldPosition_SizeU32(U32 Mask)
{
	int position=0, i=0;

	while((position == 0)&&(i < 32))
	{
		position = (Mask >> i) & 0x01;
		i++;
	}
	
	return (i-1);
}

/*****************************************************
**FUNCTION	::	ChipGetFieldBits
**ACTION	::	get the number of bits of a field in the chip
**PARAMS IN	::	
**PARAMS OUT::	number of bits
**RETURN	::	number of bits
*****************************************************/
int IOREG_GetFieldBits_SizeU32(int mask, int Position)
{
	int bits,bit;
	int i =0;
 
	bits = mask >> Position;
	bit = bits ;
	while ((bit > 0)&&(i<32))
	{
		i++;
		bit = bits >> i;
	}
	
	return i;
}


/*****************************************************
**FUNCTION	::	STTUNER_IOREG_SetField_SizeU32
**ACTION	::	Set value of a field in the chip
**PARAMS IN	::	DeviceMap	==> Pointer to deviceMap
**				FieldId	==> Id of the field
**				Value	==> Value to write
**PARAMS OUT::	NONE
**RETURN	::	Error
*****************************************************/
ST_ErrorCode_t STTUNER_IOREG_SetField_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldMask,U32 FieldInfo,int Value)
{
	int regValue;
	int Sign;
	int Bits;
	int Pos;
	ST_ErrorCode_t Error= ST_NO_ERROR;
	
	regValue=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle,FieldInfo);		/*	Read the register	*/
	Sign = IOREG_GetFieldSign_SizeU32(FieldInfo);
	Pos  = IOREG_GetFieldPosition_SizeU32(FieldMask);
	Bits = IOREG_GetFieldBits_SizeU32(FieldMask,Pos);
	
	if(Sign == FIELD_TYPE_SIGNED)
		Value = (Value > 0 ) ? Value : Value + (Bits);	/*	compute signed fieldval	*/

	Value = FieldMask & (Value << Pos);						/*	Shift and mask value	*/

	regValue=(regValue & (~FieldMask)) + Value;		/*	Concat register value and fieldval	*/
	Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, FieldInfo, regValue);		/*	Write the register */
	
	return Error;
}



/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_FieldExtractVal()

Description:
    Compute the value of the field with the 8 bit register value .
Parameters:
    index of the field
Return Value:
---------------------------------------------------------------------------- */
U32  STTUNER_IOREG_FieldExtractVal(U8 RegisterVal, int FieldIndex)
{
U16 RegAddress;
	
	int Mask;
	int Sign;
	int Bits;
	int Pos;	
	U32 Value;
#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTUNER_IOREG_Field_t *Field;
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_FieldExtractVal()";
    U32 regaddress;
#endif
   
    
   
        Value= RegisterVal;
	RegAddress= IOREG_GetRegAddress(FieldIndex);
    	Sign = IOREG_GetFieldSign(FieldIndex);
	Mask = IOREG_GetFieldMask(FieldIndex);
	Pos = IOREG_GetFieldPosition(Mask);
	Bits = IOREG_GetFieldBits(Mask,Pos);

     /*	Extract field */
    Value = (( Value & Mask) >> Pos);

    if( (Sign== FIELD_TYPE_SIGNED) && ( Value & (1 << (Bits-1)) ) )
    {
        Value = (Value - (1 << Bits)); /* Compute signed values */
    } 
   

#ifdef STTUNER_DEBUG_MODULE_IOREG
    
    regaddress = DeviceMap->RegMap[Field->Reg].Address;
    STTBX_Print(("%s Address(0x%x %u)\tValue(0x%x %u)\t", identity, regaddress, regaddress, Value, Value ));
    if (Field->Type == FIELD_TYPE_SIGNED) STTBX_Print(("<signed>"));
    STTBX_Print(("\n"));
#endif
    return(Value);
}


/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_SetField()

Description:
    Set register bits of the field "field" to the value "value" without changing the others bits of the register
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOREG_SetField(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldIndex, int Value)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_SetField()";
#endif
	U16 RegAddress;
	U8 value;
	int Mask;
	int Sign;
	int Bits;
	int Pos;
    ST_ErrorCode_t Error= ST_NO_ERROR;
#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTUNER_IOREG_Field_t *Field;
#endif
    		RegAddress= IOREG_GetRegAddress(FieldIndex);
    		Sign = IOREG_GetFieldSign(FieldIndex);
			Mask = IOREG_GetFieldMask(FieldIndex);
			Pos = IOREG_GetFieldPosition(Mask);
			Bits = IOREG_GetFieldBits(Mask,Pos);

#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTBX_Print(("%s BEGIN\n", identity));
    Field = &DeviceMap->FieldMap[FieldIndex];
#endif

    value=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, RegAddress); /* read device register */
     if(Sign == FIELD_TYPE_SIGNED)
    {
        Value = (Value > 0 ) ? Value : Value + (1 <<Bits);	/*	compute signed fieldval	*/
    }
    
    Value = Mask & (Value<<Pos);	/*	Shift and mask value */
    value = ( value & (~Mask) ) + Value;
    
     Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, RegAddress , value );  /*	Write the register */

 #ifdef  STTUNER_DEBUG_MODULE_IOREG
    STTUNER_IOREG_FieldSetVal(DeviceMap, FieldIndex, Value);    /*	Compute new RegMap value */

    Error = STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, Field->Reg, DeviceMap->RegMap[Field->Reg].Value );  /*	Write the register */
#endif

    Error |= DeviceMap->Error; /* GNBvd17801->Accumulate all the error with above I2C calls errors */
    
    if(Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Errror=%d\n", identity, Error));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTBX_Print(("%s END\n", identity));
#endif

    return(Error);

}



/*****************************************************
**FUNCTION	::	STTUNER_IOREG_GetField_SizeU32
**ACTION	::	get the value of a field from the chip
**PARAMS IN	::	DeviceMap,IOHandle	==>	Handle of the device
**				FieldId	==> Id of the field
**PARAMS OUT::	NONE
**RETURN	::	field's value
*****************************************************/
int	 STTUNER_IOREG_GetField_SizeU32(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U32 FieldMask,U32 FieldInfo)
{
	int value = 0xFF;
	int Sign, Pos, Bits;
	
	/* I2C Read : register address set-up */
	Sign = IOREG_GetFieldSign_SizeU32(FieldInfo);
	Pos  = IOREG_GetFieldPosition_SizeU32(FieldMask);
	Bits = IOREG_GetFieldBits_SizeU32(FieldMask,Pos);					
	
	value = STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,FieldInfo);		/*	Read the register	*/     
	value = (value & FieldMask) >> Pos;	/*	Extract field	*/

	if((Sign == FIELD_TYPE_SIGNED)&&(value & (1<<(Bits-1))))
		value = value - (1<<Bits);			/*	Compute signed value	*/
	
	return value;
}

/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_GetField()

Description:
    Set Chip register bits of the field "field" to the value "value"
Parameters:

Return Value:
---------------------------------------------------------------------------- */
U8 STTUNER_IOREG_GetField(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 FieldIndex)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_GetField()";
#endif
    U8 Value;
    U16 RegAddress;
	int Mask;
	int Sign;
	int Bits;
	int Pos;

#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTBX_Print(("%s BEGIN\n", identity));
#endif
			RegAddress= IOREG_GetRegAddress(FieldIndex);
    			Sign = IOREG_GetFieldSign(FieldIndex);
			Mask = IOREG_GetFieldMask(FieldIndex);
			Pos = IOREG_GetFieldPosition(Mask);
			Bits = IOREG_GetFieldBits(Mask,Pos);

    Value=STTUNER_IOREG_GetRegister(DeviceMap, IOHandle, RegAddress); /* read device register */
     /*	Extract field */
    Value = (( Value & Mask) >> Pos);

    if( (Sign== FIELD_TYPE_SIGNED) && ( Value & (1 << (Bits-1)) ) )
    {
        Value = (Value - (1 << Bits)); /* Compute signed values */
    } 
    
 #ifdef STTUNER_DEBUG_MODULE_IOREG
    Value = STTUNER_IOREG_FieldGetVal(DeviceMap, FieldIndex);
    STTBX_Print(("%s END\n", identity));
#endif

    if(DeviceMap->Error != ST_NO_ERROR) 
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
        STTBX_Print(("%s fail Errror=%d\n", identity, Error));
#endif
    }
    
    return(Value);
}
/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_GetFieldVal()

Description:
    Compute the value of the field of register and store the value  in  dataarray(this function is for tuner)
Parameters:
     mask containing address and field,pointer to data array,no of values to set
Return Value:
---------------------------------------------------------------------------- */

U8 STTUNER_IOREG_GetFieldVal(STTUNER_IOREG_DeviceMap_t *DeviceMap,U32 FieldIndex,U8* DataArr)
{
	U16 RegAddress;
	U8 value;
	int Mask;
	int Sign;
	int Bits;
	int Pos;
    
    			RegAddress= IOREG_GetRegAddress(FieldIndex);
    			Sign = IOREG_GetFieldSign(FieldIndex);
			Mask = IOREG_GetFieldMask(FieldIndex);
			Pos = IOREG_GetFieldPosition(Mask);
			Bits = IOREG_GetFieldBits(Mask,Pos);
			

    value= ((DataArr[RegAddress] & Mask )>>Pos);
     if((Sign == FIELD_TYPE_SIGNED)&&(value>=(((1<<Bits)-1) ) ) )
    {
        value =  value - (1 <<Bits);	/*	compute signed fieldval	*/
    }
    
  	
	return(value);	
}
/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_SetFieldVal()

Description:
    Set the value of the field of register with the value store in  dataarray(this function is for tuner)
Parameters:
    mask containing address and field,pointer to data array,no of values to set
Return Value:
---------------------------------------------------------------------------- */
void STTUNER_IOREG_SetFieldVal(STTUNER_IOREG_DeviceMap_t *DeviceMap,U32 FieldIndex,int Value,U8 *DataArr)
{
    U16 RegAddress;
	
	int Mask;
	int Sign;
	int Bits;
	int Pos;	
	
			RegAddress= IOREG_GetRegAddress(FieldIndex);
			Sign = IOREG_GetFieldSign(FieldIndex);
			Mask = IOREG_GetFieldMask(FieldIndex);
			Pos = IOREG_GetFieldPosition(Mask);
			Bits = IOREG_GetFieldBits(Mask,Pos);
 if(Sign == FIELD_TYPE_SIGNED)
    {
        Value = (Value > 0 ) ? Value : Value + (1<<Bits);	/*	compute signed fieldval	*/
    }
    
    Value = Mask & (Value<<Pos);	/*	Shift and mask value */	
	
	
	DataArr[RegAddress] = (DataArr[RegAddress] & (~Mask))+ (U8)Value ;	
}


/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_RegGetDefaultVal()

Description:
    Compute the default values of the register with the value define in reset time (for tuner specific)
Parameters:
   pointer to array of defaultvalues array,register address,number of register to get
Return Value:
---------------------------------------------------------------------------- */
U32  STTUNER_IOREG_RegGetDefaultVal(STTUNER_IOREG_DeviceMap_t *DeviceMap, U8 *DefRegVal,U16 *RegAddress ,U32 MaxRegisterNum,int RegIndex)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_RegGetDefaultVal()";
    U32 regaddress;
#endif
    U32 i, Value;
  
    /*	Extract Index */
    for (i=0;i < MaxRegisterNum;i++ )
    {
     if (RegAddress [i] == RegIndex )
      break;
    }
    if (i == MaxRegisterNum )
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
       STTBX_Print(("%s Address(0x%x %u)\t is not valid\n", identity, RegAddress, RegAddress));
   
#endif    
    i = MaxRegisterNum-1;
   }
  
   

    Value = DefRegVal[i];


#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTBX_Print(("%s Address(0x%x %u)\tValue(0x%x %u)\t", identity, RegAddress, RegAddress, Value, Value ));
    STTBX_Print(("\n"));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: STTUNER_IOREG_SegGetDefaultVal()

Description:
   Set the default values of the register with the value define in reset time(for tuner specific)
Parameters:
    pointer to array of defaultvalues array,register address,number of register to get
Return Value:
---------------------------------------------------------------------------- */
void  STTUNER_IOREG_RegSetDefaultVal(STTUNER_IOREG_DeviceMap_t *DeviceMap, U8 *DefRegVal,U16 *RegAddress ,U32 MaxRegisterNum,int RegIndex,U8 Value)
{
#ifdef STTUNER_DEBUG_MODULE_IOREG
    const char *identity = "STTUNER ioreg.c STTUNER_IOREG_RegSetDefaultVal()";
    U32 regaddress;
#endif
     U32 i;
  
    /*	Extract Index */
    for (i=0;i < MaxRegisterNum;i++ )
    {
     if (RegAddress [i] == RegIndex )
      break;
    }
    if (i == MaxRegisterNum )
    {
#ifdef STTUNER_DEBUG_MODULE_IOREG
       STTBX_Print(("%s Address(0x%x %u)\t is not valid\n", identity, RegAddress, RegAddress));
   
#endif    
    i = MaxRegisterNum-1;
   }
  
   

    DefRegVal[i]= Value;


#ifdef STTUNER_DEBUG_MODULE_IOREG
    STTBX_Print(("%s Address(0x%x %u)\tValue(0x%x %u)\t", identity, RegAddress, RegAddress, Value, Value ));
    STTBX_Print(("\n"));
#endif
    return;
}





