/* ----------------------------------------------------------------------------
File Name: diseqcdrv.c

Description: 

    
Copyright (C) 2004-2005 STMicroelectronics

History:
 
   date: 
version: 
 author: 
comment: 

date: 
version: 
 author: 
comment: 

date: 
version: 
 author: 
comment: 
    
Reference:

    
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */

      
#ifndef ST_OSLINUX
   #include <string.h>
#endif
#include "stlite.h"     /* Standard includes */
#include "sttbx.h"
#include "stcommon.h"
/* STAPI */

#include "sttuner.h"                    

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "diseqc.h"     

/* DISEQC register offset */
#define DISEQC_NUM_REGITERS 35 /*Number of diseqc registers*/



	/**Transmitter registers*/
#define DISEQC_TX_ENABLE  		 0x00/*  0x00  Enable message transmission */
#define DISEQC_TX_MESSAGE_CONFIG 	 0x01/* 0x04 Transmit message configuration*/
#define DISEQC_TX_PRE_SCALER 		 0x02/* 0x08 Holds the pre scaler value for transmission*/
#define DISEQC_TX_SUBCARRIER_DIV	 0x03/* 0x0C Holds the sub carrrier generation division value*/
#define DISEQC_TX_SILENCE_PERIOD	 0x04/* 0x10 Transmit message silence period*/
#define DISEQC_TX_DATA_BUFFER		 0x05/* 0x14 Transmit data buffer*/
#define DISEQC_TX_SYMBOL_PERIOD   	 0x06/* 0x18 Period of symbol to be transmitted */
#define DISEQC_TX_SYMBOL0_ONTIME  	 0x07 /*0x1C Symbol 0 on time period*/
#define DISEQC_TX_SYMBOL1_ONTIME  	 0x08/* 0x20 Symbol 1 on time period*/
#define DISEQC_TX_SOFT_RESET		 0x09/* 0x24 Soft resets the Tx section*/
#define DISEQC_TX_START			 0x0A/* 0x28 start message transmission*/
#define DISEQC_TX_INTERRUPT_ENABLE	 0x0B/* 0x2C Enable tx interrupt generation*/
#define DISEQC_TX_INTERRUPT_STATUS       0x0C/* 0x30 Tx interrupt status*/
#define DISEQC_TX_STATUS	         0x0D/* 0x34  Tx status */
#define DISEQC_TX_CLR_INTERRUPT_STATUS   0x0E/* 0x38 Tx clear interrupt status*/
#define DISEQC_RESERVED			 0x0F/* 0x3C reserved for future use*/
#define DISEQC_TX_AGC_CONTROL	     	 0x10/* 0x40  Tx AGC control*/

	/**Receiver registers*/
#define DISEQC_RX_ENABLE  		 0x20/*0x80  Enable receiving section */
#define DISEQC_RX_SAMPLING_PERIOD	 0x21/*0x84 Holds the sampling value*/	
#define DISEQC_RX_BYTE_COUNT		 0x22/*0x88 Number of bytes received*/
#define DISEQC_RX_SILENCE_PERIOD         0x23/*0x8C Receive message silence period*/
#define DISEQC_RX_DATA_BUFFER		 0x24/*0x90 Receive data buffer*/
#define DISEQC_RX_SYMBOL0_MIN_THRESHOLD  0x25/*0x94 Min. threshold for symbol 0 on time */
#define DISEQC_RX_SYMBOL0_MAX_THRESHOLD  0x26/*0x98 Max. threshold for symbol 0 on time*/
#define DISEQC_RX_SYMBOL1_MIN_THRESHOLD  0x27/*0x9C Min. threshold for symbol 1 on time*/
#define DISEQC_RX_SYMBOL1_MAX_THRESHOLD  0x28/*0xA0 Max. threshold for symbol 1 on time*/
#define DISEQC_RX_SOFTRESET		 0x29/*0xA4 Soft resets the Rx section*/
#define DISEQC_RX_INTERRUPT_ENABLE   	 0x2A/*0xA8 Enable Rx interrupt generation*/
#define DISEQC_RX_INTERRUPT_STATUS	 0x2B/*0xAC Rx interrupt status*/
#define DISEQC_RX_STATUS	         0x2C/*0xB0 Rx status*/
#define DISEQC_RX_CLR_INTERRUPT_STATUS   0x2D/*0xB4 Clear Rx interrupt status*/
#define DISEQC_RX_TIME_OUT	    	 0x2E/*0xB8 Rx timeout*/
#define DISEQC_RX_NOISE_SUPPRESS_WIDTH   0x2F/*0xBC Rx noise suppression width*/
#define DISEQC_POLARITY_INV		 0x30/*0xC0 Rx polarity inversion*/
#define DISEQC_SUBCARRIER_SUPRESS_ENABLE 0x31/*0xC4 Rx sub carrier supression enable*/




static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */

static BOOL        Installed = FALSE;


#define Delay_ms(x) STOS_TaskDelayUs(1000*x);

volatile U32   *RegPtr;
static int Rx_array_ptr = 0;
U32 data_rx_array[20]; /*Maximum receive buffer size taken as 20. Can be changed according to the application*/
int gRx_byte_count; /*Byte counter register value to be stored*/
int gDiseqc_rx_status;/*Byte counter register value to be stored*/
static int global1=0;
/* ---------- per instance of driver ---------- */
typedef struct DISEQC_InstanceData_s
{
    ST_DeviceName_t        *DeviceName;           /* unique name for opening under */
    STTUNER_Handle_t       TopLevelHandle;       /* access tuner, lnb etc. using this */
    IOARCH_Handle_t        IOHandle;             /* instance access to I/O driver     */
        
    ST_Partition_t         *MemoryPartition;     /* which partition this data block belongs to */
    void                   *InstanceChainPrev;   /* previous data block in chain or NULL if not */
    void                   *InstanceChainNext;   /* next data block in chain or NULL if last */ 
    U32                    *BaseAddress;/*Base address for accessing the diseqc registers*/
#ifdef ST_OS21
    interrupt_name_t       InterruptNumber; /*Interrupt number for diseqc*/
#else 
    U32                    InterruptNumber;/*Interrupt number for diseqc*/
#endif
    U32                    InterruptLevel; /*interrupt level for diseqc*/
}DISEQC_InstanceData_t;


/* instance chain, the default boot value is invalid, to catch errors */
static DISEQC_InstanceData_t *InstanceChainTop = (DISEQC_InstanceData_t *)0x7fffffff;


semaphore_t *DISEQC_Tx_Ready,*DISEQC_Rx_last_byte_received;

/* functions --------------------------------------------------------------- */

/* API */
ST_ErrorCode_t diseqc_diseqcdrv_Init(ST_DeviceName_t *DeviceName, DISEQC_InitParams_t *InitParams);
ST_ErrorCode_t diseqc_diseqcdrv_Term(ST_DeviceName_t *DeviceName, DISEQC_TermParams_t *TermParams);

ST_ErrorCode_t diseqc_diseqcdrv_Open (ST_DeviceName_t *DeviceName, DISEQC_OpenParams_t  *OpenParams, DISEQC_Handle_t  *Handle,DISEQC_Capability_t *Capability);
ST_ErrorCode_t diseqc_diseqcdrv_Close(DISEQC_Handle_t  Handle, DISEQC_CloseParams_t *CloseParams);
ST_ErrorCode_t diseqc_diseqcdrv_SendReceive (DISEQC_Handle_t,STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
						STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket);
/* local functions --------------------------------------------------------- */

DISEQC_InstanceData_t *DISEQC_GetInstFromHandle(DISEQC_Handle_t Handle);


static void Diseqc_tx_interrupt_handler(void )
{
  U32  diseqc_tx_int_status;
 global1++;
  diseqc_tx_int_status = RegPtr[DISEQC_TX_INTERRUPT_STATUS]; 
   /* TX READY */
  if ( diseqc_tx_int_status & 0x08 )
  {
    RegPtr[DISEQC_TX_CLR_INTERRUPT_STATUS] = 0x08;
    RegPtr[DISEQC_TX_INTERRUPT_ENABLE] &= ~0x08;
    STOS_SemaphoreSignal(DISEQC_Tx_Ready);
  }
}

static U32 Swap_u32(U32 buffer)
{
  U32 TempData;
  int i;

  TempData = 0;
  if ( buffer > 0xffffff)
  {
    for ( i=0;i<32;i++)
	{
	  TempData |= (((buffer>>i)&1)<< (31-i));
	}
  }
  else if ( buffer > 0xffff)
  {
    for ( i=0;i<24;i++)
	{
	  TempData |= (((buffer>>i)&1)<< (23-i));
	}
  }
  else if ( buffer > 0xff)
  {
    for ( i=0;i<16;i++)
	{
	  TempData |= (((buffer>>i)&1)<< (15-i));
	}
  }
  else /* if (buffer < 0x1ff) */
  {
    for ( i=0;i<8;i++)
	{
	  TempData |= (((buffer>>i)&1)<< (7-i));
	}
  }

  return TempData;
}


static void Diseqc_rx_interrupt_handler( void )
{
  int diseqc_rx_int_status;
  int rx_fifo_status;
  int i;

  /* read the Rx interrupt status */
  gDiseqc_rx_status = RegPtr[DISEQC_RX_STATUS];
  diseqc_rx_int_status = RegPtr[DISEQC_RX_INTERRUPT_STATUS];
  
  /* RX LAST BYTE RECEIVED */
  if ( diseqc_rx_int_status & 0x010)
  {
    rx_fifo_status = (RegPtr[DISEQC_RX_STATUS]>>9)&0x7;
   
    if (rx_fifo_status != 0)
    {
      for (i=0; i<rx_fifo_status; i++)
       {
        data_rx_array[Rx_array_ptr] = RegPtr[DISEQC_RX_DATA_BUFFER];
       	gRx_byte_count = RegPtr[DISEQC_RX_BYTE_COUNT];
       	data_rx_array[Rx_array_ptr] = Swap_u32(data_rx_array[Rx_array_ptr]);
      	switch ( gRx_byte_count )
        {
	  case 0 :
	    data_rx_array[Rx_array_ptr] &= 0x00;
	    break;
	  case 1 :
	    data_rx_array[Rx_array_ptr] &= 0xff;
	    break;
	  case 2 :
	    data_rx_array[Rx_array_ptr] &= 0xffff;
	    break;
	  case 3 :
	    data_rx_array[Rx_array_ptr] &= 0xffffff;
	    break;
	}
		Rx_array_ptr++;
      }
    }
	/* clear the interrupt status */
    RegPtr[DISEQC_RX_CLR_INTERRUPT_STATUS] = 0x10;
    RegPtr[DISEQC_RX_INTERRUPT_ENABLE] &= ~0x10;

   STOS_SemaphoreSignal(DISEQC_Rx_last_byte_received);
  }
}


static void  DiseqcHandler(void)
{

 if ( (RegPtr[DISEQC_TX_INTERRUPT_STATUS] & 0x01) == 0x01)
  {
     Diseqc_tx_interrupt_handler ();
     
  }
  if ( (RegPtr[DISEQC_RX_INTERRUPT_STATUS] & 0x01) == 0x01)
  {
     Diseqc_rx_interrupt_handler ();
  }      
  
  return;
}

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DISEQC_Install()

Description:
    
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DISEQC_5100_530X_Install(STTUNER_diseqc_dbase_t *Diseqc)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == TRUE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver already installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s installing sat:diseqc...", identity));
#endif

    /* mark ID in database */
    Diseqc->ID = STTUNER_DISEQC_5100;

    /* map API */
    Diseqc->diseqc_Init = diseqc_diseqcdrv_Init;
    Diseqc->diseqc_Term = diseqc_diseqcdrv_Term;

    Diseqc->diseqc_Open  = diseqc_diseqcdrv_Open;
    Diseqc->diseqc_Close = diseqc_diseqcdrv_Close;
    
    Diseqc->diseqc_SendReceive = diseqc_diseqcdrv_SendReceive;
    
    
    InstanceChainTop = NULL;


    Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

    Installed = TRUE;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_DISEQC_unInstall()

Description:
   
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_DISEQC_5100_530X_unInstall(STTUNER_diseqc_dbase_t *Diseqc)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
   const char *identity = "STTUNER diseqc STTUNER_DRV_DISEQC_unInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }
   
    if(Diseqc->ID != STTUNER_DISEQC_5100)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail incorrect driver type\n", identity));
#endif
        return(STTUNER_ERROR_ID);
    }

    /* has all memory been freed, by Term() */
    if(InstanceChainTop != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail at least one instance not terminated\n", identity));
#endif
        return(ST_ERROR_OPEN_HANDLE);
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s uninstalling sat:diseqc...", identity));
#endif

    /* mark ID in database */
    Diseqc->ID = STTUNER_NO_DRIVER;

    /* unmap API */
    Diseqc->diseqc_Init = NULL;
    Diseqc->diseqc_Term = NULL;

    Diseqc->diseqc_Open  = NULL;
    Diseqc->diseqc_Close = NULL;
    Diseqc->diseqc_SendReceive = NULL;
    
	
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("<"));
#endif

    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print((">"));
#endif

    InstanceChainTop = (DISEQC_InstanceData_t *)0x7ffffffe;
    
    STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);
 
    Installed        = FALSE;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("ok\n"));
#endif

    return(Error);
}



/* ----------------------------------------------------------------------------
Name: diseqc_diseqcdrv_Init()

Description:
    
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t diseqc_diseqcdrv_Init(ST_DeviceName_t *DeviceName, DISEQC_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    const char *identity = "STTUNER diseqc diseqc_diseqcdrv_Init()";
#endif
    S32                  IntReturn;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    DISEQC_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail MemoryPartition not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( DISEQC_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif    
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);           
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }

    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->BaseAddress	     = InitParams->Init_Diseqc.BaseAddress;
    InstanceNew->InterruptNumber     = InitParams->Init_Diseqc.InterruptNumber;
    InstanceNew->InterruptLevel     = InitParams->Init_Diseqc.InterruptLevel;
   
   
  /**************************************************/

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail setup register database params\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);           
    }
  
     /* install and enable interrupt for Diseqc2.0 on 5100*/
    IntReturn = interrupt_install ( InstanceNew->InterruptNumber,
                                                InstanceNew->InterruptLevel,
                                                (void (*) (void *)) DiseqcHandler,(void*)NULL );
                                               
     if (IntReturn != 0)
      {
    	return ST_ERROR_INTERRUPT_INSTALL;    
      }
      else
      {
      	  interrupt_enable_number(InstanceNew->InterruptNumber);
      	/* code to be added here */
      }


 /*Create handler related semaphores*/
/* create semaphores */
  DISEQC_Tx_Ready = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
  DISEQC_Rx_last_byte_received = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
  
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes)\n", identity, InstanceNew->DeviceName, (U32)InstanceNew, sizeof( SCR_InstanceData_t ) ));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/*----------------------------------------------------------------------------
Name: diseqc_diseqcdrv_Term()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t diseqc_diseqcdrv_Term(ST_DeviceName_t *DeviceName, DISEQC_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
   const char *identity = "STTUNER diseqc diseqc_diseqcdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    DISEQC_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s looking for first free handle[\n", identity));
#endif

    Instance = InstanceChainTop;
    while(1)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        if ( strcmp( (char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("]\n"));
#endif
            
            /*uninstall the inrrupt handler here, is it necessary to disable the interrput as well???*/
            if (interrupt_uninstall(Instance->InterruptNumber,Instance->InterruptLevel) != 0)
            {
            	return ST_ERROR_INTERRUPT_UNINSTALL;
            }
            
            interrupt_disable_number(Instance->InterruptNumber);
            
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if(InstanceNext != NULL)
                {
                InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */            
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }   

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL) 
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        
    }


#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: diseqc_diseqcdrv_Open()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t diseqc_diseqcdrv_Open(ST_DeviceName_t *DeviceName, DISEQC_OpenParams_t  *OpenParams, DISEQC_Handle_t  *Handle,DISEQC_Capability_t *Capability)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
   const char *identity = "STTUNER diseqcdrv.c diseqc_diseqcdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    DISEQC_InstanceData_t *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL) 
        {       
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print((" found ok\n"));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* finally (as nor more errors to check for, allocate handle */
    Instance->TopLevelHandle = OpenParams->TopLevelHandle;
     /* now got pointer to free (and valid) data block */
    *Handle = (U32)Instance;

     /*Get the base pointer from Instance database*/
     RegPtr = Instance->BaseAddress;

    /* FOR TRANSMISSION THIS CODE SHOULD BE DONE ONCE*/
  
     /*generate a 22Khz sub carrier frequency */
     RegPtr[DISEQC_TX_PRE_SCALER] = 0x02;
     RegPtr[DISEQC_TX_SUBCARRIER_DIV] = 0x4b9/*0x5E7*/ /*1511*/ ;/*106Mhz of system clock frequency*/   
        
    /* Configure the symbol period time to 1.5ms */
    RegPtr[DISEQC_TX_SYMBOL_PERIOD]= 0x21;
    
    /*configure symbol 0 and symbol 1 on time .... may be confussing between 0 and 1???? */
    RegPtr[DISEQC_TX_SYMBOL0_ONTIME] = 0x16;/*1 ms*/
    RegPtr[DISEQC_TX_SYMBOL1_ONTIME] = 0x0B;/* 0.5ms*/
    /*disable soft reset*/
    RegPtr[DISEQC_TX_SOFT_RESET] = 0x00; 
    
    /*Now enable the transmission section*/
    RegPtr[DISEQC_TX_ENABLE] = 0x00; 

    /*Disable all Tx interrupts*/
    RegPtr[DISEQC_TX_INTERRUPT_ENABLE] = 0x00 ;  

  
 
    /* FOR RECEIVING THIS CODE SHOULD BE DONE ONCE*/
    /*configure the sampling period time*/
    RegPtr[DISEQC_RX_SAMPLING_PERIOD] = 0x12e6;/* System clock  106Mhz,sampling frequency 22Khz*/
   	 
    RegPtr[DISEQC_POLARITY_INV] = 0x01;/*Polarity inverted*/
   	 
   	 /*configure the silence period time*/
    RegPtr[DISEQC_RX_SILENCE_PERIOD] = 0x113;/*12.5 ms  silence period for 22Khz sampling clock*/
   	 
    /*configure the min , max symbol 0 ,1 threshold period*/
    RegPtr[DISEQC_RX_SYMBOL0_MIN_THRESHOLD] = 0x11 ;/* 0.77ms for 22Khz sampling clock*/ 
    RegPtr[DISEQC_RX_SYMBOL0_MAX_THRESHOLD] = 0x1a;/*  1.18ms for 22Khz sampling clock*/ 
    RegPtr[DISEQC_RX_SYMBOL1_MIN_THRESHOLD] = 0x06 ;/* 0.27ms for 22Khz sampling clock*/ 
    RegPtr[DISEQC_RX_SYMBOL1_MAX_THRESHOLD] = 0x0f ;/* 0.68ms for 22Khz sampling clock*/ 
   	 
    RegPtr[DISEQC_RX_TIME_OUT] = 0x6e ;
    RegPtr[DISEQC_RX_NOISE_SUPPRESS_WIDTH] = 0x02;
    /*disable the soft reset */
    RegPtr[DISEQC_RX_SOFTRESET] =0x00;
    /*enable the receive section */
    RegPtr[DISEQC_RX_ENABLE] =0x00;
    /* Set DISEQC capability*/ 
    Capability->DISEQC_Mode = STTUNER_DISEQC_DEFAULT;
    Capability->DISEQC_VER = STTUNER_DISEQC_VER_2_0;
    
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   



/* ----------------------------------------------------------------------------
Name: diseqc_diseqcdrv_Close()

Description:
    
Parameters:
    
Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t diseqc_diseqcdrv_Close(DISEQC_Handle_t Handle, DISEQC_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
   const char *identity = "STTUNER diseqc diseqc_diseqcdrv_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    DISEQC_InstanceData_t     *Instance;

    /* private driver instance data */
    Instance = DISEQC_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }


    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;

#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
    STTBX_Print(("%s closed\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}   


/* ----------------------------------------------------------------------------
Name: diseqc_diseqcdrv_SendReceive()

Description:
     
   
Parameters:
         Handle -- DISEQC Handle
    
Return Value: Instance pointer
---------------------------------------------------------------------------- */

ST_ErrorCode_t diseqc_diseqcdrv_SendReceive (DISEQC_Handle_t Handle,STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket,
						STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket)
{
#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
   const char *identity = "STTUNER diseqc diseqc_diseqcdrv_SendReceive()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    DISEQC_InstanceData_t   *Instance;
    int i,j;
    U8 *TempTxData,NumWord;
    LNB_Handle_t LHandle;
    STTUNER_InstanceDbase_t *Inst;
#ifdef ST_OS21
    osclock_t Rx_delay,Rx_TimeOut,Tx_delay,Tx_TimeOut;
#else
    clock_t Rx_delay,Rx_TimeOut,Tx_delay,Tx_TimeOut;
#endif
    /* private driver instance data */
    Instance = DISEQC_GetInstFromHandle(Handle);
    RegPtr = Instance->BaseAddress;

    
    if(pDiSEqCSendPacket->uc_TotalNoOfBytes)
    {
    if ((TempTxData = STOS_MemoryAllocateClear(Instance->MemoryPartition,pDiSEqCSendPacket->uc_TotalNoOfBytes,1)) == NULL)
       {
       	#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
        STTBX_Print(("%s fail memory allocation not done\n", identity));
        #endif
        return(ST_ERROR_NO_MEMORY);
       }
    }
     
    /*Set LNBH21 in Tx mode */
    Inst = STTUNER_GetDrvInst();
    LHandle = Inst[Instance->TopLevelHandle].Sat.Lnb.DrvHandle;
   
    Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_TX);
    if (Error != ST_NO_ERROR)
     {
     	#ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
         STTBX_Print(("%s fail driver lnbh ttx mode not set\n", identity));
        #endif
        return(Error);		
     }
   
   if(pDiSEqCSendPacket->uc_msecBeforeNextCommand == 0)
   {
   	pDiSEqCSendPacket->uc_msecBeforeNextCommand = 1;
   }
    /*Configure the silence period between messages*/
    RegPtr[DISEQC_TX_SILENCE_PERIOD]= pDiSEqCSendPacket->uc_msecBeforeNextCommand*22; 
   
    switch (pDiSEqCSendPacket->DiSEqCCommandType)
    {
    case STTUNER_DiSEqC_COMMAND:
    	RegPtr[DISEQC_TX_MESSAGE_CONFIG]= 0x00;
    	RegPtr[DISEQC_TX_MESSAGE_CONFIG] = (pDiSEqCSendPacket->uc_TotalNoOfBytes)<<2;    	
    	break;
    case STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED:
    /*case STTUNER_DiSEqC_TONE_BURST_SEND_1:*/
      	RegPtr[DISEQC_TX_MESSAGE_CONFIG]= 0x03;
  	RegPtr[DISEQC_TX_MESSAGE_CONFIG]= RegPtr[DISEQC_TX_MESSAGE_CONFIG] |(0x09<<9);
        break;
   /* case STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH:*/
    case STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED:
    	RegPtr[DISEQC_TX_MESSAGE_CONFIG]= 0x01;
        RegPtr[DISEQC_TX_MESSAGE_CONFIG] = RegPtr[DISEQC_TX_MESSAGE_CONFIG] | (0x09<<9);
        break;
    case STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON:
        RegPtr[DISEQC_TX_MESSAGE_CONFIG]= 0x010000;
         /*Now enable the transmission section*/
        RegPtr[DISEQC_TX_ENABLE] = 0x01; 
        return (ST_NO_ERROR);
    case STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW:
    case STTUNER_DiSEqC_TONE_BURST_OFF:
        RegPtr[DISEQC_TX_MESSAGE_CONFIG]= 0x00;
        /*Now enable the transmission section*/
        RegPtr[DISEQC_TX_ENABLE] = 0x01; 
        return (ST_NO_ERROR);
     default:
        break;/*STTUNER_ERROR_UNSUPPORTED_MESSAGE_TYPE*/;
    }
    
    
    /*During transmission ,disable reception */
       RegPtr[DISEQC_RX_ENABLE] = 0x00;
       RegPtr[DISEQC_RX_INTERRUPT_ENABLE] = 0x00;

if (pDiSEqCSendPacket->DiSEqCCommandType == STTUNER_DiSEqC_COMMAND)
{
          /*Check for total number of words to be sent*/
         NumWord = pDiSEqCSendPacket->uc_TotalNoOfBytes/4;
         if (pDiSEqCSendPacket->uc_TotalNoOfBytes%4 != 0)
         {
   	   NumWord +=1;
         }
       
        /*Software work around for MSB to LSB conversion*/
        for(i=0;i<pDiSEqCSendPacket->uc_TotalNoOfBytes;i++)
         {   	   
          for(j=0;j<8;j++)
           {
     	    TempTxData[i] = (TempTxData[i]|(((*(pDiSEqCSendPacket->pFrmAddCmdData+i)>>j)&0x01)<<(7-j)));
           }   
         }
   
      
       /*get the current fifo status*/
  
      for (i =0; i < NumWord; i++)  
       {
         RegPtr[DISEQC_TX_DATA_BUFFER] = *(U32 *)TempTxData;
         TempTxData = TempTxData+4;
       }
}       
   /*Now enable the transmission section*/
   RegPtr[DISEQC_TX_ENABLE] = 0x01; 
   
   
   /*Clear interrupt status for Tx Ready*/
   RegPtr[DISEQC_TX_CLR_INTERRUPT_STATUS] = 0x09;
   
   Tx_delay = STOS_time_now();
   /* Start transmission of the new message */
   RegPtr[DISEQC_TX_START] = 0x01;
   /*Enable interrupt for Tx ready*/
   RegPtr[DISEQC_TX_INTERRUPT_ENABLE] = 0x09 ;
  
   
   
    /* wait for transmission to complete */ /*Wait for 1 second*/
	    Tx_TimeOut = STOS_time_plus(Tx_delay, ST_GetClocksPerSecond());
    /*Wait for the Tx to transmit data*/
  if( STOS_SemaphoreWaitTimeOut(DISEQC_Tx_Ready,&Tx_TimeOut) == -1)
  {
     #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s fail diseqc transmitter Transmission timeout occured\n", identity));
     #endif             
    	     return STTUNER_ERROR_DISEQC_TIMEOUT;    
  }
   /*Now disable  the transmission section*/
    RegPtr[DISEQC_TX_ENABLE] = 0x00;     

   if(*pDiSEqCSendPacket->pFrmAddCmdData == FB_COMMAND_REPLY
      || *pDiSEqCSendPacket->pFrmAddCmdData == FB_COMMAND_REPLY_REPEATED)
   {
   	/*reset  byte count register value */
   	gRx_byte_count = 0;
   
   	
   	/*Now set the LNBH21 into Rx mode */   
         Error = (Inst[Instance->TopLevelHandle].Sat.Lnb.Driver->lnb_setttxmode)(LHandle,STTUNER_LNB_RX);
        if (Error != ST_NO_ERROR)
	{
	  #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
             STTBX_Print(("%s fail driver lnbh ttx mode not set\n", identity));
           #endif
	 return(Error);		
	}   	
	
   	Rx_delay = STOS_time_now();
   	/*enable the receive section */
   	 RegPtr[DISEQC_RX_ENABLE] =0x01;
   	/*Enable the receiver interrupt generation*/
        RegPtr[DISEQC_RX_INTERRUPT_ENABLE] = 0x11 ;   	
       
        /* wait for receive complete */
	    Rx_TimeOut = STOS_time_plus(Rx_delay, ST_GetClocksPerSecond());

       if(STOS_SemaphoreWaitTimeOut(DISEQC_Rx_last_byte_received,&Rx_TimeOut)!=-1)
        {
          /*Check for Parity/Symbol/Code Error*/
         if ((gDiseqc_rx_status & 0x80)>>7  )
         {
           #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s fail diseqc receiver SYMBOL error occured\n", identity));
           #endif             
    	     return STTUNER_ERROR_DISEQC_SYMBOL;    	   
    	 }
    	 
    	 if((gDiseqc_rx_status & 0x20)>>5)
    	 {
    	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s fail diseqc receiver CODE error occured\n", identity));
           #endif    	     
    	     return STTUNER_ERROR_DISEQC_CODE;    	
    	 }
    	 if ((gDiseqc_rx_status & 0x40)>>6)
    	 {
    	   #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
            STTBX_Print(("%s fail diseqc receiver PARITY error occured\n", identity));
           #endif
    	   return  STTUNER_ERROR_DISEQC_PARITY;    	    
    	 }
          if ( Rx_array_ptr > 0)
	      {
	      	
	        pDiSEqCResponsePacket->uc_TotalBytesReceived = gRx_byte_count;
	      
                if ((pDiSEqCResponsePacket->ReceivedFrmAddData = STOS_MemoryAllocateClear(Instance->MemoryPartition,pDiSEqCResponsePacket->uc_TotalBytesReceived ,1)) == NULL)
    		  {
       		    #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
                      STTBX_Print(("%s fail memory allocation not done\n", identity));
                    #endif
                   return(ST_ERROR_NO_MEMORY);
                 }          
               pDiSEqCResponsePacket->ReceivedFrmAddData = (U8 *)data_rx_array;                                   
	       Rx_array_ptr = 0;
	      }
         }
         else
         {
           #ifdef STTUNER_DEBUG_MODULE_SATDRV_DISEQC
             STTBX_Print(("%s fail diseqc receiver no message recieved\n", identity));
           #endif
           return STTUNER_ERROR_DISEQC_TIMEOUT ;
         }
         
        /*Now Disable reception */
        RegPtr[DISEQC_RX_ENABLE] =0x00;
      	 /*Enable the receiver interrupt generation*/
        RegPtr[DISEQC_RX_INTERRUPT_ENABLE] = 0x00 ;
        
}
    return ST_NO_ERROR;
}						


/* ----------------------------------------------------------------------------
Name: DISEQC_GetInstFromHandle()

Description:     
   
Parameters:   
Return Value: Instance pointer
---------------------------------------------------------------------------- */
DISEQC_InstanceData_t *DISEQC_GetInstFromHandle(DISEQC_Handle_t Handle)
{

    DISEQC_InstanceData_t *Instance;
    Instance = (DISEQC_InstanceData_t *)Handle;
    return(Instance);
}


