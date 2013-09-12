/* ----------------------------------------------------------------------------
File Name: tcdrv.c (was tdrv.c from sat)

Description: external tuner drivers.

Copyright (C) 1999-2006 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

/* C libs */
#ifndef ST_OSLINUX
   
#include <string.h>

#include <stdlib.h> /* for abs() */
#endif
#include "stlite.h"     /* Standard includes */


/* STAPI */
#include "sttbx.h"




/* STAPI common includes */
/* Standard includes */
#include "stddefs.h"

/* STAPI */

#include "stcommon.h"
#include "stevt.h"
#include "sttuner.h" 



/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* I/O for this driver */
#include "ioreg.h"      /* I/O for this driver */
#include "chip.h"
#include "cdrv.h"       /* utilities */

#include "cioctl.h"     /* data structure typedefs for all the the cable ioctl functions */

#include "tcdrv.h"      /* header for this file */
#ifndef ST_OSLINUX
#include <assert.h>
#endif

/* define ----------------------------------------------------------------- */


/*************MT2011**************/
/* for casting void* to a U32 */
#define I2C_HANDLE(x)      (*((STI2C_Handle_t *)x))




#define tuner_tdrv_DelayInMicroSec(micro_sec)  STOS_TaskDelayUs(micro_sec)
#define tuner_tdrv_DelayInMilliSec(milli_sec)  tuner_tdrv_DelayInMicroSec(milli_sec*1000)

/* MT2060 */

/* LNA Band cross-over frequencies */
#ifdef STTUNER_DRV_CAB_TUN_MT2060 
static const U32 MT2060_LNA_Bands[] =
{
     95000000,     /*   0 ..  95 MHz: b1011 */
    180000000,     /*  95 .. 180 MHz: b1010 */
    260000000,     /* 180 .. 260 MHz: b1001 */
    335000000,     /* 260 .. 335 MHz: b1000 */
    425000000,     /* 335 .. 425 MHz: b0111 */
    490000000,     /* 425 .. 490 MHz: b0110 */
    570000000,     /* 490 .. 570 MHz: b0101 */
    645000000,     /* 570 .. 645 MHz: b0100 */
    730000000,     /* 645 .. 730 MHz: b0011 */
    810000000      /* 730 .. 810 MHz: b0010 */
                   /* 810 ..     MHz: b0001 */
};
#endif
typedef struct
{
    S32     min;
    S32     max;
} MT_ExclZoneS_t;


/*  Implement ceiling, floor functions.  */
#define ceil(n, d) (((n) < 0) ? (n)/(d) : (n)/(d) + ((n)%(d) != 0))
#define floor(n, d) (((n) < 0) ? (n)/(d) - ((n)%(d) != 0) : (n)/(d))
/* End MT2060*/

#define TCDRV_WAITING_TIME              5       /* In ms (Tuner polling) */
#define TCDRV_CP_WAITING_TIME           10      /* In ms (Charge Pump) */
#define TCDRV_MT2040_WAITING_TIME       10      /* In ms (Init Tuner MT) */

#define TCDRV_IO_TIMEOUT                10

#define TCDRV_TUNER_PART_CODE_MT2030    0x06
#define TCDRV_TUNER_PART_CODE_MT2040    0x07
#define TCDRV_TUNER_PART_CODE_MT2050    0x04
#define TCDRV_TUNER_PART_CODE_MT2060    0x06



typedef enum MT2040_Offsets       /* MT203X and MT2040 */
{
    TCDRV_MT2040_LO1_1 = 0,     /* 0x00: LO1 Byte 1*/
    TCDRV_MT2040_LO1_2,         /* 0x01: LO1 Byte 2*/
    TCDRV_MT2040_LOGC,          /* 0x02: LO GC Cntl*/
    TCDRV_MT2040_RESERVED_1,    /* 0x03: Reserved*/
    TCDRV_MT2040_LOVCOC,        /* 0x04: *For 2040 only : LO VCO Cntl*/
    TCDRV_MT2040_LO2_1,         /* 0x05: LO2 Byte 1*/
    TCDRV_MT2040_CONTROL_1,     /* 0x06: Control Byte 1*/
    TCDRV_MT2040_CONTROL_2,     /* 0x07: Control Byte 2*/
    TCDRV_MT2040_RESERVED_3,    /* 0x08: Reserved*/
    TCDRV_MT2040_RESERVED_4,    /* 0x09: Reserved*/
    TCDRV_MT2040_RESERVED_5,    /* 0x0A: Reserved*/
    TCDRV_MT2040_LO2_2,         /* 0x0B: LO2 Byte 2*/
    TCDRV_MT2040_LO2_3,         /* 0x0C: LO2 Byte 3*/
    TCDRV_MT2040_MIX1,          /* 0x0D: MIX1 Cntl*/
    TCDRV_MT2040_STATUS,        /* 0x0E: Status*/
    TCDRV_MT2040_TUNEA2D,       /* 0x0F: Tune A/D*/
    TCDRV_MT2040_RESERVED_6,    /* 0x10: Reserved*/
    TCDRV_MT2040_COCODE_1,      /* 0x11: Company Code Byte 1*/
    TCDRV_MT2040_COCODE_2,      /* 0x12: Company Code Byte 2*/
    TCDRV_MT2040_PARTCODE,      /* 0x13: Part Code*/
    TCDRV_MT2040_REVCODE,       /* 0x14: Revision Code*/
    TCDRV_MT2040_NB_REGS
} MT2040_Offsets_t;

typedef enum MT2050_Offsets
{
    TCDRV_MT2050_PART_REV = 0,   /*  0x00: Part/Rev Code        */
    TCDRV_MT2050_LO1C_1,         /*  0x01: LO1C Byte 1          */
    TCDRV_MT2050_LO1C_2,         /*  0x02: LO1C Byte 2          */
    TCDRV_MT2050_LO2C_1,         /*  0x03: LO2C Byte 1          */
    TCDRV_MT2050_LO2C_2,         /*  0x04: LO2C Byte 2          */
    TCDRV_MT2050_LO2C_3,         /*  0x05: LO2C Byte 3          */
    TCDRV_MT2050_PWR,            /*  0x06: PWR Byte             */
    TCDRV_MT2050_LO_STATUS,      /*  0x07: LO Status Byte       */
    TCDRV_MT2050_RSVD_08,        /*  0x08: Reserved             */
    TCDRV_MT2050_MISC_STATUS,    /*  0x09: Misc Status Byte     */
    TCDRV_MT2050_RSVD_0A,        /*  0x0A: Reserved             */
    TCDRV_MT2050_RSVD_0B,        /*  0x0B: Reserved             */
    TCDRV_MT2050_RSVD_0C,        /*  0x0C: Reserved             */
    TCDRV_MT2050_SRO,            /*  0x0D: SRO Byte             */
    TCDRV_MT2050_RSVD_0E,        /*  0x0E: Reserved             */
    TCDRV_MT2050_MIX_1,          /*  0x0F: Mix 1 Byte           */
    TCDRV_MT2050_PK_DET,         /*  0x10: Peak Detect Byte     */
    TCDRV_MT2050_NB_REGS
} MT2050_Offsets_t;


typedef enum MT2060_Offsets
{
    TCDRV_MT2060_PART_REV = 0,   /*  0x00: Part/Rev Code        */
    TCDRV_MT2060_LO1C_1,         /*  0x01: LO1C Byte 1          */
    TCDRV_MT2060_LO1C_2,         /*  0x02: LO1C Byte 2          */
    TCDRV_MT2060_LO2C_1,         /*  0x03: LO2C Byte 1          */
    TCDRV_MT2060_LO2C_2,         /*  0x04: LO2C Byte 2          */
    TCDRV_MT2060_LO2C_3,         /*  0x05: LO2C Byte 3          */
    TCDRV_MT2060_LO_STATUS,      /*  0x06: LO Status Byte       */
    TCDRV_MT2060_FM_FREQ,        /*  0x07: FM FREQ Byte         */
    TCDRV_MT2060_MISC_STATUS,    /*  0x08: Misc Status Byte     */
    TCDRV_MT2060_MISC_CTRL_1,    /*  0x09: Misc Ctrl Byte 1     */
    TCDRV_MT2060_MISC_CTRL_2,    /*  0x0A: Misc Ctrl Byte 2     */
    TCDRV_MT2060_MISC_CTRL_3,    /*  0x0B: Misc Ctrl Byte 3     */
    TCDRV_MT2060_RSVD_0C,        /*  0x0C: Reserved             */
    TCDRV_MT2060_RSVD_0D,        /*  0x0D: Reserved             */
    TCDRV_MT2060_RSVD_0E,        /*  0x0E: Reserved             */
    TCDRV_MT2060_RSVD_0F,        /*  0x0F: Reserved             */
    TCDRV_MT2060_RSVD_10,        /*  0x10: Reserved             */
    TCDRV_MT2060_RSVD_11,        /*  0x11: Reserved             */
    TCDRV_MT2060_NB_REGS
} MT2060_Offsets_t;



#ifdef STTUNER_DRV_CAB_TUN_MT2011
 typedef enum       /** MicroTune DOCSIS tuner MT2011*/
    { 
/*
 Register      | Address
 --------------|--------*/
  rMT_Part       = 0x00,
  rMT_LO1C_Byte1 = 0x01,
  rMT_LO1C_Byte2 = 0x02,
  rMT_LO1C_Byte3 = 0x03,
  rMT_LO2C_Byte1 = 0x04,
  rMT_LO2C_Byte2 = 0x05,
  rMT_LO2C_Byte3 = 0x06,
  rMT_PWR        = 0x07,
  rMT_AFC        = 0x08,
  rMT_LO_Stat    = 0x09,
  rMT_Ctrl_Byte1 = 0x0a,
  rMT_Ctrl_Byte2 = 0x0b,
  rMT_Ctrl_Byte3 = 0x0c,
  rMT_Ctrl_Byte4 = 0x0d,
  rMT_Ctrl_Byte5 = 0x0e,
  rMT_Ctrl_Byte6 = 0x0f,
  rMT_Ctrl_Byte7 = 0x10,
  rMT_CP_Ctrl    = 0x11,
  rMT_VCO_Ctrl   = 0x12,
  rMT_LO1_Byte1  = 0x13,
  rMT_LO1_Byte2  = 0x14,
  rMT_LO2_Byte1  = 0x15,
  rMT_LO2_Byte2  = 0x16,
  rMT_LO1_Byte3  = 0x17,
  rMT_LO2_Byte3  = 0x18,
  rMT_Test_Byte1 = 0x19,
  rMT_Test_Byte2 = 0x1a,
  rMT_Test_Byte3 = 0x1b
  } eRxTuner_Reg;


/************MT2011***************/

#define kINIT_REGS               26
#define kFif1                    1221000000        /* 1217.75 MHz*/

#define kFif2                    43750000          /* 44.00 MHz */

#define kFifbw                   6000000           /* 6.5 MHz*/
#define kFref                    14318000          /* 14.3 MHz*/
#define kFlo_Step                500000            /* 500 kHz*/
#define kFstep                   50000             /* 50 kHz*/
#define kMAX_STEPS               17
#define kMAX_HARMONICS           15
#define kMT_LO1LK                0x80              /* bit7*/
#define kMT_LO2LK                0x08              /* bit3*/
#define kMT_LOCKED               (kMT_LO1LK | kMT_LO2LK)

tRxtuner_Status gRxTuner; /*global*/

typedef struct /**register init structs*/
{
   eRxTuner_Reg   reg;
   U8            value;
} tRxTuner_RegInit;
 
typedef struct fractThreshStr /**fractional Thresholds*/
{
   int   ft_bottom;
   int   ft_top;
   int   ft_newnum;
   int   ft_divincr;
} fractThreshStr;
  
/***********************************
*        globals                   *  
***********************************/   
static const tRxTuner_RegInit  gRegInit[kINIT_REGS] =
{
   {rMT_LO1C_Byte1, 0x19},
   {rMT_LO1C_Byte2, 0x14},
   {rMT_LO1C_Byte3, 0x58},
   {rMT_LO2C_Byte1, 0x09},
   {rMT_LO2C_Byte2, 0xd6},
   {rMT_LO2C_Byte3, 0x4f},
   {rMT_PWR       , 0x70},
   {rMT_AFC       , 0x08},
   {rMT_Ctrl_Byte1, 0x00},
   {rMT_Ctrl_Byte2, 0x74},
   {rMT_Ctrl_Byte3, 0xb7},
   {rMT_Ctrl_Byte4, 0x99},
   {rMT_Ctrl_Byte5, 0x90},
   {rMT_Ctrl_Byte6, 0x1f}, /* 0x1c*/
   {rMT_Ctrl_Byte7, 0x44},
   {rMT_CP_Ctrl   , 0xc0},
   {rMT_VCO_Ctrl  , 0xcc},
   {rMT_LO1_Byte1 , 0x00},
   {rMT_LO1_Byte2 , 0xff},
   {rMT_LO2_Byte1 , 0x00},
   {rMT_LO2_Byte2 , 0xff},
   {rMT_LO1_Byte3 , 0xad},
   {rMT_LO2_Byte3 , 0xad},
   {rMT_Test_Byte1, 0x08},
   {rMT_Test_Byte2, 0x00},
   {rMT_Test_Byte3, 0x00}
};/*93 MHz,tuner2 should use 0x1c to disable buffered clock output  */



/** Table to adjust 2011 LO1 -60dBc*/
static const struct fractThreshStr gFracLo1Table60[ ] = {
   /* ft_bottom,  ft_top,  ft_newnum,  ft_divincr*/
   {     0,       128,        0,          0 },
   {   128,       255,      255,          0 },
   {  7936,       8063,    7936,          0 },
   {  8063,       8191,       0,          1 },
   {    -1,         -1,      -1,          0},   /* Bumper*/
}; 

/** Table to adjust 2011 LO2 -60dBc*/
static const struct fractThreshStr gFracLo2Table60[ ] = {
   /* ft_bottom,  ft_top,  ft_newnum,  ft_divincr*/
   {     0,        51,        0,          0 },
   {    51,       101,      101,          0 },
   {  8090,       8141,    8090,          0 },
   {  8141,       8191,       0,          1 },
   {    -1,         -1,      -1,          0},   /* Bumper*/
};

/** FREQUENCY EXCEPTION TABLE (provided by hardware group per platform)*/
struct exceptTableStr {
   U32           ets_tuner;        /* Tuner number 0, 1, 2 (2=the 2050)*/
   U32           ets_freq;         /* Frequency in Hz*/
   U32           ets_lo1;          /* LO1 to use unconditionally*/
   U32           ets_lo2;          /* LO2 to use unconditionally.*/
};

static const struct exceptTableStr gNullExceptionTable[ ] = 
{
   {100, 0, 0, 0},      /* Bumper*/
};

#endif


/***********************************************************/



/* private variables ------------------------------------------------------- */
extern IOARCH_HandleData_t IOARCH_Handle[STTUNER_IOARCH_MAX_HANDLES];

static semaphore_t *Lock_InitTermOpenClose; /* guard calls to the functions */


static BOOL Installed = FALSE;

static BOOL Installed_TDBE1       = FALSE;
static BOOL Installed_TDBE2       = FALSE;
static BOOL Installed_TDDE1       = FALSE;
static BOOL Installed_SP5730      = FALSE;
static BOOL Installed_MT2030      = FALSE;
static BOOL Installed_MT2040      = FALSE;
static BOOL Installed_MT2050      = FALSE;
static BOOL Installed_MT2060      = FALSE;
static BOOL Installed_DCT7040     = FALSE;
static BOOL Installed_DCT7050     = FALSE;
static BOOL Installed_DCT7710     = FALSE;
static BOOL Installed_DCF8710     = FALSE;
static BOOL Installed_DCF8720     = FALSE;
static BOOL Installed_MACOETA50DR = FALSE;
static BOOL Installed_CD1516LI    = FALSE;
static BOOL Installed_DF1CS1223   = FALSE;
static BOOL Installed_SHARPXX     = FALSE;
static BOOL Installed_TDBE1X016A  = FALSE;
static BOOL Installed_TDBE1X601   = FALSE;
static BOOL Installed_TDEE4X012A  = FALSE;
static BOOL Installed_DCT7045     = FALSE;
static BOOL Installed_TDQE3       = FALSE;
static BOOL Installed_DCF8783     = FALSE;
static BOOL Installed_DCT7045EVAL = FALSE;
static BOOL Installed_DCT70700    = FALSE;
static BOOL Installed_TDCHG       = FALSE;
static BOOL Installed_TDCJG       = FALSE;
static BOOL Installed_TDCGG       = FALSE;
static BOOL Installed_MT2011      = FALSE;



/* tuner instance data */

/* instance chain, the default boot value is invalid, to catch errors */
static TCDRV_InstanceData_t *InstanceChainTop = (TCDRV_InstanceData_t *)0x7fffffff;


/* functions --------------------------------------------------------------- */
/* API */

ST_ErrorCode_t tuner_tdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams);
ST_ErrorCode_t tuner_tdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams);

ST_ErrorCode_t tuner_tdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle);
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
ST_ErrorCode_t tuner_tdrv_Open_TDBE1      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
ST_ErrorCode_t tuner_tdrv_Open_TDBE2      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
ST_ErrorCode_t tuner_tdrv_Open_TDDE1      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_SP5730
ST_ErrorCode_t tuner_tdrv_Open_SP5730     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2030
ST_ErrorCode_t tuner_tdrv_Open_MT2030     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2040
ST_ErrorCode_t tuner_tdrv_Open_MT2040     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2050
ST_ErrorCode_t tuner_tdrv_Open_MT2050     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2060
ST_ErrorCode_t tuner_tdrv_Open_MT2060     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
ST_ErrorCode_t tuner_tdrv_Open_DCT7040    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
ST_ErrorCode_t tuner_tdrv_Open_DCT7050    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
ST_ErrorCode_t tuner_tdrv_Open_DCT7710    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
ST_ErrorCode_t tuner_tdrv_Open_DCF8710    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DCF8720
ST_ErrorCode_t tuner_tdrv_Open_DCF8720    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
ST_ErrorCode_t tuner_tdrv_Open_MACOETA50DR(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
ST_ErrorCode_t tuner_tdrv_Open_CD1516LI   (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
ST_ErrorCode_t tuner_tdrv_Open_DF1CS1223  (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
ST_ErrorCode_t tuner_tdrv_Open_SHARPXX    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
ST_ErrorCode_t tuner_tdrv_Open_TDBE1X016A (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
ST_ErrorCode_t tuner_tdrv_Open_TDBE1X601  (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
ST_ErrorCode_t tuner_tdrv_Open_TDEE4X012A (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045
ST_ErrorCode_t tuner_tdrv_Open_DCT7045    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDQE3
ST_ErrorCode_t tuner_tdrv_Open_TDQE3      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8783
ST_ErrorCode_t tuner_tdrv_Open_DCF8783    (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
ST_ErrorCode_t tuner_tdrv_Open_DCT7045EVAL (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT70700
ST_ErrorCode_t tuner_tdrv_Open_DCT70700   (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCHG
ST_ErrorCode_t tuner_tdrv_Open_TDCHG      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCJG
ST_ErrorCode_t tuner_tdrv_Open_TDCJG      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCGG
ST_ErrorCode_t tuner_tdrv_Open_TDCGG      (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2011
ST_ErrorCode_t tuner_tdrv_Open_MT2011     (ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle);
#endif
ST_ErrorCode_t tuner_tdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams);

ST_ErrorCode_t tuner_tdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency);
ST_ErrorCode_t tuner_tdrv_GetStatus    (TUNER_Handle_t Handle, TUNER_Status_t *Status);
ST_ErrorCode_t tuner_tdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked);
ST_ErrorCode_t tuner_tdrv_SetBandWidth (TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth);


/* I/O API */

ST_ErrorCode_t tuner_tdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,
    STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout);


/* access device specific low-level functions */
ST_ErrorCode_t tuner_tdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


/* local functions/var ----------------------------------------------------- */

ST_ErrorCode_t STTUNER_DRV_TUNER_TCDRV_Install      (STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);
ST_ErrorCode_t STTUNER_DRV_TUNER_TCDRV_UnInstall    (STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType);

static U32     TCDRV_TunerSetNbSteps                (TCDRV_InstanceData_t *Instance, U32 Steps);
static U32     TCDRV_TunerGetChargePump             (TCDRV_InstanceData_t *Instance);
static void    TCDRV_TunerSetChargePump             (TCDRV_InstanceData_t *Instance, U32 ChargePump);
static U32     TCDRV_TunerInitBand                  (TCDRV_InstanceData_t *Instance, U32 Frequency);
static U32     TCDRV_CalculateStepsAndFrequency     (TCDRV_InstanceData_t *Instance, U32 Frequency);

TCDRV_InstanceData_t    *TCDRV_GetInstFromHandle    (TUNER_Handle_t Handle);
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int              TCDRV_MT_CheckLOLock            (TCDRV_InstanceData_t *Instance);
static unsigned char    TCDRV_MT_SelectVCO              (long fLO1);
static int              TCDRV_MT_AdjustVCO              (TCDRV_InstanceData_t *Instance);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int              TCDRV_MT2040_IsSpurInBand       (TCDRV_InstanceData_t *Instance, long fLO1, long fLO2, long Spur);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2050) || defined(STTUNER_DRV_CAB_TUN_MT2060) 
static int              TCDRV_MT2050_IsSpurInBand       (TCDRV_InstanceData_t *Instance, long fLO1, long fLO2, long Spur);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int              TCDRV_MT2040_GetTunerLock       (TCDRV_InstanceData_t *Instance);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2050) || defined(STTUNER_DRV_CAB_TUN_MT2060) 
static int              TCDRV_MT2050_GetTunerLock       (TCDRV_InstanceData_t *Instance);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static long             TCDRV_MT2030_TunerSetFrequency  (TCDRV_InstanceData_t *Instance, long _Freq);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static long             TCDRV_MT2040_TunerSetFrequency  (TCDRV_InstanceData_t *Instance, long _Freq);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2050) || defined(STTUNER_DRV_CAB_TUN_MT2060) 
static long             TCDRV_MT2050_TunerSetFrequency  (TCDRV_InstanceData_t *Instance, long _Freq);
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2011 
static int TCDRV_MT2011_TunerSetFrequency(TCDRV_InstanceData_t *Instance, int _Freq);
BOOL TCDRV_MT2011_mt2011_PLLLockStatus(IOARCH_Handle_t Handle);
static void TCDRV_MT2011_rftuner_avoidSpurs(U32 *pLo1, U32 *pLo2, U32 If1Freq);
static void TCDRV_MT2011_rftuner_CalcLoNumDiv(U32 lo, U32 fref, U32 *pNum, U32 *pDiv);
static BOOL TCDRV_MT2011_rftuner_CheckExcepTable(U32 freq, U32 *pLo1, U32 *pLo2);
static U32 TCDRV_MT2011_rftuner_FractDetectLo(U32 lo, U32 fref, const struct fractThreshStr *pTab);
static BOOL TCDRV_MT2011_rftuner_FractSpurDetect(U32 *pLo1, U32 *pLo2);
static void TCDRV_MT2011_rftuner_lockFix(IOARCH_Handle_t Handle);
static void TCDRV_MT2011_rftuner_progRegs (IOARCH_Handle_t Handle, U32 lo1, U32 lo2);
static void TCDRV_MT2011_rftuner_setParameters(IOARCH_Handle_t Handle);
#endif

#if defined( STTUNER_DRV_CAB_TUN_MT2050) || defined(STTUNER_DRV_CAB_TUN_MT2060) 
static long             TCDRV_MT2060_TunerSetFrequency  (TCDRV_InstanceData_t *Instance, long _Freq);
static ST_ErrorCode_t   TCDRV_MT2060_ManualCalibrate    (TCDRV_InstanceData_t *Instance);
static void             TCDRV_MT2060_ResetExclZones     (MT_AvoidSpursData_t* pAS_Info);
static U32              TCDRV_MT2060_ChooseFirstIF      (MT_AvoidSpursData_t* pAS_Info);
static BOOL             TCDRV_MT2060_AvoidSpurs         (MT_AvoidSpursData_t* pAS_Info);
static U32              CalcLO1Mult                     (U32 *Div, U32 *FracN, U32 f_LO, U32 f_LO_Step, U32 f_Ref);
static U32              CalcLO2Mult                     (U32 *Div, U32 *FracN, U32 f_LO, U32 f_LO_Step, U32 f_Ref);
static S8               TCDRV_MT2060_Locked             (TCDRV_InstanceData_t *Instance);
static S8               TCDRV_MT2060_LO1LockWorkAround  (TCDRV_InstanceData_t *Instance);
static void             TCDRV_MT2060_AddExclZone        (MT_AvoidSpursData_t* pAS_Info, U32 f_min, U32 f_max);
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
/* MT2030 tuner */
/* ============ */

const int TCDRV_MT2030_MAX_LOSpurHarmonic = 5;   /* The maximum number of LO harmonics to search for internally-generated LO-related spurs */


/* MT2040 tuner */
/* ============ */

const U32 TCDRV_MT2040_VCO_FREQS[5]        = { 1890000,    /*  VCO 1 lower limit*/
                                             1720000,    /*  VCO 2 lower limit*/
                                               1530000,    /*  VCO 3 lower limit*/
                                               1370000,    /*  VCO 4 lower limit*/
                                                     0};   /*  VCO 5 lower limit*/                                               
const int  TCDRV_MT2040_MAX_LOSpurHarmonic =         7;    /* The maximum number of LO harmonics to search for internally-generated LO-related spurs */
      long TCDRV_MT2040_SRO_FREQ           =      5250;    /* The SRO Frequency (in kHz) : 5.25 MHz */
const long TCDRV_MT2040_PKENRANGE          =    400000;    /* The minimum frequency where the PKEN bit should be set : 400 MHz */


/* MT2050 tuner */
/* ============ */

const int  TCDRV_MT2050_MAX_LOSpurHarmonic =     11;  /* The maximum number of LO harmonics to search for internally-generated LO-related spurs */
      long TCDRV_MT2050_SRO_FREQ           =   4000;  /* The SRO Frequency (in kHz) : 4.00 MHz */
const long TCDRV_MT2050_PKENRANGE          = 275000;  /* The minimum frequency where the PKEN bit should be set : 275 MHz */
const long TCDRV_MT2050_IF1_BW             =  10000;  /* The First IF1 filter bandwidth (in kHz) : 10 MHz */


/* MT2060 tuner */
/* ============ */

const U32 TCDRV_MT2060_TUNER_VERSION   = 0x00010000;    /* Version 01.00                                      */
const U32 TCDRV_MT2060_REF_FREQ        =   16000000;    /* Reference oscillator Frequency (in Hz)             */
const U32 TCDRV_MT2060_IF1_CENTER      = 1220000000;    /* Center of the IF1 filter (in Hz)                   */
const U32 TCDRV_MT2060_IF1_BW          =   20000000;    /* The IF1 filter bandwidth (in Hz)                   */
const U32 TCDRV_MT2060_ZIF_BW          =    3000000;    /* Zero-IF spur-free bandwidth (in Hz)                */
const U32 TCDRV_MT2060_MIN_LO_SEP      =    1000000;    /* Minimum inter-tuner LO frequency separation        */
const U32 TCDRV_MT2060_LO1_FRACN_AVOID =     999999;    /* LO1 FracN numerator avoid region (in Hz)           */
const U32 TCDRV_MT2060_LO2_FRACN_AVOID =      99999;    /* LO2 FracN numerator avoid region (in Hz)           */
const U32 TCDRV_MT2060_IF1_FREQ        = 1220000000;    /* Default IF1 Frequency (in Hz)                      */
const U32 TCDRV_MT2060_SPUR_STEP_HZ    =     250000;    /* Step size (in Hz) to move IF1 when avoiding spurs  */
const U32 TCDRV_MT2060_LO1_STEP_SIZE   =     250000;    /* Step size (in Hz) of PLL1                          */
const U32 TCDRV_MT2060_MAX_HARMONICS_1 =         11;    /* Highest intra-tuner LO Spur Harmonic to be avoided */
const U32 TCDRV_MT2060_MAX_HARMONICS_2 =          7;    /* Highest inter-tuner LO Spur Harmonic to be avoided */
#endif

/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TCDRV_Install()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TCDRV_Install(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c STTUNER_DRV_TUNER_TCDRV_Install()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* driver wide bits to init once only */
    if (Installed == FALSE)
    {
        InstanceChainTop = NULL;

        Lock_InitTermOpenClose = STOS_SemaphoreCreateFifo(NULL,1);

        Installed = TRUE;
    }


    switch(TunerType)
    {
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
        case STTUNER_TUNER_TDBE1:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDBE1...", identity));
#endif
            if (Installed_TDBE1 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDBE1;
            Tuner->tuner_Open = tuner_tdrv_Open_TDBE1;
            Installed_TDBE1 = TRUE;
            break;
#endif
 
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
        case STTUNER_TUNER_TDBE2:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDBE2...", identity));
#endif
            if (Installed_TDBE2 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDBE2;
            Tuner->tuner_Open = tuner_tdrv_Open_TDBE2;
            Installed_TDBE2 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
        case STTUNER_TUNER_TDDE1:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDDE1...", identity));
#endif
            if (Installed_TDDE1 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDDE1;
            Tuner->tuner_Open = tuner_tdrv_Open_TDDE1;
            Installed_TDDE1 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SP5730
        case STTUNER_TUNER_SP5730:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:SP5730...", identity));
#endif
            if (Installed_SP5730 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_SP5730;
            Tuner->tuner_Open = tuner_tdrv_Open_SP5730;
            Installed_SP5730 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2030
        case STTUNER_TUNER_MT2030:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MT2030...", identity));
#endif
            if (Installed_MT2030 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MT2030;
            Tuner->tuner_Open = tuner_tdrv_Open_MT2030;
            Installed_MT2030 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2040
        case STTUNER_TUNER_MT2040:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MT2040...", identity));
#endif
            if (Installed_MT2040 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MT2040;
            Tuner->tuner_Open = tuner_tdrv_Open_MT2040;
            Installed_MT2040 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
        case STTUNER_TUNER_MT2050:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MT2050...", identity));
#endif
            if (Installed_MT2050 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MT2050;
            Tuner->tuner_Open = tuner_tdrv_Open_MT2050;
            Installed_MT2050 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
        case STTUNER_TUNER_MT2060:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MT2060...", identity));
#endif
            if (Installed_MT2060 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MT2060;
            Tuner->tuner_Open = tuner_tdrv_Open_MT2060;
            Installed_MT2060 = TRUE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
        case STTUNER_TUNER_DCT7040:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT7040...", identity));
#endif
            if (Installed_DCT7040 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT7040;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT7040;
            Installed_DCT7040 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7050
        case STTUNER_TUNER_DCT7050:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT7050...", identity));
#endif
            if (Installed_DCT7050 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT7050;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT7050;
            Installed_DCT7050 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7710
        case STTUNER_TUNER_DCT7710:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT7710...", identity));
#endif
            if (Installed_DCT7710 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT7710;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT7710;
            Installed_DCT7710 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8710
        case STTUNER_TUNER_DCF8710:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCF8710...", identity));
#endif
            if (Installed_DCF8710 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCF8710;
            Tuner->tuner_Open = tuner_tdrv_Open_DCF8710;
            Installed_DCF8710 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8720
        case STTUNER_TUNER_DCF8720:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCF8720...", identity));
#endif
            if (Installed_DCF8720 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCF8720;
            Tuner->tuner_Open = tuner_tdrv_Open_DCF8720;
            Installed_DCF8720 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case STTUNER_TUNER_MACOETA50DR:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MACOETA50DR...", identity));
#endif
            if (Installed_MACOETA50DR == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MACOETA50DR;
            Tuner->tuner_Open = tuner_tdrv_Open_MACOETA50DR;
            Installed_MACOETA50DR = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case STTUNER_TUNER_CD1516LI:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:CD1516LI...", identity));
#endif
            if (Installed_CD1516LI == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_CD1516LI;
            Tuner->tuner_Open = tuner_tdrv_Open_CD1516LI;
            Installed_CD1516LI = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
        case STTUNER_TUNER_DF1CS1223:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DF1CS1223...", identity));
#endif
            if (Installed_DF1CS1223 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DF1CS1223;
            Tuner->tuner_Open = tuner_tdrv_Open_DF1CS1223;
            Installed_DF1CS1223 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
        case STTUNER_TUNER_SHARPXX:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:SHARPXX...", identity));
#endif
            if (Installed_SHARPXX == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_SHARPXX;
            Tuner->tuner_Open = tuner_tdrv_Open_SHARPXX;
            Installed_SHARPXX = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case STTUNER_TUNER_TDBE1X016A:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDBE1X016A...", identity));
#endif
            if (Installed_TDBE1X016A == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDBE1X016A;
            Tuner->tuner_Open = tuner_tdrv_Open_TDBE1X016A;
            Installed_TDBE1X016A = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case STTUNER_TUNER_TDBE1X601:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDBE1X601...", identity));
#endif
            if (Installed_TDBE1X601 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDBE1X601;
            Tuner->tuner_Open = tuner_tdrv_Open_TDBE1X601;
            Installed_TDBE1X601 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case STTUNER_TUNER_TDEE4X012A:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDEE4X012A...", identity));
#endif
            if (Installed_TDEE4X012A == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDEE4X012A;
            Tuner->tuner_Open = tuner_tdrv_Open_TDEE4X012A;
            Installed_TDEE4X012A = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7045
        case STTUNER_TUNER_DCT7045:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT7045...", identity));
#endif
            if (Installed_DCT7045 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT7045;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT7045;
            Installed_DCT7045 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDQE3
        case STTUNER_TUNER_TDQE3:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDQE3...", identity));
#endif
            if (Installed_TDQE3 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDQE3;
            Tuner->tuner_Open = tuner_tdrv_Open_TDQE3;
            Installed_TDQE3 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8783
        case STTUNER_TUNER_DCF8783:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCF8783...", identity));
#endif
            if (Installed_DCF8783 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCF8783;
            Tuner->tuner_Open = tuner_tdrv_Open_DCF8783;
            Installed_DCF8783 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
        case STTUNER_TUNER_DCT7045EVAL:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT7045EVAL...", identity));
#endif
            if (Installed_DCT7045EVAL == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT7045EVAL;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT7045EVAL;
            Installed_DCT7045EVAL = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT70700
        case STTUNER_TUNER_DCT70700:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:DCT70700...", identity));
#endif
            if (Installed_DCT70700 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_DCT70700;
            Tuner->tuner_Open = tuner_tdrv_Open_DCT70700;
            Installed_DCT70700 = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCHG
        case STTUNER_TUNER_TDCHG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDCHG...", identity));
#endif
            if (Installed_TDCHG == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDCHG;
            Tuner->tuner_Open = tuner_tdrv_Open_TDCHG;
            Installed_TDCHG = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCJG
        case STTUNER_TUNER_TDCJG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDCJG...", identity));
#endif
            if (Installed_TDCJG == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDCJG;
            Tuner->tuner_Open = tuner_tdrv_Open_TDCJG;
            Installed_TDCJG = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCGG
        case STTUNER_TUNER_TDCGG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:TDCGG...", identity));
#endif
            if (Installed_TDCGG == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_TDCGG;
            Tuner->tuner_Open = tuner_tdrv_Open_TDCGG;
            Installed_TDCGG = TRUE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2011
        case STTUNER_TUNER_MT2011:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s installing cable:tuner:MT2011...", identity));
#endif
            if (Installed_MT2011 == TRUE)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("fail already installed\n"));
#endif
                return(STTUNER_ERROR_INITSTATE);
            }
            Tuner->ID = STTUNER_TUNER_MT2011;
            Tuner->tuner_Open = tuner_tdrv_Open_MT2011;
            Installed_MT2011 = TRUE;
            break;
#endif

            default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

    /* map rest of API */
    Tuner->tuner_Init  = tuner_tdrv_Init;
    Tuner->tuner_Term  = tuner_tdrv_Term;
    Tuner->tuner_Close = tuner_tdrv_Close;

    Tuner->tuner_SetFrequency  = tuner_tdrv_SetFrequency;
    Tuner->tuner_GetStatus     = tuner_tdrv_GetStatus;
    Tuner->tuner_IsTunerLocked = tuner_tdrv_IsTunerLocked;
    Tuner->tuner_SetBandWidth  = tuner_tdrv_SetBandWidth;

    Tuner->tuner_ioaccess = tuner_tdrv_ioaccess;
    Tuner->tuner_ioctl    = tuner_tdrv_ioctl;


#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("ok\n"));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: STTUNER_DRV_TUNER_TCDRV_UnInstall()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_DRV_TUNER_TCDRV_UnInstall(STTUNER_tuner_dbase_t  *Tuner, STTUNER_TunerType_t TunerType)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c STTUNER_DRV_TUNER_TCDRV_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;


    switch(TunerType)
    {
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
        case STTUNER_TUNER_TDBE1:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDBE1\n", identity));
#endif
            Installed_TDBE1 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
        case STTUNER_TUNER_TDBE2:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDBE2\n", identity));
#endif
            Installed_TDBE2 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
        case STTUNER_TUNER_TDDE1:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDDE1\n", identity));
#endif
            Installed_TDDE1 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SP5730
        case STTUNER_TUNER_SP5730:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:SP5730\n", identity));
#endif
            Installed_SP5730 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2030
        case STTUNER_TUNER_MT2030:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MT2030\n", identity));
#endif
            Installed_MT2030 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2040
        case STTUNER_TUNER_MT2040:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MT2040\n", identity));
#endif
            Installed_MT2040 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
        case STTUNER_TUNER_MT2050:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MT2050\n", identity));
#endif
            Installed_MT2050 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
        case STTUNER_TUNER_MT2060:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MT2060\n", identity));
#endif
            Installed_MT2060 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
        case STTUNER_TUNER_DCT7040:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT7040\n", identity));
#endif
            Installed_DCT7040 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
        case STTUNER_TUNER_DCT7050:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT7050\n", identity));
#endif
            Installed_DCT7050 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
        case STTUNER_TUNER_DCT7710:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT7710\n", identity));
#endif
            Installed_DCT7710 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
        case STTUNER_TUNER_DCF8710:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCF8710\n", identity));
#endif
            Installed_DCF8710 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8720
        case STTUNER_TUNER_DCF8720:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCF8720\n", identity));
#endif
            Installed_DCF8720 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case STTUNER_TUNER_MACOETA50DR:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MACOETA50DR\n", identity));
#endif
            Installed_MACOETA50DR = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case STTUNER_TUNER_CD1516LI:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:CD1516LI\n", identity));
#endif
            Installed_CD1516LI = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
        case STTUNER_TUNER_DF1CS1223:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DF1CS1223\n", identity));
#endif
            Installed_DF1CS1223 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
        case STTUNER_TUNER_SHARPXX:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:SHARPXX\n", identity));
#endif
            Installed_SHARPXX = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case STTUNER_TUNER_TDBE1X016A:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDBE1X016A\n", identity));
#endif
            Installed_TDBE1X016A = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case STTUNER_TUNER_TDBE1X601:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDBE1X601\n", identity));
#endif
            Installed_TDBE1X601 = FALSE;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case STTUNER_TUNER_TDEE4X012A:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDEE4X012A\n", identity));
#endif
            Installed_TDEE4X012A = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7045
        case STTUNER_TUNER_DCT7045:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT7045\n", identity));
#endif
            Installed_DCT7045 = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDQE3
        case STTUNER_TUNER_TDQE3:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDQE3\n", identity));
#endif
            Installed_TDQE3 = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8783
        case STTUNER_TUNER_DCF8783:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCF8783\n", identity));
#endif
            Installed_DCF8783 = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
        case STTUNER_TUNER_DCT7045EVAL:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT7045EVAL\n", identity));
#endif
            Installed_DCT7045EVAL = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT70700
        case STTUNER_TUNER_DCT70700:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:DCT70700\n", identity));
#endif
            Installed_DCT70700 = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCHG
        case STTUNER_TUNER_TDCHG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDCHG\n", identity));
#endif
            Installed_TDCHG = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCJG
        case STTUNER_TUNER_TDCJG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDCJG\n", identity));
#endif
            Installed_TDCJG = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDCGG
        case STTUNER_TUNER_TDCGG:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:TDCGG\n", identity));
#endif
            Installed_TDCGG = FALSE;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2011
        case STTUNER_TUNER_MT2011:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s uninstalling cable:tuner:MT2011\n", identity));
#endif
            Installed_MT2011 = FALSE;
            break;
#endif

            default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

    /* unmap API */
    Tuner->ID = STTUNER_NO_DRIVER;

    Tuner->tuner_Init  = NULL;
    Tuner->tuner_Term  = NULL;
    Tuner->tuner_Close = NULL;

    Tuner->tuner_SetFrequency  = NULL;
    Tuner->tuner_GetStatus     = NULL;
    Tuner->tuner_IsTunerLocked = NULL;
    Tuner->tuner_SetBandWidth  = NULL;

    Tuner->tuner_ioaccess = NULL;
    Tuner->tuner_ioctl    = NULL;

    if ( (Installed_TDBE1       == FALSE) &&
         (Installed_TDBE2       == FALSE) &&
         (Installed_TDDE1       == FALSE) &&
         (Installed_SP5730      == FALSE) &&
         (Installed_MT2030      == FALSE) &&
         (Installed_MT2040      == FALSE) &&
         (Installed_MT2050      == FALSE) &&
         (Installed_MT2060      == FALSE) &&
         (Installed_DCT7040     == FALSE) &&
         (Installed_DCT7050     == FALSE) &&
         (Installed_DCT7710     == FALSE) &&
         (Installed_DCF8710     == FALSE) &&
         (Installed_DCF8720     == FALSE) &&
         (Installed_MACOETA50DR == FALSE) &&
         (Installed_CD1516LI    == FALSE) &&
         (Installed_DF1CS1223   == FALSE) &&
         (Installed_SHARPXX     == FALSE) &&
         (Installed_TDBE1X016A  == FALSE) &&
         (Installed_TDBE1X601   == FALSE) &&
         (Installed_TDEE4X012A  == FALSE) &&
         (Installed_DCT7045     == FALSE) &&
         (Installed_TDQE3       == FALSE) &&
         (Installed_DCF8783     == FALSE) &&
         (Installed_DCT7045EVAL == FALSE) &&
         (Installed_DCT70700    == FALSE) &&
         (Installed_TDCHG       == FALSE) &&
         (Installed_TDCJG       == FALSE) &&
         (Installed_TDCGG       == FALSE) &&
         (Installed_MT2011      == FALSE) )
         
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s <", identity));
#endif

        STOS_SemaphoreDelete(NULL,Lock_InitTermOpenClose);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print((">"));
#endif

        InstanceChainTop = (TCDRV_InstanceData_t *)0x7ffffffe;
        Installed        = FALSE;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("all tdrv drivers uninstalled\n"));
#endif
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("ok\n"));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TDBE1_Install()
        STTUNER_DRV_TUNER_TDBE2_Install()
        STTUNER_DRV_TUNER_TDDE1_Install()
        STTUNER_DRV_TUNER_SP5730_Install()
        STTUNER_DRV_TUNER_MT2030_Install()
        STTUNER_DRV_TUNER_MT2040_Install()
        STTUNER_DRV_TUNER_MT2050_Install()
        STTUNER_DRV_TUNER_MT2060_Install()
        STTUNER_DRV_TUNER_DCT7040_Install()
        STTUNER_DRV_TUNER_DCT7050_Install()
        STTUNER_DRV_TUNER_DCT7710_Install()
        STTUNER_DRV_TUNER_DCF8710_Install()
        STTUNER_DRV_TUNER_MACOETA50DR_Install()
        STTUNER_DRV_TUNER_DCF8720_Install()
        STTUNER_DRV_TUNER_CD1516LI_Install()
        STTUNER_DRV_TUNER_DF1CS1223_Install()
        STTUNER_DRV_TUNER_SHARPXX_Install()
        STTUNER_DRV_TUNER_TDBE1X016A_Install()
        STTUNER_DRV_TUNER_TDBE1X601_Install()
        STTUNER_DRV_TUNER_TDEE4X012A_Install()
        STTUNER_DRV_TUNER_DCT7045_Install()
        STTUNER_DRV_TUNER_TDQE3_Install()
        STTUNER_DRV_TUNER_DCF8783_Install()
        STTUNER_DRV_TUNER_DCT7045EVAL_Install()
        STTUNER_DRV_TUNER_DCT70700_Install()
        STTUNER_DRV_TUNER_TDCHG_Install()
        STTUNER_DRV_TUNER_TDCJG_Install()
        STTUNER_DRV_TUNER_TDCGG_Install()
        STTUNER_DRV_TUNER_MT2011_Install()

Description:
    install a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDBE1) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE2_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDBE2) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
ST_ErrorCode_t STTUNER_DRV_TUNER_TDDE1_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDDE1) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_SP5730
ST_ErrorCode_t STTUNER_DRV_TUNER_SP5730_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_SP5730) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2030
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2030_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MT2030) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2040
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2040_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MT2040) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2050_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MT2050) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MT2060) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7040_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT7040) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7050_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT7050) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7710_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT7710) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8710_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCF8710) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8720
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8720_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCF8720) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
ST_ErrorCode_t STTUNER_DRV_TUNER_MACOETA50DR_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MACOETA50DR) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
ST_ErrorCode_t STTUNER_DRV_TUNER_CD1516LI_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_CD1516LI) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
ST_ErrorCode_t STTUNER_DRV_TUNER_DF1CS1223_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DF1CS1223) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
ST_ErrorCode_t STTUNER_DRV_TUNER_SHARPXX_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_SHARPXX) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X016A_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDBE1X016A) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X601_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDBE1X601) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEE4X012A_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDEE4X012A) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT7045) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDQE3
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQE3_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDQE3) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8783
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8783_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCF8783) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045EVAL_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT7045EVAL) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT70700
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT70700_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_DCT70700) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCHG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCHG_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDCHG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCJG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCJG_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDCJG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCGG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCGG_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_TDCGG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2011
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2011_Install(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_Install(Tuner, STTUNER_TUNER_MT2011) );
}
#endif

/* ----------------------------------------------------------------------------
Name:   STTUNER_DRV_TUNER_TDBE1_UnInstall()
        STTUNER_DRV_TUNER_TDBE2_UnInstall()
        STTUNER_DRV_TUNER_TDDE1_UnInstall()
        STTUNER_DRV_TUNER_SP5730_UnInstall()
        STTUNER_DRV_TUNER_MT2030_UnInstall()
        STTUNER_DRV_TUNER_MT2040_UnInstall()
        STTUNER_DRV_TUNER_MT2050_UnInstall()
        STTUNER_DRV_TUNER_MT2060_UnInstall()
        STTUNER_DRV_TUNER_DCT7040_UnInstall()
        STTUNER_DRV_TUNER_DCT7050_UnInstall()
        STTUNER_DRV_TUNER_DCT7710_UnInstall()
        STTUNER_DRV_TUNER_DCF8710_UnInstall()
        STTUNER_DRV_TUNER_DCF8720_UnInstall()
        STTUNER_DRV_TUNER_MACOETA50DR_UnInstall()
        STTUNER_DRV_TUNER_CD1516LI_UnInstall()
        STTUNER_DRV_TUNER_DF1CS1223_UnInstall()
        STTUNER_DRV_TUNER_SHARPXX_UnInstall()
        STTUNER_DRV_TUNER_TDBE1X016A_UnInstall()
        STTUNER_DRV_TUNER_TDBE1X601_UnInstall()
        STTUNER_DRV_TUNER_TDEE4X012A_UnInstall()
        STTUNER_DRV_TUNER_DCT7045_UnInstall()
        STTUNER_DRV_TUNER_TDQE3_UnInstall()
        STTUNER_DRV_TUNER_DCF8783_UnInstall()
        STTUNER_DRV_TUNER_DCT7045EVAL_UnInstall()
        STTUNER_DRV_TUNER_DCT70700_UnInstall()
        STTUNER_DRV_TUNER_TDCHG_UnInstall()
        STTUNER_DRV_TUNER_TDCJG_UnInstall()
        STTUNER_DRV_TUNER_TDCGG_UnInstall()
        STTUNER_DRV_TUNER_MT2011_UnInstall()

Description:
    uninstall a cable device driver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDBE1) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE2_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDBE2) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
ST_ErrorCode_t STTUNER_DRV_TUNER_TDDE1_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDDE1) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_SP5730
ST_ErrorCode_t STTUNER_DRV_TUNER_SP5730_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_SP5730) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2030
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2030_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MT2030) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2040
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2040_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MT2040) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2050_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MT2050) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MT2060) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7040_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT7040) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7050_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT7050) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7710_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT7710) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8710_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCF8710) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8720
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8720_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCF8720) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
ST_ErrorCode_t STTUNER_DRV_TUNER_MACOETA50DR_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MACOETA50DR) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
ST_ErrorCode_t STTUNER_DRV_TUNER_CD1516LI_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_CD1516LI) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
ST_ErrorCode_t STTUNER_DRV_TUNER_DF1CS1223_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DF1CS1223) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
ST_ErrorCode_t STTUNER_DRV_TUNER_SHARPXX_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_SHARPXX) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X016A_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDBE1X016A) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X601_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDBE1X601) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEE4X012A_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDEE4X012A) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT7045) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDQE3
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQE3_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDQE3) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8783
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8783_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCF8783) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045EVAL_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT7045EVAL) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT70700
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT70700_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_DCT70700) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCHG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCHG_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDCHG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCJG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCJG_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDCJG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCGG
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCGG_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_TDCGG) );
}
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2011
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2011_UnInstall(STTUNER_tuner_dbase_t *Tuner)
{
    return( STTUNER_DRV_TUNER_TCDRV_UnInstall(Tuner, STTUNER_TUNER_MT2011) );
}
#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ API /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_Init()

Description: called for every perdriver.

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Init(ST_DeviceName_t *DeviceName, TUNER_InitParams_t *InitParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Init()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *InstanceNew, *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no driver installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(InitParams->MemoryPartition);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail MemoryPartition not valid (%s)\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    Error = STTUNER_Util_CheckPtrNull(DeviceName);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail DeviceName not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    InstanceNew = STOS_MemoryAllocateClear(InitParams->MemoryPartition, 1, sizeof( TCDRV_InstanceData_t ));
    if (InstanceNew == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail memory allocation InstanceNew\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_NO_MEMORY);
    }

    /* slot into chain */
    if (InstanceChainTop == NULL)
    {
        InstanceNew->InstanceChainPrev = NULL; /* no previous instance */
        InstanceChainTop = InstanceNew;
    }
    else    /* tag onto last data block in chain */
    {
        Instance = InstanceChainTop;

        while(Instance->InstanceChainNext != NULL)
        {
            Instance = Instance->InstanceChainNext;   /* next block */
        }
        Instance->InstanceChainNext     = (void *)InstanceNew;
        InstanceNew->InstanceChainPrev  = (void *)Instance;
    }

    InstanceNew->DeviceName          = DeviceName;
    InstanceNew->TopLevelHandle      = STTUNER_MAX_HANDLES;
    InstanceNew->IOHandle            = InitParams->IOHandle;
    InstanceNew->MemoryPartition     = InitParams->MemoryPartition;
    InstanceNew->InstanceChainNext   = NULL; /* always last in the chain */
    InstanceNew->TunerType = InitParams->TunerType;

    switch(InstanceNew->TunerType)
    {
#ifdef STTUNER_DRV_CAB_TUN_TDBE1
        case STTUNER_TUNER_TDBE1:
            InstanceNew->PLLType = TUNER_PLL_TDBE1;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE2
        case STTUNER_TUNER_TDBE2:
            InstanceNew->PLLType = TUNER_PLL_TDBE2;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
        case STTUNER_TUNER_TDDE1:
            InstanceNew->PLLType = TUNER_PLL_TDDE1;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SP5730
        case STTUNER_TUNER_SP5730:
            InstanceNew->PLLType = TUNER_PLL_SP5730;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2030
        case STTUNER_TUNER_MT2030:
            InstanceNew->PLLType = TUNER_PLL_MT2030;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2040
        case STTUNER_TUNER_MT2040:
            InstanceNew->PLLType = TUNER_PLL_MT2040;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2050
        case STTUNER_TUNER_MT2050:
            InstanceNew->PLLType = TUNER_PLL_MT2050;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
        case STTUNER_TUNER_MT2060:
            InstanceNew->PLLType = TUNER_PLL_MT2060;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
        case STTUNER_TUNER_DCT7040:
             InstanceNew->PLLType = TUNER_PLL_DCT7040;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
        case STTUNER_TUNER_DCT7050:
            InstanceNew->PLLType = TUNER_PLL_DCT7050;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
        case STTUNER_TUNER_DCT7710:
            InstanceNew->PLLType = TUNER_PLL_DCT7710;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
        case STTUNER_TUNER_DCF8710:
            InstanceNew->PLLType = TUNER_PLL_DCF8710;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8720
        case STTUNER_TUNER_DCF8720:
            InstanceNew->PLLType = TUNER_PLL_DCF8720;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case STTUNER_TUNER_MACOETA50DR:
            InstanceNew->PLLType = TUNER_PLL_MACOETA50DR;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case STTUNER_TUNER_CD1516LI:
            InstanceNew->PLLType = TUNER_PLL_CD1516LI;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
        case STTUNER_TUNER_DF1CS1223:
            InstanceNew->PLLType = TUNER_PLL_DF1CS1223;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
        case STTUNER_TUNER_SHARPXX:
            InstanceNew->PLLType = TUNER_PLL_SHARPXX;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case STTUNER_TUNER_TDBE1X016A:
            InstanceNew->PLLType = TUNER_PLL_TDBE1X016A;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case STTUNER_TUNER_TDBE1X601:
            InstanceNew->PLLType = TUNER_PLL_TDBE1X601;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case STTUNER_TUNER_TDEE4X012A:
            InstanceNew->PLLType = TUNER_PLL_TDEE4X012A;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045
        case STTUNER_TUNER_DCT7045:
            InstanceNew->PLLType = TUNER_PLL_DCT7045;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDQE3
        case STTUNER_TUNER_TDQE3:
            InstanceNew->PLLType = TUNER_PLL_TDQE3;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8783
        case STTUNER_TUNER_DCF8783:
            InstanceNew->PLLType = TUNER_PLL_DCF8783;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
        case STTUNER_TUNER_DCT7045EVAL:
            InstanceNew->PLLType = TUNER_PLL_DCT7045EVAL;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT70700
        case STTUNER_TUNER_DCT70700:
            InstanceNew->PLLType = TUNER_PLL_DCT70700;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCHG
        case STTUNER_TUNER_TDCHG:
            InstanceNew->PLLType = TUNER_PLL_TDCHG;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCJG
        case STTUNER_TUNER_TDCJG:
            InstanceNew->PLLType = TUNER_PLL_TDCJG;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDCGG
        case STTUNER_TUNER_TDCGG:
            InstanceNew->PLLType = TUNER_PLL_TDCGG;
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2011
        case STTUNER_TUNER_MT2011:
            InstanceNew->PLLType = TUNER_PLL_MT2011;
            break;
#endif

            default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s incorrect tuner index", identity));
#endif
            return(ST_ERROR_UNKNOWN_DEVICE);
            break;
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s allocated & initalized block named '%s' at 0x%08x (%d bytes) for  tuner ID=%d\n", identity, InstanceNew->DeviceName, InstanceNew, sizeof( TCDRV_InstanceData_t ), InstanceNew->TunerType ));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_Term()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Term(ST_DeviceName_t *DeviceName, TUNER_TermParams_t *TermParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Term()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance, *InstancePrev, *InstanceNext;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check params ---------- */
    Error = STTUNER_Util_CheckPtrNull(TermParams);
    if( Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail TermParams not valid\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(Error);
    }

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }


    /* reap next matching DeviceName */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("Looking (%s)", Instance->DeviceName));
#endif
    while(1)
    {
        if ( strcmp((char *)Instance->DeviceName, (char *)DeviceName) == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("]\n"));
#endif
            /* found so now xlink prev and next(if applicable) and deallocate memory */
            InstancePrev = Instance->InstanceChainPrev;
            InstanceNext = Instance->InstanceChainNext;

            /* if instance to delete is first in chain */
            if (Instance->InstanceChainPrev == NULL)
            {
                InstanceChainTop = InstanceNext;        /* which would be NULL if last block to be term'd */
                if (InstanceNext != NULL)
                {
                    InstanceNext->InstanceChainPrev = NULL; /* now top of chain, no previous instance */
                }
            }
            else
            {   /* safe to set value for prev instaance (because there IS one) */
                InstancePrev->InstanceChainNext = InstanceNext;
            }

            /* if there is a next block in the chain */
            if (InstanceNext != NULL)
            {
                InstanceNext->InstanceChainPrev = InstancePrev;
            }

            STOS_MemoryDeallocate(Instance->MemoryPartition, Instance);
            #if defined(STTUNER_DRV_CAB_STV0297E)
            STOS_MemoryDeallocate(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition, IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream);
            #endif 
            
            
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s freed block at %0x%08x\n", identity, (U32)Instance ));
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s terminated ok\n", identity));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(Error);
        }
        else if(Instance->InstanceChainNext == NULL)
        {       /* error we should have found a matching name before the end of the list */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("\n%s fail no free handle before end of list\n", identity));
#endif
                SEM_UNLOCK(Lock_InitTermOpenClose);
                return(STTUNER_ERROR_INITSTATE);
        }
        else
        {
            Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("(%s)", Instance->DeviceName));
#endif
        }

    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s FAIL! this point should NEVER be reached\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tdrv_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    /* now safe to lock semaphore */
    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    /* find  handle from name */
    Instance = InstanceChainTop;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s looking (%s)", identity, Instance->DeviceName));
#endif
    while(strcmp((char *)Instance->DeviceName, (char *)DeviceName) != 0)
    {
        /* error, should have found matching DeviceName before end of list */
        if(Instance->InstanceChainNext == NULL)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("\n%s fail no block named '%s' before end of list\n", identity, DeviceName));
#endif
            SEM_UNLOCK(Lock_InitTermOpenClose);
            return(STTUNER_ERROR_INITSTATE);
        }
        Instance = Instance->InstanceChainNext;   /* next block */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("(%s)", Instance->DeviceName));
#endif
    }
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print((" found ok\n"));
#endif

    if(Instance->TopLevelHandle != STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver instance already open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }

    /* now got pointer to free (and valid) data block */

    *Handle = (U32)Instance;
    

    
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);
}


#ifdef STTUNER_DRV_CAB_TUN_TDBE1
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDBE1()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDBE1(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDBE1()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDBE1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDBE1 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;    
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 470000L;
    Instance->Status.IntermediateFrequency = 36150;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDBE2
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDBE2()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDBE2(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDBE2()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDBE2)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDBE2 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

     #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif



    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 470000L;
    Instance->Status.IntermediateFrequency = 36150;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif  
  #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDDE1
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDDE1()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDDE1(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDDE1()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDDE1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDDE1 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Instance->FreqFactor = 1000;

    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif



    Instance->TunerLowBand      = 150000L; /* When Center freq is 146M, B1; Center freq is 154M, B2*/
    Instance->TunerMidBand      = 438000L;  /* When Center freq is 434M, B2; Center freq is 442M, B4*/
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x09;
    Instance->IOBuffer[1] = 0x62;
    Instance->IOBuffer[2] = 0xCE;
    Instance->IOBuffer[3] = 0x01;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif   
 #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_SP5730
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_SP5730()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_SP5730(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_SP5730()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_SP5730)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_SP5730 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    
    
    
    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 470000L;
    Instance->Status.IntermediateFrequency = 36150;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x8E;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif  
 #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2030
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MT2030()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MT2030(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MT2030()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    U16 CompanyCode;
    U8  PartCode, RevisionCode;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail, Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_MT2030)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2030 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /*
    --- Read registers
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), TCDRV_MT2040_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, TCDRV_MT2040_NB_REGS, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]));
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }

    CompanyCode     = ((Instance->IOBuffer[TCDRV_MT2040_COCODE_1]) << 8) + (Instance->IOBuffer[TCDRV_MT2040_COCODE_2]);
    PartCode        = Instance->IOBuffer[TCDRV_MT2040_PARTCODE];
    RevisionCode    = Instance->IOBuffer[TCDRV_MT2040_REVCODE];
    STTBX_Print(("Compagny Code %04x Part Code %02x Revision Code %02x\n",CompanyCode, PartCode, RevisionCode));
    if (PartCode != TCDRV_TUNER_PART_CODE_MT2030)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s %s must be connected to MT2030\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
    #endif


    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 0L;
    Instance->TunerMidBand      = 0L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[TCDRV_MT2040_LO1_1]      = 0x00;  /* LO1_1 = 0,          0x00: LO1 Byte 1 */
    Instance->IOBuffer[TCDRV_MT2040_LO1_2]      = 0x00;  /* LO1_2,              0x01: LO1 Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_LOGC]       = 0xFF;  /* LOGC,               0x02: LO GC Cntl */
                                                     /* Set LOGC Control to maximum values */

    Instance->IOBuffer[TCDRV_MT2040_RESERVED_1] = 0x0F;  /* RESERVED_1,         0x03: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LOVCOC] = 0x1F;      /* RESERVED_2,         0x04: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LO2_1]      = 0x00;  /* LO2_1,              0x05: LO2 Byte 1 */

    Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]  = 0xE4;  /* CONTROL_1,          0x06: Control Byte 1 */
                                                     /* Enable Tune Line A/D's; Enable AFC A/D */

    Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]  = 0x8F;  /* CONTROL_2,          0x07: Control Byte 2 */
                                                     /* Enable LO lock detect; max SRO gain */

    Instance->IOBuffer[TCDRV_MT2040_RESERVED_3] = 0xC3;  /* RESERVED_3,         0x08: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_4] = 0x4E;  /* RESERVED_4,         0x09: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_5] = 0xEC;  /* RESERVED_5,         0x0A: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LO2_2]      = 0x00;  /* LO2_2,              0x0B: LO2 Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_LO2_3]      = 0x00;  /* LO2_3,              0x0C: LO2 Byte 3 */
    Instance->IOBuffer[TCDRV_MT2040_MIX1]       = 0x32;  /* MIX1,               0x0D: MIX1 Cntl */
    Instance->IOBuffer[TCDRV_MT2040_STATUS]     = 0x00;  /* STATUS,             0x0E: Status */
    Instance->IOBuffer[TCDRV_MT2040_TUNEA2D]    = 0x00;  /* TUNEA2D,            0x0F: Tune A/D */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_6] = 0x00;  /* RESERVED_6,         0x10: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_COCODE_1]   = 0x00;  /* COCODE_1,           0x11: Company Code Byte 1 */
    Instance->IOBuffer[TCDRV_MT2040_COCODE_2]   = 0x00;  /* COCODE_2,           0x12: Company Code Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_PARTCODE]   = 0x00;  /* PARTCODE,           0x13: Part Code */
    Instance->IOBuffer[TCDRV_MT2040_REVCODE]    = 0x00;  /* REVCODE             0x14: Revision Code */

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /*
    --- Write these register values to the tuner
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 3, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 3);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_CONTROL_1, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]), 5, TCDRV_IO_TIMEOUT);
  }
  #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_CONTROL_1, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]), 5);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_MIX1, &(Instance->IOBuffer[TCDRV_MT2040_MIX1]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif
  			   
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_MIX1, &(Instance->IOBuffer[TCDRV_MT2040_MIX1]), 1);
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    /*
    --- Delay at least 10 msec before verifying XOGC setting
    */
    tuner_tdrv_DelayInMilliSec(TCDRV_MT2040_WAITING_TIME);

    /*
    --- Decrement the XOGC register until:
    --- I2C-bus error -OR- XOGC reaches 4 -OR- the XOK bit becomes 1.
    */
    while (Instance->IOBuffer[TCDRV_MT2040_CONTROL_2] > 0x8C)
    {
        /*	Read the XOK bit from the status register*/
     #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_STATUS, &(Instance->IOBuffer[TCDRV_MT2040_STATUS]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif  
 
        #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
        Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_STATUS, 1, &(Instance->IOBuffer[TCDRV_MT2040_STATUS]));
    }
    #endif
    

        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
            return Error;
        }

        if (((Instance->IOBuffer[TCDRV_MT2040_STATUS]) & 0x01) == 1)        /* if XOK is set, we're done */
        {
            break;
        }

        --(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]);                     /*  decrement XOGC */
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif	
        #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
        Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1);
    }
    #endif
    

        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
            return Error;
        }

        /* .... */
        tuner_tdrv_DelayInMilliSec(TCDRV_MT2040_WAITING_TIME);
    }

    /*
    --- End of MT
    */
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2040
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MT2040()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MT2040(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MT2040()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    U16 CompanyCode;
    U8  PartCode, RevisionCode;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
    #endif

    if (Instance->TunerType != STTUNER_TUNER_MT2040)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2040 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /*
    --- Read registers
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), TCDRV_MT2040_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, TCDRV_MT2040_NB_REGS, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]));
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }

    CompanyCode     = ((Instance->IOBuffer[TCDRV_MT2040_COCODE_1]) << 8) + (Instance->IOBuffer[TCDRV_MT2040_COCODE_2]);
    PartCode        = Instance->IOBuffer[TCDRV_MT2040_PARTCODE];
    RevisionCode    = Instance->IOBuffer[TCDRV_MT2040_REVCODE];
    STTBX_Print(("Compagny Code %04x Part Code %02x Revision Code %02x\n",CompanyCode, PartCode, RevisionCode));
    if (PartCode != TCDRV_TUNER_PART_CODE_MT2040)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s %s must be connected to MT2040\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 0L;
    Instance->TunerMidBand      = 0L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[TCDRV_MT2040_LO1_1]      = 0x1f;  /* LO1_1 = 0,          0x00: LO1 Byte 1 */
    Instance->IOBuffer[TCDRV_MT2040_LO1_2]      = 0x00;  /* LO1_2,              0x01: LO1 Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_LOGC]       = 0xFF;  /* LOGC,               0x02: LO GC Cntl */
                                                     /* Set LOGC Control to maximum values */

    Instance->IOBuffer[TCDRV_MT2040_RESERVED_1] = 0x0F;  /* RESERVED_1,         0x03: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LOVCOC]     = 0x1F;  /* RESERVED_2,         0x04: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LO2_1]      = 0x00;  /* LO2_1,              0x05: LO2 Byte 1 */

    Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]  = 0xE4;  /* CONTROL_1,          0x06: Control Byte 1 */
                                                     /* Enable Tune Line A/D's; Enable AFC A/D */

    Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]  = 0x0f;  /* CONTROL_2,          0x07: Control Byte 2 */
                                                     /* Enable LO lock detect; max SRO gain */

    Instance->IOBuffer[TCDRV_MT2040_RESERVED_3] = 0xC3;  /* RESERVED_3,         0x08: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_4] = 0x4E;  /* RESERVED_4,         0x09: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_5] = 0xEC;  /* RESERVED_5,         0x0A: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_LO2_2]      = 0x00;  /* LO2_2,              0x0B: LO2 Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_LO2_3]      = 0x00;  /* LO2_3,              0x0C: LO2 Byte 3 */
    Instance->IOBuffer[TCDRV_MT2040_MIX1]       = 0x32;  /* MIX1,               0x0D: MIX1 Cntl */
    Instance->IOBuffer[TCDRV_MT2040_STATUS]     = 0x00;  /* STATUS,             0x0E: Status */
    Instance->IOBuffer[TCDRV_MT2040_TUNEA2D]    = 0x00;  /* TUNEA2D,            0x0F: Tune A/D */
    Instance->IOBuffer[TCDRV_MT2040_RESERVED_6] = 0x00;  /* RESERVED_6,         0x10: Reserved */
    Instance->IOBuffer[TCDRV_MT2040_COCODE_1]   = 0x00;  /* COCODE_1,           0x11: Company Code Byte 1 */
    Instance->IOBuffer[TCDRV_MT2040_COCODE_2]   = 0x00;  /* COCODE_2,           0x12: Company Code Byte 2 */
    Instance->IOBuffer[TCDRV_MT2040_PARTCODE]   = 0x00;  /* PARTCODE,           0x13: Part Code */
    Instance->IOBuffer[TCDRV_MT2040_REVCODE]    = 0x00;  /* REVCODE             0x14: Revision Code */

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /*
    --- Write these register values to the tuner
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif	 
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 1);
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif  

    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_CONTROL_1, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]), 2, TCDRV_IO_TIMEOUT);
  }
  #endif	  
   #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_CONTROL_1, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_1]), 2);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    /*
    --- Delay at least 10 msec before verifying XOGC setting
    */
    tuner_tdrv_DelayInMilliSec(TCDRV_MT2040_WAITING_TIME);

    /*
    --- Read all registers
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), TCDRV_MT2040_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, TCDRV_MT2040_NB_REGS, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]));
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    /*
    --- End of MT
    */
    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2050
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MT2050()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MT2050(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MT2050()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    U8  PartCode, RevisionCode;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
    #endif

    if (Instance->TunerType != STTUNER_TUNER_MT2050)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2050 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /*
    --- Read registers
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, 0, &(Instance->IOBuffer[0]), TCDRV_MT2050_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipGetRegisters(Instance->IOHandle, 0, TCDRV_MT2050_NB_REGS, &(Instance->IOBuffer[0]));
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }

    PartCode        = Instance->IOBuffer[TCDRV_MT2050_PART_REV]; /* Read Part + Revision */
    RevisionCode    = PartCode & 0x0F;
    PartCode        = PartCode >> 4;
    STTBX_Print(("Part Code %02x Revision Code %02x\n", PartCode, RevisionCode));
    if (PartCode != TCDRV_TUNER_PART_CODE_MT2050)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s %s must be connected to MT2050\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 0L;
    Instance->TunerMidBand      = 0L;
#ifdef STTUNER_DRV_CAB_DOCSIS_FE
    Instance->Status.IntermediateFrequency = 43750;
#else
    Instance->Status.IntermediateFrequency = 36150;
#endif
    Instance->Status.TunerStep = 50000;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /*
    --- Init registers
    */
    Instance->IOBuffer[TCDRV_MT2050_LO1C_1]      = 0x2F;
    Instance->IOBuffer[TCDRV_MT2050_LO1C_2]      = 0x25;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2050_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO1C_1]), (TCDRV_MT2050_LO1C_2-TCDRV_MT2050_LO1C_1+1), TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2050_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO1C_1]), (TCDRV_MT2050_LO1C_2-TCDRV_MT2050_LO1C_1+1));
  }
  #endif
  
  


    Instance->IOBuffer[TCDRV_MT2050_LO2C_1]      = 0xC1;
    Instance->IOBuffer[TCDRV_MT2050_LO2C_2]      = 0x00;
    Instance->IOBuffer[TCDRV_MT2050_LO2C_3]      = 0x63;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2050_LO2C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO2C_1]), (TCDRV_MT2050_LO2C_3-TCDRV_MT2050_LO2C_1+1), TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2050_LO2C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO2C_1]), (TCDRV_MT2050_LO2C_3-TCDRV_MT2050_LO2C_1+1));
  }
  #endif
  


    Instance->IOBuffer[TCDRV_MT2050_PWR]         = 0x10;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {
    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2050_PWR,    &(Instance->IOBuffer[TCDRV_MT2050_PWR]),    (1), TCDRV_IO_TIMEOUT);
  }
  #endif
    #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {
    Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2050_PWR,    &(Instance->IOBuffer[TCDRV_MT2050_PWR]),    (1));
  }
  #endif
  



    /*
    --- Read all registers
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, 0, &(Instance->IOBuffer[0]), TCDRV_MT2050_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error |= ChipGetRegisters(Instance->IOHandle, 0, TCDRV_MT2050_NB_REGS, &(Instance->IOBuffer[0]));
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    /*
    ** Verify that the SRO error bit is not set.  If it is, the tuner cannot function.
    */
    tuner_tdrv_DelayInMilliSec (10);
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2050_SRO, &(Instance->IOBuffer[TCDRV_MT2050_SRO]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2050_SRO, 1, &(Instance->IOBuffer[TCDRV_MT2050_SRO]));
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    if ((Instance->IOBuffer[TCDRV_MT2050_SRO] & 0x40) == 0x40)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s SRO error bit not set, tuner cannot function\n", identity));
#endif
        return (STTUNER_ERROR_HWFAIL);
    }

    Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    return (ST_NO_ERROR);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2060
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MT2060()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MT2060(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MT2060()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    U8  PartCode, RevisionCode, i;

    U8 defaults[] =
    {
        0x3F,            /* reg[0x01] <- 0x3F               */
        0x74,            /* reg[0x02] <- 0x74               */
        0x00,            /* reg[0x03] <- 0x00               */
        0x08,            /* reg[0x04] <- 0x08               */
        0x93,            /* reg[0x05] <- 0x93               */
        0x88,            /* reg[0x06] <- 0x88 (RO)          */
        0x80,            /* reg[0x07] <- 0x80 (RO)          */
        0x60,            /* reg[0x08] <- 0x60 (RO)          */
        0x20,            /* reg[0x09] <- 0x20               */
        0x1E,            /* reg[0x0A] <- 0x1E               */
        0x31,            /* reg[0x0B] <- 0x31 (VGAG = 1)    */
        0xFF,            /* reg[0x0C] <- 0xFF               */
        0x80,            /* reg[0x0D] <- 0x80               */
        0xFF,            /* reg[0x0E] <- 0xFF               */
        0x00,            /* reg[0x0F] <- 0x00               */
        0x2C,            /* reg[0x10] <- 0x2C (HW def = 3C) */
        0x42,            /* reg[0x11] <- 0x42               */
    };

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_SUBADR_8_NS_MICROTUNE;
    #endif

    if (Instance->TunerType != STTUNER_TUNER_MT2060)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2060 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /*
    --- Read registers
    */
    /*Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, 0, &(Instance->IOBuffer[0]), TCDRV_MT2060_NB_REGS, TCDRV_IO_TIMEOUT);*/
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, 0, &(Instance->IOBuffer[TCDRV_MT2060_PART_REV]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif	 

    /*Error = ChipGetRegisters(Instance->IOHandle, 0, TCDRV_MT2060_NB_REGS, &(Instance->IOBuffer[0]));*/
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, 0, 1, &(Instance->IOBuffer[TCDRV_MT2060_PART_REV]));
  }
  #endif
  

    #ifndef ST_OSLINUX 
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }
    #endif

    PartCode     = Instance->IOBuffer[TCDRV_MT2060_PART_REV]; /* Read Part + Revision */
    
    RevisionCode = PartCode & 0x0F;
    PartCode     = PartCode >> 4;
    STTBX_Print(("Part Code %02x Revision Code %02x\n", PartCode, RevisionCode));
    #ifndef ST_OSLINUX 
    if (PartCode != TCDRV_TUNER_PART_CODE_MT2060)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s %s must be connected to MT2060\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    #endif

    Instance->FreqFactor                   = 1000;
    Instance->TunerLowBand                 = 0L;
    Instance->TunerMidBand                 = 0L;
#ifdef STTUNER_DRV_CAB_DOCSIS_FE
    Instance->Status.IntermediateFrequency = 43750;
#else
    Instance->Status.IntermediateFrequency = 36150;
#endif
    Instance->Status.TunerStep             = 50000;
    Instance->Status.IQSense               = 1;
    Instance->BandWidth[0]                 = 8000;
    Instance->BandWidth[1]                 = 0;
    Instance->Status.Bandwidth             = Instance->BandWidth[0];                /* Select default bandwidth */
    Instance->version                      = TCDRV_MT2060_TUNER_VERSION;            /* Initialize the MicroTuner state. */
    Instance->AS_Data.f_ref                = TCDRV_MT2060_REF_FREQ;
    Instance->AS_Data.f_in                 = 651000000;
    Instance->AS_Data.f_if1_Center         = TCDRV_MT2060_IF1_CENTER;
    Instance->AS_Data.f_if1_bw             = TCDRV_MT2060_IF1_BW;
    Instance->AS_Data.f_out                = Instance->Status.IntermediateFrequency * 1000;
    Instance->AS_Data.f_out_bw             = 6750000;
    Instance->AS_Data.f_zif_bw             = TCDRV_MT2060_ZIF_BW;
    Instance->AS_Data.f_LO1_Step           = TCDRV_MT2060_REF_FREQ / 64;
    Instance->AS_Data.f_LO2_Step           = Instance->Status.TunerStep;
    Instance->AS_Data.maxH1                = TCDRV_MT2060_MAX_HARMONICS_1;
    Instance->AS_Data.maxH2                = TCDRV_MT2060_MAX_HARMONICS_2;
    Instance->AS_Data.f_min_LO_Separation  = TCDRV_MT2060_MIN_LO_SEP;
    Instance->AS_Data.f_if1_Request        = TCDRV_MT2060_IF1_CENTER;
    Instance->AS_Data.f_LO1                = 1871000000;
    Instance->AS_Data.f_LO2                = 1176250000;
    Instance->f_IF1_actual                 = Instance->AS_Data.f_LO1 - Instance->AS_Data.f_in;
    Instance->AS_Data.f_LO1_FracN_Avoid    = TCDRV_MT2060_LO1_FRACN_AVOID;
    Instance->AS_Data.f_LO2_FracN_Avoid    = TCDRV_MT2060_LO2_FRACN_AVOID;
    for ( i = 0; i < 10; i++ )
    {
        Instance->LNA_Bands[i] = MT2060_LNA_Bands[i];
    }

    /*
    --- Init registers
    */
    for ( i = 0; i < sizeof(defaults); i++ )
    {
        Instance->IOBuffer[i+1] = defaults[i];
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
    if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
   {

    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2060_LO1C_1]), sizeof(defaults), TCDRV_IO_TIMEOUT);

    /*
    --- Read all registers
    */
    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, 0, &(Instance->IOBuffer[0]), TCDRV_MT2060_NB_REGS, TCDRV_IO_TIMEOUT);
  }
  #endif
 #if defined(STTUNER_DRV_CAB_STV0297E)
    if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
   {

    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2060_LO1C_1]), sizeof(defaults));

    /*
    --- Read all registers
    */
    Error |= ChipGetRegisters(Instance->IOHandle, 0, TCDRV_MT2060_NB_REGS, &(Instance->IOBuffer[0]));
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return Error;
    }

    /* Different defaults for A2 chip */
    if (Instance->IOBuffer[0] == 0x62)
    {
        Instance->IOBuffer[TCDRV_MT2060_MISC_CTRL_1] = 0x30;
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_MISC_CTRL_1, &(Instance->IOBuffer[TCDRV_MT2060_MISC_CTRL_1]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif   
        #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_MISC_CTRL_1, &(Instance->IOBuffer[TCDRV_MT2060_MISC_CTRL_1]), 1);
    }
    #endif
    

        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
            return Error;
        }
    }

    Error = TCDRV_MT2060_ManualCalibrate(Instance);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }

    Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    return (ST_NO_ERROR);
}
#endif
ST_ErrorCode_t TCDRV_MT2060_ManualCalibrate(TCDRV_InstanceData_t *Instance)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    U8              idx;    /* loop index variable          */

    /*  Set the FM1 Single-Step bit  */
    Instance->IOBuffer[TCDRV_MT2060_LO2C_1] |= 0x40;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1, TCDRV_IO_TIMEOUT);
  }
  #endif	 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1);
  }
  #endif
  

    if (Error != ST_NO_ERROR) return Error;

    /*  Set the FM1CA bit to start the calibration  */
    Instance->IOBuffer[TCDRV_MT2060_LO2C_1] |= 0x80;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1, TCDRV_IO_TIMEOUT);
  }
  #endif
  		 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1);
  }
  #endif

    if (Error != ST_NO_ERROR) return Error;

    Instance->IOBuffer[TCDRV_MT2060_LO2C_1] &= 0x7F;    /*  FM1CA bit automatically goes back to 0  */

    /*  Toggle the single-step 8 times  */
    for (idx=0; idx<8; ++idx)
    {
        tuner_tdrv_DelayInMilliSec(20);
        /*  Clear the FM1 Single-Step bit  */
        Instance->IOBuffer[TCDRV_MT2060_LO2C_1] &= 0xBF;
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1, TCDRV_IO_TIMEOUT);
    }
    #endif	
        #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1);
    }
    #endif
    
    

        if (Error != ST_NO_ERROR) return  Error;

        tuner_tdrv_DelayInMilliSec(20);
        /*  Set the FM1 Single-Step bit  */
        Instance->IOBuffer[TCDRV_MT2060_LO2C_1] |= 0x40;
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1, TCDRV_IO_TIMEOUT);
    }
    #endif	
        #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1);
    }
    #endif
    

        if (Error != ST_NO_ERROR) return Error;
    }

    /*  Clear the FM1 Single-Step bit  */
    Instance->IOBuffer[TCDRV_MT2060_LO2C_1] &= 0xBF;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1, TCDRV_IO_TIMEOUT);
  }
  #endif
  			
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO2C_1, &Instance->IOBuffer[TCDRV_MT2060_LO2C_1], 1);
  }
  #endif

    if (Error != ST_NO_ERROR) return Error;

    idx = 25;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_MISC_STATUS, &Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS], 1, TCDRV_IO_TIMEOUT);
  }
  #endif 
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_MISC_STATUS, 1, &Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS]);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR) return Error;

    /*  Poll FMCAL until it is set or we time out  */
    while ( --idx && (Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS] & 0x40) == 0)
    {
        tuner_tdrv_DelayInMilliSec(20);
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_MISC_STATUS, &Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS], 1, TCDRV_IO_TIMEOUT);
    }
    #endif   
       #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_MISC_STATUS, 1, &Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS]);
    }
    #endif
    

        if (Error != ST_NO_ERROR) return Error;
    }

    if ((Instance->IOBuffer[TCDRV_MT2060_MISC_STATUS] & 0x40) != 0)
    {
        /*  Read the 1st IF center offset register  */
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_FM_FREQ, &Instance->IOBuffer[TCDRV_MT2060_FM_FREQ], 1, TCDRV_IO_TIMEOUT);
    }
    #endif	 
        #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_FM_FREQ, 1, &Instance->IOBuffer[TCDRV_MT2060_FM_FREQ]);
    }
    #endif
    
    

        if (Error != ST_NO_ERROR) return Error;

        Instance->AS_Data.f_if1_Center = (U32)(1000000L * (1082L + Instance->IOBuffer[TCDRV_MT2060_FM_FREQ]));
    }

    return (ST_NO_ERROR);
}


#ifdef STTUNER_DRV_CAB_TUN_DCT7040
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7040()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT7040(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT7040()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    if (Instance->TunerType != STTUNER_TUNER_DCT7040)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT7040 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 0;
    Instance->TunerMidBand      = 0;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x86;   /* Control Byte 1 */
    Instance->IOBuffer[3] = 0x00;   /* B5 not used */

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 3 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 3, TCDRV_IO_TIMEOUT*100);
  }
  #endif
  	  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,3);
  }
  #endif

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7050
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7050()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT7050(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT7050()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCT7050)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT7050 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
     
    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 141000;
    Instance->TunerMidBand      = 426000;
    Instance->Status.IntermediateFrequency = 43750;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 6000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x86;   /* Control Byte 1 */
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 3 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 3, TCDRV_IO_TIMEOUT*100);
  }
  #endif
  	   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,3);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCT7710
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7710()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT7710(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT7710()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCT7710)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT7710 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    
    
    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 145000;
    Instance->TunerMidBand      = 428000;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT*100);
  }
  #endif   
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8710
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCF8710()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCF8710(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCF8710()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    
    if (Instance->TunerType != STTUNER_TUNER_DCF8710)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCF8710 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 146000L;
    Instance->TunerMidBand      = 430000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif  
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_DCF8720
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCF8720()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCF8720(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCF8720()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCF8720)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCF8720 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
    
    
    
    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 146000L;
    Instance->TunerMidBand      = 430000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x8e;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif   
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MACOETA50DR()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MACOETA50DR(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MACOETA50DR()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_MACOETA50DR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MACOETA50DR for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
     
     #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif
     
     
    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 147000L;
    Instance->TunerMidBand      = 427000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x8e;   /* Control Byte 1 */
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif
  		   
  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_CD1516LI()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_CD1516LI(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_CD1516LI()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_CD1516LI)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_CD1516LI for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 454000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif   
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DF1CS1223()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DF1CS1223(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DF1CS1223()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DF1CS1223)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DF1CS1223 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 172000L;
    Instance->TunerMidBand      = 450000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x86;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif	
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_SHARPXX()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_SHARPXX(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_SHARPXX()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    
    
    Instance = TCDRV_GetInstFromHandle(*Handle);
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif


    if (Instance->TunerType != STTUNER_TUNER_SHARPXX)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_SHARPXX for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 146000L;
    Instance->TunerMidBand      = 430000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x8E;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif 
  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDBE1X016A()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDBE1X016A(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDBE1X016A()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDBE1X016A)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDBE1X016A for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 470000L;
    Instance->Status.IntermediateFrequency = 36150;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif	
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDBE1X601()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDBE1X601(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDBE1X601()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    
    

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDBE1X601)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDBE1X601 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 174000L;
    Instance->TunerMidBand      = 470000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x8E;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif   
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDEE4X012A()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDEE4X012A(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDEE4X012A()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDEE4X012A)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDEE4X012A for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 153000L;
    Instance->TunerMidBand      = 430000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0B;
    Instance->IOBuffer[1] = 0x02;
    Instance->IOBuffer[2] = 0x85;
    Instance->IOBuffer[3] = 0x08;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
  }
  #endif 
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif


#ifdef STTUNER_DRV_CAB_TUN_DCT7045
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7045()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT7045(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT7045()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCT7045)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT7045 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 185000L;
    Instance->TunerMidBand      = 465000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x16;
    Instance->IOBuffer[1] = 0x60;
    Instance->IOBuffer[2] = 0x86;
    Instance->IOBuffer[3] = 0x01;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif  
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_TDQE3
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDQE3()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDQE3(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDQE3()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDQE3)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDQE3 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 153000;
    Instance->TunerMidBand      = 430000;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x1f;
    Instance->IOBuffer[1] = 0xe2;
    Instance->IOBuffer[2] = 0xa3;
    Instance->IOBuffer[3] = 0xaa;
    Instance->IOBuffer[4] = 0xc6;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 5 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 5, TCDRV_IO_TIMEOUT*100);
  }
  #endif
  		 
  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,5);
  }
  #endif

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif




#ifdef STTUNER_DRV_CAB_TUN_DCF8783
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCF8783()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCF8783(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCF8783()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCF8783)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCF8783 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 179000L;
    Instance->TunerMidBand      = 459000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x16;
    Instance->IOBuffer[1] = 0x60;
    Instance->IOBuffer[2] = 0x86;
    Instance->IOBuffer[3] = 0x01;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif	
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_DCT7045EVAL
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT7045EVAL()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT7045EVAL(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT7045EVAL()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCT7045EVAL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT7045EVAL for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 185000L;
    Instance->TunerMidBand      = 465000L;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x16;
    Instance->IOBuffer[1] = 0x60;
    Instance->IOBuffer[2] = 0x86;
    Instance->IOBuffer[3] = 0x01;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif  
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_DCT70700
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_DCT70700()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_DCT70700(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_DCT70700()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_DCT70700)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_DCT70700 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 185000;
    Instance->TunerMidBand      = 465000;
    Instance->Status.IntermediateFrequency = 36000;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x0b;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x8b;
    Instance->IOBuffer[3] = 0x01;
    Instance->IOBuffer[4] = 0xc3;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 5 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 5, TCDRV_IO_TIMEOUT*100);
  }
  #endif   
  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,5);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_TDCHG
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDCHG()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDCHG(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDCHG()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDCHG)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDCHG for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 160000L;
    Instance->TunerMidBand      = 445000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x00;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x00;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif  
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_TDCJG
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDCJG()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDCJG(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDCJG()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDCJG)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDCJG for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 160000L;
    Instance->TunerMidBand      = 445000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x00;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x00;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif 
 #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif



#ifdef STTUNER_DRV_CAB_TUN_TDCGG
/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_TDCGG()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_TDCGG(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_TDCGG()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }

    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_TDCGG)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_TDCGG for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    #if defined(STTUNER_DRV_CAB_STV0297E)
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Timeout   = TCDRV_IO_TIMEOUT*100;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.Mode      = IOREG_MODE_NOSUBADR;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.WrSize = 4;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.RdSize =1;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition =Instance->MemoryPartition;
    IOARCH_Handle[Instance->IOHandle].DeviceMap.ByteStream = STOS_MemoryAllocateClear(IOARCH_Handle[Instance->IOHandle].DeviceMap.MemoryPartition,5,sizeof(U8));
    #endif

    Instance->FreqFactor = 1000;

    Instance->TunerLowBand      = 160000L;
    Instance->TunerMidBand      = 445000L;
    Instance->Status.IntermediateFrequency = 36125;
    Instance->Status.TunerStep = 62500;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    Instance->IOBuffer[0] = 0x00;
    Instance->IOBuffer[1] = 0x00;
    Instance->IOBuffer[2] = 0x00;
    Instance->IOBuffer[3] = 0x00;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /* Set device configuration (demod MUST have been initalized if this tuner is using its repeater)
       subaddress 0, Tx 4 bytes
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, 4, TCDRV_IO_TIMEOUT*100);
  }
  #endif  
  #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,4);
  }
  #endif
  

    if (Error == ST_NO_ERROR)
    {
        Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s opened ok\n", identity));
#endif
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
    }

    return(Error);
}
#endif


#ifdef STTUNER_DRV_CAB_TUN_MT2011


/*------------------------------------------------------------------------------
 Function:   rxtuner_Read()

 Scope:      Public

 Parameters: reg   -  The address of the register to read
             pData -  Where to put the data that is read
             len   -  How many bytes to read

 Returns:    None

 Purpose:    Reads a MT2011 register or registers at the specified location.

 Behavior:   Utilizes I2C driver to communicate with the MT2011 part.
------------------------------------------------------------------------------*/
void rxtuner_Read(void *I2CHandle,U8 reg, U8 *pData, U8 len)
{
   U8 rData[8],WriteTimeout=100,ReadTimeout=100;
   U32 actLen;
   ST_ErrorCode_t Error=ST_NO_ERROR;

   Error = STI2C_Write(I2C_HANDLE(I2CHandle), (U8 *) &reg, 1, WriteTimeout, &actLen );
   /*if(Error!=ST_NO_ERROR)
   	{
   		printf("\n\nError by STI2C_Write API in rxtuner_Read!!-->  Error:0x%x\n\n",Error);
   	}*/
   Error = STI2C_Read(I2C_HANDLE(I2CHandle), rData, len, ReadTimeout, &actLen);
   /*if(Error!=ST_NO_ERROR)
   	{
   		printf("\n\nError by STI2C_Read API in rxtuner_Read!!-->  Error:0x%x\n\n",Error);
   	}*/
   memcpy(pData, &rData[0], len);
}




/*------------------------------------------------------------------------------
 Function:   rxtuner_Write()

 Scope:      Public

 Parameters: reg   -  The address of the register to write
             pData -  An array of bytes to write
             len   -  How many bytes to write

 Returns:    None

 Purpose:    Writes a MT2011 register or registers at the specified location.

 Behavior:   Utilizes I2C driver to communicate with the MT2011 part.
------------------------------------------------------------------------------*/
void rxtuner_Write (void *I2CHandle, U8 reg, U8 *pData, U8 len)
{
   
   U8 wData[12], WriteTimeout=100;
   U32 actLen;
   /*U8 tempvar[1]={0};*/
   ST_ErrorCode_t Error=ST_NO_ERROR;
   
   wData[0] = reg;
   memcpy(&wData[1], pData, len++);
   Error = STI2C_Write( I2C_HANDLE(I2CHandle), wData, len, WriteTimeout, &actLen );
   
   /*if(Error!=ST_NO_ERROR)
   	{
   		printf("\n\nError by STI2C_Write API in rxtuner_Write!!-->  Error:0x%x\n\n",Error);
   	}
   	
   if (len==2)
   	{
   		printf("\n\nSETTUNER Regaddr:0x%x Value:0x%x\r\n",reg,wData[1]);
   	}
   if (len ==3)
   	{
   		printf("\n\nSETTUNER Regaddr:0x%x Value:0x%x\r\n",reg,wData[1]);
   		printf("\n\nSETTUNER Regaddr:0x%x Value:0x%x\r\n",reg,wData[2]);
   	}
   
   rxtuner_Read(I2CHandle, reg, &(tempvar[0]), len);
   if( (reg!=0x08) && (reg!=0x13) && (reg!=0x15) )
   {
   if( tempvar[0] != (*pData) )
   	{
   		printf("\n\nError in Tuner Write Value!!-->  RegisterSubAddrs/Offset:0x%x, Written:%u, Read:%u\n\n",reg,(*pData),tempvar[0]);
   	}
   }*/
}

/*------------------------------------------------------------------------------
 Function:   rftuner_read8()

 Scope:      Private

 Parameters: reg   -  The address of the register to read

 Returns:    value of the requested register

 Purpose:    Reads a MT2011 register at the specified location.

 Behavior:   Utilizes I2C driver to communicate with the MT2011 part.
------------------------------------------------------------------------------*/
static U8 rftuner_read8(void *I2CHandle, U8 reg)
{
   U32 actLen;
   U8 pData[2], WriteTimeout=100, ReadTimeout=100; /*data would be read in pData[0]*/
   ST_ErrorCode_t Error=ST_NO_ERROR;

   
   /*address is written from where to read the value, Ack bit(9)would be received from Slave ie tuner here*/
   Error = STI2C_Write(I2C_HANDLE(I2CHandle), (U8 *) &reg, 1, WriteTimeout, &actLen );
   /*if(Error!=ST_NO_ERROR)
   	{
   		printf("\n\nError by STI2C_Write API in rftuner_read8!!-->  Error:0x%x\n\n",Error);
   	}*/
   /*data would be read into pData[0] from tuner.*/
   Error = STI2C_Read(I2C_HANDLE(I2CHandle), pData, 1, ReadTimeout, &actLen);
   /*if(Error!=ST_NO_ERROR)
   	{
   		printf("\n\nError by STI2C_Read API in rftuner_read8!!-->  Error:0x%x\n\n",Error);
   	}*/

   return pData[0];
}


/* ----------------------------------------------------------------------------
Name:   tuner_tdrv_Open_MT2011()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Open_MT2011(ST_DeviceName_t *DeviceName, TUNER_OpenParams_t *OpenParams, TUNER_Capability_t *Capability, TUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Open_MT2011()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    U8  i,tmp8;
    
    Error = tuner_tdrv_Open(DeviceName, OpenParams, Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(Error);
    }
    Instance = TCDRV_GetInstFromHandle(*Handle);

    if (Instance->TunerType != STTUNER_TUNER_MT2011)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail no STTUNER_TUNER_MT2011 for name (%s)\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    
    
    /** Initialize the registers*/
   for (i=0; i<kINIT_REGS; i++)
   {
      tmp8 = gRegInit[i].value;
      rxtuner_Write(&IOARCH_Handle[Instance->IOHandle].ExtDev, gRegInit[i].reg, &tmp8, 1);
   }
   STOS_TaskDelayUs(100000);
   
      gRxTuner.type = kRx_TypeMT2011;
      gRxTuner.IF2 = kFif2;  
      gRxTuner.PartCode = (rftuner_read8(&IOARCH_Handle[Instance->IOHandle].ExtDev, rMT_Part) & 0xF8) >> 3;
      gRxTuner.RevCode = rftuner_read8(&IOARCH_Handle[Instance->IOHandle].ExtDev, rMT_Part) & 0x07;
    
    Instance->FreqFactor = 1000;
    Instance->Status.IntermediateFrequency = kFif2/1000;
    Instance->Status.TunerStep = kFstep;
    Instance->Status.IQSense = 1;

    Instance->BandWidth[0] = 8000;
    Instance->BandWidth[1] = 0;

    /* Select default bandwidth */
    Instance->Status.Bandwidth = Instance->BandWidth[0];

    /*
    --- Read registers
    */
      
    STTBX_Print(("Part Code %02x Revision Code %02x\n", gRxTuner.PartCode, gRxTuner.RevCode));
    #ifndef ST_OSLINUX 
    /*if (PartCode != TCDRV_TUNER_PART_CODE_MT2011)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s %s must be connected to MT2011\n", identity, Instance->DeviceName ));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }*/
    #endif

   
    Instance->TopLevelHandle = OpenParams->TopLevelHandle; /* mark as used */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s opened ok\n", identity));
#endif

    return (ST_NO_ERROR);
}
#endif



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_Close(TUNER_Handle_t Handle, TUNER_CloseParams_t *CloseParams)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;

    Instance = TCDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail Error=%d\n", identity, Error));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    SEM_LOCK(Lock_InitTermOpenClose);

    /* ---------- check that at least one init has taken place ---------- */
    if(InstanceChainTop == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail nothing initalized\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(STTUNER_ERROR_INITSTATE);
    }

    if(Instance->TopLevelHandle == STTUNER_MAX_HANDLES)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver instance not open\n", identity));
#endif
        SEM_UNLOCK(Lock_InitTermOpenClose);
        return(ST_ERROR_OPEN_HANDLE);
    }
    Instance->TopLevelHandle = STTUNER_MAX_HANDLES;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s closed\n", identity));
#endif
    SEM_UNLOCK(Lock_InitTermOpenClose);
    return(Error);

}
/* ----------------------------------------------------------------------------
Name: tuner_tdrv_SetFrequency()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_SetFrequency (TUNER_Handle_t Handle, U32 Frequency, U32 *NewFrequency)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
	U32     NbSteps, TunerNbSteps;
    U32     TunerBand;
    U32     ChargePump;
    BOOL    Locked=FALSE;
    U32     ii=0;
    BOOL    MicrotuneTuner=FALSE;

    TCDRV_InstanceData_t *Instance;

    Instance = TCDRV_GetInstFromHandle(Handle);

    /*
    --- MT or not MT
    */
    if (Instance->TunerType == STTUNER_TUNER_MT2030)
    {
    	#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
        *NewFrequency = (U32)TCDRV_MT2030_TunerSetFrequency(Instance, Frequency);
        MicrotuneTuner=TRUE;
    	#endif
    }
    else if (Instance->TunerType == STTUNER_TUNER_MT2040)
    {
       #if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
       *NewFrequency = (U32)TCDRV_MT2040_TunerSetFrequency(Instance, Frequency);
        MicrotuneTuner=TRUE;
        #endif
    }
    else if (Instance->TunerType == STTUNER_TUNER_MT2050)
    {
       #if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
       *NewFrequency = (U32)TCDRV_MT2050_TunerSetFrequency(Instance, Frequency);
        MicrotuneTuner=TRUE;
       #endif 
    }
    else if (Instance->TunerType == STTUNER_TUNER_MT2060)
    {
      #if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
       *NewFrequency = (U32)TCDRV_MT2060_TunerSetFrequency(Instance, Frequency);
        MicrotuneTuner=TRUE;
    	#endif
    }
    else if (Instance->TunerType == STTUNER_TUNER_MT2011)
    {
      #ifdef STTUNER_DRV_CAB_TUN_MT2011
       *NewFrequency = (U32)TCDRV_MT2011_TunerSetFrequency(Instance, Frequency);
        MicrotuneTuner=TRUE;
    	#endif
    }

    if (MicrotuneTuner==TRUE)
    {
        if (*NewFrequency == 0)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
           STTBX_Print(("%s Can't access lock FrontEnd to %d KHz\n", identity, Frequency));
#endif
           *NewFrequency = Frequency;
        }
    }
    else
    {
        /*
        --- Calculate number of steps for nominated frequency
        */
        NbSteps = TCDRV_CalculateStepsAndFrequency(Instance, Frequency);

        /*
        --- Set InBand
        */
        TunerBand = TCDRV_TunerInitBand(Instance, Frequency);

        /*
        --- Set Steps
        */
        TunerNbSteps = TCDRV_TunerSetNbSteps(Instance, NbSteps);

        /*
        --- Set new frequency of device
        */
        Instance->Status.Frequency = ((TunerNbSteps*(Instance->Status.TunerStep)) - ((Instance->FreqFactor))*(Instance->Status.IntermediateFrequency))/((Instance->FreqFactor));
        *NewFrequency = Instance->Status.Frequency;

        /*
        --- ChargePump
        */
        ChargePump = TCDRV_TunerGetChargePump(Instance) ;
        if( (Instance->TunerType != STTUNER_TUNER_DCT7045)&&(Instance->TunerType != STTUNER_TUNER_TDQE3)&&(Instance->TunerType != STTUNER_TUNER_DCT7045EVAL)&&(Instance->TunerType != STTUNER_TUNER_DCT70700)&&(Instance->TunerType != STTUNER_TUNER_TDCHG)&&(Instance->TunerType != STTUNER_TUNER_TDCJG)&&(Instance->TunerType != STTUNER_TUNER_TDCGG) )
        {
        	TCDRV_TunerSetChargePump(Instance, 1);
        }

        /*
        --- Update Tuner register
        */
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
    }
    #endif	
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
        Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
    }
    #endif
    

        if (Error == ST_NO_ERROR)
        {
            /*
            --- Tuner Is Locked ?
            */
            ii = 0 ;
            while( (!Locked) && (ii < 200) )
            {
                tuner_tdrv_IsTunerLocked(Handle, &Locked);
                tuner_tdrv_DelayInMilliSec(TCDRV_WAITING_TIME);
                ii +=TCDRV_WAITING_TIME;
            }


	        if( (Instance->TunerType != STTUNER_TUNER_DCT7045)&&(Instance->TunerType != STTUNER_TUNER_TDQE3)&&(Instance->TunerType != STTUNER_TUNER_DCT7045EVAL)&&(Instance->TunerType != STTUNER_TUNER_DCT70700)&&(Instance->TunerType != STTUNER_TUNER_TDCHG)&&(Instance->TunerType != STTUNER_TUNER_TDCJG)&&(Instance->TunerType != STTUNER_TUNER_TDCGG) )
	        {
            /*
            --- Set ChargePump
            */
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME) ;
            TCDRV_TunerSetChargePump(Instance, ChargePump) ;

            /*
            --- Update Tuner register
            */
            #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
            Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_WRITE, 0, Instance->IOBuffer, TCDRV_IOBUFFER_SIZE, TCDRV_IO_TIMEOUT);
    }
    #endif	
       #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
            Error = ChipSetRegisters(Instance->IOHandle, 0, Instance->IOBuffer,TCDRV_IOBUFFER_SIZE);
    }
    #endif
    
    

            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("%s Can't access to FrontEnd (Write loop locked)\n", identity));
#endif
            }
        	}
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Can't access to FrontEnd (Write Update register)\n", identity));
#endif
        }
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s Frequency %u KHz NewFrequency %u KHz\n", identity, Frequency, *NewFrequency));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: tuner_tdrv_GetStatus()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_GetStatus(TUNER_Handle_t Handle, TUNER_Status_t *Status)
{
    TCDRV_InstanceData_t *Instance;

    Instance = TCDRV_GetInstFromHandle(Handle);

    *Status = Instance->Status;

    return(ST_NO_ERROR);
}



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_IsTunerLocked()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_IsTunerLocked(TUNER_Handle_t Handle, BOOL *Locked)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_IsTunerLocked()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_IOARCH_Operation_t OpType = STTUNER_IO_READ;/*removing warning on LINUX*/
    U8 RxStatus=0;
    U8 SubAddr=0;
    U8 BitMask=0;
    TCDRV_InstanceData_t *Instance;

    Instance = TCDRV_GetInstFromHandle(Handle);

    *Locked = FALSE;    /* Assume not locked */


    switch (Instance->PLLType)
    {
#if defined ( STTUNER_DRV_CAB_TUN_MT2030)||defined (STTUNER_DRV_CAB_TUN_MT2040)        
        case TUNER_PLL_MT2030:
        case TUNER_PLL_MT2040:
            BitMask = 0x06;
            SubAddr = TCDRV_MT2040_STATUS;
            OpType  = STTUNER_IO_SA_READ_NS;
            break;
#endif            
#ifdef STTUNER_DRV_CAB_TUN_MT2050
        case TUNER_PLL_MT2050:
            BitMask = 0x88;
            SubAddr = TCDRV_MT2050_LO_STATUS;
            OpType  = STTUNER_IO_SA_READ_NS;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MT2060
        case TUNER_PLL_MT2060:
            BitMask = 0x88;
            SubAddr = TCDRV_MT2060_LO_STATUS;
            OpType  = STTUNER_IO_SA_READ_NS;
            break;
#endif 
#ifdef STTUNER_DRV_CAB_TUN_MT2011
        case TUNER_PLL_MT2011:
            /*BitMask = 0x88;
            SubAddr = TCDRV_MT2011_LO_STATUS;
            OpType  = STTUNER_IO_SA_READ_NS;*/
            *Locked = TCDRV_MT2011_mt2011_PLLLockStatus(Instance->IOHandle);
            return(Error);
            break;
#endif


        case TUNER_PLL_TDBE1:
        case TUNER_PLL_TDBE2:
        case TUNER_PLL_TDDE1:
        case TUNER_PLL_SP5730:
        case TUNER_PLL_DCT7040:
        case TUNER_PLL_DCT7050:
        case TUNER_PLL_DCT7710:
        case TUNER_PLL_DCF8710:
        case TUNER_PLL_DCF8720:
        case TUNER_PLL_MACOETA50DR:
        case TUNER_PLL_CD1516LI:
        case TUNER_PLL_DF1CS1223:
        case TUNER_PLL_SHARPXX:
        case TUNER_PLL_TDBE1X016A:
        case TUNER_PLL_TDBE1X601:
        case TUNER_PLL_TDEE4X012A:
        case TUNER_PLL_TDQE3: /*Bitmask corresponds to FL bit here*/
        case TUNER_PLL_DCF8783: /*reading whole register but comparing 7th bit*/
        case TUNER_PLL_DCT70700:
        case TUNER_PLL_TDCHG:
        case TUNER_PLL_TDCJG:
        case TUNER_PLL_TDCGG:
            BitMask = 0x40;
            SubAddr = 0x00;
            OpType  = STTUNER_IO_READ;
            break;
        
        case TUNER_PLL_DCT7045:
        case TUNER_PLL_DCT7045EVAL:
        	BitMask = 0x7F; /*compare 7bits from status register value*/
            SubAddr = 0x00;
            OpType  = STTUNER_IO_READ;
            break;
        
         default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

    /* Read 'locked' status, missing a read? */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, OpType, SubAddr, &RxStatus, 1, TCDRV_IO_TIMEOUT*100);
  }
  #endif
  
     #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, SubAddr, 1, &RxStatus);
  }
  #endif
  
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Can't access to FrontEnd (Read Status)\n", identity));
#endif
    }

    /* Check status bits */
    if ( (RxStatus & BitMask) != 0) *Locked = TRUE;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s Locked: ", identity));
    if (*Locked == TRUE)
    {
        STTBX_Print(("Yes\n"));
    }
    else
    {
        STTBX_Print(("no\n"));
    }
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_SetBandWidth()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_SetBandWidth(TUNER_Handle_t Handle, U32 BandWidth, U32 *NewBandWidth)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_SetBandWidth()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 index = 1;
    TCDRV_InstanceData_t *Instance;

    Instance = TCDRV_GetInstFromHandle(Handle);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s BandWidth %u\n", identity, BandWidth));
#endif
    /* Find closest (largest) bandwidth to requested value */
    while( (Instance->BandWidth[index] != 0) && (BandWidth > Instance->BandWidth[index]) )
    {
        index++;
    }

    /* Check whether or not we found a valid band (i.e. no zero) */
    if (Instance->BandWidth[index] == 0)
    {
      index = 0;  /* Use default band */
    }

    /* Update status */
    Instance->Status.Bandwidth = Instance->BandWidth[index];
    *NewBandWidth              = Instance->BandWidth[index];

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s NewBandWidth=%u\n", identity, *NewBandWidth));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_ioctl()

Description:
    access device specific low-level functions

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_ioctl(TUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status)

{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_ioctl()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    TCDRV_InstanceData_t *Instance;
    #if defined(STTUNER_DRV_CAB_STV0297E)
    U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif

    Instance = TCDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s tuner driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
#endif

    /* ---------- select what to do ---------- */
    switch(Function){

        case STTUNER_IOCTL_RAWIO: /* raw I/O to device */
    
      #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
      if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
           {
                Error =  STTUNER_IOARCH_ReadWrite( Instance->IOHandle,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Operation,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->SubAddr,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Data,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize,
                                                 ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout );
           }
      #endif
            
            
     #if defined(STTUNER_DRV_CAB_STV0297E)
      SubAddress = (U8)(((CABIOCTL_IOARCH_Params_t *)InParams)->SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(((CABIOCTL_IOARCH_Params_t *)InParams)->Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((CABIOCTL_IOARCH_Params_t *)InParams)->Data,((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((CABIOCTL_IOARCH_Params_t *)InParams)->Data,((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), ((CABIOCTL_IOARCH_Params_t *)InParams)->Data, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), ((CABIOCTL_IOARCH_Params_t *)InParams)->Data, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize+1, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), ((CABIOCTL_IOARCH_Params_t *)InParams)->Data, ((CABIOCTL_IOARCH_Params_t *)InParams)->TransferSize, ((CABIOCTL_IOARCH_Params_t *)InParams)->Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
     #endif
     
     if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
            }
            break;

       default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s function %d not found\n", identity, Function));
#endif
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }



#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s function %d called\n", identity, Function));
#endif
    return(Error);
}



/* ----------------------------------------------------------------------------
Name: tuner_tdrv_ioaccess()

Description:
    no passthru for a tuner.
Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t tuner_tdrv_ioaccess(TUNER_Handle_t Handle, IOARCH_Handle_t IOHandle,STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 *Data, U32 TransferSize, U32 Timeout)

{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c tuner_tdrv_ioaccess()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    IOARCH_Handle_t MyIOHandle;
    TCDRV_InstanceData_t *Instance;
     #if defined(STTUNER_DRV_CAB_STV0297E)
     U8 Buffer[30];
    U32 Size;
    U8  SubAddress;
    #endif
    #ifdef STTUNER_DEBUG_MODULE_SHARED_TUNSHDRV
        STTBX_Print(("%s tuner driver instance handle=%d\n", identity, Handle));
        STTBX_Print(("%s                     function=%d\n", identity, Function));
        STTBX_Print(("%s                   I/O handle=%d\n", identity, Instance->IOHandle));
    #endif

    Instance = TCDRV_GetInstFromHandle(Handle);

    if(Installed == FALSE)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s fail driver not installed\n", identity));
#endif
        return(STTUNER_ERROR_INITSTATE);
    }

    MyIOHandle = Instance->IOHandle;

        /* if STTUNER_IOARCH_MAX_HANDLES then passthrough function required using our device handle/address */
    if (IOHandle == STTUNER_IOARCH_MAX_HANDLES)
    {
        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
        Error = STTUNER_IOARCH_ReadWrite(MyIOHandle, Operation, SubAddr, Data, TransferSize, Timeout);
    }
    #endif
    
     #if defined(STTUNER_DRV_CAB_STV0297E)
     SubAddress = (U8)(SubAddr & 0xFF);
					      if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)  
					     { 
 					       	switch(Operation)
 					  	       {
							  	   case STTUNER_IO_SA_READ_NS:
							            Error = STI2C_WriteNoStop(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size);
				                       Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
							
							     case STTUNER_IO_SA_READ:
							           Error = STI2C_Write(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev),&SubAddress, 1, Timeout, &Size); /* fix for cable (297 chip) */
							            /* fallthrough (no break;) */
							     case STTUNER_IO_READ:
							           Error |= STI2C_Read(I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data,TransferSize, Timeout, &Size);
							            break;
				                case STTUNER_IO_SA_WRITE_NS:
							            Buffer[0] = SubAddress;
							            memcpy( (Buffer + 1), Data, TransferSize);
							            Error = STI2C_WriteNoStop(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_SA_WRITE:				         
							           Buffer[0] = SubAddress;
								        memcpy( (Buffer + 1), Data, TransferSize);
								        Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Buffer, TransferSize+1, Timeout, &Size);
							            break;
							     case STTUNER_IO_WRITE:
							            Error = STI2C_Write(  I2C_HANDLE(&IOARCH_Handle[Instance->IOHandle].ExtDev), Data, TransferSize, Timeout, &Size);
							            break;
							     default:
							            Error = STTUNER_ERROR_ID;
							            break;
 				   
 				               }
				        }
     #endif
    

        if (Error != ST_NO_ERROR)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        }
    }
    else    /* repeater */
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED; /* not supported for this tuner */
    }


#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s called\n", identity));
#endif
    return(Error);
}



/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */

TCDRV_InstanceData_t *TCDRV_GetInstFromHandle(TUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_GetInstFromHandle()";
#endif
    TCDRV_InstanceData_t *Instance;

    Instance = (TCDRV_InstanceData_t *)Handle;
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s block at 0x%08x\n", identity, Instance));
#endif

    return(Instance);
}


static U32 TCDRV_TunerSetNbSteps(TCDRV_InstanceData_t *Instance, U32 Steps)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_TunerSetNbSteps()";
#endif

    switch (Instance->PLLType)
    {
        case TUNER_PLL_TDBE1:
        case TUNER_PLL_TDBE2:
        case TUNER_PLL_TDDE1:
        case TUNER_PLL_SP5730:
        case TUNER_PLL_DCT7040:
        case TUNER_PLL_DCT7050:
        case TUNER_PLL_DCT7710:
        case TUNER_PLL_DCF8710:
        case TUNER_PLL_DCF8720:
        case TUNER_PLL_MACOETA50DR:
        case TUNER_PLL_CD1516LI:
        case TUNER_PLL_SHARPXX:
        case TUNER_PLL_TDBE1X016A:
        case TUNER_PLL_TDBE1X601:
        case TUNER_PLL_TDEE4X012A:
        case TUNER_PLL_DCT7045:
        case TUNER_PLL_TDQE3:
        case TUNER_PLL_DCF8783:
        case TUNER_PLL_DCT7045EVAL:
        case TUNER_PLL_DCT70700:
        case TUNER_PLL_TDCHG:
        case TUNER_PLL_TDCJG:
        case TUNER_PLL_TDCGG:
            Instance->IOBuffer[0] = (U8)((Steps & 0x00007F00) >> 8);
            Instance->IOBuffer[1] = (U8) (Steps & 0x000000FF);
            break;
#if defined (STTUNER_DRV_CAB_TUN_MT2030) ||defined (STTUNER_DRV_CAB_TUN_MT2040)||defined (STTUNER_DRV_CAB_TUN_MT2050)||defined (STTUNER_DRV_CAB_TUN_MT2060)
        case TUNER_PLL_MT2030:
        case TUNER_PLL_MT2040:
        case TUNER_PLL_MT2050:
        case TUNER_PLL_MT2060:
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223            
        case TUNER_PLL_DF1CS1223:
            Instance->IOBuffer[0] = (U8)((Steps & 0x00007F00) >> 8);
            Instance->IOBuffer[1] = (U8) (Steps & 0x000000FF);
            Instance->IOBuffer[2]|= (U8)((Steps & 0x00018000) >> 10);
            Instance->IOBuffer[2]&= 0x9F;
            break;
#endif
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

    return(Steps);
}


static U32 TCDRV_TunerGetChargePump(TCDRV_InstanceData_t *Instance)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_TunerGetChargePump()";
#endif
    U32 ChargePump=0;

    switch (Instance->PLLType)
    {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)||defined (STTUNER_DRV_CAB_TUN_SP5730)
        case TUNER_PLL_TDBE1:
        case TUNER_PLL_TDBE2:
        case TUNER_PLL_TDDE1:        
        case TUNER_PLL_SP5730:
            ChargePump = (Instance->IOBuffer[3])&0x80 ;
            break;
#endif
#if defined ( STTUNER_DRV_CAB_TUN_SHARPXX)||defined (STTUNER_DRV_CAB_TUN_CD1516LI)
       case TUNER_PLL_SHARPXX:
        case TUNER_PLL_CD1516LI:
            ChargePump = (Instance->IOBuffer[2])&0x40 ;
            break;
#endif
#if defined ( STTUNER_DRV_CAB_TUN_MT2030)||defined (STTUNER_DRV_CAB_TUN_MT2040) ||defined (STTUNER_DRV_CAB_TUN_MT2050)||defined (STTUNER_DRV_CAB_TUN_MT2060)  
        case TUNER_PLL_MT2030:
        case TUNER_PLL_MT2040:
        case TUNER_PLL_MT2050:
        case TUNER_PLL_MT2060:
        case TUNER_PLL_DF1CS1223:
            break;
#endif
#if defined (STTUNER_DRV_CAB_TUN_DCT7040) || defined (STTUNER_DRV_CAB_TUN_DCT7050) || defined (STTUNER_DRV_CAB_TUN_DCT7045) || defined(STTUNER_DRV_CAB_TUN_TDQE3) || defined(STTUNER_DRV_CAB_TUN_DCF8783) || defined(STTUNER_DRV_CAB_TUN_DCT7045EVAL) || defined(STTUNER_DRV_CAB_TUN_DCT70700) || defined(STTUNER_DRV_CAB_TUN_TDCHG) || defined(STTUNER_DRV_CAB_TUN_TDCJG) || defined(STTUNER_DRV_CAB_TUN_TDCGG)
        case TUNER_PLL_DCT7040:
        case TUNER_PLL_DCT7050:
        case TUNER_PLL_DCT7045:
        case TUNER_PLL_DCF8783:
        case TUNER_PLL_DCT7045EVAL:
        case TUNER_PLL_TDCHG:
        case TUNER_PLL_TDCJG:
        case TUNER_PLL_TDCGG:
            ChargePump = ( (Instance->IOBuffer[2])&0x40)>>6 ;
            break;
        
        case TUNER_PLL_TDQE3:
        	ChargePump = ( (Instance->IOBuffer[3])&0xC0)>>6 ; /*CTRL2[6,7] bits of ctrl2 register*/
        	break;
        
        case TUNER_PLL_DCT70700:
        	ChargePump = ( (Instance->IOBuffer[3])&0xC0)>>6 ; /*BB[6,7] bits of BB register*/
        	break;
#endif

        case TUNER_PLL_DCT7710:
        case TUNER_PLL_DCF8710:
        case TUNER_PLL_DCF8720:
            break;
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR   
        case TUNER_PLL_MACOETA50DR:
          ChargePump = (Instance->IOBuffer[2])&0x40 ;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case TUNER_PLL_TDBE1X016A:
            ChargePump = (Instance->IOBuffer[3])&0x80 ;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case TUNER_PLL_TDBE1X601:
            ChargePump = (Instance->IOBuffer[2])&0x40 ;
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case TUNER_PLL_TDEE4X012A:
            ChargePump = (Instance->IOBuffer[3])&0x80 ;
	        if ( (Instance->Status.Frequency/1000) >= 822000)
	          ChargePump = 1;
            break;
#endif
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

    return(ChargePump);
}


static void TCDRV_TunerSetChargePump(TCDRV_InstanceData_t *Instance, U32 ChargePump)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_TunerSetNbSteps()";
#endif
    int Frequency;

    Frequency = Instance->Status.Frequency/1000;
    switch (Instance->PLLType)
    {
#ifdef STTUNER_DRV_CAB_TUN_SP5730
        case TUNER_PLL_SP5730:
            Instance->IOBuffer[2] = 0x8E ;
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x80;
            }
            else
            {
                Instance->IOBuffer[3] &= 0x7F;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCF8710
		case TUNER_PLL_DCF8720:
            Instance->IOBuffer[2] = 0x8e ;
            break;
#endif
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_DCF8710)       
        case TUNER_PLL_DCF8710:
        case TUNER_PLL_TDBE1:
        case TUNER_PLL_TDBE2:
            Instance->IOBuffer[2] = 0x85 ;
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x80;
            }
            else
            {
                Instance->IOBuffer[3] &= 0x7F;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDDE1
        case TUNER_PLL_TDDE1:
            Instance->IOBuffer[2] = ( (ChargePump == 1)? 0xCE:0x8E );
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case TUNER_PLL_CD1516LI:
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] = 0xCE;
            }
            else
            {
                Instance->IOBuffer[3] = 0x8E;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX
        case TUNER_PLL_SHARPXX:
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x40;
            }
            else
            {
                Instance->IOBuffer[3] &= 0xBF;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A
        case TUNER_PLL_TDBE1X016A:
            Instance->IOBuffer[2] = 0x85;
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x80;
            }
            else
            {
                Instance->IOBuffer[3] &= 0x7F;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case TUNER_PLL_TDBE1X601:
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x40;
            }
            else
            {
                Instance->IOBuffer[3] &= 0xBF;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A
        case TUNER_PLL_TDEE4X012A:
            Instance->IOBuffer[2] = 0x85;
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x80;
            }
            else
            {
                Instance->IOBuffer[3] &= 0x7F;
            }
            break;
#endif
        case TUNER_PLL_MT2030:
        case TUNER_PLL_MT2040:
        case TUNER_PLL_MT2050:
        case TUNER_PLL_MT2060:
        case TUNER_PLL_DF1CS1223:
            break;
#if defined (STTUNER_DRV_CAB_TUN_DCT7040)|| defined(STTUNER_DRV_CAB_TUN_DCT7050)
        case TUNER_PLL_DCT7040:
        case TUNER_PLL_DCT7050:
            if(ChargePump == 1)
            {
                Instance->IOBuffer[3] |= 0x40;
            }
            else
            {
                Instance->IOBuffer[3] &= 0xBF;
            }
            break;
#endif

#if defined (STTUNER_DRV_CAB_TUN_DCT7045)|| defined(STTUNER_DRV_CAB_TUN_TDQE3) || defined(STTUNER_DRV_CAB_TUN_DCF8783)|| defined(STTUNER_DRV_CAB_TUN_DCT7045EVAL) || defined(STTUNER_DRV_CAB_TUN_DCT70700) || defined(STTUNER_DRV_CAB_TUN_TDCHG) || defined(STTUNER_DRV_CAB_TUN_TDCJG) || defined(STTUNER_DRV_CAB_TUN_TDCGG)
        case TUNER_PLL_DCT7045:
        case TUNER_PLL_DCF8783:
        case TUNER_PLL_DCT7045EVAL:
        case TUNER_PLL_TDCHG:
        case TUNER_PLL_TDCJG:
        case TUNER_PLL_TDCGG:
            if(ChargePump == 1)
            {
                Instance->IOBuffer[2] |= 0x40;
            }
            else
            {
                Instance->IOBuffer[2] &= 0xBF;
            }
            break;
        
        case TUNER_PLL_TDQE3:
        case TUNER_PLL_DCT70700:
        	if(ChargePump == 1) /*01*/
            {
                Instance->IOBuffer[3] &= 0x7F; /*0*/
                Instance->IOBuffer[3] |= 0x40; /*1*/
            }
            else if(ChargePump == 2) /*10*/
            {
                Instance->IOBuffer[3] |= 0x80; /*1*/
                Instance->IOBuffer[3] &= 0xBF; /*0*/
            }
            else if(ChargePump == 3) /*11*/
            {
                Instance->IOBuffer[3] |= 0xC0; /*11*/
            }
            else
            {
                Instance->IOBuffer[3] &= 0x3F; /*00*/
            }
            break;
#endif
            
        case TUNER_PLL_DCT7710:
            break;
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case TUNER_PLL_MACOETA50DR:
            if((Frequency <= 403)||((Frequency >= 435)&&(Frequency <= 762)))
            {
                Instance->IOBuffer[3] &= 0xBF;
            }
            else
            {
                Instance->IOBuffer[3] |= 0x40;
            }
            break;
#endif
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

}


static U32 TCDRV_TunerInitBand(TCDRV_InstanceData_t *Instance, U32 Frequency)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_TunerInitBand()";
#endif

    U32 TunerBand=0;

    switch (Instance->PLLType)
    {
#if defined( STTUNER_DRV_CAB_TUN_TDBE1) || defined(STTUNER_DRV_CAB_TUN_TDBE2) || defined (STTUNER_DRV_CAB_TUN_TDDE1)
        case TUNER_PLL_TDBE1:
        case TUNER_PLL_TDBE2:
        case TUNER_PLL_TDDE1:
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x1;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x2;
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x8;
            }
            break;
#endif

#if defined( STTUNER_DRV_CAB_TUN_DCT7045) || defined(STTUNER_DRV_CAB_TUN_TDQE3) || defined (STTUNER_DRV_CAB_TUN_DCF8783) || defined(STTUNER_DRV_CAB_TUN_DCT7045EVAL) || defined(STTUNER_DRV_CAB_TUN_DCT70700) || defined(STTUNER_DRV_CAB_TUN_TDCHG) || defined(STTUNER_DRV_CAB_TUN_TDCJG) || defined(STTUNER_DRV_CAB_TUN_TDCGG)
        case TUNER_PLL_DCT7045:
        case TUNER_PLL_DCT7045EVAL:
        	
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x2;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x1;
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x3;
            }
            /*For chargepump*/
            Instance->IOBuffer[2] |= 0x40;
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
            
        case TUNER_PLL_TDQE3:
        	
        	TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] &= 0xF0; /*xxxx0000*/
                /*For chargepump->*/
                Instance->IOBuffer[3] &= 0x7F; /*0xxxxxxx*/
                Instance->IOBuffer[3] |= 0x40; /*01xxxxxx*/
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] &= 0xF2; /*xxxx00?0*/
                Instance->IOBuffer[3] |= 0x02; /*xxxx0010*/
                /*For chargepump->*/
                Instance->IOBuffer[3] &= 0xBF; /*x0xxxxxx*/
                Instance->IOBuffer[3] |= 0x80; /*10xxxxxx*/
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] &= 0xFA; /*xxxx?0?0*/
                Instance->IOBuffer[3] |= 0x0A; /*xxxx1010*/
                /*For chargepump->*/
                Instance->IOBuffer[3] &= 0xBF; /*x0xxxxxx*/
                Instance->IOBuffer[3] |= 0x80; /*10xxxxxx*/
            }
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
        
        case TUNER_PLL_DCF8783:
        	
        	TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x01; /*00000001*/
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x02; /*00000010*/
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x0A; /*00001010*/
            }
            /*For chargepump*/
            Instance->IOBuffer[2] |= 0x40;
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
            
        case TUNER_PLL_DCT70700:
        	
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] &= 0xF1; /*xxxx000?*/
                Instance->IOBuffer[3] |= 0x01; /*xxxx0001*/
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] &= 0xF6; /*xxxx0??0*/
                Instance->IOBuffer[3] |= 0x06; /*xxxx0110*/
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] &= 0xFC; /*xxxx??00*/
                Instance->IOBuffer[3] |= 0x0C; /*xxxx1100*/
            }
            
            /*For chargepump->*/
            Instance->IOBuffer[3] &= 0x7F; /*0xxxxxxx*/
            Instance->IOBuffer[3] |= 0x40; /*01xxxxxx*/
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
            
        case TUNER_PLL_TDCHG:
        	
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x1;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x1;
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x4;
            }
            /*For chargepump*/
            Instance->IOBuffer[2] |= 0x40;
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
                        
        case TUNER_PLL_TDCJG:
        	
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x1;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x1;
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x4;
            }
            /*For chargepump*/
            Instance->IOBuffer[2] |= 0x40;
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
            
                                    
        case TUNER_PLL_TDCGG:
        	
            TunerBand = 3;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
                Instance->IOBuffer[3] = 0x1;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
                Instance->IOBuffer[3] = 0x1;
            }
            else
            {
                TunerBand = 2;
                Instance->IOBuffer[3] = 0x4;
            }
            /*For chargepump*/
            Instance->IOBuffer[2] |= 0x40;
            tuner_tdrv_DelayInMilliSec(TCDRV_CP_WAITING_TIME);
            break;
#endif

#ifdef STTUNER_DRV_CAB_TUN_SP5730
        case TUNER_PLL_SP5730:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            Instance->IOBuffer[3] &= 0xF0; /* band digits clear */
            /**/
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x08;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x04;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            break;
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) || defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
        case TUNER_PLL_MT2030:
        case TUNER_PLL_MT2040:
        case TUNER_PLL_MT2050:
        case TUNER_PLL_MT2060:
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7040
        case TUNER_PLL_DCT7040:
            TunerBand = 0;  /* no bands for this tuner */
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7050
        case TUNER_PLL_DCT7050:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            Instance->IOBuffer[3] &= 0x00; /* band digits clear */
            /**/
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x02;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x04;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DCT7710
        case TUNER_PLL_DCT7710:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            if(TunerBand == 0)
            {
                if(Frequency < 130000L)
                {
                    Instance->IOBuffer[3] = 0x05;
                }
                else if(Frequency < 140000L)
                {
                    Instance->IOBuffer[3] = 0x45;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x85;
                }
            }
            else if(TunerBand == 1)
            {
                if(Frequency < 350000L)
                {
                    Instance->IOBuffer[3] = 0x06;
                }
                else if(Frequency < 400000L)
                {
                    Instance->IOBuffer[3] = 0x46;
                }
                else if(Frequency < 420000L)
                {
                    Instance->IOBuffer[3] = 0x86;
                }
                else
                {
                    Instance->IOBuffer[3] = 0xC6;
                }
            }
            else if(TunerBand == 2)
            {
                if(Frequency < 660000L)
                {
                    Instance->IOBuffer[3] = 0x03;
                }
                else if(Frequency < 790000L)
                {
                    Instance->IOBuffer[3] = 0x43;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x83;
                }
            }
            break;
#endif
#if defined( STTUNER_DRV_CAB_TUN_DCF8710) || defined(STTUNER_DRV_CAB_TUN_DCF8720)
        case TUNER_PLL_DCF8710:
        case TUNER_PLL_DCF8720:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            if(TunerBand == 0)
            {
                if(Frequency < 133000L)
                {
                    Instance->IOBuffer[3] = 0x05;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x45;
                }
            }
            else if(TunerBand == 1)
            {
                if(Frequency < 350000L)
                {
                    Instance->IOBuffer[3] = 0x06;
                }
                else if(Frequency < 398000L)
                {
                    Instance->IOBuffer[3] = 0x46;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x86;
                }
            }
            else if(TunerBand == 2)
            {
                if(Frequency < 660000L)
                {
                    Instance->IOBuffer[3] = 0x03;
                }
                else if(Frequency < 796000L)
                {
                    Instance->IOBuffer[3] = 0x43;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x83;
                }
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_MACOETA50DR
        case TUNER_PLL_MACOETA50DR:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            Instance->IOBuffer[3] &= 0x00; /* band digits clear */
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x02;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x08;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_CD1516LI
        case TUNER_PLL_CD1516LI:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] = 0xA1;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3] = 0x92;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] = 0x34;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_DF1CS1223
        case TUNER_PLL_DF1CS1223:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            if(TunerBand == 0)
            {
                if(Frequency < 107000L)
                {
                    Instance->IOBuffer[3] = 0x0A;
                }
                else
                {
                    Instance->IOBuffer[3] = 0x02;
                }
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3] = 0x01;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] = 0x04;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_SHARPXX   
        case TUNER_PLL_SHARPXX:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            Instance->IOBuffer[3] &= 0xF8; /* band digits clear */
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x04;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x02;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X016A    
        case TUNER_PLL_TDBE1X016A:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            Instance->IOBuffer[3] &= 0xF0; /* band digits clear */
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x08;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x04;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            break;
#endif
#ifdef STTUNER_DRV_CAB_TUN_TDBE1X601
        case TUNER_PLL_TDBE1X601:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            Instance->IOBuffer[3] &= 0xF0; /* band digits clear */
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x02;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x08;
            }
            break;
#endif 
#ifdef STTUNER_DRV_CAB_TUN_TDEE4X012A  
        case TUNER_PLL_TDEE4X012A:
            /* Init Band */
            TunerBand = 2;
            if(Frequency < Instance->TunerLowBand)
            {
                TunerBand = 0;
            }
            else if(Frequency < Instance->TunerMidBand)
            {
                TunerBand = 1;
            }
            /**/
            Instance->IOBuffer[3] &= 0xF0; /* band digits clear */
            if(TunerBand == 0)
            {
                Instance->IOBuffer[3] |= 0x01;
            }
            else if(TunerBand == 1)
            {
                Instance->IOBuffer[3]  |= 0x02;
            }
            else if(TunerBand == 2)
            {
                Instance->IOBuffer[3] |= 0x08;
            }
            break;
#endif
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s WARNING - no PLL specified\n", identity));
#endif
            break;
    }

    return(TunerBand);
}

static U32 TCDRV_CalculateStepsAndFrequency(TCDRV_InstanceData_t *Instance, U32 Frequency)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_CalculateStepsAndFrequency()";
#endif
    U32     TunerStep=0;
    U32     NbSteps;
	U32     NbStepsLow;
	U32     NbStepsHigh;
    U32     TunerFrequency;
	U32     TunerFrequencyLow;
	U32     TunerFrequencyHigh;

    /* Calculate number of steps for nominated frequency */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s Frequency %u KHz IntermediateFrequency %u KHz TunerStep %u Hz Bandwidth %u KHz\n",
            identity,
            Frequency,
            Instance->Status.IntermediateFrequency,
            Instance->Status.TunerStep,
            Instance->Status.Bandwidth
            ));
#endif
    TunerStep = Instance->Status.TunerStep;
    NbStepsLow  = ((Instance->FreqFactor)*Frequency) + ((Instance->FreqFactor)*(Instance->Status.IntermediateFrequency));
    if (TunerStep != 0)
    {
        NbStepsLow  /= TunerStep;
    }
    else
    {
        NbStepsLow  = 0;
    }
    NbStepsHigh = NbStepsLow + 1;

    /* Calculate Frequency */
    TunerFrequencyLow = NbStepsLow*TunerStep -(Instance->FreqFactor)*(Instance->Status.IntermediateFrequency);
    TunerFrequencyHigh = NbStepsHigh*TunerStep -(Instance->FreqFactor)*(Instance->Status.IntermediateFrequency);

    if((TunerFrequencyHigh - ((Instance->FreqFactor)*Frequency)) < (((Instance->FreqFactor)*Frequency) - TunerFrequencyLow))
    {
        TunerFrequency = TunerFrequencyHigh ;
        NbSteps = NbStepsHigh ;
    }
    else
    {
        TunerFrequency = TunerFrequencyLow ;
        NbSteps = NbStepsLow ;
    }
    Instance->Status.Frequency = (TunerFrequency)/(Instance->FreqFactor);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    STTBX_Print(("%s TunerFrequencyLow %u TunerFrequencyHigh %u ==> TunerFrequency %u\n",
            identity,
            TunerFrequencyLow,
            TunerFrequencyHigh,
            TunerFrequency
            ));
    STTBX_Print(("%s NbStepsLow %u NbStepsHigh %u ==> NbSteps %u\n",
            identity,
            NbStepsLow,
            NbStepsHigh,
            NbSteps
            ));
#endif

    return(NbSteps);
}

/*


	Name: TCDRV_MT_GetTunerLock

	Description:	Read PLL lock status from the MT-2030.

	Parameters:		None.

	Returns:		1 if the PLL is locked, 0 otherwise.

	Revision History:

	 SCR      Date      Author  Description
	-------------------------------------------------------------------------
   N/A   06-07-2001    DAD    Modified from tun0297.c


*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int TCDRV_MT2040_GetTunerLockCached(TCDRV_InstanceData_t *Instance)
{
    return ((Instance->IOBuffer[TCDRV_MT2040_STATUS]) & 0x06) == 0x06 ? 1 : 0;
}

#endif
#if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static int TCDRV_MT2050_GetTunerLockCached(TCDRV_InstanceData_t *Instance)
{
    return ((Instance->IOBuffer[TCDRV_MT2050_LO_STATUS]) & 0x88) == 0x88 ? 1 : 0;
}
#endif
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int TCDRV_MT2040_GetTunerLock(TCDRV_InstanceData_t *Instance)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_MT2040_GetTunerLock()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_STATUS, &(Instance->IOBuffer[TCDRV_MT2040_STATUS]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif	
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_STATUS, 1, &(Instance->IOBuffer[TCDRV_MT2040_STATUS]));
  }
  #endif
  

    if ( Error != ST_NO_ERROR )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to Tuner MT2040\n", identity));
#endif
        return 0;
    }
    return (TCDRV_MT2040_GetTunerLockCached(Instance));
}
#endif
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static int TCDRV_MT2050_GetTunerLock(TCDRV_InstanceData_t *Instance)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
   const char *identity = "STTUNER tcdrv.c TCDRV_MT2050_GetTunerLock()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2050_LO_STATUS, &(Instance->IOBuffer[TCDRV_MT2050_LO_STATUS]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2050_LO_STATUS, 1, &(Instance->IOBuffer[TCDRV_MT2050_LO_STATUS]));
  }
  #endif
  

    if ( Error != ST_NO_ERROR )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to Tuner MT2050\n", identity));
#endif
        return 0;
    }
    return (TCDRV_MT2050_GetTunerLockCached(Instance));
}
#endif
/*
    Name: TCDRV_MT_CheckLOLock
	Description:	Checks to see if LO1 and LO2 are locked.
	Parameters:		None.
	Returns:		1 if the tuner is locked.
					0 otherwise.

    Revision History:
	 SCR      Date      Author  Description
	-------------------------------------------------------------------------
   N/A   06-07-2001    DAD    Original, modified from mt203x.cpp
*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int TCDRV_MT_CheckLOLock(TCDRV_InstanceData_t *Instance)
{
    int i;

    /* Try at most 10 times */
    for (i=0; i<10; ++i)
    {
        tuner_tdrv_DelayInMilliSec(1);
        if (TCDRV_MT2040_GetTunerLock(Instance))
        {
            return 1;
        }
    }
    return 0;
}
#endif

/*
	Name: TCDRV_MT_SelectVCO
	Description:	Select the VCO to be used based on the 1st LO frequency.
	Parameters:		fLO1 -	1st LO frequency selected by the tuning
					algorithm.
	Returns:		the VCO selection

	Revision History:
	 SCR      Date      Author  Description
	-------------------------------------------------------------------------
   N/A   06-07-2001    DAD    Original, modified from mt203x.cpp
*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static unsigned char TCDRV_MT_SelectVCO(long fLO1)
{
    unsigned char Sel;

    /*
    	Define the LO1 frequency breakover points for VCO selection.
    	These are used when the TAD1 is read back to adjust the
    	VCO for better phase noise perfomance.

    	Define the correct VCO selection to use when the TAD1
    	will not be read back for fine tuning.

    	VCO		 LO1 frequency 	    SEL
    	------+------------------+------------------+-----
    	VCO 1	1890 -      MHz		 0
    	VCO 2	1720 - 1890 MHz		 1
    	VCO 3	1530 - 1720 MHz 	 2
    	VCO 4	1370 - 1530 MHz 	 3
    	VCO 5	   0 - 1370 MHz 	 4
    */

    for (Sel=0; Sel<5; ++Sel)
    {
        if (fLO1 > TCDRV_MT2040_VCO_FREQS[Sel]) break;
    }
    return Sel;
}
#endif
/*


	Name: TCDRV_MT_AdjustVCO

	Description:	Use LO1's Tune A/D to adjust the VCO selection for
					optimal phase noise performance.

	Parameters:		None.

	Returns:		0 if there were no errors.

	Revision History:

	 SCR      Date      Author  Description
	-------------------------------------------------------------------------
   N/A   06-07-2001    DAD    Original, modified from mt203x.cpp


*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int TCDRV_MT_AdjustVCO(TCDRV_InstanceData_t *Instance)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT_AdjustVCO()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int sel;

    sel = (Instance->IOBuffer[TCDRV_MT2040_LO1_2] & 0x70) >> 4;     /* SEL register field value */

    /* Read the TAD1 register field*/
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2040_TUNEA2D, &(Instance->IOBuffer[TCDRV_MT2040_TUNEA2D]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2040_TUNEA2D, 1, &(Instance->IOBuffer[TCDRV_MT2040_TUNEA2D]));
  }
  #endif
  
  

    if ( Error != ST_NO_ERROR )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(Error);
    }

    switch ((Instance->IOBuffer[TCDRV_MT2040_TUNEA2D]) & 7)               /* get TAD1 register field */
    {
        /* Desired readback value */
        case 0:
        case 1:
            break;

        /* Reduce VCO selection */
        case 2:
            if (sel > 0)
            {
                Instance->IOBuffer[TCDRV_MT2040_LO1_2] = ((Instance->IOBuffer[TCDRV_MT2040_LO1_2]) & 0x8F) | (--sel << 4);    /* reassemble LO1_2 register */
                #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
                Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO1_2, &(Instance->IOBuffer[TCDRV_MT2040_LO1_2]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif   
                #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
                Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_2, &(Instance->IOBuffer[TCDRV_MT2040_LO1_2]), 1);
    }
    #endif
    
    

                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                    STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
                    return(Error);
                }
                TCDRV_MT_CheckLOLock(Instance);                                     /* Check/delay for LO's to lock */
            }
            break;

        /* Increase VCO selection */
        case 5:
        case 6:
            if (++sel < 5)
            {
                Instance->IOBuffer[TCDRV_MT2040_LO1_2] = ((Instance->IOBuffer[TCDRV_MT2040_LO1_2]) & 0x8F) | (sel << 4);        /*  reassemble LO1_2 register */
                #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
                Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO1_2, &(Instance->IOBuffer[TCDRV_MT2040_LO1_2]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif	 
                #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
                Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_2, &(Instance->IOBuffer[TCDRV_MT2040_LO1_2]), 1);
    }
    #endif
    
    

                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                    STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
                    return(Error);
                }
                TCDRV_MT_CheckLOLock(Instance);                                     /* Check/delay for LO's to lock */
            }
            break;

        default:
            return -1;
    }

    return 0;
}
#endif
/*
	Name: TCDRV_MT_IsSpurInBand

	Description:	Checks to see if a spur will be present within the IF's
					bandwidth.

	Parameters:		None.

	Returns:		-1 if an LO spur would be present, otherwise 0.

	Revision History:

	 SCR      Date      Author  Description
	-------------------------------------------------------------------------
   N/A   06-07-2001    DAD    Original, modified from mt203x.cpp
*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static int TCDRV_MT2040_IsSpurInBand(TCDRV_InstanceData_t *Instance, long fLO1, long fLO2, long Spur)
{
    int nLO1 = 1;       /*  LO1 multiplier */
    int nLO2;           /*  LO2 multiplier */
    long fSpur;         /*  calculated LO spur frequency */

    /*
    --- Start out with fLO1 - 2fLO2 - TunerIF
    --- Add LO1 harmonics until Spur is reached.
    */
    do
    {
        nLO2 = -nLO1;
        fSpur = nLO1 * (fLO1 - fLO2);

        /* Subtract LO2 harmonics until fSpur is below IF2 band pass*/
        do
        {
            --nLO2;
            fSpur -= fLO2;

            if (abs(abs((int)fSpur) - (int)(Instance->Status.IntermediateFrequency)) < ((Instance->BandWidth[0]) / 2))
            {
                return -1;
            }

        } while ((fSpur > fLO2 - (Instance->Status.IntermediateFrequency) - (Instance->BandWidth[0]))
            && (nLO2 > -Spur));
    } while (++nLO1 < Spur);

    return 0;
}
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static int TCDRV_MT2050_IsSpurInBand(TCDRV_InstanceData_t *Instance, long fLO1, long fLO2, long MAX_LOSpurHarmonic)
{
    int nLO1 = 1;
    int nLO2;
    long fSpur;
    long fIFBW;
    long fIFOut;
    long TunerStep;

    /* Convert all values from kHz to Hz */
    fIFBW  = 1000L * Instance->BandWidth[0];
    fIFOut = 1000L * Instance->Status.IntermediateFrequency;

    /* Convert all values from Hz to number of steps*/
    TunerStep = Instance->Status.TunerStep;
    fIFBW     = (fIFBW  + TunerStep/2) / TunerStep;
    fIFOut    = (fIFOut + TunerStep/2) / TunerStep;

    /*  Start out with fLO1 - 2*fLO2 - fIFOut                  */
    /*  Add LO1 harmonics until MAX_LOSpurHarmonic is reached. */
    do
    {
        nLO2 = -nLO1;
        fSpur = nLO1 * (fLO1 - fLO2);

        /*  Subtract LO2 harmonics until fSpur is below fIFOut band pass. */
        do
        {
            --nLO2;
            fSpur -= fLO2;

            /*
            ** If the spur is within the band width - quit looking.
            ** Compare 2x < BW vs. x < BW/2 for accuracy
            */
            if (2 * (abs(abs((int)fSpur) - (int)fIFOut)) < (int)fIFBW)
                return (1);

          /* To avoid losing a low order bit, compare 2x < BW vs. x < BW/2 */
        } while ( (2 * (fLO2 - fIFOut - (int)fSpur) < fIFBW)
               && (nLO2 > -MAX_LOSpurHarmonic) );

    } while (++nLO1 < MAX_LOSpurHarmonic);

    return (0);
}
#endif

/*


	Name: TCDRV_MT20x0_TunerSetFrequency

	Description:	Change the tuner's tuned frequency to _Freq.

	Parameters:		_Freq      - RF input center frequency (in kHz).

	Returns:		resulting tuned frequency, if the tune was successful.
                  0 otherwise


*/
#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static long TCDRV_MT2030_TunerSetFrequency(TCDRV_InstanceData_t *Instance, long _Freq)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT2030_TunerSetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int LOLocked;						/*	flag indicating LO status */
    long fLO1_Desired;					/*	ideal 1st LO frequency (in kHz)*/
    long LO1;							/*	1st LO register value */
    long LO2;							/*	2nd LO register value */
    long fLO1;							/*	1st LO frequency (in kHz)*/
    long fLO2;							/*	2nd LO frequency (in kHz)*/
    long FracN;							/*	NUM register value */
    long Sel;							/*	SEL register value */
    long nLO1Adjust;                    /*  spur check loop variable */
    int nLockCheck;                     /*  PLL lock loop counter */

    /*
    --- Calculate the number of FracN steps to make a TunerStep
    */
    unsigned long FracNStep;
    FracNStep = (Instance->Status.TunerStep) * 3780L;
    FracNStep /= TCDRV_MT2040_SRO_FREQ;
    FracNStep /= 1000L;

    /*
    --- Calculate fLO1 desired, LO1, fLO1 and fLO2 (desired)
    --- Calculate the desired LO1 frequency
    */
    fLO1_Desired = 1090000 + _Freq;

    /*
    --- LO1 value has a resolution of 5.25 MHz (TCDRV_MT_SRO_FREQ), so we'll
    --- round it to the closest value.
    */
    LO1 = (fLO1_Desired + (TCDRV_MT2040_SRO_FREQ / 2)) / TCDRV_MT2040_SRO_FREQ;

    /*
    --- Calculate the real (rounded to the nearest TCDRV_MT2040_SRO_FREQ) LO1 frequency
    */
    fLO1 = LO1 * TCDRV_MT2040_SRO_FREQ;
    fLO2 = fLO1 - (Instance->Status.IntermediateFrequency) - _Freq;

    /*
    --- Avoid Spurs
    ---
    --- Make three attempts at settings that have no spurs within the IF output
    --- bandwidth.
    */
    for (nLO1Adjust = 1; (nLO1Adjust < 4) && TCDRV_MT2040_IsSpurInBand(Instance, fLO1, fLO2, TCDRV_MT2030_MAX_LOSpurHarmonic); nLO1Adjust++)
    {
        LO1 += (fLO1_Desired > fLO1) ? nLO1Adjust : -nLO1Adjust;
        fLO1 = LO1 * TCDRV_MT2040_SRO_FREQ;
        fLO2 = fLO1 - (Instance->Status.IntermediateFrequency) - _Freq;
    }

    /*
    --- Select a VCO
    */
    Sel = TCDRV_MT_SelectVCO(fLO1);

    /*
    --- Calculate LO2 and FracN
    --- Calculate the LO2 value (rounded down)
    */
    LO2 = fLO2 / TCDRV_MT2040_SRO_FREQ;

    /*
    --- Calculate the fractional part of 5.25 MHz remaining in LO2
    --- and round to the nearest step size (50 or 62.5 KHz).
    */
    FracN  = 3780 * (fLO2 % TCDRV_MT2040_SRO_FREQ);
    FracN /= TCDRV_MT2040_SRO_FREQ;
    FracN  = (FracN + FracNStep / 2);

    /*
    --- Arrange the fields into register values
    --- Set LO1N register
    */
    Instance->IOBuffer[TCDRV_MT2040_LO1_1] = (U8)((LO1 >> 3) - 1);                          /*  reg[0x00] */

    /*
    --- Set VCO selection & LO1A register
    */
    Instance->IOBuffer[TCDRV_MT2040_LO1_2] = (U8)((Sel << 4) | (LO1 & 7));                  /*  reg[0x01] */

    /*
    --- Set LOGC to maximum values
    */
    Instance->IOBuffer[TCDRV_MT2040_LOGC] = 0x86;                                           /*  reg[0x02] */

    /*
    --- Set LO2N & LO2A values
    */
    Instance->IOBuffer[TCDRV_MT2040_LO2_1] = (U8)((LO2 << 5) | ((LO2 >> 3) - 1));           /*  reg[0x05] */

    /*
    --- Set the PKEN bit for RF input frequencies greater than TCDRV_MT_PKENRANGE
    */
    Instance->IOBuffer[TCDRV_MT2040_CONTROL_1] = (_Freq < TCDRV_MT2040_PKENRANGE) ? 0xE4 : 0xF4; /* reg[0x06] */

    /*
    --- Turn off charge pump
    */
    Instance->IOBuffer[TCDRV_MT2040_CONTROL_2] &= 0x0F;                                     /*  reg[0x07] */

    /*
    --- Set NUM registers, fine-tuning the LO2 frequency
    */
    Instance->IOBuffer[TCDRV_MT2040_LO2_2] = (U8)(FracN & 0xFF);                                /*  reg[0x0B] */
    Instance->IOBuffer[TCDRV_MT2040_LO2_3] = (U8)(0x80 | ((FracN >> 8) & 0xF));             /*  reg[0x0C] */

    /*
    --- Write out the computed register values
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 3, TCDRV_IO_TIMEOUT);
  }
  #endif	
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 3);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO2_1, &(Instance->IOBuffer[TCDRV_MT2040_LO2_1]), 3, TCDRV_IO_TIMEOUT);
  }
  #endif   
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO2_1, &(Instance->IOBuffer[TCDRV_MT2040_LO2_1]), 3);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO2_2, &(Instance->IOBuffer[TCDRV_MT2040_LO2_2]), 2, TCDRV_IO_TIMEOUT);
  }
  #endif 

    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO2_2, &(Instance->IOBuffer[TCDRV_MT2040_LO2_2]), 2);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    /*
    --- Check for LO's locking
    */
    nLockCheck = 0;
    do
    {
        TCDRV_MT_CheckLOLock(Instance);
        TCDRV_MT_AdjustVCO(Instance);

        /*
        --- If we still haven't locked, we need to try raising the SRO
        --- loop gain control voltage in case it has fallen too low.
        */
        LOLocked = TCDRV_MT2040_GetTunerLock(Instance);
        if (!LOLocked)
        {
            Instance->IOBuffer[TCDRV_MT2040_CONTROL_2] |= 0x80;         /*  Turn charge pump on */
            #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
            Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif	
            #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
            Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1);
    }
    #endif
    
    

            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
                break;
            }

            tuner_tdrv_DelayInMilliSec(TCDRV_MT2040_WAITING_TIME);

            Instance->IOBuffer[TCDRV_MT2040_CONTROL_2] &= 0x0F;         /*  Turn charge pump back off */
            #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
            Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1, TCDRV_IO_TIMEOUT);
    }
    #endif	
            #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
            Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_CONTROL_2, &(Instance->IOBuffer[TCDRV_MT2040_CONTROL_2]), 1);
    }
    #endif
    

            if (Error != ST_NO_ERROR)
            {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
                STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
                break;
            }

            LOLocked = TCDRV_MT2040_GetTunerLock(Instance);
        }
    } while ((!LOLocked) && (++nLockCheck < 3));

    if (!LOLocked)
        return 0;

        /*
        --- Reduce the LOGC to nominal values
    */
    Instance->IOBuffer[TCDRV_MT2040_LOGC] = 0x20;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif
  		 
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1);
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }
    else
    {
        return (fLO1 - fLO2 - (Instance->Status.IntermediateFrequency));
    }
}/* TCDRV_MT2030_TunerSetFrequency */
#endif

#if defined( STTUNER_DRV_CAB_TUN_MT2030) || defined(STTUNER_DRV_CAB_TUN_MT2040) 
static long TCDRV_MT2040_TunerSetFrequency(TCDRV_InstanceData_t *Instance, long _Freq)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT2040_TunerSetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int LOLocked;						/*	flag indicating LO status */
    long fLO1_Desired;					/*	ideal 1st LO frequency (in kHz)*/
    long LO1;							/*	1st LO register value */
    long LO2;							/*	2nd LO register value */
    long fLO1;							/*	1st LO frequency (in kHz)*/
    long fLO2;							/*	2nd LO frequency (in kHz)*/
    long FracN;							/*	NUM register value */
    long Sel;							/*	SEL register value */
    long nLO1Adjust;                    /*  spur check loop variable */
    int nLockCheck;                     /*  PLL lock loop counter */

    /*
    --- Calculate the number of FracN steps to make a TunerStep
    */
    unsigned long FracNStep;
    FracNStep = (Instance->Status.TunerStep) * 3780L;
    FracNStep /= TCDRV_MT2040_SRO_FREQ;
    FracNStep /= 1000L;

    /*
    --- Calculate fLO1 desired, LO1, fLO1 and fLO2 (desired)
    --- Calculate the desired LO1 frequency
    */
    fLO1_Desired = 1090000 + _Freq;

    /*
    --- LO1 value has a resolution of 5.25 MHz (TCDRV_MT_SRO_FREQ), so we'll
    --- round it to the closest value.
    */
    LO1 = (fLO1_Desired + (TCDRV_MT2040_SRO_FREQ / 2)) / TCDRV_MT2040_SRO_FREQ;

    /*
    --- Calculate the real (rounded to the nearest TCDRV_MT_SRO_FREQ) LO1 frequency
    */
    fLO1 = LO1 * TCDRV_MT2040_SRO_FREQ;
    fLO2 = fLO1 - (Instance->Status.IntermediateFrequency) - _Freq;

    /*
    --- Avoid Spurs
    ---
    --- Make three attempts at settings that have no spurs within the IF output
    --- bandwidth.
    */
    for (nLO1Adjust = 1; (nLO1Adjust < 4) && TCDRV_MT2040_IsSpurInBand(Instance, fLO1, fLO2, TCDRV_MT2040_MAX_LOSpurHarmonic); nLO1Adjust++)
    {
        LO1 += (fLO1_Desired > fLO1) ? nLO1Adjust : -nLO1Adjust;
        fLO1 = LO1 * TCDRV_MT2040_SRO_FREQ;
        fLO2 = fLO1 - (Instance->Status.IntermediateFrequency) - _Freq;
    }

    /*
    --- Select a VCO
    */
    Sel = TCDRV_MT_SelectVCO(fLO1);

    /*
    --- Calculate LO2 and FracN
    --- Calculate the LO2 value (rounded down)
    */
    LO2 = fLO2 / TCDRV_MT2040_SRO_FREQ;

    /*
    --- Calculate the fractional part of 5.25 MHz remaining in LO2
    --- and round to the nearest step size (50 or 62.5 KHz).
    */
    FracN  = 3780 * (fLO2 % TCDRV_MT2040_SRO_FREQ);
    FracN /= TCDRV_MT2040_SRO_FREQ;
    FracN  = (FracN + FracNStep / 2);

    /* If NUM rounded all the way to max value,				*/
	/* clear it and increment the integer portion by one.	  	*/
	if (FracN >= 3780)
	{
		FracN = 0;
		++LO2;
	}

    /*
    **  Offset NUM numerator by one
    */
    if (FracN > 0)
    {
        FracN++;
    }

    /*
    --- Arrange the fields into register values
    --- Set LO1N register
    */
    Instance->IOBuffer[TCDRV_MT2040_LO1_1] = (U8)((LO1 >> 3) - 1);                          /*  reg[0x00] */

    /*
    --- Set VCO selection & LO1A register
    */
    Instance->IOBuffer[TCDRV_MT2040_LO1_2] = (U8)((Sel << 4) | (LO1 & 7));                  /*  reg[0x01] */

    /*
    --- Set LOGC to maximum values
    */
    Instance->IOBuffer[TCDRV_MT2040_LOGC] = 0x86;                                           /*  reg[0x02] */

    /*
    --- Set LO2N & LO2A values
    */
    Instance->IOBuffer[TCDRV_MT2040_LO2_1] = (U8)((LO2 << 5) | ((LO2 >> 3) - 1));           /*  reg[0x05] */

    /*
    --- Set the PKEN bit for RF input frequencies greater than TCDRV_MT_PKENRANGE
    */
    Instance->IOBuffer[TCDRV_MT2040_CONTROL_1] = (_Freq < TCDRV_MT2040_PKENRANGE) ? 0xE4 : 0xF4; /* reg[0x06] */

    /*
    --- Turn off charge pump
    */
    /*Instance->IOBuffer[TCDRV_MT_CONTROL_2] &= 0x0F;*/									    /*	reg[0x07] */

    /*
    --- Set NUM registers, fine-tuning the LO2 frequency
    */
    Instance->IOBuffer[TCDRV_MT2040_LO2_2] = (U8)(FracN & 0xFF);                            /*  reg[0x0B] */
    Instance->IOBuffer[TCDRV_MT2040_LO2_3] = (U8)(0x80 | ((FracN >> 8) & 0xF));             /*  reg[0x0C] */

    /*
    --- Write out the computed register values
    */
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 3, TCDRV_IO_TIMEOUT);
  }
  #endif	 
   #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO1_1, &(Instance->IOBuffer[TCDRV_MT2040_LO1_1]), 3);
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO2_1, &(Instance->IOBuffer[TCDRV_MT2040_LO2_1]), 3, TCDRV_IO_TIMEOUT);
  }
  #endif   
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO2_1, &(Instance->IOBuffer[TCDRV_MT2040_LO2_1]), 3);
  }
  #endif
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LO2_2, &(Instance->IOBuffer[TCDRV_MT2040_LO2_2]), 2, TCDRV_IO_TIMEOUT);
  }
  #endif
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LO2_2, &(Instance->IOBuffer[TCDRV_MT2040_LO2_2]), 2);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    TCDRV_MT_AdjustVCO(Instance);

    /*
    --- Check for LO's locking
    */
    nLockCheck = 0;

    if (Sel < 2)
    {
        Instance->IOBuffer[TCDRV_MT2040_LOVCOC] |= 0x80;
    }
    else
    {
        Instance->IOBuffer[TCDRV_MT2040_LOVCOC] &= 0x7F;
    }

    /*  Set NUM enable value                                MT_Reg[0x04]     */
    Instance->IOBuffer[TCDRV_MT2040_LOVCOC] = (FracN == 0) ? Instance->IOBuffer[TCDRV_MT2040_LOVCOC] & 0xF8 : Instance->IOBuffer[TCDRV_MT2040_LOVCOC] | 0x07;

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LOVCOC, &(Instance->IOBuffer[TCDRV_MT2040_LOVCOC]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LOVCOC, &(Instance->IOBuffer[TCDRV_MT2040_LOVCOC]), 1);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    LOLocked = TCDRV_MT2040_GetTunerLock(Instance);

    if (!LOLocked)
        return 0;

    /*
    --- Reduce the LOGC to nominal values
    */
    Instance->IOBuffer[TCDRV_MT2040_LOGC] = 0x20;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1, TCDRV_IO_TIMEOUT);
  }
  #endif
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2040_LOGC, &(Instance->IOBuffer[TCDRV_MT2040_LOGC]), 1);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }
    else
    {
        return (fLO1 - fLO2 - (Instance->Status.IntermediateFrequency));
    }

} /* TCDRV_MT2040_TunerSetFrequency */
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static long TCDRV_MT2050_TunerSetFrequency(TCDRV_InstanceData_t *Instance, long _Freq)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT2050_TunerSetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    long f_IF1_BW;             /* First IF1 Bandwidth */
    int  LOLocked;             /*  flag indicating LO status */
    int  SpurFound;            /*  Flag for locating spurs         */
    long LO1;                  /*  1st LO register value */
    long Num1;                 /*  Numerator for LO1 reg. value    */
    long LO2;                  /*  2nd LO register value           */
    long Num2;                 /*  Numerator for LO2 reg. value    */
    long fLO1;                 /*  1st LO frequency (in kHz)*/
    long fLO2;                 /*  2nd LO frequency (in kHz)*/
    long zfLO1;                /*  Temp 1st LO frequency (in Hz)   */
    long zfLO2;                /*  Temp 2nd LO frequency (in Hz)   */
    long nLOAdjust;            /*  # of frequency adjustments      */
    long fAdjust;              /* Frequency adjustment size        */

    long f_in;                 /* RF input center frequency       */
    long f_out;                /* IF output center frequency      */
    long f_IFBW;               /* IF output bandwidth, plus guard */
    long TunerStep;
    long OneMeg;
    long PkEnRange;

#define IF1_CENTER  (1218000L)  /* Center of the IF1 filter (in kHz) */

    /* SRO frequency */
    long f_Ref;

    /* 1st IF filter Center */
    long f_IF1_Center;

    /* LO1 has 1 MHz step */
    const long ONE_MEG = 1000L;
    /*
    **  Quantize all input arguments to the nearest TUNE_STEP_SIZE
    */

    /* Convert all values from kHz to Hz */
    f_IF1_BW     = 1000L * TCDRV_MT2050_IF1_BW;
    f_Ref        = 1000L * TCDRV_MT2050_SRO_FREQ;
    PkEnRange    = 1000L * TCDRV_MT2050_PKENRANGE;
    f_IFBW       = 1000L * Instance->BandWidth[0];
    f_IF1_Center = 1000L * IF1_CENTER;
    f_in         = 1000L * _Freq;
    f_out        = 1000L * Instance->Status.IntermediateFrequency;

    /* Convert all values from Hz to number of steps*/
    TunerStep    = Instance->Status.TunerStep;
    f_IF1_BW     = (f_IF1_BW     + TunerStep/2) / TunerStep;
    f_Ref        = (f_Ref        + TunerStep/2) / TunerStep;
    PkEnRange    = (PkEnRange    + TunerStep/2) / TunerStep;
    f_IFBW       = (f_IFBW       + TunerStep/2) / TunerStep;
    f_IF1_Center = (f_IF1_Center + TunerStep/2) / TunerStep;
    f_in         = (f_in         + TunerStep/2) / TunerStep;
    f_out        = (f_out        + TunerStep/2) / TunerStep;
    OneMeg       = (1000000      + TunerStep/2) / TunerStep;

    /*
    **  Calculate frequency settings.  f_IF1_Center + f_in is the
    **  desired LO1 frequency, but LO1 only operates in steps of 1 MHz,
    **  so we have to round fLO1 to the nearest 1 MHz;
    */
    fLO1 = f_IF1_Center + f_in;
    fLO1 = OneMeg * ((fLO1 + OneMeg/2) / OneMeg);
    fLO2 = fLO1 - f_out - f_in;

    /*
    ** If there is an LO spur in this band, work outward until we find a
    ** spur-free frequency or run up against the 1st IF SAW band edge.
    ** Use temporary copies of fLO1 and fLO2 so that they will be unchanged
    ** if a spur-free setting is not found.  NOTE: If the input IF1 freq
    ** is NOT 1220 MHz, this search will hit one band edge before searching
    ** the entire bandwidth. This should not be a problem since there should
    ** be little reason to use a different IF1 value.
    */
    SpurFound = TCDRV_MT2050_IsSpurInBand(Instance, fLO1, fLO2, TCDRV_MT2050_MAX_LOSpurHarmonic);
    if (SpurFound)
    {
        nLOAdjust = 1;
        zfLO1 = fLO1;
        zfLO2 = fLO2;
        do
        {
            fAdjust = nLOAdjust * ONE_MEG;
            if (nLOAdjust % 2)  /* Apply any adjustments */
            {
                zfLO1 += fAdjust;
                zfLO2 += fAdjust;
            }
            else
            {
                zfLO1 -= fAdjust;
                zfLO2 -= fAdjust;
            }

            /*
            ** If our next try is outside the 1st IF bandpass, quit trying
            ** To avoid loosing a low order bit, compare 2x > BW vs. x > BW/2
            */
            if ((2 * (abs((int)zfLO1 - (int)f_in - (int)f_IF1_Center)) + (int)f_IFBW) > (int)f_IF1_BW)
                break;

            ++nLOAdjust;
            SpurFound = TCDRV_MT2050_IsSpurInBand(Instance, fLO1, fLO2, TCDRV_MT2050_MAX_LOSpurHarmonic);
        }
        while (SpurFound);

        /*
        ** Use the LO-spur free values found.  If the search went all the way to
        ** the 1st IF band edge and always found spurs, just use the original
        ** choice.  It's as "good" as any other.
        */
        if ( SpurFound == 0 )
        {
            fLO1 = zfLO1;
            fLO2 = zfLO2;
        }
    }

    /*
    ** Recalculate the LO information since it may have changed while
    ** searching for spur-free settings.
    */
    LO1 = fLO1 / f_Ref;
    Num1 = (4 * (fLO1 % f_Ref)) / f_Ref;                 /* Truncate here */
    LO2 = fLO2 / f_Ref;
    Num2 = ((4095L * (fLO2 % f_Ref)) + f_Ref/2) / f_Ref;  /* Round here */

    /*
    **  Place all of the calculated values into the local tuner
    **  register fields.
    */
    Instance->IOBuffer[TCDRV_MT2050_LO1C_1] = (Instance->IOBuffer[TCDRV_MT2050_LO1C_1] & ~0x03) | (unsigned char)Num1;              /* LO1N */
    Instance->IOBuffer[TCDRV_MT2050_LO1C_1] = (Instance->IOBuffer[TCDRV_MT2050_LO1C_1] & ~0x3C) | (unsigned char)((LO1 % 12) << 2); /* LO1B */
    Instance->IOBuffer[TCDRV_MT2050_LO1C_2] = (Instance->IOBuffer[TCDRV_MT2050_LO1C_2] & ~0x3F) | (unsigned char)(LO1 / 12 - 1);    /* LO1A */

    if (Num2 == 0)                                        /* FNON */
        Instance->IOBuffer[TCDRV_MT2050_LO2C_3] &= ~0x40;
    else
        Instance->IOBuffer[TCDRV_MT2050_LO2C_3] |= 0x40;

    Instance->IOBuffer[TCDRV_MT2050_LO2C_2] = ((unsigned char)Num2) & 0xFF;                              /* F2N0-7 */
    Instance->IOBuffer[TCDRV_MT2050_LO2C_1] = (Instance->IOBuffer[TCDRV_MT2050_LO2C_1] & ~0x0F) | ( (unsigned char)(Num2 >> 8) & 0x0F ); /* F2N8-11 */
    Instance->IOBuffer[TCDRV_MT2050_LO2C_1] = (Instance->IOBuffer[TCDRV_MT2050_LO2C_1] & ~0xE0) | (unsigned char)((LO2 % 8) << 5);     /* LO2B */
    Instance->IOBuffer[TCDRV_MT2050_LO2C_3] = (Instance->IOBuffer[TCDRV_MT2050_LO2C_3] & ~0x3F) | (unsigned char)(LO2 / 8 - 1);        /* LO2A */

    if (f_in < PkEnRange)                                /* VLLP */
        Instance->IOBuffer[TCDRV_MT2050_LO1C_1] |= 0x80;
    else
        Instance->IOBuffer[TCDRV_MT2050_LO1C_1] &= ~0x80;

    /*
    ** Now write out the computed register values
    ** IMPORTANT: There is a required order for writing some of the
    **            registers.  R3 must follow R1 & R2 and  R5 must
    **            follow R4. Simple numerical order works, so...
    */

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
     if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
    {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2050_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO1C_1]), 5, TCDRV_IO_TIMEOUT);
  }
  #endif 
    #if defined(STTUNER_DRV_CAB_STV0297E)
     if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
    {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2050_LO1C_1, &(Instance->IOBuffer[TCDRV_MT2050_LO1C_1]), 5);
  }
  #endif
  
  

    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return 0;
    }

    tuner_tdrv_DelayInMilliSec(10);

    /*
    **  Check for LO's locking
    */
    LOLocked = TCDRV_MT2050_GetTunerLock(Instance);
    if (!LOLocked)
        return 0;

    return ((fLO1 - fLO2 - f_out) * TunerStep) / 1000;

} /* TCDRV_MT2050_TunerSetFrequency */
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static U32 Round_fLO(U32 f_LO, U32 f_LO_Step, U32 f_ref)
{
    return f_ref * (f_LO / f_ref)
        + f_LO_Step * (((f_LO % f_ref) + (f_LO_Step / 2)) / f_LO_Step);
}
#endif
#if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static long TCDRV_MT2060_TunerSetFrequency(TCDRV_InstanceData_t *Instance, long _Freq)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT2060_TunerSetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

{

    U32     LO1;                  /* 1st LO register value        */
    U32     Num1;                 /* Numerator for LO1 reg. value */
    U32     LO2;                  /* 2nd LO register value        */
    U32     Num2;                 /* Numerator for LO2 reg. value */
    U32     f_IF1, f_in, f_out;
    U8      LNABand;              /* LNA Band setting             */
    U8      idx;                  /* index loop                   */
    S8      Locked;

    /* Convert all values from kHz to Hz */
/*    f_IF1_BW     = 1000L * TCDRV_MT2060_IF1_BW;
    f_Ref        = 1000L * TCDRV_MT2060_SRO_FREQ;
    PkEnRange    = 1000L * TCDRV_MT2060_PKENRANGE;
    f_IFBW       = 1000L * Instance->BandWidth[0];
    f_IF1_Center = 1000L * IF1_CENTER;*/
    f_in         = 1000 * (U32)(_Freq);
    f_out        = 1000 * Instance->Status.IntermediateFrequency;
    f_IF1        = TCDRV_MT2060_IF1_FREQ;

    /* Find LNA Band setting */

    LNABand = 1;                        /* def when f_in > all */
    for (idx=0; idx<10; ++idx)
    {
        if (Instance->LNA_Bands[idx] >= f_in)
        {
            LNABand = 11 - idx;
            break;
        }
    }
/*    VERBOSE_PRINT1("Using LNA Band #%d.\n", LNABand); */

    /* Assign in the requested values */

    Instance->AS_Data.f_in     = f_in;
    Instance->AS_Data.f_out    = f_out;
    Instance->AS_Data.f_out_bw = TCDRV_MT2060_IF1_BW;
    /* Request a 1st IF such that LO1 is on a step size */
    Instance->AS_Data.f_if1_Request = Round_fLO(f_IF1 + f_in, Instance->AS_Data.f_LO1_Step, Instance->AS_Data.f_ref) - f_in;

    /* Calculate frequency settings.  f_IF1_FREQ + f_in is the desired LO1 frequency */

    TCDRV_MT2060_ResetExclZones(&Instance->AS_Data);
    f_IF1 = TCDRV_MT2060_ChooseFirstIF(&Instance->AS_Data);
    Instance->AS_Data.f_LO1 = Round_fLO(f_IF1 + f_in, Instance->AS_Data.f_LO1_Step, Instance->AS_Data.f_ref);
    Instance->AS_Data.f_LO2 = Round_fLO(Instance->AS_Data.f_LO1 - f_out - f_in, Instance->AS_Data.f_LO2_Step, Instance->AS_Data.f_ref);

    /* Check for any LO spurs in the output bandwidth and adjust the LO settings to avoid them if needed */

    TCDRV_MT2060_AvoidSpurs(&Instance->AS_Data);

    /* MT_AvoidSpurs spurs may have changed the LO1 & LO2 values. */
    /* Recalculate the LO frequencies and the values to be placed in the tuning registers. */

    Instance->AS_Data.f_LO1 = CalcLO1Mult(&LO1, &Num1, Instance->AS_Data.f_LO1, Instance->AS_Data.f_LO1_Step, Instance->AS_Data.f_ref);
    Instance->AS_Data.f_LO2 = Round_fLO(Instance->AS_Data.f_LO1 - f_out - f_in, Instance->AS_Data.f_LO2_Step, Instance->AS_Data.f_ref);
    Instance->AS_Data.f_LO2 = CalcLO2Mult(&LO2, &Num2, Instance->AS_Data.f_LO2, Instance->AS_Data.f_LO2_Step, Instance->AS_Data.f_ref);

    /* Make sure that the Auto CapSelect is turned on since it may have been turned off last time we tuned. */

    Instance->IOBuffer[TCDRV_MT2060_RSVD_0C] |= 0x80;
    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
       {
       	Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_RSVD_0C, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0C], 1, TCDRV_IO_TIMEOUT); 
      }
      #endif   
    #if defined(STTUNER_DRV_CAB_STV0297E)
        if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
       {
       	Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_RSVD_0C, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0C], 1); 
      }
      #endif
      
    
      /* 0x0C */

    /* Place all of the calculated values into the local tuner register fields. */

    Instance->IOBuffer[TCDRV_MT2060_LO1C_1] = (U8)((LNABand << 4) | (Num1 >> 2));
    Instance->IOBuffer[TCDRV_MT2060_LO1C_2] = (U8)(LO1 & 0xFF);

    Instance->IOBuffer[TCDRV_MT2060_LO2C_1] = (U8)(Num2 & 0x000F) | ((Num1 & 0x03) << 4);
    Instance->IOBuffer[TCDRV_MT2060_LO2C_2] = (U8)((Num2 & 0x0FF0) >> 4);
    Instance->IOBuffer[TCDRV_MT2060_LO2C_3] = (U8)(((LO2 << 1) & 0xFE) | ((Num2 & 0x1000) >> 12));

    /* Now write out the computed register values                              */
    /* IMPORTANT: There is a required order for writing some of the registers. */
    /*            R2 must follow R1 and R5 must follow R3 & R4.                */
    /*            Simple numerical order works, so...                          */

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
       {
       	Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_LO1C_1, &Instance->IOBuffer[TCDRV_MT2060_LO1C_1], 5, TCDRV_IO_TIMEOUT); 
      }
      #endif  
    #if defined(STTUNER_DRV_CAB_STV0297E)
        if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
       {
       	Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_LO1C_1, &Instance->IOBuffer[TCDRV_MT2060_LO1C_1], 5); 
      }
      #endif
      
    
      /* 0x01 - 0x05 */
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(0);
    }

    /* Check for LO's locking */

    Locked = TCDRV_MT2060_Locked(Instance); /* -1 = I2C error; 1 = both LO's locked; 0 = LO's not locked */
    if (Locked == -1)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
        STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
        return(0);
    }

    /* Special Case: LO1 not locked, LO2 locked */

    if ((Locked == 0) && ((Instance->IOBuffer[TCDRV_MT2060_LO_STATUS] & 0x88) == 0x08) )
    {
        Locked = TCDRV_MT2060_LO1LockWorkAround(Instance); /* -1 = I2C error; 1 = both LO's locked; 0 = LO's not locked */
        if (Locked == -1)
        {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
            STTBX_Print(("%s Cant Access to FrontEnd\n", identity));
#endif
            return(0);      /* I2C error */
        }
    }

    if (Locked == 0)
    {
        return(0);      /* not tuned */
    }

    /* If we locked OK, assign calculated data to MT2121_Info_t structure */
    Instance->f_IF1_actual = Instance->AS_Data.f_LO1 - f_in;

    /* If tuned, return the actual tuned frequency (in Hz) */
    return ((Instance->AS_Data.f_LO1 - (long)Instance->AS_Data.f_LO2 - f_out) / 1000);
    }

}
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
/****************************************************************************
**
**  Name: MT2060_Locked
**
**  Description:    Checks to see if LO1 and LO2 are locked.
**
**  Parameters:     h            - Open handle to the tuner (from MT2060_Open).
**
**  Returns:        -1 if I2C error
**                  1 if both LO's are locked,
**                  otherwise it returns 0.
**
**  Dependencies:   MT_ReadSub    - Read byte(s) of data from the serial bus
**                  MT_Sleep      - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**         04Nov2004     GB     Re-written for STTUNER
**
****************************************************************************/
S8 TCDRV_MT2060_Locked(TCDRV_InstanceData_t *Instance)
{
    const U32 nMaxWait  = 200;            /*  wait a maximum of 200 msec  */
    const U32 nPollRate =   2;            /*  poll status bits every 2 ms */
    const U32 nMaxLoops = nMaxWait / nPollRate;
    U32 nDelays         =   0;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    do
    {
      #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_370QAM)
          if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
         {
       Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_LO_STATUS, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS], 1, TCDRV_IO_TIMEOUT);

          }
       #endif  
  
        #if defined(STTUNER_DRV_CAB_STV0297E)
          if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
         {
       Error = ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_LO_STATUS, 1, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS]);

  }
  #endif
  
       if (Error!= ST_NO_ERROR)
            return -1;

        if ((Instance->IOBuffer[TCDRV_MT2060_LO_STATUS] & 0x88) == 0x88)
            return 1;

        tuner_tdrv_DelayInMilliSec(2);       /*  Wait between retries  */
    }
    while (++nDelays < nMaxLoops);

    return 0;
}
#endif
/****************************************************************************
**
**  Name: CalcLO1Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO1's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   05-05-2004    JWS    Original
****************************************************************************/
#if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static U32 CalcLO1Mult(U32 *Div,
                       U32 *FracN,
                       U32  f_LO,
                       U32  f_LO_Step,
                       U32  f_Ref)
{
    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (64 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step)+ (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div)) + (f_LO_Step * (*FracN));
}
#endif

/****************************************************************************
**
**  Name: CalcLO2Mult
**
**  Description:    Calculates Integer divider value and the numerator
**                  value for LO2's FracN PLL.
**
**                  This function assumes that the f_LO and f_Ref are
**                  evenly divisible by f_LO_Step.
**
**  Parameters:     Div       - OUTPUT: Whole number portion of the multiplier
**                  FracN     - OUTPUT: Fractional portion of the multiplier
**                  f_LO      - desired LO frequency.
**                  f_LO_Step - Minimum step size for the LO (in Hz).
**                  f_Ref     - SRO frequency.
**
**  Returns:        Recalculated LO frequency.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-04-2003    JWS    Original, derived from Mtuner version
****************************************************************************/
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static U32 CalcLO2Mult(U32 *Div,
                       U32 *FracN,
                       U32  f_LO,
                       U32  f_LO_Step,
                       U32  f_Ref)
{
#define MAX_UDATA         (0xFFFFFFFF)  /*  max value storable in UData_t   */

    const U32 FracN_Scale = (f_Ref / (MAX_UDATA >> 13)) + 1;

    /*  Calculate the whole number portion of the divider */
    *Div = f_LO / f_Ref;

    /*  Calculate the numerator value (round to nearest f_LO_Step) */
    *FracN = (8192 * (((f_LO % f_Ref) + (f_LO_Step / 2)) / f_LO_Step) + (f_Ref / f_LO_Step / 2)) / (f_Ref / f_LO_Step);

    return (f_Ref * (*Div))
         + FracN_Scale * (((f_Ref / FracN_Scale) * (*FracN)) / 8191);

}
#endif

/****************************************************************************
**
**  Name: LO1LockWorkAround
**
**  Description:    Correct for problem where LO1 does not lock at the
**                  transition point between VCO1 and VCO2.
**
**  Parameters:     Instance       - Pointer to MT2060_Info Structure
**
**  Returns:        -1 if I2C error
**                  1 if both LO's are now locked
**                  0 if either LO remains unlocked.
**
**  Dependencies:   None
**
**                  MT_ReadSub      - Read byte(s) of data from the two-wire-bus
**                  MT_WriteSub     - Write byte(s) of data to the two-wire-bus
**                  MT_Sleep            - Delay execution for x milliseconds
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   04-20-2004    JWS    Original.
**         04Nov2004     GB     Re-written for STTUNER
**
****************************************************************************/
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
S8 TCDRV_MT2060_LO1LockWorkAround(TCDRV_InstanceData_t *Instance)
{
    U8 cs;
    U8 tad;
    S32 ChkCnt = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    cs = 30;  /* Pick starting CapSel Value */

    Instance->IOBuffer[TCDRV_MT2060_RSVD_0C] &= 0x7F;  /* Auto CapSelect off */
    Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] = (Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] & 0xC0) | cs;
   
   

   #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
       if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
      {
    Error = STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_RSVD_0C, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0C], 2, TCDRV_IO_TIMEOUT); 
      }
   #endif
   #if defined(STTUNER_DRV_CAB_STV0297E)
       if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
      {
    Error = ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_RSVD_0C, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0C], 2); 
      }
   #endif
      
      
     /* 0x0C - 0x0D */

    while (ChkCnt < 64)
    {
        tuner_tdrv_DelayInMilliSec(2);

        #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
            if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
           {
        Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_LO_STATUS, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS], 1, TCDRV_IO_TIMEOUT);

  }
  #endif   

        #if defined(STTUNER_DRV_CAB_STV0297E)
            if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
           {
        Error |= ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_LO_STATUS, 1, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS]);

  }
  #endif
  
  


        tad = (Instance->IOBuffer[TCDRV_MT2060_LO_STATUS] & 0x70) >> 4;  /* LO1AD */
        if (tad == 0)   /* Found good spot -- quit looking */
            break;
        else if (tad & 0x04 )  /* either 4 or 5; decrement */
        {
            Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] = (Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] & 0xC0) | --cs;
            #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
                if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
               {
            Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_RSVD_0D, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0D], 1, TCDRV_IO_TIMEOUT); 
              }
              #endif
            #if defined(STTUNER_DRV_CAB_STV0297E)
                if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
               {
            Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_RSVD_0D, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0D], 1); 
              }
              #endif
              
              /* 0x0D */
        }
        else if (tad & 0x02 )  /* either 2 or 3; increment */
        {
            Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] = (Instance->IOBuffer[TCDRV_MT2060_RSVD_0D] & 0xC0) | ++cs;
            #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
                if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
               {
            Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_WRITE, TCDRV_MT2060_RSVD_0D, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0D], 1, TCDRV_IO_TIMEOUT); 
              }
              #endif 
            #if defined(STTUNER_DRV_CAB_STV0297E)
                if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
               {
            Error |= ChipSetRegisters(Instance->IOHandle, TCDRV_MT2060_RSVD_0D, &Instance->IOBuffer[TCDRV_MT2060_RSVD_0D], 1); 
              }
              #endif
              
              /* 0x0D */
        }
        ChkCnt ++;  /* Count this attempt */
    }

    #if defined(STTUNER_DRV_CAB_STV0297) || defined(STTUNER_DRV_CAB_STV0498) || defined(STTUNER_DRV_CAB_STV0297J) || defined(STTUNER_DRV_CAB_STB0370QAM)
        if (*IOARCH_Handle[Instance->IOHandle].TargetFunction != NULL)
       {
    Error |= STTUNER_IOARCH_ReadWrite(Instance->IOHandle, STTUNER_IO_SA_READ_NS, TCDRV_MT2060_LO_STATUS, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS], 1, TCDRV_IO_TIMEOUT);

  }
  #endif
  
      #if defined(STTUNER_DRV_CAB_STV0297E)
        if (*IOARCH_Handle[Instance->IOHandle].RepeaterFn != NULL)
       {
    Error |= ChipGetRegisters(Instance->IOHandle, TCDRV_MT2060_LO_STATUS, 1, &Instance->IOBuffer[TCDRV_MT2060_LO_STATUS]);

  }
  #endif

    if (Error != ST_NO_ERROR)
        return(-1);   /* I2C error */

    if((Instance->IOBuffer[TCDRV_MT2060_LO_STATUS] & 0x88) == 0x88)  /* Both LOs should now be locked */
        return (1);
    else
        return (0);

} 
#endif
/* TCDRV_MT2060_TunerSetFrequency */


/* Reset all exclusion zones.                           */
/* Add zones to protect the PLL FracN regions near zero */
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
void TCDRV_MT2060_ResetExclZones(MT_AvoidSpursData_t* pAS_Info)
{
    U32     center;

    pAS_Info->nZones = 0;

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 + pAS_Info->f_in) / pAS_Info->f_ref) - pAS_Info->f_in;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO1_FracN_Avoid)
    {
        /* Exclude LO1 FracN */
        TCDRV_MT2060_AddExclZone(pAS_Info, center-pAS_Info->f_LO1_FracN_Avoid, center-1);
        TCDRV_MT2060_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO1_FracN_Avoid);
        center += pAS_Info->f_ref;
    }

    center = pAS_Info->f_ref * ((pAS_Info->f_if1_Center - pAS_Info->f_if1_bw/2 - pAS_Info->f_out) / pAS_Info->f_ref) + pAS_Info->f_out;
    while (center < pAS_Info->f_if1_Center + pAS_Info->f_if1_bw/2 + pAS_Info->f_LO2_FracN_Avoid)
    {
        /* Exclude LO2 FracN */
        TCDRV_MT2060_AddExclZone(pAS_Info, center-pAS_Info->f_LO2_FracN_Avoid, center-1);
        TCDRV_MT2060_AddExclZone(pAS_Info, center+1, center+pAS_Info->f_LO2_FracN_Avoid);
        center += pAS_Info->f_ref;
    }
}
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
/*
**  Add (and merge) an exclusion zone into the list.
**  If the range (f_min, f_max) is totally outside the 1st IF BW,
**  ignore the entry.
**
*/
void TCDRV_MT2060_AddExclZone(MT_AvoidSpursData_t* pAS_Info,
                              U32                  f_min,
                              U32                  f_max)
{
    U32     i, j, k;
    U32     bAdded = 0;
    if ((f_max > (pAS_Info->f_if1_Center - (pAS_Info->f_if1_bw / 2)))
        && (f_min < (pAS_Info->f_if1_Center + (pAS_Info->f_if1_bw / 2))))
    {
        /*                                                                           */
        /*   New entry:  |---|    |--|        |--|       |-|      |---|         |--| */
        /*                     or          or        or       or        or           */
        /*   Existing:  |--|          |--|      |--|    |---|      |-|      |--|     */
        /*                                                                           */
        for (i=0; (i<pAS_Info->nZones) && (bAdded==0); i++)
        {
            if (pAS_Info->MT_ExclZones[i].min > f_max)
            {
                /*                                               */
                /*   New entry: |--|                             */
                /*                                               */
                /*   Existing:       |--|                        */
                /*                                               */
                /*  Insert [f_min, f_max] before MT_ExclZones[i] */
                for (j=pAS_Info->nZones; j>i; j--)
                {
                    /* Move subsequent entries down one entry */
                    pAS_Info->MT_ExclZones[j].min = pAS_Info->MT_ExclZones[j-1].min;
                    pAS_Info->MT_ExclZones[j].max = pAS_Info->MT_ExclZones[j-1].max;
                }
                /* Insert the new entry */
                pAS_Info->MT_ExclZones[i].min = f_min;
                pAS_Info->MT_ExclZones[i].max = f_max;
                pAS_Info->nZones++;
                bAdded = 1;
            }
            else if ((pAS_Info->MT_ExclZones[i].min <= f_max) && (pAS_Info->MT_ExclZones[i].max >= f_max))
            {
                /*                                 */
                /*   New entry:  |--|         |-|  */
                /*                       or        */
                /*   Existing:     |--|      |---| */
                /*                                 */
                if (pAS_Info->MT_ExclZones[i].min > f_min)
                {
                    pAS_Info->MT_ExclZones[i].min = f_min;
                }
                bAdded = 1;
            }
            else if ((pAS_Info->MT_ExclZones[i].max <= f_max) && (pAS_Info->MT_ExclZones[i].max >= f_min))
            {
                /*                               */
                /*   New entry:  |---|     |---| */
                /*                     or        */
                /*   Existing:  |--|        |-|  */
                /*                               */
                pAS_Info->MT_ExclZones[i].max = f_max;
                if (pAS_Info->MT_ExclZones[i].min > f_min)
                    pAS_Info->MT_ExclZones[i].min = f_min;

                /* Check for overlaps of subsequent entries */
                j = i+1;
                while ((j<pAS_Info->nZones) && (pAS_Info->MT_ExclZones[j].min < pAS_Info->MT_ExclZones[i].max))
                {
                    /*                                                */
                    /*   New entry:  |-------|     |--------|         */
                    /*                         or                     */
                    /*   Existing:  |--|  |-|     |--|  |----|        */
                    /*                                                */
                    /*  The new entry overlaps the next entry as well */
                    if (pAS_Info->MT_ExclZones[i].max < pAS_Info->MT_ExclZones[j].max)
                        pAS_Info->MT_ExclZones[i].max = pAS_Info->MT_ExclZones[j].max;

                    /* Remove entry j from the list */
                    for (k=j+1; k<pAS_Info->nZones; k++)
                    {
                        pAS_Info->MT_ExclZones[k-1].min = pAS_Info->MT_ExclZones[k].min;
                        pAS_Info->MT_ExclZones[k-1].max = pAS_Info->MT_ExclZones[k].max;
                    }
                    pAS_Info->nZones--;
                }
                bAdded = 1;
            }
        }

        if (bAdded == 0)
        {
            /*  Add to end of array   */
            /*                        */
            /*   New entry:      |--| */
            /*                        */
            /*   Existing:   |--|     */
            /*                        */
            pAS_Info->MT_ExclZones[i].max = f_max;
            pAS_Info->MT_ExclZones[i].min = f_min;
            pAS_Info->nZones++;
        }
    }
}
#endif

#if defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
/* Return f_Desired if it is not excluded.                              */
/* Otherwise, return the value closest to f_Center that is not excluded */

U32 TCDRV_MT2060_ChooseFirstIF(MT_AvoidSpursData_t* pAS_Info)
{
    const U32     f_Desired = pAS_Info->f_if1_Request;
    const U32     f_Step = (pAS_Info->f_LO1_Step > pAS_Info->f_LO2_Step) ? pAS_Info->f_LO1_Step : pAS_Info->f_LO2_Step;
    U32     f_Center;

    S32     i;
    S32     j = 0;
    U32     bDesiredExcluded = 0;
    U32     bZeroExcluded = 0;
    S32     tmpMin, tmpMax;
    S32     bestDiff;

    MT_ExclZoneS_t zones[MAX_ZONES];

    if (pAS_Info->nZones == 0)
        return f_Desired;

    /* f_Center needs to be an integer multiple of f_Step away from f_Desired */
    if ((pAS_Info->f_if1_Center + f_Step/2) > f_Desired)
        f_Center = f_Desired + f_Step * (((pAS_Info->f_if1_Center + f_Step/2) - f_Desired) / f_Step);
    else
        f_Center = f_Desired - f_Step * ((f_Desired - (pAS_Info->f_if1_Center - f_Step/2)) / f_Step);

    assert(abs((S32) f_Center - (S32) pAS_Info->f_if1_Center) <= (S32) (f_Step/2));

    /* Take MT_ExclZones, center around f_Center and change the resolution to f_Step */
    for (i=0; i<((S32) pAS_Info->nZones); ++i)
    {
        if (pAS_Info->MT_ExclZones[i].min < f_Center)
            /* Implement floor for negative numbers */
            tmpMin = floor(-(S32) (f_Center - pAS_Info->MT_ExclZones[i].min), (S32) f_Step);
        else
            /* floor function */
            tmpMin = (S32) floor((S32)(pAS_Info->MT_ExclZones[i].min - f_Center), f_Step);

        if (pAS_Info->MT_ExclZones[i].max < f_Center)
            /* Implement ceil for negative numbers */
            tmpMax = ceil(-(S32) (f_Center - pAS_Info->MT_ExclZones[i].max), (S32) f_Step);
        else
            /* ceil function */
            tmpMax = (S32) ceil((S32)(pAS_Info->MT_ExclZones[i].max - f_Center), f_Step);

        if ((pAS_Info->MT_ExclZones[i].min < f_Desired) && (pAS_Info->MT_ExclZones[i].max > f_Desired))
            bDesiredExcluded = 1;

        if ((tmpMin < 0) && (tmpMax > 0))
            bZeroExcluded = 1;

        /* See if this zone overlaps the previous */
        if ((j>0) && (tmpMin < zones[j-1].max))
            zones[j-1].max = tmpMax;
        else
        {
            /* Add new zone */
            zones[j].min = tmpMin;
            zones[j].max = tmpMax;
            j++;
        }
    }

    /*
    **  If the desired is okay, return with it
    */
    if (bDesiredExcluded == 0)
        return f_Desired;

    /*
    **  If the desired is excluded and the center is okay, return with it
    */
    if (bZeroExcluded == 0)
        return f_Center;

    /* Find the value closest to 0 (f_Center) */
    bestDiff = zones[j-1].max;
    for (i=j-1; i>=0; i--)
    {
        if (abs(zones[i].max) > abs(bestDiff))
            break;
        bestDiff = zones[i].max;

        if (abs(zones[i].min) > abs(bestDiff))
            break;
        bestDiff = zones[i].min;
    }

    if (bestDiff < 0)
        return f_Center - ((U32) (-bestDiff) * f_Step);

    return f_Center + (bestDiff * f_Step);
}
#endif

/****************************************************************************
**
**  Name: gcd
**
**  Description:    Uses Euclid's algorithm
**
**  Parameters:     u, v     - unsigned values whose GCD is desired.
**
**  Global:         None
**
**  Returns:        greatest common divisor of u and v, if either value
**                  is 0, the other value is returned as the result.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-01-2004    JWS    Original
**   N/A   08-03-2004    DAD    Changed to Euclid's since it can handle
**                              unsigned numbers.
**
****************************************************************************/
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)

static U32 gcd(U32 u, U32 v)
{
    U32     r;

    while (v != 0)
    {
        r = u % v;
        u = v;
        v = r;
    }
    return u;
}
#endif
/****************************************************************************
**
**  Name: umax
**
**  Description:    Implements a simple maximum function for unsigned numbers.
**                  Implemented as a function rather than a macro to avoid
**                  multiple evaluation of the calling parameters.
**
**  Parameters:     a, b     - Values to be compared
**
**  Global:         None
**
**  Returns:        larger of the input values.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   06-02-2004    JWS    Original
**
****************************************************************************/
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)

static U32 umax(U32 a, U32 b)
{
    return (a >= b) ? a : b;
}
#endif
/****************************************************************************
**
**  Name: IsSpurInBand
**
**  Description:    Checks to see if a spur will be present within the IF's
**                  bandwidth. (fIFOut +/- fIFBW, -fIFOut +/- fIFBW)
**
**                    ma   mb                                     mc   md
**                  <--+-+-+-------------------+-------------------+-+-+-->
**                     |   ^                   0                   ^   |
**                     ^   b=-fIFOut+fIFBW/2      -b=+fIFOut-fIFBW/2   ^
**                     a=-fIFOut-fIFBW/2              -a=+fIFOut+fIFBW/2
**
**                  Note that some equations are doubled to prevent round-off
**                  problems when calculating fIFBW/2
**
**  Parameters:     pAS_Info - Avoid Spurs information block
**                  fm       - If spur, amount f_IF1 has to move negative
**                  fp       - If spur, amount f_IF1 has to move positive
**
**  Global:         None
**
**  Returns:        1 if an LO spur would be present, otherwise 0.
**
**  Dependencies:   None.
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   11-28-2002    DAD    Implemented algorithm from applied patent
**
****************************************************************************/
#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
static BOOL TCDRV_MT2060_IsSpurInBand(MT_AvoidSpursData_t* pAS_Info,
                                        U32* fm,
                                        U32* fp)
{
    /*
    **  Calculate LO frequency settings.
    */
#define MAX_UDATA         (0xFFFFFFFF)  /*  max value storable in UData_t   */

    U32     n, n0;
    const U32     f_LO1 = pAS_Info->f_LO1;
    const U32     f_LO2 = pAS_Info->f_LO2;
    const U32     d = pAS_Info->f_out + pAS_Info->f_out_bw/2;
    const U32     c = d - pAS_Info->f_out_bw;
    const U32     f = pAS_Info->f_zif_bw/2;
    const U32     f_Scale = (f_LO1 / (MAX_UDATA/2 / pAS_Info->maxH1)) + 1;
    S32     f_nsLO1, f_nsLO2;
    S32     f_Spur;
    U32     ma, mb, mc, md, me, mf;
    U32     lo_gcd, gd_Scale, gc_Scale, gf_Scale;

    *fm = 0;

    /*
    ** For each edge (d, c & f), calculate a scale, based on the gcd
    ** of f_LO1, f_LO2 and the edge value.  Use the larger of this
    ** gcd-based scale factor or f_Scale.
    */
    lo_gcd = gcd(f_LO1, f_LO2);
    gd_Scale = umax((U32) gcd(lo_gcd, d), f_Scale);
    gc_Scale = umax((U32) gcd(lo_gcd, c), f_Scale);
    gf_Scale = umax((U32) gcd(lo_gcd, f), f_Scale);

    n0 = ceil((S32)(f_LO2 - d), f_LO1 - f_LO2);

    /* Check out all multiples of LO1 from n0 to m_maxLOSpurHarmonic */
    for (n=n0; n<=pAS_Info->maxH1; ++n)
    {
        md = (n*(f_LO1/gd_Scale) - (d/gd_Scale)) / (f_LO2/gd_Scale);

        /* If # fLO2 harmonics > m_maxLOSpurHarmonic, then no spurs present */
        if (md >= pAS_Info->maxH1)
            break;

        ma = (n*(f_LO1/gd_Scale) + (d/gd_Scale)) / (f_LO2/gd_Scale);

        /* If no spurs between +/- (f_out + f_IFBW/2), then try next harmonic */
        if (md == ma)
            continue;

        mc = (n*(f_LO1/gc_Scale) - (c/gc_Scale)) / (f_LO2/gc_Scale);
        if (mc != md)
        {
            f_nsLO1 = (S32) (n*(f_LO1/gc_Scale));
            f_nsLO2 = (S32) (mc*(f_LO2/gc_Scale));
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - mc*(f_LO2 % gc_Scale);

            *fp = ((f_Spur - (S32) c) / (mc - n)) + 1;
            *fm = (((S32) d - f_Spur) / (mc - n)) + 1;
            return 1;
        }

        /* Location of Zero-IF-spur to be checked */
        me = (n*(f_LO1/gf_Scale) + (f/gf_Scale)) / (f_LO2/gf_Scale);
        mf = (n*(f_LO1/gf_Scale) - (f/gf_Scale)) / (f_LO2/gf_Scale);
        if (me != mf)
        {
            f_nsLO1 = n*(f_LO1/gf_Scale);
            f_nsLO2 = me*(f_LO2/gf_Scale);
            f_Spur = (gf_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gf_Scale) - me*(f_LO2 % gf_Scale);

            *fp = ((f_Spur + (S32) f) / (me - n)) + 1;
            *fm = (((S32) f - f_Spur) / (me - n)) + 1;
            return 1;
        }

        mb = (n*(f_LO1/gc_Scale) + (c/gc_Scale)) / (f_LO2/gc_Scale);
        if (ma != mb)
        {
            f_nsLO1 = n*(f_LO1/gc_Scale);
            f_nsLO2 = ma*(f_LO2/gc_Scale);
            f_Spur = (gc_Scale * (f_nsLO1 - f_nsLO2)) + n*(f_LO1 % gc_Scale) - ma*(f_LO2 % gc_Scale);

            *fp = (((S32) d + f_Spur) / (ma - n)) + 1;
            *fm = (-(f_Spur + (S32) c) / (ma - n)) + 1;
            return 1;
        }
    }

    /* No spurs found */
    return 0;
}
#endif

#if  defined (STTUNER_DRV_CAB_TUN_MT2050) || defined (STTUNER_DRV_CAB_TUN_MT2060)
BOOL TCDRV_MT2060_AvoidSpurs(MT_AvoidSpursData_t* pAS_Info)
{
    U32     fm, fp;                     /*  restricted range on LO's        */
    pAS_Info->bSpurAvoided = 0;
    pAS_Info->nSpursFound = 0;

    /*
    **  Avoid LO Generated Spurs
    **
    **  Make sure that have no LO-related spurs within the IF output
    **  bandwidth.
    **
    **  If there is an LO spur in this band, start at the current IF1 frequency
    **  and work out until we find a spur-free frequency or run up against the
    **  1st IF SAW band edge.  Use temporary copies of fLO1 and fLO2 so that they
    **  will be unchanged if a spur-free setting is not found.
    */
    pAS_Info->bSpurPresent = TCDRV_MT2060_IsSpurInBand(pAS_Info, &fm, &fp);
    if (pAS_Info->bSpurPresent)
    {
        U32 zfIF1 = pAS_Info->f_LO1 - pAS_Info->f_in; /* current attempt at a 1st IF    */
        U32 zfLO1 = pAS_Info->f_LO1;                  /* current attempt at an LO1 freq */
        U32 zfLO2 = pAS_Info->f_LO2;                  /* current attempt at an LO2 freq */
        U32 delta_IF1;
        U32 new_IF1;

        /* Spur was found, attempt to find a spur-free 1st IF */
        do
        {
            pAS_Info->nSpursFound++;

            /* Raise f_IF1_upper, if needed */
            TCDRV_MT2060_AddExclZone(pAS_Info, zfIF1 - fm, zfIF1 + fp);

            /*  Choose next IF1 that is closest to f_IF1_CENTER              */
            new_IF1 = TCDRV_MT2060_ChooseFirstIF(pAS_Info);

            if (new_IF1 > zfIF1)
            {
                pAS_Info->f_LO1 += (new_IF1 - zfIF1);
                pAS_Info->f_LO2 += (new_IF1 - zfIF1);
            }
            else
            {
                pAS_Info->f_LO1 -= (zfIF1 - new_IF1);
                pAS_Info->f_LO2 -= (zfIF1 - new_IF1);
            }
            zfIF1 = new_IF1;

            if (zfIF1 > pAS_Info->f_if1_Center)
                delta_IF1 = zfIF1 - pAS_Info->f_if1_Center;
            else
                delta_IF1 = pAS_Info->f_if1_Center - zfIF1;
        }
        /* Continue while the new 1st IF is still within the 1st IF bandwidth */
        /* and there is a spur in the band (again)                            */
        while ((2*delta_IF1 + pAS_Info->f_out_bw <= pAS_Info->f_if1_bw) &&
              (pAS_Info->bSpurPresent == TCDRV_MT2060_IsSpurInBand(pAS_Info, &fm, &fp)));

        /*
        ** Use the LO-spur free values found.  If the search went all the way to
        ** the 1st IF band edge and always found spurs, just leave the original
        ** choice.  It's as "good" as any other.
        */
        if (pAS_Info->bSpurPresent == 1)
        {
            pAS_Info->f_LO1 = zfLO1;
            pAS_Info->f_LO2 = zfLO2;
        }
        else
            pAS_Info->bSpurAvoided = 1;
    }
    return pAS_Info->bSpurAvoided;
}
#endif

#ifdef STTUNER_DRV_CAB_TUN_MT2011/*Freq to tune should be in hz*/
static int TCDRV_MT2011_TunerSetFrequency(TCDRV_InstanceData_t *Instance, int _Freq)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_TCDRV
    const char *identity = "STTUNER tcdrv.c TCDRV_MT2011_TunerSetFrequency()";
#endif

   STTUNER_InstanceDbase_t *Inst;

   BOOL retVal = FALSE;
   U32  lo1, lo2, If1Freq,frequency;

   U32 scale = 1;
   Inst = STTUNER_GetDrvInst();
   
   /* Calculate LO1 and LO2. Must be in multiples of Fstep*/
   frequency = scale * _Freq * (Instance->FreqFactor); /*multiply with 1000 since FE_xx_search passes in KHz & no FreqFactor for this tuner in use */
   
   If1Freq = kFif1;
   
   lo1 = frequency + If1Freq;
   
   /* round to nearest step val*/
   lo1 = (((lo1 + kFstep/2)/kFstep)*kFstep);
   
   /* update If1Freq due to rounding of LO1*/
   If1Freq = lo1 - frequency;
   
   lo2 = If1Freq - gRxTuner.IF2;/*lo2 = If1Freq - kFif2;   */
   
      
   /*exceptional frequencies must use precalculated values from table*/
   
   if(!TCDRV_MT2011_rftuner_CheckExcepTable(frequency, &lo1, &lo2))
   {  
      
      /*otherwise, adjust for spurs*/
     
      TCDRV_MT2011_rftuner_avoidSpurs(&lo1, &lo2, If1Freq);
   }

   gRxTuner.gFrequency = frequency;
   gRxTuner.gLO1 = lo1;
   gRxTuner.gLO2 = lo2;
   

   
   /* Set up other parameters which differ from the defaults*/
   TCDRV_MT2011_rftuner_setParameters(Instance->IOHandle);  
   
   /* Program the tuner registers*/
   TCDRV_MT2011_rftuner_progRegs(Instance->IOHandle, lo1, lo2);
   
   /* Check for lock*/
   STOS_TaskDelayUs(75000);  /* wait 75 ms before checking lock*/
   retVal = TCDRV_MT2011_mt2011_PLLLockStatus(Instance->IOHandle);
   if(retVal)
   {
   	return(gRxTuner.gFrequency);
   }
   return 0;
    
    
    
} 

/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_CheckExcepTable()

 Scope:      Private

 Parameters: U32 freq  - frequency in Hz
             U32 *pLo1 - Pointer to LO1
             U32 *pLo2 - Pointer to LO2

 Returns:    TRUE     Frequency is in the table:
                      LO1 and LO2 have been replaced. Use them.
             FALSE    Not an exception. Calculate as usual

 Operation:  Checks the Exception table for a match for Tuner and
             Frequency. Uses precalcualted values if found in table.
------------------------------------------------------------------------------*/
static BOOL TCDRV_MT2011_rftuner_CheckExcepTable(U32 freq, U32 *pLo1, U32 *pLo2)
{
   const struct exceptTableStr *pEt = NULL;
   
   pEt = gNullExceptionTable;

   
   /* Stop when tuner number in table is higher than tuner we are looking for*/
   for(; pEt->ets_tuner != 100; pEt++)
   {
      /* For right tuner, passed the frequency*/
      if(pEt->ets_freq > freq)
         break;

      /* Still too low: keep looking*/
      if(pEt->ets_freq < freq)
         continue;

      *pLo1 = pEt->ets_lo1;
      *pLo2 = pEt->ets_lo2;
     
      return TRUE;
   }
   
   return FALSE;
}
 
/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_avoidSpurs()

 Scope:      Private

 Parameters: lo1 - first LO value (Hz)
             lo2 - second LO value (Hz)
             If1Freq  - The IF Frequency

 Returns:    None

 Purpose:    Performs spur-avoidance algorithm as described in MT2011 AP note
             Uses floats internally to avoid range problems when calculating
             upper harmonics.
------------------------------------------------------------------------------*/
static void TCDRV_MT2011_rftuner_avoidSpurs(U32 *pLo1, U32 *pLo2, U32 If1Freq)
{
   U32     flo1best, flo2best, harm1best, flo1, flo2, n, x;
   signed int   fcurrstep, fsign, ftest, flimit, fPassMin, fPassMax;
   BOOL  spurFound;
  
   /* To track "best" spurs set so far.
      Scale frequency input params to avoid 32-bit overflow on high harmonics.
      Typical numbers will be even kHz and so divisible evenly by 100.*/
   If1Freq /= 100;
   flo1best = flo1 = *pLo1 / 100;
   flo2best = flo2 = *pLo2 / 100;
   harm1best = 0;
   fcurrstep = (kFlo_Step / 100);
   fsign = 1;  /* Alternates positive and negative.
                  We try frequencies on alternating sides, at
                  increasing distances.*/
        
   /* For fast range check*/
   fPassMin = (gRxTuner.IF2 / 100) - (kFifbw / 200); /*kFif2 jcl d2*/
   fPassMax = (gRxTuner.IF2 / 100) + (kFifbw / 200); /*kFif2 jcl d2*/

   /* Try frequencies in spiral order if spur found. */
   for (x = 0; x < kMAX_STEPS; x++)
   {
      n = 0;
      spurFound = FALSE;

      while ( (n++ < kMAX_HARMONICS) && (spurFound == FALSE) )
      {  
         
         
         /*Check harmonics lowest to highest. Since flo1 > flo2
         values "above the diagonal" cannot be in range.*/
         ftest = (n * (flo1 - flo2)) - flo2;   /* Starting test value.*/
         
         
         /*Since we are testing below a diagonal, each pass has fewer harms
         of second oscillator to test. What frequency does that represent?*/
         flimit = ftest - (flo2 * (kMAX_HARMONICS - n - 1));
         
         
         /*Remaining to test don't even reach edge of range?*/
         
         if (flimit > fPassMax)
         {
            continue;
         }
         
         
         /* But no need to test after we have passed the range: then they just
             get farther away*/
         if (flimit < (-fPassMax))
         {
            flimit = (-fPassMax);
         }
         while ( (ftest >= flimit) && (spurFound == FALSE) )
         {
            if (ftest <= fPassMax)
            {
               if ( (ftest <= (-fPassMin)) || (ftest >= fPassMin) )
               {
                   /*If this spur at a higher harmonic than current best,
                   save as new best
                   If a tie, the early (more centered)one wins*/
                  if (n > harm1best)
                  {
                     harm1best = n;
                     flo1best = flo1;
                     flo2best = flo2;
                  }

                  flo1   += fsign * fcurrstep;
                  flo2   += fsign * fcurrstep;
                  If1Freq += fsign * fcurrstep;
                  fsign   = -fsign;
                  fcurrstep += (kFlo_Step / 100);
                  spurFound = TRUE;

                 
               }
            }
            ftest -= flo2;
         }
      }
      if (!spurFound)
      {  
         *pLo1 = flo1 * 100;
         *pLo2 = flo2 * 100;   
          if (TCDRV_MT2011_rftuner_FractSpurDetect(pLo1, pLo2))
         {  
           
            return;
         }
         else
         {
            flo1   += fsign * fcurrstep;
            flo2   += fsign * fcurrstep;
            If1Freq += fsign * fcurrstep;
            fsign   = -fsign;
            fcurrstep += (kFlo_Step / 100);

         } 
      } 
   }  /* Try the next step until we are too far away*/

   *pLo1 = flo1best * 100;  /* If we are here, we couldn't avoid spurs*/
   *pLo2 = flo2best * 100;  /* Return freq combo with spurs at highest harmonics*/

}   

/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_FractSpurDetect()

 Scope:      Private

 Parameters: U32  *pLo1    Pointer to Lo1 value to be adjusted
                            in Hz units
             U32  *pLo2    Pointer to Lo2 value to be adjusted
                            in Hz units

 Returns:    TRUE           Current values in Lo1 and Lo2 can be used
             FALSE          Could not avoid spurs without adjusting

 Operation:  Returns TRUE if current Lo1 and Lo2 will not induce
             fractional spurs. Returns FALSE otherwise. Does not
             attempt to perform adjustment.
------------------------------------------------------------------------------*/
static BOOL TCDRV_MT2011_rftuner_FractSpurDetect(U32 *pLo1, U32 *pLo2)
{
   int result1, result2;

   result1 = TCDRV_MT2011_rftuner_FractDetectLo(*pLo1, kFref, gFracLo1Table60);
   result2 = TCDRV_MT2011_rftuner_FractDetectLo(*pLo2, kFref, gFracLo2Table60);

   if(result1 || result2)
   {
      return FALSE;
   }
   return TRUE;
}

/*------------------------------------------------------------------------------
 Function:   mt_FractDetectLo()

 Scope:      Private

 Parameters: U32  lo             LO value in HZ.
             U32  fref           Reference frequency in HZ
             struct fracThreshStr *pTab Pointer to threshold table to use.

 Returns:    U32  TRUE        There would be a fractional spur
                   FALSE       There would NOT be a fractional spur.
------------------------------------------------------------------------------*/
static U32 TCDRV_MT2011_rftuner_FractDetectLo(U32 lo, U32 fref, const struct fractThreshStr *pTab)
{
   U32 num, div;

   TCDRV_MT2011_rftuner_CalcLoNumDiv(lo, fref, &num, &div);

   for(; pTab->ft_bottom >= 0; pTab++)
   {
      if(num > (U32)pTab->ft_bottom &&
         num <= (U32)pTab->ft_top)
      {
         return TRUE;/* Fractional spur range*/
      }
   }
   return FALSE;     /* No fractional spur*/
}


/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_setParameters(IOARCH_Handle_t Handle)

 Scope:      Private

 Parameters: none

 Returns:    None

 Purpose:    Program the MT2011 tuner registers for everything that needs to
             be set up, but is not directly related to tuning.
 Behavior:   This function is used to set up the part to pass the reference
             frequency on to the 2nd IB tuner as needed and also to turn off
             part features we are not using.
------------------------------------------------------------------------------*/
static void TCDRV_MT2011_rftuner_setParameters(IOARCH_Handle_t Handle)
{  
   U8 wData[2];
   U8 tmp=0;

   

   /* Disable bandswitch selection and disable the low band mixer */
   wData[0] = 0x74;
   wData[1] = 0xb7;
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_Ctrl_Byte2, wData, 2);

   /* Disable FDC amp and PIN diode drivers*/
   rxtuner_Read(&IOARCH_Handle[Handle].ExtDev, rMT_PWR, &tmp, 1);
   tmp &= ~( (1<<7) | (1<<3) );
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_PWR, &tmp, 1);   
}

/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_progRegs(IOARCH_Handle_t Handle)

 Scope:      Private

 Parameters: lo1  - LO1
             lo2  - LO2

 Returns:    None

 Purpose:    Program the MT2011 tuner registers to tune to a frequency

 Behavior:   Calculates the DIV and NUM values for LO1 and LO2, and writes
             them to the device in the correct order.

             div = LO / Fref
             num = [8191 * (LO % fref)] / Fref
------------------------------------------------------------------------------*/
static void TCDRV_MT2011_rftuner_progRegs (IOARCH_Handle_t Handle, U32 lo1, U32 lo2)
{
   U32  div, num;
   U8   wData[6];
   U8   tmp=0;

   /** Disable LO1 automatic charge pump*/
   tmp = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_Ctrl_Byte3);
   tmp &= ~(1<<7);
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_Ctrl_Byte3, &tmp, 1);

   /** Set LO1 charge pump gain control(LO1GC)*/
   tmp = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_CP_Ctrl);
   tmp |= (3<<6);
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_CP_Ctrl, &tmp, 1);

   /** LO1*/
   TCDRV_MT2011_rftuner_CalcLoNumDiv(lo1, kFref, &num, &div);
   wData[0] = num >> 8;
   wData[1] = num;
   wData[2] = div;

   /** LO2*/
   TCDRV_MT2011_rftuner_CalcLoNumDiv(lo2, kFref, &num, &div);
   wData[3] = num >> 8;
   wData[4] = num;
   wData[5] = div;
  
   /** Write LOC1 and LOC2 registers */     
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO1C_Byte1, &(wData[0]), 2);		
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO1C_Byte3, &(wData[2]), 1);
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO2C_Byte1, &(wData[3]), 2);
   rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO2C_Byte3, &(wData[5]), 1);
   	
   STOS_TaskDelayUs(4000);	/*jcl demod test*/
   wData[0]=wData[1]=wData[2]=wData[3]=wData[4]=wData[5]=0;
   rxtuner_Read(&IOARCH_Handle[Handle].ExtDev, rMT_LO1C_Byte1, &(wData[0]), 2); 
   rxtuner_Read(&IOARCH_Handle[Handle].ExtDev, rMT_LO1C_Byte3, &(wData[2]), 1);       
   rxtuner_Read(&IOARCH_Handle[Handle].ExtDev, rMT_LO2C_Byte1, &(wData[3]), 2);
   rxtuner_Read(&IOARCH_Handle[Handle].ExtDev, rMT_LO2C_Byte3, &(wData[5]), 1);		
  

}          


/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_CalcLoNumDiv()

 Scope:      Private

 Parameters: U32     lo       Local Oscillator Freq in Hz.
             U32     fref     Reference Frequency in Hz.
             U32     *pNum    for return of this LO's Num value
             U32     *pDiv    for return of this LO's Div value

 Returns:    none

 Operation:  calculate div's, lo_num's from fref and lo's
             Note, the ability of the scale factors to prevent
             over- and underflow depends on fref being on the
             order of 14,800,000 Hz.
------------------------------------------------------------------------------*/
static void TCDRV_MT2011_rftuner_CalcLoNumDiv(U32 lo, U32 fref, U32 *pNum, U32 *pDiv)
{
   U32 num, div;
 
   /** div = integer of (lo/fref) */
   div = lo/fref;
   
   
   /* num = 8191 * (lo/fref - div) Rounded to nearest int*/
   /*num = ((8191 * ((lo/kFstep) % (fref/kFstep))) + (fref/kFstep) / 2) / (fref/kFstep);*/
   num = (((2 * 8191 * ((lo % fref)/500))/(fref/500)) + 1)/2;
   
    
   /* The value of 8191 is possible because of rounding
    but it is equivalent to 0 with next larger div.*/
   if(num >= 8191)
   {
      num = 0;
      div++;
   }
   *pDiv = div;
   *pNum = num;
     
}



/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_mt2011_PLLLockStatus(IOARCH_Handle_t Handle)

 Scope:      Public

 Parameters: none

 Returns:    TRUE Tuned successfully with LO lock
             FALSE Failed to achieve LO lock

 Purpose:    Checks for LO lock
------------------------------------------------------------------------------*/
BOOL TCDRV_MT2011_mt2011_PLLLockStatus(IOARCH_Handle_t Handle)
{
   BOOL success = FALSE;
   U8   tmp;
   U32 steps;

   for (steps = 0; steps < 50; steps++)
   {  
      
      /* if not locked and last iteration through loop, then attempt workaround*/
      
      if ((steps == 49) && ((rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO_Stat) & kMT_LOCKED) != kMT_LOCKED))
      {
           TCDRV_MT2011_rftuner_lockFix(Handle);
      }
      
      /* if locked then enable automatic charge pump and adjust AGC*/
      
      if (((tmp=rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO_Stat)) & kMT_LOCKED) == kMT_LOCKED)
      {
         
         success = TRUE;
         break;
      }
      STOS_TaskDelayUs(4000);
   }

   if (success)
   {   
      
      /* Enable automatic charge pump */
      
      tmp = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_Ctrl_Byte3);
      tmp |= (1<<7);
      rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_Ctrl_Byte3, &tmp, 1);
   }

   return success;
}


/*------------------------------------------------------------------------------
 Function:   TCDRV_MT2011_rftuner_lockFix(IOARCH_Handle_t Handle)

 Scope:      Private

 Parameters: None

 Returns:    None

 Purpose: from Mike Gaul...
    It appears that some (a low percentage; we are trying now to determine how low)
    of the MT2011's have trouble locking certain frequencies. The frequencies
    involved vary from chip to chip, and most chips do not seem to have the problem.
    When the problem occurs, it typically involves a band of frequencies less then
    2MHz wide, typically somewhere in the 450-550 MHz range.

    The nature of the defect is that the manufacturing tests that were in use to
    produce the units currently in the field (or in the warehouse) might not have
    caught all the problems. Microtune has investigated this problem and has
    recommended this software workaround to acheive a lock when this problem occurs.

 Behavior:
    During checking for local LO lock:
    First look for a local LO lock in the usual way
        (both lock bits set in the lock register).
    If you get one, there is no change.
    If it was LO2 that didn't lock, there is some different problem.
        The known issue involves only LO1.
    If it was LO1 that didn't lock, you try the workaround:
        disable CAP1en, and try various CAP1SEL values (register 0x13) using
        the lock register contents (0x9) to indicate in what direction to
        adjust.
    If this procedure fails to produce a lock:
        enable CAP1en as last gasp effort to produce lock
------------------------------------------------------------------------------*/
static void TCDRV_MT2011_rftuner_lockFix(IOARCH_Handle_t Handle)
{
   U8 value, data;
   int iterate;
   
   /* If LO2 did not lock, don't attempt LO1 correction.*/
   
  if((rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO_Stat) & kMT_LO2LK) == kMT_LO2LK)
   { 
      
      /* Disable automatic cap selection*/
     

      data = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO1_Byte2) & ~0x80;
      rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO1_Byte2, &data, 1);

      value = 0x1e;  /** Starting value*/
      for(iterate = 0; iterate < 10; iterate++)
      {
         rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO1_Byte1, &value, 1);
         STOS_TaskDelayUs(2000); /** Wait 2 ms before testing for lock*/

         data = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO_Stat);

          /*these 3 bits are not widely publicized numbers,something called LO1AD[2:0]*/
         
         switch((data >> 4) & 0x7)
         {
            case 0:    /** Good: Locked!*/
               return;

            case 1:    /** unexpected values, give up*/
            case 7:
               iterate = 10;
               break;

            case 2:    /** Capacitor value is too low. Raise it.*/
            case 3:
               value++;
               break;

            case 4:    /** Capacitor value is too high. Lower it.*/
            case 5:
               value--;
               break;
         }
      }
      
       /*not locked, enable automatic cap selection for last gasp effort*/
      
      data = rftuner_read8(&IOARCH_Handle[Handle].ExtDev, rMT_LO1_Byte2) | 0x80;
      rxtuner_Write(&IOARCH_Handle[Handle].ExtDev, rMT_LO1_Byte2, &data, 1); 
   }
}  
#endif

/* EOF */

