
// FILE:			Device/Interface/Unit/DMA/FDMA/IFDMAChannelClient.h
//
// AUTHOR:		Gabriel Gooslin
// COPYRIGHT:	(c) 2003 STMicroelectronics.  All Rights Reserved.
// CREATED:		10.01.2003
//
// PURPOSE:		FDMA Channel client interface
//
// SCOPE:		INTERNAL Interface File
///

#ifndef IFDMACHANNELCLIENT_H
#define IFDMACHANNELCLIENT_H




//TBD: eventually add more states to the FDMAChannel...This is an example struct
//typedef enum _FDMAChannelState
//	{
//	CHANNEL_STOPPED,				// 
//	CHANNEL_STOPPING,				// stop command is going through this state, if the channel is currently running (not on hold or idle)
//	CHANNEL_RUNNING,				// channel is active working on data
//	CHANNEL_IDLE,					// channel is not stopped, not paused, all data was transferred, no pending entries
//	CHANNEL_ON_HOLD,				// channel is not stopped, not paused, but currently not active, because of an internal event (needed for SCD channel)
//	CHANNEL_PAUSING,				// pause command is going through this state, if the channel is currently running
//	CHANNEL_PAUSED_RUNNING,		// channel was paused while running, there is pending data to be transferred
//	CHANNEL_PAUSED_IDLE,			// channel was paused while idle, 
//	CHANNEL_PAUSED_ON_HOLD,		// channel was paused while on hold
//	CHANNEL_RESTARTING
//	}FDMAChannelState;



typedef enum _FDMAChannelState
	{
	CHANNEL_STOPPED,
	CHANNEL_RUNNING,
	CHANNEL_ON_HOLD,				// channel is not stopped, not paused, but currently not active, because of an internal event (needed for SCD channel)
	CHANNEL_IDLE,					// channel is not stopped, not paused, all data was transferred, no pending entries
	CHANNEL_PAUSED,
	CHANNEL_RESTARTING
	}FDMAChannelState;


class IFDMAChannelClient
	{
	public:

		// a transfer that was posted with "param" != 0 has completed
		virtual STFResult SignalTransferComplete(uint32 param) = 0;

		// when the client calls StopTransfer() the channel will not stop immediatly.  However, when the channel
		// does finally stop this callback will be made
		virtual STFResult SignalChannelStop() = 0;

		// only need to override this if your channel is going to divide transfers into smaller transfers.  
		virtual STFResult SignalTransferInProgress(uint32 param) {STFRES_RAISE_OK;}

		// when the client calls StopTransfer() the channel will not stop immediatly.  However, when the channel
		// does finally stop this callback will be made
		virtual STFResult SignalChannelStateChange(FDMAChannelState state) {STFRES_RAISE_OK;}

		// this currently has no use, it may in the future
		virtual STFResult SignalChannelError(STFResult error) {STFRES_RAISE_OK;}
	};


class ISCDChannelClient : public virtual IFDMAChannelClient
	{
	public:
		// an MPEG startcode of 32bits (00 00 01 ID) was found at the given address with the given identifier
		virtual STFResult FoundStartCode(uint8 * address, uint8 codeID) = 0;
	};

#endif //IFDMACHANNELCLIENT_H
