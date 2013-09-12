/******************************************************************************

File name   : nginit.c

Description : NexGen TCP/IP subsystem initialisation

COPYRIGHT (C) 2007 STMicroelectronics

Date               Modification                 Name
----               ------------                 ----
17/07/07           Created                      J.ROY

NOTE:
    - BUF_HDRMAX: The ethernet header is usually 14-bytes long. USB host-to-host
      devices usually prefix the packet with its length (an addition 4-bytes).
    - BUF_DATAMAX: As ethernet packets will be a maximum of 1514-bytes, usually
      this is set to 1500. Better performance should be achievable by using
      larger values, provided that the partner in the transfer is able to
      accept the larger MTU size.
    - If the network interface is a USB host-to-host cable, then a default MAC
      address of 00:50:77:01:00:00 will be set. This must be overridden on
      the command line.

/!\ Important: OSPLUS_INPUT_STREAM_BYPASS  is a default rule, don't change it

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>

#include "stdevice.h"
#include "stsys.h"
#include "stnet.h"
#include "sttbx.h"
#include "nginit.h"

#include <ngip.h>
#include <ngtcp.h>
#include <ngudp.h>
#include <ngeth.h>
#include <ngdev.h>
#include <ngmodule.h>
#include <ngresolv.h>
#include <nexgen/ngip/ethernet.h>
#include <ngping.h>

#include <osplus/dev.h>
#include <osplus/registry.h>
#include <osplus/h2heth.h>
#include <osplus/lan91.h>
#include <osplus/lan9x1x.h>
#include <osplus/stmac110.h>


/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */
static partition_t      *STNETi_SystemPartition;

static char             STNETi_OSPlusDeviceName[] = "/ethernet0";
static device_handle_t  *STNETi_OSPLUS_LanHandle = NULL;
static task_t           *STNETi_OSPLUS_ReceiverTask = NULL;

/* Private Macros ---------------------------------------------------------- */
#define STNETi_HEX_CHAR_TO_DECIMAL(__Char__)    \
                ((((__Char__) >= '0') && ((__Char__) <= '9')) ? ((__Char__) - '0') :            \
                 (((__Char__) >= 'A') && ((__Char__) <= 'F')) ? (10 + ((__Char__) - 'A')) :     \
                 (((__Char__) >= 'a') && ((__Char__) <= 'f')) ? (10 + ((__Char__) - 'a')) : 0)

#define STNETi_HEX_CHAR_NIBBLE_TO_BYTE(__UpperNibble__, __LowerNibble__)    \
                ((STNETi_HEX_CHAR_TO_DECIMAL(__UpperNibble__) << 4) | STNETi_HEX_CHAR_TO_DECIMAL(__LowerNibble__))

/* Private functions prototypes -------------------------------------------- */
void                  *STNETi_NexGenAlloc(unsigned int Size);
static int            STNETi_OSPLUS_Open(NGifnet *netp);
static int            STNETi_OSPLUS_Close(NGifnet *netp);
static void           STNETi_OSPLUS_Send(NGifnet *netp);
static void           STNETi_OSPLUS_InputTask(void *arg);
static void           STNETi_OSPLUS_SetFilters(NGifnet *netp);
static int            STNETi_PingCallback(int seq,int err,int datasize,int ttl,int timen,NGsockaddr *saddr,void *data);

/* Global Variables -------------------------------------------------------- */

/* Public functions -------------------------------------------------------- */

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.2 Network Buffers Options
 *****************************************************************************/
#define BUF_HDRMAX      14      /* See comment at top of this file */
#define BUF_DATAMAX     1500    /* See comment at top of this file */
#define BUF_DATAMAX_H2H 1500    /* 4000 */
#define BUF_INPQMAX     1024    /* Network input queue size */

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.3 Socket Options
 *****************************************************************************/
#define SOCK_MAX        32      /* Maximum number of sockets */
static NGsock  Sock[SOCK_MAX];
static NGOSsem SockSem[SOCK_MAX];

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.4 RTOS Options
 *****************************************************************************/
#define OSPLUS_INPUT_TASK_STACKSIZE 16384
/* Give max priority to I/P task */
#define OSPLUS_INPUT_TASK_PRIORITY  ((MAX_USER_PRIORITY-15)+15)

#define OSPLUS_TIMER_TASK_PRIORITY  ((MAX_USER_PRIORITY-15)+10)

static NGubyte     OSPLUS_inputstack[OSPLUS_INPUT_TASK_STACKSIZE];
static NGubyte     OSPLUS_timerstack[OSPLUS_INPUT_TASK_STACKSIZE];

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.7 IP Protocol
 *****************************************************************************/
#define INMULTI_MAX     16       /* Maximum number of multicast group entries */
static NGinmulti InMultiTable[INMULTI_MAX];

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.8 ARP Protocol
 *****************************************************************************/
#define ARP_MAX         8       /* Maximum number of ARP entries */
static NGarpent ARPTable[ARP_MAX];

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.10 TCP Protocol
 *****************************************************************************/
static NGtcpcb TCPcb[SOCK_MAX];

/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.13 Adding a Network Interface
 *****************************************************************************/

#define BUF_OUTQMAX     1024    /* Network output queue size */

static NGethifnet ngNetDrv_data_eth0;

static const NGnetdrv ngNetDrv_osplus_ethernet =
{
 "NexGen OSPlus Driver",
 NG_IFT_ETHER,
 NG_IFF_BROADCAST|NG_IFF_ALLMULTI,
 ETHERMTU,
 100000000UL, /* 100 Mbps, unsigned long */
 ngEtherInit,
 STNETi_OSPLUS_Open,
 STNETi_OSPLUS_Close,
 ngEtherOutput,
 STNETi_OSPLUS_Send,
 ngEtherCntl,
 STNETi_OSPLUS_SetFilters
};


static char  ngNetDrv_osplus_mac[6]  = {0,0,0,0,0,0};


/******************************************************************************
 * NexGenIP Programming Guide, Section 2.3.15 Loopback Interface
 *****************************************************************************/
#define LOOPBACK_DEVICE_NAME "lo0"
static NGifnet ngNetDrv_data_lo0;

/*****************************************************************************
 * NexGen configuration structure
 *****************************************************************************/
static NGcfgent TCPConfig[] = {
  /* NexGenIP Programming Guide, Section 2.3.2 Network Buffers Options */
  {NG_BUFO_HEADER_SIZE,     NG_CFG_INT(BUF_HDRMAX)},
  {NG_BUFO_DATA_SIZE,       NG_CFG_INT(BUF_DATAMAX)},
  {NG_BUFO_INPQ_MAX,        NG_CFG_INT(BUF_INPQMAX)},
  {NG_BUFO_MAX,             NG_CFG_INT(BUF_INPQMAX)},
  {NG_BUFO_ALLOC_BUF_F,     NG_CFG_PTR(STNETi_NexGenAlloc)},

  /* NexGenIP Programming Guide, Section 2.3.3 Socket Options */
  {NG_SOCKO_MAX,            NG_CFG_INT(sizeof (Sock)/sizeof (Sock[0]))},
  {NG_SOCKO_TABLE,          NG_CFG_PTR(&Sock)},
  {NG_SOCKO_SEMTABLE,       NG_CFG_PTR(SockSem)},

  /* NexGenIP Programming Guide, Section 2.3.4 RTOS Options */
  {NG_RTO_INPUT_PRIO,       NG_CFG_INT(OSPLUS_INPUT_TASK_PRIORITY)},
  {NG_RTO_INPUT_STACK_SIZE, NG_CFG_INT(OSPLUS_INPUT_TASK_STACKSIZE)},
  {NG_RTO_INPUT_STACK,      NG_CFG_PTR(OSPLUS_inputstack)},
  {NG_RTO_TIMER_PRIO,       NG_CFG_INT(OSPLUS_TIMER_TASK_PRIORITY)},
  {NG_RTO_TIMER_STACK_SIZE, NG_CFG_INT(OSPLUS_INPUT_TASK_STACKSIZE)},
  {NG_RTO_TIMER_STACK,      NG_CFG_PTR(OSPLUS_timerstack)},

  /* NexGenIP Programming Guide, Section 2.3.7 IP Protocol */
  {NG_CFG_PROTOADD,         NG_CFG_PTR(&ngProto_IP)},
  {NG_IPO_INMULTI_MAX,      NG_CFG_INT(sizeof (InMultiTable)/sizeof (InMultiTable[0]))},
  {NG_IPO_INMULTI_TABLE,    NG_CFG_PTR(&InMultiTable)},

  /* NexGenIP Programming Guide, Section 2.3.8 ARP Protocol */
  {NG_CFG_PROTOADD,         NG_CFG_PTR(&ngProto_ARP)},
  {NG_ARPO_MAX,             NG_CFG_INT(sizeof (ARPTable)/sizeof (ARPTable[0]))},
  {NG_ARPO_TABLE,           NG_CFG_PTR(&ARPTable)},
  {NG_ARPO_EXPIRE,          NG_CFG_INT(120)},

  /* NexGenIP Programming Guide, Section 2.3.9 UDP Protocol */
  {NG_CFG_PROTOADD,         NG_CFG_PTR(&ngProto_UDP)},

  /* NexGenIP Programming Guide, Section 2.3.10 TCP Protocol */
  {NG_CFG_PROTOADD,         NG_CFG_PTR(&ngProto_TCP)},
  {NG_TCPO_TCB_MAX,         NG_CFG_INT(sizeof (TCPcb)/sizeof (TCPcb[0]))},
  {NG_TCPO_TCB_TABLE,       NG_CFG_PTR(&TCPcb)},

  /* NexGenIP Programming Guide, Section 2.3.11 RAW-IP Protocol */
  {NG_CFG_PROTOADD,         NG_CFG_PTR( &ngProto_RAWIP)},

  /* NexGenIP Programming Guide, Section 2.3.13 Adding a Network Interface */
  {NG_CFG_IFADD,            NG_CFG_PTR(&ngNetDrv_data_eth0)},
  {NG_CFG_DRIVER,           NG_CFG_PTR(&ngNetDrv_osplus_ethernet)},
  {NG_IFO_NAME,             NG_CFG_PTR(ETHERNET_DEVICE_NAME)},
  {NG_IFO_OUTQ_MAX,         NG_CFG_INT(BUF_OUTQMAX)},
  {NG_IFO_MTU,              NG_CFG_INT(BUF_DATAMAX)},
  {NG_IFO_ADDR,             NG_CFG_ADR(0,0,0,0)},
  {NG_IFO_NETMASK,          NG_CFG_ADR(0,0,0,0)},
  {NG_IFO_ALLMULTI,         NG_CFG_TRUE},
  {NG_IPO_ROUTE_DEFAULT,    NG_CFG_ADR(0,0,0,0)},
  {NG_ETHIFO_FULLDUPLEX,    NG_CFG_TRUE},

  /* NexGenIP Programming Guide, Section 2.3.15 Loopback Interface */
  {NG_CFG_IFADD,            NG_CFG_PTR(&ngNetDrv_data_lo0)},
  {NG_CFG_DRIVER,           NG_CFG_PTR(&ngNetDrv_LOOPBACK)},
  {NG_IFO_NAME,             NG_CFG_PTR(LOOPBACK_DEVICE_NAME)},

  /* Terminate the configuration structure */
  {NG_CFG_END}
} ;


/******************************************************************************
Name         : STNETi_NexGenInit()

Description  : Initializes NexGen IP stack

Parameters   : STNET_InitParams_t  *InitParams_p    Pointer to the params structure

Return Value : Error code
******************************************************************************/
ST_ErrorCode_t STNETi_NexGenInit(STNET_InitParams_t *InitParams_p)
{
    S32         Err,ErrorValue =0;
    U32         i, j;
    U32         BufferDataMax = BUF_DATAMAX,
                BufferHdrMax  = BUF_HDRMAX;
    NGuint      IPAddress,
                IPGateway,
                IPSubnetMask;

    /* Intialise cache partition used by NG IP */
    STNETi_SystemPartition = InitParams_p->DriverPartition;

    /* Initialise the MAC address */
    for(i=0,j=0; i<6; i++, j+=3)
    {
        ngNetDrv_osplus_mac[i] = STNETi_HEX_CHAR_NIBBLE_TO_BYTE(InitParams_p->MACAddress[j], InitParams_p->MACAddress[j+1]);
    }

    switch(InitParams_p->EthernetDevice)
    {
        case STNET_DEVICE_LAN91:
            /* It is necessary to set the correct MAC address for each board */
            ErrorValue = registry_key_set(LAN91_CONFIG_PATH,LAN91_CONFIG_MAC_ADDRESS, (void *)ngNetDrv_osplus_mac);
            if (ErrorValue!=0)
            {
                STTBX_Print(("Failed to set ngNetDrv_osplus_mac, Error =%d\n",ErrorValue));
            }

            if(lan91_initialize () != 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to initialise the LAN91 driver\n"));
                return ST_ERROR_BAD_PARAMETER;
            }

            BufferDataMax = BUF_DATAMAX;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_STMAC110:
            /* It is necessary to set the correct MAC address for each board */
            ErrorValue = registry_key_set(STMAC110_CONFIG_PATH"/0",STMAC110_CONFIG_MAC_ADDRESS, (void *)ngNetDrv_osplus_mac);
            if (ErrorValue!=0)
            {
                STTBX_Print(("Failed to set ngNetDrv_osplus_mac, Error =%d\n",ErrorValue));
            }

            if(stmac110_initialize () != 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to initialise the STMAC110 driver\n"));
                return ST_ERROR_BAD_PARAMETER;
            }
            BufferDataMax = BUF_DATAMAX;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_LAN9X1X:
            /* It is necessary to set the correct MAC address for each board */
            ErrorValue = registry_key_set(LAN9X1X_CONFIG_PATH"/0",LAN9X1X_CONFIG_MAC_ADDRESS, (void *)ngNetDrv_osplus_mac);
            /* NexGen buffers are only 16-bit aligned - set the RX offset to 2 */
            ErrorValue |= registry_key_set (LAN9X1X_CONFIG_PATH"/0", LAN9X1X_CONFIG_RX_OFFSET, (void *) 2);
            if (ErrorValue!=0)
            {
                STTBX_Print(("Failed to set ngNetDrv_osplus_mac, Error =%d\n",ErrorValue));
            }

            if(lan9x1x_initialize () != 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to initialise the LAN9x1x driver\n"));
                return ST_ERROR_BAD_PARAMETER;
            }
            BufferDataMax = BUF_DATAMAX;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_USB_CBL_MDM:
            BufferDataMax = BUF_DATAMAX;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_USB_ETHERNET:
            BufferDataMax = BUF_DATAMAX;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_USB_H2HETH:
            if(h2heth_initialize () != 0)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to initialise the h2heth driver\n"));
                return ST_ERROR_BAD_PARAMETER;
            }
            BufferDataMax = BUF_DATAMAX_H2H;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        case STNET_DEVICE_USB_H2H:
            BufferDataMax = BUF_DATAMAX_H2H;
            BufferHdrMax  = BUF_HDRMAX;
            break;

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unknown driver interface\n"));
            return ST_ERROR_BAD_PARAMETER;
    }

    /* Patch the NexGen configuration structure default driver with new one */
    NGcfgent *Config = TCPConfig;

    while(Config->cfg_option != NG_CFG_END)
    {
        /* Find the buffer header size parameter */
        if((Config->cfg_option == NG_BUFO_HEADER_SIZE) && (Config->cfg_arg == (NGcfgarg)BUF_HDRMAX))
        {
            Config->cfg_arg = (NGcfgarg) BufferHdrMax;
        }

        /* Find the buffer data size parameter */
        if((Config->cfg_option == NG_BUFO_DATA_SIZE) && (Config->cfg_arg == (NGcfgarg)BUF_DATAMAX))
        {
            Config->cfg_arg = (NGcfgarg) BufferDataMax;
        }
        Config++;
    }

    /* Initialise the NexGen TCP/IP subsystem */
    if((Err = ngInit(TCPConfig)) != NG_EOK)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Failed to initialise NexGen TCP/IP Err[%d]\n", Err));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Set IP/NETMASK/GETWAY */
    {
        NGifnet                 *netp;
        netp=ngIfGetPtr(ETHERNET_DEVICE_NAME);

        /* set IP add */
        if (ngInetATON(InitParams_p->IPAddress, &IPAddress)!=NG_EOK)
        {
            STTBX_Print(("--> Invalid ip address, should be [\"192.168.0.100\"] !\n"));
            return ST_ERROR_BAD_PARAMETER;
        }
        if (ngIfSetOption(netp,NG_IFO_ADDR,&IPAddress)!=NG_EOK)
        {
            STTBX_Print(("--> Unable to set the ip address\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        /* set subnet mask */
        if (ngInetATON(InitParams_p->IPSubnetMask, &IPSubnetMask)!=NG_EOK)
        {
            STTBX_Print(("--> Invalid netmask, should be [\"255.255.255.0\"] !\n"));
            return ST_ERROR_BAD_PARAMETER;
        }
        if (ngIfSetOption(netp,NG_IFO_NETMASK,&IPSubnetMask)!=NG_EOK)
        {
            STTBX_Print(("--> Unable to set the netmask\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        /* set the gateway */
        if (ngInetATON(InitParams_p->IPGateway, &IPGateway)!=NG_EOK)
        {
            STTBX_Print(("--> Invalid gateway address, should be [\"192.168.0.254\"] !\n"));
            return ST_ERROR_BAD_PARAMETER;
        }
        if (ngRouteDefault(IPGateway)!=NG_EOK)
        {
            STTBX_Print(("--> Unable to set the gateway\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

    }

    /* Ping the Ethernet device */
    {
        NGsockaddr Ipaddr;
        NGcfgping  Cfgping;

        STTBX_Print(("\n--> Pinging ethernet device !!!\n"));

        ngMemSet(&Cfgping,0,sizeof(Cfgping));
        Cfgping.repeat       = 4;
        Cfgping.data_len     = 0;
        Cfgping.display      = STNETi_PingCallback;
        ngInetATON((const char*)InitParams_p->IPAddress,&Ipaddr.sin_addr);
        if (ngPing(&Ipaddr,&Cfgping)!=NG_EOK)
        {
            STTBX_Print(("--> Ping function has returned an error !\n"));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    return ST_NO_ERROR;
}


/******************************************************************************
Name         : STNETi_NexGenAlloc()

Description  : Memory allocation function for NexGen

Parameters   : Size of memory required

Return Value : void * address of allocated memory
******************************************************************************/
void *STNETi_NexGenAlloc(unsigned int Size)
{
    U32 Buffer;

    /* Allocate a buffer from the heap that is cache aligned */
    /* ----------------------------------------------------- */
    Buffer=(U32)osplus_cache_safe_allocate(STNETi_SystemPartition,Size+512+(ngBufDataOffset-ETHER_HDR_LEN-2)+32);
    Buffer=Buffer+(ngBufDataOffset-ETHER_HDR_LEN-2);
    Buffer=((Buffer+511)&0xFFFFFE00);
    Buffer=Buffer-(ngBufDataOffset-ETHER_HDR_LEN-2);

    /* Return ptr */
    /* ---------- */
    return((void *)Buffer);
}

/******************************************************************************
Name         : STNETi_OSPLUS_Open

Description  : Initialize and open ethernet device
******************************************************************************/
static int STNETi_OSPLUS_Open(NGifnet *netp)
{
    /* Ensure that the ethernet device is not already running */
    /* ------------------------------------------------------ */
    if (netp->if_flags & NG_IFF_RUNNING)
    {
        return(NG_EALREADY);
    }

    /* Make sure that interface is not already open and then open it */
    /* ------------------------------------------------------------- */
    if (STNETi_OSPLUS_LanHandle==NULL)
    {
        /* Open the ip chip */
        if ((STNETi_OSPLUS_LanHandle=device_open_timeout(STNETi_OSPlusDeviceName,OSPLUS_O_RDWR,TIMEOUT_INFINITY))==NULL)
        {
            return(NG_EINVAL);
        }
    }

    /* Get the MAC address */
    /* ------------------- */
    if (device_ioctl(STNETi_OSPLUS_LanHandle,OSPLUS_IOCTL_ETH_GET_MAC,(void*)NG_ETHIF_DATA(netp,eif_addr))!=0)
    {
        STTBX_Print(("STNETi_OSPLUS_Open():**ERROR** !!! Unable to get the mac address !!!\n"));
        device_close(STNETi_OSPLUS_LanHandle);
        STNETi_OSPLUS_LanHandle=NULL;
        return(NG_EINVAL);
    }

    /* Set the multicast tables */
    /* ------------------------ */
    STNETi_OSPLUS_SetFilters(netp);

    /* Mark the interface as running */
    /* ----------------------------- */
    netp->if_flags|=(NG_IFF_RUNNING|NG_IFF_UP);

    /* Create the OSPLUS receiver task */
    /* ------------------------------- */
    /* Task priority of input task should be at maximum, otherwise it could generates overrun */
    /* Problem seen witn LAN91C111 driver and overrun can't recover                               */
    if ((STNETi_OSPLUS_ReceiverTask= task_create(STNETi_OSPLUS_InputTask,(void *)netp,OSPLUS_INPUT_TASK_STACKSIZE,OSPLUS_INPUT_TASK_PRIORITY,"OSPLUS Input Task",0/*task_flags_no_min_stack_size*/)) == NULL)
    {
        STTBX_Print(("STNETi_OSPLUS_Open():**ERROR** !!! Unable to create a task !!!\n"));
        device_close(STNETi_OSPLUS_LanHandle);
        STNETi_OSPLUS_LanHandle=NULL;
        return(NG_ENOMEM);
    }

    /* Return no errors */
    /* ---------------- */
    return(NG_EOK);
}


/******************************************************************************
Name         : STNETi_OSPLUS_Close

Description  : Close ethernet device
******************************************************************************/
static int STNETi_OSPLUS_Close(NGifnet *netp)
{
    NGbuf *pkt;

    /* The interface must be up and running in order to close */
    /* ------------------------------------------------------ */
    if (!(netp->if_flags & NG_IFF_RUNNING))
    {
        return(NG_EALREADY);
    }

    /* Clear the flags */
    /* --------------- */
    netp->if_flags&=~(NG_IFF_RUNNING | NG_IFF_UP | NG_IFF_OACTIVE);

    /* Wait until the receive task exits, then delete it */
    /* ------------------------------------------------- */
    task_wait(&STNETi_OSPLUS_ReceiverTask,1,TIMEOUT_INFINITY);
    if(task_delete(STNETi_OSPLUS_ReceiverTask) != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "task_delete failed\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Close the LAN91 device */
    /* ---------------------- */
    device_close(STNETi_OSPLUS_LanHandle);
    STNETi_OSPLUS_LanHandle=NULL;

    /* Clear any pending buffers */
    /* ------------------------- */
    while(1)
    {
        ngBufDequeue(netp, pkt);
        if (pkt==NULL) break;
        ngBufOutputFree(pkt);
    }

    /* Return no errors */
    /* ---------------- */
    return(NG_EOK);
}


/******************************************************************************
Name         : STNETi_OSPLUS_Send

Description  : Start sending any pending messages
******************************************************************************/
static void STNETi_OSPLUS_Send(NGifnet *netp)
{
    int flags;
    NGbuf *pkt;

    /* Ensure driver is up & running and output is not active */
    /* -------------------------------------------------------- */
    flags = netp->if_flags & (NG_IFF_RUNNING | NG_IFF_UP | NG_IFF_OACTIVE);
    if (flags != (NG_IFF_RUNNING | NG_IFF_UP))
    {
        return;
    }

    /* Indicate output is active */
    /* ------------------------- */
    netp->if_flags |= NG_IFF_OACTIVE;

    /* Loop sending packets */
    /* -------------------- */
    while(1)
    {
        ngBufDequeue(netp,pkt);
        if (pkt==NULL) break;
        if (device_write(STNETi_OSPLUS_LanHandle,pkt->buf_datap,pkt->buf_datalen)==pkt->buf_datalen)
        {
            /* Increment the number of bytes and packets written */
            netp->if_opackets++;
            netp->if_obytes+=pkt->buf_datalen;
            /* Update the multicast count */
            if (pkt->buf_flags&(NG_BUFF_BCAST|NG_BUFF_MCAST))
            {
                netp->if_omcasts++;
            }
            /* Free packet */
            ngBufOutputFree(pkt);
        }
        else
        {
            /* Error sending packet, update statistics, and free packet */
            STTBX_Print(("STNETi_OSPLUS_Send():**ERROR** !!! Unable to send a packet on the ethernet !!!\n"));
            netp->if_oerrors++;
            netp->if_oqdrops++;
            ngBufOutputFree(pkt);
            break;
        }
    }

    /* Indicate no longer output active */
    /* -------------------------------- */
    netp->if_flags &= ~NG_IFF_OACTIVE;
}


/******************************************************************************
Name         : STNETi_OSPLUS_SetFilters

Description  : Set the multicast filters
******************************************************************************/
static void STNETi_OSPLUS_SetFilters(NGifnet *netp)
{
    U32 OSPLUS_Flags=0;

    /* Set promiscuous mode */
    /* -------------------- */
    if (netp->if_flags & NG_IFF_PROMISC)
    {
        OSPLUS_Flags|=OSPLUS_ETH_FILTER_PROMISCUOUS;
    }

    /* Set receive of all multicasts */
    /* ----------------------------- */
    if (netp->if_flags & NG_IFF_ALLMULTI)
    {
        OSPLUS_Flags|=OSPLUS_ETH_FILTER_ALL_MULTICAST;
    }

    /* Set receive of all broadcasts */
    /* ----------------------------- */
    if (netp->if_flags & NG_IFF_BROADCAST)
    {
        OSPLUS_Flags|=OSPLUS_ETH_FILTER_BROADCAST;
    }

    /* Program a new multicast table */ /* to check */
    /* ----------------------------- */
    if (netp->if_flags & NG_IFF_MULTICAST)
    {
        ethernet_multicast_t multicasts;
        U32                  i;
        U8                   addresses[NG_ETHIF_MULTIADDRS_MAX][6];
        /* Setup flags */

        OSPLUS_Flags=OSPLUS_ETH_FILTER_MULTICAST;

        /* Set up the multicasts */
        multicasts.number    = 0;
        multicasts.addresses = (unsigned char *)addresses;
        /* Set the ethernet interface multicast filters */
        for (i=0;i<NG_ETHIF_MULTIADDRS_MAX;i++)
        {
            /* Is this multicast address valid? */
            if (NG_ETHIF_DATA(netp,eif_multiaddrs)[i].eifm_refcount)
            {
                memcpy(addresses[multicasts.number++],NG_ETHIF_DATA(netp,eif_multiaddrs)[i].eifm_addr,6);
            }
        }
        /* Set the multicast table */
        if (multicasts.number>0)
        {
            if (device_ioctl(STNETi_OSPLUS_LanHandle,OSPLUS_IOCTL_ETH_SET_MCAST_FILTERS,(void *)&multicasts)!=0)
            {
                STTBX_Print(("STNETi_OSPLUS_SetFilters():**ERROR** !!! Unable to set the multicast table !!!\n"));
            }
        }
    }

    /* Update filter packet mode */
    /* ------------------------- */
    if (device_ioctl(STNETi_OSPLUS_LanHandle,OSPLUS_IOCTL_ETH_SET_PKT_FILTER,(void *)OSPLUS_Flags)!=0)
    {
        STTBX_Print(("STNETi_OSPLUS_SetFilters():**ERROR** !!! Unable to set all packet filtering mode !!!\n"));
    }
}


/******************************************************************************
Name         : STNETi_OSPLUS_InputTask

Description  : This task will continuously read packets from the ethernet chip
******************************************************************************/
static void STNETi_OSPLUS_InputTask(void *arg)
{
    NGifnet *netp=(NGifnet *)arg;
    int length;
    NGbuf **MsgSend;


    /* Infinite loop */
    /* ============= */
    while(netp->if_flags&NG_IFF_RUNNING)
    {
        NGbuf *pkt=NULL;

        /* Allocate a buffer for the packet */
        /* -------------------------------- */
        ngBufAlloc(pkt);

        /* If the packet is null, go out */
        /* ----------------------------- */
        if (pkt==NULL)
        {
            STTBX_Print(("STNETi_OSPLUS_InputTask():**ERROR** !!! Unable to allocate a packet buffer !!!\n"));
            task_delay(ST_GetClocksPerSecond()/1000);
        }
        /* Ensure we allocated a buffer */
        /* ---------------------------- */
        else
        {
            /* Set the buffer up ready to receive the packet */ /* to check */
            pkt->buf_datap   = ((NGubyte *)pkt)+ngBufDataOffset-ETHER_HDR_LEN;
            pkt->buf_datalen = ngBufDataMax+ETHER_HDR_LEN;

            /* Read the packet into the buffer */
            length = device_read(STNETi_OSPLUS_LanHandle,pkt->buf_datap,pkt->buf_datalen);
            /* If we failed the read, the free the buffer */
            if (length==-1)
            {
                STTBX_Print(("STNETi_OSPLUS_InputTask():**ERROR** !!! Packet length is -1, errno = %d !!!\n",errno));
                /* Free the packet */
                pkt->buf_datalen=0;
                ngBufFree(pkt);
                /* Update drop count */
                netp->if_iqdrops++;
            }
            else
            {

#if 0
                /* Dump the ethernet packet received, debug only */
                /* --------------------------------------------- */
                U32 i,j;
                STNETi_TraceDBG1(("STNETi_OSPLUS_InputTask(): Packet addr   = 0x%08x\n",pkt));
                STNETi_TraceDBG1(("STNETi_OSPLUS_InputTask(): Packet length = %d\n",length));
                for (i=0,j=1;i<length;i++)
                {
                    STNETi_TraceDBG1(("%02x ",pkt->buf_datap[i]));
                    if (j==16) {STNETi_TraceDBG1(("\n")); j=0;}
                    j++;
                }
                    STNETi_TraceDBG1(("\n"));

#endif
                /* We don't use any acceleration, just pass the packet received to the stack */
                /* ------------------------------------------------------------------------- */
#if !defined(OSPLUS_INPUT_STREAM_BYPASS)
                pkt->buf_datalen=length;
                ngEtherInput(netp,pkt,1);
#endif

#if 0
                /* If length is less than 7 UDP packet plus Header discard it */
                if (length<((7*188)+42))
                {
                    pkt->buf_datalen=length;
                    ngEtherInput(netp,pkt,1);
                }
#endif

                /* Now we have to send this ip packet to STNET_ReceiverTask() */
                /* ---------------------------------------------------------- */
#if defined(OSPLUS_INPUT_STREAM_BYPASS)
                if((NG_Start == TRUE)&&(((char *)pkt->buf_datap)[0x24]==((char *)&NG_SocketAddr.sin_port)[0])&&(((char *)pkt->buf_datap)[0x25]==((char *)&NG_SocketAddr.sin_port)[1]))
                {
                    /* STNETi_TraceDBG1(("\n !!!!!Valid Pkt found inside nginit !!!!!\n"));*/
                    /* Send pkt to Receiver task */

                    MsgSend = (NGbuf **)message_claim(STNETi_TaskMQ_p);
                    pkt->buf_datalen=length;
                    *MsgSend = pkt;
                    message_send(STNETi_TaskMQ_p, MsgSend);

                }
                else
                {
                    pkt->buf_datalen=length;
                    ngEtherInput(netp,pkt,1);
                }
#endif

            }
        }
    }
}

/******************************************************************************
Name:        STNETi_PingCallback
Description: Ping callback function
******************************************************************************/
static int STNETi_PingCallback(int seq,int err,int datasize,int ttl,int time,NGsockaddr *saddr,void *data)
{
    char ipaddr[40];

    /* Display result of the ping */
    /* -------------------------- */
    ngMemSet(ipaddr,0,sizeof(ipaddr));
    ngInetNTOA(saddr->sin_addr,ipaddr,40);
    if (err==NG_EOK)
    {
        STTBX_Print(("%d bytes from %s: icmp_seq=%d ttl=%d time=%d ms\n",datasize,ipaddr,seq,ttl,time));
    }
    else
    {
        STTBX_Print(("Host not responding\n"));
    }

    /* Return no errors */
    /* ---------------- */
    return(NG_EOK);
}


/* EOF ---------------------------------------------------------------------- */


