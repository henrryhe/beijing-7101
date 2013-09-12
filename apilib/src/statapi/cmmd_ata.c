/************************************************************************

Source file name : cmmd_ata.c

Description: Implementation of the ATA Command Interface intermediate module
            following the API Design Document v0.9.0 of the ATAPI driver.

COPYRIGHT (C) STMicroelectronics  2000

31-08-00  Added an extra 400 ns delay at the end of the PIO in protocol
          to allow slow drives as CD-ROM's to update their status
          registers. (MV)

************************************************************************/

/*Includes-------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "stlite.h"
#include "statapi.h"
#include "stsys.h"
#include "hal_atapi.h"
#include "ata.h"
#include "stcommon.h"

#include "sttbx.h"

/*Private Types--------------------------------------------------------*/

/*Private Constants----------------------------------------------------*/

/*Private Variables----------------------------------------------------*/

#if defined(BMDMA_ENABLE)
extern U32 RegsMasks[STATAPI_NUM_REG_MASKS];
#endif

#if defined(ATAPI_DEBUG)
extern U32 atapi_intcount;
extern U32 atapi_inttrace[15];

/* Applications wishing to use this should declare it 'extern'. When
 * true, it will enable output; if false, debug output is disabled.
 */
BOOL ATAPI_Trace = FALSE;
#endif

/* Kind of messy, but since we need to get status differently for
 * an extended command, and we can't expect the user to tell us what type
 * a command was...
 */
static BOOL LastCommandExtended = FALSE;
/*Private Macros-------------------------------------------------------*/
#ifdef ENABLE_ATA_TIME_TRACE
#define ENABLE_TRACE
#include "trace.h"

#define ATA_TIME_TRACE(__String__)  \
            {   \
                U32 Time;   \
                __asm{ ldc 0; ldclock; st Time; }  \
                TraceBuffer((#__String__ "\t%10u\n", Time));   \
            }
#else

#define ATA_TIME_TRACE(__String__)      /* nop */
#endif

/*Private functions prototypes-----------------------------------------*/

static void GetCmdStatus(ata_ControlBlock_t *Ata_p,
                         STATAPI_CmdStatus_t *Stat_p);
static void ata_cmd_WriteRegs(ata_ControlBlock_t *Ata_p,
                                       ata_Cmd_t *Cmd_p);

/*Functions------------------------------------------------------------*/

/************************************************************************
Name: ata_cmd_WriteRegs

Description: Programs registers for sector count/number, cylinder, etc.

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return: none
************************************************************************/
static void ata_cmd_WriteRegs(ata_ControlBlock_t *Ata_p,
                                       ata_Cmd_t *Cmd_p)
{
    /* Feature is reserved with extended commands, so don't write. */
    if (((Cmd_p->CmdType & ATA_EXT_BIT) == 0) && (Cmd_p->Feature != 0xFF))
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_FEATURE, Cmd_p->Feature);
#if defined(ATAPI_DEBUG) && defined(STTBX_PRINT)
    if (ATAPI_Trace == TRUE)
    {
        STTBX_Print(("Feature: 0x%02x\n", Cmd_p->Feature));
        STTBX_Print(("SC: 0x%x\nSN: 0x%02x\nCL: 0x%02x\nCH: 0x%02x\n",
                    Cmd_p->SecCount, Cmd_p->SecNum,
                    Cmd_p->CylLow, Cmd_p->CylHigh));
        STTBX_Print(("DH: %i\n", Cmd_p->DevHead));
        STTBX_Print(("CC: 0x%02x\n", Cmd_p->CommandCode));
    }
#endif
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_SECCOUNT, Cmd_p->SecCount);
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_SECNUM, Cmd_p->SecNum);
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CYLLOW, Cmd_p->CylLow);
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CYLHIGH, Cmd_p->CylHigh);

    /* "first" write of devhead is also reserved */
    if ((Cmd_p->CmdType & ATA_EXT_BIT) == 0)
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_DEVHEAD, Cmd_p->DevHead);

    /* If it's an extended command, now write the other bits and head. */
    if ((Cmd_p->CmdType & ATA_EXT_BIT) != 0)
    {
        LastCommandExtended = TRUE;

        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_SECCOUNT, Cmd_p->SecCount2);
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_SECNUM, Cmd_p->SecNum2);
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CYLLOW, Cmd_p->CylLow2);
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CYLHIGH, Cmd_p->CylHigh2);
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_DEVHEAD, Cmd_p->DevHead);
#if defined(ATAPI_DEBUG) && defined(STTBX_PRINT)
        if (ATAPI_Trace == TRUE)
        {
            STTBX_Print(("SC2: 0x%x\nSN2: 0x%02x\nCL2: 0x%02x\nCH2: 0x%02x\n",
                        Cmd_p->SecCount2, Cmd_p->SecNum2,
                        Cmd_p->CylLow2, Cmd_p->CylHigh2));
        }
#endif
    }
    else
        LastCommandExtended = FALSE;
}

/************************************************************************
Name: ata_cmd_NoData

Description: executes a non data command. The device is already selected
             no parameters check is performed. We totally trust on the caller
Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error

************************************************************************/
BOOL ata_cmd_NoData(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    DU8 status;

    /* First we write the registers */
    ata_cmd_WriteRegs(Ata_p, Cmd_p);

    Ata_p->HalHndl->DataDirection = ATA_DATA_NONE;
    
#if !(ATAPI_USING_INTERRUPTS)
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif
#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

    if(WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, DRDY_BIT_MASK, DRDY_BIT_MASK))
    {
        STTBX_Print(("Device not ready 0x26\n"));
        Ata_p->LastExtendedErrorCode= 0x26;
        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
        return TRUE;
    }
    /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);
    WAIT400NS;
    
#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
   	    if(hal_AwaitInt(Ata_p->HalHndl,INT_TIMEOUT))
    	{
        	STTBX_Print(("Not interrupt 0x22\n"));
        	Ata_p->LastExtendedErrorCode= 0x22;
        	GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
        	return TRUE;
    	}
    }
    else
    {
   	     WAIT400NS;
	     if(WaitForBit(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	     {
	        STTBX_Print(("BSY not set, 0x23\n"));
	        Ata_p->LastExtendedErrorCode= 0x23;
	        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
	        return TRUE;
	     }
	}
#else   
	     WAIT400NS;
	    if(WaitForBit(Ata_p->HalHndl,ATA_REG_ALTSTAT,BSY_BIT_MASK,0))
	    {
	        STTBX_Print(("BSY not set, 0x23\n"));
	        Ata_p->LastExtendedErrorCode= 0x23;
	        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
	        return TRUE;
	    }
#endif /* ATAPI_USING_INTERRUPTS */

    /* Now Check the Status */
    status= hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);

    if( status & (BSY_BIT_MASK | DRQ_BIT_MASK| DF_BIT_MASK | ERR_BIT_MASK))
    {
        STTBX_Print(("Unwanted bit 0x21\n"));
        Ata_p->LastExtendedErrorCode= 0x21;
        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
        return TRUE;
    }

    GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
    Ata_p->LastExtendedErrorCode= 0x00;

#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
      hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif
    return FALSE;
}

/************************************************************************
Name: ata_cmd_PioIn

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
BOOL ata_cmd_PioIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    volatile U8 status;
    BOOL Error=FALSE;
    U32  *Data_p;
    U32  FullSectorCount;
    U32  IncrementStep;
    U32  BurstSectorCount;

    ATA_TIME_TRACE(PioIn Start);

    *Cmd_p->BytesRW = 0;
    Data_p = (U32 *)Cmd_p->DataBuffer;
    /* 32-bit access, so we do two words at a time. */
    IncrementStep = SECTOR_WSIZE >> 1;

    /* First we write the registers */
    ata_cmd_WriteRegs(Ata_p, Cmd_p);

    Ata_p->HalHndl->DataDirection = ATA_DATA_IN;
#if !(ATAPI_USING_INTERRUPTS)
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif    
#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
      	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

    /*No need to check DRDY in case of Serial ATAPI.
      This is a case with identify packet*/
    if(Ata_p->DeviceType != STATAPI_SATA)
    {
	    if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, DRDY_BIT_MASK, DRDY_BIT_MASK))
	    {
	        STTBX_Print(("Device not ready 0x36\n"));
	        Ata_p->LastExtendedErrorCode = 0x36;
	        GetCmdStatus(Ata_p, Cmd_p->Stat.CmdStat);
	        return TRUE;
	    }
    }

    /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);

    WAIT400NS;
    /* Transfering Data: Read Loop*/
    if ((Cmd_p->CmdType & ATA_EXT_BIT) != 0)
    {
        FullSectorCount = (Cmd_p->SecCount << 8) | Cmd_p->SecCount2;
        /* Take account of 65536-sector reads */
        if (FullSectorCount == 0)
            FullSectorCount = 65536;
    }
    else
    {
        FullSectorCount = Cmd_p->SecCount;
        if (FullSectorCount == 0)
            FullSectorCount = 256;
    }
        BurstSectorCount = Cmd_p->MultiCnt;
    /*Bug 51676*/        
    while((Error == FALSE) && (FullSectorCount > 0))
    {
        /* If we have a residue from transferring multiple sectors, deal
         * with it.
         */
        if (FullSectorCount < Cmd_p->MultiCnt)
            BurstSectorCount = FullSectorCount;

#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
       if(hal_AwaitInt(Ata_p->HalHndl, INT_TIMEOUT))
       {
            STTBX_Print(("Int timed out 0x34\n"));
            GetCmdStatus(Ata_p, Cmd_p->Stat.CmdStat);
       	    Ata_p->LastExtendedErrorCode= 0x34;
            Error= TRUE;
            break;
       }
    }
    else
    {
   	    WAIT400NS;
        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY didn't clear\n"));
            Ata_p->LastExtendedErrorCode= 0x45;
            Error= TRUE;
            break;
        }
	}
      
#if defined(ATAPI_DEBUG)
        atapi_intcount = 0;
#endif	
#else   
        WAIT400NS;
        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY didn't clear\n"));
            Ata_p->LastExtendedErrorCode= 0x45;
            Error= TRUE;
            break;
        }
#endif /* ATAPI_USING_INTERRUPTS */

        /* check status also, to be sure */
        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY bit didn't clear 0x35\n"));
            Ata_p->LastExtendedErrorCode= 0x35;
            Error= TRUE;
            break;
        }

        /* Now check the status*/
        status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

        /* Checking if Busy =0 and DRQ=1 in order to
           enter into the transfer phase                    */
        if ((status & (BSY_BIT_MASK | DRQ_BIT_MASK)) == DRQ_BIT_MASK)
        {

            /* Read one DRQ sector*/
#ifdef BMDMA_ENABLE
            ATA_BMDMA((U8*)((U32)Ata_p->HalHndl->BaseAddress | (RegsMasks[ATA_REG_DATA])),
                      (U8*)Data_p, BurstSectorCount * SECTOR_BSIZE);
#else
            ATA_TIME_TRACE(RegIn Start);


            hal_RegInBlock(Ata_p->HalHndl,
                           Data_p,
                           SECTOR_BSIZE * BurstSectorCount,
                           Cmd_p->UseDMA,
                          (Cmd_p->Feature == 0xFF) ? TRUE : FALSE );
            ATA_TIME_TRACE(RegIn End);
#endif
            Data_p += IncrementStep * BurstSectorCount;
            *Cmd_p->BytesRW += SECTOR_BSIZE * BurstSectorCount;

            FullSectorCount -= BurstSectorCount;
        }
        else
        {
            /* BSY 0, DRQ 0 => error */
            status=hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

            if((status & (ERR_BIT_MASK | DF_BIT_MASK)) != 0)
            {
                STTBX_Print(("Unwanted bits; 0x%02x; 0x31\n", status));
                STTBX_Print(("Feature: 0x%02x\n", Cmd_p->Feature));
                STTBX_Print(("SC: 0x%x\nSN: 0x%02x\nCL: 0x%02x\nCH: 0x%02x\n",
                            Cmd_p->SecCount, Cmd_p->SecNum,
                            Cmd_p->CylLow, Cmd_p->CylHigh));
                STTBX_Print(("DH: %i\n", Cmd_p->DevHead));
                STTBX_Print(("CC: 0x%02x\n", Cmd_p->CommandCode));
                Ata_p->LastExtendedErrorCode= 0x31;
                Error= TRUE;
                break;
            }
        }
    }
    /*read the output registers and store them in the cmd status structure*/
    GetCmdStatus(Ata_p, Cmd_p->Stat.CmdStat);

    ATA_TIME_TRACE(PioIn End);

#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif
    return Error;
}


/************************************************************************
Name: ata_cmd_PioOut

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
BOOL ata_cmd_PioOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    volatile U8 status;
    BOOL Error=FALSE;

    U32 *Data_p;
    U32 IncrementStep;
    U32 FullSectorCount, BurstSectorCount;

    ATA_TIME_TRACE(PioOut Start);

    *Cmd_p->BytesRW =0;
    Data_p= (U32*) Cmd_p->DataBuffer;
    /* 32-bit buffer, so we're doing two words at once. */
    IncrementStep = SECTOR_WSIZE >> 1;
    /* First we write the registers */
    ata_cmd_WriteRegs(Ata_p, Cmd_p);
 
    Ata_p->HalHndl->DataDirection = ATA_DATA_OUT;

/*In polling mode disable device interrupts*/
#if !(ATAPI_USING_INTERRUPTS)
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif    
  
#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
     	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
#endif

    if(WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, DRDY_BIT_MASK, DRDY_BIT_MASK))
    {
        STTBX_Print(("Device not ready 0x46\n"));
        Ata_p->LastExtendedErrorCode= 0x46;
        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
        return TRUE;
    }

    /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);

    WAIT400NS;

    if(WaitForBit(Ata_p->HalHndl,ATA_REG_STATUS,BSY_BIT_MASK,0))
    {
        STTBX_Print(("Device busy 0x45\n"));
        Ata_p->LastExtendedErrorCode= 0x45;
        return TRUE;
    }

    /* Transfering Data: Write Loop*/
    if ((Cmd_p->CmdType & ATA_EXT_BIT) != 0)
    {
        FullSectorCount = (Cmd_p->SecCount << 8) | Cmd_p->SecCount2;
        /* Take account of 65536-sector reads */
        if (FullSectorCount == 0)
            FullSectorCount = 65536;
    }
    else
    {
        FullSectorCount = Cmd_p->SecCount;
        if (FullSectorCount == 0)
            FullSectorCount = 256;
    }

    BurstSectorCount = Cmd_p->MultiCnt;
    while((Error == FALSE) && (FullSectorCount > 0))
    {
        if (FullSectorCount < Cmd_p->MultiCnt)
            BurstSectorCount = FullSectorCount;

#if defined(ATAPI_DEBUG)
        atapi_intcount = 0;
#endif
        /* Now check the status*/
        status=hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

        /* Checking if Busy =0 and DRQ=1 in order to
           enter into the transfer phase                    */
        if((status & ( BSY_BIT_MASK | DRQ_BIT_MASK))== DRQ_BIT_MASK)
        {

            /* Write one DRQ sector*/

#ifdef BMDMA_ENABLE
            ATA_BMDMA((U8*)Data_p,
                      (U8*)((U32)Ata_p->HalHndl->BaseAddress | (RegsMasks[ATA_REG_DATA])),
                      BurstSectorCount * SECTOR_BSIZE);
#else
            ATA_TIME_TRACE(RegOut Start);
            hal_RegOutBlock(Ata_p->HalHndl,
                            Data_p,
                            SECTOR_BSIZE * BurstSectorCount,
                            Cmd_p->UseDMA,
                            (Cmd_p->Feature == 0xFF) ? TRUE : FALSE );
                            
            ATA_TIME_TRACE(RegOut End);
#endif
            Data_p += IncrementStep * BurstSectorCount;
            *Cmd_p->BytesRW += SECTOR_BSIZE * BurstSectorCount;

            FullSectorCount -= BurstSectorCount;
        }
        else
        {
            /* BSY 0, DRQ 0 => error */
            status=hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

            if((status & (ERR_BIT_MASK | DF_BIT_MASK)) != 0)
            {
                STTBX_Print(("Unwanted bits; 0x%02x; 0x31\n", status));
                STTBX_Print(("Feature: 0x%02x\n", Cmd_p->Feature));
                STTBX_Print(("SC: 0x%x\nSN: 0x%02x\nCL: 0x%02x\nCH: 0x%02x\n",
                            Cmd_p->SecCount, Cmd_p->SecNum,
                            Cmd_p->CylLow, Cmd_p->CylHigh));
                STTBX_Print(("DH: %i\n", Cmd_p->DevHead));
                STTBX_Print(("CC: 0x%02x\n", Cmd_p->CommandCode));
                Ata_p->LastExtendedErrorCode= 0x31;
                Error= TRUE;
                break;
            }
        }
#if  (ATAPI_USING_INTERRUPTS) 
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
       if(hal_AwaitInt(Ata_p->HalHndl, INT_TIMEOUT))
        {
            status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
            STTBX_Print(("int timeout; status: 0x%02x; 0x44\n", status));
            Ata_p->LastExtendedErrorCode = 0x44;
            Error = TRUE;
            break;
        }
    }
    else
    {
          WAIT400NS;
        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY didn't clear\n"));
            Ata_p->LastExtendedErrorCode= 0x45;
            Error= TRUE;
            break;
        } 	    
	}
#else   
        WAIT400NS;
        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY didn't clear\n"));
            Ata_p->LastExtendedErrorCode= 0x45;
            Error= TRUE;
            break;
        }
#endif /* ATAPI_USING_INTERRUPTS */


        /* check status also, to be sure */
        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
        {
            STTBX_Print(("BSY bit didn't clear 0x35\n"));
            Ata_p->LastExtendedErrorCode= 0x35;
            Error= TRUE;
            break;
        }
    }
    if (Error != TRUE)
    {
        status=hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
        /*  So was there any error?  */
        if ((status & (ERR_BIT_MASK | DF_BIT_MASK)) != 0)
        {
            STTBX_Print(("DF or ERR; 0x%02x; 0x41\n", status));
            Ata_p->LastExtendedErrorCode = 0x41;


        }
        else if ((status & DRQ_BIT_MASK) != 0)
        {
            STTBX_Print(("DRQ bit not empty; 0x%02x; 0x42\n", status));
            STTBX_Print(("Sector count: %d\n", FullSectorCount));
            Ata_p->LastExtendedErrorCode = 0x42;
        }
    }

    /*read the output registers and store them in the cmd status structure*/
    GetCmdStatus(Ata_p, Cmd_p->Stat.CmdStat);

    ATA_TIME_TRACE(PioOut End);

#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif
    return Error;
}


/************************************************************************
Name: ata_cmd_DmaIn

Description:
    Executes a command with data transfer from the host to the device via DMA.
    The device and mode are already selected, no parameter checks are
    performed. Buffers are assumed to be 16-bit aligned (for DMA).

Parameters:
    Ata_p       Pointer to the ATA control block
    Cmd_p       Pointer to a command structure

Return:
    TRUE        Extended error is set
    FALSE       No error
************************************************************************/
BOOL ata_cmd_DmaIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    volatile U8 status;
    BOOL Error=FALSE, CrcError = FALSE;
    BOOL HaveRead = FALSE;
    U16* Data_p;
    U32 FullSectorCount;
    U32 RemainByteCount=0;
    U8 FirstTime = 1;
#if defined(ST_7100) || defined(ST_7109)        
	U32 SentByteCount=0;
	U32 XBytes=0;
#endif	
    *Cmd_p->BytesRW=0;
   
    /* Set up sector count */
    /* Note: do we fully support the DMA_EXT commands*/
    if ((Cmd_p->CmdType & ATA_EXT_BIT) != 0)
        FullSectorCount = (Cmd_p->SecCount << 8) | Cmd_p->SecCount2;
    else
        FullSectorCount = Cmd_p->SecCount;

    /* Take account of 256-sector reads */
    if (FullSectorCount == 0)
        FullSectorCount = 256;
     /* Take account of 256-sector reads */
    /* First we write the registers */
    ata_cmd_WriteRegs(Ata_p, Cmd_p);
     /* Transfering Data: Read Loop*/
    Ata_p->HalHndl->DataDirection = ATA_DATA_IN;
    Data_p = (U16*)Cmd_p->DataBuffer;

	/*In SATA DMA transfer in 8*K Blocks*/        
	if(Ata_p->DeviceType == STATAPI_SATA)
	{
#if defined(ST_7100) || defined(ST_7109)    		
 	    RemainByteCount = FullSectorCount * SECTOR_BSIZE;
    	/*Bug 51676*/
    	while((Error == FALSE) && (RemainByteCount > 0))
    	{
        	XBytes =(RemainByteCount < MAX_DMA_BLK_SIZE)? RemainByteCount:MAX_DMA_BLK_SIZE;
       		FullSectorCount = XBytes/(SECTOR_BSIZE);

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
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT,DRDY_BIT_MASK,
                                                          DRDY_BIT_MASK))
                        {
                            Ata_p->LastExtendedErrorCode= 0x36;
                            GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
                            return TRUE;
                        }

                        /* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
                        {
                            STTBX_Print(("BSY bit didn't clear 0x35\n"));
                            Ata_p->LastExtendedErrorCode= 0x35;
                            Error= TRUE;
                            break;
                        }

                        /* Finally write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        FirstTime = 0;
                        /* Now check the status*/
                 }/*if*/
                          
                 HaveRead = TRUE;
                 /* Read one DRQ sector*/
                 Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                 Cmd_p->DevCtrl, Cmd_p->DevHead,
                                 Data_p, FullSectorCount * SECTOR_WSIZE,
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
	                    Data_p += SECTOR_WSIZE * FullSectorCount;
	                    FullSectorCount = 0;
                 }

                 /* Do any tidying up required */
                 hal_AfterDma(Ata_p->HalHndl);
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
                        status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((status & (DRDY_BIT_MASK)) == 0)
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

                            status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
                            if (status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | DRQ_BIT_MASK))
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
        }/*Outer while loop*/
#endif/*7100 SATA*/
	}
	else
	{
            while ((Error == FALSE) && (FullSectorCount > 0))
            {
#if defined(ATAPI_DEBUG)
    atapi_intcount = 0;
#endif
                 if(FirstTime == 1)
                 {
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT,DRDY_BIT_MASK,
                                                      DRDY_BIT_MASK))
                        {
	                        Ata_p->LastExtendedErrorCode= 0x36;
	                        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
	                        return TRUE;
                        }

                        /* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
                        {
	                        STTBX_Print(("BSY bit didn't clear 0x35\n"));
	                        Ata_p->LastExtendedErrorCode= 0x35;
	                        Error= TRUE;
	                        break;
                        }

                       /* Finally write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        FirstTime = 0;
                    /* Now check the status*/
                 }
                 WAIT400NS;
                 status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                 /* Checking if Busy =0 and DRQ=1 in order to
                 enter into the transfer phase*/
                 if ((status & (BSY_BIT_MASK | DRQ_BIT_MASK)) != 0)
                 {
                        HaveRead = TRUE;
                        /* Read one DRQ sector*/
                        Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                         Cmd_p->DevCtrl, Cmd_p->DevHead,
                                         Data_p, FullSectorCount * SECTOR_WSIZE,
                                         Cmd_p->BufferSize, Cmd_p->BytesRW,
                                         TRUE, &CrcError);

                        if (Error == FALSE)
                        {

            	            Data_p += (Cmd_p->MultiCnt * SECTOR_WSIZE);
                            /* We just read all the data */
                            FullSectorCount = 0;
                        }
                         /* Do any tidying up required */
                            hal_AfterDma(Ata_p->HalHndl);
                 }

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
                        status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((status & (DRDY_BIT_MASK)) == 0)
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

                                status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
                                if (status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | DRQ_BIT_MASK))
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
    }/*if*/
    
    /*read the output registers and store them in the cmd status structure*/
    GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
    return Error;
}

/************************************************************************
Name: ata_cmd_DmaOut

Description:
    Executes a command with data transfer from the host to the device via DMA.
    The device and mode are already selected, no parameter checks are
    performed. Buffers are assumed to be 16-bit aligned (for DMA).

Parameters:
    Ata_p       Pointer to the ATA control block
    Cmd_p       Pointer to a command structure

Return:
    TRUE        Extended error is set
    FALSE       No error
************************************************************************/
BOOL ata_cmd_DmaOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    volatile U8 status;
    BOOL Error=FALSE, CrcError = FALSE;
    BOOL HaveWritten = FALSE;
    U16* Data_p;
    U32 FullSectorCount ;
    U32 RemainByteCount=0;
    U8 FirstTime=1;
#if defined(ST_7100) || defined(ST_7109)    
    U32 SentByteCount=0;
    U32 XBytes=0;
#endif    
    *Cmd_p->BytesRW=0;

    /* Set up sector count */
    if ((Cmd_p->CmdType & ATA_EXT_BIT) != 0)
        FullSectorCount = (Cmd_p->SecCount << 8) | Cmd_p->SecCount2;
    else
        FullSectorCount = Cmd_p->SecCount;

    /* Take account of 256-sector reads */
    if (FullSectorCount == 0)
        FullSectorCount = 256;

    /* First we write the registers */
    ata_cmd_WriteRegs(Ata_p, Cmd_p);
    Ata_p->HalHndl->DataDirection = ATA_DATA_OUT;
    Data_p = (U16*)Cmd_p->DataBuffer;

   	if(Ata_p->DeviceType == STATAPI_SATA)
   	{
#if defined(ST_7100) || defined(ST_7109)       		
  		RemainByteCount = FullSectorCount * SECTOR_BSIZE;
    	while((Error == FALSE) && (RemainByteCount > 0))
    	{
	        XBytes =(RemainByteCount < MAX_DMA_BLK_SIZE)? RemainByteCount:MAX_DMA_BLK_SIZE;
	        FullSectorCount = XBytes/(SECTOR_BSIZE);
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
                     	if(WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT,
                                                  DRDY_BIT_MASK | BSY_BIT_MASK, DRDY_BIT_MASK))
                        {
                            Ata_p->LastExtendedErrorCode = 0x46;
                            GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
                            return TRUE;
                        }
                    	/* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
                        {
                            STTBX_Print(("BSY bit didn't clear 0x35\n"));
                            Ata_p->LastExtendedErrorCode= 0x35;
                            Error= TRUE;
                            break;
                         }

                         /* Write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        FirstTime=0;

                 }

                 HaveWritten = TRUE;
                 /* Write sector(s) */
                 Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                             Cmd_p->DevCtrl, Cmd_p->DevHead,
                                             Data_p, FullSectorCount * SECTOR_WSIZE,
                                             Cmd_p->BufferSize, Cmd_p->BytesRW,
                                             FALSE, &CrcError);

                 if (Error == FALSE)
                 {

                        while(!(STSYS_ReadRegDev32LE((U32)Ata_p->HalHndl->BaseAddress +
                                                SATA_AHB2STBUS_PC_STATUS) & 0x1))
                                /* wait for AHB to go idle*/
                        {
                      	}
                        RemainByteCount -= XBytes;
                        SentByteCount += XBytes;
                        Data_p += SECTOR_WSIZE * FullSectorCount;
                        FullSectorCount = 0;

                 } /*if*/
                 /* Do any tidying up that might be required */
                 hal_AfterDma(Ata_p->HalHndl);
            
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
                 if( RemainByteCount == 0)
                 {
                        /* Check all these bits return to 0*/
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS,
                                       DRQ_BIT_MASK | BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK,
                                       0))
                        {
                                Ata_p->LastExtendedErrorCode = 0x41;
                                Error = TRUE;
                                break;
                        }

                        /* Check the status*/
                        status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((status & (DRDY_BIT_MASK)) == 0)
                        {
                                Ata_p->LastExtendedErrorCode= 0x32;
                                Error= TRUE;
                                break;
                        }
                        /* SecCount == 0 when we haven't transferred is valid, 256 sect */
                        if ((FullSectorCount < 1) && (HaveWritten == TRUE))
                        {
                            /*Since the drive has transferred all of the requested sectors
                              without error, the drive should not have BUSY, DEVICE FAULT,
                              DATA REQUEST or ERROR active now.*/
                                status = hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
                                if(status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | \
                                                                                  DRQ_BIT_MASK))
                                {
                                        Ata_p->LastExtendedErrorCode= 0x43;
                                        Error= TRUE;
                                        break;
                                }
                                /* All sectors have been read without error, exit */
                                Ata_p->LastExtendedErrorCode= 0x00;
                                break;
                        }
                 }/*RemainSectorCount=0)*/

                 /* Do any tidying up that might be required */
                 hal_AfterDma(Ata_p->HalHndl);

           } /*End of write loop*/
        } /*end of outer while loop*/
#endif/*7100 SATA*/
	}
	else 
	{
            while ((Error == FALSE) && (FullSectorCount > 0))
            {
#if defined(ATAPI_DEBUG)
        atapi_intcount = 0;
#endif
                 if(FirstTime == 1)
                 {
                     	if(WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT,
                                                  DRDY_BIT_MASK | BSY_BIT_MASK, DRDY_BIT_MASK))
                        {
                            Ata_p->LastExtendedErrorCode = 0x46;
                            GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
                            return TRUE;
                        }
                    	/* check status also, to be sure */
                        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
                        {
                            STTBX_Print(("BSY bit didn't clear 0x35\n"));
                            Ata_p->LastExtendedErrorCode= 0x35;
                            Error= TRUE;
                            break;
                        }
                        /* Write the command */
                        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_COMMAND, Cmd_p->CommandCode);
                        FirstTime=0;
                 }/*If */
                 WAIT400NS;
                 /* protocol changed for sata in DMA ,BSY=1*/
                 /* Wait for BSY == 0 and DRQ == 1 */
                 if (WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS,
                                   BSY_BIT_MASK | DRQ_BIT_MASK, DRQ_BIT_MASK))
                 {
                        /* Busy signal, or drive not ready for data */
                        Ata_p->LastExtendedErrorCode = 0x45;
                        return  TRUE;
                 }

                 /* Transfering Data: Write Loop*/
                 status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                 /* Check for BSY xor DRQ to continue */
	             if ((status & (DRQ_BIT_MASK | BSY_BIT_MASK)) != 0)
                 {
                        HaveWritten = TRUE;
                        /* Write sector(s) */
                        Error = hal_DmaDataBlock(Ata_p->HalHndl,
                                             Cmd_p->DevCtrl, Cmd_p->DevHead,
                                             Data_p, FullSectorCount * SECTOR_WSIZE,
                                             Cmd_p->BufferSize, Cmd_p->BytesRW,
                                             FALSE, &CrcError);

                        if (Error == FALSE)
                        {

                            Data_p += (Cmd_p->MultiCnt * SECTOR_WSIZE);
                            /* We just wrote all the data */
                            FullSectorCount = 0;
                        }
                 } /*if*/
                 /* Do any tidying up that might be required */
                 hal_AfterDma(Ata_p->HalHndl);
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
                 if( RemainByteCount == 0)
                 {
                        /* Check all these bits return to 0*/
                        if (WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS,
                                       DRQ_BIT_MASK | BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK,
                                       0))
                        {
                                Ata_p->LastExtendedErrorCode = 0x41;
                                Error = TRUE;
                                break;
                        }

                        /* Check the status*/
                        status = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);

                        if ((status & (DRDY_BIT_MASK)) == 0)
                        {
                                Ata_p->LastExtendedErrorCode= 0x32;
                                Error= TRUE;
                                break;
                        }
                        /* SecCount == 0 when we haven't transferred is valid, 256 sect */
                        if ((FullSectorCount < 1) && (HaveWritten == TRUE))
                        {
                            /*Since the drive has transferred all of the requested sectors
                              without error, the drive should not have BUSY, DEVICE FAULT,
                              DATA REQUEST or ERROR active now.*/
                            status = hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
                            if(status & (BSY_BIT_MASK | ERR_BIT_MASK | DF_BIT_MASK | \
                                                                                  DRQ_BIT_MASK))
                            {
                                 Ata_p->LastExtendedErrorCode= 0x43;
                                 Error= TRUE;
                                 break;
                            }
                            /* All sectors have been read without error, exit */
                            Ata_p->LastExtendedErrorCode= 0x00;
                            break;
                        }
                 }/*RemainSectorCount=0)*/
                 /* Do any tidying up that might be required */
                 hal_AfterDma(Ata_p->HalHndl);
            } /*End of write loop*/
    } 
    /*read the output registers and store them in the cmd status structure*/
    GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
    return Error;
}


/************************************************************************
Name: ata_cmd_ExecuteDiagnostics

Description: Executes this special command
Parameter:  Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command structure
Return:     TRUE:  Extended error is set
            FALSE: No error


************************************************************************/
BOOL ata_cmd_ExecuteDiagnostics(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p)
{
    BOOL Error=FALSE;

    Ata_p->HalHndl->DataDirection = ATA_DATA_NONE;
    /* First we write the registers */

#if  (ATAPI_USING_INTERRUPTS) 
   if(Ata_p->DeviceType == STATAPI_SATA)
   		 hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET);
   else
         hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#else
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_SET); /*Disable Interrupts*/
#endif

    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD, Cmd_p->DevHead);
    /* Finally write the command */
    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_COMMAND, Cmd_p->CommandCode);

#if  (ATAPI_USING_INTERRUPTS) 
#if defined (ATAPI_DEBUG)
	STTBX_Print(("ata_cmd_ExecuteDiagnostics(%08x)\n",Ata_p));
#endif
    if(Ata_p->DeviceType == STATAPI_EMI_PIO4)
    {
	    if(hal_AwaitInt(Ata_p->HalHndl,INT_TIMEOUT * 10))
	    {
	        Ata_p->LastExtendedErrorCode= 0x24;
	        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
	        return TRUE;
	    }
    }
    else
    {
	    /* Wait t > 2msec */
	    task_delay(TWO_MS);    
	}
#if defined (ATAPI_DEBUG)
	STTBX_Print(("ata_cmd_ExecuteDiagnostics(%08x) \n",Ata_p));
#endif	
#else   
    /* Wait t > 2msec */
    task_delay(TWO_MS);
#endif /* ATAPI_USING_INTERRUPTS */

    if (WaitForBit(Ata_p->HalHndl, ATA_REG_ALTSTAT, BSY_BIT_MASK, 0))
    {
        Ata_p->LastExtendedErrorCode= 0x24;
        GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
        return TRUE;
    }

    /*read the output registers and store them in the cmd status structure*/
    GetCmdStatus(Ata_p,Cmd_p->Stat.CmdStat);
    Ata_p->LastExtendedErrorCode= 0x00;

#if  (ATAPI_USING_INTERRUPTS) 
	if(Ata_p->DeviceType == STATAPI_SATA)
    	hal_RegOutByte(Ata_p->HalHndl,ATA_REG_CONTROL, nIEN_CLEARED);
#endif
     return Error;
}


/************************************************************************
Name: GetCmdStatus

Description: Retrieves the command block registers and store them in the
             structure previosly allocated

Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Cmd_p : Pointer to a command status structure


************************************************************************/

static void GetCmdStatus(ata_ControlBlock_t *Ata_p,STATAPI_CmdStatus_t *Stat_p)
{
    U32 Dummy;

    Stat_p->Status = hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);
    Stat_p->CylinderLow = hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLLOW);
    Stat_p->CylinderHigh = hal_RegInByte(Ata_p->HalHndl,ATA_REG_CYLHIGH);
    Stat_p->SectorCount = hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECCOUNT);
    Stat_p->Head = hal_RegInByte(Ata_p->HalHndl,ATA_REG_DEVHEAD);
    Stat_p->Sector = hal_RegInByte(Ata_p->HalHndl,ATA_REG_SECNUM);

    Stat_p->Error = hal_RegInByte(Ata_p->HalHndl,ATA_REG_ERROR);

    Stat_p->LBA=0x00000000;

    Dummy = Stat_p->Sector;
    Stat_p->LBA |= Dummy & 0x000000FF;
    Dummy = Stat_p->CylinderLow;
    Stat_p->LBA |= (Dummy<<8) & 0x0000FF00;
    Dummy = Stat_p->CylinderHigh;
    Stat_p->LBA |= (Dummy<<16) & 0x00FF0000;
    Dummy = Stat_p->Head;
    Stat_p->LBA |= (Dummy<<24 )& 0x0F000000;

    if (LastCommandExtended == TRUE)
    {
      
       /* Set the HOB bit and Interrupt enable bit depending on mode used*/
#if !(ATAPI_USING_INTERRUPTS)       
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CONTROL, CONTROL_HOB_SET | nIEN_SET );
#else        
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CONTROL, CONTROL_HOB_SET | nIEN_CLEARED );
#endif /*ATAPI_USING_INTERRUPTS*/
        
        /* We can destroy these values, since if it's an extended command
         * they shouldn't be looking at them anyway.
         */
        Stat_p->CylinderLow = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLLOW);
        Stat_p->CylinderHigh = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLHIGH);
        Stat_p->Sector = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECNUM);

        Dummy = Stat_p->Sector & 0xff;
        Dummy <<= 24;
        Stat_p->LBA |= Dummy;

        Dummy = Stat_p->CylinderLow  & 0x00ff;
        Stat_p->LBAExtended  = Dummy;

        Dummy = Stat_p->CylinderHigh & 0xff00;
        Dummy <<= 8;
        Stat_p->LBAExtended |= Dummy;

        /* Reset HOB bit */
#if !(ATAPI_USING_INTERRUPTS)       
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CONTROL, CONTROL_HOB_CLEARED | nIEN_SET );
#else        
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_CONTROL, CONTROL_HOB_CLEARED | nIEN_CLEARED );
#endif /*ATAPI_USING_INTERRUPTS*/
        
    }
}

/*end of cmmd_ata.c --------------------------------------------------*/
