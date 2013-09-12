
// FILE:		Device/Source/Unit/DMA/Specific/FDMA/Generic/STFDMAChannel.cpp
//
// AUTHOR:		Mark Walker
// COPYRIGHT:	(c) 2004 STMicroelectronics.  All Rights Reserved.
// CREATED:		27.07.2003
//
// PURPOSE:		STFDMA driver interface Channel implementation
//
// SCOPE:		INTERNAL Source File

#include "STFDMAChannel.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Physical STFDMA Channel
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// STFDMAChannel public functions
///////////////////////////////////////////////////////////////////////////////

STFDMAChannel::STFDMAChannel(VDRUID unitID, 
    char*           STFDMAInstance, 
    ST_Partition_t* theNCachePartition,
    STFDMA_Pool_t   pool, 
    uint32          ls) : 
  SharedPhysicalUnit(unitID),
  instanceName(STFDMAInstance),
  NCachePartition(theNCachePartition),
  channelPoolType(pool),
  listSize(ls)
{
  uint32 i;

  FDMAFunctionNameDP("STFDMAChannel::STFDMAChannel()\n");

  nextAvailableNode = 0;
  nextNodeToSend = 0;
  numBufferedNodes = 0;
  numConnectedClients = 0;
  unalignedNodeBuffer = NULL;
  currentNode = NULL;
  nodeList = NULL;
  forceCompletAllTransfers = false;
  nodeListChange.ResetSignal();
  nodeListChange.SetSignal();
  channelStateChange.ResetSignal();
  channelStateChange.SetSignal();
  channelState = CHANNEL_STOPPED;
  desiredChannelState = CHANNEL_STOPPED;

  for (i = 0; i < MAX_CONNECTED_CLIENTS; i ++)
		channelClients[i] = NULL;	
}

STFDMAChannel::~STFDMAChannel(void)
{
  FDMAFunctionNameDP("STFDMAChannel::~STFDMAChannel()\n");

  STFDMA_UnlockChannel(channelId);
  DestroyNodeList();
}

STFResult STFDMAChannel::ChannelStopInterrupt()
{
  STFResult err = STFRES_OK;

  STFRES_REASSERT(SetChannelState(CHANNEL_STOPPED));
  STFRES_REASSERT(SignalNodeCompleted(currentNode));

  // if the list ran empty we will need to reprogram the channel
  // NOTE: set the desired state first.  This prevents unnecessary restarting because
  // the channel is stopped but the desired state is not yet updated.
  switch (desiredChannelState)
  {
  case CHANNEL_RESTARTING:
  case CHANNEL_RUNNING:
    STFRES_REASSERT(SetDesiredChannelState(CHANNEL_RESTARTING));
    STFRES_REASSERT(LockNodeList(false));
    StartChannel();
  break;

  case CHANNEL_STOPPED:
    STFRES_REASSERT(FlushNodeList());
  break;

  case CHANNEL_PAUSED:
  break;
  }

  if (err)
    err = STFRES_OPERATION_FAILED;

  STFRES_RAISE(err);
}

STFResult STFDMAChannel::Connect(long localID, IPhysicalUnit * source)
{
  FDMAFunctionNameDP("STFDMAChannel::Connect()\n");

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::Create(long * createParams)
{
  FDMAFunctionNameDP("STFDMAChannel::Create()\n");

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::CreateVirtual(IVirtualUnit*& unit, IVirtualUnit* parent,  IVirtualUnit* root)
{
    FDMAFunctionNameDP("STFDMAChannel::CreateVirtual()\n");
    
    unit = (IVirtualUnit*)(new VirtualSTFDMAChannel(this));
    
    if (unit)
    {
        STFRES_REASSERT(unit->Connect(parent, root));
    }
    else
    {
        STFRES_RAISE(STFRES_NOT_ENOUGH_MEMORY);
    }
    
    STFRES_RAISE_OK;
}

STFResult STFDMAChannel::EarlyInterrupt()
{
  FDMAFunctionNameDP("STFDMAChannel::EarlyInterrupt()\n");

  STFRES_REASSERT(SignalNodeCompleted(currentNode->control.prevNode));

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::Initialize(long * depUnitsParams)
{
  STFResult result = STFRES_OK;
  FDMAFunctionNameDP("STFDMAChannel::Initialize()\n");

  STFRES_REASSERT(AllocateNodeList());

  if (ST_NO_ERROR != STFDMA_LockChannelInPool(channelPoolType, &channelId))
  {
    result = STFRES_OPERATION_FAILED;
  }

  STFRES_RAISE(result);
}

void STFDMAChannel::TransferCallback
(
  U32   TransferId,         /* Transfer id */
  U32   CallbackReason,     /* Reason for callback */
  U32*  CurrentNode_p,      /* Node transfering */
  U32   NodeBytesTransfered,/* Number of bytes transfered */
  BOOL  Error
)
{
  if (TransferId == transferId)
  {
    currentNode = (STFDMA_STCMGenericNode_t*)CurrentNode_p;
    switch (CallbackReason)
    {
    case STFDMA_NOTIFY_TRANSFER_COMPLETE:
    case STFDMA_NOTIFY_TRANSFER_ABORTED:
      ChannelStopInterrupt();
      break;
    case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
      EarlyInterrupt();
      break;
    default:
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// STFDMAChannel protected functions
///////////////////////////////////////////////////////////////////////////////

STFResult STFDMAChannel::AllocateNodeList()
{
  FDMAFunctionNameDP("STFDMAChannel::AllocateNodeList()\n");

  if (nodeList != NULL)
    STFRES_REASSERT(DestroyNodeList());

  nextAvailableNode = 0;
  if (listSize != 0)
  {
    uint8*  bsPtr;
    uint32  i;

    // create a byte aligned buffer to put the node list in
    unalignedNodeBuffer = (U8*)memory_allocate_clear(NCachePartition,
        1,
        sizeof(STFDMA_STCMGenericNode_t) * listSize + BYTE_ALIGNMENT);

    if (unalignedNodeBuffer == NULL)
      STFRES_RAISE(STFRES_NOT_ENOUGH_MEMORY);

    bsPtr = (uint8*)((((uint32)unalignedNodeBuffer) + BYTE_ALIGNMENT) -  ((uint32)unalignedNodeBuffer % BYTE_ALIGNMENT));
    nodeList = (STFDMA_STCMGenericNode_t*)bsPtr;

    for (i = 0; i != listSize - 1; i ++)
    {
      //nodeList[i].nextNode = &(nodeList[i + 1]);
      nodeList[i].control.index = i;
      STFRES_REASSERT(ResetNode(&(nodeList[i])));
    }
    //nodeList[i].nextNode = &(nodeList[0]);
    nodeList[i].control.index = i;
    STFRES_REASSERT(ResetNode(&(nodeList[i])));
    STFRES_REASSERT(LockNodeList(false));
  }

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::DestroyNodeList()
{
  FDMAFunctionNameDP("STFDMAChannel::DestroyNodeList()\n");

  if (unalignedNodeBuffer != NULL)
    memory_deallocate(NCachePartition, unalignedNodeBuffer);

  unalignedNodeBuffer = NULL;
  nodeList = NULL;
  nextNodeToSend = 0;

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::FlushNodeList()
{
  if (nodeListChange.WaitTimeout(STFLoPrec32BitDuration(50)) == STFRES_TIMEOUT)
    return STFRES_OBJECT_FULL;

  for (uint32 i = 0; i < listSize; i ++)
    STFRES_REASSERT(ResetNode(&(nodeList[i])));

  nextAvailableNode = 0;
  nextNodeToSend = 0;
  numBufferedNodes = 0;
  
  nodeListChange.SetSignal();
  
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::GetStartingNode(STFDMA_STCMGenericNode_t*& node)
{
  node = &nodeList[nextNodeToSend];

  // since this is the first node to send, there is no previous node
  nodeList[nextNodeToSend].control.prevNode = NULL;

  // Make sure that the previous node does not point to it
  nodeList[(node->control.index + listSize - 1) % listSize].node.Gen.Next_p = NULL;
  
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::LockNodeList(bool locked)
{
  // work backwards from the last node put on the list breaking every link.  this will allow us to insure the FDMA reaches a NULL
  // pointer.
  for(uint32 i = 0; i != listSize; i++)
    nodeList[i].control.locked = locked;

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::PauseChannel()
{
  // lock the list to prevent any new transfer requests
  LockNodeList(true);
  SetDesiredChannelState(CHANNEL_PAUSED);
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::PopulateNode(struct STFDMA_NodeData const *  pNodeData,
                                      VirtualSTFDMAChannel*           virtualUintPoster,
                                      uint32                          cbParam,
                                      bool                            finalSection)
{
  STFResult err = STFRES_OK;
  
  FDMAFunctionNameDP("STFDMAChannel::PopulateNode()\n");

  switch (pNodeData->Type)
  {
  case STFDMA_NDTGeneral:
    {
      struct STFDMA_GeneralNodeData* pGenNodeData = (struct STFDMA_GeneralNodeData*)pNodeData;
      STFDMA_Node_t* pNode = &(nodeList[nextAvailableNode].node.Node);

      pNode->NodeControl.PaceSignal = (pGenNodeData->PaceSignal & 0x1F);
      pNode->NodeControl.SourceDirection = (pGenNodeData->SourceDirection & 0x3);
      pNode->NodeControl.DestinationDirection = (pGenNodeData->DestinationDirection & 0x3);
      pNode->NodeControl.Reserved = 0;
      
      if (pGenNodeData->NodeCompletePause)
        pNode->NodeControl.NodeCompletePause = 1;

      if (pGenNodeData->NodeCompleteNotify)
        pNode->NodeControl.NodeCompleteNotify = 1;

      pNode->NumberBytes = pGenNodeData->NumberBytes;
      pNode->SourceAddress_p = pGenNodeData->SourceAddress;
      pNode->DestinationAddress_p = pGenNodeData->DestinationAddress;
      pNode->Length = pGenNodeData->Length;
      pNode->SourceStride = pGenNodeData->SourceStride;
      pNode->DestinationStride = pGenNodeData->DestinationStride;
    }
    break;
  case STFDMA_NDTSPDIF:
    {
      struct STFDMA_SPDIFNodeData* pSPDIFNodeData = (struct STFDMA_SPDIFNodeData*)pNodeData;
      STFDMA_SPDIFNode_t* pNode = &(nodeList[nextAvailableNode].node.SPDIFNode);

      pNode->Extended = STFDMA_EXTENDED_NODE;
      pNode->Type = STFDMA_EXT_NODE_SPDIF;
      pNode->DReq = STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO;
      pNode->BurstEnd = pSPDIFNodeData->BurstEnd;
      pNode->Valid = 1;

      if (pSPDIFNodeData->NodeCompletePause)
        pNode->NodeCompletePause = 1;

      if (pSPDIFNodeData->NodeCompleteNotify)
        pNode->NodeCompleteNotify = 1;

      pNode->NumberBytes = pSPDIFNodeData->NumberBytes;
      pNode->SourceAddress_p = pSPDIFNodeData->SourceAddress;
      pNode->DestinationAddress_p = pSPDIFNodeData->DestinationAddress;

      pNode->PreambleB = pSPDIFNodeData->PreambleB;
      pNode->PreambleA = pSPDIFNodeData->PreambleA; 
      pNode->PreambleD = pSPDIFNodeData->PreambleD;
      pNode->PreambleC = pSPDIFNodeData->PreambleC;
      pNode->BurstPeriod = pSPDIFNodeData->BurstPeriod;
      pNode->Channel0.Status_0 = pSPDIFNodeData->Channel0Status0;
      pNode->Channel0.Status_1 = pSPDIFNodeData->Channel0Status1;
      pNode->Channel0.Valid = pSPDIFNodeData->Channel0Valid;
      pNode->Channel0.UserStatus = pSPDIFNodeData->Channel0UserStatus;
      pNode->Channel1.Status_0 = pSPDIFNodeData->Channel1Status0;
      pNode->Channel1.Status_1 = pSPDIFNodeData->Channel1Status1;
      pNode->Channel1.Valid = pSPDIFNodeData->Channel1Valid;
      pNode->Channel1.UserStatus = pSPDIFNodeData->Channel1UserStatus;
    }
    break;
  default:
    err = STFRES_INVALID_PARAMETERS;
    break;
  }

  nodeList[nextAvailableNode].control.pending = true;
  nodeList[nextAvailableNode].control.cbParam = cbParam;  // this value is passed back on an interrupt
  nodeList[nextAvailableNode].control.virtualUintPoster = virtualUintPoster;
  nodeList[nextAvailableNode].control.finalSection = finalSection;
  nodeList[nextAvailableNode].control.prevNode = &(nodeList[(nextAvailableNode + listSize - 1) % listSize]);
  nodeList[(nextAvailableNode + listSize - 1) % listSize].node.Gen.Next_p = &(nodeList[nextAvailableNode].node); //do this last so we know the node is valid before the FDMA takes it

  STFRES_RAISE(err);
}

STFResult STFDMAChannel::PreformClientSignalStateChange()
{
  for (uint32 i = 0; i < numConnectedClients; i ++)
  {
    if (channelState == CHANNEL_STOPPED)
      channelClients[i]->SignalChannelStop();
     else
      channelClients[i]->SignalChannelStateChange(channelState);
  }

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::ProgramNode(STFDMA_STCMGenericNode_t* programNode)
{
  if (programNode == NULL)
    return STFRES_INVALID_PARAMETERS;

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::RegisterClientSignalInterface(IFDMAChannelClient* clientCallback)
{
  FDMAFunctionNameDP("FDMAChannel::RegisterClientSignalInterface()\n");

  uint32 i;

  if (clientCallback == NULL)
    return STFRES_INVALID_PARAMETERS;

  for (i = 0; i < numConnectedClients; i ++)
    if (channelClients[i] == clientCallback)
      return STFRES_OBJECT_ALREADY_JOINED;

  channelClients[numConnectedClients] = clientCallback;
  numConnectedClients++;

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::RemoveClientSignalInterface(IFDMAChannelClient* clientCallback)
{
  FDMAFunctionNameDP("STFDMAChannel::RemoveClientSignalInterface()\n");

  if (clientCallback == NULL)
    return STFRES_INVALID_PARAMETERS;

  uint32 i;
  for (i = 0; i < numConnectedClients; i ++)
    if (channelClients[i] == clientCallback)
      break;

  if (i < numConnectedClients)
  {
    channelClients[i] = channelClients[numConnectedClients - 1];
    numConnectedClients--;
  }
  else
    return STFRES_OBJECT_NOT_FOUND;

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::ResetChannel()
{
  FDMAFunctionNameDP("STFDMAChannel::ResetChannel()\n");

  STFDMA_AbortTransfer(transferId);
  STFRES_REASSERT(LockNodeList(false));
  STFRES_REASSERT(FlushNodeList());
  STFRES_REASSERT(ResetChannelSpecific());
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::ResetChannelSpecific()
{
  FDMAFunctionNameDP("STFDMAChannel::ResetChannelSpecific()\n");
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::ResetNode(STFDMA_STCMGenericNode_t* node)
{
  if (node == NULL)
    return STFRES_INVALID_PARAMETERS;

  node->control.virtualUintPoster = NULL;
  node->control.finalSection = true;
  node->control.pending = false;

  switch(channelPoolType)
  {
  case STFDMA_SPDIF_POOL:
    {
      STFDMA_SPDIFNode_t* pNode = (STFDMA_SPDIFNode_t*)&(node->node);
      pNode->Next_p = NULL;
      pNode->Extended = 0;
      pNode->Type = 0;
      pNode->DReq = 0;
      pNode->BurstEnd = 0;
      pNode->Valid = 0;
      pNode->NodeCompletePause = 0;
      pNode->NodeCompleteNotify = 0;
      pNode->SourceAddress_p = NULL;
      pNode->DestinationAddress_p = NULL;
      pNode->PreambleA = 0;
      pNode->PreambleB = 0;
      pNode->PreambleC = 0;
      pNode->PreambleD = 0;
      pNode->BurstPeriod = 0;
      pNode->Channel0.Status_0 = 0;
      pNode->Channel0.Status_1 = 0;
      pNode->Channel0.UserStatus = 0;
      pNode->Channel0.Valid = 0;
      pNode->Channel1.Status_0 = 0;
      pNode->Channel1.Status_1 = 0;
      pNode->Channel1.UserStatus = 0;
      pNode->Channel1.Valid = 0;
    }
    break;
  case STFDMA_PES_POOL:
    {
      STFDMA_ContextNode_t* pNode = (STFDMA_ContextNode_t*)&(node->node);
      pNode->Next_p = NULL;
      pNode->Extended = 0;
      pNode->Type = 0;
      pNode->Context = 0;
      pNode->Tag = 0;
      pNode->NodeCompletePause = 0;
      pNode->NodeCompleteNotify = 0;
      pNode->NumberBytes = 0;
      pNode->SourceAddress_p = NULL;
    }
    break;
  default:
    {
      STFDMA_Node_t* pNode = (STFDMA_Node_t*)&(node->node);
      pNode->Next_p = NULL;
      pNode->NodeControl.PaceSignal = 0;
      pNode->NodeControl.SourceDirection = 0;
      pNode->NodeControl.DestinationDirection = 0;
      pNode->NodeControl.NodeCompletePause = 0;
      pNode->NodeControl.NodeCompleteNotify = 0;
      pNode->NumberBytes = 0;
      pNode->SourceAddress_p = NULL;
      pNode->DestinationAddress_p = NULL;
      pNode->Length = 0;
      pNode->SourceStride = 0;
      pNode->DestinationStride = 0;
    }
    break;
  }

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::SetChannelState(FDMAChannelState newState)
{
  if (channelState != newState)
  {
    channelState = newState;
    PreformClientSignalStateChange();
  }

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::SetDesiredChannelState(FDMAChannelState newState)
{
  desiredChannelState = newState;
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::SignalNodeCompleted(STFDMA_STCMGenericNode_t* completedNode)
{
  uint32 lastNode;
  
  if ((completedNode < nodeList) || (completedNode > nodeList + sizeof(STFDMA_STCMGenericNode_t) * listSize))
  {
    BREAKPOINT;
    return STFRES_RANGE_VIOLATION;
  }

  if (completedNode->control.virtualUintPoster != NULL)
  {
    //DP("Cb %d (%d)\n", nodeList[nextNodeToSend].index, channelBase);
    completedNode->control.virtualUintPoster->PerformCallback(completedNode);
  }

  if (nodeListChange.WaitTimeout(STFLoPrec32BitDuration(50)) == STFRES_TIMEOUT)
    return STFRES_OBJECT_FULL;

  do
  {
    if (nodeList[nextNodeToSend].control.pending)
    {
      numBufferedNodes--;
      STFRES_REASSERT(ResetNode(&(nodeList[nextNodeToSend])));
    }
    else
      BREAKPOINT;
    
    lastNode = nextNodeToSend;
    nextNodeToSend = (nextNodeToSend + 1) % listSize;
  }
  while(&(nodeList[lastNode]) != completedNode);

  nodeListChange.SetSignal();

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::StartChannel()
{
  FDMAFunctionNameDP("STFDMAChannel::StartChannel()\n");

  STFResult                       err         = STFRES_OK;
  STFDMA_STCMGenericNode_t*       programNode = NULL;
  STFDMA_TransferGenericParams_t  transferP;

  STFRES_REASSERT(VerifyStartNecessary());

  if (channelStateChange.WaitTimeout(STFLoPrec32BitDuration(50)) == STFRES_TIMEOUT)
    return STFRES_OPERATION_PENDING;

  switch (desiredChannelState)
  {
  case CHANNEL_RESTARTING:
    err |= GetStartingNode(programNode);
    err |= ProgramNode(programNode);
    err |= SetDesiredChannelState(CHANNEL_RUNNING);
    err |= SetChannelState(CHANNEL_RESTARTING);

    transferP.ChannelId = channelId;
    transferP.Pool = channelPoolType;
    transferP.NodeAddress_p = &(programNode->node);
    transferP.BlockingTimeout = 0;
    transferP.CallbackFunction = GlobalTransferCallback;
    transferP.ApplicationData_p = this;

    if (ST_NO_ERROR != STFDMA_StartGenericTransfer(&transferP, &transferId))
    {
       err |= STFRES_OPERATION_FAILED;
    }

  case CHANNEL_RUNNING:
    err |= SetChannelState(CHANNEL_RUNNING);
  break;

  case CHANNEL_STOPPED:
  break;

  case CHANNEL_PAUSED:
  break;

  default:
    DP("unknown desired channel state!\n"); 
  }

  channelStateChange.SetSignal();

  if (err)
    err = STFRES_OPERATION_FAILED;

  STFRES_RAISE(err);
}

STFResult STFDMAChannel::StopChannel()
{
  STFDMA_STCMGenericNode_t* node;
  
  // lock the list to prevent any new transfer requests
  
  LockNodeList(true);

  // Break all the next pointers to ensure that FDMA reaches a NULL pointer.
  
  for (int i = 0;i < listSize;i++)
  {
      node = &(nodeList[i]);
      node->node.Gen.Next_p = NULL;
  }

  SetDesiredChannelState(CHANNEL_STOPPED);
  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::Transfer(struct STFDMA_NodeData const *  pNodeData,
                                  VirtualSTFDMAChannel*           virtualUintPoster,
                                  uint32                          cbParam,
                                  bool                            finalSection)
{
  STFResult ret = STFRES_OK;
  
  FDMAFunctionNameDP("STFDMAChannel::Transfer()\n");

  if (nodeListChange.WaitTimeout(STFLoPrec32BitDuration(50)) == STFRES_TIMEOUT)
    return STFRES_OBJECT_FULL;

  // always do this update befoe putting into the list, make sure the list is not full
  ret = VerifyAvailableNodes();
  if (STFRES_IS_OK(ret))
  {
    //		DP("inserted source 0x%x node %d (%d)\n", source, nextAvailableNode,channelBase);
    ret = PopulateNode(pNodeData, virtualUintPoster, cbParam, finalSection);
    nextAvailableNode = (nextAvailableNode + 1) % listSize;
    numBufferedNodes++;
  }

  nodeListChange.SetSignal();

  StartChannel();
  STFRES_RAISE(ret);
}

STFResult STFDMAChannel::VerifyAvailableNodes(uint32 numEmptyNodes)
{
  for(uint32 i = 0; i < numEmptyNodes; i++)
  {
    if ((nodeList[(nextAvailableNode + i) % listSize].control.pending) || 
      (nodeList[(nextAvailableNode + i) % listSize].control.locked))
    {
      return STFRES_OBJECT_FULL;
    }
  }

  STFRES_RAISE_OK;
}

STFResult STFDMAChannel::VerifyStartNecessary()
{
  bool      startNow      = false;
  bool      startChannel  = false;
  STFResult result        = STFRES_OK;

  // These conditions determine if the channel should be started at all
  startChannel |= (desiredChannelState == CHANNEL_RESTARTING);
  startChannel |= (desiredChannelState == CHANNEL_RUNNING);

  // if the channel is already running we do not need to start it
  startChannel &= (channelState != CHANNEL_RUNNING);

  // These conditions determine if the channel should be started at this time
  startNow |= (numBufferedNodes >= numBufferedNodesNeedToStart);
  startNow |= forceCompletAllTransfers;

  if (!startChannel)
    result = STFRES_OPERATION_PROHIBITED;  // Regardless of startNow, if the channel is not supposed to start we will return an error
  else if (!startNow)
    result = STFRES_OPERATION_PENDING;  // The channel needs to be started, but not at this time
  else
    result = STFRES_OK;// The channel needs to be started at this time

  STFRES_RAISE(result);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Virtual STFDMA Channel
//
///////////////////////////////////////////////////////////////////////////////

VirtualSTFDMAChannel::VirtualSTFDMAChannel(STFDMAChannel* physicalChannel) :
  VirtualUnit(physicalChannel)
{
    this->physicalChannel = physicalChannel;
}

VirtualSTFDMAChannel::~VirtualSTFDMAChannel()
{
  if (clientCallback != NULL)
    physicalChannel->RemoveClientSignalInterface(clientCallback);
}

STFResult VirtualSTFDMAChannel::Initialize(void)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::Initialize()\n");
  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::InternalUpdate(void)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::InternalUpdate()\n");
  STFRES_RAISE_OK;
}

bool VirtualSTFDMAChannel::IsRunning()
{
  return physicalChannel->IsRunning();
}
  
STFResult VirtualSTFDMAChannel::PerformCallback(STFDMA_STCMGenericNode_t* callbackNode)
{
  uint32 param = callbackNode->control.cbParam; 
  bool finalSection = callbackNode->control.finalSection;

  if (clientCallback != NULL)
  {
    if (finalSection)
      clientCallback->SignalTransferComplete(param);
    else
      clientCallback->SignalTransferInProgress(param);
  }

  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::PauseTransfer(void)
{
  STFRES_REASSERT(physicalChannel->PauseChannel());
  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::QueryInterface(VDRIID iid, void*& ifp)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::QueryInterface()\n");
    
  VDRQI_BEGIN
    VDRQI_IMPLEMENT(VDRIID_STFDMA_CHANNEL, ISTFDMAChannel);
  VDRQI_END(VirtualSTFDMAChannel);
  
  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::RegisterClientSignalInterface(IFDMAChannelClient* clientCallback)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::RegisterClientSignalInterface()\n");

  if (clientCallback == NULL)
    return STFRES_INVALID_PARAMETERS;

  physicalChannel->RegisterClientSignalInterface(clientCallback);
  this->clientCallback = clientCallback;

  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::RequestTransfer(struct STFDMA_NodeData const * pNodeData, uint32 cbParam)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::RequestTransfer()\n");

  STFRES_REASSERT(physicalChannel->Transfer(pNodeData, this, cbParam));

  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::ResumeTransfer()
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::ResumeTransfer()\n");

  // we will start the channel when the first correct number of buffers are posted, the test is done as each buffer
  // is transfered
  if (!physicalChannel->IsRunning())
  {
    physicalChannel->ResetChannel();
    physicalChannel->SetDesiredChannelState(CHANNEL_RESTARTING);
    physicalChannel->StartChannel();
  }

  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::SetRequiredBufferedTransfersBeforeStart(uint32 afterBufferedTransfers)
{
  physicalChannel->numBufferedNodesNeedToStart = 1 > afterBufferedTransfers ? 1 : afterBufferedTransfers;
  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::StartTransfer(uint32 afterBufferedTransfers)
{
  FDMAFunctionNameDP("VirtualSTFDMAChannel::StartTransfer()\n");

  // we will start the channel when the first correct number of buffers are posted, the test is done as each buffer
  // is transfered
  if (!physicalChannel->IsRunning())
  {
    physicalChannel->ResetChannel();
    physicalChannel->SetDesiredChannelState(CHANNEL_RESTARTING);
    physicalChannel->numBufferedNodesNeedToStart = 1 > afterBufferedTransfers ? 1 : afterBufferedTransfers;
    physicalChannel->StartChannel();
  }

  STFRES_RAISE_OK;
}

STFResult VirtualSTFDMAChannel::StopTransfer(void)
{
  STFRES_REASSERT(physicalChannel->StopChannel());
  STFRES_RAISE_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Transfer callback function
//
///////////////////////////////////////////////////////////////////////////////

void GlobalTransferCallback
(
  U32   TransferId,         /* Transfer id */
  U32   CallbackReason,     /* Reason for callback */
  U32*  CurrentNode_p,      /* Node transfering */
  U32   NodeBytesTransfered,/* Number of bytes transfered */
  BOOL  Error,              /* Interrupt missed */
  void* ApplicationData_p   /* Application data - should be a pointer to an STFDMA_Channel object */
)
{
  STFDMAChannel* pObj = (STFDMAChannel*)ApplicationData_p;
  pObj->TransferCallback(TransferId, CallbackReason, CurrentNode_p, NodeBytesTransfered, Error);
}
