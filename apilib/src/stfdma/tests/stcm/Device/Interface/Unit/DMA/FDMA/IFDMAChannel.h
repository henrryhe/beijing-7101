
// FILE:			Device/Interface/Unit/DMA/FDMA/IFDMAChannel.h
//
// AUTHOR:		Gabriel Gooslin
// COPYRIGHT:	(c) 2003 STMicroelectronics.  All Rights Reserved.
// CREATED:		10.01.2003
//
// PURPOSE:		FDMA Channel interface
//
// SCOPE:		INTERNAL Interface File
///

#ifndef IFDMACHANNEL_H
#define IFDMACHANNEL_H

#include "VDR/Interface/Base/IVDRBase.h"
#include "IFDMAChannelClient.h"

static const VDRIID VDRIID_FDMA_INTERRUPT_CALLBACK = 0x80007002;
static const VDRIID VDRIID_FDMA_CHANNEL = 0x80007003;


enum FDMATimingModel
	{
	FDMATM_FREE_RUNNING,	// no timing control
	FDMATM_PACED_SRC,		// read single unit from source on request
	FDMATM_PACED_DEST		// write single unit to destination on request
	};


enum FDMADataUnit
	{
	FDMADU_1_BYTE,
	FDMADU_2_BYTES,
	FDMADU_4_BYTES,
	FDMADU_8_BYTES,
	FDMADU_16_BYTES,
	FDMADU_32_BYTES
	};


// FDMA only does 1D at the moment, but I have left all types in in case the functionallity is added
enum FDMADataStructure
	{
	FDMADS_CONST,	// fixed address
	FDMADS_1D_INC,	// linear incrementing
	FDMADS_1D_DEC,	// linear decrementing
	FDMADS_2D_INC,	// array incrementing (only allowed for linked list channels)
	FDMADS_2D_DEC	// array decrementing (only allowed for linked list channels)
	};

class IFDMAChannel : public virtual IVDRBase
	{
	public:
		virtual STFResult RegisterClientSignalInterface(IFDMAChannelClient* channelClient) = 0;

		//! This can be called as a test to agree about timing model.
		//! If this call fails (unsupported value), retry with other timing model.
		virtual STFResult QueryTimingModel(FDMATimingModel timingModel) = 0;

		//! This can be called as a test figure out if the data units need to be aligned with source and destination pointers.
		//! If this call fails (unsupported values), retry with source and/or destination units aligned.
		virtual STFResult QueryDataAlignment(bool sourceDataUnitsAligned, bool destinationDataUnitsAligned) = 0;

		//! This can be called as a test to agree about data units used by source and destination.
		//! If this call fails (unsupported values), retry with other data units.
		virtual STFResult QueryDataUnits(FDMADataUnit sourceDataUnit, FDMADataUnit destinationDataUnit) = 0;

		//! This can be called as a test to agree about data structures used by source and destination.
		//! If this call fails (unsupported values), retry with other data structures.
		virtual STFResult QueryDataStructures(FDMADataStructure sourceDataStructure, FDMADataStructure destinationDataStructure) = 0;

		//! This will change the required number of transfers buffered before the FDMA channel is permitted to start.
		virtual STFResult SetRequiredBufferedTransfersBeforeStart(uint32 afterBufferedTransfers) = 0;

		//! This will start the FDMA channel after a certain number of transfers have been posted.
		virtual STFResult StartTransfer(uint32 afterBufferedTransfers = 0) = 0;

		//! This function will stop the channel as soon as possibile, then flush the transfer list.  This call cannot ensure that previously posted 
		//! transfers are not executed by the FDMA.
		virtual STFResult StopTransfer(void) = 0;

		//! This function will stop the channel as soon as possibile but will leave the nodelist intact
		virtual STFResult PauseTransfer(void) = 0;

		//! This function will resume the FDMA Channel from where it was previouly paused.  This resume also will only happen provided there are enough 
		//! transfers posted to the channel.  if there are enough transfers posted, the channel will start, if not the channel will start after 
		//! a call to PostTransfer() which filled the list to the required level.
		virtual STFResult ResumeTransfer(void) = 0;

		//! This function will set the channel into a completeing state where all remaining transfers are executed regardless of "afterBufferedTransfers"
		//! also any new transfer request called after this call will still be honored
		virtual STFResult ForceCompleteAllTransfers(void) = 0;

		//! Returns true if the channel is currently in a running state
		virtual bool IsRunning(void) = 0;

		//! Returns the current channel state
		virtual FDMAChannelState GetChannelState(void) = 0;
	};


class IFDMAChannelInterrupt
	{
	public:
		virtual STFResult EarlyInterrupt() = 0;
		virtual STFResult ChannelStopInterrupt() = 0;
		virtual bool IsRunning() = 0;
	};

#endif //IFDMACHANNEL_H
