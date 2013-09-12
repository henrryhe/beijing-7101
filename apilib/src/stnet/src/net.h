/******************************************************************************

File name   : net.h

Description : Device Struct and important headers

COPYRIGHT (C) 2007 STMicroelectronics

******************************************************************************/
#ifndef __NET_H
#define __NET_H

/* Includes ---------------------------------------------------------------- */

#ifndef MODULE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <stos.h>
#include <stnet.h>
#include <stinj.h>

#ifndef ST_OSLINUX
 #include <osplus.h>
 #include <ngip.h>
 #include <ngtcp.h>
 #include <ngudp.h>
#else
#ifndef MODULE
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* Private Types ----------------------------------------------------------- */


typedef enum STNETi_ReceiverState_e
{
    RECEIVER_RUNNING,
    RECEIVER_PAUSED,
    RECEIVER_STOPPED,
} STNETi_ReceiverState_t;

typedef STNETi_ReceiverState_t STNETi_TransmitterState_t;

/* The reason why the inject tasklet was last called. Indicates what
 * it should do.
 */
typedef enum {
	STNETi_NOWORK,
	STNETi_NORMALINJECT,
	STNETi_BUFFERFULL,
	STNETi_TIMEOUT,
} STNETi_ReceiverSBInjector_Reason_t;

/* Create some type abstractions for Linux and the NextGen stack */
#ifdef ST_OSLINUX
typedef struct in_addr	STNETi_SocketAddress_t;
typedef S32		STNETi_SocketDescriptor_t;
#else
typedef NGsockaddr	STNETi_SocketAddress_t;
typedef NGsock		STNETi_SocketDescriptor_t;
#endif


typedef struct STNETi_ReceiverHandle_s
{

    STNETi_ReceiverState_t      State;              /* Receiver task state */

  #ifndef ST_OSLINUX
    NGsock                     *SocketDescriptor;
    NGsockaddr                  TransmitterSocketAddress;
    NGsockaddr                  ReceiverSocketAddress;
    NGinmulti                  *MulticastDescriptor;
  #else
    S32                         SocketDescriptor;
    struct sockaddr_in          TransmitterSocketAddress;
    struct sockaddr_in          ReceiverSocketAddress;
  #endif


    /* Buffer params */
    U8                         *BufferPtr;          /* Allocated address */
    U8                         *BufferBase;         /* Aligned address */
    U32                         BufferSize;         /* TS buffer size */
    BOOL                        AllowOverflow;      /* Allow TS buffer overflow */
    U8                         *BufferWritePtr;     /* TS buffer Write pointer */
    U8                         *BufferReadPtr;      /* TS buffer Read pointer */

    /* Task params */
    task_t                     *Task_p;
    tdesc_t                     Tdesc;
    void                       *Stack_p;


}STNETi_ReceiverHandle_t;

typedef struct STNETi_TransmitterHandle_s
{
    BOOL                        IsBlocking;
    STNETi_TransmitterState_t   State;           /* Transmitter task state - Incase of Nonblocking calls*/

#ifndef ST_OSLINUX
    NGsock                     *SocketDescriptor;
    NGsockaddr                  TransmitterSocketAddress;
    NGsockaddr                  ReceiverSocketAddress;/*Might not be really required*/
    NGinmulti                  *MulticastDescriptor;
#else
    S32                         SocketDescriptor;
    struct sockaddr_in          TransmitterSocketAddress;
    struct sockaddr_in          ReceiverSocketAddress;
#endif
    U16                         SequenceNumber;     /* RTP sequence number to transmit */

} STNETi_TransmitterHandle_t;

/*
 * Here is a description of which component touches what attribute.
 *
 * It is depicted during normal use, excluding any API setup and intialisation.
 * This will help identify concurrancy issues. The components here are
 * the logical ones, and do not map directly onto the sourcefiles.
 * Most notably, stnetsb_dequeue() is located in stnet_core_nethook.c
 * but is called from the injection task. Thus only the injection task
 * writes seq_first, the network hook task only reads it.
 *
 *
 * Attribute		Network hook		Injection task
 * ----------------------------------------------------------------------
 * INJDeviceName	-				read
 * ReceiverPort		-				-
 * inputport		read				-
 * inputaddress		read				-
 * WindowSize		read				read
 * WindowTimeout	-				read
 * skbuff		write if NULL		read and clear if non-NULL
 * payload		write if NULL		read and clear if non-NULL
 * buffsize		read				read
 * seq_first		read				write
 * seq_last		write				read
 * rtp_ssrc		write				-
 * rtpseq_max		write				-
 * rtpseq_bad		write				-
 * rtpseq_probation	write				-
 * rtp_cycles		write				-
 * rtpseq_offset	read				write
 * inject_reason	write(1)			write(1)
 * InjCount		write				-
 * InjErrorCount	write				-
 * RtpHoles		-				write
 *
 * Special notes:
 * 1) These writes are atomic. No locking needed.
 */
typedef struct STNETi_ReceiverSBInjectorHandle_s
{
#ifdef ST_OSLINUX		/* This device is Linux-only */

    ST_DeviceName_t		INJDeviceName;
    U16				ReceiverPort;
    U16				inputport;
    STNETi_SocketAddress_t	inputaddress;
    STNETi_SocketAddress_t	hton_inputaddress;
    struct socket		*sock;
    U32				WindowSize;	/*Reordering Depth*/
    U32				WindowTimeout;	/*In ms*/

	/* Buffer handling */
    struct sk_buff		**skbuff;
    STINJ_BufferList_t		*payload;
    U16				buffsize;
    U16				seq_first, seq_last;
	/* RTP handling */
    U32				rtp_ssrc;	/* Our syncronization source */
    U16				rtpseq_max;
    U32				rtpseq_bad;
    U16				rtp_probation;
    U32				rtp_cycles;
	/* RTP sequence no of first buffer entry. */
    U16				rtpseq_offset;

    STNETi_ReceiverSBInjector_Reason_t	inject_reason;
    unsigned long			last_inject_time;

	/* Injection statistics */
    U32				InjCount;
    U32				InjErrorCount;
    U32				RtpHoles;
    U32				SumBuffLevel;
#ifdef STNET_DEBUG
    unsigned long		InjMinTime, InjMaxTime, InjSumTime;
    unsigned long		NonInjMinTime, NonInjMaxTime, NonInjSumTime;
#endif
#endif

} STNETi_ReceiverSBInjectorHandle_t;



typedef struct STNETi_Handle_s
{

    U8          Instance;
    semaphore_t                *AccessMutex;
    struct STNETi_Device_s     *Device_p;
    STNET_TransmissionProtocol_t    TransmissionProtocol;
	/* Packet statistics */
    U32				Count;
    U32				ErrorCount;
	/* Type specific data */
    union
    {
        STNETi_ReceiverHandle_t		  Receiver;
	    STNETi_TransmitterHandle_t	  Transmitter;
 	    STNETi_ReceiverSBInjectorHandle_t ReceiverSBInjector;
 	}Type;
    struct STNETi_Handle_s* Next_p;


} STNETi_Handle_t;

typedef enum STNETi_DeviceState_e
{
    DEVICE_NOT_USED,
    DEVICE_INITIALISING,
    DEVICE_INITIALISED,
    DEVICE_TERMINATING,
    DEVICE_TERMINATED
} STNETi_DeviceState_t;


typedef struct STNETi_Device_s
{
    STNETi_DeviceState_t        State;
    STNET_Device_t              DeviceType;
    STNET_EthernetDevice_t      EthernetDeviceType;
    ST_DeviceName_t             DeviceName;
    ST_DeviceName_t             EVTDeviceName;
    partition_t                *DriverPartition;
    partition_t                *NCachePartition;

    U8                          Num_Open; /*Number of opens on the device so far*/
    STNETi_Handle_t            *HandleList_p;/*List head*/
    semaphore_t                *HandleListMutex;

    /* Host IP address for future reference */
    U32                         IPAddress;

#ifdef MODULE
   struct list_head		List;	/* Linked list */
#endif
} STNETi_Device_t;


/*  Constants ------------------------------------------------------- */

#define STNETi_MAX_DEVICES              3/*One for Receiver,Transmitter,SB Receiver*/
#define STNETi_TS_PACKET_SIZE           188
#define STNETi_TS_NB_PACKETS_IN_UDP     7

#ifdef ST_OS21
    #define STNETi_RECEIVER_TASK_SIZE       ((OS21_DEF_MIN_STACK_SIZE)+(2*1024))
#else
    #define STNETi_RECEIVER_TASK_SIZE       (2*1024)
#endif
#define STNETi_RECEIVER_TASK_PRIORITY   14

#define STNETi_BUFFER_ALIGNMENT         32
#define STNETi_RECEIVER_BUFFER_SIZE     (STNETi_TS_PACKET_SIZE * 512 * 8)
#define STNETi_MAX_ADDRESS_LENGTH       20
#define STNETi_MAX_HANDLES              254


/* Private Macros ---------------------------------------------------------- */
#ifdef  LITTLE_ENDIAN_CPU
#define STNETi_ENDIAN_MEMCPY_WORDS(__Dst__, __Src__, __Size__) memcpy(__Dst__, __Src__, __Size__)
#else
#define STNETi_ENDIAN_MEMCPY_WORDS(__Dst__, __Src__, __Size__) \
                do  \
                {   \
                    U8     *Dst = (U8 *)(__Dst__),          \
                           *Src = (U8 *)(__Src__);          \
                    U32     WordCount = ((__Size__)>>2);    \
                    \
                    while(WordCount--)      \
                    {   \
                        Dst[3] = Src[0];    \
                        Dst[2] = Src[1];    \
                        Dst[1] = Src[2];    \
                        Dst[0] = Src[3];    \
                        Src += 4;           \
                        Dst += 4;           \
                    }   \
                }while(0)
#endif




#define DEV_TYPE(__Handle__)                   ((__Handle__) >> 8)
#define INSTANCE_NO(__Handle__)                ((__Handle__) & 0x00FF)
#define TO_HANDLE(__Type__,__Inst__)	(((__Type__)<<8)|(__Inst__))

#ifndef MAX
#define MAX(X,Y)        ((X)>=(Y) ? (X) : (Y))
#define MIN(X,Y)        ((X)<=(Y) ? (X) : (Y))
#endif

/* TODO: issue with STOS in linux user mode, should be removed */
#undef STOS_TaskLock
#undef STOS_TaskUnlock

#define STOS_TaskLock()        task_lock()
#define STOS_TaskUnlock()      task_unlock()


#ifdef __cplusplus
}
#endif

#endif
/* EOF --------------------------------------------------------------------- */

