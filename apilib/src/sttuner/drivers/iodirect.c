#include "sti2c.h"
#include "ioarch.h"
#include "iodirect.h"
#ifndef ST_OSLINUX

#include <string.h>
#endif

long PowOf2(int number);

#ifdef STTUNER_MINIDRIVER
	STI2C_Handle_t DEMOD_I2CHandle, LNB_I2CHandle, TUNER_I2CHandle;
#endif
#ifdef STTUNER_DRV_SAT_STV0299 
  #define NSBUFFER_SIZE 70
#elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E) ||defined(STTUNER_DRV_TER_STV0360)
  #define NSBUFFER_SIZE 81
#endif
#define REPEATER_ON 
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))

/* ----------------------------------------------------------------------------
Name: STTUNER_IODIRECT_Open()

---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IODEMOD_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams)
{

	#ifdef STTUNER_MINIDRIVER
	
	ST_ErrorCode_t     Error;
    	STI2C_OpenParams_t I2cOpenParams;
        memset(&I2cOpenParams, '\0', sizeof( STI2C_OpenParams_t ) );

	/* ---------- Open an existing, Initialized I2C driver ---------- */
	I2cOpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
	I2cOpenParams.BusAccessTimeOut = 20; /* GNBvd07229 */
	I2cOpenParams.I2cAddress       = OpenParams->Address;

	Error = STI2C_Open(OpenParams->DriverName, &I2cOpenParams, &DEMOD_I2CHandle);
    
        return(Error);
        #endif
        #ifndef STTUNER_MINIDRIVER
	Error = STTUNER_IOARCH_Open(Handle, OpenParams);
	#endif
}


ST_ErrorCode_t STTUNER_IOTUNER_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams)
{

	#ifdef STTUNER_MINIDRIVER
	
	ST_ErrorCode_t     Error;
    	STI2C_OpenParams_t I2cOpenParams;
        memset(&I2cOpenParams, '\0', sizeof( STI2C_OpenParams_t ) );

	/* ---------- Open an existing, Initialized I2C driver ---------- */
	I2cOpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
	I2cOpenParams.BusAccessTimeOut = 20; /* GNBvd07229 */
	I2cOpenParams.I2cAddress       = OpenParams->Address;

	Error = STI2C_Open(OpenParams->DriverName, &I2cOpenParams, &TUNER_I2CHandle);
    
        return(Error);
        #endif
        #ifndef STTUNER_MINIDRIVER
	Error = STTUNER_IOARCH_Open(Handle, OpenParams);
	#endif
}

#if defined (STTUNER_DRV_SAT_LNB21) || defined (STTUNER_DRV_SAT_LNBH21)  || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
/* ----------------------------------------------------------------------------
Name: STTUNER_IOLNB_Open()

---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOLNB_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams)
{

	#ifdef STTUNER_MINIDRIVER
	
	ST_ErrorCode_t     Error;
    	STI2C_OpenParams_t I2cOpenParams;
        memset(&I2cOpenParams, '\0', sizeof( STI2C_OpenParams_t ) );

	/* ---------- Open an existing, Initialized I2C driver ---------- */
	I2cOpenParams.AddressType      = STI2C_ADDRESS_7_BITS;
	I2cOpenParams.BusAccessTimeOut = 20; /* GNBvd07229 */
	I2cOpenParams.I2cAddress       = OpenParams->Address;

	Error = STI2C_Open(OpenParams->DriverName, &I2cOpenParams, &LNB_I2CHandle);
    
        return(Error);
        #endif
        #ifndef STTUNER_MINIDRIVER
	Error = STTUNER_IOARCH_Open(Handle, OpenParams);
	return(Error);
	#endif
}
#endif
/* ----------------------------------------------------------------------------
Name: STTUNER_IODEMOD_Close()

---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IODEMOD_Close(IOARCH_Handle_t *Handle, STTUNER_IOARCH_CloseParams_t  *CloseParams)
{
	ST_ErrorCode_t     Error;

	#ifdef STTUNER_MINIDRIVER
	
	Error = STI2C_Close( DEMOD_I2CHandle );
    
        return(Error);
        
        #endif
        #ifndef STTUNER_MINIDRIVER
	Error = STTUNER_IOARCH_Close(Handle, CloseParams);
	return(Error);
	#endif
}
#if defined (STTUNER_DRV_SAT_LNB21) || defined (STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
/* ----------------------------------------------------------------------------
Name: STTUNER_IODEMOD_Close()

---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IOLNB_Close(IOARCH_Handle_t *Handle, STTUNER_IOARCH_CloseParams_t  *CloseParams)
{

	ST_ErrorCode_t     Error;
	#ifdef STTUNER_MINIDRIVER
	
	Error = STI2C_Close( LNB_I2CHandle );
    
        return(Error);
        #endif
        #ifndef STTUNER_MINIDRIVER
	Error = STTUNER_IOARCH_Close(Handle, CloseParams);
	#endif
}

#endif
/*---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_IODIRECT_ReadWrite(STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 FieldIndex, U8 FieldLength, U8 *Data, U32 TransferSize, BOOL REPEATER)
{
#ifdef STTUNER_DEBUG_MODULE_IOARCH
   const char *identity = "STTUNER iodirect.c STTUNER_IODIRECT_ReadWrite()";
#endif

	ST_ErrorCode_t     Error;
	U32 Size;
    	U8  SubAddress;
    	U8  nsbuffer[NSBUFFER_SIZE];
    	U8 d1, n = 0;
    	U32 Timeout = 20;
    	U8  Length;
    	U8 mask = 0x00;
    	
    	SubAddress = (U8)(SubAddr & 0xFF);
	
	#ifdef STTUNER_MINIDRIVER
	
	if(REPEATER == TRUE)
	{
		Error = tuner_repeater_on();
		if(Error != ST_NO_ERROR)
		return(Error);
		if(Operation == STTUNER_IO_SA_READ)
		Operation = STTUNER_IO_READ;
		if(Operation == STTUNER_IO_SA_WRITE)
		Operation = STTUNER_IO_WRITE;
	}
	
	if((Operation == STTUNER_IO_SA_WRITE ) && FieldLength != 0)
	{
		Length = FieldLength;
		while(Length>0)
		{
			/*mask += (U32)(pow(2,(n+FieldIndex)));*/
			mask += (U8)PowOf2((int)(n+FieldIndex));
			--Length;
			++n;
		}
		Error = STI2C_Write(DEMOD_I2CHandle, &SubAddress, 1, Timeout, &Size);
		Error = STI2C_Read(DEMOD_I2CHandle, &d1, 1, Timeout, &Size);
		d1 &= ~(mask);
		mask &= ((*Data) << FieldIndex);
		*Data = d1 | mask;
		
	}
	
		
	if(REPEATER == TRUE)
	{
		switch (Operation)
	    	{
	        
	        case STTUNER_IO_SA_READ:
	            Error = STI2C_Write((STI2C_Handle_t)TUNER_I2CHandle, &SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
	            /* fallthrough (no break;) */
	        case STTUNER_IO_READ:
	            if (Error == ST_NO_ERROR)
	            Error = STI2C_Read((STI2C_Handle_t)TUNER_I2CHandle, Data,TransferSize, Timeout, &Size);
	            break;       
	
	        case STTUNER_IO_SA_WRITE:
	            
	            nsbuffer[0] = SubAddress;
	            memcpy( (nsbuffer + 1), Data, TransferSize);
	            
	            Error = STI2C_Write( (STI2C_Handle_t)TUNER_I2CHandle, nsbuffer, TransferSize+1, Timeout, &Size);
	            break;
	
	        case STTUNER_IO_WRITE:
	            if (Error == ST_NO_ERROR)
	                Error = STI2C_Write( (STI2C_Handle_t)TUNER_I2CHandle, Data, TransferSize, Timeout, &Size);
	            break;
	
	        /* ---------- Error ---------- */
	        default:
	#ifdef STTUNER_DEBUG_MODULE_IOARCH
	            STTBX_Print(("%s not in STTUNER_IOARCH_Operation_t %d\n", identity, Operation));
	#endif
	            Error = STTUNER_ERROR_ID;
	            break;
	    }
	
	}
	else
	{
	switch (Operation)
    	{
        
        case STTUNER_IO_SA_READ:
            Error = STI2C_Write((STI2C_Handle_t)DEMOD_I2CHandle, &SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
            /* fallthrough (no break;) */
        case STTUNER_IO_READ:
            if (Error == ST_NO_ERROR)
            Error = STI2C_Read((STI2C_Handle_t)DEMOD_I2CHandle, Data,TransferSize, Timeout, &Size);
            break;       

        case STTUNER_IO_SA_WRITE:
            
            nsbuffer[0] = SubAddress;
            memcpy( (nsbuffer + 1), Data, TransferSize);
            
            Error = STI2C_Write( (STI2C_Handle_t)DEMOD_I2CHandle, nsbuffer, TransferSize+1, Timeout, &Size);
            break;

        case STTUNER_IO_WRITE:
            if (Error == ST_NO_ERROR)
                Error = STI2C_Write( (STI2C_Handle_t)DEMOD_I2CHandle, Data, TransferSize, Timeout, &Size);
            break;
     
#ifdef STTUNER_USE_SAT       
        case STTUNER_IO_SA_LNB_READ:
            Error = STI2C_Write((STI2C_Handle_t)LNB_I2CHandle, &SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
            if (Error == ST_NO_ERROR)
            Error = STI2C_Read((STI2C_Handle_t)LNB_I2CHandle, Data,TransferSize, Timeout, &Size);
            break;
            
        case STTUNER_IO_LNB_READ:
            Error = STI2C_Read((STI2C_Handle_t)LNB_I2CHandle, Data,TransferSize, Timeout, &Size);
            break;       

        case STTUNER_IO_SA_LNB_WRITE:
            
            nsbuffer[0] = SubAddress;
            memcpy( (nsbuffer + 1), Data, TransferSize);
           
            Error = STI2C_Write( (STI2C_Handle_t)LNB_I2CHandle, nsbuffer, TransferSize+1, Timeout, &Size);
            break;
            
        case STTUNER_IO_LNB_WRITE:
            
            if(Error == ST_NO_ERROR)
            Error = STI2C_Write( (STI2C_Handle_t)LNB_I2CHandle, Data, TransferSize, Timeout, &Size);
            break;
#endif

        /* ---------- Error ---------- */
        default:
#ifdef STTUNER_DEBUG_MODULE_IOARCH
            STTBX_Print(("%s not in STTUNER_IOARCH_Operation_t %d\n", identity, Operation));
#endif
            Error = STTUNER_ERROR_ID;
            break;
    }
    
    if((Operation == STTUNER_IO_SA_READ ) && FieldLength != 0)
	{
		Length = FieldLength;
		while(Length>0)
		{
			
			mask += (U8)PowOf2((int)(n+FieldIndex));
			--Length;
			++n;
		}
		
		*Data &= (mask);
		*Data = ((*Data) >> FieldIndex);
		
	}
    }
        return(Error);
        
        #endif
        #ifndef STTUNER_MINIDRIVER
	
	#endif
}

ST_ErrorCode_t tuner_repeater_on(void)
{
	ST_ErrorCode_t     Error;
	
    	U8  nsbuffer[2];
    	U32 Size;
    	
    	#ifdef STTUNER_DRV_SAT_STV0299
    	nsbuffer[0] = 0x05;
    	#endif
    	#ifdef STTUNER_DRV_TER_STV0360
    	nsbuffer[0] = 0x01;
    	#endif
    	Error  = STI2C_Write(DEMOD_I2CHandle, nsbuffer, 1, 20, &Size); /* fix for cable (297 chip) */
        Error |= STI2C_Read(DEMOD_I2CHandle, nsbuffer,1, 20, &Size);
        
        nsbuffer[1] = nsbuffer[0] | 0x80;
        #ifdef STTUNER_DRV_SAT_STV0299
        nsbuffer[0] = 0x05;
    	#endif
    	#ifdef STTUNER_DRV_TER_STV0360
    	nsbuffer[0] = 0x01;
    	#endif
        
        Error = STI2C_Write(DEMOD_I2CHandle, nsbuffer, 2, 20, &Size);
        return(Error);
}

long PowOf2(int number)
{
	int i;
	long result=1;
	
	for(i=0;i<number;i++)
		result*=2;
		
	return result;
}

#ifdef STTUNER_DRV_TER_STV0360
ST_ErrorCode_t ChipDemodSetField(U32 FieldId, U8 Value)
{
    ST_ErrorCode_t Error;
    U8  Datval ;
   		union v32bit {
		U32 dble_wrd;  
		U16 wrd[2];
		U8  byt[4];
			 };
    static union v32bit      F_Id;

    F_Id.dble_wrd = FieldId;
	
    Datval= Value;
    /*(RX123)||(FX_S << 16)|(FX_L << 24)*/
	
	Error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_WRITE, F_Id.wrd[0], F_Id.byt[2],F_Id.byt[3], &Datval, 1, FALSE);
        
	return(Error);
}


ST_ErrorCode_t ChipDemodGetField( U32 FieldId)
{
    
    ST_ErrorCode_t Error;

    U8  Datval ;
    union v32bit {
		U32 dble_wrd;  
		U16 wrd[2];
		U8  byt[4];
			 };
    static union v32bit      F_Id;

    F_Id.dble_wrd = FieldId;
	
	
	Error=STTUNER_IODIRECT_ReadWrite(STTUNER_IO_SA_READ, F_Id.wrd[0], F_Id.byt[2],F_Id.byt[3], &Datval, 1, FALSE);
    
	return(Datval);
}

#endif /* include two fns for 360 only*/
