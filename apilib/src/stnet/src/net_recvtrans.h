/******************************************************************************

File name   : net_recvtrans.h

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/

#ifndef  __NET_RECVTRANS_H
#define __NET_RECVTRANS_H


#ifdef __cplusplus
extern "C" {
#endif

/*Lists manager functions*/
U8 STNETi_AddToList(STNETi_Device_t* Device_p,STNETi_Handle_t* Handle_p);
STNETi_Handle_t *STNETi_GetHandlefromList(STNETi_Device_t* Device_p,U8 Instance);
STNETi_Handle_t * STNETi_RemovefromList(STNETi_Device_t* Device_p,U8 Instance);

/*Receiver Functions*/
ST_ErrorCode_t STNETi_Receiver_Open(STNETi_Device_t* Device_p,U8 *Index,const STNET_OpenParams_t *OpenParams_p);
ST_ErrorCode_t STNETi_Receiver_Close(STNETi_Device_t* Device_p,U8 Index);
ST_ErrorCode_t STNETi_Receiver_CloseAll(STNETi_Device_t* Device_p);
ST_ErrorCode_t STNETi_Receiver_Start(STNETi_Device_t *Device_p, U8 Instance , const STNET_ReceiverStartParams_t * ReceiverStartParams_p);
ST_ErrorCode_t STNETi_Receiver_Stop(STNETi_Device_t *Device_p, U8 Instance);

/*Transmitter Functions*/
ST_ErrorCode_t STNETi_Transmitter_Open(STNETi_Device_t* Device_p,U8 *Index,const STNET_OpenParams_t* OpenParams_p);
ST_ErrorCode_t STNETi_Transmitter_Close(STNETi_Device_t* Device_p,U8 Index);
ST_ErrorCode_t STNETi_Transmitter_CloseAll(STNETi_Device_t* Device_p);
ST_ErrorCode_t STNETi_Transmitter_Start(STNETi_Device_t *Device_p, U8 Instance , const STNET_TransmitterStartParams_t * TransmitterStartParams_p);
ST_ErrorCode_t STNETi_Transmitter_Stop(STNETi_Device_t *Device_p, U8 Instance);

#ifdef __cplusplus
}
#endif

#endif


