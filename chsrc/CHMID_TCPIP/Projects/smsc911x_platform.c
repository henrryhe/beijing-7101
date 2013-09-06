/***************************************************************************
 *
 * Copyright (C) 2008  SMSC
 *
 * All rights reserved.
 *
 ***************************************************************************
 * File: new_smsc911x_platform.c
 */

#include "smsc_environment.h"
#include "interfaces/smsc911x.h"
#include "smsc911x_platform.h"

#if !SMSC911X_ENABLED
#error Platform functions not implemented
#endif

#define SMSC_INTERRUPT_STACK_SIZE	(2048)

#if SMSC_ERROR_ENABLED
#define SMSC911X_PLATFORM_DATA_SIGNATURE (0xB8973454)
#endif

typedef struct SMSC911X_PLATFORM_DATA_
{
	DECLARE_SIGNATURE
	SMSC911X_ISR isr;
	void * isr_argument;
	
	/* platform specific data structure: io address, irq, isr
	handler and other platform specific data need to be part of
	this structure.
	*/
} SMSC911X_PLATFORM_DATA;

static SMSC911X_PLATFORM_DATA Smsc911xPlatformData[SMSC911X_INTERFACE_COUNT];
static SMSC911X_PRIVATE_DATA Smsc911xPrivateData[SMSC911X_INTERFACE_COUNT];

#if SMSC_TRACE_ENABLED
void Smsc911xPlatform_DisplayResources(int debugMode,int interfaceIndex)
{
	/* Prints the platform specific data such as io address, irq
	number, level and other platform specific information if any.
	*/
}
#endif

SMSC911X_PRIVATE_DATA * Smsc911xPlatform_Initialize(int interfaceIndex)
{
	/* Initialize the platform io address for the interface, assign
	irq number & level.
	*/
}

mem_ptr_t Smsc911xPlatform_GetIOAddress(int interfaceIndex)
{
	/* Return the io address for the interface from
	SMSC911X_PLATFORM_DATA structure.
	 */
}

static void Smsc911xPlatform_IsrWrapper(void * argument)
{
	/* ISR wrapper:
	NOTE: not all platforms will require an ISR wrapper. 
	This wrapper is used because some platforms does not use a
	return value where as the driver is written as if there 
	was a return value. 
	*/
}

err_t Smsc911xPlatform_InstallIsr(int interfaceIndex,SMSC911X_ISR isr,void * isrArgument)
{
	/* This function should do interrupt request and install
	interrupt handler.
	*/
}

void Smsc911xPlatform_GetMacAddress(unsigned char macAddress[6])
{
	int i;
	/* Generate random mac address */
	SMSC_ASSERT(macAddress!=NULL);
	for(i=0;i<6;i++) {
		macAddress[i]=(u8_t)rand();
	}
	macAddress[0]&=~(0x01);/* not a multicast address */
	macAddress[0]|=(0x02);/* locally assigned address */
}

/* Writes a buffer to the TX_DATA_FIFO */
void Smsc911xPlatform_TxWriteFifo(mem_ptr_t ioAddress, u32_t *buf, u16_t wordCount)
{
#define TX_DATA_FIFO		0x20
	/* NOTE: These larger loops with repeated writes
	    have greately improved speed over the small 
	    loop with single write */
	while(wordCount>=16) {
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 0*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 1*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 2*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 3*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 4*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 5*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 6*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 7*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 8*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 9*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 10*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 11*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 12*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 13*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 14*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 15*/
		wordCount-=16;
	}
	while(wordCount>=4) {
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 0*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 1*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 2*/
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));/*word 3*/
		wordCount-=4;
	}
	while (wordCount) {
		SMSC911X_PLATFORM_WRITE_REGISTER(ioAddress,TX_DATA_FIFO,*(buf++));
		wordCount--;
	}
}

void Smsc911xPlatform_RxReadFifo(mem_ptr_t ioAddress, u32_t * buf, u16_t wordCount)
{
#define RX_DATA_FIFO 0x00
	/* NOTE: These larger loops with repeated reads
	    have greately improved speed over the small 
	    loop with single read */
	while(wordCount>=16) {
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 0*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 1*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 2*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 3*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 4*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 5*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 6*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 7*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 8*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 9*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 10*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 11*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 12*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 13*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 14*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 15*/
		wordCount-=16;
	}
	while(wordCount>=4) {
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 0*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 1*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 2*/
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);/*word 3*/
		wordCount-=4;
	}
	while(wordCount) {
		*(buf++)=SMSC911X_PLATFORM_READ_REGISTER(ioAddress,RX_DATA_FIFO);
		wordCount--;
	}
}

