/*****************************************************************************

File name   :  ether_dm9000.h

Description :  header module for ethernet driver.

Date               Modification                 Name        	
----               ------------                -------				
27/07/02          Beta-release		       ZAKARIA

*****************************************************************************/

/* --- prevents recursive inclusion --------------------------------------- */
#ifndef _STETHERNET_H_
#define _STETHERNET_H_

/*------- includes files ----------------------------------------------------*/
#include "stddefs.h"

/*Spenser ------------*/
#pragma ST_device(DU8)
typedef volatile U8 DU8; /*FC*/
/*-------------------*/
#pragma ST_device(DU16)
typedef volatile U16 DU16; /*FC*/
#pragma ST_device(DU32)
typedef volatile U32 DU32;/*FC*/

#define ETHERNET_BASE_ADDRESS		0x40000000

#define ADR_SHIFT 	              1  /*  2 */ /* TONY, no bits shift in Changhong board */
#define SMC_INTERRUPT 		          64/*		67*/
#define INT_LEVEL 					9
#define TRANS_LEN                   2
#define USE16BIT

#define CH_RX_QUEUE/*zxg 20081016 add*/

/*----- defines -------------------------------------------------------------*/

extern U32 STETHER_Ethernet_Base_Address;
extern U16 STETHER_Address_Shift;
extern U16 STETHER_SMC_Interrupt;
extern U16 STETHER_Interrupt_Level;
extern U16 STETHER_USE16bit;
extern U8  STETHER_TRANS_Len;

void STETHER_Config(U32 Eth_BaseAddr, 
                    U8 Addr_Shift,
                    U8 Smc_Interrupt,
                    U8 Int_Level,
                    U8 Use16Bit,
                    U8 Trans_Len);

/*=====================================================
 defines masks for the chip register
 =====================================================*/

/*Spenser------------------------------------------------------------*/
/*=========================================================== 
 				Network Control Register
 ===========================================================*/
#define NCR_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x00)
#define NCR_EXT_PHY			0x80	/* When 1 external MII used */
#define NCR_WAKEEN			0x40	/* When 1 enable wakeup function */
#define NCR_FDX				0x08	/* When 1 in Full-Duplex mode */
#define NCR_MAC_LBK			0x02	/* Internal MAC loopback*/
#define NCR_PHY_LBK			0x04	/* Internal PHY 100M digital loopback */
#define NCR_RST				0x01	/* Software Reset */

#define NSR_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x01)

#define TXCTL_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x02)

#define RXCTL_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x05)
#define PASS_ALMULTI		0x08
#define RXCTL_DEFAULT		0x30	/*Default value*/

#define BPT_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x08)
#define BACK_PRESSURE_THRESHOLD	0x37

#define FCT_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x09)
#define FLOWCONTROL_THRESHOLD	0x5A

#define FLCTL_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x0A)
#define TX_RX_FLOWCONTROL		0x29

#define EEPROM_PHY_CTL_REG	*(DU8*)(STETHER_Ethernet_Base_Address+0x0B)
#define PHY_WRITE_COMMAND	0x0A	/*Write PHY command*/
#define EEPROM_READ_COMMAND	0x04	/*Read eeprom command*/

#define EEPROM_PHY_ADR_REG	*(DU8*)(STETHER_Ethernet_Base_Address+0x0C)
#define DM9000_PHY			0x40	/* PHY address 0x01 */

#define EE_PHY_L_REG		*(DU8*)(STETHER_Ethernet_Base_Address+0x0D)
#define EE_PHY_H_REG		*(DU8*)(STETHER_Ethernet_Base_Address+0x0E)

#define PAB0_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x10)
#define PAB1_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x11)
#define PAB2_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x12)
#define PAB3_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x13)
#define PAB4_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x14)
#define PAB5_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x15)

#define MAB0_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x16)
#define MAB1_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x17)
#define MAB2_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x18)
#define MAB3_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x19)
#define MAB4_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x1A)
#define MAB5_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x1B)
#define MAB6_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x1C)
#define MAB7_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0x1D)

#define GPR_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x1F)
#define SMC_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0x2F)
#define MWCMD_REG			*(DU8*)(STETHER_Ethernet_Base_Address+0xF8)

#define ISR_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0xFE)
#define DM9000_RX_INTR		0x01
#define DM9000_TX_INTR		0x02
#define DM9000_OVERFLOW_INTR 0x04

#define IMR_REG				*(DU8*)(STETHER_Ethernet_Base_Address+0xFF)
#define IMR_DEFAULT			0x83
#define IMR_DIS_ALL			0x80


#define DM9000_DWORD_MODE	1
#define DM9000_BYTE_MODE	2
#define DM9000_WORD_MODE	0

#define DM9000_PKT_RDY		0x01	/* Packet ready to receive */
#define DM9000_PKT_MAX	1536	/* Received packet max size */


/*----------------------------------------------------------Spenser*/
/*----------------------------------------------------------------------
 . Basically, the chip has 4 banks of registers ( 0 to 3 ), which
 . are accessed by writing a number into the BANK_SELECT register
 . The banks are configured so that for most purposes, bank 2 is all
 . that is needed for simple run time tasks.  
 -----------------------------------------------------------------------*/

/*=============================================================
 . Bank Select Register: 
 .
 .		yyyy yyyy 0000 00xx  
 .		xx 		= bank number
 .		yyyy yyyy	= 0x33, for identification purposes.
 ============================================================*/
#define	BANK_SELECT				*(DU16*)(STETHER_Ethernet_Base_Address+0x000E*STETHER_Address_Shift)
/*------------------------ BANK 0--------------------------  */
/*=========================================================== 
 				Transmit Control Register
 ===========================================================*/
#define	TCR_REG 					*(DU16*)(STETHER_Ethernet_Base_Address+0x0000*STETHER_Address_Shift) 	/* transmit control register */
#define 	TCR_ENABLE				0x0001	/* When 1 we can transmit*/
#define 	TCR_LOOP					0x0002	/* Controls output pin LBK*/
#define 	TCR_FORCOL				0x0004	/* When 1 will force a collision*/
#define 	TCR_PAD_EN				0x0080	/* When 1 will pad tx frames < 64 bytes w/0*/
#define 	TCR_NOCRC				0x0100	/* When 1 will not append CRC to tx frames*/
#define 	TCR_MON_CSN				0x0400	/* When 1 tx monitors carrier*/
#define 	TCR_FDUPLX    			0x0800   /* When 1 enables full duplex operation*/
#define 	TCR_STP_SQET			0x1000	/* When 1 stops tx if Signal Quality Error*/
#define	TCR_EPH_LOOP			0x2000	/* When 1 enables EPH block loopback*/
#define	TCR_SWFDUP				0x8000	/* When 1 enables Switched Full Duplex mode*/
#define	TCR_CLEAR				0	      /* do NOTHING */
/* the default settings for the TCR register : */ 
#define	TCR_DEFAULT  		TCR_ENABLE 

/*==============================================================
 					EPH Status Register
 ==============================================================*/
#define EPH_STATUS_REG			*(DU16*)(STETHER_Ethernet_Base_Address+0x0002*STETHER_Address_Shift)
#define ES_TX_SUC					0x0001	/* Last TX was successful*/
#define ES_SNGL_COL				0x0002	/* Single collision detected for last tx*/
#define ES_MUL_COL				0x0004	/* Multiple collisions detected for last tx*/
#define ES_LTX_MULT				0x0008	/* Last tx was a multicast*/
#define ES_16COL					0x0010	/* 16 Collisions Reached*/
#define ES_SQET					0x0020	/* Signal Quality Error Test*/
#define ES_LTXBRD					0x0040	/* Last tx was a broadcast*/
#define ES_TXDEFR					0x0080	/* Transmit Deferred*/
#define ES_LATCOL					0x0200	/* Late collision detected on last tx*/
#define ES_LOSTCARR				0x0400	/* Lost Carrier Sense*/
#define ES_EXC_DEF				0x0800	/* Excessive Deferral*/
#define ES_CTR_ROL				0x1000	/* Counter Roll Over indication*/
#define ES_LINK_OK				0x4000	/* Driven by inverted value of nLNK pin*/
#define ES_TXUNRN					0x8000	/* Tx Underrun*/

/*================================================================
					Receive Control Register
 =================================================================*/
#define	RCR_REG					*(DU16*)(STETHER_Ethernet_Base_Address+0x0004*STETHER_Address_Shift)
#define	RCR_RX_ABORT			0x0001	/* Set if a rx frame was aborted*/
#define	RCR_PRMS					0x0002	/* Enable promiscuous mode*/
#define	RCR_ALMUL				0x0004	/* When set accepts all multicast frames*/
#define	RCR_RXEN					0x0100	/* IFF this is set, we can receive packets*/
#define	RCR_STRIP_CRC			0x0200	/* When set strips CRC from rx packets*/
#define	RCR_ABORT_ENB			0x0200	/* When set will abort rx on collision */
#define	RCR_FILT_CAR			0x0400	/* When set filters leading 12 bit s of carrier*/
#define 	RCR_SOFTRST				0x8000 	/* resets the chip*/
/* the normal settings for the RCR register : */
#define	RCR_DEFAULT				(RCR_STRIP_CRC|RCR_RXEN)
#define	RCR_CLEAR				0x0	/* set it to a base state*/

/*=================================================================
 						Counter Register
 =================================================================*/
#define	COUNTER_REG				*(DU16*)(STETHER_Ethernet_Base_Address+0x0006*STETHER_Address_Shift)
/*=================================================================
 					Memory Information Register
 ==================================================================*/
#define	MIR_REG					*(DU16*)(STETHER_Ethernet_Base_Address+0x0008*STETHER_Address_Shift)
/*=================================================================
 					Receive/Phy Control Register
 =================================================================*/
#define	RPC_REG					*(DU16*)(STETHER_Ethernet_Base_Address+0x000A*STETHER_Address_Shift)
#define	RPC_SPEED				0x2000	/* When 1 PHY is in 100Mbps mode.*/
#define	RPC_DPLX					0x1000	/* When 1 PHY is in Full-Duplex Mode*/
#define	RPC_ANEG					0x0800	/* When 1 PHY is in Auto-Negotiate Mode*/
#define	RPC_LSXA_SHFT			5	      /* Bits to shift LS2A,LS1A,LS0A to lsb*/
#define	RPC_LSXB_SHFT			2	      /* Bits to get LS2B,LS1B,LS0B to lsb*/
#define 	RPC_LED_100_10			(0x00)	/* LED = 100Mbps OR's with 10Mbps link detect*/
#define 	RPC_LED_RES				(0x01)	/* LED = Reserved*/
#define 	RPC_LED_10				(0x02)	/* LED = 10Mbps link detect*/
#define 	RPC_LED_FD				(0x03)	/* LED = Full Duplex Mode*/
#define 	RPC_LED_TX_RX			(0x04)	/* LED = TX or RX packet occurred*/
#define 	RPC_LED_100				(0x05)	/* LED = 100Mbps link dectect*/
#define 	RPC_LED_TX				(0x06)	/* LED = TX packet occurred*/   
#define 	RPC_LED_RX				(0x07)	/* LED = RX packet occurred*/
#define 	RPC_DEFAULT 		   (RPC_ANEG|(RPC_LED_100_10 << RPC_LSXA_SHFT) | (RPC_LED_TX_RX << RPC_LSXB_SHFT) | RPC_DPLX)

/*--------------------------- BANK 1---------------------------------------- */
/*----------------------------- BANK 2 ----------------------------------------*/
/*----------------------------- BANK 3 ------------------------------------------------*/

/*------- MACROS -------------------------------------------------------------*/


/* ------------------------------- End of file ---------------------------- */
#endif  /* _STETHERNET_H_ */


