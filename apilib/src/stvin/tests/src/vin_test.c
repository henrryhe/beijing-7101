/************************************************************************
COPYRIGHT (C) STMicroelectronics 2000

File name   : vin_test.c
Description : VIN macros

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
12 Feb 2001   Creation                                        JG
02 Sep 2001   cut file vin_test.c in vin_cmd.c (api function)
              and vin_test.c (specific function test)         JG
              prepare test for ST40GX1

************************************************************************/
/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testcfg.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stvin.h"
#include "stsys.h"
#include "sttbx.h"

#if !defined(ST_OSLINUX)
#include "stlite.h"
#else  /*!ST_OSLINUX*/
#include "iocstapigat.h"
#endif  /*!ST_OSLINUX*/


#include "testtool.h"
#include "stgxobj.h"
#include "api_cmd.h"
#include "startup.h"

#include "clevt.h"
#ifdef USE_CLKRV
#include "clclkrv.h"
#endif /*USE_CLKRV*/
#include "clintmr.h"
#include "vin_cmd.h"
#include "vid_cmd.h"

#if defined(ST_GX1)
#include "st40gx1.h"
#include "mb317.h"
#endif

/* --- Extern functions prototypes --- */
BOOL VID_RegisterCmd2 (void);
BOOL VID_Inj_RegisterCmd (void);
BOOL VIDTEST_Init(void);

/* Private preliminary definitions (internal use only) ---------------------- */
#ifdef ST_OS20
#define STACK_SIZE 4092
#endif
#if defined ST_OS21 || defined ST_OSLINUX
#define STACK_SIZE 16*1024
#endif

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

#if defined (ST_GX1)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0xBB420000
#define VIN_BASE_ADDRESS         0
#elif defined (ST_7015)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT
#define VIN_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VIN_BASE_ADDRESS         ST7015_SDIN1_OFFSET
#define VIN_BASE_ADDRESS_SD2     ST7015_SDIN2_OFFSET
#define VIN_BASE_ADDRESS_HD      ST7015_HDIN_OFFSET
#elif defined (ST_7020)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT
#define VIN_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VIN_BASE_ADDRESS         ST7020_SDIN1_OFFSET
#define VIN_BASE_ADDRESS_SD2     ST7020_SDIN2_OFFSET
#define VIN_BASE_ADDRESS_HD      ST7020_HDIN_OFFSET
#elif defined (ST_7710)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7710_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0
#define VIN_BASE_ADDRESS         ST7710_DVP_BASE_ADDRESS
#elif defined (ST_7100)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7100_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0
#define VIN_BASE_ADDRESS         ST7100_DVP_BASE_ADDRESS
#elif defined (ST_7109)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7109_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0
#define VIN_BASE_ADDRESS         ST7109_DVP_BASE_ADDRESS
#else
#error Not defined processor
#endif

#if defined (ST_OSLINUX)     /* ST_OSLINUX */
  extern MapParams_t   DvpMap;
  extern MapParams_t   Disp0Map;
  STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
  #define VIN_BASE_ADDRESS_OS          DvpMap.MappedBaseAddress
  #define DISP0_BASE_ADDRESS_OS        Disp0Map.MappedBaseAddress
#else  /* ST_OSLINUX */
  extern STAVMEM_PartitionHandle_t     AvmemPartitionHandle[1];
  #define VIN_BASE_ADDRESS_OS          VIN_BASE_ADDRESS
  #define DISP0_BASE_ADDRESS_OS        DISP0_BASE_ADDRESS
#endif  /* ST_OSLINUX */

/*extern ST_Partition_t *SystemPartition_p;    */

#define stvin_read(a)      STSYS_ReadRegMem32LE((void*)(a))
#define stvin_write(a,v)   STSYS_WriteRegMem32LE((void*)(a),(v))

/* --- Private Constants --- */
#define NB_OF_VIN_EVENTS         1
#define NB_MAX_OF_STACKED_EVENTS 200

#define SDINn_RFP    0x08    /* Reconstruction luma frame pointer     */
#define SDINn_RCHP   0x0C    /* Reconstruction chroma frame pointer   */
#define SDINn_PFP    0x28    /* Presentation luma frame pointer       */
#define SDINn_PCHP   0x2C    /* Presentation chroma frame pointer     */
#define SDINn_PFLD   0x38    /* Presentation field main display       */
#define SDINn_ANC    0x50    /* Ancillary data buffer begin           */
#define SDINn_DFW    0x60    /* Input frame width                     */
#define SDINn_PFW    0x6C    /* Presentation frame width              */
#define SDINn_PFH    0x70    /* Presentation frame height             */
#define SDINn_DMOD   0x80    /* Reconstruction memory mode            */
#define SDINn_PMOD   0x84    /* Presentation memory mode              */
#define SDINn_ITM    0x98    /* Interrupt mask                        */
#define SDINn_ITS    0x9C    /* Interrupt status                      */
#define SDINn_STA    0xA0    /* Status                                */
#if defined (ST_7015)
#define SDINn_CTRL   0xB0    /* Control for SD pixel port             */
#define SDINn_LL     0xB4    /* Lower pixel count limit               */
#define SDINn_UL     0xB8    /* Upper pixel count limit               */
#define SDINn_LNCNT  0xBC    /* Line count at most recent Vsync       */
#define SDINn_HOFF   0xC0    /* Horizontal offset Hsync to D1Hsync    */
#define SDINn_VLOFF  0xC4    /* Vertical offset for Vsync to D1Vsync  */
#define SDINn_VHOFF  0xC8    /* Horizontal offset for Vsync to D1Vsync*/
#define SDINn_VBLANK 0xCC    /* Vertical blanking after Vsync         */
#define SDINn_HBLANK 0xD0    /* Time between Hsync and active video   */
#define SDINn_LENGTH 0xD4    /* Ancillary data capture length         */
#elif defined (ST_7020)
#define SDINn_CTL   0xB0    /* Control for SD pixel port             */
#define SDINn_TFO    0xB4    /* SD input top field offset             */
#define SDINn_TFS    0xB8    /* SD input top field stop               */
#define SDINn_BFO    0xBC    /* SD input bottom field offset          */
#define SDINn_BFS    0xC0    /* SD input bottom field stop            */
#define SDINn_VSD    0xC4    /* SD input vertical synchronization delay   */
#define SDINn_HSD    0xC8    /* SD input horizontal synchronization delay */
#define SDINn_HLL    0xCC    /* SD input half-line length                 */
#define SDINn_HLFLN  0xD0    /* SD input half-lines per verical           */
#define SDINn_APS    0xD4    /* SD input ancillary data page size         */
#endif /* ST_7020 */

#define HDIN_RFP      0x08    /* Reconstruction luma frame pointer    */
#define HDIN_RCHP     0x0C    /* Reconstruction chroma frame pointer  */
#define HDIN_PFP      0x28    /* Presentation luma frame pointer      */
#define HDIN_PCHP     0x2C    /* Presentation chroma frame pointer    */
#define HDIN_PFLD     0x38    /* Presentation field main display      */
#define HDIN_DFW      0x60    /* Input frame width                    */
#define HDIN_PFW      0x6C    /* Presentation frame width             */
#define HDIN_PFH      0x70    /* Presentation frame height            */
#define HDIN_DMOD     0x80    /* Reconstruction memory mode           */
#define HDIN_PMOD     0x84    /* Presentation memory mode             */
#define HDIN_ITM      0x98    /* Interrupt mask                       */
#define HDIN_ITS      0x9C    /* Interrupt status                     */
#define HDIN_STA      0xA0    /* Status                               */
#define HDIN_CTRL     0xB0    /* Control for HD pixel port            */
#define HDIN_LL       0xB4    /* Lower horizontal limit               */
#define HDIN_UL       0xB8    /* Upper horizontal limit               */
#define HDIN_LNCNT    0xBC    /* Line count                           */
#define HDIN_HOFF     0xC0    /* Time Hsync to HDHsync rising edge    */
#define HDIN_VLOFF    0xC4    /* Vertical offset for HDVsync          */
#define HDIN_VHOFF    0xC8    /* Horizontal offset for HDVsync        */
#define HDIN_VBLANK   0xCC    /* Vertical offset for active video     */
#define HDIN_HBLANK   0xD0    /* Horizontal offset for active video   */
#define HDIN_HSIZE    0xD4    /* Number of samples per input line     */
#define HDIN_TOP_VEND 0xD8    /* Last line of top field/frame         */
#define HDIN_BOT_VEND 0xDC    /* Last line of bottom field            */

#define DVPn_CTL   0x00    /* Control register                               */
#define DVPn_TFO   0x04    /* Top field offset                               */
#define DVPn_TFS   0x08    /* Top field stop                                 */
#define DVPn_BFO   0x0C    /* Bottom field offset                            */
#define DVPn_BFS   0x10    /* Bottom field stop                              */
#define DVPn_VTP   0x14    /* Video top pointer                              */
#define DVPn_VBP   0x18    /* Video bottom pointern                          */
#define DVPn_VMP   0x1C    /* Video memory pitch                             */
#define DVPn_CVS   0x20    /* Captured video size                            */
#define DVPn_VSD   0x24    /* Vertical synchro delay                         */
#define DVPn_HSD   0x28    /* Horizontal synchro delay                       */
#define DVPn_HLL   0x2C    /* Half line length                               */
#define DVPn_HSRC  0x30    /* Horizontal sample rate converter configuration */
#define DVPn_HFC   0x58    /* Horizontal filter coefficients                 */
#define DVPn_VSRC  0x60    /* Vertical sample rate converter configuration   */
#define DVPn_VFC   0x78    /* Vertical filter coefficients                   */
#define DVPn_ITM   0x98    /* DVP interrupt mask                             */
#define DVPn_ITS   0x9C    /* DVP interrupt status                           */
#define DVPn_STA   0xA0    /* DVP interrupt register                         */
#define DVPn_LNSTA 0xA4    /* Line number status                             */
#define DVPn_HLFLN 0xA8    /* Half lines per field                           */
#define DVPn_ABA   0xAc    /* Ancillary data base address                    */
#define DVPn_AEA   0xB0    /* Ancillary data end address                     */
#define DVPn_APS   0xB4    /* Ancillary data page size                       */
#define DVPn_PKZ   0xFC    /* Packet size on SuperHyway protocol             */

#define DVP_CTL      0x00    /* Control register                                          */
#define DVP_TFO      0x04    /* Top Field Offset (h & v, wrt (0,0))                       */
#define DVP_TFS      0x08    /* Top Field Stop (h & v, wrt (0,0))                         */
#define DVP_BFO      0x0C    /* Bottom Field Offset (h & v, wrt (0,0))                    */
#define DVP_BFS      0x10    /* Bottom Field Stop (h & v, wrt (0,0))                      */
#define DVP_VTP      0x14    /* Video Top Pointer (memory location for top-left pixel)    */
#define DVP_VBP      0x18    /* Video Bottom Pointer (memory location for top-left pixel) */
#define DVP_VMP      0x1C    /* Video Memory Pitch                                        */
#define DVP_CVS      0x20    /* Captured Video Size                                       */
#define DVP_VSD      0x24    /* Vertical synchro delay                                    */
#define DVP_HSD      0x28    /* Horizontal synchro delay                                  */
#define DVP_HLL      0x2C    /* Half line length                                          */
#define DVP_HSRC     0x30    /* Horizontal Sample Rate Converter configuration            */
#define DVP_HFC0     0x34    /* Horizontal Filter Coefficients (10 addresses)             */
#define DVP_HFC1     0x38    /*                                                           */
#define DVP_HFC2     0x3C    /*                                                           */
#define DVP_HFC3     0x40    /*                                                           */
#define DVP_HFC4     0x44    /*                                                           */
#define DVP_HFC5     0x48    /*                                                           */
#define DVP_HFC6     0x4C    /*                                                           */
#define DVP_HFC7     0x50    /*                                                           */
#define DVP_HFC8     0x54    /*                                                           */
#define DVP_HFC9     0x58    /*                                                           */
#define DVP_VSRC     0x60    /* Vertical Sample Rate Converter configuration              */
#define DVP_VFC0     0x64    /* Vertical Filter Coefficients (6 addresses)                */
#define DVP_VFC1     0x68    /*                                                           */
#define DVP_VFC2     0x6c    /*                                                           */
#define DVP_VFC3     0x70    /*                                                           */
#define DVP_VFC4     0x74    /*                                                           */
#define DVP_VFC5     0x78    /*                                                           */
#define DVP_ITM      0x98    /* DVP interrupt mask                                        */
#define DVP_ITS      0x9C    /* DVP interrupt status                                      */
#define DVP_STA      0xA0    /* DVP interrupt register                                    */
#define DVP_LNSTA    0xA4    /* Line Number STAtus                                        */
#define DVP_HLFLN    0xA8    /* Half lines per field                                      */
#define DVP_ABA      0xAC    /* Ancillary data Base Address                               */
#define DVP_AEA      0xB0    /*Ancillary data End Address                                 */
#define DVP_APS      0xB4    /*Ancillary data Page Size                                   */
#define DVP_PKZ      0xFC    /*Packet Size on STBus protocol                              */

#define DISP_CTRL        0x00
#define DISP_LUMA_HSRC   0x04
#define DISP_LUMA_VSRC   0x08
#define DISP_CHR_HSRC    0x0C
#define DISP_CHR_VSRC    0x10
#define DISP_TARGET_SIZE 0x14
#define DISP_NLZZD_Y     0x18
#define DISP_NLZZD_C     0x1C
#define DISP_PDELTA      0x20
#define DISP_MA_CTRL     0x80
#define DISP_LUMA_BA     0x84
#define DISP_CHR_BA      0x88
#define DISP_PMP         0x8C
#define DISP_LUMA_XY     0x90
#define DISP_CHR_XY      0x94
#define DISP_LUMA_SIZE   0x98
#define DISP_CHR_SIZE    0x9C
#define DISP_HFBA        0xA0
#define DISP_VFBA        0xA4
#define DISP_PKZ         0xFC

#define DEI_CTL           0x00
#define DEI_VIEWPORT_ORIG 0x04
#define DEI_VIEWPORT_SIZE 0x08
#define DEI_VF_SIZE       0x0C
#define DEI_PYF_BA        0x14
#define DEI_CYF_BA        0x18
#define DEI_NYF_BA        0x1C
#define DEI_PCF_BA        0x20
#define DEI_CCF_BA        0x24
#define DEI_NCF_BA        0x28
#define DEI_PMF_BA        0x2C
#define DEI_CMF_BA        0x30
#define DEI_NMF_BA        0x34
#define DEI_YF_STACK_Ln   0x3C
#define DEI_YF_STACK_Pn   0x50
#define DEI_CF_STACK_Ln   0x60
#define DEI_CF_STACK_Pn   0x74
#define DEI_MF_STACK_L0   0x80
#define DEI_MF_STACK_P0   0x84


const unsigned char AMAMAM[] = {0x94,0x25,0x94,0x25,0x94,0xAD,0x94,0xAD,0x94,0x40,0x94,0x40,0xC1,0xCD,0xC1,0xCD,0xC1,0xCD};

/* --- Global variables --- */
char    VINt_Msg[250];            /* text for trace */

typedef struct
{
    U8  DID;
    U8  SDID;
    struct
    {
        U8  Data1;
        U8  Data2;
    }CC_AncillaryData;
    /* ... to be completed if required ...*/
} AncillaryData_t;

typedef struct
{
    long EvtNum;          /* event number */
    STOS_Clock_t EvtTime;      /* time of event detection */
    STVIN_UserData_t EvtData;  /* associated data */
} RcvEventInfo_t ;

typedef struct {
    STVIN_InitParams_t VINInitParams;
    STVIN_Capability_t VINCapability;
    STVIN_OpenParams_t VINOpenParams;
    STVIN_Handle_t VINHndl;
    STVIN_SyncType_t VINSyncType;
    STGXOBJ_ScanType_t VINScanType;
    STVIN_AncillaryDataType_t VINDataType;
    U16 DataCaptureLength;
    STVIN_InputType_t VINInputType;
    STVIN_VideoParams_t VINVideoParams_p;
    STVIN_DefInputMode_t VINDefInputMode;
    STVIN_StartParams_t VINStartParams;
    STVIN_Stop_t VINStopMode;
    STVIN_TermParams_t TermVINParam;
}   TaskParams_t ;

static STEVT_Handle_t  EvtRegHandle;                /* handle for the registrant */
static STEVT_Handle_t  EvtSubHandle;                /* handle for the subscriber */
static STEVT_EventID_t EventID[NB_OF_VIN_EVENTS];   /* event IDs (one ID per event number) */
static RcvEventInfo_t  RcvEventStack[NB_MAX_OF_STACKED_EVENTS]; /* list of event numbers occured */

static short RcvEventCnt[NB_OF_VIN_EVENTS];               /* events counting (one count per event number)*/
static short NbOfRcvEvents=0;                             /* nb of events in stack */
static short NewRcvEvtIndex=0;                            /* top of the stack */
static short LastRcvEvtIndex=0;                           /* bottom of the stack */
static BOOL EvtOpenAlreadyDone=FALSE;              /* flag */
static BOOL EvtEnableAlreadyDone=FALSE;            /* flag */

AncillaryData_t AncillaryData[NB_MAX_OF_STACKED_EVENTS];
U32             AncillaryDataCounter;

/* --- Extern variables --- */

static BOOL VIN_TraceEvtData(S16);

/* Private Function prototypes ---------------------------------------------- */
#if defined (ST_7015) || defined (ST_7020)
static BOOL VIN_ReturnTest       (  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_AllParametersTest( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_ReadSD(            STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_ReadHD(            STTST_Parse_t *pars_p, char *result_sym_p );
static void test_overhead(void *dummy);
static void test_typical(void *dummy);
#endif

static BOOL VIN_EnableEvt ( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_DeleteEvt ( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_DisableEvt( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_ShowEvt   ( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_TestEvt   ( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL VIN_GetEventNumber(STTST_Parse_t *pars_p, STEVT_EventConstant_t *EventNumber);
BOOL VIN_EvtFound(long ExpectedEvt, STOS_Clock_t *EvtArrivalTime_p);
static BOOL VIN_InitCommand2(void);

static int shift( int n)   {   int p;  for ( p=0; n!=0; p++, n>>=1);   return(p);  }

BOOL Test_CmdStart(void);
void os20_main(void *ptr);

/* MACROS ---------------------------------------------------------------- */
#define REPORT_ERROR(err,func) { \
    if ((err) != ST_NO_ERROR) { \
        STTBX_Print(( "ERROR: %s returned %d\n", func, err )); \
    } \
} \

/*#######################################################################*/
/*##########################  SPECIFIC TESTS    #########################*/
/*#######################################################################*/
/* Functions ---------------------------------------------------------------- */

#if defined (ST_7015) || defined (ST_7020)
/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;

    STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};

    Params_p = (TaskParams_t *) dummy;
    Err = ST_NO_ERROR;

/*
 *     vin_init / vin_capa / vin_open / vin_getparams / vin_setparams / vin_inputtype / vin_scantype /
 *     vin_synctype / vin_ancillaryparameters / vin_start / vin_stop / vin_close / vin_term
*/
    strcpy( Params_p->VINInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    strcpy( Params_p->VINInitParams.VidHandlerName, STVID_DEVICE_NAME1);
    strcpy( Params_p->VINInitParams.ClockRecoveryName, STCLKRV_DEVICE_NAME);
    strcpy( Params_p->VINInitParams.InterruptEventName, STINTMR_DEVICE_NAME);
    Params_p->VINInitParams.CPUPartition_p  = SystemPartition_p;
    Params_p->VINInitParams.AVMEMPartition  = AvmemPartitionHandle[0];
    Params_p->VINInitParams.MaxOpen = 1;
#if defined (ST_GX1)
    Params_p->VINInitParams.DeviceType = STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT;
    Params_p->VINInitParams.DeviceBaseAddress_p = 0;
    Params_p->VINInitParams.RegistersBaseAddress_p = 0;
#elif defined (ST_7015)
    Params_p->VINInitParams.DeviceType = STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT;
    Params_p->VINInitParams.DeviceBaseAddress_p = (void *)STI7015_BASE_ADDRESS;
    Params_p->VINInitParams.RegistersBaseAddress_p = (void *)ST7015_SDIN1_OFFSET;
#elif defined (ST_7020)
    Params_p->VINInitParams.DeviceType = STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT;
    Params_p->VINInitParams.DeviceBaseAddress_p = (void *)STI7020_BASE_ADDRESS;
    Params_p->VINInitParams.RegistersBaseAddress_p = (void *)ST7020_SDIN1_OFFSET;
#endif
    Params_p->VINInitParams.InputMode = STVIN_SD_NTSC_720_480_I_CCIR;
    Params_p->VINInitParams.InterruptEvent = 0;

    Params_p->VINInitParams.UserDataBufferSize      = (19*1024);
    Params_p->VINInitParams.UserDataBufferNumber    = 2;

    Err = STVIN_Init( STVIN_DEVICE_NAME, &(Params_p->VINInitParams));
    REPORT_ERROR((Err), ("STVIN_Init"));

/*    Err = STVIN_GetCapability(STVIN_DEVICE_NAME, &(Params_p->VINCapability));
    ReportError(Err, "STVIN_GetCapability");*/

    Err = STVIN_Open(STVIN_DEVICE_NAME, &(Params_p->VINOpenParams), &(Params_p->VINHndl));
    REPORT_ERROR((Err), ("STVIN_Open"));

    Params_p->VINVideoParams_p.InputType                    = STVIN_INPUT_DIGITAL_YCbCr422_8_MULT;
    Params_p->VINVideoParams_p.ScanType                     = STGXOBJ_INTERLACED_SCAN;
    Params_p->VINVideoParams_p.SyncType                     = STVIN_SYNC_TYPE_EXTERNAL;
    Params_p->VINVideoParams_p.FrameWidth                   = 720;
    Params_p->VINVideoParams_p.FrameHeight                  = 480;
    Params_p->VINVideoParams_p.HorizontalBlankingOffset     = 0;
    Params_p->VINVideoParams_p.VerticalBlankingOffset       = 0;
    Params_p->VINVideoParams_p.HorizontalLowerLimit         = 0;
    Params_p->VINVideoParams_p.HorizontalUpperLimit         = 0x210;
    Params_p->VINVideoParams_p.HorizontalSyncActiveEdge     = STVIN_RISING_EDGE;
    Params_p->VINVideoParams_p.VerticalSyncActiveEdge       = STVIN_RISING_EDGE;
    Params_p->VINVideoParams_p.HSyncOutHorizontalOffset     = 0;
    Params_p->VINVideoParams_p.HorizontalVSyncOutLineOffset = 0;
    Params_p->VINVideoParams_p.VerticalVSyncOutLineOffset   = 0;
    Params_p->VINVideoParams_p.ExtraParams.StandardDefinitionParams.StandardType               = STVIN_STANDARD_NTSC;
    Params_p->VINVideoParams_p.ExtraParams.StandardDefinitionParams.AncillaryDataType          = STVIN_ANC_DATA_RAW_CAPTURE;
    Params_p->VINVideoParams_p.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength = 0;
    Err = STVIN_SetParams( Params_p->VINHndl, &Params_p->VINVideoParams_p);
    REPORT_ERROR((Err), ("STVIN_SetParams"));

    Params_p->VINInputType = STVIN_INPUT_DIGITAL_YCbCr422_8_MULT;
    Err = STVIN_SetInputType( Params_p->VINHndl, Params_p->VINInputType);
    REPORT_ERROR((Err), ("STVIN_SetInputType"));

    Params_p->VINScanType = STGXOBJ_INTERLACED_SCAN;
    Err = STVIN_SetScanType( Params_p->VINHndl, Params_p->VINScanType);
    REPORT_ERROR((Err), ("STVIN_SetScanType"));

    Params_p->VINSyncType = STVIN_SYNC_TYPE_CCIR;
    Err = STVIN_SetSyncType( Params_p->VINHndl, Params_p->VINSyncType);
    REPORT_ERROR((Err), ("STVIN_SetSyncType"));

    Params_p->VINDataType = STVIN_ANC_DATA_RAW_CAPTURE;
    Params_p->DataCaptureLength = 0;
    Err = STVIN_SetAncillaryParameters( Params_p->VINHndl, Params_p->VINDataType, Params_p->DataCaptureLength);
    REPORT_ERROR((Err), ("STVIN_SetAncillaryParameters"));

    Err = STVIN_GetParams( Params_p->VINHndl, &Params_p->VINVideoParams_p, &Params_p->VINDefInputMode);
    REPORT_ERROR((Err), ("STVIN_GetParams"));

    Err = STVIN_Start( Params_p->VINHndl, &Params_p->VINStartParams);
    REPORT_ERROR((Err), ("STVIN_Start"));

    Params_p->VINStopMode = STVIN_STOP_NOW;
    Err = STVIN_Stop ( Params_p->VINHndl, Params_p->VINStopMode);
    REPORT_ERROR((Err), ("STVIN_Stop"));

    Err = STVIN_Close(Params_p->VINHndl);
    REPORT_ERROR((Err), ("STVIN_Close"));

    Params_p->TermVINParam.ForceTerminate = FALSE;
    Err = STVIN_Term( STVIN_DEVICE_NAME, &(Params_p->TermVINParam));
    REPORT_ERROR((Err), ("STVIN_Term"));
}

void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;
    Err = ST_NO_ERROR;

    Params_p = NULL;

   REPORT_ERROR((Err), ("test_overhead"));
}
#endif /*(ST_7015) || (ST_7020)*/

/*#######################################################################*/
/*################### CALLBACK FUNCTIONS FOR EVENT TESTS ################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VIN_ReceiveEvt
 *            Log the occured event (callback function)
 * Input    :
 * Output   : RcvEventStack[]
 *            NewRcvEvtIndex
 *            NbOfRcvEvents
 *            RcvEventCnt[]
 * Return   :
 * ----------------------------------------------------------------------*/
static void VIN_ReceiveEvt ( STEVT_CallReason_t Reason,
                             const ST_DeviceName_t RegistrantName,
                             STEVT_EventConstant_t Event,
                             void *EventData,
                             const void *SubscriberData_p )
{
    U8  Data1     =  0;
    U8  Data2     =  0;
    S16 CountIndex = -2;

    const U8 *UserData_p;
    STVIN_UserData_t * EvtData = (STVIN_UserData_t *) EventData;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(SubscriberData_p);
/*    STTBX_Print((">>   VIN_ReceiveEvt() : ... \n"));*/
    if ( NewRcvEvtIndex >= NB_MAX_OF_STACKED_EVENTS ) /* if the stack is full...*/
    {
        return; /* ignore current event */
    }

    switch(Event)
    {
#ifdef VIN_1_0_0A4
        case STVIN_CONFIGURATION_CHANGE_EVT:
            CountIndex = 0;
            break;
#else
        case STVIN_USER_DATA_EVT:

            /* Memorize the event data structure.   */
            RcvEventStack[NewRcvEvtIndex].EvtData = *(EvtData);

            UserData_p = (const U8 *)((STVIN_UserData_t *)EventData)->Buff_p;

            CountIndex = 0;

            /* Fill user data captures. */
            /* First bytes is Data ID value (DID).      */
            AncillaryData[NewRcvEvtIndex].DID = (U8) *((const U8 *)UserData_p);
            /* Test id Type1 or Type2 received.         */
            if ((U8) *((const U8 *)UserData_p) >= 0x80)
            {
                /* Type 1 detected (not expected !!!) */
                AncillaryData[NewRcvEvtIndex].SDID = 0x00; /* not significant byte. */
                break;
            }
            /* Remember the SDID field. */
            AncillaryData[NewRcvEvtIndex].SDID = (U8) *((const U8 *)UserData_p +1);

            /* test SDID field.         */
            switch ((U8) *((const U8 *)UserData_p +1))
            {
                case 0x44 :
                case 0x85 :
                    if ( ((U8) *((const U8 *)UserData_p + 3)) != 0x80)
                    {
                        Data1 = 0;
                    }
                    /* Close caption data received. Memorize corresponding data.    */
                    Data1 += ( (((U8) *((const U8 *)UserData_p + 3)) & 0x0f) << 0);
                    Data1 += ( (((U8) *((const U8 *)UserData_p + 4)) & 0x0f) << 4);

                    Data2 += ( (((U8) *((const U8 *)UserData_p + 5)) & 0x0f) << 0);
                    Data2 += ( (((U8) *((const U8 *)UserData_p + 6)) & 0x0f) << 4);

                    AncillaryData[NewRcvEvtIndex].CC_AncillaryData.Data1 = Data1;
                    AncillaryData[NewRcvEvtIndex].CC_AncillaryData.Data2 = Data2;
                    break;

                default :
                    /* Other cases are not managed for the moment.                  */
                    break;
            }
            break;
#endif
        default:
            CountIndex = -1; /* unknown video event */
    }

    RcvEventStack[NewRcvEvtIndex].EvtNum    = Event;
    RcvEventStack[NewRcvEvtIndex].EvtTime   = STOS_time_now();
    NewRcvEvtIndex++;
    /* counting */
    if ( CountIndex >= 0 )
    {
        RcvEventCnt[CountIndex]++;
    }
    NbOfRcvEvents++;

} /* end of VIN_ReceiveEvt() */


/*#######################################################################*/
/*########################### EVT FUNCTIONS #############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VIN_GetEventNumber
 *            Get the event number or name
 *            If it is a name, convert it into a number
 *            If nothing, return the number 0 (it means "all events")
 * Input    : pars_p
 * Output   : EventNumber
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL VIN_GetEventNumber(STTST_Parse_t *pars_p, STEVT_EventConstant_t *EventNumber)
{
    BOOL RetErr;
    STEVT_EventConstant_t EvtNo;
    S32 SVar;
    char ItemEvtNo[80];

    memset(ItemEvtNo, 0, sizeof(ItemEvtNo));
    EvtNo = 0;

    /* Get event name or number */

    RetErr = STTST_GetItem( pars_p, "", ItemEvtNo, sizeof(ItemEvtNo));
    if ( strlen(ItemEvtNo)>0 )
    {
        RetErr = STTST_EvaluateInteger( ItemEvtNo, &SVar, 10);
        EvtNo = (STEVT_EventConstant_t)SVar;
        if ( RetErr == TRUE )
        {
#ifdef VIN_1_0_0A4
            if ( strcmp(ItemEvtNo,"STVIN_CONFIGURATION_CHANGE_EVT")==0 )
            {
                EvtNo = STVIN_CONFIGURATION_CHANGE_EVT;
            }
            else
            {
                STTST_TagCurrentLine( pars_p, "expected event name with no quotes,like STVIN_CONFIGURATION_CHANGE_EVT, or event number" );
                return(API_EnableError ? TRUE : FALSE );
            }
#else
            if ( strcmp(ItemEvtNo,"STVIN_USER_DATA_EVT")==0 )
            {
                EvtNo = STVIN_USER_DATA_EVT;
            }
            else
            {
                STTST_TagCurrentLine( pars_p, "expected event name with no quotes,like STVIN_USER_DATA_EVT, or event number" );
                return(API_EnableError ? TRUE : FALSE );
            }
#endif
        } /* end if it is an event name */
        if ( EvtNo!=0 && (EvtNo<STVIN_DRIVER_BASE && EvtNo>=STVIN_DRIVER_BASE+NB_OF_VIN_EVENTS) )
        {
            sprintf( VINt_Msg,
                     "expected event number (%d to %d), or name, or none if all events are requested",
                     STVIN_DRIVER_BASE, STVIN_DRIVER_BASE+NB_OF_VIN_EVENTS-1 );
            STTST_TagCurrentLine( pars_p, VINt_Msg );
            return(API_EnableError ? TRUE : FALSE );
        }
    } /* if length not null */

    *EventNumber = EvtNo;
    return( FALSE );

} /* end of VIN_GetEventNumber() */

/*-------------------------------------------------------------------------
 * Function : VIN_DeleteEvt
 *            Reset the contents of the event stack
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_DeleteEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    if ( EvtEnableAlreadyDone == FALSE )
    {
        NewRcvEvtIndex = 0;
        LastRcvEvtIndex = 0;
        NbOfRcvEvents = 0;
        memset(RcvEventStack, 0, sizeof(RcvEventStack));
        memset(RcvEventCnt, 0, sizeof(RcvEventCnt));
        STTBX_Print((">>   VIN_DeleteEvt()  : events stack purged \n"));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print((">>   VIN_DeleteEvt()  : events stack is in used (evt. not disabled) ! \n"));
        RetErr = TRUE;
    }
    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_DeleteEvt() */

/*-------------------------------------------------------------------------
 * Function : VIN_DisableEvt
 *            Disable one event (unsubscribe)
 *            or close the event handler (unsubscribe all events/close/term)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_DisableEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    S16 Count;
    STEVT_EventConstant_t EvtNo;

    UNUSED_PARAMETER(result_sym_p);

    /* Get event name or number, change it into a number */
    RetErr = VIN_GetEventNumber(pars_p, &EvtNo);
    if ( RetErr == TRUE )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    /* Disable all unreleased events */

    if ( EvtNo == 0 ) /* all events */
    {
        RetErr = ST_NO_ERROR;
#ifdef VIN_1_0_0A4
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, STVIN_DEVICE_NAME,
                            (STEVT_EventConstant_t)STVIN_CONFIGURATION_CHANGE_EVT);
#else
        RetErr |= STEVT_UnsubscribeDeviceEvent( EvtSubHandle, STVIN_DEVICE_NAME,
                            (STEVT_EventConstant_t)STVIN_USER_DATA_EVT);
#endif

        if ( RetErr != ST_NO_ERROR )
        {
            STTBX_Print((">>   VIN_DisableEvt() : unsubscribe event error %d !\n", RetErr));
        }
        else
        {
            STTBX_Print((">>   VIN_DisableEvt() : unsubscribe events done\n"));
        }

        /* Close */

        RetErr = STEVT_Close(EvtSubHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            sprintf(VINt_Msg, ">>   VIN_DisableEvt() : close subscriber evt. handler error %d !\n", RetErr);
            STTBX_Print((VINt_Msg));
        }
        else
        {
            STTBX_Print( (">>   VIN_DisableEvt() : close subscriber evt. handler done\n"));
            EvtEnableAlreadyDone = FALSE;
            EvtOpenAlreadyDone = FALSE;
            for ( Count=0 ; Count<NB_OF_VIN_EVENTS ; Count++ )
            {
                EventID[Count] = (U32)-1;
            }
        }
    }
    else
    {
        /* Disable only one event */

        RetErr = STEVT_UnsubscribeDeviceEvent( EvtSubHandle, STVIN_DEVICE_NAME,
                        (U32)EvtNo);
        if ( RetErr != ST_NO_ERROR )
        {
            sprintf(VINt_Msg, ">>   VID_DisableEvt() : unsubscribe event %d error %d !\n", EvtNo, RetErr);
            STTBX_Print((VINt_Msg));
        }
        else
        {
            sprintf(VINt_Msg, ">>   VIN_DisableEvt() : unsubscribe event %d done\n", EvtNo);
            STTBX_Print((VINt_Msg));
            EvtEnableAlreadyDone = FALSE;
        }
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_DisableEvt() */


/*-------------------------------------------------------------------------
 * Function : VIN_EnableEvt
 *            If not already done, init the event handler (init/open/reg)
 *            If no argument, enable all events (subscribe)
 *                      else, enable one event (subscribe)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_EnableEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STEVT_InitParams_t InitParams;
    STEVT_OpenParams_t OpenParams;
    STEVT_DeviceSubscribeParams_t DevSubscribeParams;
    STEVT_EventConstant_t EvtNo;

    UNUSED_PARAMETER(result_sym_p);

    /* Get event name or number, change it into a number */
    RetErr = VIN_GetEventNumber(pars_p, &EvtNo);
    if ( RetErr == TRUE )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    /* --- Initialization --- */

    if ( !EvtOpenAlreadyDone )
    {
        /* Init. the event handler */

        InitParams.ConnectMaxNum = NB_OF_VIN_EVENTS;
        InitParams.EventMaxNum = NB_OF_VIN_EVENTS;
        InitParams.SubscrMaxNum = NB_OF_VIN_EVENTS;
        RetErr = ST_NO_ERROR ;

        /* Open the event handler for the subscriber */

        RetErr = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtSubHandle);
        if ( RetErr!= ST_NO_ERROR )
        {
            sprintf(VINt_Msg, ">>   VIN_EnableEvt()  : STEVT_Open() error %d for subscriber !\n", RetErr);
            STTBX_Print((VINt_Msg));
        }
        else
        {
            EvtOpenAlreadyDone = TRUE;
            STTBX_Print((">>   VIN_EnableEvt()  : STEVT_Open() for subscriber done\n"));
        }

    } /* end if init not done */

    /* --- Subscription --- */

    if ( RetErr==ST_NO_ERROR && EvtOpenAlreadyDone)
    {
        /* event reception in a stack (based on the selected event(s) */
        DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VIN_ReceiveEvt;
        DevSubscribeParams.SubscriberData_p = NULL;
        if ( EvtNo == 0 ) /* if all events required ... */
        {
#ifdef VIN_1_0_0A4
            RetErr =  STEVT_SubscribeDeviceEvent(EvtSubHandle, STVIN_DEVICE_NAME,
                            (STEVT_EventConstant_t)STVIN_CONFIGURATION_CHANGE_EVT, &DevSubscribeParams);
#else
            RetErr =  STEVT_SubscribeDeviceEvent(EvtSubHandle, STVIN_DEVICE_NAME,
                            (STEVT_EventConstant_t)STVIN_USER_DATA_EVT, &DevSubscribeParams);
#endif
            if ( RetErr != ST_NO_ERROR )
            {
                sprintf(VINt_Msg, ">>   VIN_EnableEvt()  : subscribe events error %d !\n", RetErr);
                STTBX_Print((VINt_Msg));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
                STTBX_Print( (">>   VIN_EnableEvt()  : subscribe events done\n"));
                EvtEnableAlreadyDone = TRUE;
            }
        }
        else /* else only 1 event required... */
        {
            RetErr = STEVT_SubscribeDeviceEvent(EvtSubHandle, STVIN_DEVICE_NAME,
                            (STEVT_EventConstant_t)EvtNo, &DevSubscribeParams);
            if ( RetErr != ST_NO_ERROR )
            {
                sprintf(VINt_Msg, ">>   VIN_EnableEvt()  : subscribe event %d error %d !\n", EvtNo, RetErr);
                STTBX_Print((VINt_Msg));
                EvtEnableAlreadyDone = FALSE;
            }
            else
            {
                sprintf(VINt_Msg, ">>   VIN_EnableEvt()  : subscribe event %d done\n", EvtNo);
                STTBX_Print((VINt_Msg));
                EvtEnableAlreadyDone = TRUE;
            }
        }
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_EnableEvt() */


/*-------------------------------------------------------------------------
 * Function : VIN_NotifyEvt
 *            Notify the specified event
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_NotifyEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STEVT_OpenParams_t OpenParams;
#ifdef VIN_1_0_0A4
    S32 EvtNo = STVIN_CONFIGURATION_CHANGE_EVT;
#else
    S32 EvtNo = STVIN_USER_DATA_EVT;
#endif

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    /* Open the event handler for the registrant */
    RetErr = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtRegHandle);
    if ( RetErr!= ST_NO_ERROR )
    {
        sprintf(VINt_Msg, ">>   VIN_NotifyEvt()  : STEVT_Open() error %d for registrant !\n", RetErr);
        STTBX_Print((VINt_Msg));
    }
    else
    {
        STTBX_Print((">>   VIN_NotifyEvt()  : STEVT_Open() for registrant done\n"));
    }

    /**/
#ifdef VIN_1_0_0A4
    RetErr = STEVT_RegisterDeviceEvent(EvtRegHandle, STVIN_DEVICE_NAME, STVIN_CONFIGURATION_CHANGE_EVT, &EventID[0]);
#else
    RetErr = STEVT_RegisterDeviceEvent(EvtRegHandle, STVIN_DEVICE_NAME, STVIN_USER_DATA_EVT, &EventID[0]);
#endif
    if ( RetErr!= ST_NO_ERROR )
    {
        sprintf(VINt_Msg, ">>   VIN_NotifyEvt()  : STEVT_RegisterDeviceEvent() error %d !\n", RetErr);
        STTBX_Print((VINt_Msg));
    }
    else
    {
        STTBX_Print((">>   VIN_NotifyEvt()  : STEVT_RegisterDeviceEvent() done\n"));
    }

    /* event to notify */
    RetErr = STEVT_Notify(EvtRegHandle, EventID[0] , (const void *)NULL);
    if ( RetErr!= ST_NO_ERROR )
    {
        sprintf(VINt_Msg, ">>   VIN_NotifyEvt() : evt. %d notification error %d !\n", EvtNo, RetErr);
        STTBX_Print((VINt_Msg));
    }

    /**/
#ifdef VIN_1_0_0A4
    RetErr = STEVT_UnregisterDeviceEvent(EvtRegHandle, STVIN_DEVICE_NAME, STVIN_CONFIGURATION_CHANGE_EVT);
#else
    RetErr = STEVT_UnregisterDeviceEvent(EvtRegHandle, STVIN_DEVICE_NAME, STVIN_USER_DATA_EVT);
#endif
    if ( RetErr!= ST_NO_ERROR )
    {
        sprintf(VINt_Msg, ">>   VIN_NotifyEvt()  : STEVT_UnregisterDeviceEvent() error %d !\n", RetErr);
        STTBX_Print((VINt_Msg));
    }
    else
    {
        STTBX_Print((">>   VIN_NotifyEvt()  : STEVT_UnregisterDeviceEvent() done\n"));
    }

    /* Close the event handler for the registrant */
    RetErr = STEVT_Close(EvtRegHandle);
    if ( RetErr!= ST_NO_ERROR )
    {
        sprintf(VINt_Msg, ">>   VIN_NotifyEvt()  : STEVT_Close() error %d for registrant !\n", RetErr);
        STTBX_Print((VINt_Msg));
    }
    else
    {
        STTBX_Print((">>   VIN_NotifyEvt()  : STEVT_Close() for registrant done\n"));
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_NotifyEvt() */


/*-------------------------------------------------------------------------
 * Function : VIN_ShowEvt
 *            Show event counters & stack
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_ShowEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    char Number[10];
    char ShowOption[10];
    S16 Count;

    RetErr = STTST_GetString( pars_p, "", ShowOption, sizeof(ShowOption));
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected option (\"D\"=details, default=summary)" );
    }

    /* --- Statistics (event counts) --- */

#ifdef VIN_1_0_0A4
    sprintf( VINt_Msg, ">>   VIN_ShowEvt()    : configuration change = %d \n", RcvEventCnt[0] );
#else
    sprintf( VINt_Msg, ">>   VIN_ShowEvt()    : user data = %d \n", RcvEventCnt[0] );
#endif
    STTBX_Print((VINt_Msg));
    sprintf( VINt_Msg, ">>                      Nb of stacked events=%d (index %d to %d)\n",
             NbOfRcvEvents, LastRcvEvtIndex, NewRcvEvtIndex );
    STTBX_Print((VINt_Msg));

    /* --- Show events stack --- */

    if ( ShowOption[0]=='d' || ShowOption[0]=='D')
    {
        /* List of events with their data */
        Count = LastRcvEvtIndex;
        while ( Count < NewRcvEvtIndex )
        {
            VIN_TraceEvtData(Count);
            Count++;
        }
    }
    else
    {
        /* List of event numbers */
        strcpy( VINt_Msg, ">>                      Last events =  ");
        Count = 0;
        while ( Count < NbOfRcvEvents )
        {
            if ( Count%10==0 ) /* to avoid error if string too long */
            {
                strcat(VINt_Msg, "\n>>  ");
                STTBX_Print((VINt_Msg));
                VINt_Msg[0] = '\0';
            }
            sprintf( Number, "%ld ", RcvEventStack[LastRcvEvtIndex+Count].EvtNum );
            strcat( VINt_Msg, Number );
            Count++;
        }
        strcat( VINt_Msg, "\n");
        STTBX_Print((VINt_Msg));
    }
    /* number of events remaining in the stack */
    STTST_AssignInteger(result_sym_p, NbOfRcvEvents, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_ShowEvt() */


/*-------------------------------------------------------------------------
 * Function : VIN_TestEvt
 *            Wait for an event in the stack
 *            Test if it is the expected event number
 *            Test if associated are equal to expected values (optionnal)
 *            Purge the event from the stack
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_TestEvt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STEVT_EventConstant_t ReqEvtNo;
    S32 ReqEvtData1;
    S16 Count;

    /* Get event name or number, change it into a number */

    RetErr = VIN_GetEventNumber(pars_p, &ReqEvtNo);
    if ( RetErr == TRUE )
    {
        return(API_EnableError ? TRUE : FALSE );
    }

    if ( RetErr == FALSE )
    {
        /* get data1 */
        RetErr = STTST_GetInteger( pars_p, -1, (S32 *)&ReqEvtData1);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected data1" );
        }
    }

    /* --- Wait until the stack is not empty --- */

    Count = 0;
    while ( NbOfRcvEvents == 0 && Count < 50 ) /* wait the 1st event */
    {
        STOS_TaskDelay(100);
        Count++;
    }
    if ( NbOfRcvEvents == 0 && Count >= 50 )
    {
        sprintf(VINt_Msg, ">>   VIN_TestEvt()    : event %d not happened (timeout) ! \n", ReqEvtNo);
        STTBX_Print((VINt_Msg));
   	    API_ErrorCount++;
        RetErr = TRUE;
    }
    else
    {
        /* --- Is it the expected event ? --- */

        if ( RcvEventStack[LastRcvEvtIndex].EvtNum != (long) ReqEvtNo )
        {
            sprintf(VINt_Msg, ">>   VIN_TestEvt()    : event %ld detected instead of %d ! \n",
                           RcvEventStack[LastRcvEvtIndex].EvtNum, ReqEvtNo);
            STTBX_Print((VINt_Msg));
   		    API_ErrorCount++;
            RetErr = FALSE;
        }
        else
        {
            sprintf(VINt_Msg, ">>   VIN_TestEvt()    : event %d detected \n", ReqEvtNo );
            STTBX_Print((VINt_Msg));

            /* --- Trace the data --- */

            VIN_TraceEvtData(LastRcvEvtIndex);

            /* --- Control of the associated data --- */

            if ( ReqEvtData1 != -1 ) /* is the 1st associated data ok ? */
            {
                switch( ReqEvtNo )
                {
#ifdef VIN_1_0_0A4
                    case STVIN_CONFIGURATION_CHANGE_EVT :
                        STTBX_Print( (">>   VIN_TestEvt()    : no data\n"));
#else
                    case STVIN_USER_DATA_EVT :
#endif
                        STTBX_Print( (">>   VIN_TestEvt()    : no data\n"));
                        RetErr = FALSE;
                        break;
                    default :
                        break;
                }
            } /* end if 1st data */

            if ( ReqEvtData1 != -1 ) /* if data test required... */
            {
                if ( RetErr == FALSE )
                {
                    STTBX_Print( (">>   VIN_TestEvt()    : event data are ok\n"));
                }
                else
                {
                    STTBX_Print( (">>   VIN_TestEvt()    : unexpected event data values !\n"));
  			        API_ErrorCount++;
                }
            }

        } /* end else evt ok */

        /* Purge last element in stack (bottom) */

        STOS_InterruptLock();
        if ( LastRcvEvtIndex < NewRcvEvtIndex )
        {
            LastRcvEvtIndex++;
        }
        NbOfRcvEvents--;
        STOS_InterruptUnlock();

    } /* end if stack not empty */

    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_TestEvt() */

/*-------------------------------------------------------------------------
 * Function : VIN_TestCCData
 *            Test if received data are corresponding to test pattern
 *            (AMAMAM)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_TestCCData(STTST_Parse_t *pars_p, char *result_sym_p)
{
    const unsigned char * ReferencePattern_p;
    const unsigned char * EndReferencePattern_p;
    S16       Count;
    BOOL      RetErr = TRUE;

    UNUSED_PARAMETER(pars_p);

    sprintf( VINt_Msg, "VIN_TestCCData()    :");
    STTBX_Print((VINt_Msg));

    EndReferencePattern_p = &AMAMAM[17];
    if (NbOfRcvEvents != 0)
    {
        /* Parse the array and try to recognise the knwon pattern. */
        ReferencePattern_p = AMAMAM;
        /* List of events with their data */
        Count = LastRcvEvtIndex;
        while ( Count < NewRcvEvtIndex )
        {
            if (AncillaryData[Count].DID != 0)
            {
                if ( (AncillaryData[Count].DID == 0x41) &&
                    ((AncillaryData[Count].SDID== 0x85)||(AncillaryData[Count].SDID== 0x44)) )
                {
                    /* Data received. Check it. */
                    if (ReferencePattern_p == EndReferencePattern_p)
                    {
                        if (AncillaryData[Count].CC_AncillaryData.Data1 == *ReferencePattern_p)
                        {
                            /* Finished.    */
                            RetErr = FALSE;
                            sprintf( VINt_Msg, " OK (Reference pattern found !!!)\n");
                            STTBX_Print((VINt_Msg));
                            break;
                        }
                    }
                    else
                    {
                        if ((AncillaryData[Count].CC_AncillaryData.Data1 == *ReferencePattern_p) &&
                            (AncillaryData[Count].CC_AncillaryData.Data2 == *(ReferencePattern_p+1)) )
                        {
                            ReferencePattern_p += 2;
                            if (ReferencePattern_p > EndReferencePattern_p)
                            {
                                /* Finished.    */
                                RetErr = FALSE;
                                sprintf( VINt_Msg, " OK (Reference pattern <AMAMAM> found !!!)\n");
                                STTBX_Print((VINt_Msg));
                                break;
                            }
                        }
                    }
                }
                else
                {
                    /* Error in parsing.    */
                    ReferencePattern_p = AMAMAM;
                }
            }
            Count ++;
        }
        if (RetErr)
        {
            sprintf( VINt_Msg, " KO (Data parsing error !!!)\n");
            STTBX_Print((VINt_Msg));
            Count = LastRcvEvtIndex;
            while ( Count < NewRcvEvtIndex )
            {
                VIN_TraceEvtData(Count);
                Count++;
            }
        }
    }
    else
    {
        sprintf( VINt_Msg, " KO (No data received !!!)\n");
        STTBX_Print((VINt_Msg));
    }


    STTST_AssignInteger(result_sym_p, RetErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* End of VIN_TestCCData() function. */


/*-------------------------------------------------------------------------
 * Function : VIN_TraceEvtData
 *            Display evt name with its associated data found in the stack
 *
 * Input    : EvtIndex (position in the stack)
 * Output   :
 * Return   : FALSE if no error, TRUE if error
 * ----------------------------------------------------------------------*/
static BOOL VIN_TraceEvtData(S16 EvtIndex)
{
    BOOL RetErr;

    RetErr = FALSE;
    if ( EvtIndex <0 || EvtIndex >= NB_MAX_OF_STACKED_EVENTS )
    {
        return(TRUE);
    }

    /* Event name */
    switch(RcvEventStack[EvtIndex].EvtNum)
    {
#ifdef VIN_1_0_0A4
        case STVIN_CONFIGURATION_CHANGE_EVT:
            sprintf( VINt_Msg, ">>   STVIN_CONFIGURATION_CHANGE_EVT : ");
            break;
#else
        case STVIN_USER_DATA_EVT:
            sprintf( VINt_Msg, ">>   STVIN_USER_DATA_EVT : ");
            break;
#endif
        default:
            sprintf( VINt_Msg, ">>   .trace!... \n");
            break;
    }

    /* Event data */
    switch(RcvEventStack[EvtIndex].EvtNum)
    {
#ifdef VIN_1_0_0A4
        case STVIN_CONFIGURATION_CHANGE_EVT:
            strcat( VINt_Msg, "   no data\n");
#else
        case STVIN_USER_DATA_EVT:
#endif
            sprintf( VINt_Msg, "Length %d, OverFlow %d, Buff_p 0x%xh ",
                    RcvEventStack[EvtIndex].EvtData.Length,
                    RcvEventStack[EvtIndex].EvtData.BufferOverflow,
                    (U32)(RcvEventStack[EvtIndex].EvtData.Buff_p));
            STTBX_Print((VINt_Msg));


            sprintf( VINt_Msg, "DID %02xh, SDID %02xh",
                    AncillaryData[EvtIndex].DID,
                    AncillaryData[EvtIndex].SDID);
            STTBX_Print((VINt_Msg));

            if ( (AncillaryData[EvtIndex].SDID == 0x85) ||
                 (AncillaryData[EvtIndex].SDID == 0x44) )
            {
                /* Close caption data detected. rint it on display : */
                sprintf( VINt_Msg, " [%02xh,%02xh]\n",
                        AncillaryData[EvtIndex].CC_AncillaryData.Data1,
                        AncillaryData[EvtIndex].CC_AncillaryData.Data2);
                STTBX_Print((VINt_Msg));
            }
            else
            {
                sprintf( VINt_Msg, " [--,--]\n");
                STTBX_Print((VINt_Msg));
            }
            break;
        default:
            STTBX_Print((VINt_Msg));
            break;
    }

    return(RetErr);

} /* end of VIN_TraceEvtData() */

/*-------------------------------------------------------------------------
 * Function : VIN_EvtFound
 *            wait and found for the expected event in the event stack
 *
 * Input    : ExpectedEvt
 * Output   : EvtArrivalTime_p
 * Return   : TRUE if not found, FALSE if found
 * ----------------------------------------------------------------------*/
BOOL VIN_EvtFound(long ExpectedEvt, STOS_Clock_t *EvtArrivalTime_p)
{
    BOOL  NotFound;
    BOOL  TimeOut;
    S16 EvtCnt;
    S16 WaitCnt;

    NotFound = TRUE;
    TimeOut = FALSE;

    WaitCnt = 0;
    while ( TimeOut == FALSE && NotFound == TRUE )
    {
        /* search in the stack (if it is not empty) */
        EvtCnt = LastRcvEvtIndex;
        while ( EvtCnt < NewRcvEvtIndex && NotFound == TRUE )
        {
            if ( RcvEventStack[EvtCnt].EvtNum == ExpectedEvt )
            {
                NotFound = FALSE ;
                *EvtArrivalTime_p = RcvEventStack[EvtCnt].EvtTime ;
            }
            EvtCnt++;
        }

        WaitCnt++;
        if ( NotFound == TRUE )
        {
            STOS_TaskDelay(15625/2); /* wait 0.5 sec. */
            if ( WaitCnt >= 10 )
            {
                TimeOut = TRUE;
            }
        }
    }
    return ( NotFound );

} /* end of VIN_EvtFound() */


/*#######################################################################*/
/*##########################  SPECIFIC TESTS    #########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VIN_ReturnTest
 * Description : Bad parameter test
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_ReturnTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
  	char DeviceName[80];
    STVIN_InitParams_t VINInitParams;
    STVIN_OpenParams_t VINOpenParams;
    STVIN_Handle_t VINHndl1;
    STVIN_TermParams_t VINTermParams;
    STVIN_DefInputMode_t DefMode;
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;
    BOOL OnceOneInit = FALSE;

    UNUSED_PARAMETER(pars_p);

    STTBX_Print(( "Call API functions with null pointers...\n" ));
    NbErr = 0;

    sprintf( VINInitParams.EvtHandlerName , STEVT_DEVICE_NAME);
#ifndef VIN_1_0_0A4
    sprintf( VINInitParams.VidHandlerName , STVID_DEVICE_NAME1);
    sprintf( VINInitParams.ClockRecoveryName , STCLKRV_DEVICE_NAME);
    sprintf( VINInitParams.InterruptEventName , STINTMR_DEVICE_NAME);
    VINInitParams.InterruptEvent = 0;
#endif
    VINInitParams.CPUPartition_p = SystemPartition_p;
    VINInitParams.MaxOpen = 1;
    VINInitParams.DeviceBaseAddress_p = (void *)VIN_DEVICE_BASE_ADDRESS;
    VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS_OS;
    VINInitParams.InputMode = STVIN_SD_NTSC_720_480_I_CCIR;
    VINInitParams.DeviceType = VIN_DEVICE_TYPE;
    VINInitParams.AVMEMPartition = AvmemPartitionHandle[0];
    VINInitParams.UserDataBufferSize      = (19*1024);
    VINInitParams.UserDataBufferNumber    = 2;
#if defined(ST_7710)
    VINInitParams.InterruptEvent = ST7710_DVP_INTERRUPT;
#elif defined(ST_7100)
    VINInitParams.InterruptEvent = ST7100_DVP_INTERRUPT;
#elif defined(ST_7109)
    VINInitParams.InterruptEvent = ST7109_DVP_INTERRUPT;
#elif defined(ST_5528)
    VINInitParams.InterruptEvent = ST5528_DVP_INTERRUPT;
#endif

    /* Test STVIN_Init with a NULL pointer parameter */
  	DeviceName[0] = '\0';
    ErrCode = STVIN_Init(DeviceName, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ( NULL,...)            : unexpected return code=%d !\n", ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
    }
    /* Test STVIN_Init with a NULL pointer parameter */
    strcpy(DeviceName , "");
    ErrCode = STVIN_Init(DeviceName, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ( NULL,...)            : unexpected return code=%d !\n", ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
    }

    /* Test STVIN_Init with a NULL pointer parameter */
    sprintf(DeviceName , STVIN_DEVICE_NAME);
    ErrCode = STVIN_Init(DeviceName, NULL);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s', NULL)          : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s', NULL)          : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    /* Init a device to for further tests needing an init device */
    ErrCode = STVIN_Init(DeviceName, &VINInitParams);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "---STVIN_Init('%s')                         : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init('%s')                         : ok\n", STVIN_DEVICE_NAME ));
        /* Test STVIN_GetCapability with a NULL pointer parameter */
        ErrCode = STVIN_GetCapability("", NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_GetCapability( NULL,...)            : unexpected return code=%d \n", ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_GetCapability( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
        }

        /* Test STVIN_GetCapability with a NULL pointer parameter */
        ErrCode = STVIN_GetCapability(STVIN_DEVICE_NAME, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_GetCapability('%s', NULL,...)      : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_GetCapability('%s', NULL,...)      : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
        }

        /* Test STVIN_Open with a NULL pointer parameter */
        ErrCode = STVIN_Open("", &VINOpenParams, &VINHndl1);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_Open         ( NULL,...)            : unexpected return code=%d \n", ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Open         ( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
        }

        /* Test STVIN_Open with a NULL pointer parameter */
        ErrCode = STVIN_Open(STVIN_DEVICE_NAME, NULL, &VINHndl1);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_Open         ('%s', NULL,...)      : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Open         ('%s', NULL,...)      : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
        }

        /* Test STVIN_Open with a NULL pointer parameter */
        ErrCode = STVIN_Open(STVIN_DEVICE_NAME, &VINOpenParams, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_Open         ('%s', ...,NULL)      : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Open         ('%s', NULL,...)      : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
        }

        /* Open device to get a good handle for further tests */
        ErrCode = STVIN_Open(STVIN_DEVICE_NAME, &VINOpenParams, &VINHndl1);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "---STVIN_Open('%s')                         : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Open('%s')                         : ok\n", STVIN_DEVICE_NAME ));
            /* Test function with NULL pointer parameter */

            ErrCode = STVIN_SetParams( VINHndl1, NULL);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "---STVIN_SetParams    ( Hndl, NULL)          : unexpected return code=%d \n", ErrCode ));
                NbErr++;
            }
            else
            {
                STTBX_Print(( "+++STVIN_SetParams    ( Hndl, NULL)          : ST_ERROR_BAD_PARAMETER ok\n"  ));
            }

            ErrCode = STVIN_GetParams( VINHndl1, NULL, &DefMode);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "---STVIN_GetParams    ( Hndl, NULL, ...)     : unexpected return code=%d \n", ErrCode ));
                NbErr++;
            }
            else
            {
                STTBX_Print(( "+++STVIN_GetParams    ( Hndl, NULL, ...)     : ST_ERROR_BAD_PARAMETER ok\n"  ));
            }

            ErrCode = STVIN_Start( VINHndl1, NULL);
            if ( ErrCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "---STVIN_Start        ( Hndl, NULL)          : unexpected return code=%d \n", ErrCode ));
                NbErr++;
            }
            else
            {
                STTBX_Print(( "+++STVIN_Start        ( Hndl, NULL)          : ST_ERROR_BAD_PARAMETER ok\n"  ));
            }
        }

        /* Test STVIN_Term with a NULL pointer parameter */
        ErrCode = STVIN_Term("", &VINTermParams);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_Term         ( NULL, ...)           : unexpected return code=%d \n", ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Term         ( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
        }

        /* Test STVIN_Term with a NULL pointer parameter */
        ErrCode = STVIN_Term(STVIN_DEVICE_NAME, NULL);
        if ( ErrCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "---STVIN_Term         ('%s', NULL)           : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( "+++STVIN_Term         ( NULL,...)            : ST_ERROR_BAD_PARAMETER ok\n" ));
        }
        /* Terminate device use */
        VINTermParams.ForceTerminate = 1;
        ErrCode = STVIN_Term(STVIN_DEVICE_NAME, &VINTermParams);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "---STVIN_Term('%s')                         : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        }
        else
        {
            STTBX_Print(( "+++STVIN_Term('%s')                         : ok\n", STVIN_DEVICE_NAME ));
        }
    }

    /* cases of structures containing a null pointer */
    strcpy( VINInitParams.EvtHandlerName , "");
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Evt   = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Evt   = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    sprintf( VINInitParams.EvtHandlerName , STEVT_DEVICE_NAME);
#ifndef VIN_1_0_0A4
    strcpy( VINInitParams.VidHandlerName , "");
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Vid   = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Vid   = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    sprintf( VINInitParams.VidHandlerName , STVID_DEVICE_NAME1);
/* Clkrv Device Name = NULL is not an error with GX1 */
/*
    sprintf( VINInitParams.ClockRecoveryName , "");
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Clkrv = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Clkrv = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }
    sprintf( VINInitParams.ClockRecoveryName , STCLKRV_DEVICE_NAME);
*/
/* Intmr Device Name = NULL is not an error with GX1 */
/*    sprintf( VINInitParams.InterruptEventName , "");
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Intmr = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Intmr = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    sprintf( VINInitParams.InterruptEventName , STINTMR_DEVICE_NAME);
*/
#endif
    VINInitParams.CPUPartition_p  = NULL;
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(CPUpr = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(CPUPr = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    /* Restore the default value.   */
    VINInitParams.CPUPartition_p = SystemPartition_p;
#if !defined(ST_OSLINUX)
    VINInitParams.AVMEMPartition  = (STAVMEM_PartitionHandle_t) NULL;
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(AVMEM = NULL)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(AVMEM = NULL)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }
#endif /*!ST_OSLINUX*/

    STTBX_Print(( "Call API functions with bad parameters...\n" ));

    /* Restore the default value.   */
    VINInitParams.AVMEMPartition = AvmemPartitionHandle[0];
    VINInitParams.UserDataBufferNumber = 10000; /* Out of boundary test.    */
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Bad buff num)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Bad buff num)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }
    /* Restore the default value.   */
    VINInitParams.UserDataBufferNumber = 3;
    VINInitParams.UserDataBufferSize = 10*1024*024;
    ErrCode = STVIN_Init(STVIN_DEVICE_NAME, &VINInitParams);
    if ( ErrCode != ST_ERROR_BAD_PARAMETER )
    {
        STTBX_Print(( "---STVIN_Init         ('%s',(Bad buff siz)) : unexpected return code=%d \n", STVIN_DEVICE_NAME, ErrCode ));
        NbErr++;
        if ( ErrCode == ST_NO_ERROR ) OnceOneInit = TRUE;
    }
    else
    {
        STTBX_Print(( "+++STVIN_Init         ('%s',(Bad buff siz)) : ST_ERROR_BAD_PARAMETER ok\n", STVIN_DEVICE_NAME ));
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VIN_ReturnTest() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VIN_ReturnTest() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
        RetErr = TRUE;
        if ( OnceOneInit) {
            VINTermParams.ForceTerminate = 1;
            ErrCode = STVIN_Term(STVIN_DEVICE_NAME, &VINTermParams);
        }
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_ReturnTest() */



/*-------------------------------------------------------------------------
 * Function : VIN_AllParametersTest
 * Description :
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_AllParametersTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
  	char DeviceName[80];
    STVIN_InitParams_t VINInitParams;
    STVIN_OpenParams_t VINOpenParams;
    STVIN_Handle_t VINHndl1;
    STVIN_TermParams_t VINTermParams;
    STVIN_InputMode_t InputMode;
    BOOL RetErr = FALSE;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;
    STVIN_VideoParams_t Params;
    STVIN_InputType_t                   InputType/*,  DefInput     */ = STVIN_INPUT_DIGITAL_YCbCr422_8_MULT;
    STGXOBJ_ScanType_t                  ScanType/*,   DefScan      */ = STGXOBJ_INTERLACED_SCAN;
    STVIN_SyncType_t                    SyncType/*,   DefSync      */ = STVIN_SYNC_TYPE_EXTERNAL;
    U16                              /* FrameW,     */DefFrameW     = 704;
    U16                              /* FrameH,     */DefFrameH     = 480;
    U16                              /* HBlankOff,  */DefHBlankOff  = 0;
    U16                              /* VBlankOff,  */DefVBlankOff  = 0;
    U16                              /* HLowLim,    */DefHLowLim    = 0;
    U16                              /* HUpLim,     */DefHUpLim     = 0;
    STVIN_ActiveEdge_t               /* HSync,      */DefHSync      = STVIN_RISING_EDGE;
    STVIN_ActiveEdge_t               /* VSync,      */DefVSync      = STVIN_RISING_EDGE;
    U16                              /* HSyncOutH,  */DefHSyncOutH  = 0;
    U16                              /* HVSyncOutL, */DefHVSyncOutL = 0;
    U16                              /* VVSyncOutL, */DefVVSyncOutL = 0;
    STVIN_Standard_t                 /* Standard,   */DefStandard   = STVIN_STANDARD_NTSC;
    STVIN_AncillaryDataType_t        /* AncData,    */DefAncData    = shift(STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED)-1;
    U16                              /* DataLength, */DefDataLength = 0;
    STVIN_ActiveEdge_t               /* Clock,      */DefClock      = STVIN_RISING_EDGE;
    STVIN_FieldDetectionMethod_t     /* Method,     */DefMethod     = STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING;
    STVIN_PolarityOfUpperFieldOutput_t /*Polarity,   */DefPolarity   = STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH;
    char Msg1[120],Msg2[120], Msg3[120], Msg4[120], Msg5[120],Msg[10];

    UNUSED_PARAMETER(pars_p);

    STTBX_Print(( "Call API functions to test all parameters...\n" ));
    NbErr = 0;

    sprintf(DeviceName , STVIN_DEVICE_NAME);
    VINInitParams.DeviceType = VIN_DEVICE_TYPE;
    VINInitParams.DeviceBaseAddress_p = (void *)VIN_DEVICE_BASE_ADDRESS;
    VINInitParams.AVMEMPartition = AvmemPartitionHandle[0];

    sprintf( VINInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    sprintf( VINInitParams.VidHandlerName, STVID_DEVICE_NAME1);
    sprintf( VINInitParams.ClockRecoveryName, STCLKRV_DEVICE_NAME);
    sprintf( VINInitParams.InterruptEventName, STINTMR_DEVICE_NAME);

    VINInitParams.CPUPartition_p = SystemPartition_p;
    VINInitParams.MaxOpen = 1;



#if defined (ST_7710)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_SD_PAL_768_576_I_CCIR; InputMode++) {
        VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS;
        VINInitParams.InterruptEvent = ST7710_DVP_INTERRUPT;
#elif defined (ST_7100)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_SD_PAL_768_576_I_CCIR; InputMode++) {
        VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS_OS;
        VINInitParams.InterruptEvent = ST7100_DVP_INTERRUPT;
#elif defined (ST_7109)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_SD_PAL_768_576_I_CCIR; InputMode++) {
        VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS_OS;
        VINInitParams.InterruptEvent = ST7109_DVP_INTERRUPT;
#elif defined (ST_7015)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_HD_RGB_640_480_P_EXT; InputMode++) {
        if ( InputMode < STVIN_HD_YCbCr_1920_1080_I_CCIR) {
            VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS;
            VINInitParams.InterruptEvent = ST7015_SDIN1_INT;
        }
        else {
            VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS_HD;
            VINInitParams.InterruptEvent = ST7015_HDIN_INT;
        }
#elif defined (ST_7020)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_HD_RGB_640_480_P_EXT; InputMode++) {
        if ( InputMode < STVIN_HD_YCbCr_1920_1080_I_CCIR) {
            VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS;
            VINInitParams.InterruptEvent = ST7020_SDIN1_INT;
        }
        else {
            VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS_HD;
            VINInitParams.InterruptEvent = ST7020_HDIN_INT;
        }
#elif defined (ST_GX1)
    for ( InputMode = STVIN_SD_NTSC_720_480_I_CCIR; InputMode <= STVIN_SD_PAL_768_576_I_CCIR; InputMode++) {
        VINInitParams.RegistersBaseAddress_p = (void *)VIN_BASE_ADDRESS;
        VINInitParams.InterruptEvent = 1;
#endif
        VINInitParams.InputMode = InputMode;

        /* Init a device to for further tests needing an init device */
        ErrCode = STVIN_Init(DeviceName, &VINInitParams);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "  STVIN_Init() : unexpected return code=%d \n", ErrCode ));
        }
        else
        {
            /* Open device to get a good handle for further tests */
            ErrCode = STVIN_Open( DeviceName, &VINOpenParams, &VINHndl1);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(( "   STVIN_Open() : unexpected return code=%d \n", ErrCode ));
            }
            else
            {
                STTBX_Print(( "    STVIN_Init/Open( %s, DeviceType %2x, InputMode %2x)\n", DeviceName, VINInitParams.DeviceType, VINInitParams.InputMode));
                sprintf ( Msg1, "InputType : ");
                sprintf ( Msg2, "ScanType  : ");
                sprintf ( Msg3, "SyncType  : ");
                sprintf ( Msg4, "SetP.  SD : ");
                sprintf ( Msg5, "SetP.  HD : ");
                for ( InputType = STVIN_INPUT_DIGITAL_YCbCr422_8_MULT; InputType <= STVIN_INPUT_DIGITAL_CD_SERIAL_MULT; InputType++) {
                    for ( ScanType = STGXOBJ_PROGRESSIVE_SCAN; ScanType <= STGXOBJ_INTERLACED_SCAN; ScanType++) {
                        for ( SyncType = STVIN_SYNC_TYPE_EXTERNAL ; SyncType <= STVIN_SYNC_TYPE_CCIR; SyncType++) {
                            sprintf ( Msg, " %1x", InputType);
                            strcat ( Msg1, Msg);
                            sprintf ( Msg, " %1x", ScanType);
                            strcat ( Msg2, Msg);
                            sprintf ( Msg, " %1x", SyncType);
                            strcat ( Msg3, Msg);

                            Params.InputType                    = InputType     ;
                            Params.ScanType                     = ScanType      ;
                            Params.SyncType                     = SyncType      ;
                            Params.FrameWidth                   = DefFrameW     ;
                            Params.FrameHeight                  = DefFrameH     ;
                            Params.HorizontalBlankingOffset     = DefHBlankOff  ;
                            Params.VerticalBlankingOffset       = DefVBlankOff  ;
                            Params.HorizontalLowerLimit         = DefHLowLim    ;
                            Params.HorizontalUpperLimit         = DefHUpLim     ;
                            Params.HorizontalSyncActiveEdge     = DefHSync      ;
                            Params.VerticalSyncActiveEdge       = DefVSync      ;
                            Params.HSyncOutHorizontalOffset     = DefHSyncOutH  ;
                            Params.HorizontalVSyncOutLineOffset = DefHVSyncOutL ;
                            Params.VerticalVSyncOutLineOffset   = DefVVSyncOutL ;

                            Params.ExtraParams.StandardDefinitionParams.StandardType               = DefStandard   ;
                            Params.ExtraParams.StandardDefinitionParams.AncillaryDataType          = DefAncData    ;
                            Params.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength = DefDataLength ;

/*                            STTBX_Print(( "STVIN_SetParams( hnd, InputType %2x, ScanType %2x, SyncType %2x) :", InputType, ScanType, SyncType));
*/                            ErrCode = STVIN_SetParams(VINHndl1, &Params);
/*                            if ( ErrCode != ST_NO_ERROR) {
                                STTBX_Print(( " Error1 %x", ErrCode));
                            }
*/
                            sprintf( Msg, " %1x", ErrCode);
                            strcat ( Msg4, Msg);

                            Params.ExtraParams.HighDefinitionParams.ClockActiveEdge = DefClock    ;
                            Params.ExtraParams.HighDefinitionParams.DetectionMethod = DefMethod   ;
                            Params.ExtraParams.HighDefinitionParams.Polarity        = DefPolarity ;

                            ErrCode = STVIN_SetParams(VINHndl1, &Params);
/*                            if ( ErrCode != ST_NO_ERROR) {
                                STTBX_Print(( " Error2 %x", ErrCode));
                            }
*/
                            sprintf( Msg, " %1x", ErrCode);
                            strcat ( Msg5, Msg);

/*                            STTBX_Print(("\n"));
*/                      } /* SyncType  */
                    } /* ScanType  */
                } /* InputType */
            }

            /* Close device */
            ErrCode = STVIN_Close ( VINHndl1);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(( "   STVIN_Close() : unexpected return code=%d \n", ErrCode ));
            }
            else
            {
                /* Term device */
                ErrCode = STVIN_Term( DeviceName, &VINTermParams);
                if ( ErrCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "   STVIN_Term() : unexpected return code=%d \n", ErrCode ));
                }

            }
            STTBX_Print(("%s\n", Msg1));
            STTBX_Print(("%s\n", Msg2));
            STTBX_Print(("%s\n", Msg3));
            STTBX_Print(("%s\n", Msg4));
            STTBX_Print(("%s\n", Msg5));
        }
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VIN_AllParametersTest() */


/*
 * utilities :
 */

#if defined (ST_7015) || defined (ST_7020)
static BOOL VIN_ReadSD( STTST_Parse_t *pars_p, char *result_sym_p)
{
    long SDn;
    long base = VIN_DEVICE_BASE_ADDRESS;
    U32 tab[24];
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&SDn);
    if ( (RetErr == TRUE) || (SDn < 1 || SDn >2) )
    {
        STTST_TagCurrentLine( pars_p, "SD Input, 1-2");
        return(TRUE);
    }

    switch ( SDn) {
        case 1 :
            base += VIN_BASE_ADDRESS;
            break;
        case 2 :
            base += VIN_BASE_ADDRESS_SD2;
            break;
    }

    STTBX_Print (( "read SD%1d Input register : (0x%8.8x)\n", SDn, base));

    STOS_InterruptLock();
    tab[0]  = *(volatile U32 *)(base + SDINn_RFP   );
    tab[1]  = *(volatile U32 *)(base + SDINn_RCHP  );
    tab[2]  = *(volatile U32 *)(base + SDINn_PFP   );
    tab[3]  = *(volatile U32 *)(base + SDINn_PCHP  );
    tab[4]  = *(volatile U32 *)(base + SDINn_PFLD  );
    tab[5]  = *(volatile U32 *)(base + SDINn_ANC   );
    tab[6]  = *(volatile U32 *)(base + SDINn_DFW   );
    tab[7]  = *(volatile U32 *)(base + SDINn_PFW   );
    tab[8]  = *(volatile U32 *)(base + SDINn_PFH   );
    tab[9]  = *(volatile U32 *)(base + SDINn_DMOD  );
    tab[10] = *(volatile U32 *)(base + SDINn_PMOD  );
    tab[11] = *(volatile U32 *)(base + SDINn_ITM   );
    tab[12] = *(volatile U32 *)(base + SDINn_ITS   );
    tab[13] = *(volatile U32 *)(base + SDINn_STA   );
#if defined (ST_7020)
    tab[14] = *(volatile U32 *)(base + SDINn_CTL  );
    tab[15] = *(volatile U32 *)(base + SDINn_TFO   );
    tab[16] = *(volatile U32 *)(base + SDINn_TFS   );
    tab[17] = *(volatile U32 *)(base + SDINn_BFO   );
    tab[18] = *(volatile U32 *)(base + SDINn_BFS   );
    tab[19] = *(volatile U32 *)(base + SDINn_VSD   );
    tab[20] = *(volatile U32 *)(base + SDINn_HSD   );
    tab[21] = *(volatile U32 *)(base + SDINn_HLL   );
    tab[22] = *(volatile U32 *)(base + SDINn_HLFLN );
    tab[23] = *(volatile U32 *)(base + SDINn_APS   );
#else /* ST_7020 */
    tab[14] = *(volatile U32 *)(base + SDINn_CTRL  );
    tab[15] = *(volatile U32 *)(base + SDINn_LL    );
    tab[16] = *(volatile U32 *)(base + SDINn_UL    );
    tab[17] = *(volatile U32 *)(base + SDINn_LNCNT );
    tab[18] = *(volatile U32 *)(base + SDINn_HOFF  );
    tab[19] = *(volatile U32 *)(base + SDINn_VLOFF );
    tab[20] = *(volatile U32 *)(base + SDINn_VHOFF );
    tab[21] = *(volatile U32 *)(base + SDINn_VBLANK);
    tab[22] = *(volatile U32 *)(base + SDINn_HBLANK);
    tab[23] = *(volatile U32 *)(base + SDINn_LENGTH);
#endif /* ST_7015 */
    STOS_InterruptUnlock();

    STTBX_Print(( " RFP    %8.8x RCHP   %8.8x PFP    %8.8x PCHP   %8.8x PFLD   %8.8x ANC    %8.8x \n", tab[0], tab[1], tab[2], tab[3], tab[4], tab[5] ));
    STTBX_Print(( " DFW    %8.8x PFW    %8.8x PFH    %8.8x DMOD   %8.8x PMOD   %8.8x ITM    %8.8x \n", tab[6], tab[7], tab[8], tab[9], tab[10], tab[11] ));
#if defined (ST_7020)
    STTBX_Print(( " ITS    %8.8x STA    %8.8x CTL    %8.8x TFO    %8.8x TFS    %8.8x BFO    %8.8x \n", tab[12], tab[13], tab[14], tab[15], tab[16], tab[17] ));
    STTBX_Print(( " BFS    %8.8x VSD    %8.8x HSD    %8.8x HLL    %8.8x HLFLN  %8.8x APS    %8.8x \n", tab[18], tab[19], tab[20], tab[21], tab[22], tab[23] ));
#else /* ST_7020 */
    STTBX_Print(( " ITS    %8.8x STA    %8.8x CTRL   %8.8x LL     %8.8x UL     %8.8x LNCNT  %8.8x \n", tab[12], tab[13], tab[14], tab[15], tab[16], tab[17] ));
    STTBX_Print(( " HOFF   %8.8x VLOFF  %8.8x VHOFF  %8.8x VBLANK %8.8x HBLANK %8.8x LENGTH %8.8x \n", tab[18], tab[19], tab[20], tab[21], tab[22], tab[23] ));
#endif /* ST_7015 */

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_ReadSD */
#endif


#if defined (ST_7015) || defined (ST_7020)
static BOOL VIN_ReadHD( STTST_Parse_t *pars_p, char *result_sym_p)
{
    long base = VIN_DEVICE_BASE_ADDRESS + VIN_BASE_ADDRESS_HD;
    U32 tab[26];
    BOOL RetErr;

    STTBX_Print (( "read HD Input register : (0x%8.8x)\n", base));

    STOS_InterruptLock();
    tab[0]  = *(volatile U32 *)(base + HDIN_RFP     );
    tab[1]  = *(volatile U32 *)(base + HDIN_RCHP    );
    tab[2]  = *(volatile U32 *)(base + HDIN_PFP     );
    tab[3]  = *(volatile U32 *)(base + HDIN_PCHP    );
    tab[4]  = *(volatile U32 *)(base + HDIN_PFLD    );
    tab[6]  = *(volatile U32 *)(base + HDIN_DFW     );
    tab[7]  = *(volatile U32 *)(base + HDIN_PFW     );
    tab[8]  = *(volatile U32 *)(base + HDIN_PFH     );
    tab[9]  = *(volatile U32 *)(base + HDIN_DMOD    );
    tab[10] = *(volatile U32 *)(base + HDIN_PMOD    );
    tab[11] = *(volatile U32 *)(base + HDIN_ITM     );
    tab[12] = *(volatile U32 *)(base + HDIN_ITS     );
    tab[13] = *(volatile U32 *)(base + HDIN_STA     );
    tab[14] = *(volatile U32 *)(base + HDIN_CTRL    );
    tab[15] = *(volatile U32 *)(base + HDIN_LL      );
    tab[16] = *(volatile U32 *)(base + HDIN_UL      );
    tab[17] = *(volatile U32 *)(base + HDIN_LNCNT   );
    tab[18] = *(volatile U32 *)(base + HDIN_HOFF    );
    tab[19] = *(volatile U32 *)(base + HDIN_VLOFF   );
    tab[20] = *(volatile U32 *)(base + HDIN_VHOFF   );
    tab[21] = *(volatile U32 *)(base + HDIN_VBLANK  );
    tab[22] = *(volatile U32 *)(base + HDIN_HBLANK  );
    tab[23] = *(volatile U32 *)(base + HDIN_HSIZE   );
    tab[24] = *(volatile U32 *)(base + HDIN_TOP_VEND);
    tab[25] = *(volatile U32 *)(base + HDIN_BOT_VEND);
    STOS_InterruptUnlock();

    STTBX_Print(( " RFP    %8.8x RCHP   %8.8x PFP    %8.8x PCHP   %8.8x PFLD   %8.8x              \n", tab[0], tab[1], tab[2], tab[3], tab[4] ));
    STTBX_Print(( " DFW    %8.8x PFW    %8.8x PFH    %8.8x DMOD   %8.8x PMOD   %8.8x ITM    %8.8x \n", tab[6], tab[7], tab[8], tab[9], tab[10], tab[11] ));
    STTBX_Print(( " ITS    %8.8x STA    %8.8x CTRL   %8.8x LL     %8.8x UL     %8.8x LNCNT  %8.8x \n", tab[12], tab[13], tab[14], tab[15], tab[16], tab[17] ));
    STTBX_Print(( " HOFF   %8.8x VLOFF  %8.8x VHOFF  %8.8x VBLANK %8.8x HBLANK %8.8x HSIZE  %8.8x \n", tab[18], tab[19], tab[20], tab[21], tab[22], tab[23] ));
    STTBX_Print(( " TOP_VE %8.8x BOT_VE %8.8x                                                     \n", tab[24], tab[25]));

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_ReadHD */
#endif /* ST_7015 || ST_7020 */

#if defined (ST_GX1) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
static BOOL VIN_ReadDVP( STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined (ST_GX1)
    long DVPn;
    long base = (long)0xBB420000;
#else
    S32 DVPn = 0;
    U32 base = (U32)VIN_BASE_ADDRESS_OS;
#endif


    U32 tab[39];
    BOOL RetErr = FALSE;

    UNUSED_PARAMETER(result_sym_p);
#if !defined (ST_GX1)
    UNUSED_PARAMETER(pars_p);
#endif /*!ST_GX1*/
#if defined (ST_GX1)
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DVPn);
    if ( (RetErr == TRUE) || (DVPn < 0 || DVPn > 1) )
    {
        STTST_TagCurrentLine( pars_p, "DVP Input, 0-1");
        return(TRUE);
    }

    switch ( DVPn) {
        case 0 :
            base += 0x0;
            break;
        case 1 :
            base += 0x100;
            break;
    }
#endif /* ST_GX1 */

    STTBX_Print (( "read DVP%1d Input register : (0x%8.8x)\n", DVPn, base));

    STOS_InterruptLock();
    tab[0]  = *(volatile U32 *)(base + DVP_CTL  );
    tab[1]  = *(volatile U32 *)(base + DVP_TFO  );
    tab[2]  = *(volatile U32 *)(base + DVP_TFS  );
    tab[3]  = *(volatile U32 *)(base + DVP_BFO  );
    tab[4]  = *(volatile U32 *)(base + DVP_BFS  );
    tab[5]  = *(volatile U32 *)(base + DVP_VTP  );
    tab[6]  = *(volatile U32 *)(base + DVP_VBP  );
    tab[7]  = *(volatile U32 *)(base + DVP_VMP  );
    tab[8]  = *(volatile U32 *)(base + DVP_CVS  );
    tab[9]  = *(volatile U32 *)(base + DVP_VSD  );
    tab[10] = *(volatile U32 *)(base + DVP_HSD  );
    tab[11] = *(volatile U32 *)(base + DVP_HLL  );
    tab[12] = *(volatile U32 *)(base + DVP_HSRC );
    tab[13] = *(volatile U32 *)(base + DVP_HFC0 );
    tab[14] = *(volatile U32 *)(base + DVP_HFC1 );
    tab[15] = *(volatile U32 *)(base + DVP_HFC2 );
    tab[16] = *(volatile U32 *)(base + DVP_HFC3 );
    tab[17] = *(volatile U32 *)(base + DVP_HFC4 );
    tab[18] = *(volatile U32 *)(base + DVP_HFC5 );
    tab[19] = *(volatile U32 *)(base + DVP_HFC6 );
    tab[20] = *(volatile U32 *)(base + DVP_HFC7 );
    tab[21] = *(volatile U32 *)(base + DVP_HFC8 );
    tab[22] = *(volatile U32 *)(base + DVP_HFC9 );
    tab[23] = *(volatile U32 *)(base + DVP_VSRC );
    tab[24] = *(volatile U32 *)(base + DVP_VFC0 );
    tab[25] = *(volatile U32 *)(base + DVP_VFC1 );
    tab[26] = *(volatile U32 *)(base + DVP_VFC2 );
    tab[27] = *(volatile U32 *)(base + DVP_VFC3 );
    tab[28] = *(volatile U32 *)(base + DVP_VFC4 );
    tab[29] = *(volatile U32 *)(base + DVP_VFC5 );
    tab[30] = *(volatile U32 *)(base + DVP_ITM  );
    tab[31] = *(volatile U32 *)(base + DVP_ITS  );
    tab[32] = *(volatile U32 *)(base + DVP_STA  );
    tab[33] = *(volatile U32 *)(base + DVP_LNSTA);
    tab[34] = *(volatile U32 *)(base + DVP_HLFLN);
    tab[35] = *(volatile U32 *)(base + DVP_ABA  );
    tab[36] = *(volatile U32 *)(base + DVP_AEA  );
    tab[37] = *(volatile U32 *)(base + DVP_APS  );
    tab[38] = *(volatile U32 *)(base + DVP_PKZ  );
    STOS_InterruptUnlock();

    STTBX_Print(( " DVP_CTL   %8.8x \n", tab[0]));
    STTBX_Print(( " DVP_TFO   %8.8x \n", tab[1]));
    STTBX_Print(( " DVP_TFS   %8.8x \n", tab[2]));
    STTBX_Print(( " DVP_BFO   %8.8x \n", tab[3]));
    STTBX_Print(( " DVP_BFS   %8.8x \n", tab[4]));
    STTBX_Print(( " DVP_VTP   %8.8x \n", tab[5]));
    STTBX_Print(( " DVP_VBP   %8.8x \n", tab[6]));
    STTBX_Print(( " DVP_VMP   %8.8x \n", tab[7]));
    STTBX_Print(( " DVP_CVS   %8.8x \n", tab[8]));
    STTBX_Print(( " DVP_VSD   %8.8x \n", tab[9]));
    STTBX_Print(( " DVP_HSD   %8.8x \n", tab[10]));
    STTBX_Print(( " DVP_HLL   %8.8x \n", tab[11]));
    STTBX_Print(( " DVP_HSRC  %8.8x \n", tab[12]));
    STTBX_Print(( " DVP_HFC0  %8.8x \n", tab[13]));
    STTBX_Print(( " DVP_HFC1  %8.8x \n", tab[14]));
    STTBX_Print(( " DVP_HFC2  %8.8x \n", tab[15]));
    STTBX_Print(( " DVP_HFC3  %8.8x \n", tab[16]));
    STTBX_Print(( " DVP_HFC4  %8.8x \n", tab[17]));
    STTBX_Print(( " DVP_HFC5  %8.8x \n", tab[18]));
    STTBX_Print(( " DVP_HFC6  %8.8x \n", tab[19]));
    STTBX_Print(( " DVP_HFC7  %8.8x \n", tab[20]));
    STTBX_Print(( " DVP_HFC8  %8.8x \n", tab[21]));
    STTBX_Print(( " DVP_HFC9  %8.8x \n", tab[22]));
    STTBX_Print(( " DVP_VSRC  %8.8x \n", tab[23]));
    STTBX_Print(( " DVP_VFC0  %8.8x \n", tab[24]));
    STTBX_Print(( " DVP_VFC1  %8.8x \n", tab[25]));
    STTBX_Print(( " DVP_VFC2  %8.8x \n", tab[26]));
    STTBX_Print(( " DVP_VFC3  %8.8x \n", tab[27]));
    STTBX_Print(( " DVP_VFC4  %8.8x \n", tab[28]));
    STTBX_Print(( " DVP_VFC5  %8.8x \n", tab[29] ));
    STTBX_Print(( " DVP_ITM   %8.8x \n", tab[30]));
    STTBX_Print(( " DVP_ITS   %8.8x \n", tab[31]));
    STTBX_Print(( " DVP_STA   %8.8x \n", tab[32]));
    STTBX_Print(( " DVP_LNSTA %8.8x \n", tab[33]));
    STTBX_Print(( " DVP_HLFLN %8.8x \n", tab[34]));
    STTBX_Print(( " DVP_ABA   %8.8x \n", tab[35]));
    STTBX_Print(( " DVP_AEA   %8.8x \n", tab[36]));
    STTBX_Print(( " DVP_APS   %8.8x \n", tab[37]));
    STTBX_Print(( " DVP_PKZ   %8.8x \n", tab[38]));

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_ReadDVP */
#endif

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
static BOOL VIN_ReadDISP( STTST_Parse_t *pars_p, char *result_sym_p)
{
    S32 DISPn = 0;
    U32 base = (U32)DISP0_BASE_ADDRESS_OS;

    U32 tab[39];
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DISPn);
    if ( (RetErr == TRUE) || (DISPn < 0 || DISPn > 1) )
    {
        STTST_TagCurrentLine( pars_p, "DISPn Input, 0-1");
        return(TRUE);
    }

    switch ( DISPn) {
        case 0 :
            base += 0x0;
            break;
        case 1 :
            base += 0x1000;
            break;
    }

    STTBX_Print (( "read DISP%1d Input register : (0x%8.8x)\n", DISPn, base));

    STOS_InterruptLock();
    if (DISPn == 0)
    {
#if defined (ST_7109)
        tab[0]  = *(volatile U32 *)(base + DEI_CTL        );
        tab[1]  = *(volatile U32 *)(base + DEI_VIEWPORT_ORIG );
        tab[2]  = *(volatile U32 *)(base + DEI_VIEWPORT_SIZE );
        tab[3]  = *(volatile U32 *)(base + DEI_VF_SIZE       );
        tab[4]  = *(volatile U32 *)(base + DEI_PYF_BA        );
        tab[5]  = *(volatile U32 *)(base + DEI_CYF_BA        );
        tab[6]  = *(volatile U32 *)(base + DEI_NYF_BA        );
        tab[7]  = *(volatile U32 *)(base + DEI_PCF_BA        );
        tab[8]  = *(volatile U32 *)(base + DEI_CCF_BA        );
        tab[9]  = *(volatile U32 *)(base + DEI_NCF_BA        );
        tab[10] = *(volatile U32 *)(base + DEI_PMF_BA        );
        tab[11] = *(volatile U32 *)(base + DEI_CMF_BA        );
        tab[12] = *(volatile U32 *)(base + DEI_NMF_BA        );
        tab[13] = *(volatile U32 *)(base + DEI_YF_STACK_Ln   );
        tab[14] = *(volatile U32 *)(base + DEI_YF_STACK_Pn   );
        tab[15] = *(volatile U32 *)(base + DEI_CF_STACK_Ln   );
        tab[16] = *(volatile U32 *)(base + DEI_CF_STACK_Pn   );
        tab[17] = *(volatile U32 *)(base + DEI_MF_STACK_L0   );
        tab[18] = *(volatile U32 *)(base + DEI_MF_STACK_P0   );
        tab[19] = 0;   /* To remove compilation warning */
#else
        tab[0]  = *(volatile U32 *)(base + DISP_CTRL        );
        tab[1]  = *(volatile U32 *)(base + DISP_LUMA_HSRC   );
        tab[2]  = *(volatile U32 *)(base + DISP_LUMA_VSRC   );
        tab[3]  = *(volatile U32 *)(base + DISP_CHR_HSRC    );
        tab[4]  = *(volatile U32 *)(base + DISP_CHR_VSRC    );
        tab[5]  = *(volatile U32 *)(base + DISP_TARGET_SIZE );
        tab[6]  = *(volatile U32 *)(base + DISP_NLZZD_Y     );
        tab[7]  = *(volatile U32 *)(base + DISP_NLZZD_C     );
        tab[8]  = *(volatile U32 *)(base + DISP_PDELTA      );
        tab[9]  = *(volatile U32 *)(base + DISP_MA_CTRL     );
        tab[10] = *(volatile U32 *)(base + DISP_LUMA_BA     );
        tab[11] = *(volatile U32 *)(base + DISP_CHR_BA      );
        tab[12] = *(volatile U32 *)(base + DISP_PMP         );
        tab[13] = *(volatile U32 *)(base + DISP_LUMA_XY     );
        tab[14] = *(volatile U32 *)(base + DISP_CHR_XY      );
        tab[15] = *(volatile U32 *)(base + DISP_LUMA_SIZE   );
        tab[16] = *(volatile U32 *)(base + DISP_CHR_SIZE    );
        tab[17] = *(volatile U32 *)(base + DISP_HFBA        );
        tab[18] = *(volatile U32 *)(base + DISP_VFBA        );
        tab[19] = *(volatile U32 *)(base + DISP_PKZ         );
#endif
    }
    else
    {
        tab[0]  = *(volatile U32 *)(base + DISP_CTRL        );
        tab[1]  = *(volatile U32 *)(base + DISP_LUMA_HSRC   );
        tab[2]  = *(volatile U32 *)(base + DISP_LUMA_VSRC   );
        tab[3]  = *(volatile U32 *)(base + DISP_CHR_HSRC    );
        tab[4]  = *(volatile U32 *)(base + DISP_CHR_VSRC    );
        tab[5]  = *(volatile U32 *)(base + DISP_TARGET_SIZE );
        tab[6]  = *(volatile U32 *)(base + DISP_NLZZD_Y     );
        tab[7]  = *(volatile U32 *)(base + DISP_NLZZD_C     );
        tab[8]  = *(volatile U32 *)(base + DISP_PDELTA      );
        tab[9]  = *(volatile U32 *)(base + DISP_MA_CTRL     );
        tab[10] = *(volatile U32 *)(base + DISP_LUMA_BA     );
        tab[11] = *(volatile U32 *)(base + DISP_CHR_BA      );
        tab[12] = *(volatile U32 *)(base + DISP_PMP         );
        tab[13] = *(volatile U32 *)(base + DISP_LUMA_XY     );
        tab[14] = *(volatile U32 *)(base + DISP_CHR_XY      );
        tab[15] = *(volatile U32 *)(base + DISP_LUMA_SIZE   );
        tab[16] = *(volatile U32 *)(base + DISP_CHR_SIZE    );
        tab[17] = *(volatile U32 *)(base + DISP_HFBA        );
        tab[18] = *(volatile U32 *)(base + DISP_VFBA        );
        tab[19] = *(volatile U32 *)(base + DISP_PKZ         );
    }
    STOS_InterruptUnlock();

    if (DISPn == 0)
    {
#if defined (ST_7109)
        STTBX_Print(( " DEI_CTL             %8.8x \n", tab[0]));
        STTBX_Print(( " DEI_VIEWPORT_ORIG   %8.8x \n", tab[1]));
        STTBX_Print(( " DEI_VIEWPORT_SIZE   %8.8x \n", tab[2]));
        STTBX_Print(( " DEI_VF_SIZE         %8.8x \n", tab[3]));
        STTBX_Print(( " DEI_PYF_BA          %8.8x \n", tab[4]));
        STTBX_Print(( " DEI_CYF_BA          %8.8x \n", tab[5]));
        STTBX_Print(( " DEI_NYF_BA          %8.8x \n", tab[6]));
        STTBX_Print(( " DEI_PCF_BA          %8.8x \n", tab[7]));
        STTBX_Print(( " DEI_CCF_BA          %8.8x \n", tab[8]));
        STTBX_Print(( " DEI_NCF_BA DVP_VSD  %8.8x \n", tab[9]));
        STTBX_Print(( " DEI_PMF_BA          %8.8x \n", tab[10]));
        STTBX_Print(( " DEI_CMF_BA          %8.8x \n", tab[11]));
        STTBX_Print(( " DEI_NMF_BA          %8.8x \n", tab[12]));
        STTBX_Print(( " DEI_YF_STACK_Ln     %8.8x \n", tab[13]));
        STTBX_Print(( " DEI_YF_STACK_Pn     %8.8x \n", tab[14]));
        STTBX_Print(( " DEI_CF_STACK_Ln     %8.8x \n", tab[15]));
        STTBX_Print(( " DEI_CF_STACK_Pn     %8.8x \n", tab[16]));
        STTBX_Print(( " DEI_MF_STACK_L0     %8.8x \n", tab[17]));
        STTBX_Print(( " DEI_MF_STACK_P0     %8.8x \n", tab[18]));
#else
        STTBX_Print(( " DISP_CTRL         %8.8x \n", tab[0]));
        STTBX_Print(( " DISP_LUMA_HSRC    %8.8x \n", tab[1]));
        STTBX_Print(( " DISP_LUMA_VSRC    %8.8x \n", tab[2]));
        STTBX_Print(( " DISP_CHR_HSRC     %8.8x \n", tab[3]));
        STTBX_Print(( " DISP_CHR_VSRC     %8.8x \n", tab[4]));
        STTBX_Print(( " DISP_TARGET_SIZE  %8.8x \n", tab[5]));
        STTBX_Print(( " DISP_NLZZD_Y      %8.8x \n", tab[6]));
        STTBX_Print(( " DISP_NLZZD_C      %8.8x \n", tab[7]));
        STTBX_Print(( " DISP_PDELTA       %8.8x \n", tab[8]));
        STTBX_Print(( " DISP_MA_CTRL      %8.8x \n", tab[9]));
        STTBX_Print(( " DISP_LUMA_BA      %8.8x \n", tab[10]));
        STTBX_Print(( " DISP_CHR_BA       %8.8x \n", tab[11]));
        STTBX_Print(( " DISP_PMP          %8.8x \n", tab[12]));
        STTBX_Print(( " DISP_LUMA_XY      %8.8x \n", tab[13]));
        STTBX_Print(( " DISP_CHR_XY       %8.8x \n", tab[14]));
        STTBX_Print(( " DISP_LUMA_SIZE    %8.8x \n", tab[15]));
        STTBX_Print(( " DISP_CHR_SIZE     %8.8x \n", tab[16]));
        STTBX_Print(( " DISP_HFBA         %8.8x \n", tab[17]));
        STTBX_Print(( " DISP_VFBA         %8.8x \n", tab[18]));
        STTBX_Print(( " DISP_PKZ          %8.8x \n", tab[19]));
#endif
    }
    else
    {
        STTBX_Print(( " DISP_CTRL         %8.8x \n", tab[0]));
        STTBX_Print(( " DISP_LUMA_HSRC    %8.8x \n", tab[1]));
        STTBX_Print(( " DISP_LUMA_VSRC    %8.8x \n", tab[2]));
        STTBX_Print(( " DISP_CHR_HSRC     %8.8x \n", tab[3]));
        STTBX_Print(( " DISP_CHR_VSRC     %8.8x \n", tab[4]));
        STTBX_Print(( " DISP_TARGET_SIZE  %8.8x \n", tab[5]));
        STTBX_Print(( " DISP_NLZZD_Y      %8.8x \n", tab[6]));
        STTBX_Print(( " DISP_NLZZD_C      %8.8x \n", tab[7]));
        STTBX_Print(( " DISP_PDELTA       %8.8x \n", tab[8]));
        STTBX_Print(( " DISP_MA_CTRL      %8.8x \n", tab[9]));
        STTBX_Print(( " DISP_LUMA_BA      %8.8x \n", tab[10]));
        STTBX_Print(( " DISP_CHR_BA       %8.8x \n", tab[11]));
        STTBX_Print(( " DISP_PMP          %8.8x \n", tab[12]));
        STTBX_Print(( " DISP_LUMA_XY      %8.8x \n", tab[13]));
        STTBX_Print(( " DISP_CHR_XY       %8.8x \n", tab[14]));
        STTBX_Print(( " DISP_LUMA_SIZE    %8.8x \n", tab[15]));
        STTBX_Print(( " DISP_CHR_SIZE     %8.8x \n", tab[16]));
        STTBX_Print(( " DISP_HFBA         %8.8x \n", tab[17]));
        STTBX_Print(( " DISP_VFBA         %8.8x \n", tab[18]));
        STTBX_Print(( " DISP_PKZ          %8.8x \n", tab[19]));
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_ReadDISP */
#endif


#if defined (ST_7015) || defined (ST_7020)
/*******************************************************************************
Name        : VIN_StackUsage
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static BOOL VIN_StackUsage(STTST_Parse_t *pars_p, char *result_sym_p)
{
    tdesc_t tdesc;
    task_t* task_p;
    task_status_t status;
    int overhead_stackused = 0;
    void* stack;
    char *funcname[]= {
        "test_overhead",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;
    TaskParams_t* TaskParams_p;
    ST_ErrorCode_t Error;
/*

#if defined (ST_7015)
    vid_init , "D" 1
    HDLVID1=vid_open "VID"
    layinit "LAYER1" 4 720 480 1 ,
    HDLVPVID1=vid_openvp HDLVID1 "LAYER1"
    vid_setmemprof HDLVID1 720 525 4 1 12
    vid_start HDLVID1
#endif
*/

    STTBX_Print(("*************************************\n"));
    STTBX_Print(("* Stack usage calculation beginning *\n"));
    STTBX_Print(("*************************************\n"));
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        Error = STOS_TaskCreate( func, (void *) TaskParams_p,
                                 DriverPartition_p,
                                 STACK_SIZE,
                                 &stack,
                                 DriverPartition_p,
                                 &task_p,
                                 &tdesc,
                                 MAX_USER_PRIORITY,
                                 "stack_test",
                                 0);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Unable to create Stack test task\n"));
            return(TRUE);
        }

        /* Wait for task to complete */
        STOS_TaskWait(&task_p,  TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, 1);
        /* store overhead value */
        if (i==0)
        {
            STTBX_Print(("*-----------------------------------*\n"));
            overhead_stackused = status.task_stack_used;
            STTBX_Print(("%s \t func=0x%08lx stack = %d bytes used\n", funcname[i], (long) func,
                         status.task_stack_used));
            STTBX_Print(("*-----------------------------------*\n"));
        }
        else
        {
            STTBX_Print(("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", funcname[i], (long) func,
                   status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused));
        }
        /* Tidy up */
        STOS_TaskDelete( task_p, DriverPartition_p, stack, DriverPartition_p);
    }
    STTBX_Print(("*************************************\n"));
    STTBX_Print(("*    Stack usage calculation end    *\n"));
    STTBX_Print(("*************************************\n"));

/*
#if defined (ST_7015)
    vid_stop
    vid_closevp HDLVPVID1
    vid_close HDLVID1
    vid_term "VID"
    if ERRORCODE!=ERR_NO_ERROR
      vin_term "vin1" 1
    end
    layterm "LAYER1" 1
#endif
*/
    return(FALSE );
}
#endif

/*-------------------------------------------------------------------------
 * Function : VIN_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_InitCommand2(void)
{
    BOOL RetErr;

    RetErr  = FALSE;
/* events management */
    RetErr |= STTST_RegisterCommand( "VIN_EVT"      ,VIN_EnableEvt , "<Evt><Flag><TimeOut> Enable 1 or all events (init+open+reg+subs)");
    RetErr |= STTST_RegisterCommand( "VIN_DELEVT"   ,VIN_DeleteEvt , "Delete events stack");
    RetErr |= STTST_RegisterCommand( "VIN_NOEVT"    ,VIN_DisableEvt, "<Evt> Disable 1 or all events (unsubs. or unsubs.+close+term)");
    RetErr |= STTST_RegisterCommand( "VIN_SHOWEVT"  ,VIN_ShowEvt   , "<\"D\"> Show counts and received events or their Details");
    RetErr |= STTST_RegisterCommand( "VIN_TESTEVT"  ,VIN_TestEvt   , "<Evt><Data1><Data2><Data3><Data4> Wait for an event");
    RetErr |= STTST_RegisterCommand( "VIN_TESTCC"   ,VIN_TestCCData, "Test if known pattern was received (AMAMAM)");
    RetErr |= STTST_RegisterCommand( "VIN_NOTIFY",   VIN_NotifyEvt, "<Evt> Notify an event");

/* utilities : */
    RetErr |= STTST_RegisterCommand( "VIN_TestRet",  VIN_ReturnTest,"Call API functions with bad parameters.");
    RetErr |= STTST_RegisterCommand( "VIN_AllParam", VIN_AllParametersTest,"Call API functions to test all parameters ");
#if defined (ST_7015) || defined (ST_7020)
    RetErr |= STTST_RegisterCommand( "VIN_ReadSD",   VIN_ReadSD,"Read SD Input registers");
    RetErr |= STTST_RegisterCommand( "VIN_ReadHD",   VIN_ReadHD,"Read HD Input registers");
    RetErr |= STTST_RegisterCommand( "VIN_StackUsage",VIN_StackUsage,"Call stack usage task");
#elif defined (ST_GX1) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    RetErr |= STTST_RegisterCommand( "VIN_ReadDVP",  VIN_ReadDVP,"<port> Read DVP Input registers");
#ifndef ST_GX1
    RetErr |= STTST_RegisterCommand( "VIN_ReadDISP",  VIN_ReadDISP,"<port> Read DISP registers");
#endif
#endif

    return(!RetErr);

} /* end VIN_MacroInit */



/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL Test_CmdStart()
{
    BOOL RetOk;

    RetOk = VID_RegisterCmd2();

    if ( RetOk )
    {
       RetOk = VID_Inj_RegisterCmd();
    }
    /* In the case of Linux we need to initialise the kernel module of vidtest */
#ifdef ST_OSLINUX
    VIDTEST_Init();
#endif

    if ( RetOk )
    {
        RetOk = VIN_InitCommand2();
    }
    if ( RetOk  )
    {
        sprintf(VINt_Msg, "VIN_RegisterCmd2() \t: ok\n");
    }
    else
    {
        sprintf(VINt_Msg, "VIN_RegisterCmd2() \t: failed !\n" );
    }
    STTST_Print((VINt_Msg));

    return(RetOk);
}

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    UNUSED_PARAMETER(ptr);
    STAPIGAT_Init();
    STAPIGAT_Term();

} /* end os20_main */

/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
UNUSED_PARAMETER(argc);
UNUSED_PARAMETER(argv);

#ifdef ST_OS21
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif

    os20_main(NULL);

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* end of file */

