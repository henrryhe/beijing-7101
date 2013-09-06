/*****************************************************************************

File name   :  ether_dm9000.c

Description :  DM9000A driver for st

Date               Modification                 Name
----               ------------                -------
01/08/2005                   Anky

*****************************************************************************/
#ifdef USE_INIT_TASK /*sqzow test*/
    #undef USE_INIT_TASK
#endif
/* --- includes ----------------------------------------------------------- */
#include <string.h>
#include <interrup.h>
#include <stdlib.h>
#include <semaphor.h>
#include <debug.h>
#include <task.h>
#include "stddefs.h"
#include "move2d.h"
#include "ether_dm9000.h"
#include "report.h"
#include "network_stack.h"
#include "smsc911x.h"
#include "smsc_threading.h"
#include "packet_manager.h"
#include "stsys.h"

/*--------- defines-------------------------------------------------------- */
/*#define FOR_SWITCH_FUNC*/
#define AUTO_RELEASE              /* auto release transmitted packet*/
#define MANUAL
#define MOVE2D
#define TASK_MODE
/*#define LOOP_BACK*/
#define DEBUG_DM_DRIVER             0
#ifdef MOVE2D
    #define DATA8                   *(DU8*)(iodata)
    #define DATA16                  *(DU16*)(iodata)
#endif

#define IO_RE_CHECK
#define RX_FIFO_CHECK
#define BROADCAST_JUMP
#define BROADCAST_MAX               128

static unsigned int PTR_TMP, PTR_TMP1;/**/

#define udelay(ms)                  task_delay(ms * ST_GetClocksPerSecondLow() / 1000); //us delay
#define sdelay(ms)                  task_delay(ms * ST_GetClocksPerSecondLow());

#ifdef TASK_MODE
    #ifdef USE_INIT_TASK
        static task_t EtherNetTaskId;
        static tdesc_t EtherNetTaskId_desc;
    #else
        static task_t * Task;
    #endif
    extern u8_t smsc_print_out;
    static semaphore_t  CopyGo;
    static semaphore_t sem_tx_rx_lock;
    /*static U32 rows_num;  */
#endif

/*--------- globales variables ---------------------------------------------*/
/* static NGbuf * bufpstatic;   the buffer to be send on the IRQ handler*/
static U32 pktsize;    /* to save the packet size for stat*/
/*Spenser-----------------------------------------*/
//U32 iobase = 0x40000000/*STETHER_Ethernet_Base_Address*/;
//U32 iodata = 0x40000004/*STETHER_Ethernet_Base_Address + 4*/;
#define iobase                      0x40000000
#define iodata                      0x40000004
/*static U8 mac_addr[6];*/
static U8 tx_pkt_cnt;
static U8 dm9000_iomode;
static int queue_pkt_len;
static U8 device_wait_reset;
static U8 device_new_ver = 0;
static U8 device_imr_set = IMR_DEFAULT;
static U16 device_debug = 0;
static U16 device_rx_max = 0;
U8 DM_MAC_ADDRESS[6];

U32 STETHER_Ethernet_Base_Address = 0x40000000;
U16 STETHER_Address_Shift;
U16 STETHER_DM9000_Interrupt;
U16 STETHER_Interrupt_Level;
U16 STETHER_USE16bit;
U8  STETHER_TRANS_Len;

#define DM9KS_VID_L                 0x28
#define DM9KS_VID_H                 0x29
#define DM9KS_PID_L                 0x2A
#define DM9KS_PID_H                 0x2B

#define DM9KS_NCR                   0x00 /* Network control Reg.*/
#define DM9KS_FCR                   0x0a

#define DM9KS_ID                    0x90000A46
#define DM9010_ID                   0x90100A46
#define DM9003_ID                   0x90030A46

#define DM9KS_EPDRL                 0x0d
#define DM9KS_EPDRH                 0x0e

/* DM9KS_IMR */
#define IMR_SRAM_autoReturn         (1<<7)
#define IMR_Link_change             (1<<5)
#define IMR_TX_underrun             (1<<4)
#define IMR_RX_OFcnt_overflow       (1<<3)
#define IMR_RX_Overflow             (1<<2)
#define IMR_TX_complete             (1<<1)
#define IMR_RX_coming               1

U8 dmfe_port_mode = 0;

#define DM9KS_MONITOR2              0x41

#define DM9KS_REGFF                 (IMR_SRAM_autoReturn | IMR_TX_complete | IMR_RX_coming)
#define DM9KS_NO_RX_INTR            (IMR_SRAM_autoReturn | IMR_TX_complete)
#define DM9KS_DISINTR               IMR_SRAM_autoReturn

/* switch's register and setting */
#define DM9KS_SCR                   0x52
#define DM9KS_SCR_ResetSwitch       (1<<6)
#define DM9KS_VLANCR                0x53
#define DM9KS_PIndex                0x60
#define DM9KS_PCTRL                 0x61
#define DM9KS_PRATE                 0x66
#define DM9KS_PBW                   0x67
#define DM9KS_PPRI                  0x6D
#define DM9KS_VLAN_TAGL             0x6E

/* DM9KS_NCR */
#define NCR_MAC_loopback            2
#define NCR_Reset                   1


/*-------- prototypes of locals functions ----------------------------------*/
static void STETHER_InterruptHandler(void * netp);
/*  static ST_ErrorCode_t STETHER_phy_configure(void);      */
static int STETHER_Open(void * netp);
static void STETHER_setMulticast(void * netp);
static U32 STETHER_MacLoad(void * netp);
/*Spenser------------------------------------------------------*/
static U16 read_srom_word(int offset);
/*static unsigned long cal_CRC(unsigned char * Data, unsigned int Len, U8 flag);*/
static int dmfe_start_xmit(void * buf, u16_t data_len);
static void dmfe_packet_receive(void* netp);
static void STETHER_CopyRx_Task(void * netp);
static void STETHER_Rx_Task(void * netp);
static void STETHER_Tx_Task(void * netp);

/*---------- FUNCTIONS IN THIS DERIVER -------------------------------------*/

#define u16                         U16
#define u8                          U8
#define u32                         U32

void outb( U8 bRegvsData, U32 b4Addr)
{
    *((U8 *) b4Addr) = bRegvsData;
}

U8 inb( U32 b4Addr )
{
    U8 bRtn;
    bRtn = *(U8 *)b4Addr;
    return ( bRtn );
}

U16 inw ( U32 b4Addr )
{
    U16 bRtn;
    bRtn = *(U16 *)b4Addr;
    return ( bRtn );
}

void outw( U16 b2RegvsData, U32 b4Addr)
{
    *((U16 *) b4Addr) = b2RegvsData;
}

#ifdef IO_RE_CHECK
void write_index_check(U8 reg)
{
    U8 re_try;
    
    for(re_try = 10; re_try > 0; re_try--)
    {
        outb(reg, iobase);
        if(reg == inb(iobase)) break;
    }
}
#endif

U8 ior(U8 reg)
{
#ifdef IO_RE_CHECK
    U8 re_check_data;
    U8 re_try;

    write_index_check(reg);
        
    for(re_try = 10; re_try > 0; re_try--)
    {
        re_check_data = (U8) inb(iodata);
        if(re_check_data == inb(iodata)) break;
    }
    
    return((U8) re_check_data);
#else    
    outb(reg, iobase);
    return ((U16) inb(iodata));
#endif
}

void iow(U8 reg, U8 RegData)
{
#ifdef IO_RE_CHECK
    U8 re_try;

    write_index_check(reg);

    for(re_try = 10; re_try > 0; re_try--)
    {
        outb(RegData, iodata);
        if(RegData == inb(iodata)) break;
    }    
#else
    outb(reg, iobase);
    outb(RegData, (iodata));
#endif
}

void iow_one_time(U8 reg, U8 RegData)
{
#ifdef IO_RE_CHECK
    U8 re_try;

    write_index_check(reg);

    outb(RegData, iodata);
#else
    outb(reg, iobase);
    outb(RegData, (iodata));
#endif
}

U8 ior_one_time(U8 reg)
{
#ifdef IO_RE_CHECK
    U8 re_try;

    write_index_check(reg);
   
    return((U8) inb(iodata));

#else
    outb(reg, iobase);
    return ((U16) inb(iodata));
#endif
}

void tcr_check(void)
{
    if(!(ior(0x2) & 0x01))
    {
//        tx_pkt_cnt--;
        tx_pkt_cnt = 0;
        iow_one_time(0xfe, DM9000_TX_INTR);
    }        
}


u8 MAKE_MASK(int p3, int p2, int p1, int p0)
{
    p3 = p3 ? 1 : 0;
    p2 = p2 ? 1 : 0;
    p1 = p1 ? 1 : 0;
    p0 = p0 ? 1 : 0;
    
    return (p3 << 3) | (p2 << 2) | (p1 << 1) | p0;
}

static void vlan_group_map(u8 reg_group, u8 mapping)
{
    iow(reg_group, mapping);
}


static int vlan_port_vid( int port, u8 VID)
{
    if ((port < 0) || (port > 3))
    {
        do_report8(0, "<DM9SW>Port number error\n ");
        return 1;
    }
    
    iow(DM9KS_PIndex, port);
    
    iow(DM9KS_VLAN_TAGL, VID);
    return 0;
}

/*
   Read a word from phyxcer
*/
static u16 phy_read_sw( int reg, int port)
{
    u8 re_try = 10;

    reg = (port << 6) | reg;
    /* Fill the phyxcer register into REG_0C */
    iow(0xc, reg);
    
    iow(0xb, 0x8);  /* Clear phyxcer read command */
    iow_one_time(0xb, 0xc);  /* Issue phyxcer read command */
    while(re_try-- && (ior(0x0b) & 0x01)) 
    {
        udelay(10);
    }
    iow(0xb, 0x0);  /* Clear phyxcer read command */

    /* The read data keeps on REG_0D & REG_0E */
    return (ior(DM9KS_EPDRH) << 8) | ior(DM9KS_EPDRL);
    
}

/*
   Write a word to phyxcer
*/
static void phy_write_sw( int reg, u16 value, int port)
{
    u8 re_try = 10;

    reg = (port << 6) | reg;
    /* Fill the phyxcer register into REG_0C */
    iow(0xc, reg);
    
    /* Fill the written data into REG_0D & REG_0E */
    iow(0xd, (value & 0xff));
    iow(0xe, ((value >> 8) & 0xff));
    
    iow(0xb, 0x8); /* Clear phyxcer write command */
    iow_one_time(0xb, 0xa); /* Issue phyxcer write command */
    while(re_try-- && (ior(0x0b) & 0x01)) 
    {
        udelay(30);
    }
    iow(0xb, 0x0);  /* Clear phyxcer read command */  
}


void dm9sw_timer(void)
{
    U8 reg_save;
    U16 tmp_d;
    U8 re_try;

    reg_save = inb(iobase);
    
    if(device_debug)
    {
        do_report8(0, "@@@ jump broadcast packet  %d \n", device_debug);
        device_debug = 0;
    }

    iow(0x60, 0x00);
//    do_report8(0, "@@@ port0 link %x \n", ior(0x62));
//    iow(0x60, 0x01);
//    do_report8(0, "@@@ port1 link %x \n", ior(0x62));

    iow_one_time(0x80, 0x03);
    for(re_try = 10; re_try > 0; re_try--)
        if(ior(0x80) & 0x80) break;

    tmp_d = ior_one_time(0x81);
    if(tmp_d && re_try)
    {
        do_report8(0, "@@@ rx discast %x \n", tmp_d);
//
        iow_one_time(0x80, 10);
        for(re_try = 10; re_try > 0; re_try--)
            if(ior(0x80) & 0x80) break;
        
        tmp_d = ior_one_time(0x81);
        if(tmp_d && re_try) do_report8(0, "@@@ rx crc_err %x \n", tmp_d);
//
        iow_one_time(0x80, 11);
        for(re_try = 10; re_try > 0; re_try--)
            if(ior(0x80) & 0x80) break;
        
        tmp_d = ior_one_time(0x81);
        if(tmp_d && re_try) do_report8(0, "@@@ rx UdSize %x \n", tmp_d);
//
        iow_one_time(0x80, 12);
        for(re_try = 10; re_try > 0; re_try--)
            if(ior(0x80) & 0x80) break;
        
        tmp_d = ior_one_time(0x81);
        if(tmp_d && re_try) do_report8(0, "@@@ rx OvSize %x \n", tmp_d);
    }

    
    iow_one_time(0x80, 0x13);
    for(re_try = 10; re_try > 0; re_try--)
        if(ior(0x80) & 0x80) break;
    
    tmp_d = ior_one_time(0x81);
    if(tmp_d && re_try)
    {
        do_report8(0, "@@@ port_status link %x \n", ior(0x62));
        do_report8(0, "@@@ rx Rx_abort %x \n", tmp_d);
//
        iow_one_time(0x80, 14);
        for(re_try = 10; re_try > 0; re_try--)
            if(ior(0x80) & 0x80) break;
        
        tmp_d = ior_one_time(0x81);
        if(tmp_d && re_try) do_report8(0, "@@@ rx RX_Pause %x \n", tmp_d);
//
        iow_one_time(0x80, 19);
        for(re_try = 10; re_try > 0; re_try--)
            if(ior(0x80) & 0x80) break;
        
        tmp_d = ior_one_time(0x81);
        if(tmp_d && re_try) do_report8(0, "@@@ rx TX_Pause %x \n", tmp_d);
    }        

    outb(reg_save, iobase);

}

/*

void io_debug(void)
{
#define STI5100_FMI_BANK0_DATA0            (0x20200100)
#define STI5100_FMI_BANK0_DATA1            (0x20200108)
#define STI5100_FMI_BANK0_DATA2            (0x20200110)
#define STI5100_FMI_BANK0_DATA3            (0x20200118)

    U32 io_cnt = 0;
    U32 io_add_set = 0;
    U8 err_cnt1, err_cnt2, err_cnt3, err_cnt4, err_cnt5, err_cnt6;
    U16 err_cnt;
    U8 tmp_d;

    do_report8(0, "@ io_debug @ default config_0  %x \n",  STSYS_ReadRegDev32LE((void*)(STI5100_FMI_BANK0_DATA0)));
    do_report8(0, "@ io_debug @ default config_1  %x \n",  STSYS_ReadRegDev32LE((void*)(STI5100_FMI_BANK0_DATA1)));
    do_report8(0, "@ io_debug @ default config_2  %x \n",  STSYS_ReadRegDev32LE((void*)(STI5100_FMI_BANK0_DATA2)));

    for(io_add_set = 0x8c142511; io_add_set < 0xb2142511; io_add_set += 0x4000000)
    {
                
        STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA1,io_add_set);
        STSYS_WriteRegDev32LE(STI5100_FMI_BANK0_DATA2,io_add_set);
        
        do_report8(0, "@ io_debug @ edit config_1_2 to %x \n", STSYS_ReadRegDev32LE((void*)(STI5100_FMI_BANK0_DATA1)));
    
    for(io_cnt = 0, err_cnt = 0; io_cnt < 0x600000; io_cnt++)
    {
        
        if(0 == (io_cnt % 0x100000)) do_report8(0, "@ io_debug @ io_debug time %x \n", io_cnt);

        iow(0xff, 0x80);
        tmp_d = inb(iobase);
        if(tmp_d != 0xff) 
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 1 w index error check 0xff, %x \n", tmp_d);
        }
        tmp_d = ior(0xff);
        if(tmp_d != 0x80)
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 2 w data error check 0x80, %x \n", tmp_d);
        }

        iow(0xff, 0x83);
        tmp_d = inb(iobase);
        if(tmp_d != 0xff) 
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 3 w index error check 0xff, %x \n", tmp_d);
        }
        tmp_d = ior(0xff);
        if(tmp_d != 0x83)
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 4 w data error check 0x83, %x \n", tmp_d);
        }

        iow(0x10, 0x5a);
        tmp_d = ior(0x10);
        if(tmp_d != 0x5a)
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 5 w data error check 0x5a, %x \n", tmp_d);
        }

        iow(0x10, 0xa5);
        tmp_d = ior(0x10);
        if(tmp_d != 0xa5)
        {
            err_cnt++;
            do_report8(0, "@ io_debug @ istep 6 w data error check 0xa5, %x \n", tmp_d);
        }

        if(err_cnt > 10) break;
    }
    }

}

*/


/*---------------------------------------------------------------------------
 Name  : set_PHY_mode
 Description : PHY setting
 Author  : Spenser
-----------------------------------------------------------------------------*/
void set_DM9003_PHY_mode(void)
{
    int i;
// u16 phy_reg0 = 0x1200;// Auto-negotiation & Restart Auto-negotiation
// u16 phy_reg4 = 0x01e1;// Default flow control disable

    for (i = 0; i < (dmfe_port_mode - 1); i++) /* reset PHY */
    {
        phy_write_sw( 0, 0x8000, i);
//      wait_PHY_bit_clear(i, 0, 15);
        udelay(300);
    }
    
    do_report8(0, "wait to link :");
    for(i = 0; i < 100; i++)
    {
        iow(0x60, i & 0x01);
        if(ior(0x62) & 0x01) break;
        do_report8(0, ".");
        sdelay(3); //3s 
    }
    iow(0x60, 0x00);
    do_report8(0, "@@@ port0 link %x \n", ior(0x62));
    iow(0x60, 0x01);
    do_report8(0, "@@@ port1 link %x \n", ior(0x62));
       
}

static void Smsc911x_EthernetOutput(
    struct NETWORK_INTERFACE * networkInterface,
    PPACKET_BUFFER packet)
{
    SMSC911X_PRIVATE_DATA * privateData;
    SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(networkInterface != NULL);
    CHECK_SIGNATURE(networkInterface, NETWORK_INTERFACE_SIGNATURE);
    privateData = networkInterface->mHardwareData;
    SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(privateData != NULL);
    CHECK_SIGNATURE(privateData, SMSC911X_PRIVATE_DATA_SIGNATURE);
    SMSC_FUNCTION_PARAMS_CHECK_RETURN_VOID(packet != NULL);
    SMSC_ASSERT(packet->mNext == NULL);/* We only accept one frame at a time */
    SMSC_TRACE(SMSC911X_DEBUG, ("Smsc911x_EthernetOutput[%p]", packet));
    
    PacketQueue_Push(&(privateData->mTxPacketQueue), packet);
    
    if (Task_IsIdle(&(privateData->mTxTask)))
    {
        TaskManager_ScheduleAsSoonAsPossible(&(privateData->mTxTask));
    }
}

err_t Smsc911x_InitializeInterface(struct NETWORK_INTERFACE * networkInterface, int interfaceIndex)
{
    return STETHER_Open(networkInterface);
}

extern SMSC911X_PRIVATE_DATA * Smsc911xPlatform_Initialize(int interfaceIndex);


/*===========================================================================
 Name    :  STETEHR_Open
 Description :  Open and initialise the card
      Set up everything, reset the card, etc ...
 Return   :  0 on success, 1 if an error occur.
  ==========================================================================*/
static int STETHER_Open(void *netp)
{

    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    U32 id_val;
    SMSC911X_PRIVATE_DATA * privateData;
    static U32 err = 0;
    u8_t macAddress[6];
    mem_ptr_t ioAddress;
    u8 i;
    
    /*is interface already running?*/
    
    if ((NetInterface->mFlags)&NETWORK_INTERFACE_FLAG_LINK_UP)
    {
        return(1);
    }
    
    smsc_semaphore_initialize(&CopyGo, 0);
    smsc_semaphore_initialize(&sem_tx_rx_lock, 1);
	
    SMSC_FUNCTION_PARAMS_CHECK_RETURN(NetInterface != NULL,-1);
    CHECK_SIGNATURE(NetInterface, NETWORK_INTERFACE_SIGNATURE);
    
    SMSC_NOTICE(SMSC911X_DEBUG, ("Initializing SMSC911X interface, for use with the SMSC IP Stack"));
    SMSC_NOTICE(SMSC911X_DEBUG, ("Driver Compiled: %s, %s", __DATE__, __TIME__));
    
    if ((privateData = Smsc911xPlatform_Initialize(0)) == NULL)
    {
        SMSC_ERROR(("Smsc911x_InitializeInterface: Failed to initialize platform"));
        return 1;
    }
    
    Smsc911xPlatform_DisplayResources(SMSC911X_DEBUG, NetInterface);
    
    ioAddress = iobase;
    
    memset(privateData, 0, sizeof(*privateData));
    ASSIGN_SIGNATURE(privateData, SMSC911X_PRIVATE_DATA_SIGNATURE);
    privateData->mIoAddress = ioAddress;
    privateData->mInterfaceIndex = 0;
    NetInterface->mHardwareData = privateData;
    
    STETHER_Config(iobase, 0, 64, 9, 0, 2);
    /*set the LINK  flags*/
    NetInterface->mFlags = NETWORK_INTERFACE_FLAG_LINK_UP;
#ifdef TASK_MODE/*bu用异步接收数据方式*/
#if 1
    /* Initialize Rx Task */
    Task_Initialize(&(privateData->mRxTask),
                    TASK_PRIORITY_RX, STETHER_Rx_Task, NetInterface);
                    
    /* Schedule Rx Task on Interrupt activation */
    TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));/**/
#ifdef CH_RX_QUEUE
    PacketQueue_Initialize(&(privateData->mRxPacketQueue));
#endif
//#else
    Task = task_create(STETHER_CopyRx_Task, NetInterface, 1024 * 10, 15, "TCPIP_receive_task", (task_flags_t) 0);
    
    if (Task == NULL)
    {
        return -1;
    }
    
#endif
    
#endif
    /*Spenser---------------------------------------------------------*/
    
    id_val  = ior(DM9KS_VID_L);
    id_val |= ior(DM9KS_VID_H) << 8;
    id_val |= ior(DM9KS_PID_L) << 16;
    id_val |= ior(DM9KS_PID_H) << 24;
     
    if (id_val == DM9003_ID)
    {
        init_dm9003();
            
        /*Set the Mac address*/
            
        err = STETHER_MacLoad(netp);
            
#if 0 /*zlming081016 change */
        /* set Multicast table */
        STETHER_setMulticast(netp);
#else
    #if 1
        for(i = 0x16; i < 0x1d; i++)
        {
            iow(i, 0x0);
        }
        iow(0x1d, 0x80);
    #else
        for(i = 0x16, i < 0x1e, i++)
        {
            iow(i, 0xff);
        }
    #endif
#endif
            /* Set PHY */
        set_DM9003_PHY_mode();
            
    }
    else
    {
        do_report(0, "NONE ETH IC!\n");
    }
        
        
#ifdef AUTO_RELEASE
        
    /* automatically  release successfully transmitted packets */
    
#endif
    
#ifdef LOOP_BACK
    iow(0, 0x02); /*MAC internal loop back.*/
#endif
    
    /*install the interupt handler */
#if 0
    err = interrupt_install(STETHER_DM9000_Interrupt, STETHER_Interrupt_Level, (void(*)(void * netp))STETHER_InterruptHandler, (void *)netp);
    
    err = interrupt_trigger_mode_number(STETHER_DM9000_Interrupt, interrupt_trigger_mode_falling);
    
    err = interrupt_enable_number(STETHER_DM9000_Interrupt);
    
    interrupt_enable_global();
    
    err = interrupt_enable(STETHER_Interrupt_Level); /*FC: enabled Interrupt level because in stboot.c is disabled for backward compatibility*/
    
#else
    {
        int irq_levl = 4;
        int irq_num = 64;
    
        interrupt_disable(irq_levl);/**/
    
        if (interrupt_install((int)irq_num,
                              (int)irq_levl,
                              (void(*)(void*))STETHER_InterruptHandler,
                              (void*)netp) == 0)
        {
    
            printf("<DM9KS>:DM9000AE interrupt_install!\r\n");
            interrupt_trigger_mode_number(irq_num, interrupt_trigger_mode_low_level/*interrupt_trigger_mode_falling*/);/* interrupt_trigger_mode_high_level */
            printf("<DM9KS>:DM9000AE interrupt_trigger_mode_number!\r\n");
            interrupt_enable_number(irq_num);
            printf("<DM9KS>:DM9000AE interrupt_enable_number!\r\n");
            interrupt_enable_global();
            printf("<DM9KS>:DM9000AE interrupt_enable_global!\r\n");
            interrupt_enable(irq_levl);
            printf("<DM9KS>:DM9000AE interrupt_enable!\r\n");
        }
        else
        {
            return -1;
        }
    
    }
    
#endif
    /* enables IRQs */
    
    
    /* Activate DM9000 *//*zlming081016 change */

    iow(0x5, 0x01); /* RX enable */
    
    iow_one_time(0xfe, 0xff);/*zlming081016 change */
    
    /* By Anky */
    iow(0xff, device_imr_set);  /* Enable TX/RX interrupt mask */
    
    /* Initialize Tx Task */
    Task_Initialize(&(privateData->mTxTask),
                    TASK_PRIORITY_TX, STETHER_Tx_Task, NetInterface);
                    
    PacketQueue_Initialize(&(privateData->mTxPacketQueue));
    
    SMSC_ASSERT(MINIMUM_MTU <= 1500);
    
    NetInterface->mMTU = 1500;
    
    sprintf(NetInterface->mName, "SMSC_%d", 0);
    
    SMSC_ASSERT(strlen(NetInterface->mName) < NETWORK_INTERFACE_NAME_SIZE);
    
    Ethernet_InitializeInterface(
        NetInterface,
        Smsc911x_EthernetOutput,
        DM_MAC_ADDRESS,
        ETHERNET_FLAG_ENABLE_IPV4 | ETHERNET_FLAG_ENABLE_IPV6);
        
    /*do_report(0,"print() OK!\n");*/
    PTR_TMP = 0;
    
    return(err);
}


static void dm9sw_switch_reset(void)
{
    int i;
    /* reset VLAN mapping table */
    
    for (i = 0xb0; i <= 0xbf; i++)
        vlan_group_map(i, MAKE_MASK(1, 1, 1, 1));
        
    /* reset Per-Port VID*/
    for (i = 0; i < 4; i++)
        vlan_port_vid(i, 1);
        
    /* reset VLAN control */
    iow (DM9KS_VLANCR, 0);
    
    /* reset Per-Port switch control */
    for (i = 0; i < 4; i++)
    {
        iow( DM9KS_PIndex, i);
        iow( 0x61, 0);
        iow( 0x66, 0);
        iow( 0x67, 0);
        iow( 0x6D, 0);
        iow( 0x6F, 0);
    }
    
}

static void dm9sw_switch_config(void)
{
    dm9sw_switch_reset();
    
    /* switch control */
#if 0
    VLAN_SETUP(db, 1); /* 0:port-base, 1:Tag-base */
#endif
#if 0
    QoS_SETUP(db, 0); /* 0:port, 1:Tag, 2:TOS */
#endif
#if 0
    /* Bandwidth control
     dm9k_bw_control(db,port_no, bw_type, bit7-4, bit3-0)
     bw_type(reg61H.[3]): 0:control with Ingress and Egress separately
                   1:control with Ingress or Egress
     bit7-4 and bit 3-0:refer to reg66H and reg67H
     If bw_type=1, bit7-4 must be "0".
    */
    dm9k_bw_control(db, 1, 0, 0, 2);
    dm9k_bw_control(db, 0, 0, 2, 0);
#endif
}

static void dm9sw_soft_reset(void)
{
    
    SMSC_ERROR(("@@@ device_reset \n"));

    /* software reset */
    iow_one_time(DM9KS_NCR, NCR_Reset);
    udelay(300);

    iow(0xff, IMR_DIS_ALL);

    /* Program operating register */
    iow(DM9KS_NCR, 0x20); /* reg.01 cleared by write 1 */
    
//    if(device_new_ver)
    if(1)
    {
        iow(DM9KS_FCR, 0x20); /* RX Flow Control Enable */
    }
    else
    {
        iow(DM9KS_FCR, 0x00); /* RX Flow Control Disable */
    }

#if defined(CHECKSUM)
    printk("<DM9SW>Enable checksum offload \n");
    /* TX checksum enable */
    iow(db, DM9KS_TCCR, TCCR_UDP_Chksum | TCCR_TCP_Chksum | TCCR_IP_Chksum);
    /* RX checksum enable */
    iow(db, DM9KS_RCSR, RCSR_RX_Chksum_enable);
#endif

    tx_pkt_cnt = 0;
    device_imr_set = IMR_DEFAULT;
    device_wait_reset = FALSE;

    iow_one_time(0xfe, 0xff);/*zlming081016 change */
    iow(0x5, 0x01); /* RX enable */

//    iow(0xff, device_imr_set);  /* Enable TX/RX interrupt mask */
}

/*
 Initilize dm9000 board
*/
ST_ErrorCode_t init_dm9003(void)
{

    do_report(0, "<DM9KS>:DM9003E!\n");

    device_new_ver = ior(0x2C);
    if(device_new_ver < 1) iow(0x3f, 0x14);
//    if(device_new_ver < 1) iow(0x3f, 0x20);
    // bval = (64 - (bval + 1)) << 8; ==> 64 - 21 = 43 , 0x2b << 8 = 101011 0000 0000
    device_rx_max = (64 - ((ior(0x3f) & 0x3f) + 1)) << 8;
    do_report8(0, "uP RX FIFO %x\n", device_rx_max);

    /* switch reset */
    iow(DM9KS_SCR, DM9KS_SCR_ResetSwitch);
    udelay(300);
    
    iow(0x59, 0xe1);
    iow(0x38, 0x61);
//    do_report8(0, "reg 0x39 = %x\n", ior(0x39));
    iow(0x39, 0x01);

    /* software reset */
    iow(DM9KS_NCR, NCR_Reset);
    udelay(300);

    /* I/O mode */
    dm9000_iomode = ior(0xfe) >> 6; /* ISR bit7:6 keeps I/O mode */
    
    /* DM9013 is 2 port mode or 3 port mode */
    dmfe_port_mode = (ior(DM9KS_MONITOR2) & (1 << 6)) ? 2 : 3;
    
    do_report8(0, "dmfe_port_mode = %x\n", dmfe_port_mode);

    dm9sw_soft_reset();
    iow(0x5, 0x00); /* RX disable */
    
    dm9sw_switch_config();
    
    /* Initial Global variable */
    
    return 0;
    
}


/*-------------------------------------------------------------------
 Name  : read_srom_word
 Description : Read a word data from SROM
 Parameters : offset = Word offset of eeprom.
 Author  : Spenser
--------------------------------------------------------------------*/
U16 read_srom_word(int offset)
{
    u8 re_try = 10;
    u8 reg;

    reg = offset;
    /* Fill the phyxcer register into REG_0C */
    iow(0xc, reg);
    
    iow(0xb, 0x8);  /* Clear phyxcer read command */
    iow_one_time(0xb, 0x4);  /* Issue phyxcer read command */
    while(re_try-- && (ior(0x0b) & 0x01)) 
    {
        udelay(10);
    }
    iow(0xb, 0x0);  /* Clear phyxcer read command */

    /* The read data keeps on REG_0D & REG_0E */
    return (ior(DM9KS_EPDRH) << 8) | ior(DM9KS_EPDRL);
}


#define MAXIMUM_ADDITIONAL_FIFO_SPACE (52)
extern SMSC911X_PRIVATE_DATA tmp_SMSC911X_PRIVATE_DATA;
/*===========================================================================
 Name    : STETHER_Start
 Description :  called by functions in the higher layer
      to start sending a packet
 Modify  : Spenser
 ============================================================================*/
static void STETHER_Tx_Task(void * netp)
{

    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    SMSC911X_PRIVATE_DATA * privateData;
    mem_ptr_t ioAddress;
    PPACKET_QUEUE queue;
    u32_t tx_cmd_a, tx_cmd_b;
    u16_t length, transmitCount = 0;
    
    privateData = NetInterface->mHardwareData;
    
    if (/*( netp->if_flags) & NG_IFF_OACTIVE*/0)
    {
        /* interface running*/
        return;
    }
    
    else
    {
        /* set active flags*/
        /*netp->if_flags|=NG_IFF_OACTIVE;*/
        
        queue = &(privateData->mTxPacketQueue);
        
        if (queue == NULL)
        {
            /* Interface is now busy */
            /*(netp->if_flags)&=~(NG_IFF_OACTIVE); */
            /*break;*/
            SMSC_ERROR(("$$$  NULL QUEUE \n"));
            return;
        }
        
//        while (/*(transmitCount<SMSC911X_TX_BURST_COUNT)&&*//* Am I allowed to transmit? */
//            transmitCount < 1 && (queue->mHead != NULL) /* Is a packet available? */
//            /*&&(((queue->mHead->mThisLength)+MAXIMUM_ADDITIONAL_FIFO_SPACE)<=fifoFreeBytes)*/)
//        while ((queue->mHead != NULL) && (0 == tx_pkt_cnt) && (transmitCount < 1))
//        while ((queue->mHead != NULL) && (0 == tx_pkt_cnt) && (transmitCount < 3))
        while ((queue->mHead != NULL) && (transmitCount < 5))
        {
            /*allocation success, now send the packet */
            PPACKET_BUFFER packet = PacketQueue_Pop(queue);
            mem_ptr_t startAddress = (mem_ptr_t)PacketBuffer_GetStartPoint(packet);
            u16_t offset = (u16_t)(startAddress & 0x03);
            u16_t whole_dwords = (packet->mThisLength + 3 + offset) >> 2;
            
            length = (u16_t)(packet->mThisLength);
            /*tx_cmd_b = ((u32_t)length) << 16;
            tx_cmd_b |= (u32_t)length;*/
            SMSC_TRACE(SMSC911X_DEBUG, ("OUTPUT_PACKET.thisLen = [%d],totalLen = [%d]", packet->mThisLength, packet->mTotalLength));
            /*do_report(0,"OUTPUT_PACKET.thisLen = [%d],totalLen = [%d]\n",packet->mThisLength,packet->mTotalLength);*/
            /*ch_vod_dbgprintdata("STACK SEND DATA",startAddress,length);*/
            
            if (!dmfe_start_xmit((void *)startAddress, length/*(void *)(startAddress&(~((mem_ptr_t)(0x03)))),whole_dwords*/))
            {
                // PacketQueue_UnPop(queue,packet);
                SMSC_ERROR(("$$$ T err pkt\n"));
                break;
            }
            
            /* we are done with this packet so lets decrease its reference */
            PacketBuffer_DecreaseReference(packet);
            
            //task_delay(ST_GetClocksPerSecond()/1000);
            transmitCount++;
            
            //do_report(0,"SEND length  =%d \n",length);
        } ;
        
        if (!PacketQueue_IsEmpty(queue))
        {
#if 1
            /* Packets are still available */
            
            if (((PacketQueue_Peek(queue)->mThisLength) + MAXIMUM_ADDITIONAL_FIFO_SPACE))
            {
                /* We have enough fifo space available */
                /* Reschedule this task to run as soon as possible */
                TaskManager_ScheduleAsSoonAsPossible(&(privateData->mTxTask));
            }
            else
            {
                /* We do not have enough fifo space available */
                /*   Schedule for interrupt activation */
                TaskManager_ScheduleByInterruptActivation(&(privateData->mTxTask));
                /* Enable Tx Interrupt */
                /*Smsc911x_SetTDFL(ioAddress,0x32);*/
            }
#else
            TaskManager_ScheduleByInterruptActivation(&(privateData->mTxTask));
#endif
            
        }
        else
        {
        
            // dmfe_packet_receive(netp);
            /* No packets are available yet */
            /* exit with out rescheduling */
            /* This task will be rescheduled when packets are pushed on the queue
              see Smsc911x_EthernetOutput */
            //SMSC_ASSERT(Task_IsIdle(&(privateData->mTxTask)));
        }
    }
}

/*-----------------------------------------------------------------------------
 Name : dmfe_start_xmit
 Description : Hardware start transmission. Send a packet to media from the upper layer.

--------------------------------------------------------------------------------*/
static int dmfe_start_xmit(void * buf, u16_t data_len)
{
    char * data_ptr;
    int i;
    int sent_pkt_len;
    int tmplen;
    U8 re_try = 5;
    
#ifdef MOVE2D
    U16 rows_nu;    /* number of rows to be copied using mode2d*/
#endif
    U8 reg_save;
 
    smsc_semaphore_wait(&sem_tx_rx_lock);

    reg_save = inb(iobase);
    
    while(tx_pkt_cnt && re_try--)
    {
        tcr_check();
        udelay(1)
    }

    if (tx_pkt_cnt || (data_len < 32))
    {
        do_report8(0, "dmfe_start_xmit tx_pkt_cnt %x data_len %x \n", tx_pkt_cnt, data_len); 
        SMSC_ERROR(("Can't output packet!\n"));
        device_wait_reset = TRUE;
        smsc_semaphore_signal(&sem_tx_rx_lock);
        outb(reg_save, iobase);
        return 0; /**/
    }
    
//    do_report8(0, "dmfe_start_xmit data_len = %x \n", data_len); 

    /* Disable all interrupt */
    iow(0xff, IMR_DIS_ALL);

//for debug
#if 0
    io_debug();
#endif
    data_ptr = (char *)buf;
    sent_pkt_len = data_len;

    /* Set TX length to DM9000 */
    iow(0xfc, sent_pkt_len & 0xff);
    iow(0xfd, (sent_pkt_len >> 8) & 0xff);

    /* Move data to DM9000 TX RAM */
    
#ifdef IO_RE_CHECK    
    write_index_check(0xf8);
#else
    outb(0xf8, iobase);
#endif
    /* Word mode */
//    rows_nu = ((sent_pkt_len + 1) / STETHER_TRANS_Len);  /*anky add 1.*/
    tmplen = (sent_pkt_len + 1) / 2;
#ifdef MOVE2D
//    move2d_all((buf), (void*)(&DATA16), STETHER_TRANS_Len, rows_nu, STETHER_TRANS_Len, 0);
    move2d_all((buf), (void*)(&DATA16), STETHER_TRANS_Len, tmplen, STETHER_TRANS_Len, 0);
#else
    for (i = 0; i < tmplen; i++)
        outw(((U16 *)data_ptr)[i], iodata);
#endif
    
#if DEBUG_DM_DRIVER
    DM_dprintf("Driver send:");
    
    for (i = 0;i < sent_pkt_len;i++)
    {
        if ((i % 20) == 0)
            DM_dprintf("\n");
            
        DM_dprintf("0x%02x  ", ((unsigned char *)buf)[i]);
    }
    
    DM_dprintf("\n\n");
    
#endif
       
    /* Issue TX polling command */
    iow_one_time(0x2, 0x1);  /* Cleared after TX complete */
//    tx_pkt_cnt++;
    tx_pkt_cnt = 1;

    /*save data length to use it in stat*/
    pktsize = sent_pkt_len;

    /* Update multicast counters
      if (bufp->buf_flags & (NG_BUFF_BCAST|NG_BUFF_MCAST))
      {
       netp->if_omcasts++;
      } */
    
#if 1
    udelay(1)

    tcr_check();
#endif

    /* Re-enable interrupt*/
    iow(0xff, device_imr_set);

    outb(reg_save, iobase);

    smsc_semaphore_signal(&sem_tx_rx_lock);

    return sent_pkt_len;
}



/*============================================================================
 Name    : STETHER_CopyData
 Description : Copy Data from chip to  NexGen buffer.
 ============================================================================*/
static void STETHER_Rx_Task(void * netp)
{

#ifdef CH_RX_QUEUE
    {
    
        struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
        SMSC911X_PRIVATE_DATA * privateData;
        mem_ptr_t ioAddress;
        PPACKET_QUEUE queue;
        u32_t tx_cmd_a, tx_cmd_b;
        u16_t length, RevCount = 0;
        
        privateData = NetInterface->mHardwareData;
        queue = &(privateData->mRxPacketQueue);
        
        TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));
        
        if (queue == NULL)
        {
            return;
        }
        
        while (
            RevCount < 2  && (queue->mHead != NULL) /* Is a packet available? */)
        {
            /*allocation success, now send the packet */
            PPACKET_BUFFER packet = PacketQueue_Pop(queue);
            //mem_ptr_t startAddress=(mem_ptr_t)PacketBuffer_GetStartPoint(packet);
            
            SMSC_TRACE(SMSC911X_DEBUG, ("Ethernet_Input packet[%p] len[%d]\n", packet, packet->mThisLength));
            /*enqueue the message*/
            Ethernet_Input(NetInterface, packet);
            // do_report(0,"Ethernet_Input out\n");
            /* we are done with this packet so lets decrease its reference */
            //PacketBuffer_DecreaseReference(packet);
            RevCount++;
            
        } ;
        
        if (!PacketQueue_IsEmpty(queue))
        {
            // TaskManager_ScheduleAsSoonAsPossible(&(privateData->mRxTask));
            //  TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));
            TaskManager_InterruptActivation(&(privateData->mRxTask));
        }
        else
        {
        
        }
    }
#else
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    while (1)
    {
        smsc_semaphore_wait(&CopyGo);
        //task_lock();/**/
        /*interrupt_lock();*/
        /*do_report(0,"Rx_Task REICEVE...");*/
        dmfe_packet_receive(NetInterface);
        /*interrupt_unlock();*/
        //task_unlock(); /**/
    }
#endif
}

/*============================================================================
 Name    : STETHER_CopyData
 Description : Copy Data from chip to  NexGen buffer.
 ============================================================================*/
static void STETHER_CopyRx_Task(void * netp)
{

    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    while (1)
    {
        smsc_semaphore_wait(&CopyGo);
        
        task_lock();
        //iow(0xff, 0x82);
        dmfe_packet_copy(NetInterface);
        //iow(0xff, IMR_DEFAULT);
        task_unlock();
        
    }
}

/*============================================================================
 Name    : STETHER_Close
 Description : this makes the board clean up everything that it can
       and not talk to the outside world.
  Modify  : Spenser
 ============================================================================*/
static int STETHER_Close(void * netp)
{
#if 0
    /*U16 cfg_reg;*/
    /*static NGbuf * bufp;*/
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    /*interface must be already running*/
    
    if (!(netp->if_flags)&NG_IFF_RUNNING)
    {
        return(1);
    }
    
    /* clear flags*/
    (netp->if_flags) &= ~(NG_IFF_RUNNING | NG_IFF_UP);
    
    
    /* RESET devie */
    /*phy_write(db, 0x00, 0x8000);  PHY RESET */
    iow(0xff, 0x80);  /* Disable all interrupt */
    
    iow(0x05, 0x00);  /* Disable RX */
    
    iow(0x1f, 0x01);   /* Power-Down PHY */
    
    /*uninstall the irq*/
    interrupt_disable_number(STETHER_DM9000_Interrupt);
    
    interrupt_uninstall(STETHER_DM9000_Interrupt, STETHER_Interrupt_Level);
    
#ifdef TASK_MODE
    task_run = FALSE;
    
#ifdef USE_INIT_TASK
    task_delete(&EtherNetTaskId);
    
#else
    task_delete(Task);
    
#endif
    semaphore_delete(CopyGo);
    
#endif
    
    /* Remove pending buffers */
    while (1)
    {
        ngBufDequeue(netp, bufp);
        
        if (bufp == NULL)
        {
            break;
        }
        
        ngBufOutputFree(bufp);
    }
    
#endif
    return(0);
}

/*-------------------------------------------------------------------------
 Name : STETHER_InterruptHandler
 Description : handler of the interruptions.
 Author :
--------------------------------------------------------------------------*/
static void STETHER_InterruptHandler(void * netp)
{
    static int i_overflow = 0;
    static int i_overflow1 = 0;
    int int_status, tx_status, print_temp;
    U8 reg_save;
    SMSC911X_PRIVATE_DATA * privateData;
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    privateData = (SMSC911X_PRIVATE_DATA *)(NetInterface->mHardwareData);
    /* A real interrupt coming */
    /*在中断中不能出现打印函数，否则会死锁*/
    /*保存打印状态量，禁止打印*/
    print_temp = smsc_print_out;
    smsc_print_out = 0;
    /* Save previous register address */
    reg_save = inb(iobase);
    
    /* Disable all interrupt */
    iow(0xff, IMR_DIS_ALL);

    /* Got DM9000 interrupt status */
    int_status = ior(0xfe);  /* Got ISR */
    iow_one_time(0xfe, int_status);  /* Clear ISR status */
    
    /* Received the coming packet */
    if (int_status & DM9000_RX_INTR)
    {
        /*TaskManager_InterruptActivation(&(tmp_SMSC911X_PRIVATE_DATA.mRxTask));*/
        /*TaskManager_ScheduleAsSoonAsPossible(&(tmp_SMSC911X_PRIVATE_DATA.mRxTask));*/
        /*bu能异步接收数据*/
#ifdef TASK_MODE
        smsc_semaphore_signal(&CopyGo);
        //dmfe_packet_copy(netp);
#else
        dmfe_packet_receive(netp);
#endif
    }

    int_status |= ior(0xfe);  /* Got ISR */

    /* Trnasmit Interrupt check */
    if (int_status & DM9000_TX_INTR)
    {
        tcr_check();
    }
    
    if (int_status & DM9000_OVERFLOW_INTR )
    {
        i_overflow++;
    }
    
    if (int_status & 0x08 )
    {
        i_overflow1++;
    }

    /* Re-enable interrupt mask */
    iow(0xff, device_imr_set);

    /*printk("<RX interrupt number: %d>\n", rxintrflag);*/
    /*printk("<<Total interrupt number: %d>>\n", tintrflag);*/
    /* Restore previous register address */
    outb(reg_save, iobase);
    
    /* spin_unlock(&db->lock); */
    /*恢复打印*/
    smsc_print_out = print_temp;
}


/*--------------------------------------------------------------------------
 Name : dmfe_packet_receive
 Description : Received a packet and pass to upper layer.
 Author :
----------------------------------------------------------------------------*/

static void dmfe_packet_receive(void * netp)
{
    U8 rxbyte,  temp;
    U32 *rdptr;
    U16 i, RxStatus, RxLen, GoodPacket, tmplen, MDRAH, MDRAL;
    /*U32 tmpdata;*/
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    static PPACKET_BUFFER   bufReceiv; /*buffer to store the received packet*/
    SMSC911X_PRIVATE_DATA * privateData = (SMSC911X_PRIVATE_DATA *)NetInterface->mHardwareData;
    int i_count = 30;

    u16 save_mrr, check_mrr, calc_mrr;
    u16 device_new_len;
    u8 re_try = 10;
    device_wait_reset = FALSE;

    /* Check packet ready or not */
    do
    {
        ior(0xf0);   /* Dummy read */
        save_mrr = (ior(0xf5) << 8) | ior(0xf4);
        rxbyte = ior(0xf0); /* Got most updated data */
        
        /* packet ready to receive check */
        if (rxbyte == DM9000_PKT_RDY)
        {
            if(device_new_ver)
            {
                device_new_len = (ior(0x21) << 8) | ior(0x20);
            }

            /* A packet ready now  & Get status/length */
            GoodPacket = TRUE;
#ifdef IO_RE_CHECK    
            write_index_check(0xf2);/*Enable Read command with address incement*/
#else
            outb(0xf2, iobase);/*Enable Read command with address incement*/
#endif            
                      
            RxStatus = (U16)0;
            RxLen    = (U16)0;
            /* Selecting io mode */
            
            RxStatus = inw(iodata);
            RxLen    = inw(iodata);

            if(device_new_ver)
            {
                RxLen = device_new_len;
            }
            
            /* allocate memory for the recieved packet*/
            bufReceiv = PacketBuffer_Create(RxLen);
            
            if (bufReceiv == NULL)
            {
                /*re-load the value into Memory data read address register*/
                iow(0xf4, save_mrr & 0xff );
                iow(0xf5, (save_mrr >> 8) & 0xff);
                SMSC_ERROR(("@@@ No Mem,len[%d]\n", RxLen));
                return;
            }
            else
            {
                /*prepare structure */
                rdptr = (U32 *)PacketBuffer_GetStartPoint(bufReceiv);
                
                /* Read received packet from RX SARM */
                /* Word mode */
#ifdef MOVE2D
                tmplen = (RxLen + 1) / 2;
                move2d_all((void*)(&DATA16), rdptr, STETHER_TRANS_Len, tmplen, 0, STETHER_TRANS_Len);
#else
                tmplen = (RxLen + 1) / 2;
                    
                for (i = 0; i < tmplen; i++)
                    ((U16 *)rdptr)[i] = inw(iodata);
#endif
                
#if 0
                if (RxLen == 64 && rdptr[47] == 0x12)
                {
                    do_report(0, "SYN ACK\n");
                }
#endif
#if 0
                /* Trim 2 byte padding */
                if (PacketBuffer_MoveStartPoint(bufReceiv, 2) != ERR_OK)
                {
                    /* This should not happen because we allocated an extra 2 bytes */
                    SMSC_ERROR(("Smsc911x_RxTask: could not move start point"));
                }
#endif
                /* Trim trailing CRC */
                if (PacketBuffer_MoveEndPoint(bufReceiv, -4) != ERR_OK)
                {
                    /* This should not happen because every packet comes with a trailing CRC */
                    SMSC_ERROR(("Smsc911x_RxTask: could not move end point"));
                }

                if(device_new_ver)
                {
                    inw(iodata);
                    inw(iodata);
                    re_try = 10;
                    while(re_try-- && !(ior(0x26) & 0x2))
                    {
                        ior(0xf2);
                    }

                    if(re_try == 0)
                    {
                        device_wait_reset = 1;
                    }
                }
                else
                {
                    calc_mrr = save_mrr + 4 + RxLen;
                    if(RxLen & 0x01) calc_mrr++;
                    // bval = (64 - (bval + 1)) << 8; ==> 64 - 21 = 43 , 0x2b << 8 = 101011 0000 0000
                    if(calc_mrr >= 0x2b00) calc_mrr -= 0x2b00;

                    check_mrr = (ior(0xf5) << 8) | ior(0xf4);
                    if(check_mrr != calc_mrr) 
                    {
                        iow(0xf4, calc_mrr & 0xff );
                        iow(0xf5, (calc_mrr >> 8) & 0xff); 
                    }
                }                
                
                /*enqueue the message*/
                Ethernet_Input(NetInterface, bufReceiv);
                
                i_count --;
            }
        }
        else if(rxbyte > DM9000_PKT_RDY)
            {
                device_wait_reset = 1;
            }

    }
    while (rxbyte == DM9000_PKT_RDY && !device_wait_reset && i_count > 0);

    /* re-start ethernet */
    if (device_wait_reset)
    {
        dm9sw_soft_reset();
    }
   
#if 0
    if (i_count  ==  0)
    {
        /* Packets are still available */
        /* Reschedule this task to run as soon as possible */
        TaskManager_ScheduleAsSoonAsPossible(&(privateData->mRxTask));
    }
    else
    {
        /* No packets appear to be available yet */
        /*   Schedule for interrupt activation */
        TaskManager_ScheduleByInterruptActivation(&(privateData->mRxTask));
        {/* Enable Rx Interrupt */
        }
    }
    
#endif
}



/*========================================================================================
 * Name    :  STETHER_setMulticast                                                   *
 * Description :  Set the internal hardware table to filter out unwanted multicast       *
 *          packets before they take up memory.                         *
  Modify  : Spenser
 *=======================================================================================*/
static void STETHER_setMulticast(void * netp)
{
#if 0
    static U8 ht[8];    /* hash table */
    static u32 crc;   /* crc of the dest adr*/
    static U32 i;
    U32 oft;
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    /*disable int*/
    interrupt_lock();
    
    /* prepare hash table */
    
    if ( (netp->if_flags & (NG_IFF_PROMISC | NG_IFF_ALLMULTI)) ||
            NG_ETHIF_DATA( netp, eif_allmultis))
    {
        iow(0x5, RXCTL_DEFAULT | PASS_ALMULTI); /*Pass all multicast packet*/
        interrupt_unlock();
        return;
    }
    else
    {
        /* clear table */
        for ( i = 0; i < 8; i++)
        {
            ht[i] = 0;
        }
        
        ht[7] = 0x80;/*2005 8 17 add for multi cast*/
        
        /* parse list of groups */
        
        for ( i = 0; i < ETHIF_MULTIADDRS; i++)
        {
            if ( NG_ETHIF_DATA( netp, eif_multiaddrs)[i].eifm_refcount)
            {
                crc = ngEtherCRC32( NG_ETHIF_DATA( netp, eif_multiaddrs)[i].eifm_addr, 6);
                crc >>= 26;
                ht[crc>>3] |= 1 << (crc & 0x7);
            }
        }
        
    }
    
    /* set filter */
    for (i = 0, oft = 0x16; i < 8; i++, oft++)
        iow(oft, ht[i]);
        
    interrupt_unlock();
    
    return;
    
#else
    static U8 ht[8];    /* hash table */
    
    static u32 crc;   /* crc of the dest adr*/
    
    static U32 i;
    
    U32 oft;
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    /*disable int*/
    interrupt_lock();
    
    /* clear table */
    for ( i = 0; i < 8; i++)
    {
        ht[i] = 0;
    }
    
    ht[7] = 0x80;/*2005 8 17 add for multi cast*/
    
    /* set filter */
    
    for (i = 0, oft = 0x16; i < 8; i++, oft++)
        iow(oft, ht[i]);
    
    interrupt_unlock();
    
    return;
    
#endif
}

/*===========================================================================
 Name    :   STETHER_Load
 Description :  initialise the MAC address.
 returns  : 0 if success, 1 if mac adr not defined.
 Modify  : Spenser
 ===========================================================================*/
static U32 STETHER_MacLoad(void * netp)
{
    U32 mac_address = 0;
    DU8 address[6];
    U32 i, oft;
    U16 mac_addr_t[3];
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    
    /*read the byte */
    for (i = 0; i < 6; i++)
    {
        mac_address |= DM_MAC_ADDRESS[i];
    }
    
    if (mac_address)
    {
        /* application-defined MAC address */
        for (i = 0; i < 6; i++)
        {
            address[i] = DM_MAC_ADDRESS[i];
            
        }
        
        /* Set Node address */
        for (i = 0, oft = 0x10; i < 6; i++, oft++)
            iow(oft, address[i]);
    }
    else
    {
        /* Read SROM content */
        for (i = 0; i < 3; i++)
            mac_addr_t[i] = read_srom_word(i);
            
        /* read EEPROM MAC address */
        address[0] = (U8)(mac_addr_t[0] & 0xff);
        
        address[1] = (U8)((mac_addr_t[0] >> 8) & 0xff);
        
        address[2] = (U8)(mac_addr_t[1] & 0xff);
        
        address[3] = (U8)((mac_addr_t[1] >> 8) & 0xff);
        
        address[4] = (U8)(mac_addr_t[2] & 0xff);
        
        address[5] = (U8)((mac_addr_t[2] >> 8) & 0xff);
        
        /*test if the mac address is null*/
        for (i = 0; i < 6; i++)
        {
            mac_address |= address[i];
        }
        
        if (!mac_address)
        {
            return(1);
        }
        
        for (i = 0; i < 6; i++)
            DM_MAC_ADDRESS[i] = address[i];
    }
    
    for (i = 0;i < 6;i++)
        NetInterface->mLinkData.mEthernetData.mMacAddress[i] = DM_MAC_ADDRESS[i];
        
    return(0);
}

void dm_phy_test()
{
    do_report(0, "~~~~~~~~~~~~~~~~~~~~MII_BMSR = [0x%02x]\n", ior(0x01));
}

void STETHER_Config(U32 Eth_BaseAddr,
                    U8 Addr_Shift,
                    U8 Smc_Interrupt,
                    U8 Int_Level,
                    U8 Use16Bit,
                    U8 Trans_Len)
{
    STETHER_Ethernet_Base_Address = Eth_BaseAddr;
    STETHER_Address_Shift = Addr_Shift;
    STETHER_DM9000_Interrupt = Smc_Interrupt;
    STETHER_Interrupt_Level = Int_Level;
    STETHER_USE16bit = Use16Bit;
    STETHER_TRANS_Len = Trans_Len;
}

#if 0

/*----------------------------------- Driver entry point -------------------------------*/
const NGnetdrv ngNetDrv_dm9000a =
{
    "DM9000A",          /*driver name*/
    NG_IFT_ETHER,         /* adaptator type */
    NG_IFF_BROADCAST | NG_IFF_MULTICAST,  /* interface at startup*/
    ETHERMTU,           /* maximum transmission unit*/
    100000000UL,          /* link speed (b/s)*/
    ngEtherInit,          /* initialise the NGethifnet*/
    STETHER_Open,
    STETHER_Close,
    ngEtherOutput,
    STETHER_Start,
    ngEtherCntl,
    STETHER_setMulticast
};
#endif

void CH_ReadRxStatus(void)
{
    char  c_status;
    iow(DM9KS_PIndex, 0);
    c_status = ior(0x62);
    do_report(0, "c_status[0] = %d\n", c_status);
    
    iow(DM9KS_PIndex, 1);
    c_status = ior(0x62);
    do_report(0, "c_status[1] = %d\n", c_status);
    
    iow(DM9KS_PIndex, 2);
    c_status = ior(0x62);
    do_report(0, "c_status[2] = %d\n", c_status);
    
    
    iow(DM9KS_PIndex, 3);
    c_status = ior(0x62);
    do_report(0, "c_status[3] = %d\n", c_status);
}

#ifdef CH_RX_QUEUE
void dmfe_packet_copy(void * netp)
{
    U8 rxbyte,  temp;
    U32 *rdptr;
    U16 i, RxStatus, RxLen, tmplen, MDRAH, MDRAL;
    
    struct NETWORK_INTERFACE * NetInterface = (struct NETWORK_INTERFACE *)netp;
    
    static PPACKET_BUFFER   bufReceiv; /*buffer to store the received packet*/
    SMSC911X_PRIVATE_DATA * privateData = (SMSC911X_PRIVATE_DATA *)NetInterface->mHardwareData;
    U8 MacAddr[6] = {0x00, 0x21, 0x70, 0xa6, 0x7c, 0x52};
    U8 Broafpacket[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    U8 *upc_data;
    int i_count = 100;

    u16 save_mrr, check_mrr, calc_mrr;
    u16 device_new_len;
    u8 re_try = 10;
//    device_wait_reset = FALSE;
    
    smsc_semaphore_wait(&sem_tx_rx_lock);

    while (!device_wait_reset /*&& i_count > 0*/)
    {
        ior(0xf0);   /* Dummy read */
        save_mrr = (ior(0xf5) << 8) | ior(0xf4);
        rxbyte = ior(0xf0); /* Got most updated data */
        
        if(rxbyte != DM9000_PKT_RDY)
        {
            if(rxbyte > DM9000_PKT_RDY)
            {
                do_report8(0, "@@@ rxbyte > 1, rxbyte = %x \n", rxbyte);
                device_wait_reset = TRUE;
            }
            break;
        }
        else
        {
            if(device_new_ver)
            {
                device_new_len = (ior(0x21) << 8) | ior(0x20);
            }

            /* A packet ready now  & Get status/length */
#ifdef IO_RE_CHECK    
            write_index_check(0xf2);/*Enable Read command with address incement*/
#else
            outb(0xf2, iobase);/*Enable Read command with address incement*/
#endif    
            
            RxStatus = inw(iodata);
            RxLen    = inw(iodata);

//            do_report8(0, "@@@ RxStatus %x , RxLen %x , device_new_ver %x \n", RxStatus, RxLen, device_new_ver);
            if(device_new_ver)
            {
                RxLen = device_new_len;
            }
            
#ifdef BROADCAST_JUMP   //jump packet 
            if((RxStatus & 0x4000) && (RxLen > BROADCAST_MAX))
            {
                //do_report8(0, "@@@ jump packet RxStatus %x , RxLen %x \n", RxStatus, RxLen);
                device_debug++;
                calc_mrr = save_mrr + 4 + RxLen;
                if(RxLen & 0x01) calc_mrr++;
                if(calc_mrr >= device_rx_max) calc_mrr -= device_rx_max;

                iow(0xf4, calc_mrr & 0xff );
                iow(0xf5, (calc_mrr >> 8) & 0xff); 
                continue;
            }

#endif

            /* allocate memory for the recieved packet*/
            bufReceiv = PacketBuffer_Create(RxLen);
            
            if (bufReceiv == NULL)
            {
                /*re-load the value into Memory data read address register*/
                iow(0xf4, save_mrr & 0xff );
                iow(0xf5, (save_mrr >> 8) & 0xff);
                SMSC_ERROR(("@@@ No Mem,len[%d]\n", RxLen));
                smsc_semaphore_signal(&sem_tx_rx_lock);
                return;
            }
            else
            {
                /*prepare structure */
                rdptr = (U32 *)PacketBuffer_GetStartPoint(bufReceiv);
                
                /* Read received packet from RX SARM */
                /* Word mode */
                tmplen = (RxLen + 1) / 2;
#ifdef MOVE2D
                move2d_all((void*)(&DATA16), rdptr, STETHER_TRANS_Len, tmplen, 0, STETHER_TRANS_Len);
#else
                for (i = 0; i < tmplen; i++)
                    ((U16 *)rdptr)[i] = inw(iodata);
#endif
                
                upc_data = (U8 *)rdptr;
                
                if ((memcmp(upc_data, MacAddr, 6) == 0) || ( memcmp(rdptr, Broafpacket, 6) == 0))
                {
                
                }
                else
                {
                    /*   do_report(0,"No  My data [%x,%x,%x,%x,%x,%x]\n",
                    upc_data[0],upc_data[1],upc_data[2],upc_data[3],upc_data[4],upc_data[5]);*/
                }
                
                /* Trim trailing CRC */
                if (PacketBuffer_MoveEndPoint(bufReceiv, -4) != ERR_OK)
                {
                    /* This should not happen because every packet comes with a trailing CRC */
                    //SMSC_ERROR(("Smsc911x_RxTask: could not move end point"));
                }
                
                PacketQueue_Push(&(privateData->mRxPacketQueue), bufReceiv);
                
                i_count --;                

#ifdef RX_FIFO_CHECK
                if(device_new_ver)
                {
                    inw(iodata);
                    inw(iodata);
                    for(re_try = 10; re_try > 0; re_try--)
                    {
                        if(ior(0x26) & 0x2) break;
                        ior(0xf2);
                    }

                    if(re_try == 0)
                    {
                        device_wait_reset = TRUE;
                    }
                }
                else
                {
                    calc_mrr = save_mrr + 4 + RxLen;
                    if(RxLen & 0x01) calc_mrr++;
                    if(calc_mrr >= device_rx_max) calc_mrr -= device_rx_max;

                    check_mrr = (ior(0xf5) << 8) | ior(0xf4);
                    if(check_mrr != calc_mrr) 
                    {
                        do_report8(0, "@@@ check_mrr %x , calc_mrr %x \n", check_mrr, calc_mrr);
                        iow(0xf4, calc_mrr & 0xff );
                        iow(0xf5, (calc_mrr >> 8) & 0xff); 
                    }
                }                
#endif
            }
        }
    }

    smsc_semaphore_signal(&sem_tx_rx_lock);
    
#ifdef TASK_MODE
    // do_report(0,"i_count[%d ]LEN=[%d]\n",i_count,RxLen);
#endif
    
    TaskManager_InterruptActivation(&(privateData->mRxTask));
    
    /* re-start ethernet */
    if (device_wait_reset)
    {
        iow(0xff, IMR_DIS_ALL); /* Stop INT request */

        dm9sw_soft_reset();

        iow(0xff, device_imr_set);  /* Enable TX/RX interrupt mask */
    }
}

#endif
