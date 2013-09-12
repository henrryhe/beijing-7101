
// FILE:		Device/Source/Unit/DMA/Specific/FDMA/Generic/STFDMAChannel.h
//
// AUTHOR:		Mark Walker
// COPYRIGHT:	(c) 2004 STMicroelectronics.  All Rights Reserved.
// CREATED:		27.07.2004
//
// PURPOSE:		STFDMA driver interface Channel implementation
//
// SCOPE:		INTERNAL Header File

#ifndef STFDMACHANNEL_H
#define STFDMACHANNEL_H

#include "VDR/Source/Unit/PhysicalUnit.h"
#include "VDR/Source/Unit/VirtualUnit.h"
#include "Device/Interface/Unit/DMA/FDMA/IFDMAChannel.h"


// TODO : We need to sort out how this will get picked up
// TODO : Hack to get stfdma.h to parse ok
//#ifndef ST_Partition_t
//#define ST_Partition_t void*
//#endif

#define STFDMA_STCM 1
#include "stfdma.h"

#if 0
#define FDMAFunctionNameDP(Text) DP(Text)
#else
#define FDMAFunctionNameDP(Text)
#endif

#define MAX_CONNECTED_CLIENTS 10
#define BYTE_ALIGNMENT        32

class  VirtualSTFDMAChannel;

typedef struct STFDMA_STCMNodeControlData_s
{
    struct STFDMA_STCMGenericNode_s*  prevNode;
    uint32                            index;
    uint32                            pending;
    uint32                            locked;
    uint32                            cbParam;
    VirtualSTFDMAChannel*             virtualUintPoster;
    uint32                            finalSection;
	  uint32                            pad[1];
} STFDMA_STCMNodeControlData_t;

typedef struct STFDMA_STCMGenericNode_s
{
  STFDMA_GenericNode_t          node;  
  STFDMA_STCMNodeControlData_t  control;
} STFDMA_STCMGenericNode_t;

void GlobalTransferCallback
(
  U32   TransferId,         /* Transfer id */
  U32   CallbackReason,     /* Reason for callback */
  U32*  CurrentNode_p,      /* Node transfering */
  U32   NodeBytesTransfered,/* Number of bytes transfered */
  BOOL  Error,              /* Interrupt missed */
  void* ApplicationData_p   /* Application data - should be a pointer to an STFDMA_Channel object */
);

// TODO : Get a legal one of these
static const VDRIID_STFDMA_CHANNEL  = 0x80808080;

class ISTFDMAChannel : public virtual IFDMAChannel
{
public:
  virtual STFResult RequestTransfer(
    struct STFDMA_NodeData const *  pNodeData, 
    uint32                          cbParam) = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Physical STFDMA Channel
//
///////////////////////////////////////////////////////////////////////////////


class STFDMAChannel :
  public virtual SharedPhysicalUnit, 
  public virtual IFDMAChannelInterrupt
{
  friend class VirtualSTFDMAChannel;

protected:
  STFDMA_STCMGenericNode_t* nodeList;
  volatile uint32           nextAvailableNode;
  volatile uint32           nextNodeToSend;

  STFTimeoutSignal          nodeListChange;
  STFTimeoutSignal          channelStateChange;

  FDMAChannelState          channelState;
  FDMAChannelState          desiredChannelState;

  uint8*                    unalignedNodeBuffer;
  volatile uint32           numBufferedNodes;
  uint32                    numBufferedNodesNeedToStart;
  const uint32              listSize;

  uint32                    numConnectedClients;
  IFDMAChannelClient*       channelClients[MAX_CONNECTED_CLIENTS];
  bool                      forceCompletAllTransfers;

  char*                     instanceName;
  STFDMA_STCMGenericNode_t* currentNode;
  STFDMA_Pool_t             channelPoolType;
  STFDMA_ChannelId_t        channelId;
  STFDMA_TransferId_t       transferId;
  ST_Partition_t*           NCachePartition;

public:
                    STFDMAChannel(VDRUID unitID, 
                        char*           STFDMAInstance, 
                        ST_Partition_t* theNCachePartition,
                        STFDMA_Pool_t   pool, 
                        uint32          ls);
                    ~STFDMAChannel(void);
  virtual STFResult Create(long * createParams);
  virtual STFResult Connect(long localID, IPhysicalUnit * source);
  virtual STFResult Initialize(long* depUnitsParams);
  virtual STFResult	ChannelStopInterrupt();
  virtual STFResult EarlyInterrupt();
  virtual bool      IsRunning() {return (desiredChannelState == CHANNEL_RUNNING);}
  void              TransferCallback
  (
    U32   TransferId,         /* Transfer id */
    U32   CallbackReason,     /* Reason for callback */
    U32*  CurrentNode_p,      /* Node transfering */
    U32   NodeBytesTransfered,/* Number of bytes transfered */
    BOOL  Error
  );
  
  virtual STFResult CreateVirtual(IVirtualUnit*& unit, IVirtualUnit* parent = NULL,  IVirtualUnit* root = NULL);
	
  //virtual STFResult NodeProbe(uint32 index, const char *prefix = NULL);
  //virtual STFResult WorkspaceProbe(const char *prefix = NULL);

protected:
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: AllocateNodeList
  //
  //  Occurs:
  //    1. When the channel is being initialized
  //    2. When the node list must change size on the fly
  //
  //  Preconditions:
  //    1. the existing node list must be destroyed
  //
  //  PostConditions:
  //    1. "unalignedNodeBuffer" is allocated to the size of the entire nodelist plus "BYTE_ALIGNMENT".  Where 
  //      BYTE_ALIGNMENT is the alignment required for the nodes.  The node list is then created inside of 
  //      "unalignedNodeBuffer" at an aligned boundry.
  //
  //  Returns:
  //    1. STFRES_NOT_ENOUGH_MEMORY if the node list was not allocated
  //    2. Return Dependencies: DestroyNodeList(), ResetNode(), LockNodeList()
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult AllocateNodeList();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: DestroyNodeList
  //
  //  Occurs:
  //    1. When the channel is being destroyed
  //    2. When the node list must change size on the fly
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    1. The node list is destroyed.  "unalignedNodeBuffer" is set to NULL
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult DestroyNodeList();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: FlushNodeList
  //
  //  Occurs:
  //    1. When the client needs to remove all pending transfers
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    1. All nodes in the list are emptied.
  //
  //  Returns:
  //    1. Return Dependencies: ResetNode()
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult FlushNodeList();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: GetStartingNode
  //
  //  Occurs:
  //    1. When the channel is starting, to determine the node to be placed into the channel workspace
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    1. The address of the node to program is written inside of "node"
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult GetStartingNode(STFDMA_STCMGenericNode_t*& node);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: LockNodeList
  //
  //  Occurs:
  //    1. Internally when no new transfers are to be accepted (locked = true)
  //    2. Internally when new transfers are to be accepted (locked = false)
  //
  //  Preconditions:
  //    None.
  //
  //  PostConditions:
  //    1. All nodes are either placed into or removed from a locked state.  While locked, the nodes cannot be modified
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult LockNodeList(bool locked);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: PauseChannel
  //
  //  Occurs:
  //    1. Put the channel into a paused state
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    1. The "desiredChannelState" is changed to CHANNEL_PAUSED
  //
  //  Returns:
  //    1. Return Dependencies: LockNodeList(),
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult PauseChannel();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: PopulateNode
  //
  //  Occurs:
  //    1. When a new transfer is request and there next node on the list is no longer pending
  //
  //  Preconditions:
  //    1. A defined node list which has a size of 2^n
  //    2. "nextAvailableNode" is initialize to the node index to be populated
  //
  //
  //  PostConditions:
  //    1. All necessary elements of the node have been populated and the node is ready to be used by the FDMA.
  //    2. All channel specfic control bits to be set in the nodes "ctrl" field should be done in a subclass of 
  //      this function.
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult PopulateNode(struct STFDMA_NodeData const * pNodeData, VirtualSTFDMAChannel* virtualUintPoster = NULL, uint32 cbParam = 0, bool finalSection = true);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: PreformClientSignalStateChange
  //
  //  Occurs:
  //    1. When the channels state has changed
  //
  //  Preconditions:
  //    1. All clients that wish to recieve messages for generic channel events (such as channel stop) must register with 
  //      RegisterClientSignalInterface()
  //
  //  PostConditions:
  //    1.	All call will be made to each channel client informing it of the state change
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult PreformClientSignalStateChange();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: ProgramNode
  //
  //  Occurs:
  //    1. When the channel is starting 
  //
  //  Preconditions:
  //  None
  //
  //  PostConditions:
  //    1. Programs the given node into the channel workspace in the FDMA's DMEM
  //
  //  Returns:
  //    1. STFRES_INVALID_PARAMETERS if programNode is NULL
  //    2. Return Dependencies: IMemoryMappedIO::WriteDWord()
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult ProgramNode(STFDMA_STCMGenericNode_t* programNode);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: RegisterClientSignalInterface
  //
  //  Occurs:
  //    1. Immediatly after a client has registered with the VirtualFDMAChannel
  //
  //  Preconditions:
  //    1. Existing VirtualFDMAChannel with a registered client.  
  //
  //  PostConditions:
  //    1. The client of the VirtualFDMAChannel will be registered with the physical FDMAChannel.  This allow the 
  //      physical channel to send notification to all clients for non specific channel events such as the channel stop
  //
  //  Returns:
  //    1. STFRES_INVALID_PARAMETERS if "clientCallback" is NULL
  //    2. STFRES_OBJECT_ALREADY_JOINED if "clientCallback" has already registered
  //    3. Return Dependencies: None
  //    4. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult RegisterClientSignalInterface(IFDMAChannelClient* clientCallback);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: RemoveClientSignalInterface
  //
  //  Occurs:
  //    1. When a client needs to detatch from its VirtualFDMAChannel.  Most commomly this will be a result of the 
  //      destruction of the VirtualFDMAChannel.
  //
  //  Preconditions:
  //    1. Existing VirtualFDMAChannel with a registered client.  
  //
  //  PostConditions:
  //    1. The client will be removed from the list of clients the channel needs to signal.
  //
  //  Returns:
  //    1. STFRES_INVALID_PARAMETERS if "clientCallback" is NULL
  //    2. STFRES_OBJECT_NOT_FOUND if "clientCallback" has not been registered
  //    3. Return Dependencies: None
  //    4. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult RemoveClientSignalInterface(IFDMAChannelClient* clientCallback);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: ResetChannel
  //
  //  Occurs:
  //    1. When a new stream is started
  //    2. When an serious interanal error occurs 
  //
  //  Preconditions:
  //    None.
  //
  //  PostConditions:
  //    1. The node list is empty
  //    2. The node list is unlocked
  //    3. The channel status has been reset
  //
  //  Returns:
  //    1. Return Dependencies: FlushNodeList(), ResetChannelSpecific(), IMemoryMappedIO::WriteDWord()
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult ResetChannel();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: ResetChannelSpecific
  //
  //  Occurs:
  //    1. When the channel is reset
  //    2. When the channel specific registers need to be reprogrammed
  //
  //  Preconditions:
  //    None.
  //
  //  PostConditions:
  //    1. The base should do no work in here it is provided for the derived channels to program their specific registers
  //      without having to reset the entire channel
  //
  //  Returns:
  //    1. Return Dependencies: None
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult ResetChannelSpecific();

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: ResetNode
  //
  //  Occurs:
  //    1. After a node has been serviced by the FDMA
  //    2. When the information inside the node is no longer necessary
  //
  //  Preconditions:
  //    None.
  //
  //  PostConditions:
  //    1. All elemets of the node are reset to the default vales
  //
  //  Returns:
  //    1. STFRES_INVALID_PARAMETERS if node is NULL
  //    2. Return Dependencies: None
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult ResetNode(STFDMA_STCMGenericNode_t* node);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: StartChannel
  //
  //  Occurs:
  //    1. After the number of transfers pending is greater then the required number to start the channel
  //
  //  Preconditions:
  //    1. None
  //
  //  PostConditions:
  //    1. The list's starting node is programmed into DMEM
  //    2. The mailbox bit for this channel is cleared (the channel is started)
  //
  //  Returns:
  //    1. STFRES_UNIT_INACTIVE if the FDMA firmware is not loaded
  //    2. STFRES_OPERATION_PENDING if the channel's state could not be changed
  //    3. STFRES_OPERATION_FAILED if the was a general starting problem
  //    4. Return Dependencies: IFDMAControl::IsFDMAActive(), VerifyStartNecessary()
  //    5. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult StartChannel();

  virtual STFResult SetChannelState(FDMAChannelState newState);

  virtual STFResult SetDesiredChannelState(FDMAChannelState newState);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: SignalNodeCompleted
  //
  //  Occurs:
  //    1. When an interrupt is received informing that "completedNode" has been serviced
  //    2. When there is sufficient information to conclude node X has been serviced
  //
  //  Preconditions:
  //    1. "completedNode" must be a node on the node list or STFRES_RANGE_VIOLATION will be returned.
  //
  //  PostConditions:
  //    1.	All nodes upto and including "completedNode" are marked as not pending, a callback is made to the 
  //      VirtualFDMAChannel that requested each transfer, and then each node is emptied.
  //
  //  Returns:
  //    1. STFRES_RANGE_VIOLATION if completedNode is within the bounds of the node list
  //    2. Return Dependencies: ResetNode()
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult SignalNodeCompleted(STFDMA_STCMGenericNode_t* completedNode);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: StopChannel
  //
  //  Occurs:
  //    1. When a running channel needs to stop
  //
  //  Preconditions:
  //    None.
  //
  //  PostConditions:
  //    1. "desiredChannelState" becomes CHANNEL_STOPPED and "channelState" will become CHANNEL_STOPPED after the channel
  //      stopes.
  //    2. The nodelist is broken to allow the FDMA to stop as soon as possibile
  //    3. The node list is emptied.  All pending transfers the FDMA has not serviced are lost.
  //
  //  Returns:
  //    1. Return Dependencies: LockNodeList(),
  //    2. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult StopChannel();


  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: Transfer
  //
  //  Occurs:
  //    1. When a virtual unit needs to post a transfer to the physical channel
  //
  //  Preconditions:
  //    1. If the list is full STFRES_OBJECT_FULL is returned
  //
  //  PostConditions:
  //    1. The next node on the list is populated with the given parameters
  //    2. If the channel has a start pending and the number of buffered transfers is larger than "numBufferedNodesNeedToStart"
  //      a call is made to StartTransfer()
  //
  //  Returns:
  //    1. STFRES_OBJECT_FULL if the nodeList is in use
  //    2. Return Dependencies: VerifyAvailableNodes(), PopulateNode(), 
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult Transfer(struct STFDMA_NodeData const * pNodeData, VirtualSTFDMAChannel* virtualUintPoster = NULL, uint32 cbParam = 0, bool finalSection = true);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: VerifyAvailableNodes
  //
  //  Occurs:
  //    1. When a new transfer is requested
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    None
  //
  //  Returns:
  //    1. STFRES_OBJECT_FULL if there are not "numEmptyNodes" free in the node list
  //    2. Return Dependencies: None
  //    3. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult VerifyAvailableNodes(uint32 numEmptyNodes = 1);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  Function: VerifyStartNecessary
  //
  //  Occurs:
  //    1. Durring a call to StartChannel.
  //    2. Any time the channel start condition needs to be verified. 
  //
  //  Preconditions:
  //    None
  //
  //  PostConditions:
  //    1. STFRES_OPERATION_PROHIBITED if the channel does not need to be started
  //    2. STFRES_OPERATION_PENDING if the channel needs to be started, but not at this time
  //    3. Return Dependencies: None
  //    4. STFRES_OK on success
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  virtual STFResult VerifyStartNecessary();
};

typedef enum _STFDMA_NodeDataType
{
  STFDMA_NDTGeneral,
  STFDMA_NDTSPDIF
}STFDMA_NodeDataType;

struct STFDMA_NodeData
{
  STFDMA_NodeDataType Type;
  void*               SourceAddress;
  void*               DestinationAddress;
  bool                NodeCompleteNotify;
  bool                NodeCompletePause;
  uint32              NumberBytes;
};

struct STFDMA_GeneralNodeData : public STFDMA_NodeData
{
  uint32  PaceSignal;
  uint32  Length;
  uint8   SourceDirection;
  uint8   DestinationDirection;
  int32   SourceStride;
  int32   DestinationStride;
};

struct STFDMA_SPDIFNodeData : public STFDMA_NodeData
{
  bool    BurstEnd;
  uint16  PreambleA;
  uint16  PreambleB;
  uint16  PreambleC;
  uint16  PreambleD;
  uint32  BurstPeriod;
  uint32  Channel0Status0;
  uint32  Channel0Status1;
  bool    Channel0Valid;
  bool    Channel0UserStatus;
  uint32  Channel1Status0;
  uint32  Channel1Status1;
  bool    Channel1Valid;
  bool    Channel1UserStatus;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Virtual STFDMA Channel
//
///////////////////////////////////////////////////////////////////////////////

class VirtualSTFDMAChannel :
  public virtual VirtualUnit, 
  public virtual ISTFDMAChannel
{
private:

protected:
  bool                nodeDivisionEnabled;
  uint32              nodeDivisionNumNodes;
  uint32              lastNodeSectionAdded;

  STFDMAChannel*      physicalChannel;
  IFDMAChannelClient* clientCallback;

protected:
  // If cbParam is not provided, a callback will not be generated
  virtual STFResult RequestTransfer(struct STFDMA_NodeData const * pNodeData, uint32 cbParam);
  virtual STFResult EnableNodeDivision(uint32 nodesToDivideTransferInto) { return STFRES_UNIMPLEMENTED;}

public:
  VirtualSTFDMAChannel(STFDMAChannel* physicalChannel);
  ~VirtualSTFDMAChannel();

  virtual STFResult Initialize(void);

  //ITagUnit interface implementation
  virtual STFResult InternalUpdate(void);
  virtual STFResult RegisterClientSignalInterface(IFDMAChannelClient* clientCallback);
  virtual STFResult StartTransfer(uint32 afterBufferedTransfers = 1);
  virtual STFResult ResumeTransfer();
  virtual STFResult SetRequiredBufferedTransfersBeforeStart(uint32 afterBufferedTransfers);
  virtual STFResult StopTransfer(void);
  virtual STFResult PauseTransfer(void);
  virtual STFResult ForceCompleteAllTransfers(void) { return STFRES_UNIMPLEMENTED;}

  virtual bool IsRunning();
  virtual FDMAChannelState GetChannelState() {return physicalChannel->desiredChannelState;}

  virtual STFResult QueryTimingModel(FDMATimingModel timingModel){ return STFRES_UNIMPLEMENTED;}
  virtual STFResult QueryDataAlignment(bool sourceDataUnitsAligned, bool destinationDataUnitsAligned){ return STFRES_UNIMPLEMENTED;}
  virtual STFResult QueryDataUnits(FDMADataUnit sourceDataUnit, FDMADataUnit destinationDataUnit){ return STFRES_UNIMPLEMENTED;}
  virtual STFResult QueryDataStructures(FDMADataStructure sorceDataStructure, FDMADataStructure destinationDataStructure){ return STFRES_UNIMPLEMENTED;}
  virtual STFResult QueryInterface(VDRIID iid, void*& ifp);
  virtual STFResult PerformCallback(STFDMA_STCMGenericNode_t* callbackNode);
};

#endif  //STFDMACHANNEL_H
