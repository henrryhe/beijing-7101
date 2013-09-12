/************************************************************************

Source file name : cmmd_atapi.c

Description: Implementation of the ATA Command Interface intermediate module
            following the API Design Document v0.9.0 of the ATAPI driver.


COPYRIGHT (C) STMicroelectronics  2000

************************************************************************/

/*Includes-------------------------------------------------------------*/
#include <string.h>
#include "stlite.h"
#include "statapi.h"
#include "hal_atapi.h"
#include "stsys.h"
#include "ata.h"
#include "sttbx.h"

/*Private Types--------------------------------------------------------*/

/*Private Constants----------------------------------------------------*/
/*Private Variables----------------------------------------------------*/

#if defined(ATAPI_DEBUG)
extern U32 atapi_intcount;
extern U32 atapi_inttrace[15];
#endif

/*Private Macros-------------------------------------------------------*/
#define PACKET_TIMEOUT      0x00010000

#define TAG_BIT_MASK        0xF8
#define REL_BIT_MASK        0x04
#define IO_BIT_MASK         0x02
#define CD_BIT_MASK         0x01
#define SERV_BIT_MASK       0x10
#define PACKET_INT_TIMEOUT  0x0100000

/*Private functions prototypes-----------------------------------------*/
static void GetPktStatus(ata_ControlBlock_t *Ata_p,STATAPI_PktStatus_t *Stat_p);


/*Functions------------------------------------------------------------*/
/************************************************************************
Name: ata_packet_NoData

Description: executes a non data packet. The device is already selected
             no parameters check is performed. We totally trust on the caller
Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/
BOOL ata_packet_NoData(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 Status,Reason,LowCyl,HighCyl;
    U32 TimeOut,i;

    Ata_p->HalHndl->DataDirection = ATA_DATA_NONE;

#if  (ATAPI_USING_INTERRUPTS) 
   if(Ata_p->DeviceType == STATAPI_SATA)
       hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

   /* First we write the registers */
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

   if(WaitForBit(Ata_p->HalHndl,ATA_REG_ALTSTAT,DRDY_BIT_MASK,DRDY_BIT_MASK))
   {
       Ata_p->LastExtendedErrorCode= 0x5B;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
   }

   /* Finally write the command */
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);

   WAIT400NS;

   /* Now Check the Status */
    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);

    /* Command packet transfer...*/
    /* Check for protocol failures, the device should
       have BSY=1 now, but a number of strange things can happen:
       1) the device may not set BSY=1 or DRQ=1 for a while
       2) the device may go directly to DRQ status,
       3) the device may reject the command (maybe is not really
          an  ATAPI device or has some error)
    */
    TimeOut= PACKET_TIMEOUT;

    while(TRUE)
    {
        if(Status & BSY_BIT_MASK)
        {
           break; /* BSY=1 that's OK! */
        }else
        {
           if( Status & DRQ_BIT_MASK )
           {
             break;  /* BSY=0 & DRQ=1 That's OK */
           }
           if( Status & ERR_BIT_MASK)
           {
             /* Error,that's not OK */
             break;
           }
        }
        if(TimeOut==0)
        {
            /* Error */
            break;
        }

        TimeOut--;
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);
    }

    /* Command packet transfer....                          */
    /* Poll Alternate Status for BSY=0                      */
    if(WaitForBit(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
    {
       Ata_p->LastExtendedErrorCode= 0x51;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
    }

    /* Command packet transfer....                          */
    /* Check for protocol failures.. clear interrupts       */
    if (hal_ClearInterrupt(Ata_p->HalHndl) != FALSE)
    {
        STTBX_Print(("Note - HDDI interrupt and/or semaphore needed clearing\n"));
    }


    /* Command packet transfer....
       Read the primary status register abd other ATAPI registers
    */
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
        HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

     /* Command packet transfer....
        check status must have BSY=0 DRQ-1
     */

     if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
     {
        Ata_p->LastExtendedErrorCode= 0x52;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
     }


     /* Transfer the packet*/
    {
     	U16	ui16Value;
        for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
        {
     		ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
      	    hal_RegOutWord(Ata_p->HalHndl,ui16Value);
      	    i = i+2;
        }
    }

#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
	    if(hal_AwaitInt(Ata_p->HalHndl,PACKET_INT_TIMEOUT))
	    {
	      Ata_p->LastExtendedErrorCode= 0x53;
	      GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	      return TRUE;
   		}
    }
    else
    {
	     WAIT400NS;
	    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	    {
	       Ata_p->LastExtendedErrorCode= 0x54;
	       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	       return TRUE;
	    }
	}
#else   
	     WAIT400NS;
	    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	    {
	       Ata_p->LastExtendedErrorCode= 0x54;
	       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	       return TRUE;
	    }
#endif /* ATAPI_USING_INTERRUPTS */


   Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
   Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);

    if( (Status & (BSY_BIT_MASK  | DRQ_BIT_MASK /*| SERV_BIT_MASK */| ERR_BIT_MASK))
        | (Reason & REL_BIT_MASK))
    {
       Ata_p->LastExtendedErrorCode= 0x55;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
    }


    GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
    Ata_p->LastExtendedErrorCode= 0x00;

#if  (ATAPI_USING_INTERRUPTS) 
 	if(Ata_p->DeviceType == STATAPI_SATA)
     hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif

    return FALSE;
}

/************************************************************************
Name: ata_packet_PioIn

Description: executes a command with data transfer from the device
             to the host via PIO.
             The device is already selected, no parameters check
             is performed. We totally trust on the caller

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/

BOOL ata_packet_PioIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 Status,Reason,LowCyl,HighCyl;
    U32 TimeOut,i;
    BOOL Error=FALSE;
    U32 WordCount,ByteCount;
    U32 *Data_p;

    Ata_p->HalHndl->DataDirection = ATA_DATA_IN;
#if  (ATAPI_USING_INTERRUPTS) 
 	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

   /* First we write the registers */
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

   if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,DRDY_BIT_MASK,DRDY_BIT_MASK))
   {
       Ata_p->LastExtendedErrorCode= 0x5B;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;

    }

   /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);
    WAIT400NS;

   /* Now Check the Status */
    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);

    /* Command packet transfer...*/
    /* Check for protocol failures, the device should
       have BSY=1 now, but a number of strange things can happen:
       1) the device may not set BSY=1 or DRQ=1 for a while
       2) the device may go directly to DRQ status,
       3) the device may reject the command (maybe is not really
          an  ATAPI device or has some error)
    */
    TimeOut= PACKET_TIMEOUT;

    while(TRUE)
    {
        if(Status & BSY_BIT_MASK)
        {
           break; /* BSY=1 that's OK! */
        }else
        {
           if( Status & DRQ_BIT_MASK )
           {
             break;  /* BSY=0 & DRQ=1 That's OK */
           }
           if( Status & ERR_BIT_MASK)
           {
             /* Error,that's not OK */
             break;
           }
        }
        if(TimeOut==0)
        {
            /* Error */
            break;
        }

        TimeOut--;
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);
    }

    /* Command packet transfer....                          */
    /* Poll Alternate Status for BSY=0                      */
    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
    {
       Ata_p->LastExtendedErrorCode= 0x51;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
    }

#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
	    /* Command packet transfer....                          */
	    /* Check for protocol failures.. clear interrupts       */
	    if (hal_ClearInterrupt(Ata_p->HalHndl) != FALSE)
	    {
	        STTBX_Print(("Note - HDDI interrupt and/or semaphore needed clearing\n"));
	    }
	}
#endif
    /* Command packet transfer....
       Read the primary status register and other ATAPI registers
    */
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
        HighCyl=hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

     /* Command packet transfer....
        check status must have BSY=0 DRQ-1
     */

     if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
     {
        Ata_p->LastExtendedErrorCode= 0x52;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
     }
     /* Transfer the packet*/
    {
     	U16	ui16Value;
        for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
        {
            ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
      	    hal_RegOutWord(Ata_p->HalHndl,ui16Value);
      	    i = i+2;
        }
    }

    WAIT400NS;
   /* --------------- Transfer Loop--------------- */
   *Cmd_p->BytesRW=0;
    Data_p =(U32*) Cmd_p->DataBuffer;

   while (Error==FALSE)
   {
#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
	     if(hal_AwaitInt(Ata_p->HalHndl,PACKET_INT_TIMEOUT))
	     {
	        Ata_p->LastExtendedErrorCode= 0x53;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        Error= TRUE;
	        break;
	     }
    }
    else
    {
	      WAIT400NS;
	     if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	     {
	        Ata_p->LastExtendedErrorCode= 0x54;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        Error= TRUE;
	        break;
	     }
	}
#else   
	      WAIT400NS;
	     if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	     {
	        Ata_p->LastExtendedErrorCode= 0x54;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        Error= TRUE;
	        break;
	     }
#endif /* ATAPI_USING_INTERRUPTS */


       Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
       Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
       LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
       HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

       if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) == 0)
       {
            /* end of transfer lets quit*/
           Ata_p->LastExtendedErrorCode= 0x00;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=FALSE;
           break;
       }

       if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) != DRQ_BIT_MASK)
       {
           Ata_p->LastExtendedErrorCode= 0x55;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       if( (Reason & (TAG_BIT_MASK | REL_BIT_MASK))
            || (Reason &  CD_BIT_MASK))
       {

           Ata_p->LastExtendedErrorCode= 0x5C;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       if( (Reason & IO_BIT_MASK )==0)
       {
           /* Wrong direction, we expect a IO = 1 */
           Ata_p->LastExtendedErrorCode= 0x5D;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       /* Data Transfer Loop....
          get the byte count and check for zero
       */
       ByteCount= (HighCyl<<8) | LowCyl;
       if(ByteCount<1)
       {
           Ata_p->LastExtendedErrorCode= 0x60;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }


       WordCount = ( ByteCount >> 1 ) + ( ByteCount & 0x0001 );
       /* Data Transfer ....                      */
       *Cmd_p->BytesRW+= (WordCount << 1);

       hal_RegInBlock(Ata_p->HalHndl,
       				  Data_p,
       				  ByteCount,
       				  Cmd_p->UseDMA,
       				  FALSE);

        /* Data is 32-bit pointer, ie two words per increment */
       Data_p+=(WordCount / 2);
       WAIT400NS;
   }
   /* End of transfer loop */

   if(Error == FALSE)
   {
       Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
       Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
       LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
       HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

       /* Final Status check */

       if( Status  & (BSY_BIT_MASK  | ERR_BIT_MASK))
       {
          Ata_p->LastExtendedErrorCode= 0x58;
          GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
          return TRUE;
       }
       Ata_p->LastExtendedErrorCode= 0x00;

   }

    /*read the output registers and store them in the pkt status structure*/
    GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);

#if  (ATAPI_USING_INTERRUPTS) 
 	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif

    return Error;
}

/************************************************************************
Name: ata_packet_PioOut

Description: executes a command with data transfer from the host
             to the device via PIO.
             The device is already selected, no parameters check
             is performed. We totally trust on the caller

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/
BOOL ata_packet_PioOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 Status,Reason,LowCyl,HighCyl;
    U32 TimeOut,i;
    BOOL Error=FALSE;
    U32 WordCount,ByteCount;
    U32 *Data_p;

    Ata_p->HalHndl->DataDirection = ATA_DATA_OUT;

#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

    /* First we write the registers */

   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

   if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,DRDY_BIT_MASK,DRDY_BIT_MASK))
   {
       Ata_p->LastExtendedErrorCode= 0x5B;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
   }
   /* Finally write the command */
   hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);
   WAIT400NS;

   /* Now Check the Status */
    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);

    /* Command packet transfer...*/
    /* Check for protocol failures, the device should
       have BSY=1 now, but a number of strange things can happen:
       1) the device may not set BSY=1 or DRQ=1 for a while
       2) the device may go directly to DRQ status,
       3) the device may reject the command (maybe is not really
          an  ATAPI device or has some error)
    */
    TimeOut= PACKET_TIMEOUT;

    while(TRUE)
    {
        if(Status & BSY_BIT_MASK)
        {
           break; /* BSY=1 that's OK! */
        }else
        {
           if( Status & DRQ_BIT_MASK )
           {
             break;  /* BSY=0 & DRQ=1 That's OK */
           }
           if( Status & ERR_BIT_MASK)
           {
             /* Error,that's not OK */
             break;
           }
        }
        if(TimeOut==0)
        {
            /* Error */
            break;
        }

        TimeOut--;
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);
    }

    /* Command packet transfer....                          */
    /* Poll Alternate Status for BSY=0                      */
    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
    {
       Ata_p->LastExtendedErrorCode= 0x51;
       GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
       return TRUE;
    }

    /* Command packet transfer....                          */
    /* Check for protocol failures.. clear interrupts       */
    if (hal_ClearInterrupt(Ata_p->HalHndl) != FALSE)
    {
        STTBX_Print(("Note - HDDI interrupt and/or semaphore needed clearing\n"));
    }

    /* Command packet transfer....
       Read the primary status register abd other ATAPI registers
    */
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
        HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

     /* Command packet transfer....
        check status must have BSY=0 DRQ-1
     */
     if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
     {
        Ata_p->LastExtendedErrorCode= 0x52;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
     }
     /* Transfer the packet*/

    {
        U16	ui16Value;
        for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
        {
            ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
            hal_RegOutWord(Ata_p->HalHndl,ui16Value);
            i = i+2;
        }
    }

      WAIT400NS;
   /* --------------- Transfer Loop--------------- */
   *Cmd_p->BytesRW=0;
    Data_p =(U32*) Cmd_p->DataBuffer;

   while (Error==FALSE)
   {
#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
	    if(hal_AwaitInt(Ata_p->HalHndl,PACKET_INT_TIMEOUT))
	     {
	          Ata_p->LastExtendedErrorCode= 0x53;
	          GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	          return TRUE;
	     }
    }
    else
    {
	     WAIT400NS;
	     if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	     {
	        Ata_p->LastExtendedErrorCode= 0x54;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        return TRUE;
	     }
	}
#else   
	     WAIT400NS;
	     if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	     {
	        Ata_p->LastExtendedErrorCode= 0x54;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        return TRUE;
	     }
#endif /* ATAPI_USING_INTERRUPTS */

       Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
       Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
       LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
       HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

       if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) == 0)
       {
            /* end of transfer lets quit*/
           Ata_p->LastExtendedErrorCode= 0x00;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=FALSE;
           break;
       }

       if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) != DRQ_BIT_MASK)
       {
           Ata_p->LastExtendedErrorCode= 0x55;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       if( (Reason & (TAG_BIT_MASK | REL_BIT_MASK))
            || (Reason &  CD_BIT_MASK))
       {

           Ata_p->LastExtendedErrorCode= 0x5C;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       if( (Reason & IO_BIT_MASK )!=0)
       {
           /* Wrong direction, we expect a IO = 1 */
           Ata_p->LastExtendedErrorCode= 0x5D;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       /* Data Transfer Loop....
          get the byte count and check for zero
       */
       ByteCount= (HighCyl<<8) | LowCyl;
       if(ByteCount<1)
       {
           Ata_p->LastExtendedErrorCode= 0x60;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }

       /* Data Transfer Loop....
          Check for buffer overrun
       */
       if((*Cmd_p->BytesRW)+ByteCount > Cmd_p->BufferSize )
       {
           Ata_p->LastExtendedErrorCode= 0x59;
           GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
           Error=TRUE;
           break;
       }
       WordCount = ( ByteCount >> 1 ) + ( ByteCount & 0x0001 );

       /* Data Transfer ....                      */
        hal_RegOutBlock(Ata_p->HalHndl,
        				Data_p,
        				ByteCount,
                        Cmd_p->UseDMA,
                        FALSE);
                        
        *Cmd_p->BytesRW+= (WordCount << 1);
        /* Data_p is 32-bit pointer, ie two words per increment */
        Data_p+= (WordCount >> 1);
        WAIT400NS;
   }

   if(Error == FALSE)
   {

       Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
       Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
       LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
       HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

       /* Final Status check */

       if( Status  & (BSY_BIT_MASK | DRQ_BIT_MASK | ERR_BIT_MASK))
       {
          Ata_p->LastExtendedErrorCode= 0x58;
          GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
          return TRUE;
       }
       Ata_p->LastExtendedErrorCode= 0x00;

   }
    /*read the output registers and store them in the pkt status structure*/
    GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);

#if  (ATAPI_USING_INTERRUPTS)
 	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif

    return Error;
}

/************************************************************************
Name: ata_packet_DmaIn

Description: executes a command with data transfer from the device
             to the host via DMA.
             The device is already selected, no parameters check
             is performed.

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/
BOOL ata_packet_DmaIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 Status,Reason,LowCyl,HighCyl;
    U32 i;
    BOOL Error=FALSE, CrcError = FALSE, HaveRead = FALSE;
    U32 FullSectorCount;
    U32 RemainByteCount = 0;
#if defined(ST_7100) || defined(ST_7109)        
    U32 SentByteCount = 0;
    U32 XBytes = 0;
#endif    
    U16 *Data_p;
    U8 FirstTime=1;
    U32 Bsize,Wsize;
    *Cmd_p->BytesRW = 0;
    

    Data_p =(U16*) Cmd_p->DataBuffer;

	if(Cmd_p->Pkt[0] == READ_CD_OPCODE)/*In case of Reading from CD*/
	{
	        Bsize = CD_SECTOR_BSIZE;
	        Wsize = CD_SECTOR_WSIZE;
	}
	else
	{
	        Bsize = DVD_SECTOR_BSIZE;
	        Wsize = DVD_SECTOR_WSIZE;
	}
    FullSectorCount = ((Cmd_p->Pkt[6]<< 24) | (Cmd_p->Pkt[7]<< 16) | (Cmd_p->Pkt[8]<< 8) | (Cmd_p->Pkt[9]));
    Ata_p->HalHndl->DataDirection = ATA_DATA_IN;

    if(Ata_p->DeviceType == STATAPI_SATA)
    {
#if defined(ST_7100) || defined(ST_7109)    	
		 RemainByteCount = FullSectorCount * Bsize;
   		 /*Bug 51676*/
    	 while((Error == FALSE) && (RemainByteCount > 0))
    	 {
	        XBytes =(RemainByteCount < MAX_DMA_BLK_SIZE)? RemainByteCount:MAX_DMA_BLK_SIZE;
	        FullSectorCount = XBytes/(Bsize);
	        Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

            /* First we write the registers */
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

            while ((Error == FALSE) && (FullSectorCount > 0))
            {
                 while(!(STSYS_ReadRegDev32LE((U32)Ata_p->HalHndl->BaseAddress + SATA_AHB2STBUS_PC_STATUS) & 0x1))
                 /* wait for AHB to go idle*/
                 {
                 }
#if defined(ATAPI_DEBUG)
    atapi_intcount = 0;
#endif
                 if(FirstTime == 1)
                 {
                        /* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK | DRQ_BIT_MASK, 0))
                        {
                            STTBX_Print(("BSY bit didn't clear 0x35\n"));
                            Ata_p->LastExtendedErrorCode= 0x35;
                            Error= TRUE;
                            break;
                        }
                        /* Finally write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        /* Now check the status*/
     	                WAIT400NS;
                        Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        /* Command packet transfer*/
                        /* Poll Alternate Status for BSY=0 */
                        if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
                        {
                            Ata_p->LastExtendedErrorCode= 0x51;
                            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
                            return TRUE;
                        }

                        /* Command packet transfer....
                           Read the primary status register and other ATAPI registers
                        */
                        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
                        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
                        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
                        HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

                        /* Command packet transfer....
                           check status must have BSY=0 DRQ-1
                        */
                        if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
                        {
                            Ata_p->LastExtendedErrorCode= 0x52;
                            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
                            return TRUE;
                        }
                        /* Transfer the packet in PIO mode */
                        {
                            U16	ui16Value;
                            for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
                            {
                         		ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
                          	    hal_RegOutWord(Ata_p->HalHndl,ui16Value);
                          	    i = i+2;
                            }
                         }
                         Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
            	         FirstTime=0;
                         WAIT400NS;

                 }/*if*/

                 HaveRead = TRUE;
                 /* Read one DRQ sector*/

                 /* Data Transfer .... */
                 Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                          Cmd_p->DevCtrl, Cmd_p->DevHead,
                                          Data_p, FullSectorCount * Wsize,
                                          Cmd_p->BufferSize, Cmd_p->BytesRW,
                                          TRUE, &CrcError);


                 if (Error == FALSE)
                 {
	                    while(!(STSYS_ReadRegDev32LE((U32)Ata_p->HalHndl->BaseAddress +
                                            SATA_AHB2STBUS_PC_STATUS) & 0x1))
	                    /* wait for AHB to go idle*/
	                	{
	                	}
	            	    RemainByteCount -= XBytes;
	            	    SentByteCount += XBytes;
	                    Data_p += Wsize * FullSectorCount;
	                    FullSectorCount = 0;
                 }

                 /* Do any tidying up required */
                 hal_AfterDma(Ata_p->HalHndl);
                    
                 WAIT400NS;
                 if (CrcError == TRUE)
                 {
                    Ata_p->LastExtendedErrorCode = EXTERROR_CRCERROR;
                    Error = TRUE;
                    break;
                 }

				 if(Ata_p->DeviceType != STATAPI_EMI_PIO4)
				 {
	                 if (Ata_p->HalHndl->DmaAborted == TRUE)
	                 {
	                        Ata_p->LastExtendedErrorCode = EXTERROR_DMAABORT;
	                        Error = TRUE;
	                        break;
	                 }
	             }
                 if(RemainByteCount == 0)
                 {
                        /* Check all these bits return to 0 */
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS,
                                       DRQ_BIT_MASK | BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK,
                                       0))
                        {
                            Ata_p->LastExtendedErrorCode = 0x41;
                            Error = TRUE;
                            break;
                        }

                        /* Check the status*/
                        Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((Status & (DRDY_BIT_MASK)) == 0)
                        {
                            Ata_p->LastExtendedErrorCode= 0x32;
                            Error = TRUE;
                            break;
                        }

                        /* SecCount == 0 when we haven't transferred is valid, 256 sect */
                        if ((FullSectorCount < 1) && (HaveRead == TRUE))
                        {
                           /* Since the drive has transferred all of the requested sectors
                           without error, the drive should not have BUSY, DEVICE FAULT,
                           DATA REQUEST or ERROR active now */

                            Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
                            if (Status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | DRQ_BIT_MASK))
                            {
                                Ata_p->LastExtendedErrorCode = 0x33;
                                Error= TRUE;
                                break;
                            }

                            /* All sectors have been read without error, exit */
                            Ata_p->LastExtendedErrorCode= 0x00;
                            break;
                        }

                        /* Do any tidying up that might be required */
                    	hal_AfterDma(Ata_p->HalHndl);
                 }/*RemainByteCount=0*/
             }/*End of read loop*/
         }/*End of Outer While loop*/
#endif/*7100*/         
	}/*If Sata*/
	
	else
	{
		 /* First we write the registers */
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
            hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

            while ((Error == FALSE) && (FullSectorCount > 0))
            {
#if defined(ATAPI_DEBUG)
    atapi_intcount = 0;
#endif
                 if(FirstTime == 1)
                 {
                        /* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK | DRQ_BIT_MASK, 0))
                        {
                            STTBX_Print(("BSY bit didn't clear 0x35\n"));
                            Ata_p->LastExtendedErrorCode= 0x35;
                            Error= TRUE;
                            break;
                        }


                        /* Finally write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        /* Now check the status*/

    	                 WAIT400NS;
                         Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                         /* Command packet transfer*/
                        /* Poll Alternate Status for BSY=0 */
                        if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
                        {
                            Ata_p->LastExtendedErrorCode= 0x51;
                            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
                            return TRUE;
                        }

                        /* Command packet transfer....
                           Read the primary status register and other ATAPI registers
                        */
                        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
                        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
                        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
                        HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

                        /* Command packet transfer....
                           check status must have BSY=0 DRQ-1
                        */

                        if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
                        {
                            Ata_p->LastExtendedErrorCode= 0x52;
                            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
                            return TRUE;
                        }
                          /* Transfer the packet in PIO mode */
                        {
                            U16	ui16Value;
                            for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
                            {
                         		ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
                          	    hal_RegOutWord(Ata_p->HalHndl,ui16Value);
                          	    i = i+2;
                            }
                         }
                        Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
            	        FirstTime=0;
                        WAIT400NS;

                 }/*if*/
                 /* Checking if Busy =0 and DRQ=1 in order to
                  enter into the transfer phase*/
                 if ((Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) != 0)
                 {
                        HaveRead = TRUE;
                        /* Read one DRQ sector*/
                        /* Data Transfer .... */
                        Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                                 Cmd_p->DevCtrl, Cmd_p->DevHead,
                                                 Data_p, FullSectorCount * Wsize,
                                                 Cmd_p->BufferSize, Cmd_p->BytesRW,
                                                 TRUE, &CrcError);


                        if (Error == FALSE)
                        {

                	        Data_p += FullSectorCount * Wsize ;
                            /* We just read all the data */
                            FullSectorCount = 0;
           				}
                        /* Do any tidying up required */
                        hal_AfterDma(Ata_p->HalHndl);
                 }/*if*/
                 WAIT400NS;
                 if (CrcError == TRUE)
                 {
                        Ata_p->LastExtendedErrorCode = EXTERROR_CRCERROR;
                        Error = TRUE;
                        break;
                 }
		 		 if(Ata_p->DeviceType != STATAPI_EMI_PIO4)
		 		 {
	                 if (Ata_p->HalHndl->DmaAborted == TRUE)
	                 {
	                        Ata_p->LastExtendedErrorCode = EXTERROR_DMAABORT;
	                        Error = TRUE;
	                        break;
	                 }
	             }
                 if(RemainByteCount == 0)
                 {
                        /* Check all these bits return to 0 */
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS,
                                       DRQ_BIT_MASK | BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK,
                                       0))
                        {
                            Ata_p->LastExtendedErrorCode = 0x41;
                            Error = TRUE;
                            break;
                        }
                        /* Check the status*/
                        Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((Status & (DRDY_BIT_MASK)) == 0)
                        {
                            Ata_p->LastExtendedErrorCode= 0x32;
                            Error = TRUE;
                            break;
                        }

                        /* SecCount == 0 when we haven't transferred is valid, 256 sect */
                        if ((FullSectorCount < 1) && (HaveRead == TRUE))
                        {
                            /* Since the drive has transferred all of the requested sectors
                            without error, the drive should not have BUSY, DEVICE FAULT,
                            DATA REQUEST or ERROR active now */

                            Status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
                            if (Status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | DRQ_BIT_MASK))
                            {
                                Ata_p->LastExtendedErrorCode = 0x33;
                                Error= TRUE;
                                break;
                            }

                            /* All sectors have been read without error, exit */
                            Ata_p->LastExtendedErrorCode= 0x00;
                            break;
                        }
                        /* Do any tidying up that might be required */
                        hal_AfterDma(Ata_p->HalHndl);
                 }/*RemainByteCount=0*/
            }/*End of read loop*/
    }/*else*/
        
    if(Error == FALSE)
    {

        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
        /* Final Status check */
        if( Status  & (BSY_BIT_MASK | DRQ_BIT_MASK | ERR_BIT_MASK))
        {
            Ata_p->LastExtendedErrorCode= 0x58;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            return TRUE;
        }
        Ata_p->LastExtendedErrorCode= 0x00;
    }

    /*read the output registers and store them in the pkt status structure*/
    GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
    return Error;
}

/************************************************************************
Name: ata_packet_DmaOut

Description: executes a command with data transfer from the host
             to the device via DMA.
             The device is already selected, no parameters check
             is performed.

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/
BOOL ata_packet_DmaOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 Status,Reason,LowCyl,HighCyl;
    U32 TimeOut,i;
    BOOL Error=FALSE, CrcError = FALSE;
    U32 WordCount,ByteCount;
    U16 *Data_p;
    U8 Retries = 4;

    Ata_p->HalHndl->DataDirection = ATA_DATA_OUT;


    /* First we write the registers */
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_FEATURE, Cmd_p->Feature);
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECCOUNT, Cmd_p->SecCount);
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_SECNUM, Cmd_p->SecNum);
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLLOW, Cmd_p->CylLow);
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CYLHIGH, Cmd_p->CylHigh);
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);

    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,DRDY_BIT_MASK,DRDY_BIT_MASK))
    {
        Ata_p->LastExtendedErrorCode= 0x5B;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
    }

    /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);

    WAIT400NS;

    /* Now Check the Status */
    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);

    /* Command packet transfer...*/
    /* Check for protocol failures, the device should
       have BSY=1 now, but a number of strange things can happen:
       1) the device may not set BSY=1 or DRQ=1 for a while
       2) the device may go directly to DRQ status,
       3) the device may reject the command (maybe is not really
          an  ATAPI device or has some error)
    */
    TimeOut= PACKET_TIMEOUT;

    while(TRUE)
    {
        if(Status & BSY_BIT_MASK)
        {
            break; /* BSY=1 that's OK! */
        }
        else
        {
            if( Status & DRQ_BIT_MASK )
            {
                break;  /* BSY=0 & DRQ=1 That's OK */
            }
            if( Status & ERR_BIT_MASK)
            {
                /* Error,that's not OK */
                break;
            }
        }
        if(TimeOut==0)
        {
            /* Error */
            break;
        }

        TimeOut--;
        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_ALTSTAT);
    }

    /* Command packet transfer....                          */
    /* Poll Alternate Status for BSY=0                      */
    if(WaitForBitPkt(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
    {
        Ata_p->LastExtendedErrorCode= 0x51;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
    }

    /* Command packet transfer....                          */
    /* Check for protocol failures.. clear interrupts       */
    if (hal_ClearInterrupt(Ata_p->HalHndl) != FALSE)
    {
        STTBX_Print(("Note - HDDI interrupt and/or semaphore needed clearing\n"));
    }

    /* Command packet transfer....
       Read the primary status register abd other ATAPI registers
    */
    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
    Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
    LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
    HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

     /* Command packet transfer....
        check status must have BSY=0 DRQ-1
     */
    if((Status & (BSY_BIT_MASK | DRQ_BIT_MASK))!= DRQ_BIT_MASK)
    {
        Ata_p->LastExtendedErrorCode= 0x52;
        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
        return TRUE;
    }

    /* Transfer the packet - PIO mode */
    {
	    U16	ui16Value;
        for(i=0;i<Ata_p->Handles[Ata_p->DeviceSelected].PktSize*2;)
        {
  		    ui16Value	= (((U16)Cmd_p->Pkt[i+1]<<8)|Cmd_p->Pkt[i]);
    	   	hal_RegOutWord(Ata_p->HalHndl,ui16Value);
   	        i = i+2;
        }
    }
    WAIT400NS;
    /* --------------- Transfer Loop--------------- */
    *Cmd_p->BytesRW=0;
    Data_p =(U16*) Cmd_p->DataBuffer;

    while ((Error==FALSE) && (Retries > 0))
    {
        if(WaitForBitPkt(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
        {
            Ata_p->LastExtendedErrorCode= 0x54;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            return TRUE;
        }

        Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
        Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
        LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
        HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);

        if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) == 0)
        {
            /* end of transfer lets quit*/
            Ata_p->LastExtendedErrorCode= 0x00;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=FALSE;
            break;
        }

        if( (Status & (BSY_BIT_MASK | DRQ_BIT_MASK)) != DRQ_BIT_MASK)
        {
            Ata_p->LastExtendedErrorCode= 0x55;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=TRUE;
            break;
        }
        if( (Reason & (TAG_BIT_MASK | REL_BIT_MASK))
            || (Reason &  CD_BIT_MASK))
        {

            Ata_p->LastExtendedErrorCode= 0x5C;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=TRUE;
            break;
        }
        if( (Reason & IO_BIT_MASK )==0)
        {
            /* Wrong direction, we expect a IO = 1 */
            Ata_p->LastExtendedErrorCode= 0x5D;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=TRUE;
            break;
        }
        /* Data Transfer Loop....
           get the byte count and check for zero
        */
        /* ByteCount= (HighCyl<<8) | LowCyl;*/
        ByteCount= Cmd_p->BufferSize;
        if(ByteCount<1)
        {
            Ata_p->LastExtendedErrorCode= 0x60;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=TRUE;
            break;
        }

        /* Data Transfer Loop....
           Check for buffer overrun
        */
        if((*Cmd_p->BytesRW)+ByteCount > Cmd_p->BufferSize )
        {
            Ata_p->LastExtendedErrorCode= 0x59;
            GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
            Error=TRUE;
            break;
        }
        WordCount = ( ByteCount >> 1 ) + ( ByteCount & 0x0001 );

        /* Data Transfer .... */
        Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                 Cmd_p->DevCtrl, Cmd_p->DevHead,
                                 Data_p, WordCount,
                                 Cmd_p->BufferSize, Cmd_p->BytesRW,
                                 TRUE, &CrcError);

        if (Error == FALSE)
        {
            *Cmd_p->BytesRW+= (WordCount << 1);
            Data_p+=WordCount;
        }

        /* Retry on CRC error, not abort */
        if (CrcError == TRUE)
        {
            Error = FALSE;
            --Retries;
        }
     }
    WAIT400NS;
	if(Error == FALSE)
	{
	    Status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
	    Reason= hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
	    LowCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
	    HighCyl= hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);
	
	    /* Final Status check */
	
	    if( Status  & (BSY_BIT_MASK | DRQ_BIT_MASK | ERR_BIT_MASK))
	    {
	        Ata_p->LastExtendedErrorCode= 0x58;
	        GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	        return TRUE;
	    }
	    Ata_p->LastExtendedErrorCode= 0x00;
	
	}

	/*read the output registers and store them in the pkt status structure*/
	GetPktStatus(Ata_p,Cmd_p->Stat.PktStat);
	return Error;
}

/************************************************************************
Name: GetPktStatus

Description: Retrieves the command block registers and store them in the
             structure previosly allocated

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command status structure


************************************************************************/

static void GetPktStatus(ata_ControlBlock_t *Ata_p,STATAPI_PktStatus_t *Stat_p)
{

    Stat_p->Status = hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);

    Stat_p->InterruptReason = hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);

    Stat_p->Error = hal_RegInByte(Ata_p->HalHndl,ATA_REG_ERROR);

}
/*end of cmmd_ata.c --------------------------------------------------*/
