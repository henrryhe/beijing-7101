/*****************************************************************************
File Name   : irtest.c

Description : Test harness for the BLAST driver.

Copyright (C) 2000 STMicroelectronics

Revision History :

          8/07/03:    DDTS 20197 Implementatio
          2/03/04:    Ported on espresso
          12/03/05:   Ported to 5301
          10/05/05    Ported on 8010
          14/07/05    Added STUART support and test for
                      RUWIDO protocol
          16/11/05    Ported on 7109
          30/01/06    Ported on 5188
          30/01/06    Ported on 5525
          01/05/06    Ported on 5107

Reference   :

ST API Definition "BLAST Driver API" DVD-API-22
*****************************************************************************/
#include <stdio.h>                      /* C libs */
#include <stdlib.h>
#include <string.h>

#include "stlite.h"                     /* System */
#include "stdevice.h"

#include "stblast.h"                    /* STAPI */
#include "stboot.h"                     /* Dependencies */
#include "stcommon.h"
#include "stevt.h"
#include "stos.h"

#if defined(STBLAST_USE_UART)
#include "stuart.h"
#endif

#include "irtest.h"                     /* Local */
#include "testtool.h"                   /*TestTool support*/

#if !defined(ST_OSLINUX)
#include "timer.h"
#include "stsys.h"
#include "blastint.h"                   /* Used for encode/decode prototypes */

#if !defined(DISABLE_TBX)
#include "sttbx.h"
#endif
#endif

#include "stos.h"
/*****************************************************************************
Types and definitions
*****************************************************************************/
#define SYMBOL_BUFFER_SIZE  200
#ifdef ST_OSLINUX
#define CMD_BUFFER_SIZE  10
#endif

#if !defined(PROCESSOR_C1)
/* used for timing purposes if necessary - provides microsecond resolution */
#define GetHPTimer(Value) __optasm{ldc 0; ldclock; st Value;}
#endif

/* Defines number of devices */
#define NUMBER_PIO      8
#define NUMBER_BLAST    3
#define NUMBER_CAPTURE  1
#define NUMBER_EVT      1

/* PIO Devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,
    PIO_DEVICE_7,
    PIO_DEVICE_NOT_USED
};

/* STBLAST devices */
enum
{
    BLAST_TX_IR,
    BLAST_RX_IR,
    BLAST_RX_UHF
};

/* STEVT devices */
enum
{
    EVT_DEVICE_0,
    EVT_DEVICE_1,
    EVT_DEVICE_2
};

#if defined(ST_5100)
#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT     ST5100_PIO4_INTERRUPT
#endif
#ifndef PIO_5_INTERRUPT
#define PIO_5_INTERRUPT     ST5100_PIO5_INTERRUPT
#endif
#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS  ST5100_PIO4_BASE_ADDRESS
#endif
#ifndef PIO_5_BASE_ADDRESS
#define PIO_5_BASE_ADDRESS  ST5100_PIO5_BASE_ADDRESS
#endif
#endif


/* Bit definition for an unused PIO bit */
#define PIO_BIT_NOT_USED    0

/* System clocks */
static ST_ClockInfo_t       ClockInfo;

#if defined(ARCHITECTURE_ST200)
#define CPU_CLOCK           ClockInfo.ST200
#elif defined(ARCHITECTURE_ST40)
#define CPU_CLOCK           ClockInfo.ST40
#else
#define CPU_CLOCK           ClockInfo.C2
#endif

#if defined(ARCHITECTURE_ST40) && defined(ST_5528) /* architecture ST40 */
#define IRB_CLOCK         50000000
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define IRB_CLOCK         100000000    /* since pll1 =0x1b(27MHz) in mb411.cmd */
#else
#define IRB_CLOCK         ClockInfo.CommsBlock /*for 27MHz clock in 7100/7109*/
#endif

#define BLAST_TXD_TYPE      STBLAST_DEVICE_IR_TRANSMITTER
#define BLAST_RXD_TYPE      STBLAST_DEVICE_IR_RECEIVER
#define BLAST_UHF_TYPE      STBLAST_DEVICE_UHF_RECEIVER
#define BLAST_RXD_NUM       0
#define BLAST_UHF_NUM       1

/* PIO usage */

#if defined(ST_5518)
#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_5]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_5]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_5]
#define BLAST_RXD_BIT       PIO_BIT_2
#define BLAST_UHF_BIT       PIO_BIT_3
#define BLAST_TXD_BIT       PIO_BIT_4
#define BLAST_TXDINV_BIT    PIO_BIT_5
#define CAPTURE_PIO         PioDeviceName[PIO_DEVICE_3]
#define CAPTURE_BIT         PIO_BIT_6
#define CAPTURE_CHAN        1
#define CAPTURE_EVT_DEV     EvtDeviceName[EVT_DEVICE_0]
#define CAPTURE_EVT_HDL     EvtHandle[EVT_DEVICE_0]

#elif defined(ST_8010)
#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_1]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_1]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_1]

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5301) || defined(ST_7109) || \
defined(ST_7100) || defined(ST_7200)

#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_3]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_3]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_3]

#elif defined(ST_5105) || defined(ST_5107)
#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_2]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_2]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_2]

#elif defined(ST_5525)

#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_3]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_3]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_4]

#elif defined(ST_5188)
#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_1]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_2]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_1]

#else
#define BLAST_RXD_PIO       PioDeviceName[PIO_DEVICE_5]
#define BLAST_TXD_PIO       PioDeviceName[PIO_DEVICE_5]
#define BLAST_UHF_PIO       PioDeviceName[PIO_DEVICE_5]
#endif

  /* For 5528 */
#if defined(mb376)
#define BLAST_UHF_BIT       PIO_BIT_0
#define BLAST_RXD_BIT       PIO_BIT_1

#elif defined(espresso)
#define BLAST_RXD_BIT       PIO_BIT_0
#define BLAST_TXD_BIT       PIO_BIT_2
#define BLAST_UHF_BIT       PIO_BIT_1

#elif defined(ST_8010)
#define BLAST_RXD_BIT       PIO_BIT_0
#define BLAST_UHF_BIT       PIO_BIT_0

#elif defined(ST_5100) || defined(ST_5301)
#define BLAST_RXD_BIT       PIO_BIT_6
#define BLAST_UHF_BIT       PIO_BIT_4

#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define BLAST_RXD_BIT       PIO_BIT_3
#define BLAST_UHF_BIT       PIO_BIT_4

#elif defined(ST_5105) || defined(ST_5107)
#define BLAST_RXD_BIT       PIO_BIT_4   /* Alternate Output 0 */
#define BLAST_UHF_BIT       PIO_BIT_4   /* Alternate Output 1 */

#elif defined(ST_5188)
#define BLAST_RXD_BIT       PIO_BIT_7
#define BLAST_UHF_BIT       PIO_BIT_7

#elif defined(ST_5525)
#define BLAST_RXD_BIT       PIO_BIT_6
#define BLAST_UHF_BIT       PIO_BIT_0

#else
#define BLAST_RXD_BIT       PIO_BIT_0
#define BLAST_UHF_BIT       PIO_BIT_1
#endif

#if defined(ST_5100)   || defined(ST_5301) || defined(ST_5525)
#define BLAST_TXD_BIT       PIO_BIT_7
#elif defined(ST_8010)
#define BLAST_TXD_BIT       PIO_BIT_1
#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define BLAST_TXD_BIT       PIO_BIT_5
#elif defined(ST_5105) || defined(ST_5107)
#define BLAST_TXD_BIT       PIO_BIT_5
#else
#define BLAST_TXD_BIT       PIO_BIT_2 /* 5188 */
#endif


#if defined(ST_5100) || defined(ST_5301)
#define INTERCONNECT_BASE                 (U32)0x20D00000

#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#endif

#if defined(ST_5105) || defined(ST_5107)
#define INTERCONNECT_BASE                      (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_C       0x00
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_C (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_C)
#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)


#endif

#if defined(ST_8010)
#define SYSCONF_BASE_ADDRESS  0x50003000
#define SYSCONF_CON1A        (SYSCONF_BASE_ADDRESS + 0x10)
#define SYSCONF_CON1B        (SYSCONF_BASE_ADDRESS + 0x14)
#define	SYSCONF_CON2A        (SYSCONF_BASE_ADDRESS + 0x18)
#define	SYSCONF_CON2B        (SYSCONF_BASE_ADDRESS + 0x1c)

#endif

#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define BLAST_TXDINV_BIT    PIO_BIT_6
#elif defined(ST_5301)
#define BLAST_TXDINV_BIT    PIO_BIT_5
#elif defined(ST_8010)
#define BLAST_TXDINV_BIT    PIO_BIT_2
#else
#define BLAST_TXDINV_BIT    PIO_BIT_3
#endif

#define CAPTURE_PIO         PioDeviceName[PIO_DEVICE_4]
#define CAPTURE_BIT         PIO_BIT_6

#define CAPTURE_CHAN        2
#define CAPTURE_EVT_DEV     EvtDeviceName[EVT_DEVICE_0]
#define CAPTURE_EVT_HDL     EvtHandle[EVT_DEVICE_0]

#define BLAST_EVT_DEV       EvtDeviceName[EVT_DEVICE_0]
#define BLAST_EVT_HDL       EvtHandle[EVT_DEVICE_0]
#define BLAST_RXD_HDL       BlasterHandle[BLAST_RX_IR]
#define BLAST_UHF_HDL       BlasterHandle[BLAST_RX_UHF]
#define BLAST_TXD_HDL       BlasterHandle[BLAST_TX_IR]

/* STEVT usage */
#define EVT_MAX_EVENTS          15
#define EVT_MAX_CONNECTIONS     15
#define EVT_MAX_SUBSCRIBERS     10

/* Macro for computing min of two numbers */
#ifdef min
#undef min
#endif

#define min(x,y) (x<y?x:y)

#define ST_ERROR_UNKNOWN ((U32)-1)
/*****************************************************************************
Global data
*****************************************************************************/
/* Test harness revision number */
static U8 Revision[] = "3.5.0";

static ST_DeviceName_t      PioDeviceName[] =
{
    "PIO0",
    "PIO1",
    "PIO2",
    "PIO3",
    "PIO4",
    "PIO5",
    "PIO6",
    "PIO7"
};

static ST_DeviceName_t      BlasterDeviceName[] =
{
    "TX",
    "RX",
    "UHF"
};

static ST_DeviceName_t      EvtDeviceName[] =
{
    "EVT0"
};

static STBLAST_Handle_t     BlasterHandle[NUMBER_BLAST];
static STEVT_Handle_t       EvtHandle[NUMBER_EVT];

/* Used by event handler */
ST_ErrorCode_t LastEvent;
ST_ErrorCode_t LastError;

char ErrMsg[256];

semaphore_t *BlasterSemaphore_p;

/* RC code structure */
typedef struct RCCode_s
{
    char KeyName[20];
    U32  Code;
} RCCode_t;

/* Protocol codes */
RCCode_t SonyRcCode[] =
{
    { "1", 0xFEF },
    { "2", 0x7EF },
    { "3", 0xBEF },
    { "4", 0x3EF },
    { "5", 0xDEF },
    { "6", 0x5EF },
    { "7", 0x9EF },
    { "8", 0x1EF },
    { "9", 0xEEF },
    { "0", 0x6EF },
    { "10", 0xCEF },
    { "20", 0x4EF },
    { "OK", 0x58F },
    { "STBY", 0x56F },
    { "UP", 0xF6F },
    { "DOWN", 0x76F },
    { "MUTE", 0xD6F },
    { "VOLUP", 0xB6F },
    { "VOLDOWN", 0x36F },
    { "UNKNOWN", 0xFF }
};

RCCode_t EchoStarRCCode[] =
{
    {"1", 0xEE3F},
    {"2", 0xEA3F},
    {"3", 0xE63F},
    {"4", 0xDE3F},
    {"5", 0xDA3F},
    {"6", 0xD63F},
    {"7", 0xCE3F},
    {"8", 0xCA3F},
    {"9", 0xC63F},
    {"0", 0xBA3F},
    {"KEYRELASE",0x463F},
    { "UNKNOWN", 0xFF }
};
RCCode_t LowLatencyCode[] =
{
	{"0", 0x11},
    {"1", 0x1},
    {"2", 0x2},
    {"3", 0x3},
    {"4", 0x4},
    {"5", 0x5},
    {"6", 0x6},
    {"7", 0x7},
    {"8", 0x8},
    {"9", 0x9},
    {"0", 0x11},
    {"BACK",0x27},
    {"EXIT",0x26},
    {"SELECT",0x25},
    {"BACK",0x27},
    {"EXIT",0x26},
    {"SELECT",0x25},
    {"CH+",0x0D},
    {"CH-",0x0E},
    {"Vol+",0x0D},
    {"Vol-",0x0E},    
    {"LEFT",0x23},
    {"RIGHT",0x24},
    {"UP",0x21},
    {"DOWN",0x22},
    {"POWER ON", 0x80},
    {"MENU", 0x20},  
    {"INFO", 0x2E},         
    {"POWER OFF", 0x81},
    {"POWER", 0x10},
    {"PREV CH", 0x0F},
    {"ENTER", 0x13},
    {"RED", 0x41},
    {"GREEN", 0x43},
    {"YELLOW", 0x42},
    {"MUTE", 0x42},    
    {"BLUE", 0x44},
    {"PLAY", 0x30},
    {"STOP", 0x31},
    {"FFW",0x34},
    {"PAUSE",0x32},
    {"REWIND",0x33},
    {"RECORD",0x35},
    {"REPLAY",0x36},
    {"GUIDE",0x28},
    {"LIST",0x2A},  
    {"EXIT",0x26},    
    {"ADVANCE",0x37},
    {"FORMAT",0x73},    
    { "UNKNOWN", 0xFF }
};

/* Protocol codes - these are bit reversed versions of the codes in
                    the STi5518/88 Development Platform manual */
RCCode_t ForceRcCode[] =
{
    { "1", 0x00 },
    { "2", 0x80 },
    { "3", 0x40 },
    { "4", 0xC0 },
    { "5", 0x20 },
    { "6", 0xA0 },
    { "7", 0x60 },
    { "8", 0xE0 },
    { "9", 0x10 },
    { "0", 0x90 },
    { "UP", 0x50 },
    { "OK", 0x98},
    { "DOWN", 0xD0 },
    { "LEFT", 0x30 },
    { "RIGHT", 0xB0 },
    { "MUTE", 0x70 },
    { "STBY", 0xF0 },
    { "UNKNOWN", 0xFF }
};

RCCode_t    NECRcCode []  =
{
    { "1", 0xE17A807F },
    { "2", 0xE17A40BF },
    { "3", 0xE17AC03F },
    { "4", 0xE17A20DF },
    { "5", 0xE17AA05F },
    { "6", 0xE17A609F },
    { "7", 0xE17AE01F },
    { "8", 0xE17A10EF },
    { "9", 0xE17A906F },
    { "0", 0xE17A00FF },
    { "UP",0xE17AD02F },
    { "DOWN",0xE17A30CF},
    { "LEFT",0xE17AD827},
    { "RIGHT",0xE17A38C7},
    { "OK",0xE17AA857},
    { "UNKNOWN", 0xFFFFFFFF }
};

RCCode_t    RuwidoRcCodeRC []  =
{
    { "1", 2 },
    { "2", 5 },
    { "3", 6 },
    { "4", 9 },
    { "5", 10 },
    { "6", 13 },
    { "7", 14 },
    { "8", 17 },
    { "9", 18 },
    { "0", 21 },
    { "ON/OFF",22 },
    { "TV-theek",25},
    { "TV-gids",26},
    { "Comms.",29},
    { "Kiosk",30},
    { "Telenet button", 33 },
    { "Mute", 34 },
    { "Help", 37 },
    { "V+", 38 },
    { "V-",41 },
    { "Ch+",42},
    { "Ch-",45},
    { "Ok",54},
    { "-/--",56},
    { "Back",61},
    { "UNKNOWN", 0xFF}

};
RCCode_t    RuwidoRcCodeKB []  =
{
    { "ESc", 41 },
    { "F1", 58 },
    { "F2", 59 },
    { "F3", 60 },
    { "F4", 61 },
    { "F5", 62 },
    { "1", 30 },
    { "2", 31 },
    { "3", 32 },
    { "4", 33 },
    { "5", 34 },
    { "6", 35 },
    { "7", 36 },
    { "8", 37 },
    { "9", 38 },
    { "0", 39 },
    { "A",4 },
    { "B",5 },
    { "C",6 },
    { "D",7 },
    { "E",8 },
    { "F", 9 },
    { "G", 10 },
    { "H", 11 },
    { "I", 12 },
    { "J",13 },
    { "K",14 },
    { "L",15 },
    { "M",16 },
    { "N",17 },
    { "O", 18 },
    { "P", 19 },
    { "Q", 20 },
    { "R", 21 },
    { "S",22 },
    { "T",23 },
    { "U",24 },
    { "V",25 },
    { "W", 26 },
    { "X", 27 },
    { "Y",28 },
    { "Z",29 },
    { "U",45},
    { "V",54},
    { "Ctrl",224 },
    { "Alt",226 },
    { "Tab", 43 },
    { "Shift", 225 },
    { "Space",44 },
    { "Del",76 },
    { "Enter",40},
    { "Cmd",0x220},
    { "UNKNOWN", 0xFF}

};

RCCode_t GrundigRcCode[] =
{
    { "1", 0x314 },
    { "2", 0x294 },
    { "3", 0x394 },
    { "4", 0x254 },
    { "5", 0x354 },
    { "6", 0x2d4 },
    { "7", 0x3d4 },
    { "8", 0x234 },
    { "9", 0x334 },
    { "0", 0x214 },
    { "P+", 0x2ac },
    { "P-", 0x3ac },
    { "VOL+", 0x224 },
    { "VOL-", 0x324 },
    { "MUTE", 0x300 },
    { "STBY", 0x284 },
    { "EOT", 0x3FF },
    { "UNKNOWN", 0xFF }
};

RCCode_t RC5RcCode[] =
{
    /*Added for ST RC*/
    { "0", 0x00 },
    { "1", 0x01 },
    { "2", 0x02 },
    { "3", 0x03 },
    { "4", 0x04 },
    { "5", 0x05 },
    { "6", 0x06 },
    { "7", 0x07 },
    { "8", 0x08 },
    { "9", 0x09 },
    { "UP",0x80 },  /* not all RC5 handsets will implement these codes */
    { "DOWN", 0x81 },
    { "LEFT", 0x85 },
    { "RIGHT", 0x86 },
    { "OK",0x17},
    { "UP_ARROW",0x10},
    { "DOWN_ARROW",0x11},
    { "LEFT_ARROW",0x15},
    { "RIGHT_ARROW",0x16},
    { "SELECT",0x33},
    { "RED",0x22},
    { "GREEN",0x23},
    { "YELLOW",0x24},
    { "BLUE",0x25},
    { "UNKNOWN", 0xFF }
#if 0 /*RC5 codes for other remote */
    { "0 C=1", 0x1800 },
    { "0 C=0", 0x1000 },
    { "1 C=1", 0x1801 },
    { "1 C=0", 0x1001 },
    { "2 C=1", 0x1802 },
    { "2 C=0", 0x1002 },
    { "3 C=1", 0x1803 },
    { "3 C=0", 0x1003 },
    { "4 C=1", 0x1804 },
    { "4 C=0", 0x1004 },
    { "5 C=1", 0x1805 },
    { "5 C=0", 0x1005 },
    { "6 C=1", 0x1806 },
    { "6 C=0", 0x1006 },
    { "7 C=1", 0x1807 },
    { "7 C=0", 0x1007 },
    { "8 C=1", 0x1808 },
    { "8 C=0", 0x1008 },
    { "9 C=1", 0x1809 },
    { "9 C=0", 0x1009 },
    { "UP C=1", 0x1880 },  /* not all RC5 handsets will implement these codes */
    { "UP C=0", 0x1080 },
    { "DOWN C=1", 0x1881 },
    { "DOWN C=0", 0x1081 },
    { "LEFT C=1", 0x1885 },
    { "LEFT C=0", 0x1085 },
    { "RIGHT C=1", 0x1886 },
    { "RIGHT C=0", 0x1086 },
    { "OK C=1",0x817},
    { "OK C=0",0x17},
    { "UP_ARROW",0x10},
    { "DOWN_ARROW",0x11},
    { "LEFT_ARROW",0x15},
    { "RIGHT_ARROW",0x16},
    { "SELECT",0x33},
    { "RED",0x22},
    { "GREEN",0x23},
    { "YELLOW",0x24},
    { "BLUE",0x25},
    { "UNKNOWN", 0xFF }
#endif
};

RCCode_t RC6M0Code[] =
{
    { "0 C=1", 0x2700 },
    { "0 C=0", 0x12700 },
    { "1 C=1", 0x2701 },
    { "1 C=0", 0x12701 },
    { "2 C=1", 0x12702 },
    { "2 C=0", 0x2702 },
    { "3 C=1", 0x12703 },
    { "3 C=0", 0x2703 },
    { "4 C=1", 0x12704 },
    { "4 C=0", 0x2704 },
    { "5 C=1", 0x12705 },
    { "5 C=0", 0x2705 },
    { "6 C=1", 0x12706 },
    { "6 C=0", 0x2706 },
    { "7 C=1", 0x12707 },
    { "7 C=0", 0x2707 },
    { "8 C=1", 0x12708 },
    { "8 C=0", 0x2708 },
    { "9 C=1", 0x12709 },
    { "9 C=0", 0x2709 },
    { "UNKNOWN", 0xFF }
};

RCCode_t RMapCode[] =
{
    { "0", 0x00 },
    { "1", 0x01 },
    { "2", 0x02 },
    { "3", 0x03 },
    { "4", 0x04 },
    { "5", 0x05 },
    { "6", 0x06 },
    { "7", 0x07 },
    { "8", 0x08 },
    { "9", 0x09 },
    { "V-", 0x11 },
    { "V+", 0x12 },
    { "P-", 0x13 },
    { "P+", 0x14 },
    { "UNKNOWN", 0xFF }
};

RCCode_t RMapDoubleBitCode[] =
{
     { "ON/OFF R=0", 0xC78016 },
     { "ON/OFF R=1", 0xC78116 },
     { "1 R=0",  0xC78002 },
     { "1 R=1",  0xC78102 },
     { "2 R=0",  0xC78005 },
     { "2 R=1",  0xC78105 },
     { "3 R=0",  0xC78006 },
     { "3 R=1",  0xC78106 },
     { "4 R=0",  0xC78009 },
     { "4 R=1",  0xC78109 },
     { "5 R=0",  0xC7800A },
     { "5 R=1",  0xC7800A },
     { "6 R=0",  0xC7800D },
     { "6 R=1",  0xC7810D },
     { "7 R=0",  0xC7800E },     
     { "7 R=1",  0xC7810E },      
     { "8 R=0",  0xC78011},
     { "8 R=1",  0xC78111},     
     { "9 R=0",  0xC78012 }, 
     { "9 R=1",  0xC78112 },         
     { "TV R=0", 0xC78001 },
     { "TV R=1", 0xC78101 },
     { "0 R=0",  0xC78015 },     
     { "0 R=1",  0xC78115 }, 
     { "< R=0",  0xC7803A },     
     { "< R=1",  0xC7813A },        
     
     { "V- R=0", 0xC7802A },
     { "V- R=1", 0xC7812A },     
     { "V+ R=0", 0xC78026 },
     { "V+ R=1", 0xC78126 },        
     { "P- R=0", 0xC7802D },   
     { "P- R=1", 0xC7812D },         
     { "P+ R=0", 0xC78029 },   
     { "P+ R=1", 0xC78129 },        
     { "OK R=0", 0xC7801E},    
     { "OK R=1", 0xC7811E},         
     { "UP_ARROW R=0",0xC78019},
     { "UP_ARROW R=1",0xC78119},    
     { "DOWN_ARROW R=0",0xC7801A},
     { "DOWN_ARROW R=1",0xC7811A},    
     { "LEFT_ARROW R=0",0xC7801D},     
     { "LEFT_ARROW R=1",0xC7811D},       
     { "RIGHT_ARROW R=0",0xC78021},
     { "RIGHT_ARROW R=1",0xC78121},
     
    { "RED R=0",0xC78025},
    { "RED R=1",0xC78125},
    { "GREEN R=0",0xC78039},
    { "GREEN R=1",0xC78139},
    { "YELLOW R=0",0xC78022},
    { "YELLOW R=1",0xC78122},    
    { "BLUE R=0",0xC78031},
    { "BLUE R=1",0xC78131},    
    { "UNKNOWN", 0xFF }
};

RCCode_t RCRcmmCode_32[] =
{
    { "1 C 0", 0x22512701},
    { "1 C 1", 0x2251A701},
    { "2 C 0", 0x22512702},
    { "2 C 1", 0x2251A702},
    { "3 C 0", 0x22512703},
    { "3 C 1", 0x2251A703},
    { "4 C 0", 0x22512704},
    { "4 C 1", 0x2251A704},
    { "5 C 0", 0x22512705},
    { "5 C 1", 0x2251A705},
    { "6 C 0", 0x22512706},
    { "6 C 1", 0x2251A706},
    { "7 C 0", 0x22512707},
    { "7 C 1", 0x2251A707},
    { "8 C 0", 0x22512708},
    { "8 C 1", 0x2251A708},
    { "9 C 0", 0x22512709},
    { "9 C 1", 0x2251A709},
    { "0 C 0", 0x22512700},
    { "0 C 1", 0x2251A700},
    { "UNKNOWN", 0xFF }
};
RCCode_t RCRcmmCode_24[] =
{
    { "1", 0xE0001},
    { "2", 0xE0002 },
    { "3", 0xE0003 },
    { "4", 0xE0004 },
    { "5", 0xE0005 },
    { "6", 0xE0006 },
    { "7", 0xE0007 },
    { "8", 0xE0008 },
    { "9", 0xE0009 },
    { "0", 0xE0000 },
    { "UNKNOWN", 0xFF }

};

RCCode_t RCRF8Code[] =
{
    { "1", 0x1E},
    { "2", 0x2D},
    { "3", 0x33 },
    { "4", 0x4B },
    { "5", 0x55 },
    { "6", 0x66 },
    { "7", 0x78 },
    { "8", 0x87},
    { "9", 0x99},
    { "0", 0x00},
    { "UNKNOWN", 0xFF }

};
RCCode_t RC6ARcCode[] =
{
    { "1", 1 },
    { "2", 2 },
    { "3", 3 },
    { "4", 4 },
    { "5", 5 },
    { "6", 6 },
    { "7", 7 },
    { "8", 8 },
    { "9", 9 },
    { "0", 10 },
    { "UP", 88 },
    { "DOWN", 89 },
    { "LEFT", 90 },
    { "RIGHT", 91 },
    { "BATTERY LOW", 71 },
    { "STBY", 12 },
    {"SELECT",92},
    { "UNKNOWN", 0xFF }
};

#ifndef ST_OSLINUX
#ifndef ST_OS21
/* Declarations for memory partitions */
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

#define                 NCACHE_MEMORY_SIZE          0x80000
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( external_block, "system_section")


/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_7100) \
|| defined(ST_7109) ||defined(ST_5188) || defined(ST_5525) || defined(ST_5107) || defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#ifdef UNIFIED_MEMORY
  static unsigned char    ncache2_block[1];
  #pragma ST_section      ( ncache2_block, "ncache2_section")
#endif

#else

#define                 SYSTEM_MEMORY_SIZE          0x200000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif

#else
/* define partition here */
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
#endif /* ST_OSLINUX */
/*****************************************************************************
Function prototypes
*****************************************************************************/
#if defined(STBLAST_USE_UART)
BOOL UART_Init(void);
#endif

BOOL TBX_Init(void);
BOOL BOOT_Init(void);
BOOL EVT_Init(void);
BOOL PIO_Init(void);
BOOL BLAST_Init(void);
BOOL TST_Init(void); /* Test Tool */

BOOL BLAST_Term(void);
BOOL EVT_Term(void);
BOOL TST_Term(void);
BOOL PIO_Term(void);

void InitCommands(void);

void DeviceConfig(void);
void DeviceConfig_UHF(void);

void EvtCallback(STEVT_CallReason_t Reason,
                 STEVT_EventConstant_t Event,
                 const void *EventData);
void DevEvtCallback(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p);

void test_init(BLAST_TestParams_t *Params_p);
void test_typical(BLAST_TestParams_t *Params_p);
void test_term(BLAST_TestParams_t *Params_p);
void test_overhead(BLAST_TestParams_t *Params_p);

BLAST_TestResult_t TestRC5(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestLowLatency(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRC6_Mode_0(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSpace(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestAbortSend(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestAbortReceive(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRxLoopbackRC5(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRxLoopbackPulse(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRxLoopbackRaw(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRxCapture(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestUHFRxCapture(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRCRF8Decode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestMultiProtocol(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRMapDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRStepDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRcmm_24_32_Mode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSonyBlast(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestEchoStarRC(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRuwidoDecodeRC(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRuwidoDecodeKB(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSonyBlastBatch(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSonyDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSonyDecodeBatch(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestSonyLoopback(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRC5Loopback(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestAPIErrant(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestDutyCycle(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestForceDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestForceBlast(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestLowLatencyDecode (BLAST_TestParams_t *Params);
BLAST_TestResult_t TestLowLatencyProDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRMapDoubleBitDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestNECDecode(BLAST_TestParams_t *Params);

/* added for generailsed shift implementation*/

BLAST_TestResult_t TestGrundigDecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestGrundigBlast(BLAST_TestParams_t *Params);

BLAST_TestResult_t BlastStackUsage(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRC6ADecode(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRC6ABlast(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestRC6ABlastRaw(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestLowLatencyBlast(BLAST_TestParams_t *Params);
BLAST_TestResult_t TestLowLatencyProBlast(BLAST_TestParams_t *Params);

BLAST_TestResult_t TestRC5Decode(BLAST_TestParams_t *Params);
ST_ErrorCode_t BLAST_RC6ATestEncode(const U32                 *UserBuf_p,
                                const U32                  UserBufSize,
                                STBLAST_Symbol_t           *SymbolBuf_p,
                                const U32                  SymbolBufSize,
                                U32                        *SymbolsEncoded_p,
                                const STBLAST_ProtocolParams_t *ProtocolParams_p);
ST_ErrorCode_t BLAST_DoTestEncode(const U32                  *UserBuf_p,
                              const U32                  UserBufSize,
                              STBLAST_Symbol_t           *SymbolBuf_p,
                              const U32                  SymbolBufSize,
                              U32                        *SymbolsEncoded_p,
                              const STBLAST_Protocol_t   Protocol,
                              const STBLAST_ProtocolParams_t *ProtocolParams_p);


void BLASTDataUsage(BLAST_TestParams_t *Params_p);

void PrintSupportedProtocols(void);
void ErrorReport(ST_ErrorCode_t *ErrorStore,
                 ST_ErrorCode_t ErrorGiven,
                 ST_ErrorCode_t ExpectedErr);
void DumpRegs(void);
char *RcCodeString(U32 Code, RCCode_t *Table_p);
U16 RcCodeValue(const char *Name, RCCode_t *Table_p);
U32 ShowProgress(U32 Now, U32 Total, U32 Last);

/*****************************************************************************/
/* Test table */
#ifdef TEST_AUTO
BOOL Auto_Result(parse_t * pars_p, char *result_sym_p);
BOOL Auto_APIErrant(parse_t * pars_p, char *result_sym_p);
#endif


#ifdef TEST_MANUAL
BOOL Manual_DutyCycle(parse_t * pars_p, char *result_sym_p);
BOOL Manual_SonyEncodingAndDecoding(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RC5Decode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RxCapture(parse_t * pars_p, char *result_sym_p);
BOOL Manual_UHFRxCapture(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RCRF8Decode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_ForceEncodingAndDecoding(parse_t * pars_p, char *result_sym_p);
BOOL Manual_GrundigDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RuwidoDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_NECDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RC6ADecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RC6AM0Decode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_EchoStarDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_LowLatencyDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_LowLatencyProDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RCMM_Decode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_TestAll(parse_t * pars_p, char *result_sym_p);
BOOL Manual_MultiProtocol(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RMapDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RMapDoubleBitDecode(parse_t * pars_p, char *result_sym_p);
BOOL Manual_RStepDecode(parse_t * pars_p, char *result_sym_p);
#endif

#ifdef TEST_BLAST
BOOL Blast_TestGrundig(parse_t * pars_p, char *result_sym_p);
BOOL Blast_RC6ABlast(parse_t * pars_p, char *result_sym_p);
BOOL Blast_RC6ABlastRaw(parse_t * pars_p, char *result_sym_p);
BOOL Blast_LowLatencyBlast(parse_t * pars_p, char *result_sym_p);
BOOL Blast_LowLatencyProBlast(parse_t * pars_p, char *result_sym_p);

#endif

/*****************************************************************************
Main
*****************************************************************************/

int main(void)
{
#if !defined(ST_OSLINUX)
#ifdef ST_OS21
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));
#else
    /* Create memory partitions */
    partition_init_heap(&the_internal_partition,
                        (U8*)internal_block,
                        sizeof(internal_block)
                       );
    partition_init_heap(&the_system_partition,
                        (U8*)external_block,
                        sizeof(external_block));
    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;

#if defined(ST_5528) || defined(ST_5100) ||defined(ST_7710) || defined(ST_5105) ||\
defined(ST_7100) || defined(ST_7109) ||defined(ST_5188) || defined(ST_5525) || \
defined(ST_5107) || defined(ST_7200)
    data_section[0] = 0;
#endif
#endif

#ifdef UNIFIED_MEMORY
    ncache2_block[0] = 0;
#endif

#if defined(STBLAST_USE_UART)

    if (BOOT_Init() || PIO_Init() || UART_Init() || TBX_Init() || TST_Init() || EVT_Init()  ||
        BLAST_Init())
     {
        BLAST_DebugMessage("Problem initializing drivers");
        return 0;
    }

#else

    /* Initialize all drivers */
    if (BOOT_Init() || TBX_Init() || TST_Init() || PIO_Init() || EVT_Init()  ||
        BLAST_Init())
    {
        BLAST_DebugMessage("Problem initializing drivers");
        return 0;
    }
#endif

#else /* ST_OSLINUX */


    /* Obtain clock information - to be used in BLAST_Init */
    ST_GetClockInfo(&ClockInfo);

    /* Initialize all drivers */
    if (EVT_Init()  ||  PIO_Init() ||  BLAST_Init() || TST_Init())
    {
        BLAST_DebugMessage("Problem initializing drivers");
        return 0;
    }
#endif /* ST_OSLINUX */

    /* Perform any top-level configuration */
    DeviceConfig();

    /* Now start the tests */
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugMessage("                STBLAST Test Harness               ");
    BLAST_DebugPrintf(("Driver Revision: %s\n", STBLAST_GetRevision()));
    BLAST_DebugPrintf(("Test Harness Revision: %s\n", Revision));
    BLAST_DebugPrintf(("Clock Speed : %d\n", CPU_CLOCK));
    BLAST_DebugPrintf(("IRB Clock   : %d\n", IRB_CLOCK));
    BLAST_DebugMessage("**************************************************");


    /* Get and print the supported protocols from the driver */
    PrintSupportedProtocols();

    InitCommands();

    STTST_SetMode(STTST_INTERACTIVE_MODE);       /* set to interactive mode */

    STTST_Start();                               /* Launch the Command interpretor */

    STBLAST_Print(("\nTEST END. \n"));
    STTST_Term();                                /*terminate it*/

    BLAST_Term();
    PIO_Term();
    EVT_Term();

    return 0;
}

/*****************************************************************************
Initialization
*****************************************************************************/

/* Configure Config Control register for IR receiver */
void DeviceConfig(void)
{

#if !defined(ST_5518) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_7109) && !defined(ST_5188) \
&& !defined(ST_5525) && !defined(ST_7200)
    U32 ReadValue;
    /* Config Control D - wakeup from PIO5 bit 0 */
#endif

#if defined(ST_5105) || defined(ST_5107)

    /* DISABLE IR disable bit of Config Control C register */
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_C);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_C, STSYS_ReadRegDev32LE(CONFIG_CONTROL_C) & 0xFFBFFFFF);
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_C);

    /* Setting PIO 2 bit 4 in to Alternate mode 1 */
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_F);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) | 0x00100000);
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_F);

#endif

#if defined(ST_8010)
    ReadValue = STSYS_ReadRegDev32LE(SYSCONF_CON1B);

    /* Enable irb_enable on comms */
    STSYS_WriteRegDev32LE(SYSCONF_CON1B,(ReadValue | 0x40010000));
#endif

#if defined(ST_5100) || defined(ST_5301)
 /* Configure Config Control Reg G for selecting ALT0 functions for PIO3<3:2> */
   /* 0000_0000_0000_0000_0000_0000_0000_0000 (INT_CONFIG_CONTROL_G)            */
   /* |||| |||| |||| |||| |||| |||| |||| ||||_____ pio2_altfop_mux0sel<7:0>     */
   /* |||| |||| |||| |||| |||| ||||_______________ pio2_altfop_mux1sel<7:0>     */
   /* |||| |||| |||| ||||_________________________ pio3_altfop_mux0sel<7:0>     */
   /* |||| ||||___________________________________ pio3_altfop_mux1sel<7:0>     */
   /* ========================================================================= */
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) & 0x0F0FFFFF );
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
#endif

#if defined(mb376)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x3C), ReadValue|=0x40);
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
#elif defined(espresso)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x3C), ReadValue|=0x00020);
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
#elif !defined(ST_5100) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_7100) && !\
defined(ST_5301) && !defined(ST_8010) && !defined(ST_7109) && !defined(ST_5188) && !defined(ST_5525)\
&& !defined(ST_5107) && !defined(ST_7200)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x0C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x0C), ReadValue|=0x00200000);
#endif

}

/* Configure Config Control register for UHF receiver */
void DeviceConfig_UHF(void)
{
#if !defined(ST_5100) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_5301) && !\
defined(ST_7109) && !defined(ST_7200)
    U32 ReadValue;
#endif

#if defined(ST_5105) || defined(ST_5107)
    /* Enable IR Enable bit of Config Control C register */

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_C);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_C, STSYS_ReadRegDev32LE(CONFIG_CONTROL_C) & 0xFFBFFFFF);
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_C);

    /* Setting PIO 2 bit 4 in to Alternate mode 0 */
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_F);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) & 0xFFEFFFFF);
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_F);
#endif

    /* Config Control D - wakeup from PIO5 bit 0 */
#if !defined(ST_5105) &&  !defined(ST_5107)
#if defined(mb376)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x3C), ReadValue|=0x20);
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
#elif defined(espresso)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x3C), ReadValue|=0x00040);
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x3C);
#elif !defined(ST_5100) && !defined(ST_7710) && !defined(ST_7100) && !defined(ST_5301) && !\
defined(ST_7109) && !defined(ST_7200)
    ReadValue = STSYS_ReadRegDev32LE((U32)CFG_BASE_ADDRESS + 0x0C);
    STSYS_WriteRegDev32LE(((U32)CFG_BASE_ADDRESS + 0x0C), ReadValue|=0x00400000);
#endif
#endif


}

#if defined(STBLAST_USE_UART)

#if defined(ST_8010)

#define ASC_BASE_ADDRESS    ASC_2_BASE_ADDRESS
#define ASC_INTERRUPT       ASC_2_INTERRUPT
#define ASC_INTERRUPT_LEVEL ASC_2_INTERRUPT_LEVEL

#define ASC_TXD_BIT   PIO_BIT_3
#define ASC_RXD_BIT   PIO_BIT_4
#define ASC_TXD_DEV   PIO_DEVICE_1
#define ASC_RXD_DEV   PIO_DEVICE_1
#define ASC_RTS_BIT   PIO_BIT_4
#define ASC_CTS_BIT   PIO_BIT_5
#define ASC_RTS_DEV   PIO_DEVICE_2
#define ASC_CTS_DEV   PIO_DEVICE_2

#elif defined(ST_5301)

#define ASC_BASE_ADDRESS    ASC_3_BASE_ADDRESS
#define ASC_INTERRUPT       ASC_3_INTERRUPT
#define ASC_INTERRUPT_LEVEL ASC_3_INTERRUPT_LEVEL

#define ASC_TXD_BIT   PIO_BIT_0
#define ASC_RXD_BIT   PIO_BIT_1
#define ASC_TXD_DEV   PIO_DEVICE_4
#define ASC_RXD_DEV   PIO_DEVICE_4
#define ASC_RTS_BIT   PIO_BIT_2
#define ASC_CTS_BIT   PIO_BIT_3
#define ASC_RTS_DEV   PIO_DEVICE_4
#define ASC_CTS_DEV   PIO_DEVICE_4

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)

#define ASC_BASE_ADDRESS    ASC_3_BASE_ADDRESS
#define ASC_INTERRUPT       ASC_3_INTERRUPT
#define ASC_INTERRUPT_LEVEL ASC_3_INTERRUPT_LEVEL

#define ASC_TXD_BIT   PIO_BIT_4
#define ASC_RXD_BIT   PIO_BIT_5
#define ASC_TXD_DEV   PIO_DEVICE_5
#define ASC_RXD_DEV   PIO_DEVICE_5
#define ASC_RTS_BIT   PIO_BIT_7
#define ASC_CTS_BIT   PIO_BIT_6
#define ASC_RTS_DEV   PIO_DEVICE_5
#define ASC_CTS_DEV   PIO_DEVICE_5

#elif defined(ST_7100) || defined(ST_7200)

#define ASC_BASE_ADDRESS    ASC_3_BASE_ADDRESS
#define ASC_INTERRUPT       ASC_3_INTERRUPT
#define ASC_INTERRUPT_LEVEL ASC_3_INTERRUPT_LEVEL

#define ASC_TXD_BIT   PIO_BIT_0
#define ASC_RXD_BIT   PIO_BIT_1
#define ASC_TXD_DEV   PIO_DEVICE_5
#define ASC_RXD_DEV   PIO_DEVICE_5
#define ASC_RTS_BIT   PIO_BIT_3
#define ASC_CTS_BIT   PIO_BIT_2
#define ASC_RTS_DEV   PIO_DEVICE_5
#define ASC_CTS_DEV   PIO_DEVICE_5

#endif

BOOL UART_Init(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    STUART_InitParams_t UARTInitParams;

    UARTInitParams.UARTType        = STUART_RTSCTS;
    UARTInitParams.DriverPartition = (ST_Partition_t *)system_partition;
    UARTInitParams.BaseAddress     = (U32 *)ASC_BASE_ADDRESS;
    UARTInitParams.InterruptNumber = ASC_INTERRUPT;
    UARTInitParams.InterruptLevel  = ASC_INTERRUPT_LEVEL;
    UARTInitParams.ClockFrequency  = ClockInfo.CommsBlock;
    UARTInitParams.SwFIFOEnable    = TRUE;
    UARTInitParams.FIFOLength      = 4096;
    strcpy(UARTInitParams.RXD.PortName, PioDeviceName[ASC_RXD_DEV]);
    UARTInitParams.RXD.BitMask     = ASC_RXD_BIT;
    strcpy(UARTInitParams.TXD.PortName, PioDeviceName[ASC_TXD_DEV]);
    UARTInitParams.TXD.BitMask     = ASC_TXD_BIT;
    strcpy(UARTInitParams.RTS.PortName, PioDeviceName[ASC_RTS_DEV]);
    UARTInitParams.RTS.BitMask     = ASC_RTS_BIT;
    strcpy(UARTInitParams.CTS.PortName, PioDeviceName[ASC_CTS_DEV]);
    UARTInitParams.CTS.BitMask     = ASC_CTS_BIT;
    UARTInitParams.DefaultParams   = NULL;

    error = STUART_Init("asc",&UARTInitParams);
    if(ST_NO_ERROR != error)
    {
        printf("STUART_Init() can not Initialize %d \n", error);
    }

#if defined(ST_8010)

    /* For enabling TX on ASC2 */
    STSYS_WriteRegDev32LE(0x5000301C,0x00000800 );

#endif

    return error ? TRUE : FALSE;

}
#endif /* architecture St200 */

#if !defined(ST_OSLINUX)
BOOL TBX_Init(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

#if !defined(DISABLE_TBX)
    STTBX_InitParams_t TBXInitParams;

    /* Initialize the toolbox */
#if defined(STBLAST_USE_UART)
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU | STTBX_DEVICE_UART;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
#else
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
#endif
    TBXInitParams.CPUPartition_p      = system_partition;
    strcpy(TBXInitParams.UartDeviceName, "asc");

    error = STTBX_Init( "STTBX:0", &TBXInitParams );
    BLAST_DebugError("STTBX_Init()", error);

#endif
    return error ? TRUE : FALSE;
}

BOOL BOOT_Init(void)
{
    ST_ErrorCode_t error;
    STBOOT_InitParams_t BootParams;

#ifndef DISABLE_DCACHE
#if defined CACHEABLE_BASE_ADDRESS         /* defined in individual board files mb314 and later */
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS}, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5528) || defined(ST_5100)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40000000, (U32 *)0x5FFFFFFF}, /* ok */
        { NULL, NULL }
    };

#elif defined(ST_5301)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#endif

    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
#ifdef DISABLE_ICACHE
    BootParams.ICacheEnabled = FALSE;
#else
    BootParams.ICacheEnabled = TRUE;
#endif

#ifdef DISABLE_DCACHE
    BootParams.DCacheMap = NULL;
#else
    BootParams.DCacheMap = DCacheMap;
#endif

    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

    /* Constant, defined in include; set automatically
     * for board configuration.
     */
    BootParams.MemorySize = SDRAM_SIZE;
#ifdef ST_OS21
    BootParams.TimeslicingEnabled = TRUE;
#endif

    /* Initialize the boot driver */
    error = STBOOT_Init( "STBOOT:0", &BootParams );
    if(ST_NO_ERROR == error)
    {
        printf("STBOOT is initialize with no error \n");
    }
    else
    {
        printf("STBOOT_Init() can not Initialize %d \n", error);
    }

    /* Obtain clock information */
    ST_GetClockInfo(&ClockInfo);

    return error ? TRUE : FALSE;
}

#endif /* ST_OSLINUX */

BOOL EVT_Init(void)
{
    int i;
    ST_ErrorCode_t error = ST_NO_ERROR;
    STEVT_InitParams_t EVTInitParams;
    STEVT_OpenParams_t EVTOpenParams;

    EVTInitParams.EventMaxNum = EVT_MAX_EVENTS;
    EVTInitParams.ConnectMaxNum = EVT_MAX_CONNECTIONS;
    EVTInitParams.SubscrMaxNum = EVT_MAX_SUBSCRIBERS;
    EVTInitParams.MemoryPartition = system_partition;
    EVTInitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    for (i = 0; i < NUMBER_EVT && error == ST_NO_ERROR; i++)
    {
        error = STEVT_Init(EvtDeviceName[i], &EVTInitParams);
        BLAST_DebugError("STEVT_Init()", error);

        if (error == ST_NO_ERROR)
        {
            error = STEVT_Open(EvtDeviceName[i], &EVTOpenParams,
                               &EvtHandle[i]);
            BLAST_DebugError("STEVT_Open()", error);

        }
    }
    return error ? TRUE : FALSE;
}

BOOL EVT_Term(void)
{
    ST_ErrorCode_t  error = ST_NO_ERROR;
    STEVT_TermParams_t TermParams;
    int LoopCount;

    TermParams.ForceTerminate = TRUE;

    for(LoopCount = 0; LoopCount < NUMBER_EVT && error == ST_NO_ERROR; LoopCount++)
    {
        error = STEVT_Term(EvtDeviceName[LoopCount], &TermParams);
    }

    return error ? TRUE : FALSE;
}

BOOL PIO_Init(void)
{
    ST_ErrorCode_t error;
    STPIO_InitParams_t PioInitParams;

    PioInitParams.BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams.InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams.InterruptLevel  = PIO_0_INTERRUPT_LEVEL;
    PioInitParams.DriverPartition = system_partition;
    error = STPIO_Init(PioDeviceName[PIO_DEVICE_0],
                       &PioInitParams);
    BLAST_DebugError("STPIO_Init()", error);
    if(ST_NO_ERROR != error)
    {
        printf("STPIO_Init() can not Initialize %d \n", error);
    }

    if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_1_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_1_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_1],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }

    if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_2_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_2_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_2],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }

    if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_3_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_3],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }

    }
#if !defined(ST_5105) && !defined(ST_5188) && !defined(ST_5107)
    if (error == ST_NO_ERROR)
    {

        PioInitParams.BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_4_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_4],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }

    if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_5_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_5],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }
#endif
#if defined(ST_7200)
    if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_6_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_6_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_6],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }
	if (error == ST_NO_ERROR)
    {
        PioInitParams.BaseAddress     = (U32 *)PIO_7_BASE_ADDRESS;
        PioInitParams.InterruptNumber = PIO_7_INTERRUPT;
        PioInitParams.InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
        PioInitParams.DriverPartition = system_partition;
        error = STPIO_Init(PioDeviceName[PIO_DEVICE_7],
                           &PioInitParams);
        BLAST_DebugError("STPIO_Init()", error);
        if(ST_NO_ERROR != error)
        {
            printf("STPIO_Init() can not Initialize %d \n", error);
        }
    }
#endif
    return error ? TRUE : FALSE;
}

BOOL TST_Init(void)
{
    STTST_InitParams_t STTST_InitParams;
    BOOL error;

    STTST_InitParams.CPUPartition_p = system_partition;
    STTST_InitParams.NbMaxOfSymbols = 300;
    /* Set input file name array to all zeros */

    memset(STTST_InitParams.InputFileName, 0, sizeof(char) * max_tok_len);

    /* strcpy(STTST_InitParams.InputFileName, "stpti.com"); */

    if (STTST_Init(&STTST_InitParams) == ST_NO_ERROR)
    {
        printf("STTST_Init()=OK\n");
        error = FALSE;
    }
    else
    {
        printf("STTST_Init()=FAILED\n");
        error = TRUE;
    }
    return error;
}

BOOL BLAST_Init(void)
{
    ST_ErrorCode_t error;
    STBLAST_InitParams_t BLASTInitParams;
    STBLAST_OpenParams_t BlastOpenParams;
    STEVT_DeviceSubscribeParams_t EvtSubParams;

    /* Set glitch width */
    BlastOpenParams.RxParams.GlitchWidth = 50;

    BLASTInitParams.DeviceType = BLAST_TXD_TYPE;
    BLASTInitParams.DriverPartition = system_partition;
    BLASTInitParams.ClockSpeed = IRB_CLOCK;
    BLASTInitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;
    BLASTInitParams.BaseAddress = (U32 *)IRB_BASE_ADDRESS;
    BLASTInitParams.InterruptNumber = IRB_INTERRUPT;
    BLASTInitParams.InterruptLevel = IRB_INTERRUPT_LEVEL;
    BLASTInitParams.TxPin.BitMask = BLAST_TXD_BIT;
    BLASTInitParams.TxInvPin.BitMask = BLAST_TXDINV_BIT;

#if defined(ST_OSLINUX)
    BLASTInitParams.CmdBufferSize = 10;
#endif

#if defined(ST_5518)
    BLASTInitParams.InputActiveLow = FALSE;
#else
    BLASTInitParams.InputActiveLow = TRUE;
#endif

    strcpy(BLASTInitParams.EVTDeviceName, BLAST_EVT_DEV);
    strcpy(BLASTInitParams.TxInvPin.PortName, BLAST_TXD_PIO);
    strcpy(BLASTInitParams.TxPin.PortName, BLAST_TXD_PIO);


    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &BLASTInitParams);
    BLAST_DebugError("STBLAST_Init()", error);
    if (error == ST_NO_ERROR)
    {
        error = STBLAST_Open(BlasterDeviceName[BLAST_TX_IR], &BlastOpenParams,
                             &BlasterHandle[BLAST_TX_IR]);
        BLAST_DebugError("STBLAST_Open()", error);
        if (error == ST_NO_ERROR)
        {
            /* Subscribe to the relevant blaster events */
            memset(&EvtSubParams, 0, sizeof(EvtSubParams));
            EvtSubParams.NotifyCallback = DevEvtCallback;
            STEVT_SubscribeDeviceEvent(BLAST_EVT_HDL,
                                    BlasterDeviceName[BLAST_TX_IR],
                                    STBLAST_WRITE_DONE_EVT,
                                    &EvtSubParams);
        }
    }

    BLASTInitParams.DeviceType = BLAST_RXD_TYPE;
    strcpy(BLASTInitParams.RxPin.PortName, BLAST_RXD_PIO);
    /* On espresso it needs to set PIO5 bit 1 in input mode */ 
#if defined(espresso)    
    BLASTInitParams.RxPin.BitMask = BLAST_UHF_BIT;
#else
    BLASTInitParams.RxPin.BitMask = BLAST_RXD_BIT;
#endif

    error = STBLAST_Init(BlasterDeviceName[BLAST_RX_IR], &BLASTInitParams);
    BLAST_DebugError("STBLAST_Init()", error);
    if (error == ST_NO_ERROR)
    {
        error = STBLAST_Open(BlasterDeviceName[BLAST_RX_IR], &BlastOpenParams,
                             &BlasterHandle[BLAST_RX_IR]);
        BLAST_DebugError("STBLAST_Open()", error);
        if (error == ST_NO_ERROR)
        {
            /* Subscribe to the relevant blaster events */
            memset(&EvtSubParams, 0, sizeof(EvtSubParams));
            EvtSubParams.NotifyCallback = DevEvtCallback;
            STEVT_SubscribeDeviceEvent(BLAST_EVT_HDL,
                                    BlasterDeviceName[BLAST_RX_IR],
                                    STBLAST_READ_DONE_EVT,
                                    &EvtSubParams);

        }
    }

    /****************** UHF *******************************/
#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5188) && !defined(ST_5107)

    BLASTInitParams.DeviceType = BLAST_UHF_TYPE;
    strcpy(BLASTInitParams.RxPin.PortName, BLAST_UHF_PIO);

#if defined(espresso)    
    BLASTInitParams.RxPin.BitMask = BLAST_RXD_BIT;
#else
    BLASTInitParams.RxPin.BitMask = BLAST_UHF_BIT;
#endif
 
    error = STBLAST_Init(BlasterDeviceName[BLAST_RX_UHF], &BLASTInitParams);
    BLAST_DebugError("STBLAST_Init()", error);
    if (error == ST_NO_ERROR)
    {
        STBLAST_Open(BlasterDeviceName[BLAST_RX_UHF], &BlastOpenParams,
                     &BlasterHandle[BLAST_RX_UHF]);
        BLAST_DebugError("STBLAST_Open()", error);
        if (error == ST_NO_ERROR)
        {
            /* Subscribe to the relevant blaster events*/
            memset(&EvtSubParams, 0, sizeof(EvtSubParams));
            EvtSubParams.NotifyCallback = DevEvtCallback;
            error = STEVT_SubscribeDeviceEvent(BLAST_EVT_HDL,
                                    BlasterDeviceName[BLAST_RX_UHF],
                                    STBLAST_READ_DONE_EVT,
                                    &EvtSubParams);
        }
    }
#endif
/*****************************************************/

    /* Initialize test semaphore */
    BlasterSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
    return error ? TRUE : FALSE;
}

BOOL  BLAST_Term(void)
{
    STBLAST_TermParams_t BLAST_TermParams;
    ST_ErrorCode_t error;

    BLAST_TermParams.ForceTerminate = TRUE;

    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &BLAST_TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() TX", error);
        error = FALSE ;
    }
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_IR], &BLAST_TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() RX", error);
        error = FALSE ;
    }

    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_UHF], &BLAST_TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() UHF", error);
        error = FALSE ;
    }


    return error;
}

BOOL  PIO_Term(void)
{
    STPIO_TermParams_t      PIO_TermParams; 
    ST_ErrorCode_t error;
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;

    PIO_TermParams.ForceTerminate = FALSE;
    
    error = STPIO_Term(PioDeviceName[PIO_DEVICE_0], &PIO_TermParams);
    BLAST_DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
    }
    if (error == ST_NO_ERROR)
    {
        error = STPIO_Term(PioDeviceName[PIO_DEVICE_1], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	        BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
    }
    if (error == ST_NO_ERROR)
    { 
	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_2], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	        BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
	}
	if (error == ST_NO_ERROR)
    {  
	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_3], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	        BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
    }

#if !defined(ST_5105) && !defined(ST_5188) && !defined(ST_5107)
    if (error == ST_NO_ERROR)
    {
	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_4], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	          BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
	}
	if (error == ST_NO_ERROR)
    {
  	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_5], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	           BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
	}
#endif
#if defined(ST_7200)
 	if (error == ST_NO_ERROR)
    {
	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_6], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	          BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
	}
    if (error == ST_NO_ERROR)
    {
	    error = STPIO_Term(PioDeviceName[PIO_DEVICE_7], &PIO_TermParams);
	    BLAST_DebugError("STPIO_Term()", error);
	    if (error == ST_NO_ERROR)
	    {
	          BLAST_TestPassed(Result);
	    }
	    else
	    {
	        BLAST_TestFailed(Result, "Expected ST_NO_ERROR");
	    }
	}
#endif
    return error;
}

void DevEvtCallback(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{

    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(SubscriberData_p);
    
    switch (Event)
    {
        case STBLAST_READ_DONE_EVT:
        case STBLAST_WRITE_DONE_EVT:
            LastEvent = Event;
            LastError = ((STBLAST_EvtReadDoneParams_t *)EventData)->Result;
            STOS_SemaphoreSignal(BlasterSemaphore_p);
            break;
        default:
            break;
    }
}

void EvtCallback(STEVT_CallReason_t Reason,
                 STEVT_EventConstant_t Event,
                 const void *EventData)
{
	UNUSED_PARAMETER(Reason);
    switch (Event)
    {
        case STBLAST_READ_DONE_EVT:
        case STBLAST_WRITE_DONE_EVT:
            LastEvent = Event;
            LastError = ((STBLAST_EvtReadDoneParams_t *)EventData)->Result;
            STOS_SemaphoreSignal(BlasterSemaphore_p);
            break;
        default:
            break;
    }
}
void PrintSupportedProtocols(void)
{
    ST_ErrorCode_t          error;
    STBLAST_Capability_t    Cap;

    /* We're initialising the same driver version each time, so getting
     * data from one suffices for all.
     */
    error = STBLAST_GetCapability(BlasterDeviceName[BLAST_TX_IR], &Cap);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugPrintf(("Error - could not get capabilities! %s\n",
                            BLAST_ErrorString(error)));
    }
    else
    {
        BLAST_DebugMessage("Supported protocols:");
        if (Cap.SupportedProtocols & (STBLAST_PROTOCOL_USER_DEFINED))
        {
            BLAST_DebugMessage("\t User defined protocol");
        }
        if (Cap.SupportedProtocols & (STBLAST_PROTOCOL_RC5))
        {
            BLAST_DebugMessage("\t RC5 protocol");
        }
        if (Cap.SupportedProtocols & (STBLAST_PROTOCOL_RC6A))
        {
            BLAST_DebugMessage("\t RC6A mode 6");
        }
        if ( Cap.SupportedProtocols & (STBLAST_PROTOCOL_RC6_MODE0))
        {
            BLAST_DebugMessage("\t RC6A mode 0");
        }
        if( Cap.SupportedProtocols & (STBLAST_LOW_LATENCY_PROTOCOL))
        {
            BLAST_DebugMessage("\t Low Latency Protocol");
        }
    }
}

/*****************************************************************************
Test routines
*****************************************************************************/
void BLASTDataUsage(BLAST_TestParams_t *Params_p)
{
    U8 *tmp;
    U32 add1, add2;
    ST_ErrorCode_t error;
    STBLAST_TermParams_t TermParams;
    STBLAST_InitParams_t InitParams;
    UNUSED_PARAMETER(Params_p);    
    TermParams.ForceTerminate = FALSE;

    InitParams.DeviceType       = BLAST_TXD_TYPE;
    InitParams.DriverPartition  = system_partition;
    InitParams.ClockSpeed       = IRB_CLOCK;
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;
    /* Can legitimately be 0 */
    InitParams.BaseAddress      = (U32 *)IRB_BASE_ADDRESS;
    InitParams.InterruptNumber  = IRB_INTERRUPT;
    InitParams.InterruptLevel   = IRB_INTERRUPT_LEVEL;
    InitParams.TxPin.BitMask    = BLAST_TXD_BIT;
    InitParams.TxInvPin.BitMask = BLAST_TXDINV_BIT;
    strcpy(InitParams.EVTDeviceName, BLAST_EVT_DEV);

    strcpy(InitParams.TxInvPin.PortName, BLAST_TXD_PIO);
    strcpy(InitParams.TxPin.PortName, BLAST_TXD_PIO);

#if defined(ST_5518)
    InitParams.InputActiveLow = FALSE;
#else
    InitParams.InputActiveLow = TRUE;
#endif

/*============Inits with different buffer size=============================*/
    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        BLAST_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);


    /* First init, buffer size SYMBOL_BUFFER_SIZE (TXD)*/
    error = STBLAST_Init("InitName", &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Init with buffer size = SYMBOL_BUFFER_SIZE: TX; used memory: %i\n", add1 - add2));
    error = STBLAST_Term("InitName", &TermParams);


    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        BLAST_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);


    /* Increase buffer size by 200 */
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE+200;

    error = STBLAST_Init("InitName", &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Init with buffer size = SYMBOL_BUFFER_SIZE + 200: TX; used memory: %i\n", add1 - add2));
    error = STBLAST_Term("InitName", &TermParams);

    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        BLAST_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);

    /* increase buffer size by 200 */
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE+400;

    error = STBLAST_Init("InitName", &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Init with buffer size = SYMBOL_BUFFER_SIZE + 400: TX; used memory: %i\n", add1 - add2));
    error = STBLAST_Term("InitName", &TermParams);


    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        BLAST_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);


    /* decrease buffer size by 200 */
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE+200;

    error = STBLAST_Init("InitName", &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Init with buffer size = SYMBOL_BUFFER_SIZE + 200: TX; used memory: %i\n", add1 - add2));
    error = STBLAST_Term("InitName", &TermParams);

    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE; /* reset buffer size */

/*=====================Sequential inits, same buffersize=======================*/

    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        BLAST_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);


    /* First init (TXD)*/
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("First Init: TX; used memory: %i\n", add1 - add2));

    /* Second init (RXD)*/
    InitParams.DeviceType = BLAST_RXD_TYPE;

    strcpy(InitParams.RxPin.PortName, BLAST_RXD_PIO);
    InitParams.RxPin.BitMask = BLAST_RXD_BIT;

    error = STBLAST_Init(BlasterDeviceName[BLAST_RX_IR], &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Second Init: RX; used memory: %i\n", add1 - add2));

/* Third init (RXD)*/
    InitParams.DeviceType = BLAST_UHF_TYPE;

    strcpy(InitParams.RxPin.PortName, BLAST_UHF_PIO);
    InitParams.RxPin.BitMask = BLAST_UHF_BIT;

    error = STBLAST_Init(BlasterDeviceName[BLAST_RX_UHF], &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);
    BLAST_DebugPrintf(("Third Init:UHF-RX; used memory: %i\n", add1 - add2));
    /*Terminate the Instance*/
    TermParams.ForceTerminate = TRUE;

    /* term open instances */
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() TX", error);
        return;
    }
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_IR], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() RX", error);
        return;
    }
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_UHF], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() UHF-RX", error);
        return;
    }

    return;
}

void test_overhead(BLAST_TestParams_t *Params_p) 
{
	UNUSED_PARAMETER(Params_p);    
}

void test_init(BLAST_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STBLAST_InitParams_t InitParams;
    
    UNUSED_PARAMETER(Params_p);    

    InitParams.DeviceType       = BLAST_TXD_TYPE;
    InitParams.DriverPartition  = system_partition;
    InitParams.ClockSpeed       = IRB_CLOCK;
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;
    /* Can legitimately be 0 */
    InitParams.BaseAddress      = (U32 *)IRB_BASE_ADDRESS;
    InitParams.InterruptNumber  = IRB_INTERRUPT;
    InitParams.InterruptLevel   = IRB_INTERRUPT_LEVEL;

    InitParams.TxPin.BitMask    = BLAST_TXD_BIT;
    InitParams.TxInvPin.BitMask = BLAST_TXDINV_BIT;

    strcpy(InitParams.EVTDeviceName, BLAST_EVT_DEV);
    strcpy(InitParams.TxInvPin.PortName, BLAST_TXD_PIO);
    strcpy(InitParams.TxPin.PortName, BLAST_TXD_PIO);

#if defined(ST_5518)
    InitParams.InputActiveLow = FALSE;
#else
    InitParams.InputActiveLow = TRUE;
#endif

    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("Failed to init: ", error);
    }
}
void test_typical(BLAST_TestParams_t *Params_p)
{
    ST_ErrorCode_t          error;
    STBLAST_OpenParams_t    OpenParams;
    STBLAST_Handle_t        Handle;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;

    static U32 Cmd;
    static U32 Counter = 0;
    static U32 NumSent;

    /* Setup the open params */
    OpenParams.RxParams.GlitchWidth = 0;
    UNUSED_PARAMETER(Params_p);    
    /* Open a connection */
    error = STBLAST_Open(BlasterDeviceName[BLAST_TX_IR], &OpenParams, &Handle);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Open()", error);
        return;
    }

    BLAST_DebugMessage("Check for mute toggling...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_PULSE;
    ProtocolParams.UserDefined.SubCarrierFreq = 40000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1800;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 12;

    /* Setup the PULSE coded protocol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 2400;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3000;

    STBLAST_SetProtocol(Handle, Protocol, &ProtocolParams);

    Cmd = RcCodeValue("MUTE", SonyRcCode);

    for (Counter = 0; Counter < 20; Counter++)
    {
        STBLAST_Send(Handle,
                       &Cmd,
                       1,
                       &NumSent,
                       0);

        /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            break;
        }

        STOS_TaskDelay(ST_GetClocksPerSecond()/2);

    }

    /* STBLAST_Close() */
    error = STBLAST_Close(Handle);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Close()", error);
    }
}

void test_term(BLAST_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STBLAST_TermParams_t TermParams;
    UNUSED_PARAMETER(Params_p);    
    TermParams.ForceTerminate = TRUE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("Failed to terminate", error);
    }
}

#if !defined(ST_OSLINUX)
BLAST_TestResult_t BlastStackUsage(BLAST_TestParams_t *Params_p)
{
    BLAST_TestResult_t result = TEST_RESULT_ZERO;

#ifndef ST_OS21
    task_t task;
    tdesc_t tdesc;
    char stack[4 * 1024];
#endif

    task_t *task_p;
    task_status_t status;

    U32  i;
    void (*func_table[])(BLAST_TestParams_t *) = {
                                     test_overhead,
                                     test_init,
                                     test_typical,
                                     test_term,
                                     NULL
                                   };
    void (*func)(void *);
#ifndef ST_OS21
    task_p = &task;
#endif

    for (i = 0; func_table[i] != NULL; i++)
    {
        func = (void (*)(void *))func_table[i];
#ifdef ST_OS21
        task_p = task_create(func, (void *)Params_p, (22 * 1024),
                 MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);

#else
        task_init(func, (void *)Params_p, stack, 4 * 1024, task_p,
                  &tdesc, MAX_USER_PRIORITY, "stack_test", 0);
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);
#endif


        task_delete(task_p);
        BLAST_DebugPrintf(("Function %i, stack used: %i\n",
                          i, status.task_stack_used));
    }

    BLAST_DebugPrintf(("\n"));
    BLAST_DebugMessage( "Data Usage tests\n" );

    BLASTDataUsage(Params_p);

    BLAST_TestPassed(result);

    return result;
}
#endif /* ST_OSLINUX */

BLAST_TestResult_t TestRC5Decode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of RC5 commands from handset");
    Protocol = STBLAST_PROTOCOL_RC5;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol,&ProtocolParams);

    BLAST_DebugMessage("Press fifteen buttons...");
    for (Counter = 0; Counter < 15; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0 );

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString((U8)Cmd, RC5RcCode),Cmd));
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestRCRF8Decode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of RCRF8 commands from handset");
    Protocol = STBLAST_PROTOCOL_RCRF8;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol,&ProtocolParams);

    BLAST_DebugMessage("Press fifteen buttons...");
    for (Counter = 0; Counter < 10; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0 );

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString((U8)Cmd, RCRF8Code),Cmd));
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestMultiProtocol(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[5] = {0};
    U32 Counter;
    U32 NumReceived;
    U32 Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    STBLAST_Protocol_t ProtocolSet;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of multiple protocol with same Sub Carrier Freq");
    Protocol = STBLAST_PROTOCOL_MULTIPLE;

    /* RC6A params */
    ProtocolParams.RC6A.CustomerCode = 0x00000000;
    ProtocolParams.RC6A.BufferElementsPerPayload = 1; /* not used yet */
    ProtocolParams.RC6A.NumberPayloadBits = 12;
    /* RC6AM0 params */
    ProtocolParams.RC6M0.CustomerCode = 39;
    ProtocolParams.RC6M0.BufferElementsPerPayload  = 1; /* not used yet */
    ProtocolParams.RC6M0.NumberPayloadBits = 8;

    /* RCMM params */
    ProtocolParams.Rcmm.NumberPayloadBits = 20;
    ProtocolParams.Rcmm.ShortID = FALSE;

    /* low latency */
    ProtocolParams.LowLatency.SystemCode = 0xC;
    ProtocolParams.LowLatency.NumberPayloadBits = 8;

    STBLAST_SetSubCarrierFreq(BLAST_RXD_HDL, 38000 );

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 50 buttons from RC6A/RUWIDO/RCMM remote...");
    for (Counter = 0; Counter < 50; Counter++)
    {
        STBLAST_ReceiveExt( BLAST_RXD_HDL,
                         Cmd,
                         1,
                         &NumReceived,
                         0,
                         &ProtocolSet );

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            BLAST_DebugPrintf(("Symbols Rejected\n"));
            continue;
        }

        BLAST_DebugPrintf(("NumReceived =%d\n",NumReceived));

        if ( NumReceived != 0 )
        {
            if (ProtocolSet == STBLAST_PROTOCOL_RUWIDO)
            {
            	/* Show command */
            	BLAST_DebugPrintf(("STBLAST_PROTOCOL_RUWIDO\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[0], RuwidoRcCodeRC),Cmd[0]));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[1], RuwidoRcCodeRC),Cmd[1]));
            }
            else if(ProtocolSet == STBLAST_PROTOCOL_RCMM)
            {
            	/* Show command */
            	BLAST_DebugPrintf(("STBLAST_PROTOCOL_RCMM\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[0], RCRcmmCode_32),Cmd[0]));
            }
            else if (ProtocolSet == STBLAST_PROTOCOL_RC6_MODE0)
            {
            	/* Show command */
                BLAST_DebugPrintf(("STBLAST_PROTOCOL_RC6_MODE0\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[0], RC6M0Code),Cmd[0]));
            }
            else if (ProtocolSet == STBLAST_LOW_LATENCY_PROTOCOL)
            {
            	/* Show command */
                BLAST_DebugPrintf(("STBLAST_LOW_LATENCY_PROTOCOL\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[0], LowLatencyCode),Cmd[0]));
            }
            else if (ProtocolSet == STBLAST_PROTOCOL_RC6A)
            {
            	/* Show command */
                BLAST_DebugPrintf(("STBLAST_PROTOCOL_RC6A\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd[0], RC6ARcCode),Cmd[0]));
            }
	        else if (ProtocolSet == STBLAST_PROTOCOL_RC5)
            {
            	/* Show command */
                BLAST_DebugPrintf(("STBLAST_PROTOCOL_RC5\n"));
                BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString((U8)Cmd[0], RC5RcCode),Cmd[0]));
            }
            else
            {
                BLAST_DebugPrintf(("Unknown protocol??\n"));
                BLAST_DebugPrintf(("CMD = 0x%X\n",Cmd[0]));
            }

        }

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestEchoStarRC(BLAST_TestParams_t *Params)
{

    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd = 0, Counter;
    U32 NumReceived=0, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    ST_ErrorCode_t Error;
#ifdef ST_OSLINUX
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received Echo* RC codes...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SPACE;
    ProtocolParams.UserDefined.SubCarrierFreq = 36000;
    ProtocolParams.UserDefined.NumberStartSymbols = 0;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 500;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 3000;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 500;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 2000;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 16;

    Error = STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);
    if ( Error != ST_NO_ERROR )
    {
        printf("SetProtocol failed with error %d\n", Error);
    }

    BLAST_DebugMessage("Press ten buttons...");
    for (Counter = 0; Counter < 25; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);


        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, EchoStarRCCode),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestRMapDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd;
    U32 Counter;
    U32 NumReceived;
    U32 Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of RMap Symbols");
    Protocol = STBLAST_PROTOCOL_RMAP;

    /* RMAP params */
    ProtocolParams.Ruwido.CustomID = 0x00000000;
    ProtocolParams.Ruwido.DeviceID = 0; /* not used yet */
    ProtocolParams.Ruwido.Address  = 5;
    ProtocolParams.Ruwido.FrameLength  = 22;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 20 buttons from RMAP...");
    for (Counter = 0; Counter < 20; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RMapCode),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestRMapDoubleBitDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd;
    U32 Counter;
    U32 NumReceived;
    U32 Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of RMap symbols with Double bit coding\n");
    BLAST_DebugMessage("This test requires 56Khz receiver\n");
    Protocol = STBLAST_PROTOCOL_RMAP_DOUBLEBIT;
   
    /* RMAP params */
    ProtocolParams.Ruwido.CustomID = 0x7;
    ProtocolParams.Ruwido.DeviceID = 0; /* not used yet */
    ProtocolParams.Ruwido.Address  = 0;
    ProtocolParams.Ruwido.FrameLength  = 24;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 20 buttons from RMAP...");
    for (Counter = 0; Counter < 20; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }
        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RMapDoubleBitCode),Cmd));
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}
BLAST_TestResult_t TestRStepDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd;
    U32 Counter;
    U32 NumReceived;
    U32 Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;

#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of rSTEP Symbols");
    Protocol = STBLAST_PROTOCOL_RSTEP;

    /* RMAP params */
    ProtocolParams.Ruwido.CustomID = 0x00000000;
    ProtocolParams.Ruwido.DeviceID = 0; /* not used yet */
    ProtocolParams.Ruwido.Address  = 5;
    ProtocolParams.Ruwido.FrameLength  = 23;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 20 buttons from rSTEP...");
    for (Counter = 0; Counter < 20; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RMapCode),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestRC6_Mode_0(BLAST_TestParams_t *Params)
{
    STBLAST_Protocol_t Protocol;
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_ProtocolParams_t ProtocolParams;
    U32 Counter,Cmd,Failed = 0;
    U32 NumReceived = 0;
    BLAST_DebugMessage("Test decode of RC6 mode 0 commands from handset");
    Protocol = STBLAST_PROTOCOL_RC6_MODE0;
    UNUSED_PARAMETER(Params);    
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    ProtocolParams.RC6M0.CustomerCode = 39;
    ProtocolParams.RC6M0.BufferElementsPerPayload  = 1; /* not used yet */
    ProtocolParams.RC6M0.NumberPayloadBits = 8;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 15 buttons...");
    for (Counter = 0; Counter < 15; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
            
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RC6M0Code),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestRcmm_24_32_Mode(BLAST_TestParams_t *Params)
{

    STBLAST_Protocol_t Protocol;
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_ProtocolParams_t ProtocolParams;
    U32 Counter, Cmd, NumReceived, Failed = 0;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of RCMM 24 and 32");
    Protocol = STBLAST_PROTOCOL_RCMM;

    BLAST_DebugMessage(" Configure remote in RCMM 32 mode ( Press 0,0,1 with Menu)");
    ProtocolParams.Rcmm.NumberPayloadBits = 20;
    ProtocolParams.Rcmm.ShortID = FALSE;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 20 buttons...");
    for (Counter = 0; Counter < 20; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RCRcmmCode_32),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    BLAST_DebugMessage(" Configure remote in RCMM 24 mode ( Press 0,0,3 with Menu)");

    ProtocolParams.Rcmm.NumberPayloadBits = 12;
    ProtocolParams.Rcmm.ShortID = TRUE;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 15 buttons...");
    for (Counter = 0; Counter < 15; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, RCRcmmCode_24),Cmd));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestLowLatencyDecode (BLAST_TestParams_t *Params)
{

    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of Low Latency commands from handset");
    Protocol = STBLAST_LOW_LATENCY_PROTOCOL;

    ProtocolParams.LowLatency.SystemCode = 0xC;
    ProtocolParams.LowLatency.NumberPayloadBits = 8;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);


    BLAST_DebugMessage("Press fifteen buttons...");
    for (Counter = 0; Counter < 15; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);


        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, LowLatencyCode),Cmd));

    }

    /* Await result completion */
    BLAST_DebugPrintf(("Failed =%d\n",Failed));

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

BLAST_TestResult_t TestLowLatencyProDecode (BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test decode of Low Latency commands from handset");
    BLAST_DebugMessage("UHF Protocol  \n");
    Protocol = STBLAST_LOW_LATENCY_PRO_PROTOCOL;

    ProtocolParams.RFLowLatencyPro.SystemCode = 0x1;
    ProtocolParams.RFLowLatencyPro.UserCode = 0;
    ProtocolParams.RFLowLatencyPro.LongAddress = 0;    
    ProtocolParams.RFLowLatencyPro.NumberPayloadBits = 8;
    
    STBLAST_SetProtocol(BLAST_UHF_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press fifteen buttons...");
    for (Counter = 0; Counter < 15; Counter++)
    {
        STBLAST_Receive( BLAST_UHF_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);


        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_UHF_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_UHF_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        /* Megha */
        BLAST_DebugPrintf(("CMD = %s 0x%X\n", RcCodeString(Cmd, LowLatencyCode),Cmd));

    }

    /* Await result completion */
    BLAST_DebugPrintf(("Failed =%d\n",Failed));

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;

}

#if defined(AUTO_TESTING_ENABLE)
#if !defined(ST_OSLINUX)
BLAST_TestResult_t TestRC5(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded, i, j, SymbolsConsumed, DecodedCount, Last = 0;
    U32 TxCmd, RxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_Protocol_t ProtocolSet;

    Protocol = STBLAST_PROTOCOL_RC5;

    BLAST_DebugMessage("Test: Encode -> Decode cycle");

    for (i = 0; i < 0x2000; i++)
    {
        /* Strip out start bit */
        TxCmd = i & 0x1FFF;             /*WHAT DOES THIS DO??*/

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

        BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       NULL);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 28;
            SymbolBuf[j].MarkPeriod *= 28;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       NULL,
                       &ProtocolSet);

        if (TxCmd != RxCmd)
        {
            break;
        }

        Last = ShowProgress(i, 0x1FFF, Last);
    }

    if (i != 0x2000)
    {
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x\n", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}
#endif/*AUTO_TESTING_ENABLE*/


#if defined(AUTO_TESTING_ENABLE)
BLAST_TestResult_t TestLowLatency(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded, i, j, SymbolsConsumed, DecodedCount, Last = 0;
    U32 TxCmd, RxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_Protocol_t ProtocolSet;
    STBLAST_ProtocolParams_t ProtocolParams;

    Protocol = STBLAST_LOW_LATENCY_PROTOCOL;

    ProtocolParams.LowLatency.SystemCode = 0xC;
    ProtocolParams.LowLatency.NumberPayloadBits = 8;


    BLAST_DebugMessage("Test: Encode -> Decode cycle");

    for (i = 0; i <= 0xFF; i++)
    {
        /* Strip out start bit */
        TxCmd = i ;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

       BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       &ProtocolParams);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 26;
            SymbolBuf[j].MarkPeriod *= 26;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);

        if (TxCmd != RxCmd)
        {
            break;
        }

        Last = ShowProgress(i, 0xFF, Last);
    }

    if (i != 0x100)
    {
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x\n", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}
#endif/*AUTO_TESTING_ENABLE*/

#if defined(AUTO_TESTING_ENABLE)
BLAST_TestResult_t TestRcmm(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded, i, j, SymbolsConsumed, DecodedCount, Last = 0;
    U32 TxCmd, RxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    STBLAST_Protocol_t ProtocolSet;

    Protocol = STBLAST_PROTOCOL_RCMM;

    /* Checking 32 bit mode */
    BLAST_DebugMessage("Testing : 32 bit mode ");

    ProtocolParams.Rcmm.NumberPayloadBits = 20;
    ProtocolParams.Rcmm.ShortID           = FALSE;
    ProtocolParams.Rcmm.SubMode           = 0;
    ProtocolParams.Rcmm.Address           = 0;
    ProtocolParams.Rcmm.CustomerID        = 37;

    BLAST_DebugMessage(("Test: Encode -> Decode cycle"));

    for (i = 0; i <= 0xFFFF; i++)
    {
        /* Strip out start bit */
        TxCmd = i ;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

       BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       &ProtocolParams);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 28;
            SymbolBuf[j].MarkPeriod *= 28;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);
        /* Compare the data field */
        {
            /* here short mode id is false
             * so mode will 001
             */

            U32 temp, temp1;
            if ((RxCmd >> 29) != 1)
            {
                break;
            }

            /* Compare Customer ID */
            temp = RxCmd << 3;
            temp1= temp >> 23;
            if (temp1 != ProtocolParams.Rcmm.CustomerID)
            {
                break;
            }

            /* Compare Submode */
            temp = RxCmd << 12;
            temp1 = temp >> 30;
            if (temp1 != ProtocolParams.Rcmm.SubMode)
            {
                break;
            }

            /* compare ADDRESS */
            temp = RxCmd << 14;
            temp1 = temp >> 30;
            if (temp1 != ProtocolParams.Rcmm.Address)
            {
                break;
            }

            /* Compare command */
            temp = RxCmd << 16;
            temp1 = temp >> 16;
            if (temp1 != TxCmd)
            {
                break;
            }

        }



        Last = ShowProgress(i, 0xFFFF, Last);
    }

    if (i != 0x10000)
    {
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x\n", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    /* Checking 24 bit mode */
    BLAST_DebugMessage("Testing : 24 bit mode ");

    ProtocolParams.Rcmm.NumberPayloadBits = 12;
    ProtocolParams.Rcmm.ShortID           = TRUE;
    ProtocolParams.Rcmm.SubMode           = 0;
    ProtocolParams.Rcmm.Address           = 0;
    ProtocolParams.Rcmm.CustomerID        = 32;

    for (i = 0; i <= 0xFF; i++)
    {
        /* Strip out start bit */
        TxCmd = i ;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

       BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       &ProtocolParams);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 28;
            SymbolBuf[j].MarkPeriod *= 28;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);
        /* Compare the data field */
        {
            /* here short mode id is true
             * so mode will 000011
             */

            U32 temp, temp1;
            if ((RxCmd >> 18) != 3)
            {
                break;
            }

            /* Compare Customer ID */
            temp = RxCmd << 14;
            temp1= temp >> 26;
            if (temp1 != ProtocolParams.Rcmm.CustomerID)
            {
                break;
            }

            /* Compare Submode */
            temp = RxCmd << 20;
            temp1 = temp >> 30;
            if (temp1 != ProtocolParams.Rcmm.SubMode)
            {
                break;
            }

            /* compare ADDRESS */
            temp = RxCmd << 22;
            temp1 = temp >> 30;
            if (temp1 != ProtocolParams.Rcmm.Address)
            {
                break;
            }

            /* Compare command */
            temp = RxCmd << 24;
            temp1 = temp >> 24;
            if (temp1 != TxCmd)
            {
                break;
            }

        }



        Last = ShowProgress(i, 0xFF, Last);
    }

    if (i != 0x100)
    {
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x\n", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}
#endif/*AUTO_TESTING_ENABLE*/

#if defined(AUTO_TESTING_ENABLE)
BLAST_TestResult_t TestRC6AMode0(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded, i, j, SymbolsConsumed, DecodedCount, Last = 0;
    U32 TxCmd, RxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    STBLAST_Protocol_t ProtocolSet;
    UNUSED_PARAMETER(Params);    
    Protocol = STBLAST_PROTOCOL_RC6_MODE0;

    ProtocolParams.RC6M0.CustomerCode = 39;
    ProtocolParams.RC6M0.NumberPayloadBits = 8;
    ProtocolParams.RC6M0.BufferElementsPerPayload = 1;
    BLAST_DebugMessage("Test: Encode -> Decode cycle");

    for (i = 0; i <= 0xFF; i++)
    {
        /* Strip out start bit */
        TxCmd = i ;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

/*
    if(TxCmd == 0x80)
    {
    	printf("\n wrong command \n");
    	continue;
    }
*/
       BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       &ProtocolParams);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 28;
            SymbolBuf[j].MarkPeriod *= 28;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);

        /* As Rc6 mode 0 protocol returns The toggle bit, System bits and command bits
          and we need only last 8 bits (command bits) */

       RxCmd &=0xFF;

        if (TxCmd != RxCmd)
        {
            break;
        }

        Last = ShowProgress(i, 0xFF, Last);
    }

    if (i != 0x100)
    {
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x\n", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}
#endif/*AUTO_TESTING_ENABLE*/

#if defined(AUTO_TESTING_ENABLE)
BLAST_TestResult_t TestSpace(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded, i, j, SymbolsConsumed, DecodedCount, Last = 0;
    U32 TxCmd, RxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    STBLAST_Protocol_t ProtocolSet;

    Protocol = STBLAST_PROTOCOL_USER_DEFINED;

    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SPACE;
    ProtocolParams.UserDefined.SubCarrierFreq = 36000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 2;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 2000;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 3000;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 16;

    /* Single start symbol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 2000;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 4000;

    /* Multiple stop symbols, to check that works */
    ProtocolParams.UserDefined.StopSymbols[0].MarkPeriod = 2000;
    ProtocolParams.UserDefined.StopSymbols[0].SymbolPeriod = 4000;
    ProtocolParams.UserDefined.StopSymbols[1].MarkPeriod = 3000;
    ProtocolParams.UserDefined.StopSymbols[1].SymbolPeriod = 5000;

    BLAST_DebugMessage("Test: Encode -> Decode cycle");

    for (i = 0; i <= 0xFFFF; i++)
    {
        TxCmd = i;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\n", TxCmd));
#endif

        BLAST_DoEncode(&TxCmd,
                       1,
                       SymbolBuf,
                       SYMBOL_BUFFER_SIZE,
                       &SymbolsEncoded,
                       Protocol,
                       &ProtocolParams);

        if (SymbolsEncoded == 0)
        {
            break;
        }

        /* Convert all symbols to microseconds -- for loopback */
        for (j = 0; j < SymbolsEncoded; j++)
        {
            SymbolBuf[j].SymbolPeriod *= 28;
            SymbolBuf[j].MarkPeriod *= 28;
        }

        BLAST_DoDecode(&RxCmd,
                       1,
                       SymbolBuf,
                       SymbolsEncoded,
                       &DecodedCount,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);

        if (TxCmd != RxCmd)
        {
            break;
        }

        Last = ShowProgress(i, 0xFFFF, Last);
    }

    if (i != 0x10000)
    {
        BLAST_DebugPrintf(("Symbols Encoded %d\n", SymbolsEncoded));
        sprintf(ErrMsg, "Failed on command 0x%04x - received 0x%04x", i, RxCmd);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}
#endif /* ST_OSLINUX */
#endif/*AUTO_TESTING_ENABLE*/

BLAST_TestResult_t TestAbortSend(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 Commands[10];
    U32 NumberDone;
    STBLAST_Protocol_t Protocol;
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test: try aborting a send operation");

    Protocol = STBLAST_PROTOCOL_RC5;

    BLAST_DebugMessage("Using RC5 coding @ 36KHz");
    error = STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, NULL);
    if (error == ST_NO_ERROR)
    {
        memset(Commands, 0, sizeof(Commands));
        error = STBLAST_Send(BLAST_TXD_HDL, Commands, 10, &NumberDone, 0);
        if (error == ST_NO_ERROR)
        {
            STOS_TaskDelay(20);
            error = STBLAST_Abort(BLAST_TXD_HDL);

            if (error == ST_NO_ERROR)
            {
                STOS_SemaphoreWait(BlasterSemaphore_p);
                if (LastError == STBLAST_ERROR_ABORT)
                {
                    BLAST_TestPassed(Result);
                    BLAST_DebugPrintf(("Aborted - sent %i commands\n", NumberDone));
                }
                else
                {
                    sprintf(ErrMsg, "Got %s, instead of STBLAST_ERROR_ABORT\n",
                    BLAST_ErrorString(LastError));
                    BLAST_TestFailed(Result, ErrMsg);
                }
            }
            else
            {
                sprintf(ErrMsg, "Failed to abort transmission - %s\n", BLAST_ErrorString(error));
                BLAST_TestFailed(Result, ErrMsg);
                STOS_SemaphoreWait(BlasterSemaphore_p);
            }
        }
        else
        {
            sprintf(ErrMsg, "Failed to send commands - %s\n", BLAST_ErrorString(error));
            BLAST_TestFailed(Result, ErrMsg);
        }
    }
    else
    {
        sprintf(ErrMsg, "Failed to Set Protocol - %s\n", BLAST_ErrorString(error));
        BLAST_TestFailed(Result, ErrMsg);
    }

    return Result;
}


BLAST_TestResult_t TestAbortReceive(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 Commands[10];
    U32 NumberDone;
    STBLAST_Protocol_t Protocol;
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Test: try aborting a receive operation");
    Protocol = STBLAST_PROTOCOL_RC5;

    BLAST_DebugMessage("Using RC5 coding @ 36KHz");
    error = STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, NULL);
    if (error == ST_NO_ERROR)
    {
        memset(Commands, 0, sizeof(Commands));
        error = STBLAST_Receive(BLAST_RXD_HDL, Commands, 10, &NumberDone, 0);
        if (error == ST_NO_ERROR)
        {
            STOS_TaskDelay(20);
            STBLAST_Print(("Command Send\n"));
            error = STBLAST_Abort(BLAST_RXD_HDL);
            STBLAST_Print(("Command Aborted\n"));
            if (error == ST_NO_ERROR)
            {
                STOS_SemaphoreWait(BlasterSemaphore_p);
                if (LastError == STBLAST_ERROR_ABORT)
                {
                    BLAST_TestPassed(Result);
                    BLAST_DebugPrintf(("Aborted - received %i commands\n", NumberDone));
                }
                else
                {
                    sprintf(ErrMsg, "Got %s, instead of STBLAST_ERROR_ABORT\n",
                    BLAST_ErrorString(LastError));
                    BLAST_TestFailed(Result, ErrMsg);
                }
            }
            else
            {
                sprintf(ErrMsg, "Failed to abort reception - %s\n", BLAST_ErrorString(error));
                BLAST_TestFailed(Result, ErrMsg);
                STOS_SemaphoreWait(BlasterSemaphore_p);
            }
        }
        else
        {
            sprintf(ErrMsg, "Failed to receive commands - %s\n", BLAST_ErrorString(error));
            BLAST_TestFailed(Result, ErrMsg);
        }
    }
    else
    {
         sprintf(ErrMsg, "Failed to Set Protocol - %s\n", BLAST_ErrorString(error));
         BLAST_TestFailed(Result, ErrMsg);
    }

    return Result;
}


BLAST_TestResult_t TestRxCapture(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolCount=0;
    U32  i,TotalSymbol=0;

#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    for (;;)
    {
        STBLAST_ReadRaw(BLAST_RXD_HDL,
                     SymbolBuf,
                     SYMBOL_BUFFER_SIZE,
                     &SymbolCount,
                     5000,             /* Max symbol time = 5ms */
                     5000              /* Wait forever */
                    );

        /* Await result completion */
        BLAST_DebugMessage("Awaiting symbols...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetSymbolRead(BLAST_RXD_HDL, &SymbolCount);
        if (Error != ST_NO_ERROR)
            continue;
        Error = STBLAST_CopySymbolstoBuffer(BLAST_RXD_HDL, SymbolBuf, SymbolCount, &NumCopied);
        if (Error != ST_NO_ERROR)
            continue;
#endif
        /* Check error-code */
        BLAST_DebugError("STBLAST_Read()", LastError);
        BLAST_DebugPrintf(("Received %i symbols: ", SymbolCount));

        if (SymbolCount > 0)
        {
            for (i = 0; i < SymbolCount; i++)
            {
                STBLAST_Print(("{ %d,%d }",
                             SymbolBuf[i].SymbolPeriod,
                             SymbolBuf[i].MarkPeriod));
            }
            TotalSymbol += SymbolCount;
            if (TotalSymbol > 200)
            {
                BLAST_TestPassed(Result);
                break;
            }

            STBLAST_Print(("\n"));
        }
        STOS_TaskDelay(ST_GetClocksPerSecond()/1000); /* 1msec delay */
    }

    return Result;
}


BLAST_TestResult_t TestRC6ABlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[6];
    U32 Counter = 0;
    U32 NumSent;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Transmit RC6A codes...");

    Protocol = STBLAST_PROTOCOL_RC6A;
    ProtocolParams.RC6A.CustomerCode = 0x00000000;
    ProtocolParams.RC6A.BufferElementsPerPayload = 1; /* not used yet */
    ProtocolParams.RC6A.NumberPayloadBits = 12;
    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);

    Cmd[0] = RcCodeValue("1", RC6ARcCode);
    Cmd[1] = RcCodeValue("2", RC6ARcCode);
    Cmd[2] = RcCodeValue("3", RC6ARcCode);
    Cmd[3] = RcCodeValue("4", RC6ARcCode);
    Cmd[4] = RcCodeValue("5", RC6ARcCode);
    Cmd[5] = RcCodeValue("6", RC6ARcCode);


    for (Counter = 0; Counter < 6; Counter++)
    {
        /*STTBX_Print(("irtest.c->Calling STBLAT_Send =%x\n", Cmd[Counter]));*/
        STBLAST_Send(BLAST_TXD_HDL,
                       &Cmd[Counter],
                       1,
                       &NumSent,
                       0);

        /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);
        printf ("RC6A Command sent\n");

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            break;
        }
        STOS_TaskDelay(ST_GetClocksPerSecond()*5);
    }

    if (Counter == 6)
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

/*Encodes the Raw symbols and then Transmit it*/
BLAST_TestResult_t TestRC6ABlastRaw(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolsEncoded,j;
    U32 TxCmd;
    U32 NumSent;
    U32 Counter=0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    UNUSED_PARAMETER(Params);      	
    BLAST_DebugMessage("Transmiting RC6A-WRITE RAW codes...");

    Protocol = STBLAST_PROTOCOL_RC6A;
    ProtocolParams.RC6A.CustomerCode		 	 = 0x00000000;
    ProtocolParams.RC6A.BufferElementsPerPayload = 1; /* not used yet */
    ProtocolParams.RC6A.NumberPayloadBits 		 = 12;
    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);
	
    for(Counter = 1; Counter < 6; Counter++ ) 
	{	
	          TxCmd = Counter;
 		      BLAST_DoTestEncode(&TxCmd,
	          	            	  1,
		                          SymbolBuf,
	            		          SYMBOL_BUFFER_SIZE,
	                       		  &SymbolsEncoded,
		                          Protocol,
	            		          &ProtocolParams);

	  		  for (j = 0; j < SymbolsEncoded; j++)
	          {
	            	SymbolBuf[j].SymbolPeriod *= 28;
	            	SymbolBuf[j].MarkPeriod   *= 28;
	          }
	  	      for (j = 0; j < SymbolsEncoded; j++)
	  		  {
	                U32 SymbolPeriod = 0;
			      	U32 Delay = 0;

	               /* Add on the protocol delay to the last symbol
	                  that is transmitted as part of the command*/
	                  
	                SymbolPeriod = SymbolBuf[SymbolsEncoded - 1].SymbolPeriod + Delay;
	                SymbolPeriod = min(65535, SymbolPeriod);
                    SymbolBuf[SymbolsEncoded - 1].SymbolPeriod = SymbolPeriod;
	   		  }
	   		  
	  		  STBLAST_WriteRaw(BLAST_TXD_HDL,
	        				   36000,
	        				   50,
				               SymbolBuf,
	                     	   SymbolsEncoded,
				               &NumSent,
				               0);
			 /* Await result completion */
			 STOS_SemaphoreWait(BlasterSemaphore_p);
		 
		     /* Check event code and error code */
	      	if ( LastError != ST_NO_ERROR ||
	             LastEvent != STBLAST_WRITE_DONE_EVT )
	        {
	            break;
	        }
	        printf("RC6A-RAW Symbol Sent=%x\n",Counter);
			STOS_TaskDelay(ST_GetClocksPerSecond()* 2);
	} 
    if (Counter == 5)
    {
        BLAST_TestPassed(Result);
    }
    return Result;
}

BLAST_TestResult_t TestSonyBlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd;
    U16 Counter = 0;
    U32 NumSent;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    UNUSED_PARAMETER(Params);   
    BLAST_DebugMessage("Check for mute toggling...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_PULSE;
    ProtocolParams.UserDefined.SubCarrierFreq = 40000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1800;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 12;

    /* Setup the PULSE coded protocol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 2400;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3000;

    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);

    Cmd = RcCodeValue("MUTE", SonyRcCode);

    for (Counter = 0; Counter < 2; Counter++)
    {
        STBLAST_Send(BLAST_TXD_HDL,
                       &Cmd,
                       1,
                       &NumSent,
                       0);

        /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            break;
        }

        STOS_TaskDelay(ST_GetClocksPerSecond()/2);

    }

    if (Counter == 2)
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

BLAST_TestResult_t TestSonyBlastBatch(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[10];
    U32 NumSent;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#if defined(ST_OSLINUX)
    U32 NumCmdSent=0;
#endif
    UNUSED_PARAMETER(Params);   
    BLAST_DebugMessage("Change channels 1-2-3-4-5...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_PULSE;
    ProtocolParams.UserDefined.SubCarrierFreq = 40000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1800;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 12;
    ProtocolParams.UserDefined.Delay = 1000000;     /* 1s delay */

    /* Setup the PULSE coded protocol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 2400;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3000;

    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);

    Cmd[0] = RcCodeValue("1", SonyRcCode);
    Cmd[1] = RcCodeValue("1", SonyRcCode);
    Cmd[2] = RcCodeValue("2", SonyRcCode);
    Cmd[3] = RcCodeValue("2", SonyRcCode);
    Cmd[4] = RcCodeValue("3", SonyRcCode);
    Cmd[5] = RcCodeValue("3", SonyRcCode);
    Cmd[6] = RcCodeValue("4", SonyRcCode);
    Cmd[7] = RcCodeValue("4", SonyRcCode);
    Cmd[8] = RcCodeValue("5", SonyRcCode);
    Cmd[9] = RcCodeValue("5", SonyRcCode);

    STBLAST_Send(BLAST_TXD_HDL,
                   Cmd,
                   10,
                   &NumSent,
                   0);

    /* Await result completion */
    STOS_SemaphoreWait(BlasterSemaphore_p);
#if defined(ST_OSLINUX)
    STBLAST_GetCmdSent(BLAST_TXD_HDL, &NumCmdSent);
    NumSent = NumCmdSent;
#endif

    if (NumSent != 10 || LastError != ST_NO_ERROR)
    {
        sprintf(ErrMsg, "Only sent %i commands - %s",
                        NumSent,
                        BLAST_ErrorString(LastError));
        BLAST_TestFailed(Result, ErrMsg);
    }
    else if ( LastEvent != STBLAST_WRITE_DONE_EVT )
    {
        sprintf(ErrMsg, "Got unexpected event code 0x%x", LastEvent);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

BLAST_TestResult_t TestRC6ADecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd=0, Counter;
    U32 NumReceived=0, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;

#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);   
    BLAST_DebugMessage("Display received RC6A codes...");
    Protocol = STBLAST_PROTOCOL_RC6A;
    ProtocolParams.RC6A.CustomerCode = 0x00000000;
    ProtocolParams.RC6A.BufferElementsPerPayload = 1; /* not used yet */
    ProtocolParams.RC6A.NumberPayloadBits = 12;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Display received command codes...");
    for (Counter = 0; Counter < 10; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Show command */
        /*STTBX_Print(("NumReceived = %d\n", NumReceived));*/
        BLAST_DebugPrintf(("CMD = %s\n", RcCodeString(Cmd, RC6ARcCode)));
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}
BLAST_TestResult_t TestSonyDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    ST_ErrorCode_t Error;
#ifdef ST_OSLINUX
    U32 NumCopied;
#endif    
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received Sony RC codes...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_PULSE;
    ProtocolParams.UserDefined.SubCarrierFreq = 40000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1800;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 12;

    /* Setup the PULSE coded protocol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod =  2500;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3000;

    Error = STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);
    if ( Error != ST_NO_ERROR )
    {
        printf("SetProtocol failed with error %d\n", Error);
    }

    BLAST_DebugMessage("Press five buttons...");
    for (Counter = 0; Counter < 5; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
        
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif        

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s\n", RcCodeString(Cmd, SonyRcCode)));
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}

BLAST_TestResult_t TestSonyDecodeBatch(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[5];
    U16 Counter, loop;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    U32 NumCopied;
    ST_ErrorCode_t Error;
#endif        
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received Sony RC codes...");

    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_PULSE;
    ProtocolParams.UserDefined.SubCarrierFreq = 40000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 1200;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1800;
    ProtocolParams.UserDefined.NumberPayloadBits = 12;

    /* Setup the PULSE coded protocol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod =  2500;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3000;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    for (Counter = 0; Counter < 2; Counter++)
    {
        /* Sent MUTE */
        STBLAST_Receive( BLAST_RXD_HDL,
                         Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting 5 RC codes...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
        
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif          

        if (NumReceived != 5)
        {
            sprintf(ErrMsg, "Didn't receive 5 commands - only got %i\n",
                NumReceived);
            BLAST_TestFailed(Result, ErrMsg);
            Failed++;
        }

        /* Check event code and error code */
        if (LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        for (loop = 0; loop < NumReceived; loop++)
        {
            BLAST_DebugPrintf(("CMD = %s\n",
                RcCodeString(Cmd[loop], SonyRcCode)));
        }
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed (at least once)\n");
    }

    return Result;
}


BLAST_TestResult_t TestForceBlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[10];
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    U32 NumSent;
#if defined(ST_OSLINUX)
    U32 NumCmdSent=0;
#endif
    UNUSED_PARAMETER(Params);  
    BLAST_DebugMessage("Check for mute toggling...");

    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SPACE;
    ProtocolParams.UserDefined.SubCarrierFreq = 38000;
    ProtocolParams.UserDefined.NumberStartSymbols = 10;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 500;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 2000;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 500;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1000;
    ProtocolParams.UserDefined.NumberPayloadBits = 8;
    ProtocolParams.UserDefined.BufferElementsPerPayload=1;

    /* Setup the SPACE coded protocol */
    /* Start symbol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 8000;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 12000;

    /* Address symbols */
    ProtocolParams.UserDefined.StartSymbols[1].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[1].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[2].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[2].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[3].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[3].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[4].MarkPeriod = 500;   /* 1*/
    ProtocolParams.UserDefined.StartSymbols[4].SymbolPeriod = 2000;
    ProtocolParams.UserDefined.StartSymbols[5].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[5].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[6].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[6].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[7].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[7].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[8].MarkPeriod = 500;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[8].SymbolPeriod = 1000;

    /* Padding symbol */
    ProtocolParams.UserDefined.StartSymbols[9].MarkPeriod = 500;
    ProtocolParams.UserDefined.StartSymbols[9].SymbolPeriod = 4500;

    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);


    Cmd[0] = RcCodeValue("MUTE", ForceRcCode);
    Cmd[1] = RcCodeValue("1", ForceRcCode);
    Cmd[2] = RcCodeValue("2", ForceRcCode);
    Cmd[3] = RcCodeValue("2", ForceRcCode);
    Cmd[4] = RcCodeValue("3", ForceRcCode);
    Cmd[5] = RcCodeValue("3", ForceRcCode);
    Cmd[6] = RcCodeValue("4", ForceRcCode);
    Cmd[7] = RcCodeValue("4", ForceRcCode);
    Cmd[8] = RcCodeValue("5", ForceRcCode);
    Cmd[9] = RcCodeValue("5", ForceRcCode);

    STBLAST_Send(BLAST_TXD_HDL,
                   Cmd,
                   10,
                   &NumSent,
                   0);
    printf("Waiting for semaphore \n");

    /* Await result completion */
    STOS_SemaphoreWait(BlasterSemaphore_p);
    printf(" Semaphore receive \n");

#if defined(ST_OSLINUX)
    STBLAST_GetCmdSent(BLAST_TXD_HDL, &NumCmdSent);
    NumSent = NumCmdSent;
#endif

    if (NumSent != 10 || LastError != ST_NO_ERROR)
    {
        sprintf(ErrMsg, "Only sent %i commands - %s",
                        NumSent,
                        BLAST_ErrorString(LastError));
        BLAST_TestFailed(Result, ErrMsg);
    }
    else if ( LastEvent != STBLAST_WRITE_DONE_EVT )
    {
        sprintf(ErrMsg, "Got unexpected event code 0x%x", LastEvent);
        BLAST_TestFailed(Result, ErrMsg);
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

BLAST_TestResult_t TestForceDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;

#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);   
    BLAST_DebugMessage("Display received Force RC codes...");

    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SPACE;

    ProtocolParams.UserDefined.SubCarrierFreq = 38000;
    ProtocolParams.UserDefined.NumberStartSymbols = 10;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 2000;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 600;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 1000;
    ProtocolParams.UserDefined.NumberPayloadBits = 8;
    ProtocolParams.UserDefined.BufferElementsPerPayload=1;

    /* Setup the SPACE coded protocol */
    /* Start symbol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 8000;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 12000;

    /* Address symbols */
    ProtocolParams.UserDefined.StartSymbols[1].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[1].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[2].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[2].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[3].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[3].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[4].MarkPeriod = 600;   /* 1*/
    ProtocolParams.UserDefined.StartSymbols[4].SymbolPeriod = 2000;
    ProtocolParams.UserDefined.StartSymbols[5].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[5].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[6].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[6].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[7].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[7].SymbolPeriod = 1000;
    ProtocolParams.UserDefined.StartSymbols[8].MarkPeriod = 600;   /* 0*/
    ProtocolParams.UserDefined.StartSymbols[8].SymbolPeriod = 1000;

    /* Padding symbol */
    ProtocolParams.UserDefined.StartSymbols[9].MarkPeriod = 600;
    ProtocolParams.UserDefined.StartSymbols[9].SymbolPeriod = 4500;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press ten buttons...");
    for (Counter = 0; Counter < 10; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

        /* Check event code and error code */
        if ( (LastError != ST_NO_ERROR) || (LastEvent != STBLAST_READ_DONE_EVT) )
        {
            Failed++;
            continue;
        }

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, &Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s\n", RcCodeString(Cmd, ForceRcCode)));
    }


    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}

BLAST_TestResult_t TestNECDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd, Counter;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received NEC RC codes...");

    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SPACE;

    ProtocolParams.UserDefined.SubCarrierFreq     = 38000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols  = 0;
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    /* for 7100 the received value tolerence is diffrent
      depends on receiver */
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod   = 600;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 2240;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod    = 600;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod  = 1120;
#else
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod   = 560;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod = 2240;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod    = 560;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod  = 1120;
#endif
    ProtocolParams.UserDefined.NumberPayloadBits        = 32;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;

    /* Setup the SPACE coded protocol */
    /* Start symbol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod   = 9000;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 13500;

    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press ten buttons...");
    for (Counter = 0; Counter < 10; Counter++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL,&Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        /* Show command */
        BLAST_DebugPrintf(("CMD = %s    %X\n", RcCodeString(Cmd, NECRcCode),Cmd ));

    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}



BLAST_TestResult_t TestGrundigBlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[18];
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    U32 NumSent;
    int i;
#if defined(ST_OSLINUX)
    U32 NumCmdSent=0;
    ST_ErrorCode_t Error;
#endif
    UNUSED_PARAMETER(Params);  
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SHIFT;
    ProtocolParams.UserDefined.SubCarrierFreq     = 36000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols  = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod =528;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod =0;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 0;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 0;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 10;

    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod   = 528;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3167;

    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);

    Cmd[0] = RcCodeValue("EOT", GrundigRcCode);
    Cmd[1] = RcCodeValue("1",    GrundigRcCode);
    Cmd[2] = RcCodeValue("2",    GrundigRcCode);
    Cmd[3] = RcCodeValue("3",    GrundigRcCode);
    Cmd[4] = RcCodeValue("4",    GrundigRcCode);
    Cmd[5] = RcCodeValue("5",    GrundigRcCode);
    Cmd[6] = RcCodeValue("6",    GrundigRcCode);
    Cmd[7] = RcCodeValue("7",    GrundigRcCode);
    Cmd[8] = RcCodeValue("8",    GrundigRcCode);
    Cmd[9] = RcCodeValue("9",    GrundigRcCode);
    Cmd[10] = RcCodeValue("MUTE",    GrundigRcCode);
    Cmd[11] = RcCodeValue("0",    GrundigRcCode);
    Cmd[12] = RcCodeValue("EOT",    GrundigRcCode);
    Cmd[13] = RcCodeValue("P+",    GrundigRcCode);
    Cmd[14] = RcCodeValue("P-",    GrundigRcCode);
    Cmd[15] = RcCodeValue("VOL+",    GrundigRcCode);
    Cmd[16] = RcCodeValue("VOL-",    GrundigRcCode);
    Cmd[17] = RcCodeValue("STBY",    GrundigRcCode);

    for(i=0; i<18; i++)
    {
        STBLAST_Send(BLAST_TXD_HDL,
                   &(Cmd[i]),
                   1,
                   &NumSent,
                   0);
        /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);
        BLAST_DebugMessage("Command sent\n");

#if defined(ST_OSLINUX)
    Error = STBLAST_GetCmdSent(BLAST_TXD_HDL, &NumCmdSent);
    if(Error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_GetCmdSent\n", Error);
    }
    else
    {
        NumSent = NumCmdSent;
    }
#endif
        if (NumSent !=1 || LastError != ST_NO_ERROR)
        {
            sprintf(ErrMsg, "Only sent %i commands - %s",
                        NumSent,
                        BLAST_ErrorString(LastError));
            BLAST_TestFailed(Result, ErrMsg);
            break;
        }
        else if (LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            sprintf(ErrMsg, "Got unexpected event code 0x%x", LastEvent);
            BLAST_TestFailed(Result, ErrMsg);
            break;
        }
        else if (i == 17) /* update with BLAST_TestPassed() only after the last command */
        {
            BLAST_TestPassed(Result);
        }
        STOS_TaskDelay(5000);
    }
    return Result;
}

/* Transmits Low latency codes */
BLAST_TestResult_t TestLowLatencyBlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 SymbolsEncoded, i;
    U32 TxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    UNUSED_PARAMETER(Params);   
    Protocol = STBLAST_LOW_LATENCY_PROTOCOL;

    BLAST_DebugMessage("Transmit Lowlatency codes...");

    ProtocolParams.LowLatency.SystemCode = 0xC;
    ProtocolParams.LowLatency.NumberPayloadBits = 8;
    
    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);


    BLAST_DebugMessage("Test: Encode -> cycle");

    for (i = 1; i <= 10; i++)
    {
        /* Strip out start bit */
        TxCmd = i ;

#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

        STBLAST_Send(BLAST_TXD_HDL,
                       &TxCmd,
                       1,
                       &SymbolsEncoded,
                       0);

       /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);
        printf ("Command sent\n");

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            break;
        }
    }

    if (i == 11)
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

/* Transmits Low latency codes */
BLAST_TestResult_t TestLowLatencyProBlast(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 SymbolsEncoded, i;
    U32 TxCmd;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    UNUSED_PARAMETER(Params);   
    BLAST_DebugMessage("Transmit Lowlatency RF codes...");
    
    Protocol = STBLAST_LOW_LATENCY_PRO_PROTOCOL;

    ProtocolParams.RFLowLatencyPro.SystemCode = 0x1;
    ProtocolParams.RFLowLatencyPro.UserCode = 0x0;
    ProtocolParams.RFLowLatencyPro.LongAddress = 0x0;    
    ProtocolParams.RFLowLatencyPro.NumberPayloadBits = 8;
    
    STBLAST_SetProtocol(BLAST_TXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Test: Encode -> cycle");

    for (i = 1; i <= 10; i++)
    {
        /* Strip out start bit */
        TxCmd = i;
		BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#ifdef VERBOSE
        BLAST_DebugPrintf(("0x%04x\r", TxCmd));
#endif

        STBLAST_Send(BLAST_TXD_HDL,
                       &TxCmd,
                       1,
                       &SymbolsEncoded,
                       0);

       /* Await result completion */
        STOS_SemaphoreWait(BlasterSemaphore_p);
        printf ("Command sent\n");

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_WRITE_DONE_EVT )
        {
            break;
        }
        STOS_TaskDelay(5000);
    }

    if (i == 11)
    {
        BLAST_TestPassed(Result);
    }

    return Result;
}

BLAST_TestResult_t TestRuwidoDecodeRC(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[5] = {0};
    U32 NumReceived,Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    U8 i,j;

#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received RUWIDO key codes");
    Protocol = STBLAST_PROTOCOL_RUWIDO;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press 20 buttons...");
    for (i = 0; i < 20; i++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         Cmd,
                         1,
                         &NumReceived,
                         0);

        /* Await result completion */
        BLAST_DebugMessage("Awaiting RC code...");
        STOS_SemaphoreWait(BlasterSemaphore_p);

#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

     	BLAST_DebugPrintf(("10 bit CMD = %X\n",Cmd[0] ));

        /* Show command */
        for ( j = 1; j < NumReceived; j++ )
        {
            BLAST_DebugPrintf(("CMD = %s    %X\n", RcCodeString(Cmd[j], RuwidoRcCodeRC),Cmd[j] ));
        }
        STOS_TaskDelay(5000);
    }

    if (Failed < 4)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}

BLAST_TestResult_t TestRuwidoDecodeKB(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd[5] = {0};
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    U8 i,j,count = 0,key_release = 0;
    U32 LocalBuf[255] = {0};
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Display received RUWIDO key codes...");
    Protocol = STBLAST_PROTOCOL_RUWIDO;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif    
    BLAST_DebugMessage("Press 40 buttons...");

    for (i = 0; i < 40; i++)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         Cmd,
                         1,
                         &NumReceived,
                         0);

        /*BLAST_DebugMessage("Awaiting RC code...");*/
        STOS_SemaphoreWait(BlasterSemaphore_p);
#ifdef ST_OSLINUX
        Error = STBLAST_GetCmdReceived(BLAST_RXD_HDL, &NumReceived);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_GetCmdReceived\n", Error);
        Error = STBLAST_CopyCmdstoBuffer(BLAST_RXD_HDL, Cmd, NumReceived ,&NumCopied);
        if (Error != ST_NO_ERROR)
            BLAST_DebugError("STBLAST_CopyCmdstoBuffer\n", Error);
#endif       

        /* Check event code and error code */
        if ( LastError != ST_NO_ERROR ||
             LastEvent != STBLAST_READ_DONE_EVT )
        {
            Failed++;
            continue;
        }

        for ( j = 0; j < NumReceived; j++ )
        {
            LocalBuf[count++] = Cmd[j];
        }

        if ( NumReceived == 1 )
        {
            key_release++;
        }

        if ( key_release > 3 )
        {
            /* Show command */
            for ( j = 0; j < count; j++ )
            {
                BLAST_DebugPrintf(("CMD = %s    %X\n", RcCodeString(LocalBuf[j], RuwidoRcCodeKB),LocalBuf[j] ));
            }
            key_release = 0;
            count = 0;
        }

    }
    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}
BLAST_TestResult_t TestGrundigDecode(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    U32 Cmd = 0;
    U32 StoredCommand = 0;
    U32 NumReceived, Failed = 0;
    STBLAST_Protocol_t Protocol;
    STBLAST_ProtocolParams_t ProtocolParams;
    U8 i, CommandCount;
    BOOL FirstEOTRecd = FALSE;
    UNUSED_PARAMETER(Params);  
    BLAST_DebugMessage("Display received Grundig RC codes...");
    Protocol = STBLAST_PROTOCOL_USER_DEFINED;
    ProtocolParams.UserDefined.Coding = STBLAST_CODING_SHIFT;
    ProtocolParams.UserDefined.SubCarrierFreq = 36000;
    ProtocolParams.UserDefined.NumberStartSymbols = 1;
    ProtocolParams.UserDefined.NumberStopSymbols = 0;
    ProtocolParams.UserDefined.HighDataSymbol.MarkPeriod =528 ;
    ProtocolParams.UserDefined.HighDataSymbol.SymbolPeriod =0;
    ProtocolParams.UserDefined.LowDataSymbol.MarkPeriod = 0;
    ProtocolParams.UserDefined.LowDataSymbol.SymbolPeriod = 0;
    ProtocolParams.UserDefined.BufferElementsPerPayload = 1;
    ProtocolParams.UserDefined.NumberPayloadBits = 10;

    /* Setup the SHIFT coded protocol */
    /* Start symbol */
    ProtocolParams.UserDefined.StartSymbols[0].MarkPeriod = 528;
    ProtocolParams.UserDefined.StartSymbols[0].SymbolPeriod = 3167;
    STBLAST_SetProtocol(BLAST_RXD_HDL, Protocol, &ProtocolParams);

    BLAST_DebugMessage("Press buttons... to exit press Standby key");

#ifdef IR_INVERT
    /* Test AA: Directly calls DoDecode function.  These symbol values are INVERTED
        The test should only be run if IR_INVERT is set.  */
    {
        int b, j;
        STBLAST_Protocol_t ProtocolSet;
        STBLAST_Symbol_t ArrayofBuffers[4][10] =  /* these are written {SPACE, MARK} */
        {
            {   { 0,0 },
                { 3167,2622 },
                { 1082,510 },
                { 1557,1012 },
                { 1056,510 },
                { 1583,509 },
                { 1584,1039 },
                { 1083,510 },
                { 1556,482 },
                { 65535,489 } /* key1 */
            },
            {
                { 0,0 },
                { 3171,2651 },
                { 1057,537 },
                { 1585,1065 },
                { 1055,537 },
                { 1603,539 },
                { 1569,1049 },
                { 1055,537 },
                { 1607,511 },
                { 65535,518 } /* key 1*/
            },
            {
                { 0,0 },
                { 3141,2595 },
                { 1082,510 },
                { 1029,483 },
                { 1610,1039 },
                { 1556,483 },
                { 1585,1039 },
                { 1083,510 },
                { 1555,482 },
                { 65535,489 } /* key 3 */
            },
            {   { 0,0 },
                { 3141,2596 },
                { 1584,1039 },
                { 1083,510 },
                { 1028,482 },
                { 1610,510 },
                { 1558,1012 },
                { 1055,509 },
                { 1609,509 },
                { 65535,516 } /* key 0 */
            }
        };

        U32 SymbolsAvailable = 10;
        U32 SymbolsConsumed;

        for (b=0; b<4; b++)
        {
            /* swap mark and space values around -
            this is to make it easier to use output from the capture test*/
            for (j=0; j<SymbolsAvailable; j++)
            {
                U16 temp;
                temp = ArrayofBuffers[b][j].MarkPeriod;
                ArrayofBuffers[b][j].MarkPeriod = ArrayofBuffers[b][j].SymbolPeriod;
                ArrayofBuffers[b][j].SymbolPeriod = temp;
            }
#if 0
            BLAST_DoDecode(&StoredCommand,
                       1,
                       (ArrayofBuffers[b]),
                       SymbolsAvailable,
                       &NumReceived,
                       &SymbolsConsumed,
                       Protocol,
                       36000,
                       &ProtocolParams,
                       &ProtocolSet);
#endif
            printf("Command received: 0x%x\n", StoredCommand);
        }
    }
    printf("--- End of test ---\n");
#endif


    /* Test A: Outputs all received symbols until Standby key is pressed*/
    while(1)
    {
        STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);
        /* Printing is done here, as this is a less time-critical stage in the cycle
         and printing takes tens of milliseconds (over a DCU link).  It is important
         that the delay between the semaphore_wait command below and the subsequent
         call to STBLAST_Receive is kept as short as possible. */
        printf("Command received: %s\n", RcCodeString(StoredCommand, GrundigRcCode));

        STOS_SemaphoreWait(BlasterSemaphore_p);
        if (Cmd == RcCodeValue("STBY", GrundigRcCode))
        {
            printf("STBY key received, quitting loop\n");
            break;
        }
        else
        {
            StoredCommand = Cmd;
        }
    }
    STOS_TaskDelay(5000);

    /* Test B: Detects five RC handset keys pressed by the user */
    STBLAST_Print(("\nPress, hold and release ten remote control keys. The number of commands decoded between EOT characters will be displayed\n"));
    for (i=0; i<10; i++)
    {
        CommandCount = 0;
        while(1)
        {
            STBLAST_Receive( BLAST_RXD_HDL,
                         &Cmd,
                         1,
                         &NumReceived,
                         0);
            STOS_SemaphoreWait(BlasterSemaphore_p);

            /* code here should be minimal; the next call to STBLAST Receive
            should be initiated rapidly within (approx) 14ms */
            if (Cmd == 0x3FF)  /* EOT */
            {
                if (FirstEOTRecd == TRUE) /* this EOT is end of frame EOT */
                {
                    STBLAST_Print((" Frame decoded, %d key %s commands received.\n", CommandCount, RcCodeString(StoredCommand, GrundigRcCode) ));
                    FirstEOTRecd = !FirstEOTRecd;
                    break;
                }
                FirstEOTRecd = !FirstEOTRecd;
            }
            else
            {
                CommandCount++;
                StoredCommand = Cmd;
            }
        }
    }

    if (Failed == 0)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Test failed to receive/decode value");
    }

    return Result;
}


/* Check the driver responds appropriately to bad calls */
BLAST_TestResult_t TestAPIErrant(BLAST_TestParams_t *Params)
{
    ST_ErrorCode_t          error = ST_NO_ERROR, stored = ST_NO_ERROR;
    BLAST_TestResult_t      Result = TEST_RESULT_ZERO;
    U32                     DataBuffer[1024];
    U32                     NumberRead, NumberWritten;

    STBLAST_InitParams_t    InitParams;
    STBLAST_OpenParams_t    OpenParams;
    STBLAST_TermParams_t    TermParams;
    STBLAST_Handle_t        RxHandle, TxHandle;
    STBLAST_Symbol_t        SymbolBuffer[128];
    /* Terminate existing instances, so we can use its values for
     * testing safely.
     */
    UNUSED_PARAMETER(Params);    
    BLAST_DebugMessage("Terminating drivers...");
    TermParams.ForceTerminate = TRUE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    ErrorReport(&stored, error, ST_NO_ERROR);
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_IR], &TermParams);
    ErrorReport(&stored, error, ST_NO_ERROR);

    /* Set up good values... */
    InitParams.DeviceType = BLAST_TXD_TYPE;
    InitParams.DriverPartition = system_partition;
    InitParams.ClockSpeed = IRB_CLOCK;
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;
    /* Can legitimately be 0 */
    InitParams.BaseAddress = (U32 *)IRB_BASE_ADDRESS;
    InitParams.InterruptNumber = IRB_INTERRUPT;
    InitParams.InterruptLevel = IRB_INTERRUPT_LEVEL;

    InitParams.TxPin.BitMask = BLAST_TXD_BIT;
    InitParams.TxInvPin.BitMask = BLAST_TXDINV_BIT;
    strcpy(InitParams.EVTDeviceName, BLAST_EVT_DEV);

    strcpy(InitParams.TxInvPin.PortName, BLAST_TXD_PIO);
    strcpy(InitParams.TxPin.PortName, BLAST_TXD_PIO);

#if defined(ST_5518)
    InitParams.InputActiveLow = FALSE;
#else
    InitParams.InputActiveLow = TRUE;
#endif

#ifdef ST_OSLINUX
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;
    InitParams.CmdBufferSize = CMD_BUFFER_SIZE;
#endif
    /*** STBLAST_Init() ***/
    stored = ST_NO_ERROR;
    BLAST_DebugMessage("STBLAST_Init(), null device name");
    error = STBLAST_Init(NULL, &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Init(), bad device name");
    error = STBLAST_Init("ThisNameIsFarTooLongReally", &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Init(), bad device type");
    InitParams.DeviceType = STBLAST_DEVICE_IR_TRANSMITTER - 1;
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);
    InitParams.DeviceType = BLAST_TXD_TYPE;
#ifndef ST_OSLINUX
/* No partitioning in linux */
    BLAST_DebugMessage("STBLAST_Init(), bad partition");
    InitParams.DriverPartition = NULL;
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);
    InitParams.DriverPartition = system_partition;
#endif
    BLAST_DebugMessage("STBLAST_Init(), bad clock speed");
    InitParams.ClockSpeed = 0;
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);
    InitParams.ClockSpeed = IRB_CLOCK;

    BLAST_DebugMessage("STBLAST_Init(), bad symbol buffer size");
    InitParams.SymbolBufferSize = 0;
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);
    InitParams.SymbolBufferSize = SYMBOL_BUFFER_SIZE;

    /* Test evt/pio device names? */

    /* Valid init */
    BLAST_DebugMessage("STBLAST_Init(), valid init");
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_NO_ERROR);

    BLAST_DebugMessage("STBLAST_Init(), valid init (same again)");
    error = STBLAST_Init(BlasterDeviceName[BLAST_TX_IR], &InitParams);
    ErrorReport(&stored, error, ST_ERROR_ALREADY_INITIALIZED);

    InitParams.DeviceType = BLAST_RXD_TYPE;

    strcpy(InitParams.RxPin.PortName, BLAST_RXD_PIO);

    /* On espresso it needs to set PIO5 bit 1 in input mode */
#if defined(espresso)
    InitParams.RxPin.BitMask = BLAST_UHF_BIT;
#else
    InitParams.RxPin.BitMask = BLAST_RXD_BIT;
#endif

    BLAST_DebugMessage("STBLAST_Init(), valid init (rx)");
    error = STBLAST_Init(BlasterDeviceName[BLAST_RX_IR], &InitParams);
    ErrorReport(&stored, error, ST_NO_ERROR);

    if (stored == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Init tests failed");
    }

    /*** STBLAST_Open() ***/
    stored = ST_NO_ERROR;
    OpenParams.RxParams.GlitchWidth = 0;

    BLAST_DebugMessage("STBLAST_Open(), null device name");
    error = STBLAST_Open(NULL, &OpenParams, &TxHandle);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Open(), bad device name");
    error = STBLAST_Open("ThisNameIsAlsoFarTooLong", &OpenParams, &TxHandle);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Open(), uninit device");
    error = STBLAST_Open("NotInit", &OpenParams, &TxHandle);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Open(), valid open (tx)");
    error = STBLAST_Open(BlasterDeviceName[BLAST_TX_IR], &OpenParams, &TxHandle);
    ErrorReport(&stored, error, ST_NO_ERROR);

    BLAST_DebugMessage("STBLAST_Open(), valid open (rx)");
    error = STBLAST_Open(BlasterDeviceName[BLAST_RX_IR], &OpenParams, &RxHandle);
    ErrorReport(&stored, error, ST_NO_ERROR);

    if (stored == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Open tests failed");
    }

    /*** Test Get/Set functions ***/
    stored = ST_NO_ERROR;

    BLAST_DebugMessage("STBLAST_GetCapability(), null capability");
    error = STBLAST_GetCapability(BlasterDeviceName[BLAST_TX_IR], NULL);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    {
        STBLAST_ProtocolParams_t * ProtocolParams = NULL;
        STBLAST_Protocol_t * Protocol = NULL;

        BLAST_DebugMessage("STBLAST_GetProtocol(), null protocol");
        error = STBLAST_GetProtocol(TxHandle, NULL, ProtocolParams);
        ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

        BLAST_DebugMessage("STBLAST_GetProtocol(), null protocolparams");
        error = STBLAST_GetProtocol(TxHandle, Protocol, NULL);
        ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    }

    BLAST_DebugMessage("STBLAST_SetProtocol(), null protocol, null protocolparams");
    error = STBLAST_SetProtocol(TxHandle, 0, (STBLAST_ProtocolParams_t*)NULL);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    if (stored == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "get/set tests failed");
    }

    /*** Test Send/Receive ***/
    stored = ST_NO_ERROR;

    BLAST_DebugMessage("STBLAST_Receive(), null buffer");
    error = STBLAST_Receive(RxHandle, NULL, 1024, &NumberRead, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Receive(), number to read 0");
    error = STBLAST_Receive(RxHandle, DataBuffer, 0, &NumberRead, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Receive(), null number read");
    error = STBLAST_Receive(RxHandle, DataBuffer, 1024, NULL, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Send(), null buffer");
    error = STBLAST_Send(TxHandle, NULL, 1024, &NumberWritten, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Send(), number to write 0");
    error = STBLAST_Send(TxHandle, DataBuffer, 0, &NumberWritten, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Send(), null numberwritten");
    error = STBLAST_Send(TxHandle, DataBuffer, 1024, NULL, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    /* Attempt Send/Receive without a prior call to SetProtocol */
    BLAST_DebugMessage("STBLAST_Receive(), no prior call to STBLAST_SetProtocol");
    error = STBLAST_Receive(RxHandle, DataBuffer, 1024, &NumberRead, 10);
    ErrorReport(&stored, error, STBLAST_ERROR_UNDEFINED_PROTOCOL);
    BLAST_DebugMessage("STBLAST_Send(), no prior call to STBLAST_SetProtocol");
    error = STBLAST_Send(TxHandle, DataBuffer, 1024, &NumberWritten, 10);
    ErrorReport(&stored, error, STBLAST_ERROR_UNDEFINED_PROTOCOL);

    if (stored == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "rcsend/receive tests failed");
    }

    /*** STBLAST_Read() and STBLAST_Write() ***/
    stored = ST_NO_ERROR;

    BLAST_DebugMessage("STBLAST_Read(), null buffer");
    error = STBLAST_ReadRaw(RxHandle, NULL, 128, &NumberRead, 10, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Read(), zero symbolstoread");
    error = STBLAST_ReadRaw(RxHandle, SymbolBuffer, 0, &NumberRead, 10, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Read(), null symbolsread");
    error = STBLAST_ReadRaw(RxHandle, SymbolBuffer, 128, NULL, 10, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Read(), zero maxsymboltime");
    error = STBLAST_ReadRaw(RxHandle, SymbolBuffer, 128, &NumberRead, 0, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Write(), zero subcarrier");
    error = STBLAST_WriteRaw(TxHandle, 0, 50, SymbolBuffer, 128, &NumberWritten, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Write(), null buffer");
    error = STBLAST_WriteRaw(TxHandle, 36000, 50, NULL, 128, &NumberWritten, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Write(), zero symbolstowrite");
    error = STBLAST_WriteRaw(TxHandle, 36000, 50, SymbolBuffer, 0,
                          &NumberWritten, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    BLAST_DebugMessage("STBLAST_Write(), null symbolswritten");
    error = STBLAST_WriteRaw(TxHandle, 36000, 50, SymbolBuffer, 128, NULL, 10);
    ErrorReport(&stored, error, ST_ERROR_BAD_PARAMETER);

    if (stored == ST_NO_ERROR)
    {
        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "read/write tests failed");
    }

    /*** STBLAST_Close() and STBLAST_Term() ***/
    stored = ST_NO_ERROR;

    BLAST_DebugMessage("STBLAST_Close(), invalid TxHandle");
    error = STBLAST_Close(0);
    ErrorReport(&stored, error, ST_ERROR_INVALID_HANDLE);

    TermParams.ForceTerminate = FALSE;
    BLAST_DebugMessage("STBLAST_Term(), open TxHandle, unforced");
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    ErrorReport(&stored, error, ST_ERROR_OPEN_HANDLE);

    BLAST_DebugMessage("STBLAST_Term(), null device name");
    error = STBLAST_Term(NULL, &TermParams);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Term(), bad device name");
    error = STBLAST_Term("ThisIsYetAnotherLongDeviceName", &TermParams);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Close(), rxhandle, no errors");
    error = STBLAST_Close(RxHandle);
    ErrorReport(&stored, error, ST_NO_ERROR);

    BLAST_DebugMessage("STBLAST_Term(), open TxHandle, forced termination");
    TermParams.ForceTerminate = TRUE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    ErrorReport(&stored, error, ST_NO_ERROR);

    BLAST_DebugMessage("STBLAST_Term(), already terminated device");
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    ErrorReport(&stored, error, ST_ERROR_UNKNOWN_DEVICE);

    BLAST_DebugMessage("STBLAST_Term(), no open handle");
    TermParams.ForceTerminate = FALSE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_IR], &TermParams);
    ErrorReport(&stored, error, ST_NO_ERROR);

#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5188) && !defined(ST_5107)
    BLAST_DebugMessage("STBLAST_Term(), no open handle");
    TermParams.ForceTerminate = TRUE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_UHF], &TermParams);
    ErrorReport(&stored, error, ST_NO_ERROR);
#endif

    if (stored == ST_NO_ERROR)
    {

        BLAST_TestPassed(Result);
    }
    else
    {
        BLAST_TestFailed(Result, "Close/Term tests failed");
    }

    return Result;
}


typedef struct TestDutyCycle_s
{
    U8 Duty;
    ST_ErrorCode_t ExpectedError;
} TestDutyCycle_t;

BLAST_TestResult_t TestDutyCycle(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t LastFail = ST_NO_ERROR, Error;
    STBLAST_Capability_t Caps;
    TestDutyCycle_t TestData[] =
    {
        { 25, ST_NO_ERROR },
        { 33, ST_NO_ERROR },
        { 50, ST_NO_ERROR },
        { 66, ST_NO_ERROR },
        { 75, ST_NO_ERROR },
        { 0, ST_ERROR_BAD_PARAMETER },
        { 100, ST_ERROR_BAD_PARAMETER },
        { 200, ST_ERROR_BAD_PARAMETER },
        { (U8)-1, (U8)-1 }
    };
    int i;
    UNUSED_PARAMETER(Params);    

    /* Firstly check device's capabilities to ensure the duty cycle
     * really is programmable.
     */
    Error = STBLAST_GetCapability(BlasterDeviceName[BLAST_TX_IR], &Caps);
    BLAST_DebugError("STBLAST_GetCapability()", Error);

    BLAST_DebugMessage("Checking device capabilities...");
    if (!Caps.ProgrammableDutyCycle)
    {
        BLAST_DebugMessage("*** Device is not programmable! ***");
    }
    else
    {
        BLAST_DebugMessage("*** Device is programmable ***");

        for (i = 0; TestData[i].Duty != (U8)-1; i++)
        {
            BLAST_DebugPrintf(("Setting duty cycle to %d%%...\n", TestData[i].Duty));
            if (Error == ST_NO_ERROR)
            {
                int j;
                U32 NumberWritten;
                STBLAST_Symbol_t OneMs = { 1000, 500 };

                BLAST_DebugMessage("Now measure TX subcarrier with scope");

                for (j = 0; j < 2000; j++)
                {
                    STBLAST_WriteRaw(   BLAST_TXD_HDL,
                                        36000,
                                        TestData[i].Duty,
                                        &OneMs, 1,
                                        &NumberWritten, 10);
                    STOS_SemaphoreWait(BlasterSemaphore_p);
                }
            }
        }
    }

    /* Check for failure */
    if (LastFail != ST_NO_ERROR)
    {
        BLAST_TestFailed(Result, "Unexpected error");
    }
    else
    {
        BLAST_TestPassed(Result);
    }

    return  Result;
}

void ErrorReport(ST_ErrorCode_t *ErrorStore,
                 ST_ErrorCode_t ErrorGiven,
                 ST_ErrorCode_t ExpectedErr)
{
    BLAST_DebugPrintf(("Got %s", BLAST_ErrorString(ErrorGiven)));
    if (ErrorGiven != ExpectedErr)
    {
        STBLAST_Print((" =======> Mismatch, expected %s\n",
            BLAST_ErrorString(ExpectedErr)));
    }
    else
    {
        STBLAST_Print(("\n"));
    }

    /* if ErrorGiven does not match ExpectedErr, update
     * ErrorStore only if no previous error was noted
     */
    if( ErrorGiven != ExpectedErr )
    {
        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
        }
    }

}

/*****************************************************************************
Error routines
*****************************************************************************/

/* Error message look up table */
static BLAST_ErrorMessage_t BLAST_ErrorLUT[] =
{
    /* STBLAST */
    { STBLAST_ERROR_EVENT_REGISTER, "STBLAST_ERROR_EVENT_REGISTER" },
    { STBLAST_ERROR_EVENT_UNREGISTER, "STBLAST_ERROR_EVENT_UNREGISTER" },
    { STBLAST_ERROR_INVALID_DEVICE_TYPE, "STBLAST_ERROR_INVALID_DEVICE_TYPE" },
    { STBLAST_ERROR_PIO, "STBLAST_ERROR_PIO" },
    { STBLAST_ERROR_OVERRUN,"STBLAST_ERROR_OVERRUN" },
    { STBLAST_ERROR_ABORT, "STBLAST_ERROR_ABORT" },
    { STBLAST_ERROR_UNDEFINED_PROTOCOL, "STBLAST_ERROR_UNDEFINED_PROTOCOL" },

    /* STEVT */
    { STEVT_ERROR_INVALID_EVENT_ID, "STEVT_ERROR_INVALID_EVENT_ID" },
    { STEVT_ERROR_ALREADY_REGISTERED, "STEVT_ERROR_ALREADY_REGISTERED" },
    { STEVT_ERROR_NO_MORE_SPACE, "STEVT_ERROR_NO_MORE_SPACE" },
    { STEVT_ERROR_ALREADY_SUBSCRIBED, "STEVT_ERROR_ALREADY_SUBSCRIBED" },
    { STEVT_ERROR_INVALID_EVENT_NAME, "STEVT_ERROR_INVALID_EVENT_NAME" },
    { STEVT_ERROR_ALREADY_UNREGISTERED, "STEVT_ERROR_ALREADY_UNREGISTERED" },
    { STEVT_ERROR_MISSING_NOTIFY_CALLBACK, "STEVT_ERROR_MISSING_NOTIFY_CALLBACK" },

    /* STAPI */
    { ST_NO_ERROR, "ST_NO_ERROR" },
    { ST_ERROR_NO_MEMORY, "ST_ERROR_NO_MEMORY" },
    { ST_ERROR_INTERRUPT_INSTALL, "ST_ERROR_INTERRUPT_INSTALL" },
    { ST_ERROR_ALREADY_INITIALIZED, "ST_ERROR_DEVICE_INITIALIZED" },
    { ST_ERROR_UNKNOWN_DEVICE, "ST_ERROR_UNKNOWN_DEVICE" },
    { ST_ERROR_BAD_PARAMETER, "ST_ERROR_BAD_PARAMETER" },
    { ST_ERROR_OPEN_HANDLE, "ST_ERROR_OPEN_HANDLE" },
    { ST_ERROR_NO_FREE_HANDLES, "ST_ERROR_NO_FREE_HANDLES" },
    { ST_ERROR_INVALID_HANDLE, "ST_ERROR_INVALID_HANDLE" },
    { ST_ERROR_INTERRUPT_UNINSTALL, "ST_ERROR_INTERRUPT_UNINSTALL" },
    { ST_ERROR_TIMEOUT, "ST_ERROR_TIMEOUT" },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_DEVICE_BUSY, "ST_ERROR_DEVICE_BUSY" },
    { ST_ERROR_UNKNOWN, "ST_ERROR_UNKNOWN" } /* Terminator */
};

char *BLAST_ErrorString(ST_ErrorCode_t Error)
{
    BLAST_ErrorMessage_t *mp;

    mp = BLAST_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;
}

void DumpRegs(void)
{
#ifdef ST_OS20	
#if !defined(PROCESSOR_C1)
    #pragma ST_device(Regs)
#endif
#endif

    int i;
    volatile U32 *Regs = (volatile U32 *)0x2000A200;

    for (i = 0; i < 35; i++)
    {
        BLAST_DebugPrintf(("REGS(%02x) = 0x%04x\n", i*4, Regs[i]));
    }
}

char *RcCodeString(U32 Code, RCCode_t *Table_p)
{
    RCCode_t *Code_p;

    Code_p = Table_p;
    while (Code_p->Code != 0xFF) /* a strcmp with (Code_p->Name == UNKWNOWN) would be better */
    {
        if (Code_p->Code == Code)
            break;
        Code_p++;
    }

    return Code_p->KeyName;
}

U16 RcCodeValue(const char *Name, RCCode_t *Table_p)
{
    RCCode_t *Code_p;

    Code_p = Table_p;
    while (Code_p->Code != 0xFF)
    {
        if (strncmp(Name, Code_p->KeyName, 20) == 0)
            break;
        Code_p++;
    }

    return Code_p->Code;
}

U32 ShowProgress(U32 Now, U32 Total, U32 Last)
{
    U32 Div;

    Div = (Now * 100) / Total;
    if ((Div/10) != (Last/10))
    {
        STBLAST_Print(("%d%%  ", Div));
    }
    if (Div == 100)
    {
        STBLAST_Print(("\n"));
    }
    fflush(stdout);
    return Div;
}

/* Command Registration */

#ifdef TEST_AUTO
void InitCommands(void)
{
#if defined(AUTO_TESTING_ENABLE)
    STTST_RegisterCommand("AUTO", Auto_Result, "Auto Function, No Manual Intervention");
#endif
    STTST_RegisterCommand("APITEST",Auto_APIErrant, "Check stack and data usage");

}
#if defined(AUTO_TESTING_ENABLE)
BOOL Auto_Result(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO, Result;
    STBLAST_TermParams_t TermParams;
    ST_ErrorCode_t error;

#if !defined(ST_OSLINUX)
    BLAST_DebugMessage("*******************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RCMM 24 / 32..."));
    BLAST_DebugMessage("*******************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result = TestRcmm(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;


    BLAST_DebugMessage("*******************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the Low Latency protocol..."));
    BLAST_DebugMessage("*******************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result = TestLowLatency(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("*******************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RC6A M0 protocol..."));
    BLAST_DebugMessage("*******************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result = TestRC6AMode0(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

#ifndef IR_INVERT
    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of space protocol..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestSpace(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;
#endif

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RC5 protocol..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestRC5(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

#endif /* ST_OSLINUX */

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test send abort..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestAbortSend(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test receive abort..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestAbortReceive(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;


    /* Closed BLAST initilized instance to execute the  following test*/
    TermParams.ForceTerminate = TRUE;
    error = STBLAST_Term(BlasterDeviceName[BLAST_TX_IR], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() TX", error);
        return FALSE;
    }
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_IR], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() RX", error);
        return FALSE;
    }

#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5188) && !defined(ST_5107)
    error = STBLAST_Term(BlasterDeviceName[BLAST_RX_UHF], &TermParams);
    if (error != ST_NO_ERROR)
    {
        BLAST_DebugError("STBLAST_Term() RX", error);
        return FALSE;
    }
#endif/*(AUTO_TESTING_ENABLE)*/

#if !defined(ST_OSLINUX)
    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Check stack and data usage..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=BlastStackUsage(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;
#endif /* ST_OSLINUX */

    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    /*Initialize Blast Instance*/
    BLAST_Init();
    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
#endif

BOOL Auto_APIErrant(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;
    
    UNUSED_PARAMETER(pars_p); 
    UNUSED_PARAMETER(result_sym_p);        
    
    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "TestAPIErrant..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestAPIErrant(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    /* Initialize Blast again as it is closed in APITEST*/
    BLAST_Init();
    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
#endif

#ifdef TEST_MANUAL

void InitCommands(void)
{
    STTST_RegisterCommand("DUTY", Manual_DutyCycle, "Test transmitter duty cycle.");
    STTST_RegisterCommand("SONY", Manual_SonyEncodingAndDecoding, "Sony Funnctinality Test.");
    STTST_RegisterCommand("RC5", Manual_RC5Decode, "Test RC5 handset decode.");
    STTST_RegisterCommand("CAPTURE", Manual_RxCapture, "Symbol capture test.");
    STTST_RegisterCommand("UHFCAPTURE", Manual_UHFRxCapture, "Symbol capture test.");
    STTST_RegisterCommand("RCRF8", Manual_RCRF8Decode, "Test decode of RCRF8 Protocol.");	
    STTST_RegisterCommand("FORCE", Manual_ForceEncodingAndDecoding, "Force Blast and Decode.");
    STTST_RegisterCommand("SHIFT", Manual_GrundigDecode, "Test decode of shift protocol.");
    STTST_RegisterCommand("RUWIDO", Manual_RuwidoDecode, "Test decode of RUWIDO protocol.");
    STTST_RegisterCommand("NEC", Manual_NECDecode, "Test decode of NEC protocol.");
    STTST_RegisterCommand("RC6A_DECODE", Manual_RC6ADecode, "Test RC6A TV Decode.");
    STTST_RegisterCommand("RC6AM0_DECODE", Manual_RC6AM0Decode, "Test RC6A mode 0 TV Decode.");
    STTST_RegisterCommand("ECHO", Manual_EchoStarDecode, "Test Echo Star RC Decode");
    STTST_RegisterCommand("LowLatencyDecode",Manual_LowLatencyDecode,"Test Low Latency Decode. ");
    STTST_RegisterCommand("RCMM",Manual_RCMM_Decode,"Test RCMM Mode Decode. ");
    STTST_RegisterCommand("RMAP",Manual_RMapDecode,"Test RMAP Decode. ");
    STTST_RegisterCommand("RMAPDoubleBit",Manual_RMapDoubleBitDecode,"Test RMAP Double Bit Decode. ");
    STTST_RegisterCommand("RSTEP",Manual_RStepDecode,"Test RSTEP Decode. ");
    STTST_RegisterCommand("MULTI",Manual_MultiProtocol,"Test RC6AM0 and RCMM Mode Decode. ");
    STTST_RegisterCommand("TEST_ALL", Manual_TestAll, "All the protocol are tested.");
    STTST_RegisterCommand("LLP",Manual_LowLatencyProDecode,"Test Low Latency Pro UHF Decode. ");        
}

BOOL Manual_DutyCycle(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test transmitter duty cycle...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestDutyCycle(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BOOL Manual_LowLatencyDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test Low Latency handset decode...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestLowLatencyDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_LowLatencyProDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test Low Latency Pro UHF handset decode...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestLowLatencyProDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL Manual_SonyEncodingAndDecoding(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO, Result;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test SONY TV Encode..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestSonyBlast(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;
    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test SONY TV encoding (batch)..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestSonyBlastBatch(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test SONY TV Decode..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestSonyDecode(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test SONY TV decode (batch)..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    
#if !defined(ST_OS21) || !defined(ST_OSLINUX)   
    Result=TestSonyDecodeBatch(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;
#endif

    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

BOOL Manual_RC5Decode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test RC5 handset decode...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total = TestRC5Decode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_EchoStarDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test Echo* handset decode...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total = TestEchoStarRC(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BOOL Manual_RC6AM0Decode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RC6 Mode 0 protocol..."));
    BLAST_DebugMessage("*********************************************************************");

    TestParams.Ref = Section;
    Section++;
    Total = TestRC6_Mode_0(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RMapDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;
    TestParams.Ref = Section;

    Section++;
    Total = TestRMapDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RMapDoubleBitDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;
    TestParams.Ref = Section;

    Section++;
    Total = TestRMapDoubleBitDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RStepDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;
    TestParams.Ref = Section;

    Section++;
    Total = TestRStepDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_MultiProtocol(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test decode of the RC6M0 and RCMM protocol..."));
    BLAST_DebugMessage("*********************************************************************");

    TestParams.Ref = Section;
    Section++;
    Total = TestMultiProtocol(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RCMM_Decode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RCMM protocol..."));
    BLAST_DebugMessage("*********************************************************************");

    TestParams.Ref = Section;
    Section++;
    Total = TestRcmm_24_32_Mode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RCRF8Decode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode/decode of the RCRF8 protocol..."));
    BLAST_DebugMessage("*********************************************************************");

    TestParams.Ref = Section;
    Section++;
    Total = TestRCRF8Decode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");


    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BOOL Manual_RxCapture(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Symbol capture test...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestRxCapture(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_UHFRxCapture(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Symbol UHF capture test...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestUHFRxCapture(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BLAST_TestResult_t TestUHFRxCapture(BLAST_TestParams_t *Params)
{
    BLAST_TestResult_t Result = TEST_RESULT_ZERO;
    STBLAST_Symbol_t SymbolBuf[SYMBOL_BUFFER_SIZE];
    U32 SymbolCount = 0;
    U32 i,TotalSymbol=0;
    
#ifdef ST_OSLINUX
    ST_ErrorCode_t Error;
    U32 NumCopied;
#endif    
    UNUSED_PARAMETER(Params);    
    /* To change the Config control register for UHF Receive */
    DeviceConfig_UHF();

    for (;;)
    {
        STBLAST_ReadRaw(BLAST_UHF_HDL,
                     SymbolBuf,
                     SYMBOL_BUFFER_SIZE,
                     &SymbolCount,
                     5000,             /* Max symbol time = 5ms */
                     5000              /* Wait forever */
                    );

        /* Await result completion */
        BLAST_DebugMessage("Awaiting symbols...");
        STOS_SemaphoreWait(BlasterSemaphore_p);
        
#ifdef ST_OSLINUX
        Error = STBLAST_GetSymbolRead(BLAST_UHF_HDL, &SymbolCount);
        if (Error != ST_NO_ERROR)
            continue;
        Error = STBLAST_CopySymbolstoBuffer(BLAST_UHF_HDL, SymbolBuf, SymbolCount, &NumCopied);
        if (Error != ST_NO_ERROR)
            continue;
#endif        

        /* Check error-code */
        BLAST_DebugError("STBLAST_Read()", LastError);
        BLAST_DebugPrintf(("Received %i symbols: ", SymbolCount));
        if (SymbolCount > 0)
        {
            for (i = 0; i < SymbolCount; i++)
            {
                STBLAST_Print(("{ %d,%d }",
                                SymbolBuf[i].SymbolPeriod,
                                SymbolBuf[i].MarkPeriod));
            }
            TotalSymbol += SymbolCount;
            if (TotalSymbol > 150)
            {
                BLAST_TestPassed(Result);
                break;
            }

            STBLAST_Print(("\n"));
        }
    }
    /* To Restore the setting for IR reciver */
    DeviceConfig();
    return Result;
}

BOOL Manual_ForceEncodingAndDecoding(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO, Result;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test FORCE Board RC Encode..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;

    Result=TestForceBlast(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test FORCE Board RC Decode..."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Result=TestForceDecode(&TestParams);
    Total.NumberPassed += Result.NumberPassed;
    Total.NumberFailed += Result.NumberFailed;

    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_NECDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test decode of NEC protocol...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestNECDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_GrundigDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test decode of shift protocol...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestGrundigDecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RuwidoDecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test decode of RUWIDO protocol...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    BLAST_DebugMessage("Test remote control keys..");
    Total= TestRuwidoDecodeRC(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");
    TestParams.Ref = Section;
    Section++;
    BLAST_DebugMessage("Test keyboard hits..");
    Total= TestRuwidoDecodeKB(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Manual_RC6ADecode(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test RC6A TV Decode...."));
    BLAST_DebugMessage("*********************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestRC6ADecode(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL Manual_TestAll(parse_t * pars_p, char *result_sym_p)
{

    BOOL result=FALSE;

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("Duty Cycle test");
    BLAST_DebugMessage("*********************************************************************");

    result=result|Manual_DutyCycle(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("SONY test");
    BLAST_DebugMessage("*********************************************************************");

    result=result|Manual_SonyEncodingAndDecoding(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("FORCE Cycle test");
    BLAST_DebugMessage("*********************************************************************");

    result=result|Manual_ForceEncodingAndDecoding(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("SHIFT Protocol test");
    BLAST_DebugMessage("*********************************************************************");

    result=result|Manual_GrundigDecode(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage(" Capture test");
    BLAST_DebugMessage("*********************************************************************");

    result = result|Manual_RxCapture(pars_p,result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("RC6A Mode 6 test");
    BLAST_DebugMessage("*********************************************************************");

    result = result|Manual_RC6ADecode(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("RC6A Mode 0 test");
    BLAST_DebugMessage("*********************************************************************");

    result = result|Manual_RC6AM0Decode(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("Low Latency Protocol test");
    BLAST_DebugMessage("*********************************************************************");

    result = result|Manual_LowLatencyDecode(pars_p, result_sym_p);

    BLAST_DebugMessage("*********************************************************************");
    BLAST_DebugMessage("RC5 test");
    BLAST_DebugMessage("*********************************************************************");

    result = result|Manual_RC5Decode(pars_p,result_sym_p);

    return result;

}
#endif

#ifdef TEST_BLAST

void InitCommands(void)
{
    STTST_RegisterCommand("SHIFT_ENCODE", Blast_TestGrundig, "Test encode of shift protocol.");
    STTST_RegisterCommand("RC6A_ENCODE", Blast_RC6ABlast, "Test RC6A TV Encode.");
    STTST_RegisterCommand("RAWRC6", Blast_RC6ABlastRaw, "Test RC6A Raw TV Encode.");
    STTST_RegisterCommand("LLP", Blast_LowLatencyBlast, "Test Low Latency  Raw TV Encode.");  
    STTST_RegisterCommand("RFLLP", Blast_LowLatencyProBlast, "Test RF Low Latency  Raw TV Encode.");  
}

BOOL Blast_TestGrundig(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test encode of shift protocol."));
    BLAST_DebugMessage("*************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestGrundigBlast(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Blast_RC6ABlast(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test RC6A TV Encode."));
    BLAST_DebugMessage("*************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestRC6ABlast(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
BOOL Blast_LowLatencyBlast(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test Low Latency TV Encode."));
    BLAST_DebugMessage("*************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestLowLatencyBlast(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL Blast_LowLatencyProBlast(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test RF Low Latency TV Encode."));
    BLAST_DebugMessage("*************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestLowLatencyProBlast(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


BOOL Blast_RC6ABlastRaw(parse_t * pars_p, char *result_sym_p)
{
    U32 Section = 1;
    BLAST_TestParams_t TestParams;
    BLAST_TestResult_t Total = TEST_RESULT_ZERO;

    BLAST_DebugMessage("*************************************************************");
    BLAST_DebugPrintf(("SECTION %d - %s\n", Section, "Test RC6A-RAW TV Encode."));
    BLAST_DebugMessage("*************************************************************");
    TestParams.Ref = Section;
    Section++;
    Total=TestRC6ABlastRaw(&TestParams);
    BLAST_DebugMessage("**************************************************");
    BLAST_DebugPrintf(("PASSED: %2d\n", Total.NumberPassed));
    BLAST_DebugPrintf(("FAILED: %2d\n", Total.NumberFailed));
    BLAST_DebugMessage("**************************************************");

    if (Total.NumberFailed)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
#endif

/* used for encoding */
#define MID_1T   (U16)444.444
#define MID_2T   (U16)888.888
#define MID_3T   (U16)1333.332
#define MID_6T   (U16)2666.664

#define SubCarrierFreq (U16)36000

/* macros */
#define SYMBOL_TIME_MICROSECONDS(f)     ( ( 1000000 + ( (f) >> 1) ) / (f) )
#define MICROSECONDS_TO_SYMBOLS(f, m)   ( (m) / SYMBOL_TIME_MICROSECONDS(f) )
#define GenerateSymbol(mark, space, Buffer_p) \
        Buffer_p->MarkPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark);    \
        Buffer_p->SymbolPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark+space) 

#define GenerateMark(mark, Buffer_p) \
        (Buffer_p->MarkPeriod = MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark) )

#define GenerateSpace(space, Buffer_p) \
        (Buffer_p->SymbolPeriod = Buffer_p->MarkPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, space) )

#define ExtendMark(mark, Buffer_p) \
        (Buffer_p->MarkPeriod = Buffer_p->MarkPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, mark) )

#define ExtendSpace(space, Buffer_p) \
        (Buffer_p->SymbolPeriod = Buffer_p->SymbolPeriod + MICROSECONDS_TO_SYMBOLS(SubCarrierFreq, space) )
        
           
/******* Since these functions are not accessible.So defined locally ****************************************/

ST_ErrorCode_t BLAST_RC6ATestEncode(const U32                  *UserBuf_p,
                                    const U32                  UserBufSize,
                                    STBLAST_Symbol_t           *SymbolBuf_p,
                                    const U32                  SymbolBufSize,
                                    U32                        *SymbolsEncoded_p,
                                    const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    U32 SymbolCount = 0;
    int i;
    U32 Data = 0;
    BOOL LastUpdatedMark;

    *SymbolsEncoded_p = 0;             
    /* Reset symbol count */

    /**************************************************************
     * The format of an RC6A command is as follows:
     *
     * | H,S,M,T | C[8] | P[20]
     *
     * H,M.S,T - Header, Start, Mode, Trailer bits
     * C       - Customer code
     * P       - Payload
     */
    UNUSED_PARAMETER(UserBufSize);    
    UNUSED_PARAMETER(SymbolBufSize);    
    /* generate header+startbit, mode6, trailer */    
    GenerateSymbol( MID_6T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_1T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;
    GenerateSymbol( MID_1T, MID_2T, SymbolBuf_p);  SymbolBuf_p++;  SymbolCount++;

    /* generate the end of the trailer (2T) plus the first bit of the cust code */
    if((ProtocolParams_p->RC6A.CustomerCode & (1 << (7))) != 0)
    {
        /* Bit = 1 */
        GenerateMark( MID_3T, SymbolBuf_p);
        LastUpdatedMark = TRUE;
    }
    else
    {
        /* Bit = 0 */
        GenerateMark( MID_2T, SymbolBuf_p);
        LastUpdatedMark = TRUE;
    }
    
    /* compile the rest of the bits (typically 20) into a U32 Data*/    
    /* Customercode */
    Data = (U32)ProtocolParams_p->RC6A.CustomerCode;
    Data <<= ProtocolParams_p->RC6A.NumberPayloadBits;
    
    /* Payload - before ORing with Data, ensure only NumberPayloadBits of data remain in *UserBuf */
    Data |= ((*UserBuf_p << (32-ProtocolParams_p->RC6A.NumberPayloadBits)) >> (32-ProtocolParams_p->RC6A.NumberPayloadBits) );
    
    
    /* generate end of trailer and customer code */
    for(i = 8+ProtocolParams_p->RC6A.NumberPayloadBits-1; i > -1; i--)
    {
        if((Data & (1 << (i))) != 0)
        {
            /* Bit = 1 */
            if(LastUpdatedMark)
            {
                ExtendMark(MID_1T, SymbolBuf_p);
                GenerateSpace(MID_1T, SymbolBuf_p);
                LastUpdatedMark = FALSE;
            }
            else
            {
                SymbolBuf_p++;
                SymbolCount++;
                GenerateSymbol(MID_1T, MID_1T, SymbolBuf_p);
                LastUpdatedMark = FALSE;
            }
        }
        else
        {
             /* Bit = 0 */
            if(LastUpdatedMark)
            {
                GenerateSpace(MID_1T, SymbolBuf_p);
                SymbolBuf_p++;
                SymbolCount++;
                GenerateMark(MID_1T, SymbolBuf_p);
                LastUpdatedMark = TRUE;
            }
            else
            {
                ExtendSpace(MID_1T, SymbolBuf_p);
                SymbolBuf_p++;
                SymbolCount++;
                GenerateMark(MID_1T, SymbolBuf_p);
                LastUpdatedMark = TRUE;                
            }
        }
    }

    SymbolCount++;

    /* Set symbols encoded count */
    *SymbolsEncoded_p = SymbolCount;

    /* Can't really go wrong with this */
    return ST_NO_ERROR;
}
ST_ErrorCode_t BLAST_DoTestEncode(const U32 *UserBuf_p,
                              	  const U32  UserBufSize,
                              	  STBLAST_Symbol_t *SymbolBuf_p,
                              	  const U32  SymbolBufSize,
                              	  U32 *SymbolsEncoded_p,
                              	  const STBLAST_Protocol_t Protocol,
                              	  const STBLAST_ProtocolParams_t *ProtocolParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    *SymbolsEncoded_p = 0;

    /* Encode based on current protocol settings */
    switch(Protocol)
    {
       case STBLAST_PROTOCOL_RC6A:
            BLAST_RC6ATestEncode(UserBuf_p,
                              UserBufSize,
                              SymbolBuf_p,
                              SymbolBufSize,
                              SymbolsEncoded_p,
                              ProtocolParams_p);
            break;
      default:
            break;
   }
    /* Common exit */
    return error;
}
/* End of module */
