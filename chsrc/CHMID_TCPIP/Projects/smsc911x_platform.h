/*****************************************************************************
 *
 * Copyright (C) 2008  SMSC
 *
 * All rights reserved.
 *
 *****************************************************************************
 * File: smsc911x_platform.h
 */

#ifndef SMSC911X_PLATFORM_H
#define SMSC911X_PLATFORM_H

/*****************************************************************************
* CONSTANT: SMSC911X_INTERFACE_COUNT
* DESCRIPTION:
*    This must be set equal to the maximum number of interfaces that can
*    be initialized on this platform for use with this smsc911x driver.
*    It is usually set to 1, but in cases where there is more than one
*    device present it is set higher.
*****************************************************************************/
#define SMSC911X_INTERFACE_COUNT	(1)

/*****************************************************************************
* FUNCTION/MACRO: SMSC911X_PLATFORM_READ_REGISTER
* DESCRIPTION:
*    This is the low level register read function. All non fifo, read accesses
*    to the hardware will use this function for reading from registers.
* PARAMETERS:
*    mem_ptr_t ioAddress, this is the base address of the device's register
*        space. It was previously retrieved from Smsc911xPlatform_GetIOAddress.
*    u8_t reg, this is the register offset(in bytes) from the base address
*        where the returned value is read from.
* RETURN VALUE:
*    returns a u32_t which is the value obtained after reading the specified
*        register.
*****************************************************************************/
#define SMSC911X_PLATFORM_READ_REGISTER(ioaddress,reg)		\
	(*((volatile u32_t *)((ioaddress)+(reg))))

/*****************************************************************************
* FUNCTION/MACRO: SMSC911X_PLATFORM_WRITE_REGISTER
* DESCRIPTION:
*    This is the low level register write function. All non fifo, write
*    accesses to the hardware will use this function for writing to registers.
* PARAMETERS:
*    mem_ptr_t ioAddress, this is the base address of the device's register
*        space. It was previously retrieved from Smsc911xPlatform_GetIOAddress.
*    u8_t reg, this is the register offset(in bytes) from the base address
*        where the value is written to.
*    u32_t value, this is the value to write to the specified register
* RETURN VALUE:
*    None.
*****************************************************************************/
#if 0
#define SMSC911X_PLATFORM_WRITE_REGISTER(ioaddress,reg,value)	\
	{(*((volatile u32_t *)((ioaddress)+(reg))))=(value);printf("v=%x\n",value);}
#else
#define SMSC911X_PLATFORM_WRITE_REGISTER(ioaddressch,reg,value)	\
	*((volatile u32_t *)((u32_t)ioaddressch+reg))=value

#endif
/*****************************************************************************
* FUNCTION/MACRO: Smsc911xPlatform_DisplayResources
* DESCRIPTION:
*    This is used for informational purposes to display the resources used
*    by this particular device.
* PARAMETERS:
*    int debugMode, This is the value that is passed to the SMSC_TRACE
*        statements inside the call.
*    int interfaceIndex, This is the index value of the device whos resources
*        we want to display.
* RETURN VALUE:
*    None.
*****************************************************************************/
#if SMSC_TRACE_ENABLED
void Smsc911xPlatform_DisplayResources(int debugMode, int interfaceIndex);
#else 
void Smsc911xPlatform_DisplayResources(int debugMode, int interfaceIndex);
#endif

/*****************************************************************************
* FUNCTION/MACRO: Smsc911xPlatform_Initialize
* DESCRIPTION:
*    This function does any platform specific initialization that may be
*    necessary. This includes but is not limited to bus access setups.
*    This does not include normal device initialization such as reset
*    and configuration. That will be done by the driver after this call is
*    made. This function will be called before any access to the device.
* PARAMETERS:
*    int interfaceIndex, this is the device index that should be initialized.
* RETURN VALUE:
*    On success, returns a pointer to a non initialize private data area that
*        will be used for this device.
*    On failure, returns NULL.
*****************************************************************************/
SMSC911X_PRIVATE_DATA * Smsc911xPlatform_Initialize(int interfaceIndex);

/*****************************************************************************
* FUNCTION TYPE: SMSC911X_ISR
* DESCRIPTION:
*    This is the function template used by the interrupt sub routine of the
*    driver. It is passed to Smsc911xPlatform_InstallIsr. After the call to
*    Smsc911xPlatform_InstallIsr returns ERR_OK, the provided function of
*    this type will be called when ever the device signals its IRQ line.
* PARAMETERS:
*    argument, this is same as the isrArgument passed to the function
*        Smsc911xPlatform_InstallIsr.
* RETURN VALUE:
*    returns zero if the driver could not identify any work to be done.
*    returns non zero if the driver did identify work to be done, and deactivated
*        the interrupt signal.
*****************************************************************************/
typedef int (*SMSC911X_ISR)(void * argument);

/*****************************************************************************
* FUNCTION: Smsc911xPlatform_InstallIsr
* DESCRIPTION:
*    This function will install an interrupt sub routine to be attached to
*    a particular device.
* PARAMETERS:
*    int interfaceIndex, this is the index that specifies the device 
*        which the ISR should be attached to.
*    SMSC911X_ISR isr, this is the function pointer of the ISR to be called
*        when the device signals an interrupt.
*    void * isrArgument, this is the argument to use when the ISR is called.
* RETURN VALUE:
*    On Success returns ERR_OK,
*    On Failure returns something other than ERR_OK.
*****************************************************************************/
#if 0
err_t Smsc911xPlatform_InstallIsr(int interfaceIndex,SMSC911X_ISR isr,void * isrArgument);
#endif

/*****************************************************************************
* FUNCTION: Smsc911xPlatform_GetIOAddress
* DESCRIPTION:
*    This function will get the base address of the specified device
* PARAMETERS:
*    int interfaceIndex, specified the device of which to get the base
*        address.
* RETURN VALUE:
*    returns a mem_ptr_t which is the base address of the specified device
*****************************************************************************/
mem_ptr_t Smsc911xPlatform_GetIOAddress(int interfaceIndex);

/*****************************************************************************
* FUNCTION: Smsc911xPlatform_GetMacAddress
* DESCRIPTION:
*     This function is called only if the LAN9X1X can not get a mac address
*     on its own. That is the LAN9X1X is not hooked up to an eeprom, or the
*     eeprom is not loaded properly. Upon detecting that the mac address was
*     not loaded properly, this function is used to get the mac address from
*     a system specific resource.
* PARAMETERS:
*     unsigned char macAddress[6], this is where the mac address will be
*         stored.
* RETURN VALUE:
*     None.
* NOTE:
*     This function should be changed so that it also passes an 
*     interfacIndex as other function do since getting the proper
*     macAddress may depend upon known the proper interfaceIndex.
*     This maybe updated in a future version to support that.
*****************************************************************************/
void Smsc911xPlatform_GetMacAddress(unsigned char macAddress[6]);

/*****************************************************************************
* FUNCTION: Smsc911xPlatform_TxWriteFifo
* DESCRIPTION:
*    This function is used for writing a data buffer to the TX Fifo.
* PARAMETERS:
*    mem_ptr_t ioAddress, this is the base address of the device's register
*        space. It was previously retrieved from Smsc911xPlatform_GetIOAddress.
*    u32_t * buf, this is a pointer to the buffer space. The pointer is 4 byte 
*        aligned.
*    u16_t wordCount, this is the number of 4 byte words that are to be
*        written to the tx fifo.
* RETURN VALUE:
*    None.
*****************************************************************************/
void Smsc911xPlatform_TxWriteFifo(mem_ptr_t ioAddress, u32_t *buf, u16_t wordCount);

/*****************************************************************************
* FUNCTION: Smsc911xPlatform_RxReadFifo
* DESCRIPTION:
*    This function is used for reading the RX Fifo into a data buffer.
* PARAMETERS:
*    mem_ptr_t ioAddress, this is the base address of the device's register
*        space. It was previously retrieved from Smsc911xPlatform_GetIOAddress.
*    u32_t * buf, this is a pointer to the buffer space. The pointer is 4 byte 
*        aligned.
*    u16_t wordCount, this is the number of 4 byte words that are read from
*        the rx fifo.
* RETURN VALUE:
*    None.
*****************************************************************************/
void Smsc911xPlatform_RxReadFifo(mem_ptr_t ioAddress, u32_t * buf, u16_t wordCount);

#endif
