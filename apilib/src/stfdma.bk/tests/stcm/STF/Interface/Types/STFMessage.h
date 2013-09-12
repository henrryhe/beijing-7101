///
/// @file       STF/Interface/Types/STFMessage.h
///
/// @brief      Header for generic messages
///
/// @par OWNER: 
///             STF Team
/// @author     U. Sigmund
///
/// @par SCOPE: 
///             EXTERNAL Header File
///
/// @date       2002-12-05 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef STFMESSAGE_H
#define STFMESSAGE_H

#include "STF/Interface/STFMemoryManagement.h"
#include "STF/Interface/STFThread.h"
#include "STF/Interface/STFSynchronisation.h"

class STFMessageDispatcher;

///
/// @class STFMessage
///
/// @brief Message structure that is passed by value. 
///
/// Messages are passed from the sender to a sink.  Messages are passed
/// by value to eliminate the need for memory allocation in interrupts.
/// After the sink has completed processing of the message, it should call
/// its Complete() method to release any waiting thread.

class STFMessage
	{
	protected:
		/// Potentialy waiting thread
		/*!
		If a thread sends a message and waits for its completion, its handle
		is put into this member.  When the message processing is completed, the
		Complete() method will restart the thread.  This waiting is handled
		by the three methods BeginWait(), CompleteWait() and Complete().  The
		first two are called by the waiting thread, the last is called by the
		message sink.
      */
		STFThread	*	wait;

	public:
		/// Type of the message
		/*!
		Each message has a specific type.  Only some message types are global,
		most are either unit/interface specific or private to the sink/sender
		pair.
		*/
		uint32		message;

		/// First optional parameter of the message
		/*!
		If more than the message type is needed as an argument, it can be put
		into this member.
		*/
		uint32		param1;
		
      /// Second optional parameter of the message
		/*!
		If more than one message argument is needed, it can be put here.
		*/
		uint32		param2;

		/// Empty constructor for a message
		STFMessage(void)
			: wait(NULL)
			{
			}

		/// Major constructor for a message
		/*!
		Major constructor for a message.  Only the message_ argument is a required
		parameter, all others are optional, but have to be set if the message type
		requires it.
		@param message_ The type of the message
		@param param1_ The first optional argument
		@param param2_ The second optional argument
		*/
		STFMessage(uint32 message_, uint32 param1_ = 0, uint32 param2_ = 0)
			: wait(NULL), message(message_), param1(param1_), param2(param2_)
			{
			}

		/// Copy constructor for a message
		STFMessage(const STFMessage & msg)
			: wait(msg.wait), message(msg.message), param1(msg.param1), param2(msg.param2)
			{
			}

		/// Assignment operator for a message
		STFMessage & operator=(const STFMessage & msg)
			{
			message = msg.message;
			param1 = msg.param1;
			param2 = msg.param2;
			wait = msg.wait;

			return *this;
			}

		/// Prepare waiting for message completion
		/*!
		If a sending thread needs to wait for a message to be completed, it has
		to call this preparation method, <b>before</b> it sends the message to the sink.
		After sending, it should call CompleteWait() to give up the CPU until the
		message processing is complete.
		*/
		void BeginWait(void);

		/// Start waiting on message completion
		/*!
		After calling BeginWait() and sending the message, the waiting thread should
		call CompleteWait() to actually give up the CPU and wait for the message
		processing to complete.  The receiver will call Complete() after the message
		is processed, and thereby refiring the sending thread.
		*/
		void CompleteWait(void);

		/// Signal completion of message processing
		/*!
		After a receiver has finished message processing, it should call Complete() to
		awake a potentially sleeping sender.  A receiver should not call Complete(), if
		and only if it sends the message to another sink, <b>without</b> waiting for completion.
		If the thread waits for completion it still has to call Complete() afterwards.
		*/
		void Complete();
	};

//-----------------------------------------------------------------------------

///
/// @class STFMessageSink
///
/// @brief A message receiver
///
/// STFMessages are sent to and processed by STFMessageSinks.  The sender of a message
/// calls the SendMessage() method and can then either wait for the message processing or 
/// continue  without.  The message will later be processed by the ReceiveMessage() method.
/// In most cases, the messages will be handed over to a STFMessageDispatcher to cross
/// the thread boundary to the receiving thread.  However, this must not be the case if the
/// message is processed by the calling thread (eg. by using a DirectSTFMessageSink or
/// a DirectSTFMessageDispatcher).  When sending a message from an interrupt, the wait
/// parameter must be false.

class STFMessageSink
	{
	public:
		/// Sending a message to the sink
		/*!
		This message is called by the message sender.
		@param message The message to send
		@param wait Set to true if the caller shall wait for message completion
		*/
		virtual STFResult SendMessage(STFMessage message, bool wait) = 0;
		
		/// Process incoming messages
		/*!
		This method is called for all incoming messages.  It should perform the
		message processing and then either forward the message or signal completion.
		@param message The message to be processed
		*/
		virtual STFResult ReceiveMessage(STFMessage & message) = 0;
	};

///
/// @class DirectSTFMessageSink
///
/// @brief A message sink that processes on the calling thread
///
/// This type of STFMessageSink will process incoming messages directly. It will not
/// forward it to a STFMessageDispatcher.

class DirectSTFMessageSink : public STFMessageSink
	{
	public:
		STFResult SendMessage(STFMessage message, bool wait) {STFRES_RAISE(ReceiveMessage(message));}
	};

///
/// @class DispatchedSTFMessageSink
///
/// @brief STFMessageSink that passes STFMessages through a dispatcher
///
/// All messages that are sent to this sink will be forwarded to a STFMessageDispatcher,
/// which will then call the ReceiveMessage() method on the calling sink (either immediately
/// or later in the dispatchers thread context).

class DispatchedSTFMessageSink : public STFMessageSink
	{
	protected:
		/// The dispatcher that dispatches the messages for this sink
		STFMessageDispatcher	*	dispatcher;
	public:
		/// Major constructor
		DispatchedSTFMessageSink(STFMessageDispatcher * dispatcher);

		STFResult SendMessage(STFMessage message, bool wait);
	};

//-----------------------------------------------------------------------------

///
/// @class STFMessageDispatcher
///
/// @brief A dispatcher for STFMessage
///
/// Usualy a STFMessage is not immediately processed by the STFMessageSink, but forwarded
/// to the thread that owns the STFMessageSink.  This is performed by a STFMessageDispatcher.
/// It receives message and sink pairs, and calls the ReceiveMessage() method for each incoming
/// message on behalf of the owning thread.

class STFMessageDispatcher
	{
	public:
		/// Abort message dispatcher
		/*!
		This method will set the abort member to true and flush all messages.  After this
		call, this dispatcher will no longer accept or forward any messages.  This should
		typically be done before deleting it.
		*/
		virtual STFResult AbortDispatcher(void) = 0;

		/// Message entry from STFMessageSink
		/*!
		This method is usually called by the STFMessageSink SendMessage() method.  It
		will call the sinks ReceiveMessage() function either immediately or later on the
		owning threads trail.
		@param sink The calling and target sink
		@param message The message to send
		@param wait Set to true, if the caller wants to wait for completion
		@return 
		- @p GNR_INVALID_PARAMETERS If wait is set on a dispatcher that does not support waiting
		- @p GNR_OPERATION_ABORTED If the dispatcher is aborted
		- @p GNR_OBJECT_FULL If the dispatcher queue is full
		*/
		virtual STFResult DispatchMessage(STFMessageSink * sink, STFMessage message, bool wait) = 0;
	};

///
/// @class STFMessageTrigger
///
/// @brief A trigger for the arrival of STFMessages

class STFMessageTrigger
	{
	public:
		/// Signal the arrival of a message
		virtual STFResult TriggerMessageArrival(void) = 0;
	};

///
/// @class STFMessageQueue
///
/// @brief A queue for STFMessages
///
/// This is not the implementation of a STFMessage queue, but the interface for
/// one.

class STFMessageQueue
	{
	public:
		virtual ~STFMessageQueue() {}
		
		/// Flush all pending messages
		/*!
		If a dispatcher keeps STFMessages in a queue, it should flush all on this call.
		It should also call Complete() for all messages to release any potentially waiting
		threads.
		*/
		virtual STFResult FlushMessages(void) = 0;

		/// Flush all pending messages for specific STFMessageSink
		/*!
		If a dispatcher keeps STFMessages in a queue, it should flush all that belong
		to a a specific STFMessageSink on this call.  If shall also call Complete() for
		all of these messages to release any potentially waiting threads.
		@param sink The sink for which all messages shall be flushed
		*/
		virtual STFResult FlushMessages(STFMessageSink * sink) = 0;

		/// Returns the state of the queue
		/*!
		This checks for various states of the queue
		@return
		- @p GNR_OPERATION_ABORTED If the queue is aborted
		- @p GNR_OBJECT_FULL If the queue is full
		- @p GNR_OBJECT_EMPTY If the queue is empty
		- @p GNR_OK else
		*/
		virtual STFResult CheckQueueState(void) = 0;
	};

///
/// @class STFMessageProcessor
///
/// @brief A processor for STFMessage
///
/// Usualy a STFMessage is not immediately processed by the STFMessageSink, but forwarded
/// to the thread that owns the STFMessageSink.  This is performed by a STFMessageDispatcher.
/// It receives message and sink pairs.  The STFMessageProcessor will get all messages and call
/// the ReceiveMessage() method for each incoming message on behalf of the owning thread.

class STFMessageProcessor : virtual public STFMessageQueue
	{
	public:
		virtual ~STFMessageProcessor() {}

		/// Abort message processing
		/*!
		After this call, this processor will no longer accept or forward any messages.  This should
		typically be done before deleting it.
		*/
		virtual STFResult AbortProcessor(void) = 0;

		/// Process all pending messages
		/*!
		This method is called by the processing thread.  It dequeues and calls ProcessMessage
		for all pending messages.  It will return when the queue is empty
		@return 
			- @p GNR_OBJECT_EMPTY If the dispatcher queue is empty
			- @p GNR_OPERATION_ABORTED If the queue is aborted
		*/
		virtual STFResult ProcessMessages(void) = 0;
	};

///
/// @class TriggeredSTFMessageProcessor
///
/// @brief A STFMessageProcessor that provides waiting for the reader
///
/// The processing thread can call the WaitMessage() method to wait for incomming messages.
/// This call will block the thread until the processor is aborted or a message arrives.
/// It is not quaranteed, that a message is available even if this call returns with GNR_OK,
/// but if a message is available it will return.

class TriggeredSTFMessageProcessor : virtual public STFMessageProcessor
	{
	public:
		/// Wait for a message to arrive
		/*!
		Blocks the calling thread until a message arrives or the dispatcher is aborted.
		@return 
			- @p GNR_OPERATION_ABORTED If the queue is aborted
		*/
		virtual STFResult WaitMessage(void) = 0;
	};

///
/// @class STFMessageProcessingDispatcher
///
/// @brief A combination of dispatcher and processor
///
/// Dispatcher and processor typically appear as a pair.  The dispatcher providing the
/// writing side and the processor the reading side of the message queue.

class STFMessageProcessingDispatcher : virtual public STFMessageDispatcher, virtual public STFMessageProcessor
	{
	};

///
/// @class TriggeredSTFMessageProcessingDispatcher
///
/// @brief A combination of dispatcher and triggered processor
class TriggeredSTFMessageProcessingDispatcher : virtual public STFMessageProcessingDispatcher, virtual public TriggeredSTFMessageProcessor
	{
	};

//-----------------------------------------------------------------------------

///
/// @class MasterSTFMessageProcessor
///
/// @brief An aggregation of several message processors
///
/// Several message processors can be combined into a single processor, which could
/// then be served by a single thread.  The slave processors each attach themselves
/// to the master processor during their constructor.  All processing methods that
/// are called for the master are delegated to the slaves.  This class is intended
/// to be used with one of the SlaveSTFMessageProcessor classes.

class MasterSTFMessageProcessor : virtual public STFMessageProcessor
	{
	protected:
		/// An array containing all slaves of this master
		STFMessageProcessor ** slaves;

		/// The number of slaves of this master
		int	numSlaves;

		/// The size of the slave array
		int	totalSlaves;

	public:
		MasterSTFMessageProcessor(void);

      ~MasterSTFMessageProcessor(void);

		/// Attach a slave to the master
		/*!
		Attaches a slave STFMessageProcessor to this master.  The master will
		then take care of the processing of the slaves messages.  The slave should
		not be used as a processor on its own.  The only exception is, that it may
		be aborted without aborting the other slaves, thus providing dispatcher
		side abort.
		@param slave The slave to attach
		*/
		STFResult AttachSlave(STFMessageProcessor * slave);

		/// Remove a slave from the master
		/*!
		Removes a slave STFMessageProcessor from the master.  The slave will
		than have to live on its own.
		*/
		STFResult RemoveSlave(STFMessageProcessor * slave);

		STFResult AbortProcessor(void);

		STFResult ProcessMessages(void);

		STFResult FlushMessages(void);

		STFResult FlushMessages(STFMessageSink * sink);

		STFResult CheckQueueState(void);
	};

///
/// @class TriggeredMasterSTFMessageProcessor
///
/// @brief An aggregation of serveal message processors that supports read waits
///
/// This class is intended to be used with one of the TriggeringSlaveSTFMessageProcessor classes. 

class TriggeredMasterSTFMessageProcessor : public MasterSTFMessageProcessor, virtual public TriggeredSTFMessageProcessor, virtual public STFMessageTrigger
	{
	protected:
		/// Trigger that is set on incomming messages
		STFSignal		trigger;

	public:
		TriggeredMasterSTFMessageProcessor(void);
		virtual ~TriggeredMasterSTFMessageProcessor() {}

		STFResult WaitMessage(void);
		
		STFResult TriggerMessageArrival(void);	
	};


//-----------------------------------------------------------------------------
			
///
/// @class BaseSTFMessageDispatcher
///
/// @brief Base implementation of a STFMessageDispatcher
///
/// This base implementation provides support for AbortDispatcher() by using an abort
/// member.  This is set to true, if the dispatcher shall not accept any more messages.

class BaseSTFMessageDispatcher : virtual public STFMessageDispatcher
	{
	protected:
		/// Abort message dispatching
		/*!
		This member is set to true by AbortDispatch().  It will block all incomming
		messages and release potentially waiting threads.
		*/
		volatile bool abort;

	public:
		/// Major constructor
		BaseSTFMessageDispatcher(void)
			{
			abort = false;
			}
		
		/// Abort message dispatch
		/*!
		This method will set the abort member to true and flush all messages. After this
		call, this dispatcher will no longer accept or forward any messages. This should
		typically be done before deleting it.
		*/
		STFResult AbortDispatcher(void) {abort = true; STFRES_RAISE_OK;}
	};

///
/// @class BaseSTFMessageProcessor
///
/// @brief Base implementation of a STFMessageProcessor
///
/// This base implementation provides support for AbortProcessor() by using an abort
/// member. This is set to true, if the processor shall not provide any more messages.

class BaseSTFMessageProcessor : virtual public STFMessageProcessor
	{
	protected:
		/// Abort message dispatching
		/*!
		This member is set to true by AbortProcessor().  It will block all incomming
		messages and release potentially waiting threads.
		*/
		volatile bool abort;

	public:
		/// Major constructor
		BaseSTFMessageProcessor(void)
			{
			abort = false;
			}

		/// Abort message dispatch
		/*!
		This method will set the abort member to true and flush all messages. After this
		call, this dispatcher will no longer accept or forward any messages. This should
		typically be done before deleting it.
		*/
		STFResult AbortProcessor(void) {abort = true; STFRES_RAISE(FlushMessages());}
	};

///
/// @class BaseSTFMessageProcessingDispatcher
///
/// @brief Base implementation of a STFMessageProcessingDispatcher
///
/// This base implementation provides support for AbortDispatcher() and AbortProcessor()
/// by using an abort member. This is set to true, if the dispatcher shall not accept 
/// and the processor not provide any more messages.

class BaseSTFMessageProcessingDispatcher : virtual public STFMessageProcessingDispatcher, public BaseSTFMessageProcessor
	{
	public:
		STFResult AbortDispatcher(void) 
			{
			abort = true; 
			STFRES_RAISE(FlushMessages());
			}
	};

///
/// @class DirectSTFMessageDispatcher
///
/// @brief A STFMessageDispatcher that immediately calls ReceiveMessage()
///
/// This dispatcher will immediately call the ReceiveMessage() method of the calling
/// sink.

class DirectSTFMessageDispatcher	: public BaseSTFMessageDispatcher
	{
	public:
		STFResult DispatchMessage(STFMessageSink * sink, STFMessage message, bool wait);
	};

///
/// @class QueuedSTFMessageProcessingDispatcher
///
/// @brief A STFMessageDispatcher that queues STFMessages
///
/// All messages that are sent to this type of STFMessageDispatcher are put into a queue.
/// They are processed by another thread that calls the ProcessMessages() method. A dispatcher
/// of this type is interrupt save for sending, if only the interrupt uses it, but does not
/// provide wait capabilities for the sender. It is also not save to be used by potentially
/// nesting interrupts or interrupt/thread or thread/thread mixtures. It should therefore
/// only be used for a single sender thread or interrupt.

class QueuedSTFMessageProcessingDispatcher	: public BaseSTFMessageProcessingDispatcher
	{
	protected:
		/// The queue, that holds the pending messages
		struct STFMessageQueue
			{
			STFMessageSink	*	sink;			///<	The target sink of this message
			STFMessage			message;		///<	The message itself
			}	*	queue;

		/// The write pointer of the queue
		int	queueWrite;
		/// The read pointer of the queue
		int	queueRead;
		/// The size of the queue (must be a power of two)
		int	queueSize;
		/// The size mask of the queue (size - 1) used for modulo calculation.
		int	queueMask;

		/// Signal message arrival
		/*!
		Whenever a message arrives through DispatchMessage(), this method will be called.
		It can be used by derived classes to implemt waiting.
		*/
		virtual void Trigger(void) {}

		/// Protects the read side of the queue
		STFMutex	readLock;

	public:
		/// Major constructor
		/*!
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		QueuedSTFMessageProcessingDispatcher(int queueSize);

		virtual ~QueuedSTFMessageProcessingDispatcher(void);

		STFResult DispatchMessage(STFMessageSink * sink, STFMessage message, bool wait);

		STFResult ProcessMessages(void);

		STFResult FlushMessages(void);

		STFResult FlushMessages(STFMessageSink * sink);

		STFResult CheckQueueState(void);
	};

///
/// @class TriggeredQueuedSTFMessageProcessingDispatcher
///
/// @brief A STFMessageDispatcher that queues STFMessages and supports waiting on the reader side
///
/// This STFMessageDispatcher has a queue for the input side, and provides wait capability for the
/// processing thread.

class TriggeredQueuedSTFMessageProcessingDispatcher : public QueuedSTFMessageProcessingDispatcher, virtual public TriggeredSTFMessageProcessingDispatcher
	{
	protected:
		/// Signal, when a message arrives
		STFSignal		trigger;

		/// Sends the signal
		void Trigger(void);

	public:
		/// Major constructor
		/*!
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		TriggeredQueuedSTFMessageProcessingDispatcher(int queueSize);

		virtual ~TriggeredQueuedSTFMessageProcessingDispatcher(void);

		STFResult WaitMessage(void);
	};

///
/// @class SlavedQueuedSTFMessageProcessingDispatcher

class SlavedQueuedSTFMessageProcessingDispatcher : public QueuedSTFMessageProcessingDispatcher
	{
	public:
		/// Major constructor
		/*!
		@param master The master of this slave dispatcher
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		SlavedQueuedSTFMessageProcessingDispatcher(MasterSTFMessageProcessor * master, int queueSize);

		virtual ~SlavedQueuedSTFMessageProcessingDispatcher(void) {};
	};

///
/// @class TriggeringSlavedQueuedSTFMessageProcessingDispatcher

class TriggeringSlavedQueuedSTFMessageProcessingDispatcher : public SlavedQueuedSTFMessageProcessingDispatcher
	{
	protected:
		/// Master that is signaled, when a message arrives
		TriggeredMasterSTFMessageProcessor * 	master;

		/// Sends the signal	to the master
		void Trigger(void);

	public:
		/// Major constructor
		/*!
		@param master The master of this slave dispatcher
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		TriggeringSlavedQueuedSTFMessageProcessingDispatcher(TriggeredMasterSTFMessageProcessor * master, int queueSize);

		virtual ~TriggeringSlavedQueuedSTFMessageProcessingDispatcher(void) {};
	};

///
/// @class WaitableQueuedSTFMessageProcessingDispatcher
/// 
/// @brief A STFMessageDispatcher that queues STFMessages and provides sender wait
///
/// All messages that are sent to this type of STFMessageDispatcher are put into a queue.
/// They are processed by another thread that calls the ProcessMessages() method. A dispatcher
/// of this type is not interrupt save. It is save for multiple threads on the sender side.

class WaitableQueuedSTFMessageProcessingDispatcher	: public BaseSTFMessageProcessingDispatcher
	{
	protected:
		/// The queue, that holds the pending messages
		struct STFMessageQueue
			{
			STFMessageSink	*	sink;			///<	The target sink of this message
			STFMessage			message_;	///<	A copy of the message, if the sender does not wait
			STFMessage		*	message;		///<	A pointer to the message (either &message_ or to the sender stack)
			}	*	queue;

		/// The write pointer of the queue
		int	queueWrite;
		/// The read pointer of the queue
		int	queueRead;
		/// The size of the queue (<b>must be a power of two</b>)
		int	queueSize;
		/// The size mask of the queue (size - 1) used for modulo calculation.
		int	queueMask;

		/// Protects the sender side of the queue
		STFMutex	writeLock;

		/// Protects the processor side of the queue
		STFMutex	readLock;

		/// Called on message arrival
		virtual void Trigger(void) {}

	public:
		/// Major constructor
		/*!
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		WaitableQueuedSTFMessageProcessingDispatcher(int queueSize);

		~WaitableQueuedSTFMessageProcessingDispatcher(void);

		STFResult DispatchMessage(STFMessageSink * sink, STFMessage message, bool wait);

		STFResult ProcessMessages(void);
	
		STFResult FlushMessages(void);

		STFResult FlushMessages(STFMessageSink * sink);

		STFResult CheckQueueState(void);
	};

///
/// @class TriggeredWaitableQueuedSTFMessageProcessingDispatcher
///
/// @brief A STFMessageDispatcher that queues STFMessages and provides sender and reader wait
class TriggeredWaitableQueuedSTFMessageProcessingDispatcher	: public WaitableQueuedSTFMessageProcessingDispatcher, virtual public TriggeredSTFMessageProcessingDispatcher
	{
	protected:
		/// Signal, when a message arrives
		STFSignal		trigger;

		/// Sends the signal
		void Trigger(void);

	public:
		/// Major constructor
		/*!
		@param queueSize The size of the message queue, <b>it must be a power of two</b>
		*/
		TriggeredWaitableQueuedSTFMessageProcessingDispatcher(int queueSize);

		virtual ~TriggeredWaitableQueuedSTFMessageProcessingDispatcher(void) {};

		STFResult WaitMessage(void);
	};


///
/// @class STFMessageProcessorThread
///
/// @brief A thread that processes STFMessages from a STFMessageProcessor
///
/// This STFMessageQueue processes all messages of a given TriggeredSTFMessageProcessor
/// with an own thread.

class STFMessageProcessorThread : virtual public STFMessageQueue, public STFThread
	{
	protected:
		/// The delegate processor
		TriggeredSTFMessageProcessor * processor;

		/// Used for thread termination
		STFResult NotifyThreadTermination(void);

		/// Thread processing loop
		void ThreadEntry(void);
	public:
		/// Major constructor
		/*!
		@param name Name of the processing thread (for debugging purposes)
		@param stackSize Size of the processing threads stack
		@param priority Thread priority for the processing thread
		@param processor The processor that provides the message queue
		*/
		STFMessageProcessorThread(STFString name, int stackSize, STFThreadPriority priority, TriggeredSTFMessageProcessor * processor);

		/// Destructor
		virtual ~STFMessageProcessorThread(void);

		STFResult FlushMessages(void);

		STFResult FlushMessages(STFMessageSink * sink);

		STFResult CheckQueueState(void);
	};

///
/// @class ThreadedSTFMessageDispatcher
///
/// @brief A STFMessageDispatcher that has an own thread for processing messages
///
/// This STFMessageDispatcher forwards all messages to the delegate dispatcher, and
/// processes all messages of the delegate using an own thread.

class ThreadedSTFMessageDispatcher : virtual public STFMessageDispatcher, public STFMessageProcessorThread
	{
	protected:
		/// The delegate dispatcher
		STFMessageProcessingDispatcher * dispatcher;

	public:
		/// Major constructor
		/*!
		@param name Name of the processing thread (for debugging purposes)
		@param stackSize Size of the processing threads stack
		@param priority Thread priority for the processing thread
		@param dispatcher The dispatcher that provides the message queue
		*/
		ThreadedSTFMessageDispatcher(STFString name, int stackSize, STFThreadPriority priority, TriggeredSTFMessageProcessingDispatcher * dispatcher);

		/// Destructor
		virtual ~ThreadedSTFMessageDispatcher(void);

		STFResult AbortDispatcher(void);

		STFResult DispatchMessage(STFMessageSink * sink, STFMessage message, bool wait);
	};



#endif
