/************************************************************************

Source file name : ctrl_atapi.c

Description: Implementation of the ATA Control Interface and Device/Protocol
            setup  intermediate modules following the API Design Document
            v0.9.0 of the ATAPI driver.

COPYRIGHT (C) STMicroelectronics  2004

************************************************************************/

/*Includes-------------------------------------------------------------*/
#include <string.h>

#include "stlite.h"
#include "stcommon.h"

#include "stos.h"
#include "statapi.h"
#include "hal_atapi.h"
#include "ata.h"

#if defined(ATAPI_DEBUG)
#include "sttbx.h"
#endif

/* Shared Variables----------------------------------------------------*/

#if defined(ATAPI_DEBUG)
/* Temporary variable; used to turn on and off some tracing in driver
 * (eg commands written to registers echoed, etc.)
 */
extern BOOL ATAPI_Trace;
extern BOOL ATAPI_Verbose;
#endif

/*Private Types--------------------------------------------------------*/

/*Private Constants----------------------------------------------------*/

#define HARD_RESET_TIMEOUT_SECONDS  10

/*Private Variables----------------------------------------------------*/

/*Private Macros-------------------------------------------------------*/

/*Private functions prototypes-----------------------------------------*/

/*Functions------------------------------------------------------------*/
/************************************************************************
Name: ata_ctrl_SelectDevice

Description: Attempts to select the device by writing to the Device/Header reg.
             Also delays the mandatory 400 ns to ensure subsequent device access is
             safe
Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Device: number identifying the device (0,1)

************************************************************************/
BOOL ata_ctrl_SelectDevice(ata_ControlBlock_t *Ata_p, U8 Device)
{
    U8 data;


        /* First wait for not busy (BSY=0)... */
    if(WaitForBit(Ata_p->HalHndl,ATA_REG_STATUS,BSY_BIT_MASK,0))
    {
         Ata_p->LastExtendedErrorCode=0x11;
         return TRUE;
    }

        /* ..then wait for DRQ=0 */
    if(WaitForBit(Ata_p->HalHndl,ATA_REG_STATUS,DRQ_BIT_MASK,0))
    {
         Ata_p->LastExtendedErrorCode=0x12;
         return TRUE;
    }

    data= hal_RegInByte(Ata_p->HalHndl,ATA_REG_DEVHEAD);
    if(Device==DEVICE_0) data= data & ~BIT_4;
    else          data= data | BIT_4;

    hal_RegOutByte(Ata_p->HalHndl,ATA_REG_DEVHEAD,data);
    WAIT400NS;

    /* First wait for not busy (BSY=0)... */
   if(WaitForBit(Ata_p->HalHndl,ATA_REG_STATUS,BSY_BIT_MASK,0))
    {
         Ata_p->LastExtendedErrorCode=0x11;
         return TRUE;
    }
    /* ...then wait for DRQ=0 */
    if(WaitForBit(Ata_p->HalHndl,ATA_REG_STATUS,DRQ_BIT_MASK,0))
    {
         Ata_p->LastExtendedErrorCode=0x12;
         return TRUE;
    }

    return FALSE;
}
/************************************************************************
Name: ata_ctrl_SoftReset

Description: Pulses the SRST bit in the DeviceCtrl register high for 400 ns and
             then low. Delays for mandatory 400ns afterwards
Parameters: Two params:
            Ata_p : Pointer to the ATA control block
            Device: number identifying the device (0,1)

************************************************************************/
BOOL ata_ctrl_SoftReset(ata_ControlBlock_t *Ata_p)
{
    S32 TimeOut=ATA_TIMEOUT;
    U8  sc, sn;

    if (hal_SoftReset(Ata_p->HalHndl) == TRUE)
    {
#if defined(ATAPI_DEBUG)
        if (ATAPI_Verbose)
        {
            STTBX_Print(("ata_ctrl_SoftReset(): hal_SoftReset failed\n"));
        }
#endif
        return TRUE;
    }

    /*RESET DONE. This causes device 0 be selected.*/

    /* If there is a device 0, wait for device 0 to set BSY=0.*/
    if(Ata_p->DevInBus[0] != NONE_DEVICE)
    {
        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
        {
            Ata_p->LastExtendedErrorCode = 0x01;
            return TRUE;
        }
    }

    /* If there is a device 1, wait until device 1 allows
       register access.*/
    if(Ata_p->DevInBus[1] != NONE_DEVICE)
    {
        /* Select the device */
        hal_RegOutByte(Ata_p->HalHndl, ATA_REG_DEVHEAD, DEVHEAD_DEV1);
        WAIT400NS;

        while(TimeOut >= 0)
        {
            sn = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECNUM);
            sc = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECCOUNT);
            if ((sn == 0x01) && (sc == 0x01))
                break;
            TimeOut--;
        }

        if(TimeOut<0)
        {
            Ata_p->LastExtendedErrorCode = 0x02;
            return TRUE;
        }

        /* Now wait for device 1 to set BSY=0 */
        if(WaitForBit(Ata_p->HalHndl, ATA_REG_STATUS, BSY_BIT_MASK, 0))
        {
            Ata_p->LastExtendedErrorCode = 0x03;
            return TRUE;
        }
    }

    return FALSE;
}
/************************************************************************
Name: ata_ctrl_Probe

Description:
    Probes the ATA bus to determine the number of devices attached and
    their type (ATA or ATAPI)

Note: could be re-written more cleanly as a loop.

Parameter:
    Ata_p           Pointer to the ATA control block
************************************************************************/
BOOL ata_ctrl_Probe(ata_ControlBlock_t *Ata_p)
{
    BOOL error = FALSE;
    DU8 sc, sn;
#if !defined(BAD_DISK_NO_SIG_CHECK_ATA)
    DU8 cl, ch, st, dh;
#endif

    Ata_p->DevInBus[0] = NONE_DEVICE;
    Ata_p->DevInBus[1] = NONE_DEVICE;

    /* Soft reset, so the drives will load signature values (section
     * 9.12 of ATA-5 spec).
     */
    if (hal_SoftReset(Ata_p->HalHndl) == TRUE)
    {
#if defined(ATAPI_DEBUG)
        STTBX_Print(("ata_ctrl_Probe(): hal_SoftReset failed!\n"));
#endif
        return TRUE;
    }

    /* RESET DONE. This causes device 0 be selected. */

    sc = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECCOUNT);
    sn = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECNUM);
#if defined(ATAPI_DEBUG)
    if (TRUE == ATAPI_Verbose)
    {
        STTBX_Print(("ata_ctrl_Probe(): sc 0x%02x sn 0x%02x\n", sc, sn));
    }
#endif

    /* These should be 0x01 0x01 for both ATA and ATAPI, so they're a
     * useful sanity check to see if any device is there at all.
     */
    if ((sc == 0x01) && (sn == 0x01))
    {
#if defined(BAD_DISK_NO_SIG_CHECK_ATA)
        Ata_p->DevInBus[0] = ATA_DEVICE;
#else
        /* Now we know there's *something* there, but not what */
        Ata_p->DevInBus[0] = UNKNOWN_DEVICE;

        ch = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLHIGH);
        cl = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLLOW);
        dh = hal_RegInByte(Ata_p->HalHndl, ATA_REG_DEVHEAD);
        st = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
#if defined(ATAPI_DEBUG)
        STTBX_Print(("Drive status: 0x%02x\n", st));
        STTBX_Print(("Error register: 0x%02x\n",
                    hal_RegInByte(Ata_p->HalHndl, ATA_REG_ERROR)));
#endif

        if ((cl == 0x14) && (ch == 0xeb))
            Ata_p->DevInBus[0] = ATAPI_DEVICE;
        else if ((cl == 0x00) && (ch == 0x00) && ((dh & 0x5F) == 0x00)) /* Modif for WD drives */
            Ata_p->DevInBus[0] = ATA_DEVICE;
#endif

#if !defined(BAD_DISK_NO_SIG_CHECK_ATA) && defined(ATAPI_DEBUG)
        if (TRUE == ATAPI_Verbose)
        {
            STTBX_Print(("ata_ctrl_Probe(): ch 0x%02x cl 0x%02x dh 0x%02x\n",
                        ch, cl, dh));
        }
#endif
    }

    /*Now for the device 1*/
    hal_RegOutByte(Ata_p->HalHndl, ATA_REG_DEVHEAD, DEVHEAD_DEV1);
    WAIT400NS;
    
    sc = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECCOUNT);
    sn = hal_RegInByte(Ata_p->HalHndl, ATA_REG_SECNUM);
#if defined(ATAPI_DEBUG)
    if (TRUE == ATAPI_Verbose)
    {
        STTBX_Print(("ata_ctrl_Probe(): sc 0x%02x sn 0x%02x\n", sc, sn));
    }
#endif

    if ((sc == 0x01) && (sn == 0x01))
    {
#if defined(BAD_DISK_NO_SIG_CHECK_ATA)
        Ata_p->DevInBus[1] = ATA_DEVICE;
#else
        /* Now we know there's *something* there, but not what */
        Ata_p->DevInBus[1] = UNKNOWN_DEVICE;

        cl = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLHIGH);
        ch = hal_RegInByte(Ata_p->HalHndl, ATA_REG_CYLLOW);
        dh = hal_RegInByte(Ata_p->HalHndl, ATA_REG_DEVHEAD);
        st = hal_RegInByte(Ata_p->HalHndl, ATA_REG_STATUS);
#if defined(ATAPI_DEBUG)
        STTBX_Print(("Drive status: 0x%02x\n", st));
        STTBX_Print(("Error register: 0x%02x\n",
                    hal_RegInByte(Ata_p->HalHndl, ATA_REG_ERROR)));
#endif

        if ((cl == 0x14) && (ch == 0xeb))
            Ata_p->DevInBus[1] = ATAPI_DEVICE;
        else if ((cl == 0x00) && (ch == 0x00) && ((dh & 0x5F) == 0x10))
        {
         /* Added to solve the problem of Device 0 repsonding for Device 1 if not present*/
        	if(st == 0x50)  
        	      Ata_p->DevInBus[1] = ATA_DEVICE;
        }
        	  
#endif

#if !defined(BAD_DISK_NO_SIG_CHECK_ATA) && defined(ATAPI_DEBUG)
        if (TRUE == ATAPI_Verbose)
        {
            STTBX_Print(("ata_ctrl_Probe(): ch 0x%02x cl 0x%02x dh 0x%02x\n",
                        ch, cl, dh));
        }
#endif
    }

#if defined(ATAPI_DEBUG)
    if (TRUE == ATAPI_Verbose)
    {
        STTBX_Print(("ata_ctrl_Probe(): returning %i\n", error));
    }
#endif

    return error;
}

/************************************************************************
Name: ata_ctrl_HardReset

Description: Does a hard reset, then waits for BSY to clear
Parameter:  Ata_p : Pointer to the ATA control block


************************************************************************/
BOOL ata_ctrl_HardReset(ata_ControlBlock_t *Ata_p)
{
    /* Preload the value */
    volatile U8 Dummy = BSY_BIT_MASK;
    clock_t     timeout;
    U32         TimeoutTicks;

    TimeoutTicks = HARD_RESET_TIMEOUT_SECONDS * ST_GetClocksPerSecond();

    /* If hardreset fails, return TRUE for error */
    if (hal_HardReset(Ata_p->HalHndl) == TRUE)
        return TRUE;

    timeout = time_plus(time_now(), TimeoutTicks);

    /* Wait for BSY=0 or timeout */
    while(TRUE)
    {
        WAIT400NS;
        Dummy = hal_RegInByte(Ata_p->HalHndl,ATA_REG_STATUS);

        if ((Dummy & BSY_BIT_MASK) == 0)
            break;

        /* Returns 1 if first time is after the second */
        if (time_after(time_now(), timeout) == 1)
            break;
    }

    /* Check */
    if ((Dummy & BSY_BIT_MASK) == 0)
        return FALSE;
    else
        return TRUE;
}
/************************************************************************
Name: ata_bus_Acquire

Description: Obtains the bus check semaphore and checks the bus busy flag (SW
             flag). If not set the bus busy flag is set. Releases the bus check
             semaphore
Parameter:  Ata_p : Pointer to the ATA control block


************************************************************************/
BOOL ata_bus_Acquire(ata_ControlBlock_t *Ata_p)
{
    if (NULL == Ata_p)
        return TRUE;

    /* Semaphore needs to protect check, as well as any set */
    STOS_SemaphoreWait( Ata_p->BusMutexSemaphore_p );
    if(Ata_p->BusBusy==FALSE)
    {
        Ata_p->BusBusy=TRUE;
        STOS_SemaphoreSignal ( Ata_p->BusMutexSemaphore_p );
        return FALSE;
    }

    STOS_SemaphoreSignal ( Ata_p->BusMutexSemaphore_p );
    return TRUE;
}
/************************************************************************
Name: ata_bus_Release

Description: Obtains the bus check semaphore and checks the bus busy flag (SW
             flag). If set the bus busy flag is cleared. Releases the bus check
             semaphore
Parameter:  Ata_p : Pointer to the ATA control block


************************************************************************/
BOOL ata_bus_Release(ata_ControlBlock_t *Ata_p)
{
    /* Semaphore needs to protect check, as well as any set */
    STOS_SemaphoreWait ( Ata_p->BusMutexSemaphore_p );
    if(Ata_p->BusBusy == TRUE)
    {
        Ata_p->BusBusy=FALSE;
        STOS_SemaphoreSignal ( Ata_p->BusMutexSemaphore_p );
        return FALSE;
    }

    STOS_SemaphoreSignal ( Ata_p->BusMutexSemaphore_p );
    return TRUE;
}

/************************************************************************
Name: WaitForBit

Description: Wait for a bit to be set or cleared
Parameters: Three params:
            HalHndl : Pointer to the HAL control block
            regNo: Register number
            bitNo: mask with the bit to test
            expected_val: expected value of the register after applying a mask to
            use only the relevant bits

************************************************************************/
BOOL WaitForBit(hal_Handle_t        *HalHndl,
                ATA_Register_t      regNo,
                U8                  bitNo,
                U8                  expected_val)
{
    U32 i;
    volatile U8 dummy;

    for (i=0;i<ATA_TIMEOUT;i++)
    {
        WAIT400NS;
        dummy = hal_RegInByte(HalHndl, regNo);

        if ((dummy & bitNo) == expected_val)
            return FALSE;
    }

#if defined (ATAPI_DEBUG)
	STTBX_Print(("WaitForBit %d !!! TIMEOUT, Reg Val = %x !!\n",bitNo,dummy));
#endif

    return TRUE;
}
/************************************************************************
Name: WaitForBitPkt

Description: Wait for a bit to be set.Version for the packet command interface.
we need a different verion because of the longer delay of the ATAPI devices
Parameters: Three params:
            HalHndl : Pointer to the HAL control block
            regNo: Register number
            bitNo: mask with the bit to test
            expected_val: expected value of the register after applying a mask to
            use only the relevant bits

************************************************************************/
BOOL WaitForBitPkt(hal_Handle_t *HalHndl,ATA_Register_t regNo,U8 bitNo,U8 expected_val)
{
    U32 i;
    volatile U8 dummy;

    for (i=0;i<ATAPI_TIMEOUT;i++)
    {
        WAIT400NS;
        dummy=hal_RegInByte(HalHndl,regNo);

        if( (dummy & bitNo)== expected_val)
            return FALSE;
    }
    return TRUE;
}

/*end of ctrl_atapi.c --------------------------------------------------*/
