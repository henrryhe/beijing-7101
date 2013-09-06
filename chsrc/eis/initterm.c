/*
  (c) Copyright changhong
  Name:initterm.c
  Description:system initialize section for changhong STI7100 platform

  Authors:yixiaoli
  Date          Remark
  2006-09-04    Created

  Modifiction:

*/

/*include file*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>   

#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"
#include "initterm.h"
#include "RGBdata.h"
#include "..\errors\errors.h"
#include "appltype.h"
 
#include "..\dbase\vdbase.h"
#include "channelbase.h"
#include "..\ch_led\CH_VideoOutput.h"
#include "..\key\keymap.h"
#include "..\qam0297e\ch_tuner.h"
#include "..\qam0297e\ch_tuner_mid.h"
#include "sectionbase.h"

#include "companion_video.h" /*yxl 2007-06-08 add this line*/
#include "companion_audio.h"/* yxl 2007-06-08 add this line*/


#include "..\usif\graphicmid.h"

ST_ErrorCode_t CH6_OpenViewPortStill(char *PICFileName,CH_VideoOutputAspect Ratio,CH_VideoOutputMode HDMode);

#define TEST_AVM 1/*20071015 add*/

#if defined(ST_7100)

#define CLKRV_DEVICE_TYPE_71XX STCLKRV_DEVICE_TYPE_7100

#define VTG_DEVICE_TYPE_VTG_CELL_71XX STVTG_DEVICE_TYPE_VTG_CELL_7100
#define VMIX_GENERIC_GAMMA_TYPE_71XX STVMIX_GENERIC_GAMMA_TYPE_7100
#define VOUT_DEVICE_TYPE_71XX STVOUT_DEVICE_TYPE_7100
#define VID_DEVICE_TYPE_71XX_MPEG STVID_DEVICE_TYPE_7100_MPEG
#define VID_DEVICE_TYPE_71XX_H264 STVID_DEVICE_TYPE_7100_H264
#define AUD_DEVICE_STI71XX STAUD_DEVICE_STI7100
#define BLIT_DEVICE_TYPE_71XX STBLIT_DEVICE_TYPE_GAMMA_7100
#define MAIN_COMPANION_71XX_VIDEO_BXCX_CODE_SIZE MAIN_COMPANION_71XX_VIDEO_BXCX_CODE_SIZE
#define Main_Companion_71XX_Video_BxCx_Code Main_Companion_71XX_Video_BxCx_Code

#if defined(LX_FIRMWARE_FROM_JTAG) 
#else
#include "companion_7100_video_Ax.h"
#include "companion_7100_video_BxCx.h"
#include "companion_7100_audio.h"
#endif
#include "embx_types.h"
#include "embxmailbox.h"
#include "embxshm.h"
#include "mme.h"
#endif

/*yxl 2007-05-24 add below section*/

#if defined(ST_7109)

#define CLKRV_DEVICE_TYPE_71XX STCLKRV_DEVICE_TYPE_7100

#define VTG_DEVICE_TYPE_VTG_CELL_71XX STVTG_DEVICE_TYPE_VTG_CELL_7100
#define VMIX_GENERIC_GAMMA_TYPE_71XX STVMIX_GENERIC_GAMMA_TYPE_7109
#define VOUT_DEVICE_TYPE_71XX STVOUT_DEVICE_TYPE_7100
#define VID_DEVICE_TYPE_71XX_MPEG STVID_DEVICE_TYPE_7109_MPEG
#define VID_DEVICE_TYPE_71XX_H264 STVID_DEVICE_TYPE_7109_H264
#define AUD_DEVICE_STI71XX STAUD_DEVICE_STI7109
#if 1
#define BLIT_DEVICE_TYPE_71XX STBLIT_DEVICE_TYPE_BDISP_7109
#else
#define BLIT_DEVICE_TYPE_71XX STBLIT_DEVICE_TYPE_GAMMA_7100
#endif
#define MAIN_COMPANION_71XX_VIDEO_BXCX_CODE_SIZE MAIN_COMPANION_71XX_VIDEO_BX_CODE_SIZE
#define Main_Companion_71XX_Video_BxCx_Code Main_Companion_71XX_Video_Bx_Code

#if defined(LX_FIRMWARE_FROM_JTAG) 
#else
/*#include "companion_7109_video_Ax.h"
#include "companion_7109_video_Bx.h"
#include "companion_7109_audio.h"*/
#endif
#include "embx_types.h"
#include "embxmailbox.h"
#include "embxshm.h"
#include "mme.h"
#endif
/*end yxl 2007-05-24 add below section*/


#if defined(ST_7100)||defined(ST_7109)
static EMBX_FACTORY          MBX_Factory;
static STAVMEM_BlockHandle_t MBX_AVMEM_Handle;
static U32                   MBX_Address = 0;
static U32                   MBX_Size    = 0;
U8                           LX_VIDEO_Revision[512];
U8                           LX_AUDIO_Revision[512];
#endif

extern int act_src_w,act_src_h;


#ifndef DU32
#pragma ST_device(DU32)
typedef volatile unsigned int DU32;
#endif

/*#define INIT_DEBUG*/

#define YXL_INIT_DEBUG /*yxl 2004-11-12 add this */

#if 1
#define YXL_USE_GFX /*yxl 2005-07-22 add this */
#endif

/*#define USE_STILL    20070824 增加静止层*/


/*#define TSMERGE_BYPASS*/

/*#define FLASH_STFLASH_M29W640DT*/

/*yxl 2006-04-12 add below define*/
/*#define VIDEO_DEBUG_EVENT*/		/*To subscribe all STVID event and print*/
/*#define CLKRV_DEBUG_EVENT*/		/*To subscribe all CLKRV event and print*/
/*#define PTI_DEBUG_EVENT*/		/*To subscribe all PTI event and print*/
/*end yxl 2006-04-12 add below define*/

/*yxl 2007-03-18 add below macros*/

#if 1 /*ok,spasion*/
#define YXL_SPASION_FLASH
#else 

#if 0
#define YXL_STM29_FLASH
#else
#define YXL_STM28_FLASH
#endif

#endif
/*yxl 2007-03-18 add below macros*/
extern  BOOL CH_MessageInit(void);


extern  STTUNER_EventType_t     TunerEvent;
/*end include file*/

STPTI_Buffer_t VIDEOBufHandleTemp;/*20051017 add for get video bufhandle*/
/*global variable*/

STBLIT_Handle_t BLITHndl;

ST_DeviceName_t PIO_DeviceName[] = {"PIO0","PIO1","PIO2","PIO3","PIO4","PIO5"}; 
ST_DeviceName_t UART_DeviceName[]={"UART0","UART1","UART2","UART3"}; 


ST_DeviceName_t EVTDeviceName="Event";
STEVT_Handle_t EVTHandle;

ST_DeviceName_t PWMDeviceName="PWM";
ST_DeviceName_t CLKRVDeviceName="CLKRV";
STCLKRV_Handle_t CLKRVHandle;


ST_DeviceName_t I2C_DeviceName[]={"SSC0_I2C","SSC1_I2C","SSC2_I2C"};

STAVMEM_PartitionHandle_t AVMEMHandle[2];
STFLASH_Handle_t FLASHHandle;


STTUNER_Handle_t TunerHandle;


ST_DeviceName_t DENCDeviceName="DENC";/**/
STDENC_Handle_t DENCHandle;

ST_ErrorCode_t VMIX_SetBKC(VMIX_Device MIXDevice, BOOL enable, S32 red, S32 green, S32 blue );


ST_DeviceName_t VTGDeviceName[2]={"VTGMAIN","VTGAUX"};
STVTG_Handle_t VTGHandle[2];
STVTG_ModeParams_t gVTGModeParam;
STVTG_ModeParams_t gVTGAUXModeParam;


#if 0 /*yxl 2006-12-18*/
ST_DeviceName_t LAYER_DeviceName[]={"GFX1Layer","VIDEO1Layer","GFX2Layer","VIDEO2Layer","BKCOLORLayer"};
#else
ST_DeviceName_t LAYER_DeviceName[]={"VIDEO1Layer","VIDEO2Layer","GFX1Layer","GFX2Layer","STILL1","STILL2"};

#endif

STLAYER_Handle_t LAYERHandle[7];

STAVMEM_SharedMemoryVirtualMapping_t SharedMemoryVirtualMapping;

ST_DeviceName_t VOUTDeviceName[3]={"VOUTMAIN","VOUTAUX","VOUTHDMI"};
STVOUT_Handle_t VOUTHandle[3];


STVMIX_Handle_t VMIXHandle[2];

ST_DeviceName_t VMIXDeviceName[2]={"VMIXMAIN","VMIXAUX"};

ST_DeviceName_t VIDDeviceName[2]={"VIDMPEG","VIDH"};

STBLAST_Handle_t BLASTRXHandle;

semaphore_t* gpSemBLAST;/*用于遥控信号*/

ST_DeviceName_t BLITDeviceName="BLIT";

ST_DeviceName_t AUDDeviceName="AUD";

STGFX_Handle_t GFXHandle;


ST_DeviceName_t PTIDeviceName="PTI";
STPTI_Handle_t PTIHandle;
/*STPTI_Handle_t PTIHandleTemp;yxl 2005-07-26 add this variable*/

STPTI_Slot_t VideoSlot,AudioSlot,PCRSlot;

STAUD_Handle_t AUDHandle;
#if 0 /*yxl 2007-04-19 modify below section*/
STVID_Handle_t VIDHandle[2];
#else
STVID_Handle_t VIDHandle[2]={0,0};
#endif/*end yxl 2007-04-19 modify below section*/

STVID_ViewPortHandle_t VIDVPHandle[2][2];




STVID_ViewPortHandle_t VIDAUXVPHandle;/*yxl 2005-09-11 add this variable*/


CH_ViewPort_t CH6VPOSD;
CH_ViewPort_t CH6VPSTILL;


ST_ClockInfo_t gClockInfo;

STE2P_Handle_t hE2PDevice; 



STGFX_GC_t gGC;

ST_DeviceName_t AVMEMDeviceName[]={"AVMEM0","AVMEM1"};

CH6_ViewPortParams_t* pStillVPParams=NULL;
CH6_ViewPortParams_t* pOSDVPMParams=NULL;
CH6_ViewPortParams_t* pOSDVPPParams=NULL;

U8 BlitterRegister[168]={0};
/*end global variable*/



#define NCACHE_PARTITION_SIZE           (0x00100000+512*1024) /*  1.5 Mbytes */ 
/*static*/ unsigned char    NcacheBlock[NCACHE_PARTITION_SIZE];
#pragma ST_section      (NcacheBlock, "ncache_section")

#if 1
#define CH_NCACHE_PARTITION_SIZE           (0x00300000+0x00300000+0x00400000) /*  2.5 Mbytes */ 
/*static*/ unsigned char    CH_NcacheBlock[CH_NCACHE_PARTITION_SIZE];
#pragma ST_section      (CH_NcacheBlock, "CH_ncache_section")
#endif

#define SYSTEM_PARTITION_SIZE           0x00500000 /* 5 Mbytes */
/*static*/ unsigned char                   SystemBlock [SYSTEM_PARTITION_SIZE];
#pragma ST_section              (SystemBlock,   "system_section")


#ifndef ST_OS21
ST_Partition_t   TheSystemPartition;
ST_Partition_t   TheNcachePartition;
 
ST_Partition_t* NcachePartition = &TheNcachePartition;
ST_Partition_t* CHSysPartition = &TheNcachePartition; /*yxl 2006-04-12 add this line*/
ST_Partition_t* SystemPartition = &TheSystemPartition;
ST_Partition_t* DriverPartition   = &TheSystemPartition;
ST_Partition_t *system_partition = &TheSystemPartition;

#else


ST_Partition_t* NcachePartition =NULL;
ST_Partition_t* CHSysPartition =NULL; 
ST_Partition_t* SystemPartition =NULL;
ST_Partition_t* DriverPartition =NULL; 
ST_Partition_t *system_partition=NULL;

#endif
#if 0
void CH_GetViewPort(STLAYER_ViewPortHandle_t  VPHandle)
{
   STLAYER_ViewPortParams_t    ViewPortParams;
    STLAYER_ViewPortSource_t    Source;
    STGXOBJ_Bitmap_t            BitMap;
    STGXOBJ_Palette_t           Palette;
    STLAYER_ViewPortHandle_t    FilterVPHandle;

   /* Get the viewport parameters */
   ViewPortParams.Source_p = &Source;
   Source.Data.BitMap_p    = &BitMap;
   Source.Palette_p        = &Palette;	
   int i;
   STLAYER_GetViewPortParams(VPHandle,&ViewPortParams);
   i++;
}
#endif
#if 1
/*20070710 add临时内存指针*/
U8 *gpu8_DecompressBuffer=NULL;
#define DecompressBuffer_SIZE (9*1024*512)

unsigned char DecompressBuffer[DecompressBuffer_SIZE+3*1024];
/*#pragma ST_section (DecompressBuffer,"DecompressBuffer")*/

ST_Partition_t       *DecompressBuffer_partition = NULL;
#ifdef LMI_SYS128
/*******************************/
#define APPL_MEM_SIZE (1024)   
#else
#define APPL_MEM_SIZE (1*1024*1024)
#endif
static  unsigned char Appl_Mem[APPL_MEM_SIZE];
unsigned int    gAppAddr = (unsigned int)Appl_Mem;/* wz 20070521 add */
/*#pragma ST_section  (Appl_Mem,"appl_section")*/

ST_Partition_t          *appl_partition = NULL;
#endif

/**/

/*函数名:     void CH_ALL_InitAppPartition(void)
  *开发人员:zengxianggen
  *开发时间:2007/07/13 
  *函数功能:重新得到所有需要使用应用区内存分区指针
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无
  */
/*函数体定义
void CH_ALL_InitAppPartition(void)
{
       //CH_EIT_InitAppPartition();
	//CH_EPG_InitAppPartition();
}*/

void ApplMem_init(void)
{
#ifndef ST_OS21
 	partition_init_heap(appl_partition,(U8*)Appl_Mem,sizeof(Appl_Mem));	
#else
 	appl_partition=partition_create_heap((U8*)Appl_Mem,sizeof(Appl_Mem));	
#endif
       /*20070713 add
        CH_ALL_InitAppPartition();*/
}


void ApplMem_Delete(void)
{
    partition_delete(appl_partition);
}


/*------------------------------------------------------------------------
 * Function : SECTIONS_Setup
 *            setup partition sections;
 * Input    : None
 * Output   :
 * Return   : 
 * ----------------------------------------------------------------------*/
ST_Partition_t *PICPartition=NULL;
BOOL SECTIONS_Setup(void)
{

#ifndef ST_OS21
    partition_init_heap (SystemPartition,SystemBlock,sizeof(SystemBlock));
    partition_init_heap (NcachePartition,(U8 *)((U32)NcacheBlock|0xA0000000),sizeof(NcacheBlock));
#else
    SystemPartition=partition_create_heap (SystemBlock,sizeof(SystemBlock));
    NcachePartition=partition_create_heap ((U8 *)((U32)NcacheBlock|0xA0000000),sizeof(NcacheBlock));

#endif

#if 0
	CHSysPartition=NcachePartition;/*2006-11-30*/
#else
	CHSysPartition=partition_create_heap ((U8 *)((U32)CH_NcacheBlock|0xA0000000),sizeof(CH_NcacheBlock));

#endif

 	DecompressBuffer_partition=partition_create_heap((U8*)((U32)DecompressBuffer|0xA0000000),sizeof(DecompressBuffer));	

   
#ifdef YXL_INIT_DEBUG
	printf("SECTIONS_Setup()=OK\n");
#endif 
    return FALSE;

}


extern const int CacheBaseAddress;/*yxl 2004-08-02 modify this line*/






/* ========================================================================
   Name:        CH_GetChipVersion
   Description: Return the chip version
======================================================================== */

U32 CH_GetChipVersion(void)
{                              
	U32 ChipVersion=0;
	
#ifdef ST_7100
	ChipVersion = *((U32*)(ST71XX_CFG_BASE_ADDRESS+0x000/*DEVICE_ID*/));
	ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0xA0;
	if (ChipVersion==0xC0)
	{
		U32 Value,MetalLayer,LotId;
		Value      = *((DU32*)(ST71XX_CFG_BASE_ADDRESS+0x014/*SYS_STA3*/));
		MetalLayer = (Value&0xC0000000)>>30;
		LotId      = (Value&0x00007FFF)<<3;
		Value      = *((DU32*)(ST71XX_CFG_BASE_ADDRESS+0x010/*SYS_STA2*/));
		LotId      = LotId|(Value>>29);
		if (MetalLayer==0x0)
		{
			if (LotId==0x73EF) MetalLayer=0x1;
		}
		ChipVersion|=MetalLayer;
	}
#endif 
	
#ifdef ST_7109
	ChipVersion = *((DU32*)(ST71XX_CFG_BASE_ADDRESS+0x000/*DEVICE_ID*/));
	ChipVersion = ((((ChipVersion>>28)&0x0F))*0x10)+0xA0;
#endif
	
	/* Return chip version */
	/* ------------------- */
	/* ChipVersion contain the value of the cut */
	/* ChipVersion==0xA0 means cut 1.x          */
	/* ChipVersion==0xB0 means cut 2.x          */
	/* ChipVersion==0xC0 means cut 3.x          */
	/* ChipVersion==0xC2 means cut 3.2          */
	return ChipVersion;
}


/*****************************/
BOOL BOOT_Init( void ) /*71XX*/
{
    ST_ErrorCode_t	ErrCode;
	ST_DeviceName_t	BOOTDeviceName  = "BOOT";
    STBOOT_InitParams_t BootParams;

	memset(&BootParams,0,sizeof(STBOOT_InitParams_t));
	
    BootParams.SDRAMFrequency   = 0;
    BootParams.CacheBaseAddress = (U32*)0;
    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
	BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_0;
	BootParams.ICacheEnabled             = TRUE;
	BootParams.DCacheEnabled             = TRUE;
	BootParams.DCacheMap                 = (void *)0x01; /* Set a non null value to keep Dcache On */
	BootParams.IntTriggerMask            = 0x00000000;
	BootParams.TimeslicingEnabled        = TRUE;
    ErrCode = STBOOT_Init(BOOTDeviceName, &BootParams);
    if (ErrCode != ST_NO_ERROR)
	{
#ifdef YXL_INIT_DEBUG

		 printf("BOOT_Init(%s)=", BOOTDeviceName);
		 printf("%s\n", GetErrorText(ErrCode) );
#endif
	}
#ifdef YXL_INIT_DEBUG
    else
	{
         printf("BOOT_Init(%s)=", BOOTDeviceName);
		printf("%s\n", STBOOT_GetRevision() );

	}	
#endif
    return ( ErrCode == ST_NO_ERROR ? FALSE : TRUE );

} 

/*-------------------------------------------------------------------------
  * Function : PIO_Init
  *            PIO Init function
  * Input    : None
  * Output   :
  * Return   : TRUE if ErrCode, FALSE if success 
* ----------------------------------------------------------------------*/
BOOL PIO_Init( void ) /*71XX*/
{
	#define PIO_COUNT 6
    ST_ErrorCode_t      ErrCode;

    int i;

	STPIO_InitParams_t STPIO_InitParams[] = 
	{
		{ (U32*) PIO_0_BASE_ADDRESS, PIO_0_INTERRUPT, PIO_0_INTERRUPT_LEVEL },
		{ (U32*) PIO_1_BASE_ADDRESS, PIO_1_INTERRUPT, PIO_1_INTERRUPT_LEVEL },
		{ (U32*) PIO_2_BASE_ADDRESS, PIO_2_INTERRUPT, PIO_2_INTERRUPT_LEVEL },
		{ (U32*) PIO_3_BASE_ADDRESS, PIO_3_INTERRUPT, PIO_3_INTERRUPT_LEVEL },
		{ (U32*) PIO_4_BASE_ADDRESS, PIO_4_INTERRUPT, PIO_4_INTERRUPT_LEVEL },
		{ (U32*) PIO_5_BASE_ADDRESS, PIO_5_INTERRUPT, PIO_5_INTERRUPT_LEVEL }
	};

   for (i= 0; i < PIO_COUNT; i++)
   {
        STPIO_InitParams[i].DriverPartition = SystemPartition;
        ErrCode = STPIO_Init( PIO_DeviceName[i], &STPIO_InitParams[i] );
        if (ErrCode != ST_NO_ERROR)
		{
#ifdef YXL_INIT_DEBUG
			printf("PIO_Init(%s)=", PIO_DeviceName[i]);      
			printf("%s\n", GetErrorText(ErrCode) );
#endif
		}
#ifdef YXL_INIT_DEBUG
        else
 		{
			printf("PIO_Init(%s)=", PIO_DeviceName[i]);      
			printf("%s\n", GetErrorText(ErrCode) );
		}
#endif
   }
   
   return ( ErrCode == ST_NO_ERROR ? FALSE : TRUE );

}


BOOL UART_Init()/*uart2*/
{
    ST_ErrorCode_t ErrCode;
    STUART_InitParams_t InitParams;
    STUART_Params_t UartDefaultParams;

	memset((void *)&InitParams, 0, sizeof(STUART_InitParams_t));
    memset((void *)&UartDefaultParams, 0, sizeof(STUART_Params_t));



	UartDefaultParams.RxMode.DataBits = STUART_8BITS_NO_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.RxMode.BaudRate = 115200;  
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.TimeOut = 1; 
    UartDefaultParams.RxMode.NotifyFunction = NULL; 

    UartDefaultParams.TxMode.DataBits = STUART_8BITS_NO_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
    UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
    UartDefaultParams.TxMode.BaudRate = 115200;  
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.TimeOut = 0; 
    UartDefaultParams.TxMode.NotifyFunction = NULL; 

    UartDefaultParams.SmartCardModeEnabled = FALSE;
    UartDefaultParams.GuardTime = 0;
	UartDefaultParams.Retries               = 0;
	UartDefaultParams.NACKSignalDisabled    = FALSE;
	UartDefaultParams.HWFifoDisabled        = FALSE;

    InitParams.UARTType        = STUART_16_BYTE_FIFO;
    InitParams.DriverPartition = SystemPartition;
	InitParams.DefaultParams     = &UartDefaultParams;
    InitParams.BaseAddress     = (U32 *) ASC_2_BASE_ADDRESS;
    InitParams.InterruptNumber = ASC_2_INTERRUPT;
    InitParams.InterruptLevel  =  ASC_2_INTERRUPT_LEVEL; 
#if 0 /*def ST_7100*/
    InitParams.ClockFrequency  = gClockInfo.ckga.com_clk;
#else
    InitParams.ClockFrequency  = gClockInfo.CommsBlock;
#endif
    
    InitParams.SwFIFOEnable    = TRUE;
    InitParams.FIFOLength      = 1024; 

    strcpy(InitParams.RXD.PortName,  PIO_DeviceName[4]);
    InitParams.RXD.BitMask     = PIO_BIT_2;
    strcpy(InitParams.TXD.PortName, PIO_DeviceName[4]);
    InitParams.TXD.BitMask     = PIO_BIT_3;
	
	InitParams.RTS.PortName[0]   = '\0';
	InitParams.CTS.PortName[0]   = '\0';

	ErrCode = STUART_Init(UART_DeviceName[2], &InitParams);
#ifdef YXL_INIT_DEBUG
    printf("STUART_Init()=%s\n", GetErrorText(ErrCode));
#endif
	return ( ErrCode == ST_NO_ERROR ? FALSE : TRUE );




} 

  /*-------------------------------------------------------------------------
  * Function : TBX_Init
  *            Toolbox Init
  * Input    : None
  * Output   :
  * Return   : TRUE if ErrCode, FALSE if success 
* ----------------------------------------------------------------------*/

BOOL TBX_Init( void ) /*71XX*/
{
    ST_ErrorCode_t      ErrCode;
	ST_DeviceName_t TBXDeviceName="TBX";
	
    STTBX_InitParams_t  InitParams;
    InitParams.CPUPartition_p = SystemPartition;
	
    InitParams.SupportedDevices = STTBX_DEVICE_DCU|STTBX_DEVICE_UART; 
	InitParams.DefaultInputDevice 	= STTBX_DEVICE_UART ;
	 
#ifdef DEBUG_INFO_TO_DCU
	InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU; 
#else
	InitParams.DefaultOutputDevice = STTBX_DEVICE_UART; 
#endif 
	
	strcpy(InitParams.UartDeviceName, UART_DeviceName[2]);
	
	ErrCode = STTBX_Init(TBXDeviceName, &InitParams );
#ifdef YXL_INIT_DEBUG
    printf("STTBX_Init()=%s\n", GetErrorText(ErrCode));
#endif
	
    return ( ErrCode == ST_NO_ERROR ? FALSE : TRUE );
} 

/*-------------------------------------------------------------------------
 * Function : EVT_Setup
 *            Event Init function
 * Input    : None
 * Output   :
 * Return   : 
 * ----------------------------------------------------------------------*/

BOOL EVT_Setup(void) /*71XX*/
{
    ST_ErrorCode_t    ErrCode;
    STEVT_InitParams_t InitParams;
    STEVT_OpenParams_t OpenParams;

    memset(&InitParams, '\0', sizeof( STEVT_InitParams_t ) );
    memset(&OpenParams, '\0', sizeof( STEVT_OpenParams_t ) );
	
    InitParams.EventMaxNum     = 60;
    InitParams.ConnectMaxNum   = 100;
    InitParams.SubscrMaxNum    = 150;
    InitParams.MemoryPartition = SystemPartition;
    InitParams.MemorySizeFlag=STEVT_UNKNOWN_SIZE;
    InitParams.MemoryPoolSize=0;

    ErrCode = STEVT_Init(EVTDeviceName, &InitParams);

	if (ErrCode != ST_NO_ERROR) 
	{
		STTBX_Print(("STEVT_Init(%s)=%s\n", EVTDeviceName,GetErrorText(ErrCode)));	
		return TRUE;
	}
  
    ErrCode = STEVT_Open(EVTDeviceName, &OpenParams, &EVTHandle );

	if (ErrCode != ST_NO_ERROR) 
	{
	    STTBX_Print(("STEVT_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}	

    return FALSE;

}


BOOL CLKRV_Setup( void ) /*71XX*/
{
    ST_ErrorCode_t       ErrCode;
    STCLKRV_InitParams_t InitParams;
    STCLKRV_OpenParams_t OpenParams;

	memset(&InitParams ,0,sizeof(STCLKRV_InitParams_t));
    InitParams.DeviceType = CLKRV_DEVICE_TYPE_71XX;
    InitParams.Partition_p = SystemPartition;
    InitParams.InterruptNumber   = ST71XX_MPEG_CLK_REC_INTERRUPT;
    InitParams.InterruptLevel    = DCO_INTERRUPT_LEVEL;/*5;*/
    InitParams.AUDCFGBaseAddress_p = (U32 *)ST71XX_AUDIO_IF_BASE_ADDRESS;
	InitParams.FSBaseAddress_p   = (U32 *)ST71XX_CKG_BASE_ADDRESS;

 	strcpy( InitParams.PCREvtHandlerName, EVTDeviceName  );
    strcpy( InitParams.EVTDeviceName,     EVTDeviceName );
    strcpy( InitParams.PTIDeviceName,     PTIDeviceName );

#if 0 /*yxl 2007-01-28 */
    InitParams.PCRMaxGlitch      = 2;
    InitParams.PCRDriftThres     = 200;
    InitParams.MaxWindowSize     = 30;
    InitParams.MinSampleThres    = 3;
#else
#if 0/*zxg 200806修改改善部分节目无彩色问题*/
	 
	 InitParams.PCRMaxGlitch      = 2;     /* Need 2 glitches before re-baseline              */
	 InitParams.MinSampleThres    = 100;   /* Need to receive 100 PCRs before to run the algo */
	 InitParams.MaxWindowSize     = 500;   /* Filter size is 500 inputs                       */
	 InitParams.PCRDriftThres     = 30000; /* If |((S1-S0)-(P1-P0))|>30000 --> Glitch         */
#else
	 
	 InitParams.PCRMaxGlitch      = 2;     /* Need 3 glitches before re-baseline              */
	 InitParams.MinSampleThres    = 10*10;   /* Need to receive 10 PCRs before to run the algo */
	 InitParams.MaxWindowSize     = 500*2;   /* Filter size is 500 inputs                       */
	 InitParams.PCRDriftThres     =2000*10; /* If |((S1-S0)-(P1-P0))|>6000 --> Glitch         */
	 
#endif
#endif

    ErrCode = STCLKRV_Init(CLKRVDeviceName, &InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print(("CLKRV_Setup(%s)=", CLKRVDeviceName ));        
		STTBX_Print(("%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }
	#ifdef YXL_INIT_DEBUG
	STTBX_Print(("%s\n", STCLKRV_GetRevision() ));
	STTBX_Print(("CLKRV_Setup(%s)=", CLKRVDeviceName ));        
	STTBX_Print(("%s\n", GetErrorText(ErrCode) ));
	#endif

#if 0 /*yxl 2007-01-28 */
	ErrCode = STCLKRV_Open(CLKRVDeviceName, &OpenParams, &CLKRVHandle);
#else
	ErrCode = STCLKRV_Open(CLKRVDeviceName, NULL, &CLKRVHandle);
#endif
	if(ErrCode!=ST_NO_ERROR)
 	{
		STTBX_Print(("CLKRV_Open=",GetErrorText(ErrCode)));
		return TRUE;
	}
	{
#ifdef CLKRV_DEBUG_EVENT /* yxl 2006-04-12 add below section for test*/		

extern void avc_CLKRVEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p);
	STEVT_DeviceSubscribeParams_t SubscribeParams;
	int event=0;

	SubscribeParams.NotifyCallback = avc_CLKRVEvent_CallbackProc;
	for(event =STCLKRV_PCR_VALID_EVT; event <= STCLKRV_PCR_GLITCH_EVT; event++)
	{ 		
		ErrCode = STEVT_SubscribeDeviceEvent(EVTHandle, CLKRVDeviceName, (U32)event, &SubscribeParams);
		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("EVT_Subscribe(%d, Video)=%s\n", event, GetErrorText(ErrCode) ));
			return(ErrCode);
		}
	}    	


#endif /*end  yxl 2006-04-12 add below section for test*/
	}

	ErrCode=STCLKRV_SetSTCSource(CLKRVHandle,STCLKRV_STC_SOURCE_PCR);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CLKRV_Open=",GetErrorText(ErrCode)));
		return TRUE;
	}
	ErrCode=STCLKRV_InvDecodeClk(CLKRVHandle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STCLKRV_InvDecodeClk()=",GetErrorText(ErrCode)));
		return TRUE;
	}

	ErrCode=STCLKRV_SetApplicationMode(CLKRVHandle,STCLKRV_APPLICATION_MODE_NORMAL);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STCLKRV_SetApplicationMode()=",GetErrorText(ErrCode)));
		return TRUE;
	}

 #if 1
    ErrCode = STCLKRV_Enable(CLKRVHandle);
#else
    ErrCode = STCLKRV_Disable(CLKRVHandle);
 
#endif
    if (ErrCode != ST_NO_ERROR)
    {
       
		STTBX_Print(("STCLKRV_Enable=%s\n", GetErrorText(ErrCode) ));
		return TRUE;
	}
    return FALSE;

}


/*-------------------------------------------------------------------------
  * Function : I2C_Init
  *            I2C Init function
  * Input    : None
  * Output   :
  * Return   : TRUE if ErrCode, FALSE if success

* ----------------------------------------------------------------------*/

BOOL I2C_Init( void ) /*71XX*/
{
    ST_ErrorCode_t      ErrCode;
    STI2C_InitParams_t  InitParams;

   /* SSC0 I2C for tuner*/
	memset(&InitParams,0,sizeof(STI2C_InitParams_t));
    InitParams.BaseAddress         = (U32 *)ST71XX_SSC0_BASE_ADDRESS;
    InitParams.InterruptNumber     = ST71XX_SSC0_INTERRUPT;
	InitParams.InterruptLevel      = SSC_0_INTERRUPT_LEVEL;
	InitParams.MasterSlave         = STI2C_MASTER;
    InitParams.BaudRate            = STI2C_RATE_NORMAL;
    strcpy(InitParams.PIOforSDA.PortName, PIO_DeviceName[2]);
    InitParams.PIOforSDA.BitMask   = PIO_BIT_1;
    strcpy(InitParams.PIOforSCL.PortName, PIO_DeviceName[2]);
    InitParams.PIOforSCL.BitMask   = PIO_BIT_0;
    InitParams.MaxHandles          = 5;


#if 0 /*def ST_7100*/
	InitParams.ClockFrequency      = gClockInfo.ckga.com_clk;
#else
    InitParams.ClockFrequency  = gClockInfo.CommsBlock;
#endif

	InitParams.GlitchWidth         = 0;
	InitParams.SlaveAddress        = 0;
	InitParams.FifoEnabled         = FALSE; 
	InitParams.EvtHandlerName[0]   = '\0'; 
	InitParams.DriverPartition     = SystemPartition;
    ErrCode = STI2C_Init(I2C_DeviceName[0], &InitParams);
    if (ErrCode != ST_NO_ERROR)
	{

		STTBX_Print(("STI2C_Init(%s)=\n", I2C_DeviceName[0] ));	
		STTBX_Print(("%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
    else
	{
		STTBX_Print(("STI2C_Init(%s)=\n", I2C_DeviceName[0] ));	
		STTBX_Print(("%s",STI2C_GetRevision()));
	}
#endif

	/* SSC2 I2C for e2p,PIC16F72（前面板）*/
	memset(&InitParams,0,sizeof(STI2C_InitParams_t));
    InitParams.BaseAddress         = (U32 *)ST71XX_SSC2_BASE_ADDRESS;
 
    InitParams.InterruptNumber     = ST71XX_SSC2_INTERRUPT;
	InitParams.InterruptLevel      = SSC_2_INTERRUPT_LEVEL;
    InitParams.MasterSlave         = STI2C_MASTER;
    InitParams.BaudRate            = STI2C_RATE_NORMAL;
	strcpy(InitParams.PIOforSDA.PortName, PIO_DeviceName[4]);
    InitParams.PIOforSDA.BitMask   = PIO_BIT_1;
    strcpy(InitParams.PIOforSCL.PortName, PIO_DeviceName[4]);
    InitParams.PIOforSCL.BitMask   = PIO_BIT_0;
    InitParams.MaxHandles          = 6;


#if 0 /*def ST_7100*/
    InitParams.ClockFrequency      = gClockInfo.ckga.com_clk;
#else
    InitParams.ClockFrequency  = gClockInfo.CommsBlock;
#endif	
	InitParams.GlitchWidth         = 0;
	InitParams.SlaveAddress        = 0;
	InitParams.FifoEnabled         = FALSE; 
	InitParams.EvtHandlerName[0]   = '\0'; 
	InitParams.DriverPartition     = SystemPartition;

    ErrCode = STI2C_Init(I2C_DeviceName[2], &InitParams);
    if (ErrCode != ST_NO_ERROR)
	{
		
		STTBX_Print(("STI2C_Init(%s)=\n", I2C_DeviceName[2] ));	
		STTBX_Print(("%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
    else
	{
		STTBX_Print(("STI2C_Init(%s)=\n", I2C_DeviceName[2] ));	
		STTBX_Print(("%s",STI2C_GetRevision()));
	}
#endif

#if 1 /*def ST_DEMO_PLATFORM yxl 2007-02-01 add below section for HDMI*/
	/* Initialize I2C (1) */
	/* ------------------ */
	memset(&InitParams,0,sizeof(STI2C_InitParams_t));
	InitParams.BaseAddress         = (U32 *)ST71XX_SSC1_BASE_ADDRESS;
	InitParams.InterruptNumber     = ST71XX_SSC1_INTERRUPT;
	InitParams.InterruptLevel      = SSC_1_INTERRUPT_LEVEL;
	InitParams.MasterSlave         = STI2C_MASTER;
	InitParams.BaudRate            = STI2C_RATE_NORMAL;
	InitParams.MaxHandles          = 6;


#if 0 /*def ST_7100*/
	InitParams.ClockFrequency      = gClockInfo.ckga.com_clk;
#else
    InitParams.ClockFrequency  = gClockInfo.CommsBlock;
#endif	
	InitParams.DriverPartition     = SystemPartition;
	InitParams.PIOforSDA.BitMask   = PIO_BIT_1;
	InitParams.PIOforSCL.BitMask   = PIO_BIT_0;
	InitParams.EvtHandlerName[0]   = '\0'; 
	InitParams.GlitchWidth         = 0;
	InitParams.SlaveAddress        = 0;
	InitParams.FifoEnabled         = FALSE; 
	strcpy(InitParams.PIOforSDA.PortName,PIO_DeviceName[3]);
	strcpy(InitParams.PIOforSCL.PortName,PIO_DeviceName[3]);
	ErrCode=STI2C_Init(I2C_DeviceName[1],&InitParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STI2C_Init(%s)=\n", I2C_DeviceName[1] ));	
		STTBX_Print(("%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}

#endif  /*end yxl 2007-02-01 add below section for HDMI*/
 
 
 return  FALSE ;

}


/*-------------------------------------------------------------------------
   * Function : E2P_Init
   *            Initialise EEPROM
   * Input    : None
   * Output   :
   * Return   : TRUE if ErrCode, FALSE if success
 * ----------------------------------------------------------------------*/
#define NVM_APPLICATION_MEM_SIZE   (8 * 1024 - 4) /* in bytes - top 4 bytes for MFG id */
#define  NVM_APPLICATION_MEM_START     0x0000            /* physical addr in the device */
BOOL E2P_Init( void )
{
	 ST_ErrorCode_t          ErrCode;
	 ST_DeviceName_t E2PDeviceName="E2P";
	 STE2P_InitParams_t      InitParams;
	 STE2P_OpenParams_t      OpenParams;
 
	 InitParams.DriverPartition = SystemPartition;
	 InitParams.MemoryWidth = 1;                       
	 InitParams.PagingBytes = STE2P_SIZEOF_PAGE;       
	 InitParams.DeviceCode  = STE2P_SUPPORTED_DEVICE;  
	 InitParams.MaxSimOpen  = 8;                     
	 InitParams.BaseAddress = NULL; 
#ifndef YXL_STM28_FLASH /*yxl 2007-03-07 modiy below section*/	 
	 strcpy(InitParams.I2CDeviceName, I2C_DeviceName[2]);
#else/*below is old 71XXDVBC Board	 */
	 strcpy(InitParams.I2CDeviceName, I2C_DeviceName[1]);
#endif /*yxl 2007-03-07 modiy below section*/
	 InitParams.SizeInBytes = (8*1024 - 4);                /* 24LC64 in use , Top 4 bytes for MFG id */
	 
	 ErrCode = STE2P_Init( E2PDeviceName, &InitParams );
	 if(ErrCode!=ST_NO_ERROR) 
	 {
		 STTBX_Print(("STE2P_Init(%s)=%s\n", E2PDeviceName,GetErrorText(ErrCode)));	
		 return TRUE;	
	 }
#ifdef YXL_INIT_DEBUG
	 STTBX_Print(("STE2P_Init(%s)=%s\n", E2PDeviceName,GetErrorText(ErrCode)));	
#endif
	 
	 OpenParams . Offset = NVM_APPLICATION_MEM_START;
	 OpenParams . Length =NVM_APPLICATION_MEM_SIZE;
	 ErrCode =STE2P_Open(E2PDeviceName,&OpenParams,&hE2PDevice ) ; 
	 if(ErrCode!=ST_NO_ERROR) 
	 {
		 STTBX_Print(("STE2P_Open()=%s",GetErrorText(ErrCode)));	
		 return TRUE;	
	 }
#ifdef YXL_INIT_DEBUG
	 STTBX_Print(("STE2P_Open()=%s",GetErrorText(ErrCode)));	
#endif

	 return FALSE;
} /* E2P_Init() */


 
BOOL FDMA_Init(void) /*71XX*/
{
    ST_ErrorCode_t       ErrCode;
    STFDMA_InitParams_t  FDMA_InitParams;

    memset(&FDMA_InitParams,0,sizeof(STFDMA_InitParams_t));
    FDMA_InitParams.DeviceType          = STFDMA_DEVICE_FDMA_2;
	FDMA_InitParams.DriverPartition_p   = SystemPartition;
	FDMA_InitParams.NCachePartition_p   = NcachePartition;
	FDMA_InitParams.BaseAddress_p       = (void *)ST71XX_FDMA_BASE_ADDRESS;
	FDMA_InitParams.InterruptNumber     = ST71XX_FDMA_MAILBOX_FLAG_INTERRUPT;
	FDMA_InitParams.InterruptLevel      = DMA_INTERRUPT_LEVEL;
	FDMA_InitParams.NumberCallbackTasks = 1;
	FDMA_InitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();
	FDMA_InitParams.FDMABlock           = STFDMA_1;/*yxl 2007-03-03 add this line for 71XXnewdriver*/


	ErrCode = STFDMA_Init("FDMA0", &FDMA_InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STFDMA_Init()=%s",GetErrorText(ErrCode)));	
		return TRUE;	
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STFDMA_Init()=%s",GetErrorText(ErrCode)));
#endif

    return  FALSE;
}


BOOL FLASH_Setup(void)
{

	return CHFLASH_Setup();
}


void CH_71XX_ClockConfig(void)
{

	/* Call kernel clock function to active the timers */
	/* =============================================== */
	clock();
/*	return ;yxl 2007-06-22 temp add this line for test*/
#if 0	
	/* Sys clockgenA and clockgenB are powered by the same clock */
	/* ========================================================= */
	*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
	*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
#else
	/* Sys clockgenA and clockgenB are powered by the different clock */
	/* ========================================================= */
	*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
	*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x0;
#endif
	
	/* H264 preprocessor clock is different between cut 1.x and other cuts */
	/* =================================================================== */
	/* ClockGenA & ClockGenB have to be powered by the same clock - 27Mhz */
#if (STCLKRV_EXT_CLK_MHZ==27)
	if ((CH_GetChipVersion()&0xF0)==0xA0)
	{
		/* Set Pipe clock at 150MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x17;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x7AE1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
	}
	else
	{
		/* Set H264 Preproc Clock at 150MHz - ClockGenB.FS3 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x88) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x80) = 0x17;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x84) = 0x7AE1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x8C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x88) = 0x1;
	}
#endif
	
	/* H264 preprocessor clock is different between cut 1.x and other cuts */
	/* =================================================================== */
	/* ClockGenA & ClockGenB have to be powered by the same clock - 30Mhz */
#if (STCLKRV_EXT_CLK_MHZ==30)||(STCLKRV_EXT_CLK_MHZ==0)
#if 1
	if ((CH_GetChipVersion()&0xF0)==0xA0)
	{
		/* Set Pipe clock at 150MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x19;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x3408;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;  
	}
	else
	{
		/* Set H264 Preproc Clock at 150MHz - ClockGenB.FS3 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x88) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x80) = 0x19;
#if 0 /*yxl 2007-10-17 modify*/
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x84) = 0x3408;
#else
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x84) = 0x3334;
#endif
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x8C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x88) = 0x1;
	}
#endif 
#endif

	return;
}

#ifdef ST_7109
/*-------------------------------------------------------------------------
  * Function : AVMEM_Setup
  *            AVMEM Init & Open functions
  * Input    : None
  * Output   :
  * Return   : TRUE if ErrCode, FALSE if success 
* ----------------------------------------------------------------------*/
BOOL AVMEM_Setup( void ) 
{  
	ST_ErrorCode_t                       ErrCode;
    STAVMEM_InitParams_t                 InitParams;
    STAVMEM_MemoryRange_t                MemoryRange[1];
	STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
	STAVMEM_CreatePartitionParams_t      CreateParams;

	memset(&InitParams       ,0,sizeof(STAVMEM_InitParams_t));
	memset(&VirtualMapping   ,0,sizeof(STAVMEM_SharedMemoryVirtualMapping_t));
	memset(&MemoryRange,0,sizeof(STAVMEM_MemoryRange_t));
	memset(&CreateParams  ,0,sizeof(STAVMEM_CreatePartitionParams_t));


    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *) 0xA4000000;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *) 0x04000000;
	VirtualMapping.PhysicalAddressSeenFromDevice2_p = (void *) 0x04000000;
    VirtualMapping.VirtualBaseAddress_p            = (void *) 0xA4000000;
    VirtualMapping.VirtualSize                     = 0xffffffff;
    VirtualMapping.VirtualWindowOffset             = 0;
    VirtualMapping.VirtualWindowSize               = 0xffffffff;

    InitParams.SharedMemoryVirtualMapping_p = &VirtualMapping;


    InitParams.DeviceType                   = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitParams.CPUPartition_p               = SystemPartition;
	InitParams.NCachePartition_p            = NcachePartition;
    InitParams.MaxPartitions                = 2;
    InitParams.MaxBlocks                    = 200;        
    InitParams.MaxForbiddenRanges           = 3;    
    InitParams.MaxForbiddenBorders          = 3;    
    InitParams.MaxNumberOfMemoryMapRanges   = 2;

    InitParams.OptimisedMemAccessStrategy_p = NULL;
    InitParams.BlockMoveDmaBaseAddr_p       = NULL;
#if 0 /*yxl 2007-10-13 modify below for problem*/
    InitParams.VideoBaseAddr_p              = NULL;
#else
	InitParams.VideoBaseAddr_p              = VIDEO_BASE_ADDRESS;
#endif
    InitParams.CacheBaseAddr_p              = NULL;

    InitParams.NumberOfDCachedRanges        = 0;
    InitParams.DCachedRanges_p              = NULL;

    InitParams.SDRAMBaseAddr_p              = (void *)0xA4000000;
    InitParams.SDRAMSize                    = 0xffffffff;

    memset(InitParams.GpdmaName, 0, sizeof(InitParams.GpdmaName));
 
    ErrCode = STAVMEM_Init(AVMEMDeviceName[0], &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName[0], GetErrorText(ErrCode),
		MemoryRange[0].StartAddr_p,MemoryRange[0].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
    STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName, GetErrorText(ErrCode),
		MemoryRange[0].StartAddr_p,MemoryRange[0].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
#endif 

    MemoryRange[0].StartAddr_p = (void *)(AVMEM1_BASE_ADDRESS);/*0xA6000000*/
    MemoryRange[0].StopAddr_p  = (void *)(AVMEM1_BASE_ADDRESS+AVMEM1_SIZE-1);/*32 Mbytes*/

    CreateParams.PartitionRanges_p       = &MemoryRange;
    CreateParams.NumberOfPartitionRanges = (sizeof(MemoryRange)/sizeof(STAVMEM_MemoryRange_t));
    ErrCode = STAVMEM_CreatePartition(AVMEMDeviceName[0], &CreateParams, &AVMEMHandle[0]);
	if(ErrCode!=ST_NO_ERROR) 
	{
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
#endif

 
      ErrCode = STAVMEM_Init(AVMEMDeviceName[1], &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName[0], GetErrorText(ErrCode),
		MemoryRange[0].StartAddr_p,MemoryRange[0].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
		return TRUE ;
	}

    MemoryRange[0].StartAddr_p = (void *)(AVMEM2_BASE_ADDRESS);/*0xB0000000 */
    MemoryRange[0].StopAddr_p  = (void *)(AVMEM2_BASE_ADDRESS+AVMEM2_SIZE-1);/*64 Mbytes - 1920*32 */

    CreateParams.PartitionRanges_p       = &MemoryRange;
    CreateParams.NumberOfPartitionRanges = (sizeof(MemoryRange)/sizeof(STAVMEM_MemoryRange_t));
    ErrCode = STAVMEM_CreatePartition(AVMEMDeviceName[1], &CreateParams, &AVMEMHandle[1]);
	if(ErrCode!=ST_NO_ERROR) 
	{
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
#endif
		return FALSE;

} /* end of AVMEM_Setup */

#endif 


#ifdef ST_7100

/*-------------------------------------------------------------------------
  * Function : AVMEM_Setup
  *            AVMEM Init & Open functions
  * Input    : None
  * Output   :
  * Return   : TRUE if ErrCode, FALSE if success 
* ----------------------------------------------------------------------*/
BOOL AVMEM_Setup( void ) 
{  
	ST_ErrorCode_t                       ErrCode;
    STAVMEM_InitParams_t                 InitParams;
    STAVMEM_MemoryRange_t                MemoryRange[1];
	STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
	STAVMEM_CreatePartitionParams_t      CreateParams;

	memset(&InitParams       ,0,sizeof(STAVMEM_InitParams_t));
	memset(&VirtualMapping   ,0,sizeof(STAVMEM_SharedMemoryVirtualMapping_t));
	memset(&MemoryRange,0,sizeof(STAVMEM_MemoryRange_t));
	memset(&CreateParams  ,0,sizeof(STAVMEM_CreatePartitionParams_t));  
                  

    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *) 0xA0000000;
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *) NULL;
	VirtualMapping.PhysicalAddressSeenFromDevice2_p = (void *) NULL;
    VirtualMapping.VirtualBaseAddress_p            = (void *) 0xA0000000;
    VirtualMapping.VirtualSize                     = AVMEM1_SIZE;
    VirtualMapping.VirtualWindowOffset             = 0;
    VirtualMapping.VirtualWindowSize               = AVMEM1_SIZE;

    InitParams.SharedMemoryVirtualMapping_p = &VirtualMapping;

    MemoryRange[0].StartAddr_p = (void *)(AVMEM1_BASE_ADDRESS);/*0xA6000000*/
    MemoryRange[0].StopAddr_p  = (void *)(AVMEM1_BASE_ADDRESS+AVMEM1_SIZE-1);/*32 Mbytes*/

    InitParams.DeviceType                   = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitParams.CPUPartition_p               = SystemPartition;
	InitParams.NCachePartition_p            = NcachePartition;
    InitParams.MaxPartitions                = 1;
    InitParams.MaxBlocks                    = 100;        
    InitParams.MaxForbiddenRanges           = 2;    
    InitParams.MaxForbiddenBorders          = 3;    
    InitParams.MaxNumberOfMemoryMapRanges   = 1;

    InitParams.OptimisedMemAccessStrategy_p = NULL;
    InitParams.BlockMoveDmaBaseAddr_p       = NULL;
    InitParams.VideoBaseAddr_p              = NULL;
    InitParams.CacheBaseAddr_p              = NULL;

    InitParams.NumberOfDCachedRanges        = 0;
    InitParams.DCachedRanges_p              = NULL;

    InitParams.SDRAMBaseAddr_p              = (void *)NULL;
    InitParams.SDRAMSize                    = RAM1_SIZE;/*64Mb*/

    memset(InitParams.GpdmaName, 0, sizeof(InitParams.GpdmaName));

    ErrCode = STAVMEM_Init(AVMEMDeviceName[0], &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName[0], GetErrorText(ErrCode),
		MemoryRange[0].StartAddr_p,MemoryRange[0].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
    STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName, GetErrorText(ErrCode),
		MemoryRange[0].StartAddr_p,MemoryRange[0].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
#endif 

    CreateParams.PartitionRanges_p       = &MemoryRange;
    CreateParams.NumberOfPartitionRanges = (sizeof(MemoryRange)/sizeof(STAVMEM_MemoryRange_t));
    ErrCode = STAVMEM_CreatePartition(AVMEMDeviceName[0], &CreateParams, &AVMEMHandle[0]);
	if(ErrCode!=ST_NO_ERROR) 
	{
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));
#endif

	memset(&InitParams       ,0,sizeof(STAVMEM_InitParams_t));
	memset(&VirtualMapping   ,0,sizeof(STAVMEM_SharedMemoryVirtualMapping_t));
	memset(&MemoryRange,0,sizeof(STAVMEM_MemoryRange_t));
	memset(&CreateParams  ,0,sizeof(STAVMEM_CreatePartitionParams_t));
                  
    VirtualMapping.PhysicalAddressSeenFromCPU_p    = (void *) AVMEM2_BASE_ADDRESS;/*0xB0000000*/
    VirtualMapping.PhysicalAddressSeenFromDevice_p = (void *) AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE1;/*0x10000000*/
	VirtualMapping.PhysicalAddressSeenFromDevice2_p = (void *) AVMEM2_BASE_ADDRESS_SEEN_FROM_DEVICE2;/*0x10000000*/
    VirtualMapping.VirtualBaseAddress_p            = (void *) AVMEM2_BASE_ADDRESS;
    VirtualMapping.VirtualSize                     = AVMEM2_SIZE;/*64M*/
    VirtualMapping.VirtualWindowOffset             = 0;
    VirtualMapping.VirtualWindowSize               = AVMEM2_SIZE;

    InitParams.SharedMemoryVirtualMapping_p = &VirtualMapping;

    MemoryRange[0].StartAddr_p = (void *)(AVMEM2_BASE_ADDRESS);/*0xB0000000*/
    MemoryRange[0].StopAddr_p  = (void *)(AVMEM2_BASE_ADDRESS+AVMEM2_SIZE-1);/*64 Mbytes - 1920*32*/

    InitParams.DeviceType                   = STAVMEM_DEVICE_TYPE_VIRTUAL;
    InitParams.CPUPartition_p               = SystemPartition;
	InitParams.NCachePartition_p            = NcachePartition;
    InitParams.MaxPartitions                = 1;
    InitParams.MaxBlocks                    = 100;        
    InitParams.MaxForbiddenRanges           = 2;    
    InitParams.MaxForbiddenBorders          = 3;    
    InitParams.MaxNumberOfMemoryMapRanges   = 1;

    InitParams.OptimisedMemAccessStrategy_p = NULL;
    InitParams.BlockMoveDmaBaseAddr_p       = NULL;
    InitParams.VideoBaseAddr_p              = NULL;
    InitParams.CacheBaseAddr_p              = NULL;

    InitParams.NumberOfDCachedRanges        = 0;
    InitParams.DCachedRanges_p              = NULL;

    InitParams.SDRAMBaseAddr_p              = (void *)NULL;
    InitParams.SDRAMSize                    = AVMEM2_SIZE;
    memset(InitParams.GpdmaName, 0, sizeof(InitParams.GpdmaName));

    ErrCode = STAVMEM_Init(AVMEMDeviceName[1], &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName[1], GetErrorText(ErrCode),
		MemoryRange[1].StartAddr_p,MemoryRange[1].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STAVMEM_Init(%s)=%s SDRAM_BASE_ADDRESS=%lx %lx %lx %lx\n",AVMEMDeviceName[1], GetErrorText(ErrCode),
		MemoryRange[1].StartAddr_p,MemoryRange[1].StopAddr_p,InitParams.SDRAMBaseAddr_p,InitParams.SDRAMSize));
#endif 


    CreateParams.PartitionRanges_p       = &MemoryRange;
    CreateParams.NumberOfPartitionRanges = (sizeof(MemoryRange)/sizeof(STAVMEM_MemoryRange_t));;

    ErrCode = STAVMEM_CreatePartition(AVMEMDeviceName[1], &CreateParams, &AVMEMHandle[1]);
	if(ErrCode!=ST_NO_ERROR) 
	{
	    STTBX_Print(("AVMEM_CreatePartition(1)=%s\n", GetErrorText(ErrCode) ));
		return TRUE ;
	}
#ifdef YXL_INIT_DEBUG
	    STTBX_Print(("AVMEM_CreatePartition()=%s\n", GetErrorText(ErrCode) ));

#endif
				{
#if 0
				U32 FreeSize;
				ST_ErrorCode_t ErrA;

				ErrA=STAVMEM_GetFreeSize(AVMEMDeviceName[0],&FreeSize);
				if(ErrA!=ST_NO_ERROR)
				{
					STTBX_Print(("STAVMEM_GetFreeSize()=%s\n", GetErrorText(ErrA)));

				}
				else STTBX_Print(("FreeSize=%d\n", FreeSize));

				ErrA=STAVMEM_GetFreeSize(AVMEMDeviceName[1],&FreeSize);
				if(ErrA!=ST_NO_ERROR)
				{
					STTBX_Print(("STAVMEM_GetFreeSize()=%s\n", GetErrorText(ErrA)));

				}
				else STTBX_Print(("FreeSize=%d\n", FreeSize));


				ErrA=STAVMEM_GetPartitionFreeSize(AVMEMHandle[0],&FreeSize);
				if(ErrA!=ST_NO_ERROR)
				{
					STTBX_Print(("STAVMEM_GetFreeSize()=%s\n", GetErrorText(ErrA)));

				}
				else STTBX_Print(("FreeSize=%d\n", FreeSize));

				ErrA=STAVMEM_GetPartitionFreeSize(AVMEMHandle[1],&FreeSize);
				if(ErrA!=ST_NO_ERROR)
				{
					STTBX_Print(("STAVMEM_GetFreeSize()=%s\n", GetErrorText(ErrA)));

				}
				else STTBX_Print(("FreeSize=%d\n", FreeSize));
				
#endif
			}
		return FALSE;

} /* end of AVMEM_Setup */

#endif 


BOOL TSMERGE_Setup(void)/*71XX*/
{
	ST_DeviceName_t TSMERGEDeviceName = {"TSMERGE0"};
	STMERGE_InitParams_t		InitParams;
#ifndef TSMERGE_BYPASS
   STMERGE_ObjectConfig_t	Config;
    U8                      Index;
#endif
	ST_ErrorCode_t ErrCode = ST_NO_ERROR;

	U32 SimpleRAMMap[6][2] = {
		{(U32)STMERGE_TSIN_0,6*128+2*128},
		{(U32)STMERGE_TSIN_1,6*128+2*128},
		{(U32)STMERGE_TSIN_2,1*128+1*128},
		{(U32)STMERGE_SWTS_0,6*128+2*128},
		{(U32)STMERGE_ALTOUT_0,1*128},
		{0,0}/**/
		};

#ifdef ST_7100 
	/* Workaround for PTI */
	/* ================== */
	/* Workaround is not applied on cut 1.x and not applied to cut >=3.1 */
	if (((CH_GetChipVersion()&0xF0)!=0xA0)&&(CH_GetChipVersion()<0xC1))
	{
		U32 *TC_Ram     = (U32 *)(ST71XX_PTI_BASE_ADDRESS+0xC000);
		U32  TC_RamSize = 2048;
		BOOL PhaseOk    = FALSE;
		U32  Random_Data[2048];
		U32  i,j;
		
		/* ClockGenA & ClockGenB have to be powered by the same clock - 27Mhz */
		/* ------------------------------------------------------------------ */
#if (STCLKRV_EXT_CLK_MHZ==27)
#if 0
		/* Set Pipe clock at 128MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x1A;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#else
		/* Set Pipe clock at 96MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x11;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#endif
#endif
		
		/* ClockGenA & ClockGenB have to be powered by the same clock - 30Mhz */
		/* ------------------------------------------------------------------ */
#if (STCLKRV_EXT_CLK_MHZ==30)
#if 0
		/* Set Pipe clock at 128MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x1D;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#else
		/* Set Pipe clock at 96MHz - ClockGenB.FS2 */
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x13;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x0;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x1;
		*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#endif
#endif
		
		/* Perform a loop to find a correct phase on the PTI clock */
		/* ------------------------------------------------------- */
		while(PhaseOk==FALSE)
		{
			/* Assume the phase is found */
			PhaseOk=TRUE;
			/* Clear the TC ram memory */
			for (i=0;i<TC_RamSize;i++) 
			{
				TC_Ram[i]=0;
			}
			/* Fill TC Ram with a random pattern */
			for (i=0;i<TC_RamSize;i++)
			{
				Random_Data[i] = rand();
				TC_Ram[i]      = Random_Data[i];
			}
			/* Check 1000 times the TC RAM */
			for (j=0;((j<1000)&&(PhaseOk==TRUE));j++)
			{
				for (i=0;i<TC_RamSize;i++)
				{
					if (TC_Ram[i]!=Random_Data[i])
					{
						PhaseOk=FALSE;
						break;
					}
				}
			}
			/* If phase is not correct, try a random of phase */
			if (PhaseOk==FALSE)
			{
				/* Force an random PE */
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = rand();
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
				/* Reset the PE */
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x0;
				*(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
			}
		}
		
		/* Update clock configuration */
		/* -------------------------- */
		ErrCode = ST_GetClockInfo(&gClockInfo);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		} 
  }

  if (CH_GetChipVersion()>=0xC1) /*fot cut>=3.1*/
  {

#if (STCLKRV_EXT_CLK_MHZ==27)
   /* Set Pipe clock at 165MHz - ClockGenB.FS2 for 27Mhz */
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x14;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x06FB;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#endif
#if (STCLKRV_EXT_CLK_MHZ==30)
   /* Set Pipe clock at 165MHz - ClockGenB.FS2 for 30Mhz */
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x10) = 0xC0DE;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0xB8) = 0x1;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x0;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x70) = 0x17;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x74) = 0x5D18;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x7C) = 0x0;
	  *(DU32 *)(ST71XX_CKG_B_BASE_ADDRESS+0x78) = 0x1;
#endif
	  
	  /* Update clock configuration */
	  ErrCode=ST_GetClockInfo(&gClockInfo);
	  if (ErrCode!=ST_NO_ERROR)
	  {
		  return(ErrCode);
	  } 
  }
 #endif 
  
	InitParams.DeviceType        = STMERGE_DEVICE_1;
  InitParams.BaseAddress_p     = (U32 *)ST71XX_TSMERGE_BASE_ADDRESS;
	InitParams.MergeMemoryMap_p  = SimpleRAMMap;
	InitParams.DriverPartition_p = SystemPartition;
#if 0/*def STMERGE_INTERRUPT_SUPPORT*/
	InitParams.InterruptNumber    = 55;
	InitParams.InterruptLevel     = 1;
#endif
	
#ifdef TSMERGE_BYPASS
	
	InitParams.Mode	= STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
	
	ErrCode = STMERGE_Init(TSMERGEDeviceName,&InitParams);
	if(ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}
	
#else
		
	InitParams.Mode	= STMERGE_NORMAL_OPERATION;
	ErrCode = STMERGE_Init(TSMERGEDeviceName, &InitParams);
	if(ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}
	STTBX_Print(("%s\n", STMERGE_GetRevision() ));

#if 0 /*yxl 2006-12-28 move below section to other*/
		STTBX_Print(("\n YxlInfo:--------GGGGGGG------ not call TSConfig()"));
	Config.Priority	= STMERGE_PRIORITY_HIGHEST;
	Config.SOPSymbol= 0x47;
	Config.SyncLock	= 1;/*2; */
	Config.SyncDrop	= 2; 
	Config.PacketLength	= STMERGE_PACKET_LENGTH_DVB;

#if 0 /*yxl 2006-12-26 cancel below section*/
	ErrCode=STMERGE_SetStatus(STMERGE_TSIN_0);
	if (ErrCode!=ST_NO_ERROR)
	{
		return TRUE;
	}
#endif 
	
	Config.u.TSIN.SyncNotAsync	= FALSE;
	Config.u.TSIN.ReplaceSOPSymbol = FALSE;
	
	Config.u.TSIN.InvertByteClk	= FALSE;/*TRUE;*/
#if 1 
	Config.u.TSIN.SerialNotParallel = FALSE;
	Config.u.TSIN.ByteAlignSOPSymbol = FALSE;
#else /*for tuner searial output*/
	Config.u.TSIN.SerialNotParallel = TRUE;
	Config.u.TSIN.ByteAlignSOPSymbol = TRUE;
	
#endif
	ErrCode = STMERGE_SetParams(STMERGE_TSIN_0, &Config);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("\n STMERGE_SetParams Failed!!! ErrCode = %s\n",GetErrorText(ErrCode) ));
		return TRUE;
	}
	ErrCode = STMERGE_Connect(STMERGE_TSIN_0, STMERGE_PTI_0);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("\n STMERGE_Connect Failed!!! ErrCode = %s\n",GetErrorText(ErrCode) ));
		return TRUE;
	}

#endif /*end yxl 2006-12-28 move below section to other*/
#endif


		
	return FALSE;
}


/*yxl 2006-12-28 add below section from other*/
#if 0
BOOL TSConfig(void)/*71XX*/
{
#ifndef TSMERGE_BYPASS
   STMERGE_ObjectConfig_t	Config;
    U8                      Index;
#endif
	ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	#if 1 /*yxl 2006-12-28 move below section to other*/
	
	Config.Priority	= STMERGE_PRIORITY_HIGHEST;
	Config.SOPSymbol= 0x47;
	Config.SyncLock	= 1;/*2; */
	Config.SyncDrop	= 2; 
	Config.PacketLength	= STMERGE_PACKET_LENGTH_DVB;

#if 0 /*yxl 2006-12-26 cancel below section*/
	ErrCode=STMERGE_SetStatus(STMERGE_TSIN_0);
	if (ErrCode!=ST_NO_ERROR)
	{
		return TRUE;
	}
#endif 
	
	Config.u.TSIN.SyncNotAsync	= FALSE;
	Config.u.TSIN.ReplaceSOPSymbol = FALSE;
	
	Config.u.TSIN.InvertByteClk	= FALSE;/*TRUE;*/
#if 1 
	Config.u.TSIN.SerialNotParallel = FALSE;
	Config.u.TSIN.ByteAlignSOPSymbol = FALSE;
#else /*for tuner searial output*/
	Config.u.TSIN.SerialNotParallel = TRUE;
	Config.u.TSIN.ByteAlignSOPSymbol = TRUE;
	
#endif
	ErrCode = STMERGE_SetParams(STMERGE_TSIN_0, &Config);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("\n STMERGE_SetParams Failed!!! ErrCode = %s\n",GetErrorText(ErrCode) ));
		return TRUE;
	}
	ErrCode = STMERGE_Connect(STMERGE_TSIN_0, STMERGE_PTI_0);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("\n STMERGE_Connect Failed!!! ErrCode = %s\n",GetErrorText(ErrCode) ));
		return TRUE;
	}

#endif /*end yxl 2006-12-28 move below section to other*/
	return FALSE;
}

#else

BOOL TSConfig(void)/*71XX*/
{
 U32                    i;
 ST_ErrorCode_t         ErrCode = ST_NO_ERROR;
 STMERGE_ObjectConfig_t Config;
 STPTI_StreamID_t       PTI_StreamID;

 memset((void*)&Config,0,sizeof(STMERGE_ObjectConfig_t));/*yxl 2007-01-27 add this line*/

 /* Configuration of the TSMERGE connection */
 /* --------------------------------------- */
 Config.Priority     = STMERGE_PRIORITY_HIGHEST;
 Config.SyncLock     = 2;
 Config.SyncDrop     = 2;
 Config.SOPSymbol    = 0x47;
 Config.PacketLength = STMERGE_PACKET_LENGTH_DVB;

 /* Reset the TS merger input */
 /* ------------------------- */
 ErrCode=STMERGE_Connect(STMERGE_TSIN_0,STMERGE_NO_DEST);
 if (ErrCode!=ST_NO_ERROR)
  {
	return TRUE;
  }

 /* Reset the status */
 /* ---------------- */
 ErrCode=STMERGE_SetStatus(STMERGE_TSIN_0);
 if (ErrCode!=ST_NO_ERROR)
  {
 	return TRUE;
  }

 /* Switch the PTI tag id to filter */
 /* ------------------------------- */
 
PTI_StreamID = STPTI_STREAM_ID_TSIN0; 

 /* Create the new connection */
 /* ------------------------- */
 ErrCode=STPTI_SetStreamID(PTIDeviceName,PTI_StreamID);
 if (ErrCode!=ST_NO_ERROR)
  {
 	return TRUE;
  }
 

	Config.u.TSIN.SyncNotAsync	= FALSE;
	Config.u.TSIN.ReplaceSOPSymbol = FALSE;
	
	Config.u.TSIN.InvertByteClk	= FALSE;/*TRUE;*/
#if 1 
	Config.u.TSIN.SerialNotParallel = FALSE;
	Config.u.TSIN.ByteAlignSOPSymbol = FALSE;
#else /*for tuner searial output*/
	Config.u.TSIN.SerialNotParallel = TRUE;
	Config.u.TSIN.ByteAlignSOPSymbol = TRUE;
	
#endif


 ErrCode=STMERGE_SetParams(STMERGE_TSIN_0,&Config);
 if (ErrCode!=ST_NO_ERROR)
  {
  	return TRUE;
  }

 /* Connect the input to the output */
 /* ------------------------------- */
 ErrCode=STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
 if (ErrCode!=ST_NO_ERROR)
  {
  	return TRUE;
  }


	return FALSE;
}

#endif /*yxl 2006-12-28 add below section from other*/


BOOL STPTI_Setup(void) /*71XX*/
{
    ST_ErrorCode_t ErrCode;

    STPTI_InitParams_t InitParams;
    STPTI_OpenParams_t OpenParams;
    STPTI_SlotData_t SlotData;
	STPTI_DMAParams_t      DMAParams;
    STCLKRV_SourceParams_t CLKRVSourceParams;
	U32 DMAUsed;
 
	memset(&InitParams,0,sizeof(STPTI_InitParams_t));
    InitParams.Device=STPTI_DEVICE_PTI_4;
    InitParams.TCDeviceAddress_p=(U32 *)PTIA_BASE_ADDRESS;

#ifdef ST_7100
    InitParams.TCLoader_p = STPTI_DVBTCLoaderL;
#endif
#ifdef ST_7109
    InitParams.TCLoader_p = STPTI_DVBTCLoaderSL;
#endif

    InitParams.TCCodes    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    InitParams.SyncLock   = 0; 
    InitParams.SyncDrop   = 0; 

	InitParams.DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
	/*	STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;*/

    InitParams.Partition_p                = SystemPartition;
    InitParams.NCachePartition_p          = NcachePartition;
    strcpy( InitParams.EventHandlerName, EVTDeviceName);
#if 0 /*yxl 2006-12-26 modify*/
    InitParams.EventProcessPriority       = MIN_USER_PRIORITY+8; /*+5;*/

    InitParams.InterruptProcessPriority   = MAX_USER_PRIORITY-1;
#else
	
    InitParams.EventProcessPriority   =/*STPTI_EVENT_TASK_PRIORTY*/(8);
    InitParams.InterruptProcessPriority            = /*STPTI_INTERRUPT_TASK_PRIORTY*/(15);
	
#endif /*end yxl 2006-12-26 modify*/
	InitParams.InterruptLevel             = PTI_INTERRUPT_LEVEL;
    InitParams.InterruptNumber=PTIA_INTERRUPT;
	
	InitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_48x8;
	/*STPTI_FILTER_OPERATING_MODE_16x8,STPTI_FILTER_OPERATING_MODE_32x16*/
	
	InitParams.NumberOfSlots = MAX_NO_SLOT;/*48;*/
	InitParams.DiscardOnCrcError = TRUE;/*TRUE;/*FALSE;*/
	InitParams.AlternateOutputLatency  = 42;
	InitParams.NumberOfSectionFilters     = /*16 20070703 change*/MAX_NO_FILTER_TOTAL;

	strcpy(InitParams.EventHandlerName,EVTDeviceName);

#ifdef TSMERGE_BYPASS 
	InitParams.StreamID = STPTI_STREAM_ID_NOTAGS;
	InitParams.PacketArrivalTimeSource = STPTI_ARRIVAL_TIME_SOURCE_PTI;
#else
	InitParams.StreamID = STPTI_STREAM_ID_TSIN0;/*STPTI_STREAM_ID_SWTS0;STPTI_STREAM_ID_TSIN0*/
	InitParams.PacketArrivalTimeSource = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
#endif


#ifdef YXL_INIT_DEBUG
    STTBX_Print(("STPTI_GetResivion()=%s\n",STPTI_GetRevision()));
#endif

    ErrCode = STPTI_Init(PTIDeviceName, &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STPTI_Init()=%s %s\n", GetErrorText(ErrCode),STPTI_GetRevision()));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
    STTBX_Print(("STPTI_Init()=%s %s\n", GetErrorText(ErrCode),STPTI_GetRevision()));
#endif


    OpenParams.DriverPartition_p    = SystemPartition;
    OpenParams.NonCachedPartition_p = NcachePartition;
    ErrCode = STPTI_Open(PTIDeviceName, &OpenParams, &PTIHandle);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STPTI_Open()=%s", GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
    STTBX_Print(("STPTI_Open()=%s", GetErrorText(ErrCode)));	
#endif

	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_DMA_COMPLETE_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_INDEX_MATCH_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_INVALID_LINK_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_INVALID_PARAMETER_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT);
	STPTI_DisableErrorEvent(PTIDeviceName,STPTI_EVENT_CWP_BLOCK_EVT);/**/
	
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_BUFFER_OVERFLOW_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_CC_ERROR_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_INTERRUPT_FAIL_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_PACKET_ERROR_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_PCR_RECEIVED_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
	STPTI_EnableErrorEvent (PTIDeviceName,STPTI_EVENT_TC_CODE_FAULT_EVT);   

#if 1 /*yxl 2007-01-04 add below section*/
	{
		ST_ErrorCode_t ST_ErrorCode;	
#if defined(PTI_DEBUG_EVENT) || defined (CLKRV_DEBUG_EVENT)
		STEVT_DeviceSubscribeParams_t SubscribeParams;
		int event=0;
#endif			
		
#ifdef PTI_DEBUG_EVENT
		extern void avc_PTIEvent_CallbackProc(STEVT_CallReason_t     Reason,
			const ST_DeviceName_t  RegistrantName,
			STEVT_EventConstant_t  Event, 
			const void            *EventData,
			const void            *SubscriberData_p);

		SubscribeParams.NotifyCallback = avc_PTIEvent_CallbackProc;
		for(event =STPTI_EVENT_BASE; event <= STPTI_EVENT_PES_ERROR_EVT; event++)
		{    		
			ST_ErrorCode = STEVT_SubscribeDeviceEvent(EVTHandle, PTIDeviceName, (U32)event, &SubscribeParams);
			if ( ST_ErrorCode != ST_NO_ERROR )
			{
				STTBX_Print(("EVT_Subscribe(%d, Video)=%s\n", event, GetErrorText(ST_ErrorCode) ));
				return TRUE;
			}
		}
#endif
	}
#endif  /*yxl 2007-01-04 add below section*/
    return FALSE;

}


ST_ErrorCode_t VideoGetWritePtrFct(void * const Handle_p, void ** const Address_p)
{
    ST_ErrorCode_t ErrCode;
	void* pWriteBufTemp;

    ErrCode = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle_p,&pWriteBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferGetWritePointer()Failed %s\n", GetErrorText(ErrCode)));
		return ErrCode;
    }

	pWriteBufTemp=(void *)((U32)pWriteBufTemp&0x1FFFFFFF);
	*Address_p=pWriteBufTemp;

    return ST_NO_ERROR;
}

void VideoSetReadPtrFct(void * const Handle_p, void * const Address_p)
{
    ST_ErrorCode_t ErrCode;
	void* pReadBufTemp;

	pReadBufTemp=Address_p;
	pReadBufTemp=(void *)((U32)pReadBufTemp|0xA0000000);
	
	ErrCode = STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle_p,pReadBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferSetReadPointer()Failed %s\n", GetErrorText(ErrCode)));
    }
	
    return;
}



static ST_ErrorCode_t AudioGetWritePtrFct(void *const Handle_p,void **const Address_p)
{                
	ST_ErrorCode_t ErrCode;
	void* pWriteBufTemp;
	
    ErrCode = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle_p,&pWriteBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferGetWritePointer()Failed %s\n", GetErrorText(ErrCode)));
		return ErrCode;
    }
	
	*Address_p=(void *)((U32)pWriteBufTemp|0xA0000000);
	
    return ST_NO_ERROR;
}

static ST_ErrorCode_t AudioSetReadPtrFct(void *const Handle_p,void *const Address_p)
{
    ST_ErrorCode_t ErrCode;
	
	ErrCode = STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle_p,Address_p);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferSetReadPointer()Failed %s\n", GetErrorText(ErrCode)));
    }
	return ST_NO_ERROR;
}


/* =======================================================================
   Name:        STPRMi_VID_Callback
   Description: Video event callback
   ======================================================================== */

static void STPRMi_VID_Callback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32                   i;
 ST_ErrorCode_t        ErrCode=ST_NO_ERROR;
 STVID_PictureInfos_t *VID_PictureInfo;
 
 /* Switch according to the event received */
 /* ====================================== */
 switch(Event)
  {
   /* New display event */
   /* ----------------- */
 case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT :    
	 {
		 VID_PictureInfo = (STVID_PictureInfos_t *)EventData;
		 /* Only for debug */
		 if (VID_PictureInfo!=NULL)
		 {
			 STTBX_Print(("STPRMi_VID_Callback():PTS=0x%d%08x\n",VID_PictureInfo->VideoParams.PTS.PTS33?1:0,VID_PictureInfo->VideoParams.PTS.PTS));
		 }
#if 0 
		 /* Active video plane */
		 PRM->PLAY_VideoNbFrames++;
		 if ((PRM->PLAY_VideoNbFrames==STPRMi_NB_VIDEO_FRAME_BEFORE_ENABLING) && (PRM->PLAY_VideoEnabled==FALSE))
		 {
			 for (i=0;((i<STPRM_MAX_VIEWPORTS)&&(PRM->VIEW_Handle[i]!=0));i++)
			 {
				 ErrCode=STVID_EnableOutputWindow(PRM->VIEW_Handle[i]);
				 if (ErrCode!=ST_NO_ERROR)
				 {
					 STTBX_Print(("STPRMi_VID_Callback(0x%08x):**ERROR** !!! Unable to enable the video output window !!!\n",PRM));
					 break;
				 }
			 }
			 PRM->PLAY_VideoEnabled=TRUE;
		 }
#endif 
	 }
	 break;

   /* Synchronisation event */
   /* --------------------- */
   case STVID_SYNCHRONISATION_CHECK_EVT :
    {
     STVID_SynchronisationInfo_t *VID_SynchronisationInfo = (STVID_SynchronisationInfo_t *)EventData;
     /* If the synchro is stable and there is a delay that the video decoder can't recover */
     if ((VID_SynchronisationInfo->IsSynchronisationOk==TRUE) && (VID_SynchronisationInfo->ClocksDifference!=0))
      {
         STTBX_Print(("STPRMi_VID_Callback(): Set audio sync offset to ms !\n"));

 
      }
    }            
    break;

   /* Error events */
   /* ------------ */
   case STVID_DATA_ERROR_EVT             : STTBX_Print(("STPRMi_VID_Callback():**ERROR** !!! STVID_DATA_ERROR_EVT !!!\n"  )); break;
   case STVID_PICTURE_DECODING_ERROR_EVT : STTBX_Print(("STPRMi_VID_Callback():**ERROR** !!! STVID_PICTURE_DECODING_ERROR_EVT !!!\n")); break;
   case STVID_DATA_OVERFLOW_EVT          : STTBX_Print(("STPRMi_VID_Callback():**ERROR** !!! STVID_DATA_OVERFLOW_EVT !!!\n"));        break;
   case STVID_DATA_UNDERFLOW_EVT         : STTBX_Print(("STPRMi_VID_Callback():**ERROR** !!! STVID_DATA_UNDERFLOW_EVT !!!\n"));        break;

   /* Strange event */
   /* ------------- */
   default : 
    STTBX_Print(("STPRMi_VID_Callback():**ERROR** !!! Event 0x%08x received but not managed !!!\n",Event));
    break;
  }
}




#if 1/*yxl 2007-04-17 modify below section*/

STPTI_Buffer_t BufHandleTemp[2];

/*分配和配置LAYER_VIDEO1、AUDEO AND PCR slot*/
BOOL CH_AVSlotInit(void) /*71XX*/
{
    ST_ErrorCode_t ErrCode;
	STPTI_SlotData_t SlotData;
	STPTI_DMAParams_t      DMAParams;
    STCLKRV_SourceParams_t CLKRVSourceParams;
	U32 DMAUsed;
	void* Video_PESBufferAdd_p;
    U32   Video_PESBufferSize;
/*	STPTI_Buffer_t BufHandleTemp;*/

	memset((void *)&SlotData, 0, sizeof(STPTI_SlotData_t));
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
#if 1 /*2007-01-09*/
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#else
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = TRUE;
#endif
	SlotData.SlotFlags.SoftwareCDFifo = TRUE; 

	/*Video slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&VideoSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Video)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(VideoSlot);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotClearPid(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

	
/*yxl 2007-01-04 add below*/
	ErrCode = STPTI_SlotSetCCControl(VideoSlot,TRUE);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*end yxl 2007-01-04 add below*/


	ErrCode = STVID_GetDataInputBufferParams(VIDHandle[VID_MPEG2],(void **)&Video_PESBufferAdd_p,&Video_PESBufferSize);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_GetDataInputBufferParams Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STPTI_BufferAllocateManual(PTIHandle,
                                           (U8*)Video_PESBufferAdd_p, Video_PESBufferSize,
                                           1, &BufHandleTemp[VID_MPEG2]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_BufferAllocateManual Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }
#if 0 /*yxl 2007-04-17 cancel*/
	/*20051017 add for get video buf*/
	VIDEOBufHandleTemp=BufHandleTemp[VID_MPEG2];
	/*********************************/
#endif
	
#if 0 /*yxl 2007-04-17 modify below section*/
    ErrCode = STPTI_SlotLinkToBuffer(VideoSlot,BufHandleTemp[VID_MPEG2]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_SlotLinkToBuffer Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

#if 0 /*yxl 2006-12-31 modify */
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp, TRUE);/*FALSE*/
#else
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp[VID_MPEG2], FALSE);/*FALSE*/

#endif/*end yxl 2006-12-31 modify */
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STVID_SetDataInputInterface( VIDHandle[VID_MPEG2],
                                           VideoGetWritePtrFct, VideoSetReadPtrFct,
                                           (void * const)BufHandleTemp[VID_MPEG2]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

#if 0 /*yxl 2007-04-17 temp cancel below section for test*/
	ErrCode=STPTI_BufferSetReadPointer(BufHandleTemp[VID_MPEG2],Video_PESBufferAdd_p);
	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }
#endif /*end yxl 2007-04-17 temp cancel below section for test*/

#else /*yxl 2007-04-17 modify below section*/
	CH_VideoDecoderControl(VID_MPEG2,TRUE);

#endif/*end yxl 2007-04-17 modify below section*/

	ErrCode = STVID_GetDataInputBufferParams(VIDHandle[VID_H],(void **)&Video_PESBufferAdd_p,&Video_PESBufferSize);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_GetDataInputBufferParams Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STPTI_BufferAllocateManual(PTIHandle,
                                           (U8*)Video_PESBufferAdd_p, Video_PESBufferSize,
                                           1, &BufHandleTemp[VID_H]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_BufferAllocateManual Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }



	/*Audio slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&AudioSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Audio)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(AudioSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(Audio)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*yxl 2007-02-18 add below*/
	ErrCode = STPTI_SlotSetCCControl(AudioSlot,FALSE);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*end yxl 2007-02-18 add below*/


	{
		STPTI_Buffer_t AUDBufHandleTemp;
		
		ErrCode=STAUD_IPGetInputBufferParams(AUDHandle,STAUD_OBJECT_INPUT_CD0,&AUDBufferParams);
		
	/*	PRM->PTI_AudioBufferBase = (U32)AUDBufferParams.BufferBaseAddr_p;
		PRM->PTI_AudioBufferSize = AUDBufferParams.BufferSize;
	*/
		ErrCode=STPTI_BufferAllocateManual(PTIHandle,
			(U8 *)AUDBufferParams.BufferBaseAddr_p,
			AUDBufferParams.BufferSize,
			1,&AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferAllocateManual(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		ErrCode=STPTI_SlotLinkToBuffer(AudioSlot,AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotLinkToBuffer(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STPTI_BufferSetOverflowControl(AUDBufHandleTemp,FALSE);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferSetOverflowControl(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STAUD_IPSetDataInputInterface(AUDHandle,STAUD_OBJECT_INPUT_CD0,
			AudioGetWritePtrFct,AudioSetReadPtrFct,(void *)AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STAUD_IPSetDataInputInterface(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
	}



	/*PCR Slot*/
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
	SlotData.SlotFlags.SoftwareCDFifo = FALSE; 
    ErrCode = STPTI_SlotAllocate(PTIHandle,&PCRSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(PCR)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

	ErrCode = STPTI_SlotClearPid(PCRSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(PCR)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

#if 1 /*yxl 2007-01-02 add*/
	   ErrCode=STPTI_SlotSetCCControl(PCRSlot,TRUE/*FALSE*/);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STPTI_SlotSetCCControl(PCR)=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }
	   
	   
	   
	   ErrCode=STCLKRV_InvDecodeClk(CLKRVHandle);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STCLKRV_InvDecodeClk()=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }
	   
	   

	
#endif 
    CLKRVSourceParams.Source= STCLKRV_PCR_SOURCE_PTI;
    CLKRVSourceParams.Source_u.STPTI_s.Slot=PCRSlot;


    ErrCode = STCLKRV_SetPCRSource(CLKRVHandle, &CLKRVSourceParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetPCRSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#if 0 /*yxl 2007-01-02 cancel below*/
	ErrCode = STCLKRV_SetSTCSource(CLKRVHandle, STCLKRV_STC_SOURCE_PCR);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#endif 
	ErrCode=STCLKRV_SetSTCOffset(CLKRVHandle,(((-160)*90000)/1000));
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCOffset()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }



#if 0 /*yxl 2007-01-02 add below*/
	{   
		STEVT_OpenParams_t            EVT_OpenParams;
		STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
		STVID_ConfigureEventParams_t  VID_ConfigureEventParams;
		/* Subscribe to video event to manage the display */
		/* ---------------------------------------------- */
		memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
		memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
		EVT_SubcribeParams.NotifyCallback=STPRMi_VID_Callback;
#if 1 /*yxl 2007-01-04 temp cancel*/
	/*	ErrCode|=STEVT_Open(PRM->EVT_DeviceName,&EVT_OpenParams,&(EVTHandle));*/
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_SYNCHRONISATION_CHECK_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_OVERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_UNDERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_PICTURE_DECODING_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&EVT_SubcribeParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPRM_PlayStart():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
#endif 		
		/* Configure the video events */
		/* -------------------------- */
		memset(&VID_ConfigureEventParams,0,sizeof(STVID_ConfigureEventParams_t));
		VID_ConfigureEventParams.Enable              = TRUE;
		VID_ConfigureEventParams.NotificationsToSkip = 0;
		ErrCode|=STVID_ConfigureEvent(VIDHandle[VID_MPEG2],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&VID_ConfigureEventParams);
		ErrCode|=STVID_ConfigureEvent(VIDHandle[VID_MPEG2],STVID_SYNCHRONISATION_CHECK_EVT,&VID_ConfigureEventParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STVID_ConfigureEvent():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
	}
#endif 
	/*打开Audio,Video 解扰器*/
#ifdef  NAFR_KERNEL
	if(OpenVDescrambler(VideoSlot))
	{
		STTBX_Print(("OpenVDescrambler wrong\n"));
		return TRUE;
	}
#endif
#ifdef  NAFR_KERNEL
	if(/*OpenVDescrambler 20060622 change*/OpenADescrambler(AudioSlot))
	{
		STTBX_Print(("OpenVDescrambler wrong\n"));
		return TRUE;
	}
#endif

	return FALSE;
}

#else


BOOL CH_AVSlotInit(void) /*71XX*/
{
    ST_ErrorCode_t ErrCode;
	STPTI_SlotData_t SlotData;
	STPTI_DMAParams_t      DMAParams;
    STCLKRV_SourceParams_t CLKRVSourceParams;
	U32 DMAUsed;
	void* Video_PESBufferAdd_p;
    U32   Video_PESBufferSize;
	STPTI_Buffer_t BufHandleTemp;

	memset((void *)&SlotData, 0, sizeof(STPTI_SlotData_t));
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
#if 1 /*2007-01-09*/
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#else
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = TRUE;
#endif
	SlotData.SlotFlags.SoftwareCDFifo = TRUE; 

	/*Video slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&VideoSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Video)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(VideoSlot);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotClearPid(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

	
/*yxl 2007-01-04 add below*/
	ErrCode = STPTI_SlotSetCCControl(VideoSlot,TRUE);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*end yxl 2007-01-04 add below*/


	ErrCode = STVID_GetDataInputBufferParams(VIDHandle[VID_MPEG2],(void **)&Video_PESBufferAdd_p,&Video_PESBufferSize);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_GetDataInputBufferParams Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STPTI_BufferAllocateManual(PTIHandle,
                                           (U8*)Video_PESBufferAdd_p, Video_PESBufferSize,
                                           1, &BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_BufferAllocateManual Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }




	/*20051017 add for get video buf*/
	VIDEOBufHandleTemp=BufHandleTemp;
	/*********************************/

    ErrCode = STPTI_SlotLinkToBuffer(VideoSlot,BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_SlotLinkToBuffer Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

#if 0 /*yxl 2006-12-31 modify */
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp, TRUE);/*FALSE*/
#else
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp, FALSE);/*FALSE*/

#endif/*end yxl 2006-12-31 modify */
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STVID_SetDataInputInterface( VIDHandle[VID_MPEG2],
                                           VideoGetWritePtrFct, VideoSetReadPtrFct,
                                           (void * const)BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }


	ErrCode=STPTI_BufferSetReadPointer(BufHandleTemp,Video_PESBufferAdd_p);
	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

/*	ErrCode=STVID_GetBitBufferFreeSize(VIDHandle[VID_MPEG2],&PRM->PTI_VideoBitBufferSize);*/



	/*Audio slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&AudioSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Audio)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(AudioSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(Audio)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*yxl 2007-02-18 add below*/
	ErrCode = STPTI_SlotSetCCControl(AudioSlot,FALSE);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
/*end yxl 2007-02-18 add below*/

#if 1 /*yxl 2006-12-14 temp cancel*/
	{
		STAUD_BufferParams_t AUDBufferParams;
		STPTI_Buffer_t AUDBufHandleTemp;
		
		ErrCode=STAUD_IPGetInputBufferParams(AUDHandle,STAUD_OBJECT_INPUT_CD0,&AUDBufferParams);
		
	/*	PRM->PTI_AudioBufferBase = (U32)AUDBufferParams.BufferBaseAddr_p;
		PRM->PTI_AudioBufferSize = AUDBufferParams.BufferSize;
	*/
		ErrCode=STPTI_BufferAllocateManual(PTIHandle,
			(U8 *)AUDBufferParams.BufferBaseAddr_p,
			AUDBufferParams.BufferSize,
			1,&AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferAllocateManual(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		ErrCode=STPTI_SlotLinkToBuffer(AudioSlot,AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotLinkToBuffer(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STPTI_BufferSetOverflowControl(AUDBufHandleTemp,FALSE);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferSetOverflowControl(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STAUD_IPSetDataInputInterface(AUDHandle,STAUD_OBJECT_INPUT_CD0,
			AudioGetWritePtrFct,AudioSetReadPtrFct,(void *)AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STAUD_IPSetDataInputInterface(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
	}
#endif


	/*PCR Slot*/
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
	SlotData.SlotFlags.SoftwareCDFifo = FALSE; 
    ErrCode = STPTI_SlotAllocate(PTIHandle,&PCRSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(PCR)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

	ErrCode = STPTI_SlotClearPid(PCRSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(PCR)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

#if 1 /*yxl 2007-01-02 add*/
	   ErrCode=STPTI_SlotSetCCControl(PCRSlot,TRUE/*FALSE*/);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STPTI_SlotSetCCControl(PCR)=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }
	   
	   
	   
	   ErrCode=STCLKRV_InvDecodeClk(CLKRVHandle);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STCLKRV_InvDecodeClk()=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }	
	   
	   

	
#endif 
    CLKRVSourceParams.Source= STCLKRV_PCR_SOURCE_PTI;
    CLKRVSourceParams.Source_u.STPTI_s.Slot=PCRSlot;


    ErrCode = STCLKRV_SetPCRSource(CLKRVHandle, &CLKRVSourceParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetPCRSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#if 0 /*yxl 2007-01-02 cancel below*/
	ErrCode = STCLKRV_SetSTCSource(CLKRVHandle, STCLKRV_STC_SOURCE_PCR);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#endif 
	ErrCode=STCLKRV_SetSTCOffset(CLKRVHandle,(((-160)*90000)/1000));
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCOffset()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }



#if 0 /*yxl 2007-01-02 add below*/
	{   
		STEVT_OpenParams_t            EVT_OpenParams;
		STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
		STVID_ConfigureEventParams_t  VID_ConfigureEventParams;
		/* Subscribe to video event to manage the display */
		/* ---------------------------------------------- */
		memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
		memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
		EVT_SubcribeParams.NotifyCallback=STPRMi_VID_Callback;
#if 1 /*yxl 2007-01-04 temp cancel*/
	/*	ErrCode|=STEVT_Open(PRM->EVT_DeviceName,&EVT_OpenParams,&(EVTHandle));*/
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_SYNCHRONISATION_CHECK_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_OVERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_DATA_UNDERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_PICTURE_DECODING_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[VID_MPEG2],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&EVT_SubcribeParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPRM_PlayStart():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
#endif 		
		/* Configure the video events */
		/* -------------------------- */
		memset(&VID_ConfigureEventParams,0,sizeof(STVID_ConfigureEventParams_t));
		VID_ConfigureEventParams.Enable              = TRUE;
		VID_ConfigureEventParams.NotificationsToSkip = 0;
		ErrCode|=STVID_ConfigureEvent(VIDHandle[VID_MPEG2],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&VID_ConfigureEventParams);
		ErrCode|=STVID_ConfigureEvent(VIDHandle[VID_MPEG2],STVID_SYNCHRONISATION_CHECK_EVT,&VID_ConfigureEventParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STVID_ConfigureEvent():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
	}
#endif 
	return FALSE;
}

#endif  /*end yxl 2007-04-17 modify below section*/


#if 1 /*yxl 2007-04-17 add below function*/
BOOL CH_VideoDecoderControl(VID_Device VIDDevice,BOOL Status)
{
	ST_ErrorCode_t ErrCode;
	STPTI_Buffer_t BufHTemp;

	BufHTemp=BufHandleTemp[VIDDevice];

	if(Status==TRUE)
	{
		
		ErrCode = STPTI_SlotLinkToBuffer(VideoSlot,BufHTemp);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\nSTPTI_SlotLinkToBuffer Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}
		
		ErrCode = STPTI_BufferSetOverflowControl(BufHTemp, FALSE);/*FALSE*/
		
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}
#if 0 /*yxl 2007-04-18 modify below section*/
		ErrCode = STVID_SetDataInputInterface( VIDHandle[VID_MPEG2],
			VideoGetWritePtrFct, VideoSetReadPtrFct,
			(void * const)BufHTemp);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}
#else
		ErrCode = STVID_SetDataInputInterface( VIDHandle[VIDDevice],
			VideoGetWritePtrFct, VideoSetReadPtrFct,
			(void * const)BufHTemp);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}
#endif/*end yxl 2007-04-18 modify below section*/
		
		
/*		ErrCode=STPTI_BufferSetReadPointer(BufHTemp,Video_PESBufferAdd_p);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}*/
	}
	else
	{
		ErrCode = STVID_DeleteDataInputInterface(VIDHandle[VIDDevice]);
	
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\nSTVID_DeleteDataInputInterface()=%s\n",GetErrorText(ErrCode)));
			return TRUE;
		}

		ErrCode = STPTI_SlotUnLink(VideoSlot);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (("\nSTPTI_SlotLinkToBuffer Failed!!! Error = %s \n",GetErrorText(ErrCode)));
			return TRUE;
		}


	}
	
	return FALSE;
}

#endif  /*yxl 2007-04-17 add below function*/


BOOL DENC_Setup(void) /*71XX*/
{
	ST_ErrorCode_t ErrCode;
	ST_DeviceName_t DENCDeviceName="DENC";
	STDENC_InitParams_t InitParams;
    STDENC_OpenParams_t OpenParams;
	STDENC_EncodingMode_t EncodingMode;

    memset((void *)&InitParams, 0, sizeof(STDENC_InitParams_t));
#ifdef ST_7100 
	InitParams.DeviceType=STDENC_DEVICE_TYPE_V13;
#endif
#ifdef ST_7109 
	InitParams.DeviceType=	STDENC_DEVICE_TYPE_DENC;
#endif
	InitParams.MaxOpen=5;
	InitParams.AccessType=STDENC_ACCESS_TYPE_EMI;
	InitParams.STDENC_Access.EMI.BaseAddress_p=(void*)ST71XX_DENC_BASE_ADDRESS;
	InitParams.STDENC_Access.EMI.DeviceBaseAddress_p=(void*) 0;
	InitParams.STDENC_Access.EMI.Width=STDENC_EMI_ACCESS_WIDTH_32_BITS;

	ErrCode=STDENC_Init(DENCDeviceName,&InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STDENC_Init()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STDENC_Init()=%s\n",GetErrorText(ErrCode)));	
#endif

	memset((void *)&OpenParams, 0, sizeof(STDENC_OpenParams_t));
	ErrCode=STDENC_Open(DENCDeviceName,&OpenParams,&DENCHandle);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STDENC_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STDENC_Open()=%s\n",GetErrorText(ErrCode)));
#endif

	EncodingMode.Mode=STDENC_MODE_PALBDGHI;
	EncodingMode.Option.Pal.Interlaced=TRUE;
	EncodingMode.Option.Pal.SquarePixel=FALSE;

    ErrCode = STDENC_SetEncodingMode(DENCHandle, &EncodingMode);
 	if(ErrCode!=ST_NO_ERROR) 
	{
	
		STTBX_Print(("STDENC_SetEncodingMode(pal)=%s\n", GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STDENC_SetEncodingMode(pal)=%s\n", GetErrorText(ErrCode)));
#endif

	if ((CH_GetChipVersion()&0xF0)!=0xA0)
	{
		*(DU32 *)(ST71XX_DENC_BASE_ADDRESS+0x17C) = 0x17;/*yxl 2007-06-21 temp cancel this line for test*/
	}

	return FALSE;
}



BOOL VTG_Setup(void) /*71XX*/
{
	ST_ErrorCode_t ErrCode;

	STVTG_InitParams_t InitParams;
	STVTG_OpenParams_t OpenParams;
	STVTG_TimingMode_t TimingMode;

	memset(&InitParams,0,sizeof(STVTG_InitParams_t));
	memset(&OpenParams,0,sizeof(STVTG_OpenParams_t));

	InitParams.DeviceType=VTG_DEVICE_TYPE_VTG_CELL_71XX;
	InitParams.MaxOpen=3;
	InitParams.Target.VtgCell3.DeviceBaseAddress_p = (void *)0x0;
	InitParams.Target.VtgCell3.BaseAddress_p = (void*)ST71XX_VTG1_BASE_ADDRESS;
	InitParams.Target.VtgCell3.InterruptNumber     = ST71XX_VTG_0_INTERRUPT;
#ifdef ST_7100	
	InitParams.Target.VtgCell3.InterruptLevel      = 7;
#endif 
#ifdef ST_7109
	InitParams.Target.VtgCell3.InterruptLevel      = 4;
#endif
	strcpy( InitParams.Target.VtgCell3.ClkrvName, CLKRVDeviceName); 
	strcpy(InitParams.EvtHandlerName,EVTDeviceName);

	ErrCode=STVTG_Init(VTGDeviceName[VTG_MAIN],&InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVTG_Init(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STVTG_Init(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
#endif

	InitParams.Target.VtgCell3.InterruptNumber     = ST71XX_VTG_1_INTERRUPT;
	InitParams.Target.VtgCell3.BaseAddress_p = (void *)ST71XX_VTG2_BASE_ADDRESS;

	ErrCode=STVTG_Init(VTGDeviceName[VTG_AUX],&InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVTG_Init(VTG_AUX)=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STVTG_Init(VTG_AUX)=%s\n",GetErrorText(ErrCode)));
#endif

	ErrCode=STVTG_Open(VTGDeviceName[VTG_MAIN],&OpenParams,&VTGHandle[VTG_MAIN]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVTG_Open(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STVTG_Open(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
#endif

	ErrCode=STVTG_Open(VTGDeviceName[VTG_AUX],&OpenParams,&VTGHandle[VTG_AUX]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVTG_Open(VTG_AUX)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STVTG_Open(VTG_AUX)=%s\n",GetErrorText(ErrCode)));
#endif

	return FALSE;

}


BOOL LAYER_Setup(LAYER_Type LAYERType,CH_VideoOutputAspect Ratio) /*71XX*/
{
	ST_ErrorCode_t ErrCode;
	STLAYER_LayerParams_t LayerParams;
	STLAYER_InitParams_t InitParams;
	STLAYER_OpenParams_t OpenParams;
	STGXOBJ_AspectRatio_t AspectRatio;

	AspectRatio=CH_GetSTScreenARFromCH(Ratio);

	memset((void *)&InitParams, 0, sizeof(STLAYER_InitParams_t));
	memset((void *)&LayerParams, 0, sizeof(STLAYER_LayerParams_t));
	memset((void *)&LayerParams,0,sizeof(STLAYER_LayerParams_t));

	InitParams.CPUPartition_p=SystemPartition;
	InitParams.CPUBigEndian=FALSE;
	InitParams.SharedMemoryBaseAddress_p=(void*)RAM1_BASE_ADDRESS;
	InitParams.ViewPortNodeBuffer_p=NULL;
	InitParams.NodeBufferUserAllocated=FALSE;
	InitParams.ViewPortBuffer_p=NULL;
	InitParams.ViewPortBufferUserAllocated=FALSE;
	InitParams.LayerParams_p=&LayerParams;
#if 0
	InitParams.MaxViewPorts=1;/*2*/
	InitParams.MaxHandles=1;/*2*/
#else
	InitParams.MaxViewPorts=2;/*2*/
	InitParams.MaxHandles=2;/*2*/
#endif
	strcpy(InitParams.EventHandlerName,EVTDeviceName);

	switch(LAYERType)
	{
	case LAYER_GFX1:
		InitParams.LayerType=STLAYER_GAMMA_GDP;
		InitParams.AVMEM_Partition=AVMEMHandle[0] ;
		InitParams.BaseAddress_p=(void *)ST71XX_GDP1_LAYER_BASE_ADDRESS;
		InitParams.BaseAddress2_p =NULL; 
		InitParams.DeviceBaseAddress_p=NULL;

		LayerParams.Width=gVTGModeParam.ActiveAreaWidth;/*1920*/
		LayerParams.Height=gVTGModeParam.ActiveAreaHeight;/*1080*/
		LayerParams.AspectRatio=AspectRatio;
		LayerParams.ScanType=gVTGModeParam.ScanType;
		break;

	case LAYER_GFX2:
	case LAYER_GFX4:
		InitParams.LayerType=STLAYER_GAMMA_GDP;
		InitParams.AVMEM_Partition=AVMEMHandle[0] ;
#ifdef ST_7100
		InitParams.BaseAddress_p=(void *)ST71XX_GDP2_LAYER_BASE_ADDRESS;
#endif
#ifdef ST_7109
		InitParams.BaseAddress_p=(void *)ST71XX_GDP3_LAYER_BASE_ADDRESS;
#endif

		InitParams.BaseAddress2_p		=NULL; 
		InitParams.DeviceBaseAddress_p	=NULL;


		LayerParams.Width=gVTGAUXModeParam.ActiveAreaWidth;
		LayerParams.Height=gVTGAUXModeParam.ActiveAreaHeight;
		LayerParams.AspectRatio=AspectRatio;
		LayerParams.ScanType=gVTGAUXModeParam.ScanType;
		break;

	case LAYER_VIDEO1:
#ifdef ST_7100	
		InitParams.LayerType=STLAYER_HDDISPO2_VIDEO1; 
#endif
#ifdef ST_7109	
		InitParams.LayerType=STLAYER_DISPLAYPIPE_VIDEO1; 
#endif


#if 0 /*yxl 2007-01-16 modify below section*/
		InitParams.AVMEM_Partition=AVMEMHandle[1];
#else
		InitParams.AVMEM_Partition=/*AVMEMHandle[0] 20070808 change*/AVMEMHandle[TEST_AVM];
#endif
		InitParams.SharedMemoryBaseAddress_p   = (void *)NULL;
		InitParams.BaseAddress_p       = (void *)ST71XX_VID1_LAYER_BASE_ADDRESS;
		InitParams.BaseAddress2_p      = (void *)ST71XX_DISP0_BASE_ADDRESS;
		InitParams.DeviceBaseAddress_p=NULL;
		InitParams.MaxViewPorts=4;/*1*/
		InitParams.MaxHandles=2;/*1*/

		LayerParams.Width=gVTGModeParam.ActiveAreaWidth;/*1920*/
		LayerParams.Height=gVTGModeParam.ActiveAreaHeight;/*1080*/
		LayerParams.AspectRatio=AspectRatio;
		LayerParams.ScanType=gVTGModeParam.ScanType;

		break;
	case LAYER_VIDEO2:
		InitParams.LayerType=STLAYER_HDDISPO2_VIDEO2; 
		InitParams.AVMEM_Partition=/*AVMEMHandle[0] 20070808 change*/AVMEMHandle[TEST_AVM];
		InitParams.SharedMemoryBaseAddress_p   = (void *)NULL;
		InitParams.BaseAddress_p       = (void *)ST71XX_VID2_LAYER_BASE_ADDRESS;
		InitParams.BaseAddress2_p      = (void *)ST71XX_DISP1_BASE_ADDRESS;
		InitParams.DeviceBaseAddress_p=NULL;
		InitParams.MaxViewPorts=4;/*1*/
		InitParams.MaxHandles=2;/*1*/

		LayerParams.Width=gVTGAUXModeParam.ActiveAreaWidth;
		LayerParams.Height=gVTGAUXModeParam.ActiveAreaHeight;
		LayerParams.AspectRatio=AspectRatio;
		LayerParams.ScanType=gVTGAUXModeParam.ScanType;

		break;
		case LAYER_GFX3:
		InitParams.LayerType=STLAYER_GAMMA_GDP;
		InitParams.AVMEM_Partition=AVMEMHandle[0] ;
		InitParams.BaseAddress_p=(void *)ST71XX_GDP2_LAYER_BASE_ADDRESS;
		InitParams.BaseAddress2_p =NULL; 
		InitParams.DeviceBaseAddress_p=NULL;

		LayerParams.Width=gVTGModeParam.ActiveAreaWidth;/*1920*/
		LayerParams.Height=gVTGModeParam.ActiveAreaHeight;/*1080*/
		LayerParams.AspectRatio=AspectRatio;
		LayerParams.ScanType=gVTGModeParam.ScanType;
	       break;
	default:
		STTBX_Print(("\nYxlInfo:Undefined layer type"));
		return TRUE;
	}

	ErrCode=STLAYER_Init(LAYER_DeviceName[LAYERType],&InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_Init(%s)=%s %s\n",LAYER_DeviceName[LAYERType],GetErrorText(ErrCode),STLAYER_GetRevision()));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STLAYER_Init(%s)=%s %s\n",LAYER_DeviceName[LAYERType],GetErrorText(ErrCode),STLAYER_GetRevision()));
#endif

	ErrCode=STLAYER_Open(LAYER_DeviceName[LAYERType],&OpenParams,&LAYERHandle[LAYERType]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_Open(%s)=%s\n",LAYER_DeviceName[LAYERType],GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STLAYER_Open(%s)=%s\n",LAYER_DeviceName[LAYERType],GetErrorText(ErrCode)));
#endif

	ErrCode=STLAYER_SetLayerParams(LAYERHandle[LAYERType],&LayerParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_SetLayerParams(%s)=%s",LAYER_DeviceName[LAYERType],GetErrorText(ErrCode)));
		return TRUE;
	}

/*	CH_SetupViewPort(AspectRatio);*/


	return FALSE;
}

/*71XX*/
BOOL CH_SetupViewPort(LAYER_Type Lay,CH_VideoOutputAspect Ratio,CH_VideoOutputMode HDMode)
{
	int i;
	int PosXTemp;
	int PosYTemp;
	int WidthTemp;
	int HeightTemp;

	CH6_ViewPortParams_t CHVPParamTemp;

	STGXOBJ_AspectRatio_t AspectRatio;
	STVTG_TimingMode_t TimingMode[2];
	LAYER_Type LAYERType[2];
	CH_VideoOutputMode Mode[2];

	LAYERType[0]=LAYER_GFX1;
	LAYERType[1]=LAYER_GFX2;

	Mode[0]=HDMode; 
	Mode[1]=MODE_576I50;

	memset((void*)&CHVPParamTemp,0,sizeof(CH6_ViewPortParams_t));

	AspectRatio=CH_GetSTScreenARFromCH(Ratio);

	for(i=0;i<2;i++)
	{
		TimingMode[i]=CH_GetSTTimingModeFromCH(Mode[i]);
	
		
		if(LAYERType[i]==LAYER_VIDEO1||LAYERType[i]==LAYER_VIDEO2)
		{
			STTBX_Print(("\nYxlInfo:wrong layer type!"));
			return TRUE;
		}
		switch(TimingMode[i])	
		{
		case STVTG_TIMING_MODE_1080I60000_74250:
			PosXTemp=1;
			PosYTemp=41;
			break;
		case STVTG_TIMING_MODE_1080I50000_74250_1:/* 20070805 modify*/
			PosXTemp=46-10-10-4;
			PosYTemp=25-3-5-2;
			break;
		case STVTG_TIMING_MODE_720P60000_74250:
			PosXTemp=1;
			PosYTemp=11;
			break;
		case STVTG_TIMING_MODE_720P50000_74250:
			PosXTemp=1+30-3-5-8;
			PosYTemp=11+6-10+1;
			break;
		case STVTG_TIMING_MODE_576P50000_27000:/* cqj 20070726 modify*/
			PosXTemp=19-9;
			PosYTemp=14-10+3;	
			break;
		case STVTG_TIMING_MODE_576I50000_13500:
				
			PosXTemp=16+3-10;/*zxg 20070517 add +15*/
			PosYTemp=10+3-4;/*zxg 20070517 add +16*/
			break;
		default:
			PosXTemp=1;
			PosYTemp=11;
			break;  
		}
		if(LAYERType[i]==LAYER_GFX1)
		{
			WidthTemp=gVTGModeParam.ActiveAreaWidth-1;
			HeightTemp=gVTGModeParam.ActiveAreaHeight-1;
		}
		else 
		{
			WidthTemp=gVTGAUXModeParam.ActiveAreaWidth-1;
			HeightTemp=gVTGAUXModeParam.ActiveAreaHeight-1;
			
		}	
		CHVPParamTemp.ViewPortParams[i].AspectRatio=AspectRatio;
		CHVPParamTemp.ViewPortParams[i].PosX=PosXTemp;
		CHVPParamTemp.ViewPortParams[i].PosY=PosYTemp;
		CHVPParamTemp.ViewPortParams[i].Width=WidthTemp;
		CHVPParamTemp.ViewPortParams[i].Height=HeightTemp;
		CHVPParamTemp.ViewPortParams[i].BitmapType=STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
#ifdef OSD_COLOR_TYPE_RGB16
		CHVPParamTemp.ViewPortParams[i].ColorType=STGXOBJ_COLOR_TYPE_ARGB1555;
#else
		CHVPParamTemp.ViewPortParams[i].ColorType=STGXOBJ_COLOR_TYPE_ARGB8888;
#endif
		CHVPParamTemp.ViewPortParams[i].LayerHandle=LAYERHandle[LAYERType[i]];
		CHVPParamTemp.ViewPortParams[i].ViewportPartitionHandle=AVMEMHandle[0];

		CHVPParamTemp.ViewPortCount++;
	}
	
	if(pOSDVPMParams==NULL)
	{
		pOSDVPMParams=memory_allocate(SystemPartition,sizeof(CH6_ViewPortParams_t));
		if(pOSDVPMParams==NULL) 
		{
			STTBX_Print(("can't allocate memory for pOSDVPMParams"));
			return TRUE;
		}
	}

	memcpy((void*)pOSDVPMParams,(const void*)&CHVPParamTemp,sizeof(CH6_ViewPortParams_t));

	if(CH6_CreateViewPort(pOSDVPMParams,&CH6VPOSD,2))
	{
		STTBX_Print(("CH6_CreateViewPort() isn't successful"));
		return TRUE;
	}
	
#ifdef YXL_INIT_DEBUG
	else STTBX_Print(("CH6_CreateViewPort() is successful"));
#endif
/*
       CH6VPOSD.ViewPortHandle=(CH_ViewPortHandle_t)CH6VPOSD.LayerViewPortHandle[0];
	CH6VPOSD.ViewPortHandleAux=(CH_ViewPortHandle_t)CH6VPOSD.LayerViewPortHandle[1];
*/	
	return FALSE;
} /*end yxl 2007-02-05 modify below section*/


 
BOOL MIX_Setup(VMIX_Device MIXDevice,CH_VideoOutputAspect Ratio) /*71XX*/
{
	ST_ErrorCode_t ErrCode;
	STVMIX_InitParams_t InitParams;
	STVMIX_OpenParams_t OpenParams;
	ST_DeviceName_t     OutputArray[2];
	STVMIX_LayerDisplayParams_t LayerDisParams[4];
	STVMIX_LayerDisplayParams_t *VMIXLayerArray[4];
	STVMIX_ScreenParams_t ScreenParams;
	STGXOBJ_AspectRatio_t AspectRatio;

	AspectRatio=CH_GetSTScreenARFromCH(Ratio);

	memset(&InitParams,0,sizeof(STVMIX_InitParams_t));
	memset(&OpenParams,0,sizeof(STVMIX_OpenParams_t));
	memset(&ScreenParams,0,sizeof(STVMIX_ScreenParams_t)); 

	InitParams.CPUPartition_p=SystemPartition;
	InitParams.DeviceBaseAddress_p=NULL;
	InitParams.DeviceType=VMIX_GENERIC_GAMMA_TYPE_71XX;
	InitParams.MaxLayer=8;
	strcpy(InitParams.EvtHandlerName,EVTDeviceName);
 	InitParams.OutputArray_p=OutputArray;

	if(MIXDevice==VMIX_MAIN)
	{
		InitParams.MaxOpen=2;
		InitParams.BaseAddress_p=(void *)VMIX1_LAYER_BASE_ADDRESS;	/*(void *)ST71XX_VMIX1_BASE_ADDRESS	*/
		strcpy(InitParams.VTGName,VTGDeviceName[VTG_MAIN]);
#if 1
		strcpy(OutputArray[0], VOUTDeviceName[VOUT_MAIN]);
		strcpy( OutputArray[1], "" );
#else
		strcpy(OutputArray[0], VOUTDeviceName[VOUT_MAIN]);
		strcpy(OutputArray[1], VOUTDeviceName[VOUT_AUX]);
		strcpy( OutputArray[2], "" );
#endif

	}
	else 
	{
		InitParams.MaxOpen=2;
		InitParams.BaseAddress_p=(void *)VMIX2_LAYER_BASE_ADDRESS;/*(void *)ST71XX_VMIX2_BASE_ADDRESS*/		
		strcpy(InitParams.VTGName,VTGDeviceName[VTG_AUX]);
		strcpy(OutputArray[0], VOUTDeviceName[VOUT_AUX]);
		strcpy( OutputArray[1], "" );
	}

	ErrCode=STVMIX_Init(VMIXDeviceName[MIXDevice],&InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVMIX_Init()=%s %s\n",GetErrorText(ErrCode),STVMIX_GetRevision()));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STVMIX_Init()=%s %s\n",GetErrorText(ErrCode),STVMIX_GetRevision()));
#endif

	ErrCode=STVMIX_Open(VMIXDeviceName[MIXDevice],&OpenParams,&VMIXHandle[MIXDevice]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVMIX_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
			STTBX_Print(("STVMIX_Open()=%s\n",GetErrorText(ErrCode)));	
#endif

	ScreenParams.AspectRatio = AspectRatio;
	if(MIXDevice==VMIX_MAIN)
	{

		ErrCode=STVMIX_SetTimeBase(VMIXHandle[MIXDevice],VTGDeviceName[VTG_MAIN]);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STVMIX_SetTimeBase=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}
		ScreenParams.ScanType    = gVTGModeParam.ScanType;
		ScreenParams.FrameRate   = gVTGModeParam.FrameRate;
		ScreenParams.Width       = gVTGModeParam.ActiveAreaWidth;
		ScreenParams.Height      = gVTGModeParam.ActiveAreaHeight;
#if 0/* 20070805 modify equal with function <CH_SetSDVideoOutputMode>*/
		ScreenParams.XStart      = gVTGModeParam.ActiveAreaXStart;
		ScreenParams.YStart      = gVTGModeParam.ActiveAreaYStart;
#else
		ScreenParams.XStart      = gVTGModeParam.DigitalActiveAreaXStart;
		ScreenParams.YStart      = gVTGModeParam.DigitalActiveAreaYStart;

		if (ScreenParams.YStart%2) /* if odd */
			ScreenParams.YStart++;	
#endif
		ErrCode = STVMIX_SetScreenParams(VMIXHandle[MIXDevice], &ScreenParams);
		if (ErrCode!= ST_NO_ERROR)
		{
			STTBX_Print(("VMIX_SetScreenParams=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}
#ifndef USE_STILL
             strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_VIDEO1]);
		strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_GFX1]);
	

           
		LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=TRUE;/*FALSE;*/
		
		VMIXLayerArray[0]=&LayerDisParams[0];
		VMIXLayerArray[1]=&LayerDisParams[1];
		VMIXLayerArray[2]=&LayerDisParams[2];
		task_delay(8000);
		ErrCode=STVMIX_ConnectLayers(VMIXHandle[MIXDevice],(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,2);

#else
	       strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_GFX3]);
		strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_VIDEO1]);
		strcpy(LayerDisParams[2].DeviceName,LAYER_DeviceName[LAYER_GFX1]);
	

           
		LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=TRUE;/*FALSE;*/
		
		VMIXLayerArray[0]=&LayerDisParams[0];
		VMIXLayerArray[1]=&LayerDisParams[1];
		VMIXLayerArray[2]=&LayerDisParams[2];
		task_delay(8000);
		ErrCode=STVMIX_ConnectLayers(VMIXHandle[MIXDevice],(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,3);

#endif
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STVMIX_ConnectLayers()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}

	}
	else
	{
		ErrCode=STVMIX_SetTimeBase(VMIXHandle[MIXDevice],VTGDeviceName[VTG_AUX]);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STVMIX_SetTimeBase=%s\n", GetErrorText(ErrCode)));
			return( ErrCode );
		}
		ScreenParams.ScanType    = gVTGAUXModeParam.ScanType;
		ScreenParams.FrameRate   = gVTGAUXModeParam.FrameRate;
		ScreenParams.Width       = gVTGAUXModeParam.ActiveAreaWidth;
		ScreenParams.Height      = gVTGAUXModeParam.ActiveAreaHeight;
#if 0/* 20070805 modify equal with function <CH_SetSDVideoOutputMode>*/
		ScreenParams.XStart      = gVTGAUXModeParam.ActiveAreaXStart;
		ScreenParams.YStart      = gVTGAUXModeParam.ActiveAreaYStart;
#else
		ScreenParams.XStart      = gVTGAUXModeParam.DigitalActiveAreaXStart;
#if 0/*20070914修改为了入网测试*/
		ScreenParams.YStart      = gVTGAUXModeParam.DigitalActiveAreaYStart;
#else
		ScreenParams.YStart      = 48;

#endif
		if (ScreenParams.YStart%2) /* if odd */
			ScreenParams.YStart++;	
#endif
		ErrCode = STVMIX_SetScreenParams(VMIXHandle[MIXDevice], &ScreenParams);
		if (ErrCode!= ST_NO_ERROR)
		{
			STTBX_Print(("VMIX_SetScreenParams=%s\n", GetErrorText(ErrCode) ));
			return TRUE;
		}
#ifndef	USE_STILL
		strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_VIDEO2]);/**/
		strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_GFX2]);

		
		LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=TRUE;/*FALSE;*/
		
		VMIXLayerArray[0]=&LayerDisParams[0];
		VMIXLayerArray[1]=&LayerDisParams[1];
		VMIXLayerArray[2]=&LayerDisParams[2];
		task_delay(8000);
		ErrCode=STVMIX_ConnectLayers(VMIXHandle[MIXDevice],(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,2);
#else
	       strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_GFX4]);
		strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_VIDEO2]);/**/
		strcpy(LayerDisParams[2].DeviceName,LAYER_DeviceName[LAYER_GFX2]);

		
		LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=TRUE;/*FALSE;*/
		
		VMIXLayerArray[0]=&LayerDisParams[0];
		VMIXLayerArray[1]=&LayerDisParams[1];
		VMIXLayerArray[2]=&LayerDisParams[2];
		task_delay(8000);
		ErrCode=STVMIX_ConnectLayers(VMIXHandle[MIXDevice],(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,3);
#endif
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STVMIX_ConnectLayers()=%s",GetErrorText(ErrCode)));
			return TRUE;
		}

	
	}

	ErrCode=STVMIX_Enable(VMIXHandle[MIXDevice]);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Enable()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	if(MIXDevice==VMIX_MAIN)
	{
	/*	if((CH_GetChipVersion()&0xF0)!=0xA0)
			*(DU32 *)(ST71XX_VOS_BASE_ADDRESS+0xDC)=0x180;*/
	}
/*VMIX_SetBKC(MIXDevice,true,255,0,0);*/

	return FALSE;
}


ST_ErrorCode_t VMIX_SetBKC(VMIX_Device MIXDevice, BOOL enable, S32 red, S32 green, S32 blue )
{
    ST_ErrorCode_t     ErrCode;
    STGXOBJ_ColorRGB_t RGB888;

    RGB888.R = (U8) red;
    RGB888.G = (U8) green;
    RGB888.B = (U8) blue;

    ErrCode = STVMIX_SetBackgroundColor(VMIXHandle[MIXDevice], RGB888, enable);

    if (ErrCode == ST_NO_ERROR)
	{
       STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                     "   Background Color enable, RGB888 : R=0x%02x G=0x%02x B=0x%02x",
                     RGB888.R, RGB888.G, RGB888.B));
	}
	else
	{
    	STTBX_Print(( "STVMIX_SetBackgroundColor=%s", GetErrorText(ErrCode)));
	}

    return( ErrCode );

}


 
BOOL VOUT_Setup(VOUT_Type VOUTDevice,CH_VideoOutputType OutType) /*71XX*/
{
	ST_ErrorCode_t ErrCode;

	STVOUT_Capability_t Capability;
	STVOUT_InitParams_t InitParams;
	STVOUT_OpenParams_t OpenParams;
	STVOUT_OutputParams_t OutputParams;
	STVOUT_OutputType_t OutputType;


	memset(&InitParams  ,0,sizeof(STVOUT_InitParams_t));     
	memset(&OpenParams  ,0,sizeof(STVOUT_OpenParams_t));
	memset(&OutputParams,0,sizeof(STVOUT_OutputParams_t));

	OutputType=CH_GetSTVideoOutputTypeFromCH(OutType);

	InitParams.CPUPartition_p=SystemPartition;
	InitParams.MaxOpen=3;
	InitParams.DeviceType=VOUT_DEVICE_TYPE_71XX;
	InitParams.OutputType=OutputType;
	
	
	
	switch(InitParams.OutputType)
	{
#ifdef MODEY_576I 
	case STVOUT_OUTPUT_ANALOG_YUV:
	case STVOUT_OUTPUT_ANALOG_RGB:

		InitParams.Target.DualTriDacCell.BaseAddress_p=(void*)ST71XX_VOUT_BASE_ADDRESS;
		InitParams.Target.DualTriDacCell.DeviceBaseAddress_p=NULL;
		strcpy(InitParams.Target.DualTriDacCell.DencName,DENCDeviceName);

		if(InitParams.OutputType==STVOUT_OUTPUT_ANALOG_RGB)
			InitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_4 | STVOUT_DAC_5 | STVOUT_DAC_6;	
		else InitParams.Target.DualTriDacCell.DacSelect =STVOUT_DAC_1 | STVOUT_DAC_2;
		InitParams.Target.DualTriDacCell.HD_Dacs = TRUE;
		InitParams.Target.DualTriDacCell.Format  = STVOUT_SD_MODE;
		break;
#endif 
	case STVOUT_OUTPUT_HD_ANALOG_RGB:
	case STVOUT_OUTPUT_HD_ANALOG_YUV:
		InitParams.Target.DualTriDacCell.BaseAddress_p=(void*)ST71XX_VOUT_BASE_ADDRESS;
		InitParams.Target.DualTriDacCell.DeviceBaseAddress_p=NULL;
		strcpy(InitParams.Target.DualTriDacCell.DencName,DENCDeviceName);
		InitParams.Target.DualTriDacCell.HD_Dacs = TRUE;/*FALSE;*/
		InitParams.Target.DualTriDacCell.Format  = STVOUT_HD_MODE;
		InitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_4 | STVOUT_DAC_5 | STVOUT_DAC_6;


		break;
	case STVOUT_OUTPUT_ANALOG_CVBS:
		InitParams.Target.DualTriDacCell.BaseAddress_p=(void*)ST71XX_VOUT_BASE_ADDRESS;
		InitParams.Target.DualTriDacCell.DeviceBaseAddress_p=NULL;
		strcpy(InitParams.Target.DualTriDacCell.DencName,DENCDeviceName);
		InitParams.Target.DualTriDacCell.HD_Dacs = FALSE;
#ifdef ST_7100
		InitParams.Target.DualTriDacCell.DacSelect = STVOUT_DAC_3;
#endif

#ifdef ST_7109 
        InitParams.Target.DualTriDacCell.DacSelect = (STVOUT_DAC_t)(STVOUT_DAC_6);
		InitParams.Target.DualTriDacCell.Format  = STVOUT_SD_MODE;
#endif

		break;

	case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888:
		InitParams.Target.HDMICell.InterruptNumber = ST71XX_HDMI_INTERRUPT;
		InitParams.Target.HDMICell.InterruptLevel = HDMI_INTERRUPT_LEVEL;
		InitParams.Target.HDMICell.IsHDCPEnable = FALSE;
		strcpy(InitParams.Target.HDMICell.EventsHandlerName, EVTDeviceName);
		strcpy(InitParams.Target.HDMICell.VTGName, VTGDeviceName[VTG_MAIN]);
		InitParams.Target.HDMICell.HSyncActivePolarity = FALSE;
		InitParams.Target.HDMICell.VSyncActivePolarity = FALSE;
#ifdef ST_DEMO_PLATFORM /*yxl 2007-02-01 add below section for HDMI*/ 
		strcpy(InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice, I2C_DeviceName[1]);
#else
#ifdef YXL_STM28_FLASH /*yxl 2007-03-07 modify ,below is old 71XX Board*/
		strcpy(InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice, I2C_DeviceName[2]);
#else
		strcpy(InitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice, I2C_DeviceName[1]);
#endif
#endif
		strcpy(InitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice, PIO_DeviceName[2]);
		InitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit=PIO_BIT_2;
		InitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed       = FALSE;
		InitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p = NULL;
		InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p = (void *)ST71XX_HDMI_BASE_ADDRESS;
		InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)ST71XX_HDCP_BASE_ADDRESS;

	
		break;      
	
	default:
		STTBX_Print(("\nYxlInfo:undefined output type"));
		return TRUE;
	}

	ErrCode=STVOUT_Init(VOUTDeviceName[VOUTDevice],&InitParams);
	
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Init(%d)=%s\n",VOUTDevice,GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STVOUT_Init()=%s\n",GetErrorText(ErrCode)));	
#endif
	
	ErrCode=STVOUT_Open(VOUTDeviceName[VOUTDevice],&OpenParams,&VOUTHandle[VOUTDevice]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STVOUT_Open()=%s\n",GetErrorText(ErrCode)));
#endif

	if(VOUTDevice==VOUT_MAIN||VOUTDevice==VOUT_AUX)
	{
		ErrCode=STVOUT_SetInputSource(VOUTHandle[VOUTDevice],STVOUT_SOURCE_MAIN);
		if(ErrCode!=ST_NO_ERROR) 
		{
			STTBX_Print(("STVOUT_SetInputSource()=%s\n",GetErrorText(ErrCode)));	
			return TRUE;
		}
	}

	if(VOUTDevice==VOUT_MAIN)
	{
		OutputParams.Analog.StateAnalogLevel  = STVOUT_PARAMS_NOT_CHANGED;
		OutputParams.Analog.StateBCSLevel     = STVOUT_PARAMS_NOT_CHANGED;
		OutputParams.Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED;
		OutputParams.Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED;
		OutputParams.Analog.InvertedOutput    = FALSE;
		     
		if(InitParams.OutputType==STVOUT_OUTPUT_HD_ANALOG_YUV||\
			InitParams.OutputType==STVOUT_OUTPUT_ANALOG_YUV)
		{ 
			OutputParams.Analog.EmbeddedType      = TRUE; 
			OutputParams.Analog.SyncroInChroma    = FALSE;
		}
		else
		{
			
			OutputParams.Analog.EmbeddedType      = FALSE;
			OutputParams.Analog.SyncroInChroma    = TRUE;
		}
		OutputParams.Analog.ColorSpace        = STVOUT_ITU_R_601;
		ErrCode=STVOUT_SetOutputParams(VOUTHandle[VOUTDevice],&OutputParams);
		if(ErrCode!=ST_NO_ERROR) 
		{
			STTBX_Print(("STVOUT_SetOutputParams()=%s\n",GetErrorText(ErrCode)));	
			return TRUE;
		}


	}
	else if(VOUTDevice==VOUT_HDMI)
	{
		if(OutType==TYPE_DVI)
			OutputParams.HDMI.ForceDVI=TRUE;
		else OutputParams.HDMI.ForceDVI=FALSE;

		OutputParams.HDMI.AudioFrequency = 48000;
		OutputParams.HDMI.IsHDCPEnable   = FALSE;
		ErrCode=STVOUT_SetOutputParams(VOUTHandle[VOUTDevice],&OutputParams);
		if(ErrCode!=ST_NO_ERROR) 
		{
			STTBX_Print(("STVOUT_SetOutputParams()=%s\n",GetErrorText(ErrCode)));	
			return TRUE;
		}
	}
	/*yxl 2007-06-21 add below section*/
	else if(VOUTDevice==VOUT_AUX)
	{
        OutputParams.Analog.StateAnalogLevel  = STVOUT_PARAMS_DEFAULT;
        OutputParams.Analog.StateBCSLevel     = STVOUT_PARAMS_DEFAULT;
        OutputParams.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
        OutputParams.Analog.EmbeddedType      = FALSE;
        OutputParams.Analog.InvertedOutput    = FALSE;
        OutputParams.Analog.SyncroInChroma    = FALSE;
        OutputParams.Analog.ColorSpace        = STVOUT_ITU_R_601;
		ErrCode=STVOUT_SetOutputParams(VOUTHandle[VOUTDevice],&OutputParams);
		if(ErrCode!=ST_NO_ERROR) 
		{
			STTBX_Print(("STVOUT_SetOutputParams()=%s\n",GetErrorText(ErrCode)));	
			return TRUE;
		}

	}
	/*end yxl 2007-06-21 add below section*/


	if(VOUTDevice!=VOUT_HDMI)
	{
		ErrCode=STVOUT_Enable(VOUTHandle[VOUTDevice]);
		if(ErrCode!=ST_NO_ERROR) 
		{
			STTBX_Print(("STVOUT_Enable()=%s\n",GetErrorText(ErrCode)));	
			return TRUE;
		}
	}

	return FALSE;
}


extern void DispNewFrameEvt(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, 
							const void *EventData);


BOOL VID_Setup(VID_Device VIDDevice) /*71XX*/
{
    ST_ErrorCode_t ErrCode;
	STVID_InitParams_t InitParams;
	STVID_OpenParams_t OpenParams;

	STVID_ViewPortParams_t ViewPortParams;
	STVID_MemoryProfile_t MemoryProfile;
	STVID_SetupParams_t   SetupParams;

    memset(&InitParams, 0, sizeof(STVID_InitParams_t));    


	switch(VIDDevice)
	{
	case VID_MPEG2:
		InitParams.DeviceType=VID_DEVICE_TYPE_71XX_MPEG;
		InitParams.BaseAddress_p=(void *)ST71XX_VIDEO_BASE_ADDRESS;
		InitParams.BaseAddress2_p = (void *)ST71XX_DISP0_BASE_ADDRESS;  
		
		InitParams.BaseAddress3_p = NULL;
#ifdef ST_7109 /*yxl 2007-06-11 add bleow*/	
		InitParams.BaseAddress3_p = (void *)ST71XX_DISP0_BASE_ADDRESS;
#endif 
		InitParams.DeviceBaseAddress_p=NULL;
		InitParams.BitBufferAllocated=false;
		InitParams.BitBufferAddress_p=NULL;
		InitParams.BitBufferSize=0;  
		
		InitParams.InstallVideoInterruptHandler=TRUE;
		InitParams.InterruptNumber=ST71XX_GLH_INTERRUPT;
		InitParams.InterruptLevel=VIDEO_INTERRUPT_LEVEL;  
		
		InitParams.InterruptEvent=0;
		strcpy(InitParams.InterruptEventName,EVTDeviceName);
#if 1	 /*yxl 2007-06-10 temp modify*/	
		InitParams.SharedMemoryBaseAddress_p=(void*)RAM2_BASE_ADDRESS;
#else
		InitParams.SharedMemoryBaseAddress_p=(void*)0;
#endif
		InitParams.MaxOpen=2;/*1*/
		InitParams.AVSYNCDriftThreshold=2 * (90000 / 50);
		InitParams.CPUPartition_p=SystemPartition;
		InitParams.AVMEMPartition=AVMEMHandle[0] /*20070808 changeAVMEMHandle[1]*/;
		InitParams.UserDataSize=75;
		
		strcpy(InitParams.VINName,"");
		strcpy(InitParams.EvtHandlerName,EVTDeviceName);
		strcpy(InitParams.ClockRecoveryName,CLKRVDeviceName);

		break;
			
	case VID_H:
		InitParams.DeviceType=VID_DEVICE_TYPE_71XX_H264;
		InitParams.BaseAddress_p=(void *)ST71XX_DELTA_BASE_ADDRESS;
		InitParams.BaseAddress2_p = (void *)ST71XX_DISP0_BASE_ADDRESS;              
		InitParams.BaseAddress3_p = NULL;
#ifdef ST_7109 /*yxl 2007-06-11 add bleow*/	
		InitParams.BaseAddress3_p = (void *)ST71XX_DISP0_BASE_ADDRESS;
#endif 

		InitParams.DeviceBaseAddress_p=NULL;
		InitParams.BitBufferAllocated=false;
		InitParams.BitBufferAddress_p=NULL;
		InitParams.BitBufferSize=0;  
		
		InitParams.InstallVideoInterruptHandler=TRUE;
		InitParams.InterruptNumber=ST71XX_VID_DPHI_PRE0_INTERRUPT;
		InitParams.InterruptLevel=0;  
		
		InitParams.InterruptEvent=0;
		strcpy(InitParams.InterruptEventName,EVTDeviceName);
		
		InitParams.SharedMemoryBaseAddress_p=NULL;
		InitParams.MaxOpen=1;
		InitParams.AVSYNCDriftThreshold=2 * (90000 / 50);
		InitParams.CPUPartition_p=SystemPartition;
		InitParams.AVMEMPartition=/*AVMEMHandle[0] 20070808 change*/AVMEMHandle[0];/*20071016 add要解码H.264必须为0*/
		InitParams.UserDataSize=75;
		
	/*	strcpy(InitParams.VINName,"");*/
		strcpy(InitParams.EvtHandlerName,EVTDeviceName);
		strcpy(InitParams.ClockRecoveryName,CLKRVDeviceName);

		break;


	}
   
	ErrCode=STVID_Init(VIDDeviceName[VIDDevice],&InitParams);

	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVID_Init()=%s %s\n",GetErrorText(ErrCode),STVID_GetRevision()));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
   /* STTBX_Print(("%s\n", STVID_GetRevision() ));*/
#endif
			
/*yxl 2006-04-12 add below section for test*/
{
#ifdef VIDEO_DEBUG_EVENT  

	extern void avc_Vid1DispNewFrameCallBack(STEVT_CallReason_t     Reason,
		const ST_DeviceName_t  RegistrantName,
		STEVT_EventConstant_t  Event, 
		const void            *EventData,
		const void            *SubscriberData_p);
	extern void avc_VideoEvent_CallbackProc(STEVT_CallReason_t     Reason,
		const ST_DeviceName_t  RegistrantName,
		STEVT_EventConstant_t  Event, 
		const void            *EventData,
		const void            *SubscriberData_p);
	
	STEVT_DeviceSubscribeParams_t SubscribeParams;        
	
	int event;
	
	
	SubscribeParams.NotifyCallback = avc_Vid1DispNewFrameCallBack;
	
	
	if (STEVT_SubscribeDeviceEvent(EVTHandle, VIDDeviceName[VIDDevice], (U32)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT, &SubscribeParams))
	{
		STTBX_Print(( "[%d] Fatal\n", __LINE__));
	}
	if (STEVT_SubscribeDeviceEvent(EVTHandle, VIDDeviceName[VIDDevice], (U32)STVID_FRAME_RATE_CHANGE_EVT, &SubscribeParams))
	{
		STTBX_Print(( "[%d] Fatal\n", __LINE__));
	}	
	if (STEVT_SubscribeDeviceEvent(EVTHandle, VIDDeviceName[VIDDevice], (U32)STVID_OUT_OF_SYNC_EVT, &SubscribeParams))
	{
		STTBX_Print(( "[%d] Fatal\n", __LINE__));
	}
	if (STEVT_SubscribeDeviceEvent(EVTHandle, VIDDeviceName[VIDDevice], (U32)STVID_BACK_TO_SYNC_EVT, &SubscribeParams))
	{
		STTBX_Print(( "[%d] Fatal\n", __LINE__));
	}
	
	SubscribeParams.NotifyCallback = avc_VideoEvent_CallbackProc;
	for(event =STVID_ASPECT_RATIO_CHANGE_EVT; event <= STVID_SYNCHRONISATION_CHECK_EVT; event++)
	{
		if (event != STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT 
			&& event != STVID_FRAME_RATE_CHANGE_EVT
			&& event != STVID_OUT_OF_SYNC_EVT
			&& event != STVID_BACK_TO_SYNC_EVT
			)
		{
			ErrCode = STEVT_SubscribeDeviceEvent(EVTHandle, VIDDeviceName[VIDDevice], (U32)event, &SubscribeParams);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("EVT_Subscribe(%d, Video)=%s\n", event, GetErrorText(ErrCode) ));
				return(ErrCode);
			}
		}
	}
	

#endif
}
/*end yxl 2006-04-12 add below section for test*/



	OpenParams.SyncDelay = 0;
    ErrCode = STVID_Open(VIDDeviceName[VIDDevice], &OpenParams, &VIDHandle[VIDDevice]);
    if (ErrCode != ST_NO_ERROR) 
	{
		STTBX_Print(("STVID_Open()=%s\n", GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		 	STTBX_Print(("STVID_Open()=%s\n", GetErrorText(ErrCode)));
#endif	




#if 0
	MemoryProfile.MaxWidth         = gVTGModeParam.ActiveAreaWidth;
    MemoryProfile.MaxHeight        = gVTGModeParam.ActiveAreaHeight;
#else  	
	MemoryProfile.MaxWidth = 1920;/*+200; for test*/
	MemoryProfile.MaxHeight = 1088;/*+100; */
#endif

#if 0 /*yxl 2005-08-11 modify this section*/

	MemoryProfile.NbFrameStore = 4;

	MemoryProfile.CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
	MemoryProfile.DecimationFactor = STVID_DECIMATION_FACTOR_NONE;
#else
	MemoryProfile.NbFrameStore = 5;
	MemoryProfile.CompressionLevel = STVID_COMPRESSION_LEVEL_NONE;
	MemoryProfile.DecimationFactor = STVID_DECIMATION_FACTOR_2;/*H,2*/

#endif



    ErrCode = STVID_SetMemoryProfile(VIDHandle[VIDDevice], &MemoryProfile );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_SetMemoryProfile()=%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }

	if(VIDDevice==VID_MPEG2)
	{
		ErrCode=STVID_SetDecodedPictures(VIDHandle[VIDDevice],STVID_DECODED_PICTURES_ALL);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
	}


	if(VIDDevice==VID_H)
	{
		ErrCode=STVID_DisableFrameRateConversion(VIDHandle[VIDDevice]);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
	}


	SetupParams.SetupObject = STVID_SETUP_FRAME_BUFFERS_PARTITION;
	SetupParams.SetupSettings.AVMEMPartition = AVMEMHandle[TEST_AVM];
	ErrCode=STVID_Setup(VIDHandle[VIDDevice],&SetupParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_Setup()=%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }

	SetupParams.SetupObject = STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION;
	SetupParams.SetupSettings.AVMEMPartition = AVMEMHandle[TEST_AVM];
	ErrCode=STVID_Setup(VIDHandle[VIDDevice],&SetupParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_Setup()=%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }

	if(VIDDevice==VID_H)
	{
		SetupParams.SetupObject = STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION;
		SetupParams.SetupSettings.AVMEMPartition = AVMEMHandle[TEST_AVM];
		ErrCode=STVID_Setup(VIDHandle[VIDDevice],&SetupParams);
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STVID_Setup()=%s\n", GetErrorText(ErrCode) ));
			return TRUE;
		}
	}


#if 1
    ErrCode = STVID_SetErrorRecoveryMode(VIDHandle[VIDDevice],/*STVID_ERROR_RECOVERY_HIGH*/STVID_ERROR_RECOVERY_PARTIAL);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_SetErrorRecoveryMode()=%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }

#endif /*yxl 2007-01-02 cancel below*/
	
	return FALSE;		

}


/*yxl 2007-04-18 add below function*/
BOOL VID_CloseViewport(VID_Device VIDDevice,LAYER_Type LAYERType)
{
    ST_ErrorCode_t ErrCode;
	STVID_ViewPortParams_t ViewPortParams;
	STVID_WindowParams_t OutWindowParams_p;
	STVID_DisplayAspectRatioConversion_t ConversionTemp;
	BOOL OutWindowAutoMode;
	U8 LayTemp;


	if(LAYERType!=LAYER_VIDEO1&&LAYERType!=LAYER_VIDEO2)
	{
		STTBX_Print(("\nYxlInfo:VID_CloseViewport(),Bad paramter")); 
		return TRUE;
	}


	/*yxl 2007-04-19 add below section*/
	if(VIDVPHandle[VIDDevice][LAYERType]==0)
	{
#ifdef INIT_DEBUG
		STTBX_Print(("\nYxlInfo:Invalid viewport handle"));
#endif 
		return TRUE;
	}
	/*end yxl 2007-04-19 add below section*/

	
	ErrCode=STVID_CloseViewPort(VIDVPHandle[VIDDevice][LAYERType]);
	VIDVPHandle[VIDDevice][LAYERType]=0;/*yxl 2007-04-19 add this line*/
	if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print(("STVID_CloseViewPort()=%s",GetErrorText(ErrCode)));
        return TRUE;
    }
#ifdef YXL_INIT_DEBUG
//	else STTBX_Print(("STVID_CloseViewPort()=%s %s \n ",GetErrorText(ErrCode),ViewPortParams.LayerName));
#endif



	return FALSE;	

}
/*yxl 2007-04-18 add below function*/


BOOL VID_OpenViewport(VID_Device VIDDevice,LAYER_Type LAYERType,CH_VideoARConversion ARConversion)
{
    ST_ErrorCode_t ErrCode;
	STVID_ViewPortParams_t ViewPortParams;
	STVID_WindowParams_t OutWindowParams_p;
	STVID_DisplayAspectRatioConversion_t ConversionTemp;
	BOOL OutWindowAutoMode;
	U8 LayTemp;

	if(LAYERType!=LAYER_VIDEO1&&LAYERType!=LAYER_VIDEO2)
	{
		STTBX_Print(("\nYxlInfo:VID_OpenViewport(),Bad paramter")); 
		return TRUE;
	}

    memset((void *)&ViewPortParams, 0, sizeof(STVID_ViewPortParams_t));
	strcpy(ViewPortParams.LayerName,LAYER_DeviceName[LAYERType]);

	ErrCode=STVID_OpenViewPort(VIDHandle[VIDDevice],&ViewPortParams,&VIDVPHandle[VIDDevice][LAYERType]);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVID_OpenViewPort()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	else STTBX_Print(("STVID_OpenViewPort()=%s %s \n ",GetErrorText(ErrCode),ViewPortParams.LayerName));
#endif

	ConversionTemp=CH_GetSTVideoARConversionFromCH(ARConversion);
	ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[VIDDevice][LAYERType], ConversionTemp);

	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_SetDisplayAspectRatioConversion()=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

	return FALSE;		

}



/*yxl 2007-01-22 add below sections*/
BOOL MBX_Setup(void)
{
	STAVMEM_AllocBlockParams_t            AVMEM_AllocBlockParams;
	STAVMEM_SharedMemoryVirtualMapping_t  AVMEM_VirtualMapping;
	void                                 *AVMEM_VirtualAddress;
	void                                 *AVMEM_DeviceAddress;
	ST_ErrorCode_t                        ErrCode  = ST_NO_ERROR;
	EMBX_ERROR                            EmbxCode = EMBX_SUCCESS;
	MME_ERROR                             MmeCode  = MME_SUCCESS;
#if defined(LX_FIRMWARE_FROM_JTAG)
	U32                                  *FILE_Handle;
#endif
	U8                                   *Lx_Code_Address;
	U32                                   Lx_Code_Size;
	U32                                   i;

	
	/* Init the Lx for Video Decode */
	/* ============================ */
	/* Reset-out bypass for LX Deltaphi, Audio */
	STB71XX_system_config9  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
	STB71XX_system_config29 = 0x00000341; 
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
	STB71XX_system_config28 = RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+VIDEO_COMPANION_OFFSET+1;

       /*20070709 add先把companison 数据从FLASH中解压缩到临时内存0xB0000000*/
	 gpu8_DecompressBuffer=memory_allocate(DecompressBuffer_partition,DecompressBuffer_SIZE);
       CH_UnCompressCompanision((U32 *)COMPANISON_FLASHZIP_ADDR,(U32 *)gpu8_DecompressBuffer);
	/********************************/
 
	/* Load the lx code */
	/* ---------------- */
#if defined(LX_FIRMWARE_FROM_JTAG)
	if ((CH_GetChipVersion()&0xF0)==0xA0) FILE_Handle=SYS_FOpen("mbx/companion_71XX_video_Ax.bin","rb"); 
	else FILE_Handle=SYS_FOpen("mbx/companion_71XX_video_BxCx.bin","rb");
	if (FILE_Handle==NULL)
	{
		return(ST_ERROR_NO_MEMORY);
	} 
	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET);
	Lx_Code_Size    = SYS_FRead((void *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET),1,0x800000,FILE_Handle);
	SYS_FClose(FILE_Handle);
#else
#if 0 /*yxl 2007-06-10 cancel below section*/
	/* Load video firmware for cut 1.x */
	if ((CH_GetChipVersion()&0xF0)==0xA0)
	{
		Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET);
		Lx_Code_Size    = MAIN_COMPANION_71XX_VIDEO_AX_CODE_SIZE;
		for (i=0;i<MAIN_COMPANION_71XX_VIDEO_AX_CODE_SIZE;i++)
		{
			Lx_Code_Address[i] = Main_Companion_71XX_Video_Ax_Code[i];   
		}
	}
	/* Else for other firmwares */
	else
#endif /*end yxl 2007-06-10 cancel below section*/
	{

#ifdef ST_7100

		Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET);
		Lx_Code_Size    = MAIN_COMPANION_71XX_VIDEO_BXCX_CODE_SIZE;
		for (i=0;i<MAIN_COMPANION_71XX_VIDEO_BXCX_CODE_SIZE;i++)
		{
			Lx_Code_Address[i] = Main_Companion_71XX_Video_BxCx_Code[i];   
		}
#endif 

#ifdef ST_7109  /*yxl 2007-06-08 add below section*/

		Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+VIDEO_COMPANION_OFFSET);
		Lx_Code_Size    = MAIN_COMPANION_VIDEO_CODE_SIZE;
		for (i=0;i<MAIN_COMPANION_VIDEO_CODE_SIZE;i++)
		{
		#if 0/*20070709 修改从临时内存获取数据*/
			Lx_Code_Address[i] = Main_Companion_Video_Code[i];   
		#else
                    Lx_Code_Address[i] =*(U8 *)(gpu8_DecompressBuffer+i);   
		#endif
		}

#endif /*end yxl 2007-06-08 add below section*/

	}
#endif
	
	/* Identify firmware version */
	/* ------------------------- */
	memset(LX_VIDEO_Revision,0,512);
	for (i=0;i<Lx_Code_Size-sizeof("VideoLX");i++)
	{
		if (memcmp((char *)&Lx_Code_Address[i],"VideoLX",sizeof("VideoLX")-1)==0)
		{
			strncpy((char *)LX_VIDEO_Revision,(char *)&Lx_Code_Address[i],512);
			break;
		}
	}
	
	/* Reset the LX and go on */
	/* ---------------------- */
	/* disable reset lx_dh -> boot (+ set periph address again...) */
	STB71XX_system_config29 = 0x00000340;
	
	/* Init the Lx for Audio Decode */
	/* ============================ */
	/* Reset-out bypass for LX Deltaphi, Audio */
	STB71XX_system_config9  = 0x50000A8C;
	/* Enable reset lx_dh + set perih addr to 0x1A000000 */
	STB71XX_system_config27 = 0x00000345;
	/* Enable lx_dh to boot at 0x11000000 in system_config28 (TODO: fixed value for boot address) */
	STB71XX_system_config26 = RAM1_BASE_ADDRESS_SEEN_FROM_DEVICE1+AUDIO_COMPANION_OFFSET+1;
	
	/* Load the lx code */
	/* ---------------- */
#if defined(LX_FIRMWARE_FROM_JTAG)
	FILE_Handle=SYS_FOpen("mbx/companion_71XX_audio.bin","rb");
	if (FILE_Handle==NULL)
	{
		return(ST_ERROR_NO_MEMORY);
	} 
	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET);
	Lx_Code_Size    = SYS_FRead((void *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET),1,0x800000,FILE_Handle);
	SYS_FClose(FILE_Handle);
#else

#ifdef ST_7100

	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET);
	Lx_Code_Size    = MAIN_COMPANION_71XX_AUDIO_CODE_SIZE;
	for (i=0;i<MAIN_COMPANION_71XX_AUDIO_CODE_SIZE;i++)
	{

             Lx_Code_Address[i] = *(U32 *)(FLASH_FONT_ADDR+MAIN_COMPANION_VIDEO_CODE_SIZE+i);   
	}
#endif
	
#ifdef ST_7109  /*yxl 2007-06-08 add below section*/

	Lx_Code_Address = (U8 *)(RAM1_BASE_ADDRESS+AUDIO_COMPANION_OFFSET);
	Lx_Code_Size    = MAIN_COMPANION_AUDIO_CODE_SIZE;
	for (i=0;i<MAIN_COMPANION_AUDIO_CODE_SIZE;i++)
	{
#if 0/*20070709 修改从临时内存获取数据*/
		Lx_Code_Address[i] = Main_Companion_Audio_Code[i]; 
	#else
             Lx_Code_Address[i] = *(U8 *)(gpu8_DecompressBuffer+MAIN_COMPANION_VIDEO_CODE_SIZE+i);   
	#endif
	}


#endif /*end yxl 2007-06-08 add below section*/



#endif
	
	/* Identify firmware version */
	/* ------------------------- */
	memset(LX_AUDIO_Revision,0,512);
	for (i=0;i<Lx_Code_Size-sizeof("COMPANION_TAG_BEGIN");i++)
	{
		if (memcmp((char *)&Lx_Code_Address[i],"COMPANION_TAG_BEGIN",sizeof("COMPANION_TAG_BEGIN")-1)==0)
		{
			strncpy((char *)LX_AUDIO_Revision,(char *)&Lx_Code_Address[i],512);
			break;
		}
	}
	
	/* Reset the LX and go on */
	/* ---------------------- */
	/* disable reset lx_dh -> boot (+ set periph address again...) */
	STB71XX_system_config27 = 0x00000344;
	
	/* Initialize the EMBX */
	/* =================== */
	
	/* Set size of the shared memory */
	/* ----------------------------- */
#if 0 /*yxl 2007-04-16 modify below section*/ 
	MBX_Size = 2*1024*1024;
#else
	MBX_Size = 3*1024*1024;
#endif /*yxl 2007-04-16 modify below section*/ 
	
#if 1
	/* Allocate shared memory between ST40 and Lx */
	/* ------------------------------------------ */
	memset(&AVMEM_AllocBlockParams,0,sizeof(AVMEM_AllocBlockParams));
	AVMEM_AllocBlockParams.PartitionHandle          = AVMEMHandle[0];
#ifdef ST_7100
	AVMEM_AllocBlockParams.Alignment                = 128;
#endif
#ifdef ST_7109 /*yxl 2007-06-08 add below section*/ 
	AVMEM_AllocBlockParams.Alignment                = 8192;
#endif

	AVMEM_AllocBlockParams.Size                     = MBX_Size;
	AVMEM_AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
	AVMEM_AllocBlockParams.NumberOfForbiddenRanges  = 0;
	AVMEM_AllocBlockParams.ForbiddenRangeArray_p    = NULL;
	AVMEM_AllocBlockParams.NumberOfForbiddenBorders = 0;
	AVMEM_AllocBlockParams.ForbiddenBorderArray_p   = NULL;
	ErrCode = STAVMEM_AllocBlock(&AVMEM_AllocBlockParams,&MBX_AVMEM_Handle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_AllocBlock()=%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}

#endif 
	ErrCode=STAVMEM_GetBlockAddress(MBX_AVMEM_Handle,&AVMEM_VirtualAddress);
	if (ErrCode!=ST_NO_ERROR)
	{
   return(ErrCode);
	}
	ErrCode=STAVMEM_GetSharedMemoryVirtualMapping(&AVMEM_VirtualMapping);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAVMEM_GetSharedMemoryVirtualMapping()=%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}
	AVMEM_DeviceAddress = STAVMEM_VirtualToDevice(AVMEM_VirtualAddress,&AVMEM_VirtualMapping);
	MBX_Address         = (U32)STAVMEM_DeviceToCPU(AVMEM_DeviceAddress,&AVMEM_VirtualMapping);
	
	/* Start the mailbox */
	/* ----------------- */
	{
		EMBXSHM_MailboxConfig_t EMBX_Config = { "ups",                                            /* Name of the transport                       */
			0,                                                /* CpuID --> 0 means Master                    */
		{ 1, 1, 1, 0 },                                   /* Participants --> ST40 + LX video + LX audio */
		0x60000000,                                       /* PointerWarp --> LMI_VID is seen             */
		0,                                                /* MaxPorts                                    */
		16,                                               /* MaxObjects                                  */
		16,                                               /* FreeListSize                                */
		(EMBX_VOID *)MBX_Address,                         /* CPU Shared address                          */
		(EMBX_UINT)MBX_Size,                              /* Size of shared memory                       */
		(EMBX_VOID *)RAM1_BASE_ADDRESS,                   /* WarpRangeAddr                               */
		(EMBX_UINT)RAM1_SIZE                              /* WarpRangeSize                               */
		};
		/* Initialize the Mailbox */
		EmbxCode=EMBX_Mailbox_Init();
		if (EmbxCode!=EMBX_SUCCESS)
		{

			STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
			return TRUE;		

		}
		/* Register the Video Lx */
		EmbxCode=EMBX_Mailbox_Register((void *)ST71XX_MAILBOX_0_BASE_ADDRESS,OS21_INTERRUPT_MB_LX_DPHI,-1,EMBX_MAILBOX_FLAGS_SET2);
		if (EmbxCode!=EMBX_SUCCESS)
		{
			STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
			return TRUE;			

		}
		/* Register the Audio Lx */
		EmbxCode=EMBX_Mailbox_Register((void *)ST71XX_MAILBOX_1_BASE_ADDRESS,OS21_INTERRUPT_MB_LX_AUDIO,0,EMBX_MAILBOX_FLAGS_SET2);
		if (EmbxCode!=EMBX_SUCCESS)
		{ 
		
			STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
			return TRUE;

		}
		/* Setup the transport communication */
		EmbxCode=EMBX_RegisterTransport(EMBXSHM_mailbox_factory,&EMBX_Config,sizeof(EMBX_Config),&MBX_Factory);
		if (EmbxCode!=EMBX_SUCCESS)
		{
			STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
			return TRUE;

		}
		/* Initialize the complete mailbox */
		EmbxCode=EMBX_Init();
		if (EmbxCode!=EMBX_SUCCESS)
		{
			STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
			return TRUE;			

		}
	}
	
	/* Start the multicom */
	/* ================== */
	MmeCode=MME_Init();
	if (MmeCode!=MME_SUCCESS)
	{
		STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
		return TRUE;

	}
	MmeCode=MME_RegisterTransport("ups");
	if (MmeCode!=MME_SUCCESS)
	{
		STTBX_Print(("EMBX_Mailbox_Init()=ST_ERROR_NO_MEMORY\n"));
		return TRUE;
	
	}
	
	/* Return no errors */
	/* ================ */
	return FALSE;
}


/*end yxl 2007-01-22 add below sections*/






#ifdef ST_7100
BOOL AUDEO_Setup(void) /*71XX,7100ok*/
{
	ST_ErrorCode_t ErrCode;

    STInitParams_t InitParams;
    STAUD_OpenParams_t OpenParams;
    STAUD_SpeakerEnable_t Speaker;
	STAUD_Object_t AudioDecoderObject;
	STAUD_Object_t AudioInputObject;
	STAUD_Object_t AudioOutputObject;
	STAUD_Object_t AudioInputSource;
	STAUD_InputParams_t AudioInputParams;
	STAUD_BroadcastProfile_t BroadcastProfile;

    memset((void*)&InitParams, 0, sizeof(STInitParams_t));
    memset((void*)&OpenParams, 0, sizeof(STAUD_OpenParams_t));
	memset(&BroadcastProfile,0,sizeof(STAUD_BroadcastProfile_t));

#ifdef ST_7100
    InitParams.DeviceType= STAUD_DEVICE_STI7100;
#endif
#ifdef ST_7109
    InitParams.DeviceType= STAUD_DEVICE_STI7109;
#endif

    InitParams.Configuration=STAUD_CONFIG_DVB_COMPACT;/*STAUD_CONFIG_DVB_FULL;/*STAUD_CONFIG_DVB_COMPACT*/
    InitParams.InterfaceType=STAUD_INTERFACE_EMI;
    InitParams.InterfaceParams.EMIParams.BaseAddress_p=NULL;
    InitParams.InterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;
    
	InitParams.CD1InterruptNumber =0;
	InitParams.CD1InterruptLevel  = 0;
    InitParams.CD1BufferAddress_p=NULL;
    InitParams.CD1BufferSize=0;
  
	InitParams.CD2BufferAddress_p = NULL;
	InitParams.CD2BufferSize  = 0;
	InitParams.CD2InterfaceType  = STAUD_INTERFACE_EMI;
	InitParams.CD2InterruptNumber = 0; 

    InitParams.CDInterfaceType=STAUD_INTERFACE_EMI;
    InitParams.CDInterfaceParams.EMIParams.BaseAddress_p=(void *)AUDIO_IF_BASE_ADDRESS;
    InitParams.CDInterfaceParams.EMIParams.RegisterWordWidth = STAUD_WORD_WIDTH_32;/**/

    InitParams.InterruptNumber=0;
    InitParams.InterruptLevel=0;
    InitParams.MaxOpen=1;

    InitParams.InternalPLL=TRUE;
	InitParams.DACClockToFsRatio=256;
	
	InitParams.PCMInterruptNumber =0;
    InitParams.PCMInterruptLevel  =0;
    InitParams.PCMOutParams.InvertWordClock=FALSE;
    InitParams.PCMOutParams.Format=STAUD_DAC_DATA_FORMAT_I2S;
    InitParams.PCMOutParams.InvertBitClock=FALSE;
    InitParams.PCMOutParams.Precision=STAUD_DAC_DATA_PRECISION_24BITS;
#if 0 /*yxl 2007-01-17*/
    InitParams.PCMOutParams.Alignment=STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
#else
    InitParams.PCMOutParams.Alignment=STAUD_DAC_DATA_ALIGNMENT_LEFT;
#endif
    InitParams.PCMOutParams.MSBFirst=TRUE;
	InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier     = 256;
	InitParams.PCM1QueueSize = 5; 
	InitParams.PCMMode = PCM_ON;

	InitParams.SPDIFOutParams.AutoLatency = TRUE;
	InitParams.SPDIFOutParams.Latency     = 0;
	InitParams.SPDIFOutParams.AutoCategoryCode = TRUE;
	InitParams.SPDIFOutParams.CategoryCode    = 0;
	InitParams.SPDIFOutParams.AutoDTDI = TRUE;
	InitParams.SPDIFOutParams.CopyPermitted = STAUD_COPYRIGHT_MODE_NO_COPY;
    InitParams.SPDIFMode=STAUD_DIGITAL_MODE_NONCOMPRESSED;
	InitParams.SPDIFOutParams.DTDI   = 0;
	InitParams.SPDIFOutParams.Emphasis = STAUD_SPDIF_EMPHASIS_CD_TYPE;
	InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier = 128;
	InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode      = STAUD_SPDIF_DATA_PRECISION_24BITS;
 


   /* InitParams.ClockGeneratorBaseAddress_p=(void*)CKG_BASE_ADDRESS;yxl 2005-11-29 cancel this line*/
    InitParams.CPUPartition_p=SystemPartition;
    InitParams.AVMEMPartition=AVMEMHandle[0] ;

    strcpy(InitParams.EvtHandlerName,EVTDeviceName);
    strcpy(InitParams.ClockRecoveryName,CLKRVDeviceName);

#ifdef ST_7100 /*yxl 2007-05-25 add this line*/
	InitParams.DriverIndex    = 1;

#if 1 /*yxl 2007-01-17 cancel below section for 71XXnewdriver*/
	InitParams.UseInternalDac = TRUE;
	/* 0 --> Audio HDMI is coming from i2s_player connected to PCM0 */
	*(DU32 *)(ST71XX_AUDIO_IF_BASE_ADDRESS+0x204) = 0x0;
	/* Configure the I2S player */ 
	*(DU32 *)(ST71XX_SPDIF_PLAYER_BASE_ADDRESS+0x800/*AUD_SPDIF_PR_CFG*/)        = 0x25;
	*(DU32 *)(ST71XX_SPDIF_PLAYER_BASE_ADDRESS+0xA00/*AUD_SPDIF_PR_SPDIF_CTRL*/) = 0x4033;

#endif 

#endif /*yxl 2007-05-25 add this line*/

/*yxl 2007-05-25 add below section*/
#ifdef ST_7109 

	InitParams.DriverIndex    = 0;
	InitParams.UseInternalDac = FALSE;
	/* 1 --> Audio HDMI is coming from spdif_player */
	*(DU32 *)(ST71XX_AUDIO_IF_BASE_ADDRESS+0x204) = 0x1;

#endif /*end yxl 2007-05-25 add below section*/


	
    ErrCode=STAUD_Init(AUDDeviceName, &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAUD_Setup()=%s\n", GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STAUD_Setup()=%s\n", GetErrorText(ErrCode)));	
#endif

    OpenParams.SyncDelay = 0;

    ErrCode=STAUD_Open(AUDDeviceName, &OpenParams, &AUDHandle);
    STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}
 #ifdef YXL_INIT_DEBUG
	STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));	
#endif


		/* For internal dacs, we need to switch PCM0 to 128FS */
	/* -------------------------------------------------- */
	/* ========== Internal dacs ========== */
	/* PCM0-->HDMI | PCM1-->SCART | SPDIF-->SPDIF OUTPUT (compressed or non compressed) */
#if 1 /*yxl 2007-01-17 cancel below section for 71XXnewdriver*/
	{
		STAUD_OutputParams_t AUD_OutputParams;
		ErrCode=STAUD_OPGetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
		AUD_OutputParams.PCMOutParams.PcmPlayerFrequencyMultiplier=128;
		ErrCode=STAUD_OPSetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
		ErrCode=STAUD_OPEnableHDMIOutput(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0);	
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
	}
#endif /*end yxl 2007-01-17 cancel below section for 71XXnewdriver*/

	/* Set the audio profile */
	/* --------------------- */
	BroadcastProfile = STAUD_BROADCAST_DVB;
	ErrCode=STAUD_DRSetBroadcastProfile(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED0,BroadcastProfile);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAUD_DRSetBroadcastProfile()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}
#if 0 
    Speaker.Left=TRUE;
    Speaker.Right=TRUE;
    Speaker.LeftSurround=FALSE;
    Speaker.RightSurround=FALSE;
    Speaker.Center=FALSE;
    Speaker.Subwoofer=FALSE;

    ErrCode=STAUD_OPSetSpeakerEnable(AUDHandle, STAUD_OBJECT_OUTPUT_MULTIPCM1, &Speaker);
    if (ErrCode!=ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_OPSetSpeakerEnable()=%s\n", GetErrorText(ErrCode) ));
		return TRUE; 
    }    
    ErrCode=STAUD_OPSetSpeakerEnable(AUDHandle, STAUD_OBJECT_OUTPUT_SPDIF1, &Speaker);
    if (ErrCode!=ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_OPSetSpeakerEnable()=%s\n", GetErrorText(ErrCode) ));
		return TRUE; 
    }    

#endif
	return FALSE; 
	
}

#endif


#ifdef ST_7109
BOOL AUDEO_Setup(void) 
{
#if 1
	ST_ErrorCode_t ErrCode;

    STAUD_InitParams_t InitParams;
    STAUD_OpenParams_t OpenParams;
    STAUD_SpeakerConfiguration_t Speaker;
	STAUD_Object_t AudioDecoderObject;
	STAUD_Object_t AudioInputObject;
	STAUD_Object_t AudioOutputObject;
	STAUD_Object_t AudioInputSource;
	STAUD_InputParams_t AudioInputParams;
	STAUD_BroadcastProfile_t BroadcastProfile;

	 STAUD_BroadcastProfile_t      AUD_BroadcastProfile;
 STAUD_DynamicRange_t          AUD_DynamicRange;

 STAUD_OutputParams_t          AUD_OutputParams;

    memset((void*)&InitParams, 0, sizeof(STAUD_InitParams_t));
    memset((void*)&OpenParams, 0, sizeof(STAUD_OpenParams_t));
	memset(&BroadcastProfile,0,sizeof(STAUD_BroadcastProfile_t));

	memset(&Speaker,0,sizeof(STAUD_SpeakerConfiguration_t));

#if 0 /*yxl 2007-06-11 modify below line*/
    InitParams.DeviceType= STAUD_DEVICE_STI7109;
#else
    InitParams.DeviceType= STAUD_DEVICE_STI7100;
#endif/*end yxl 2007-06-11 modify below line*/
    InitParams.Configuration=STAUD_CONFIG_DVB_COMPACT;/*STAUD_CONFIG_DVB_FULL;/*STAUD_CONFIG_DVB_COMPACT*/
    InitParams.InterruptNumber=0;
    InitParams.InterruptLevel=0;
    InitParams.MaxOpen=1;
	InitParams.DACClockToFsRatio=256; 

    InitParams.PCMOutParams.InvertWordClock=FALSE;
    InitParams.PCMOutParams.Format=STAUD_DAC_DATA_FORMAT_I2S;
    InitParams.PCMOutParams.InvertBitClock=FALSE;
    InitParams.PCMOutParams.Precision=STAUD_DAC_DATA_PRECISION_24BITS;
#if 0 /*yxl 2007-01-17*/
    InitParams.PCMOutParams.Alignment=STAUD_DAC_DATA_ALIGNMENT_LEFT_SIGNED;
#else
    InitParams.PCMOutParams.Alignment=STAUD_DAC_DATA_ALIGNMENT_LEFT;
#endif
    InitParams.PCMOutParams.MSBFirst=TRUE;
	InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier     = 256;

	InitParams.PCMMode = PCM_ON;

	InitParams.SPDIFOutParams.AutoLatency = TRUE;
	InitParams.SPDIFOutParams.Latency     = 0;
	InitParams.SPDIFOutParams.AutoCategoryCode = TRUE;
	InitParams.SPDIFOutParams.CategoryCode    = 0;
	InitParams.SPDIFOutParams.AutoDTDI = TRUE;
	InitParams.SPDIFOutParams.CopyPermitted = STAUD_COPYRIGHT_MODE_NO_COPY;
       InitParams.SPDIFMode=STAUD_DIGITAL_MODE_NONCOMPRESSED;
	InitParams.SPDIFOutParams.DTDI   = 0;
	InitParams.SPDIFOutParams.Emphasis = STAUD_SPDIF_EMPHASIS_CD_TYPE;
	InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier = 128;
	InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode      = STAUD_SPDIF_DATA_PRECISION_24BITS;
 
    InitParams.CPUPartition_p=SystemPartition;
    InitParams.BufferPartition= 0;
    InitParams.AllocateFromEMBX=TRUE;
    InitParams.AVMEMPartition=/*AVMEMHandle[0] 20070808 change*/AVMEMHandle[TEST_AVM]; 

    strcpy(InitParams.EvtHandlerName,EVTDeviceName);
    strcpy(InitParams.ClockRecoveryName,CLKRVDeviceName);
#if 1
	InitParams.NumChannels = 2;
#else
	InitParams.NumChannels = 10;
#endif
#if 1
#ifdef ST_7109 

	InitParams.DriverIndex    =1;
	*(DU32 *)(ST71XX_AUDIO_IF_BASE_ADDRESS+0x204) = 0x01;

#endif 
#endif

#if 1
/*yxl 2007-06-12 add below section*/
	InitParams.PCMReaderMode.MSBFirst   	= TRUE;
	InitParams.PCMReaderMode.Alignment     	= STAUD_DAC_DATA_ALIGNMENT_LEFT;
	InitParams.PCMReaderMode.Padding	    = TRUE;
	InitParams.PCMReaderMode.FallingSCLK	= FALSE;
	InitParams.PCMReaderMode.LeftLRHigh		= FALSE;
	InitParams.PCMReaderMode.Precision		= STAUD_DAC_DATA_PRECISION_24BITS;
	InitParams.PCMReaderMode.BitsPerSubFrame = STAUD_DAC_NBITS_SUBFRAME_32;
	InitParams.PCMReaderMode.MemFormat		= STAUD_PCMR_BITS_16_0;
	InitParams.PCMReaderMode.Rounding		= STAUD_NO_ROUNDING_PCMR;
	InitParams.PCMReaderMode.Frequency		= 48000; 
	InitParams.NumChPCMReader				= 1;
#endif
/*end yxl 2007-06-12 add below section*/
	
    ErrCode=STAUD_Init(AUDDeviceName, &InitParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAUD_Setup()=%s\n", GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STAUD_Setup()=%s\n", GetErrorText(ErrCode)));	
#endif

    OpenParams.SyncDelay = 0;

    ErrCode=STAUD_Open(AUDDeviceName, &OpenParams, &AUDHandle);
    STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}
 #ifdef YXL_INIT_DEBUG
	STTBX_Print(("STAUD_Open()=%s\n",GetErrorText(ErrCode)));	
#endif

     /*20070807 add*/
     CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED();

#if 1/*20070808 add*/
    /**************/
ErrCode=STAUD_OPGetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
  if (ErrCode!=ST_NO_ERROR)
   {
    return(ErrCode);
   }
  AUD_OutputParams.PCMOutParams.PcmPlayerFrequencyMultiplier=128;

 // AUD_OutputParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier=256;
  ErrCode=STAUD_OPSetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
  if (ErrCode!=ST_NO_ERROR)
  {
    return(ErrCode);
  }
  ErrCode=STAUD_OPEnableHDMIOutput(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0);
  if (ErrCode!=ST_NO_ERROR)
  {
    return(ErrCode);
  }


  #endif

#if 0 /*yxl 2007-01-17 cancel below section for 71XXnewdriver*/
	{
		STAUD_OutputParams_t AUD_OutputParams;
		ErrCode=STAUD_OPGetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
		AUD_OutputParams.PCMOutParams.PcmPlayerFrequencyMultiplier=128;
		ErrCode=STAUD_OPSetParams(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0,&AUD_OutputParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
		ErrCode=STAUD_OPEnableHDMIOutput(AUDHandle,STAUD_OBJECT_OUTPUT_PCMP0);	
		if (ErrCode!=ST_NO_ERROR)
		{
			return(ErrCode);
		}
	}
#endif /*end yxl 2007-01-17 cancel below section for 71XXnewdriver*/
 
#if 0	 /*yxl 2007-06-08 temp cancelbelow section*/
	/* Set the audio profile */
	/* --------------------- */
	BroadcastProfile = STAUD_BROADCAST_DVB;
	ErrCode=STAUD_IPSetBroadcastProfile(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED0,BroadcastProfile);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAUD_DRSetBroadcastProfile()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}
#endif 


#if 0

    ErrCode=STAUD_SetSpeakerConfiguration(AUDHandle,  Speaker,BASS_MGT_OFF);
	
    if (ErrCode!=ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_OPSetSpeakerEnable()=%s\n", GetErrorText(ErrCode) ));
		return TRUE; 
    }    
	#if 0
    ErrCode=STAUD_OPSetSpeakerEnable(AUDHandle, STAUD_OBJECT_OUTPUT_SPDIF1, &Speaker);
    if (ErrCode!=ST_NO_ERROR)
    {
        STTBX_Print(("STAUD_OPSetSpeakerEnable()=%s\n", GetErrorText(ErrCode) ));
		return TRUE; 
    }    
#endif
#endif

			{
#if 0 /*yxl 2007-06-12 add below section */

				STAUD_Stereo_t StereoMode;
				StereoMode=STAUD_STEREO_DUAL_MONO;/*STAUD_STEREO_DUAL_MONO;ok*/
				ErrCode = STAUD_SetStereoOutput(AUDHandle,StereoMode/*STAUD_STEREO_MODE_DUAL_RIGHT */);
				if(ErrCode!=ST_NO_ERROR)
				{
					STTBX_Print(("\nYxlInfo:STAUD_SetStereoOutput=%s ",
						GetErrorText(ErrCode)));
				}
#endif /*yxl 2007-06-12 add below section */
			}

	return FALSE; 
	#else
	#include "aud_cmd.h"
	AUD_Init(AUD_INT);
         AUD_Init(AUD_EXT);
	return FALSE; 
	#endif
}

#endif



/* 
   Name:        BLAST_RX_Callback
   Purpose:     Event handler for IR Blaster receiver events.
   Arguments:   <STEVT_CallReason_t Reason> 		The reason because the
		                                        callback has been invoked
   		<const ST_DeviceName_t RegistrantName>	The registrant name
                <STEVT_EventConstant_t Event>		The event to check and 
		                                        manage
                <const void *EventData>			The event data
                <const void *SubscriberData_p>		The data to indentify
		                                        a specific subscriber 
     
*/

void BLAST_RX_Callback(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    ST_ErrorCode_t ErrCode;
	switch (Event)
    {
        case STBLAST_READ_DONE_EVT:
        case STBLAST_WRITE_DONE_EVT:
			
            ErrCode = ((STBLAST_EvtReadDoneParams_t *)EventData)->Result;
	
		
			/*if(ErrCode==ST_NO_ERROR) */
				semaphore_signal(gpSemBLAST);
            break;
        default:
            break;
    }
}

BOOL BLAST_Setup(void) /*71XX*/
{
	ST_ErrorCode_t ErrCode;
	STBLAST_InitParams_t InitParams;
	STBLAST_OpenParams_t OpenParams;
	ST_DeviceName_t BLASTRXDeviceName="IRB_RECEIVE";
    STEVT_DeviceSubscribeParams_t EvtSubParams;

	memset(&InitParams,0,sizeof(STBLAST_InitParams_t));
	memset(&OpenParams,0,sizeof(STBLAST_OpenParams_t));
	InitParams.DeviceType=STBLAST_DEVICE_IR_RECEIVER;
    InitParams.DriverPartition=SystemPartition;
    InitParams.BaseAddress=(U32 *)ST71XX_IRB_BASE_ADDRESS;
    InitParams.InterruptNumber=ST71XX_IRB_INTERRUPT;/*IRB_INTERRUPT;*/
    InitParams.InterruptLevel=IRB_INTERRUPT_LEVEL;
    strcpy(InitParams.EVTDeviceName,EVTDeviceName);


#if 0 /*def ST_7100*/
    InitParams.ClockSpeed=gClockInfo.ckga.com_clk;
#else
    InitParams.ClockSpeed=gClockInfo.CommsBlock;
#endif	
    InitParams.RxPin.BitMask=PIO_BIT_3;/*BLAST_RXD_BIT;*/
    InitParams.TxPin.BitMask    = PIO_BIT_5;
    InitParams.TxInvPin.BitMask = PIO_BIT_6;
    strcpy(InitParams.RxPin.PortName,PIO_DeviceName[3]);    /*BLAST_RXD_PIO]*/
    strcpy(InitParams.TxPin.PortName   ,PIO_DeviceName[3]);
    strcpy(InitParams.TxInvPin.PortName,PIO_DeviceName[3]);

	InitParams.InputActiveLow=false;

#ifdef USE_NEC_REMOTE
      InitParams.InputActiveLow=FALSE;/*FALSE;*/
#endif

#ifdef USE_RC6_REMOTE
      InitParams.InputActiveLow=TRUE;/*FALSE;*/
#endif


    InitParams.SymbolBufferSize=200;

	ErrCode = STBLAST_Init(BLASTRXDeviceName,&InitParams);

	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STBLAST_Init()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STBLAST_Init()=%s\n",GetErrorText(ErrCode)));	
	
	STTBX_Print (("\nyxlInfo: InitParams.DeviceType = %d InitParams.DriverPartition = %lx InitParams.ClockSpeed =%d ",
		InitParams.DeviceType,InitParams.DriverPartition,InitParams.ClockSpeed));
	
	STTBX_Print (("\nyxlInfo: InitParams.SymbolBufferSize =%d InitParams.BaseAddress = %lx InitParams.InterruptNumber =%d",
		InitParams.SymbolBufferSize,InitParams.BaseAddress,InitParams.InterruptNumber));
	
	STTBX_Print (("\nyxlInfo: InitParams.InterruptLevel = %d InitParams.EVTDeviceName=%s InitParams.RxPin.PortName=%s",
		InitParams.InterruptLevel,InitParams.EVTDeviceName,InitParams.RxPin.PortName));

	STTBX_Print (("\nyxlInfo: InitParams.RxPin.BitMask=%d InitParams.InputActiveLow=%d",
		InitParams.RxPin.BitMask,InitParams.InputActiveLow));
#endif

	OpenParams.RxParams.GlitchWidth=10;/*10;*/
	ErrCode = STBLAST_Open(BLASTRXDeviceName,&OpenParams,&BLASTRXHandle);

	if(ErrCode!=ST_NO_ERROR) 
	{
	STTBX_Print(("STBLAST_Open()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STBLAST_Open()=%s\n",GetErrorText(ErrCode)));	
#endif

	memset(&EvtSubParams, 0, sizeof(EvtSubParams));
    EvtSubParams.NotifyCallback=BLAST_RX_Callback;

#if 1
   ErrCode=STEVT_SubscribeDeviceEvent(EVTHandle,BLASTRXDeviceName/*BLASTRXDeviceName*//*NULL*/,
	   STBLAST_READ_DONE_EVT, &EvtSubParams);

	if(ErrCode!=ST_NO_ERROR)
    {
		STTBX_Print((" STEVT_SubscribeDeviceEvent(RX)=%s ",GetErrorText(ErrCode)));
        return	TRUE;
    }
#endif
#ifndef ST_OS21
    gpSemBLAST=semaphore_init_fifo_timeout(0);
#else
    gpSemBLAST=semaphore_create_fifo(0);
#endif
	return FALSE;
	
}



#ifdef ST_7109
ST_ErrorCode_t BLIT_Setup(void)/*71XX*/
{
	ST_ErrorCode_t ErrCode;
	STBLIT_InitParams_t  InitParams;
	STBLIT_AllocParams_t AllocParams;
	STBLIT_OpenParams_t OpenParams;
	STEVT_DeviceSubscribeParams_t EvtSubParams;
	memset((void *)&InitParams, 0, sizeof(STBLIT_InitParams_t));
	memset((void *)&AllocParams, 0, sizeof(STBLIT_AllocParams_t));
	InitParams.DeviceType                 = BLIT_DEVICE_TYPE_71XX;
	InitParams.BaseAddress_p              = (void *)ST71XX_BLITTER_BASE_ADDRESS;

	InitParams.CPUPartition_p             = SystemPartition;
    InitParams.AVMEMPartition             = AVMEMHandle[0] ;
    InitParams.SharedMemoryBaseAddress_p  = (void *)RAM1_BASE_ADDRESS;
    strcpy(InitParams.EventHandlerName, EVTDeviceName);
    InitParams.BlitInterruptEvent         = STBLIT_BLIT_COMPLETED_EVT;
	InitParams.BlitInterruptLevel   =BLIT_INTERRUPT_LEVEL;///* 1;*//*need to update to 1 for CUT3.1 no micversion pin,yxl 2005-12-20 recorde*/
    InitParams.BlitInterruptNumber  = ST71XX_BLITTER_INTERRUPT;/*ST7710_BLT_REG_INTERRUPT;*/
    InitParams.MaxHandles                 = 10;/*1,2,org is 1*/

    InitParams.SingleBlitNodeBufferUserAllocated =FALSE;
    InitParams.SingleBlitNodeMaxNumber    = 1000;/*400; 200   */
		       
    InitParams.SingleBlitNodeBuffer_p     = NULL;


    InitParams.JobBlitNodeBufferUserAllocated   = FALSE;
    InitParams.JobBlitNodeMaxNumber       =30;/* 400;*/
    InitParams.JobBlitNodeBuffer_p        = NULL;
    
    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber           = 30;/* 400;*/
    InitParams.JobBlitBuffer_p            = NULL;
    
    InitParams.JobBufferUserAllocated     = FALSE;
    InitParams.JobMaxNumber               = 2;/* 400;*/
    InitParams.JobBuffer_p                = NULL;
    
    InitParams.WorkBufferUserAllocated    = FALSE;
    InitParams.WorkBufferSize             = 40 * 16;
    InitParams.WorkBuffer_p               = NULL;

    InitParams.BigNotLittle               = FALSE;


	ErrCode = STBLIT_Init(BLITDeviceName, &InitParams);
    if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STBLIT_Init()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
		STTBX_Print(("STBLIT_Init()=%s %s\n",GetErrorText(ErrCode),STBLIT_GetRevision()));	
#endif

#ifndef YXL_USE_GFX	

    ErrCode = STBLIT_Open(BLITDeviceName, &OpenParams, &BLITHndl);
    if( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("BLIT Open: %s\n", GetErrorText(ErrCode)));
		return TRUE;
    }
#endif
         
	return FALSE;
} 
#endif


ST_DeviceName_t GFXDeviceName="GFX";

/*-----------------------------------------------------------------------------
 * Function : GFX_Setup
 *
 * Input    : 
 * Output   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t GFX_Setup(void)
{
	ST_ErrorCode_t ErrCode;
	STGFX_InitParams_t InitParams;
	STGXOBJ_Color_t DrawColor;
	STGXOBJ_Color_t FillColor;


#ifdef OSD_COLOR_TYPE_RGB16 

#if 1
    DrawColor.Type=FillColor.Type=STGXOBJ_COLOR_TYPE_ARGB1555;

#else
    DrawColor.Type=FillColor.Type=STGXOBJ_COLOR_TYPE_ARGB4444;
#endif
	DrawColor.Value.ARGB1555.Alpha  = 1;
	DrawColor.Value.ARGB1555.R      = 0;
	DrawColor.Value.ARGB1555.G      = 0;
	DrawColor.Value.ARGB1555.B      = 31;
  
	FillColor.Value.ARGB1555.Alpha  = 1;
	FillColor.Value.ARGB1555.R      = 31/3;
	FillColor.Value.ARGB1555.G      = 2*31/3;
	FillColor.Value.ARGB1555.B      = 0;
#else


    DrawColor.Type=FillColor.Type=STGXOBJ_COLOR_TYPE_ARGB8888;


#endif 

	InitParams.CPUPartition_p       		= SystemPartition;

	InitParams.AVMemPartitionHandle 	= AVMEMHandle[0];
	InitParams.NumBitsAccuracy 		= 0;

	strcpy(InitParams.BlitName, BLITDeviceName);
	strcpy(InitParams.EventHandlerName, EVTDeviceName);
	  
	ErrCode = STGFX_Init(GFXDeviceName, &InitParams);

	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_Init()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#ifdef YXL_INIT_DEBUG
	STTBX_Print(("STGFX_Init()=%s\n",GetErrorText(ErrCode)));	
#endif 
	ErrCode = STGFX_Open(GFXDeviceName, &GFXHandle);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_Open()=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

	ErrCode = STGFX_SetGCDefaultValues(DrawColor, FillColor, &gGC);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STGFX_SetGCDefaultValues()=%s\n", GetErrorText(ErrCode)));
		return TRUE;
		
	}

	return FALSE;
}

BOOL GlobalInit(void)
{
	
	BOOL res;
	message_queue_t  *pstMessageQueue = NULL;


#if 1 /*def ST_7100 yxl 2007-10-13 modify*/
	CH_71XX_ClockConfig();
#endif

	res=SECTIONS_Setup();


	
      /*20070808 add*/
       CH_Appl2Mem_init();

	CH_InitSema();

	//Appl3Mem_init();
     /***************/
#ifdef YXL_INIT_DEBUG
	printf("memory partition successful  yxl \n");
#endif
  
	res=BOOT_Init();
	if(res==TRUE) return res;

	ST_GetClockInfo(&gClockInfo);

#if 1/*yxl 2007-01-9 add below section*/
 {
  #define ILC_EXTERNAL_IRQ64           64
  #define ILC_EXTERNAL_IRQ65           65
  #define ILC_EXTERNAL_IRQ66           66
  #define ILC_EXTERNAL_IRQ67           67
  #define ILC_BASE_ADDR                0xB8000000
  #define _BIT(_int)                   (1<<(_int%32))
  #define _REG_OFF(_int)               (sizeof(int)*(_int/32))
  #define ILC_PRIORITY_REG(_int)       (ILC_BASE_ADDR+0x800+(8*_int))
  #define ILC_TRIGMODE_REG(_int)       (ILC_BASE_ADDR+0x804+(8*_int))
  #define ILC_SET_ENABLE_REG(_int)     (ILC_BASE_ADDR+0x500+_REG_OFF(_int))
  #define ILC_EXT_WAKPOL_EN_REG        (ILC_BASE_ADDR+0x680)
  #define ILC_SET_PRI(_int,_pri)      *(DU32 *)ILC_PRIORITY_REG(_int)   = (_pri)
  #define ILC_SET_TRIGMODE(_int,_mod) *(DU32 *)ILC_TRIGMODE_REG(_int)   = (_mod)
  #define ILC_SET_ENABLE(_int)        *(DU32 *)ILC_SET_ENABLE_REG(_int) = _BIT(_int)

#if defined(HMP71XX) && defined (IPSTB)
	/*ILY to disable active interrupt*/
  LAN9118_DATA Lan9118Data;
  Lan9118Data.dwLanBase = 0xa2000300;
  Lan9118_DisableIRQ(&Lan9118Data);
#endif /* defined(HMP71XX) && defined (IPSTB) */
  
  /* Set priorities */
  ILC_SET_PRI(ILC_EXTERNAL_IRQ64,0x8004);
  ILC_SET_PRI(ILC_EXTERNAL_IRQ65,0x8005);
  ILC_SET_PRI(ILC_EXTERNAL_IRQ66,0x8006);
  ILC_SET_PRI(ILC_EXTERNAL_IRQ67,0x8007);
  /* Now instruct the ILC to trigger interrupt on high level */
#ifdef ST_DEMO_PLATFORM
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ64,0x01);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ65,0x01);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ66,0x01);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ67,0x01);
#else
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ64,0x00);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ65,0x00);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ66,0x00);
  ILC_SET_TRIGMODE(ILC_EXTERNAL_IRQ67,0x00);
#endif
  *(DU32 *)ILC_EXT_WAKPOL_EN_REG = 0x0F;
  /* Now enable external interrupts */
  ILC_SET_ENABLE(ILC_EXTERNAL_IRQ64);
  ILC_SET_ENABLE(ILC_EXTERNAL_IRQ65);
  ILC_SET_ENABLE(ILC_EXTERNAL_IRQ66);
  ILC_SET_ENABLE(ILC_EXTERNAL_IRQ67);
 }
#endif


	res=PIO_Init();
	if(res==TRUE) return res;
	res=UART_Init();
	if(res==TRUE) return res;
	res=TBX_Init();
	if(res==TRUE) return res;

        /*20080605 add disable backgoud check*/
       CH_BootDisableBackgroundCheck();


#if 1 /*yxl 2007-01-22 move this to top*/
	res=AVMEM_Setup();
	if(res==TRUE) return res;
#endif 


	res=EVT_Setup();
	if(res==TRUE) return res;

	res=CLKRV_Setup();
	if(res==TRUE) return res;



	res=I2C_Init( );
	if(res==TRUE) return res;

#if 1 /*yxl 2007-01-22 add this section*/
	res=MBX_Setup();
	if(res==TRUE) return res;

#endif 

	res=FDMA_Init();
	if(res==TRUE) return res;

	res=E2P_Init();
	if(res==TRUE) return res;


	task_delay(5000);

	res=FLASH_Setup();
	if(res==TRUE) return res;


/*20070521 move LED init here,because Tuner will use the LED LOCK*/
      CH_LEDInit(); 

#ifdef ST_DEMO_PLATFORM
	res=CH_TunerSetup_ST();
#else
#if 0
#ifndef CH_QAM0297E
	res=ch_DVBTunerInit (TUNER_TYPE_PHILIPS_CD1616);/*使用周旭成的新DRIVER*/
#else
     res= CH_TUNER_Setup();
#endif
#else
	 res= CH_TUNER_Setup();
        do_report(0,"\nthe return of tuner init=====%d",res);
#endif
#endif
	if(res==TRUE) return res;

	if(CH_MessageInit())
	{
		STTBX_Print(("unable to init ch application message queue!\n"));
		return TRUE;	
	}
	
#ifndef ST_DEMO_PLATFORM  /*ndef INIT_DEBUG*/
	if(CH_DBASEMGRINIT())
	{
		STTBX_Print(("Nvm or Flash Mgr Init Error!\n"));
		/*return TRUE;*/
	}
#endif

	res=TSMERGE_Setup();
	if(res==TRUE) return res;

	res=STPTI_Setup();
	if(res==TRUE) return res;

/*	STTBX_Print(("\n YxlInfo:--------------call TSConfig()"));*/
	res=TSConfig();
	if(res==TRUE) return res;

	res=DENC_Setup();
	if(res==TRUE) return res;

	res=VTG_Setup();
	if(res==TRUE) return res;


{
	STVTG_TimingMode_t TimingMode;
	STVTG_OptionalConfiguration_t VTGOptionalConfiguration;
	ST_ErrorCode_t ErrCode;
       pstBoxInfo->HDVideoTimingMode = MODE_720P50;/*初始强制设置为无效*/
	TimingMode=CH_GetSTTimingModeFromCH(pstBoxInfo->HDVideoTimingMode);

	ErrCode=STVTG_SetMode(VTGHandle[VTG_MAIN],TimingMode);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_SetMode(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

	VTGOptionalConfiguration.Option                = STVTG_OPTION_EMBEDDED_SYNCHRO;
	VTGOptionalConfiguration.Value.EmbeddedSynchro = TRUE;  
	ErrCode=STVTG_SetOptionalConfiguration(VTGHandle[VTG_MAIN],&VTGOptionalConfiguration);
	if (ErrCode!=ST_NO_ERROR)
	{
		return TRUE;
	}

	ErrCode=STVTG_GetMode(VTGHandle[VTG_MAIN],&TimingMode,&gVTGModeParam);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_GetMode(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

	TimingMode=STVTG_TIMING_MODE_576I50000_13500;/*PAL*/
	ErrCode=STVTG_SetMode(VTGHandle[VTG_AUX],TimingMode);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_SetMode(VTG_AUX)=%s\n",GetErrorText(ErrCode)));

		return TRUE;
	}
	ErrCode=STVTG_GetMode(VTGHandle[VTG_AUX],&TimingMode,&gVTGAUXModeParam);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_GetMode(VTG_AUX)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

}

	res=VOUT_Setup(VOUT_MAIN,pstBoxInfo->HDVideoOutputType); 
	if(res==TRUE) return res;

	res=VOUT_Setup(VOUT_HDMI,pstBoxInfo->HDDigitalVideoOutputType); 
	if(res==TRUE) return res;

	res=VOUT_Setup(VOUT_AUX,pstBoxInfo->SDVideoOutputType);
	if(res==TRUE) return res;

#ifdef USE_STILL
     	res=LAYER_Setup(LAYER_GFX3,ASPECTRATIO_16TO9);
#endif

	res=LAYER_Setup(LAYER_VIDEO1,ASPECTRATIO_16TO9);
	if(res==TRUE) return res;
	res=LAYER_Setup(LAYER_GFX1,ASPECTRATIO_16TO9);
	if(res==TRUE) return res;
#ifdef USE_STILL
	res=LAYER_Setup(LAYER_GFX4,ASPECTRATIO_4TO3);
	if(res==TRUE) return res;
#endif
	res=LAYER_Setup(LAYER_GFX2,ASPECTRATIO_4TO3);
	if(res==TRUE) return res;

	res=LAYER_Setup(LAYER_VIDEO2,ASPECTRATIO_4TO3);
	if(res==TRUE) return res;

	res=MIX_Setup(VMIX_MAIN,ASPECTRATIO_16TO9);
	if(res==TRUE) return res;

	res=MIX_Setup(VMIX_AUX,ASPECTRATIO_4TO3);
	if(res==TRUE) return res;

	res=VID_Setup(VID_MPEG2);
	if(res==TRUE) return res;

	res=VID_Setup(VID_H);
	if(res==TRUE) return res;

	res=AUDEO_Setup();
	if(res==TRUE) return res;

#if 1

	res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
	if(res==TRUE) return res;
	res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
	if(res==TRUE) return res;
#else

	res=VID_OpenViewport(VID_H,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
	if(res==TRUE) return res;
	res=VID_OpenViewport(VID_H,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
	if(res==TRUE) return res;

#endif 
#if 1
	res=CHHDMI_Setup(); 
	if(res==TRUE) return res;
#else	
	HDMI_Setup();
#endif
	res=BLAST_Setup();
	if(res==TRUE) return res;
	
	res=BLIT_Setup();
	if(res==TRUE) return res;
	
#ifdef YXL_USE_GFX
	res=GFX_Setup();
	if(res==TRUE) return res;
#endif
#if 0

       /*20070709 增加从FLASH 中解压缩到内存0xB0000000*/
       CH_UnCompressPicFont((U32 *)FONTPIC_FLASHZIP_ADDR,(U32 *)gpu8_DecompressBuffer);

	{
		CH_GraphicInitParams GDPParam;

		GDPParam.FontStartAdr=/*FLASH_FONT_ADDR*/(unsigned int)(&(*gpu8_DecompressBuffer));
		GDPParam.PicStartAdr=/*FLASH_PIC_ADDRESS*/((unsigned int)(&(*gpu8_DecompressBuffer))+/*562648*/548208);
		/*	GDPParam.PicNumber=USED_PIC_FILE_NUM;*/
		GDPParam.PicMaxNameLen=MAX_FLASHFILE_NAME;
		GDPParam.pPartition=CHSysPartition;
		
		res=CH_InitGraphicBase(&GDPParam);
		if(res==TRUE)
		{
			STTBX_Print(("\nCH_InitGraphicBase() is not successful,quit"));
			/*return TRUE;zxg 20070424 del for test*/
		}
		res=CH_SetupViewPort(LAYER_GFX1,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
#ifdef USE_STILL
		CH6_OpenViewPortStill(BACKGROUND_PIC,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
#endif

		if(res==TRUE) return res;
	}
#else
CH_UnCompressPicFont((U32 *)FONTPIC_FLASHZIP_ADDR,(U32 *)gpu8_DecompressBuffer);
		CH_GraphicInitParams GDPParam;

		GDPParam.FontStartAdr=/*FLASH_FONT_ADDR*/(unsigned int)(&(*gpu8_DecompressBuffer));
		GDPParam.PicStartAdr=/*FLASH_PIC_ADDRESS*/((unsigned int)(&(*gpu8_DecompressBuffer))+/*562648*/548208);
		/*	GDPParam.PicNumber=USED_PIC_FILE_NUM;*/
		GDPParam.PicMaxNameLen=MAX_FLASHFILE_NAME;
		GDPParam.pPartition=CHSysPartition;
		
		res=CH_InitGraphicBase(&GDPParam);
		if(res==TRUE)
		{
			STTBX_Print(("\nCH_InitGraphicBase() is not successful,quit"));
			/*return TRUE;zxg 20070424 del for test*/
		}

#ifdef CH_MUTI_RESOLUTION_RATE
            CH_CreateOSDViewPort_HighSolution ();
#else
	      res=CH_SetupViewPort(LAYER_GFX1,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
#ifdef USE_STILL
		CH6_OpenViewPortStill(BACKGROUND_PIC,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
#endif
#endif
#endif

	res=CH_AVSlotInit();
	if(res==TRUE) return res;

#if 1 /*yxl 2007-02-13*/
	{
		ST_ErrorCode_t ErrCode;
	/*	if(pstBoxInfo->VideoOutputType==TYPE_CVBS) yxl 2006-12-14 temp cancel for debug71XX*/
	
		{
			ErrCode = STVID_EnableOutputWindow(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2]);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
			}
		}
	/*	else yxl 2006-12-14 temp cancel for debug71XX*/
		{
			ErrCode = STVID_EnableOutputWindow(VIDVPHandle[VID_MPEG2][LAYER_VIDEO1]);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
			}
#ifdef CVBS_ALWAYS_ON
			ErrCode = STVID_EnableOutputWindow(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2]);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
			}
#endif
		}
	}
#endif
#ifndef CH_MUTI_RESOLUTION_RATE
	
	if(CH6_ShowViewPort(&CH6VPOSD))
	{
		STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is not successful"));
	}
	else STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is successful"));
#endif
	return FALSE;
		
}


/************************函数说明***********************************/
/*函数名:void CH_UnCompressPicFont (U32 *FlashPicFontAddr,U32 *MemPicFontAddr)*/
/*开发人和开发时间:zengxianggen 2006-12-27                        */
/*函数功能描述:把FLASH中的压缩字库图片解压缩到指定内存                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
void CH_UnCompressPicFont (U32 *FlashPicFontAddr,U32 *MemPicFontAddr)
{
       U32   CompressSzie=1232786;
	U32   UncompressSzie=DecompressBuffer_SIZE;

 
	CompressSzie=(int)(*FlashPicFontAddr);
	/*
	 checksum1=(int)*(FlashPicFontAddr+1)
	  UncompressSzie=(int)*(FlashPicFontAddr+2);
	  checksum2=(int)*(FlashPicFontAddr+3)
	*/
	do_report(0,"start uncompress pic \n");
       
	ch_uncompress (MemPicFontAddr, &UncompressSzie, (U32 *)(FlashPicFontAddr+4), CompressSzie);	 

      	do_report(0,"uncompress pic UncompressSzie =%d done\n",UncompressSzie);	
}


/************************函数说明***********************************/
/*函数名:void CH_UnCompressCompanision (U32 *FlashPicFontAddr,U32 *MemPicFontAddr)*/
/*开发人和开发时间:zengxianggen 2007-07-09                        */
/*函数功能描述:把FLASH中的压缩Companision数据解压缩到指定内存                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
void CH_UnCompressCompanision (U32 *FlashCompanisionAddr,U32 *MemCompanisionAddr)
{
       U32   CompressSzie=710662 ;
	U32   UncompressSzie=DecompressBuffer_SIZE;

 
	CompressSzie=(int)(*FlashCompanisionAddr);
	/*
	 checksum1=(int)*(FlashPicFontAddr+1)
	  UncompressSzie=(int)*(FlashPicFontAddr+2);
	  checksum2=(int)*(FlashPicFontAddr+3)
	*/
	do_report(0,"start uncompress Companision \n");
       
	ch_uncompress (MemCompanisionAddr, &UncompressSzie, (U32 *)(FlashCompanisionAddr+4), CompressSzie);	 

      	do_report(0,"uncompress Companision  UncompressSzie =%d done\n",UncompressSzie);	
}

#if 1

/************************函数说明***********************************/
/*函数名:BOOL CH6_EnableSPDIFDigtalOutput_COMPRESSED(void)*/
        
/*开发人和开发时间:zengxianggen 2006-06-12                         */
/*函数功能描述:设置输出为SPDIF输出为COMPRESSED模式      */                           
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：   */

BOOL CH6_EnableSPDIFDigtalOutput_COMPRESSED(void)
{
	ST_ErrorCode_t ErrCode;
	STAUD_DigitalOutputConfiguration_t DigitalOutput;

	U32 TempSize;
	DigitalOutput.DigitalMode=STAUD_DIGITAL_MODE_COMPRESSED;
	DigitalOutput.Copyright=STAUD_COPYRIGHT_MODE_NO_RESTRICTION;
	DigitalOutput.Latency=0;

	ErrCode=STAUD_SetDigitalOutput(AUDHandle,DigitalOutput);
	if (ErrCode!=ST_NO_ERROR)
	{
		/*do_report(0,"STAUD_SetDigitalOutput()=%s\n", GetErrorText(ErrCode) );*/
		return TRUE;
	}    
	return FALSE;
}
/**/
/*函数名:BOOL CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED(void)     */

/*开发人和开发时间:zengxianggen 2006-06-12                         */
/*函数功能描述:设置输出为SPDIF输出为UNCOMPRESSED模式*/                                 
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：   */
BOOL CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED(void)
{

	ST_ErrorCode_t ErrCode;
	STAUD_DigitalOutputConfiguration_t DigitalOutput;
	DigitalOutput.DigitalMode=STAUD_DIGITAL_MODE_NONCOMPRESSED;
	DigitalOutput.Copyright=STAUD_COPYRIGHT_MODE_NO_RESTRICTION;
	DigitalOutput.Latency=0;
	
	ErrCode=STAUD_SetDigitalOutput(AUDHandle,DigitalOutput);
	if (ErrCode!=ST_NO_ERROR)
	{
		/*do_report(0,"STAUD_SetDigitalOutput()=%s\n", GetErrorText(ErrCode));*/
		return TRUE;
	}    
	return FALSE;
}

/*函数名:BOOL CH6_DisableSPDIFDigtalOutput(void)
        */
/*开发人和开发时间:zengxianggen 2006-06-12                         */
/*函数功能描述:关闭SPDIF输出*/                                 
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：   */
BOOL CH6_DisableSPDIFDigtalOutput(void)
{

	ST_ErrorCode_t ErrCode;
	STAUD_DigitalOutputConfiguration_t DigitalOutput;
	DigitalOutput.DigitalMode=STAUD_DIGITAL_MODE_OFF;
	DigitalOutput.Copyright=STAUD_COPYRIGHT_MODE_NO_RESTRICTION;
	DigitalOutput.Latency=0;


	ErrCode=STAUD_SetDigitalOutput(AUDHandle,DigitalOutput);
	
	if (ErrCode!=ST_NO_ERROR)
	{
		/*STTBX_Print(("STAUD_SetDigitalOutput()=%s\n", GetErrorText(ErrCode) ));*/
		return TRUE;
	}    
	return FALSE;
}
/***************/
int  gi_audFormChanged = -1;
/************************函数说明***********************************/
/*函数名:void   CH_SetSPIDFtOut(int  i_audioType)                                               */
/*开发人和开发时间:zengxianggen 2007-07-09                                      */
/*函数功能描述:设置SPDIF输出                                                        */
/*函数原理说明:                                                                                       */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                                 */
/*返回值说明：无                                                                                 */
void    CH_SetSPIDFtOut(int  i_audioType)
{


    if(i_audioType != STAUD_STREAM_CONTENT_AC3)
   {
      if(gi_audFormChanged!=STAUD_STREAM_CONTENT_PCM)
     {
       CH6_DisableSPDIFDigtalOutput();
       CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED();
      gi_audFormChanged = STAUD_STREAM_CONTENT_PCM;
     do_report(0, ">>>>>>>>AudioPcmOut<<<<<<<<<<<\n");/**/
   }
  }
  else
  {
    if(pstBoxInfo->SPDIFOUT==0)
    	{
        /* do_report(0, ">>>>>>>>AudioPcmOut<<<<<<<<<<<\n");*/
		if(gi_audFormChanged!=STAUD_STREAM_CONTENT_PCM)
			{
			CH6_DisableSPDIFDigtalOutput();
                     CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED();
	              gi_audFormChanged = STAUD_STREAM_CONTENT_PCM;
	              do_report(0, ">>>>>>>>AudioPcmOut<<<<<<<<<<<\n"); /**/
			}

    	}
	else
		{

           if(gi_audFormChanged!=STAUD_STREAM_CONTENT_AC3)
      	   {
          	CH6_DisableSPDIFDigtalOutput();
              CH6_EnableSPDIFDigtalOutput_COMPRESSED();
	       gi_audFormChanged = STAUD_STREAM_CONTENT_AC3;
	      do_report(0, ">>>>>>>>AudioDigitalOut<<<<<<<<<<<\n");/**/
      	    }

	}
    }
}

#endif

/******************************************************************/
/*函数名: void BLIT_Term(void)                                                             */                                                        
/*开发人和开发时间:zengxianggen 2007-07-24                            */
/*函数功能描述:停止BLIT                                                            */
/*函数原理说明:                                                                             */
/*输入参数：无                                                                              */
/*输出参数: 无                                                                                  */
/*使用的全局变量、表和结构：                                        */
/*调用的主要函数列表：                                                         */
/*返回值说明：无                                                                         */   
/*调用注意事项:                                                                             */
/*其他说明:                */                                                                     
void BLIT_Term(void)
{
    ST_ErrorCode_t          ErrCode;
    STBLIT_TermParams_t     TermParams;

    TermParams.ForceTerminate = true;

    /* Term blitter driver, before event, in case of ForceTerminate   */
    /* set to FALSE.                                                  */
    ErrCode = STBLIT_Term(BLITDeviceName, &TermParams);
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                  "STBLIT_Term(%s)=%s",
                  BLITDeviceName,
                  GetErrorText(ErrCode)));

    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "BLIT_Term Ok"));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "BLIT_Term Fail"));
    }
    return ;

} /* BLIT_Term () */

/******************************************************************/
/*函数名: void GFX_Term(void)                                                              */
/*开发人和开发时间:zengxianggen 2007-07-24                            */
/*函数功能描述:停止GFX                                                            */
/*函数原理说明:                                                                             */
/*输入参数：无                                                                              */
/*输出参数: 无                                                                                  */
/*使用的全局变量、表和结构：                                        */
/*调用的主要函数列表：                                                         */
/*返回值说明：无                                                                         */   
/*调用注意事项:                                                                             */
/*其他说明:                                                                                        */
void GFX_Term(void)
{
   STGFX_TermParams_t TermParams;
   STGFX_Close(GFXHandle);
   TermParams.ForceTerminate = TRUE;
   STGFX_Term(GFXDeviceName, &TermParams); 
}

/******************************************************************/
/*函数名: void CH_ResetBlitEngine(void)                                                 */
/*开发人和开发时间:zengxianggen 2007-07-24                            */
/*函数功能描述:复位BLIT操作                                                 */
/*函数原理说明:                                                                             */
/*输入参数：无                                                                              */
/*输出参数: 无                                                                                  */
/*使用的全局变量、表和结构：                                        */
/*调用的主要函数列表：                                                         */
/*返回值说明：无                                                                         */   
/*调用注意事项:                                                                             */
/*其他说明:                                                                                        */  
 void CH_ResetBlitEngine(void)
  {
	  U32 Value;
	  task_lock();
	 #if 0 /*20080730修改改善长时间BLIT出错问题*/
	  Value=STSYS_ReadRegDev32LE((void*)(0xB920B000 + 0x0a00));

	  STSYS_WriteRegDev32LE((void*)(0xB920B000 + 0x0a00), 0x00000001);
	  STSYS_WriteRegDev32LE((void*)(0xB920B000 + 0x0a00), 0);
        #endif    
	  GFX_Term();
	 #if 0 /*20080730修改改善长时间BLIT出错问题*/

	  BLIT_Term();
	  
         BLIT_Setup();
		 #endif
	  GFX_Setup( );
		   
         task_unlock();
		  
  }
#ifdef LMI_SYS128
/*20070808 add APP2 专用于股票和浏览器*/
/*tj modi 2009/06/23*/
#define            APPL2_MEM_SIZE (46*1024*1024)
 unsigned char   g_Appl2_Mem[APPL2_MEM_SIZE];
unsigned int      g_ppAddr2 = (unsigned int)g_Appl2_Mem;
ST_Partition_t   *gp_appl2_partition = NULL;

#else
/*20070808 add APP2 专用于股票和浏览器*/
#define            APPL2_MEM_SIZE (11*1024*1024+0x50)
 unsigned char   g_Appl2_Mem[APPL2_MEM_SIZE];
unsigned int      g_ppAddr2 = (unsigned int)g_Appl2_Mem;
ST_Partition_t   *gp_appl2_partition = NULL;
#endif
/**/

/*函数名:     void CH_ALL_InitApp2Partition(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/08
  *函数功能:重新得到所有需要使用应用区2内存分区指针
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
/*函数体定义*/
void CH_ALL_InitApp2Partition(void)
{
       ;
}
/*函数名:     void CH_Appl2Mem_init(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/08
  *函数功能:初始化应用区2内存
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void CH_Appl2Mem_init(void)
{
 	 //gp_appl2_partition=partition_create_heap((U8*)0xB4000000,APPL2_MEM_SIZE);	
 	 gp_appl2_partition=partition_create_heap((U8*)g_Appl2_Mem,APPL2_MEM_SIZE);	
        CH_ALL_InitApp2Partition();                    
}

/*函数名:     void CH_Appl2Mem_Delete(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/08
  *函数功能:删除应用区2内存
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void CH_Appl2Mem_Delete(void)
{
    partition_delete(gp_appl2_partition);
}

STLAYER_ViewPortHandle_t 	gVPStill[2];       /*静止层VIEWPORT*/
STGXOBJ_Bitmap_t		gStillBitmap;	    /*静止层位图*/

/* 压缩 结构 */
typedef struct
{
	U32 uOffset;
	U16	uSize;
	U16	uData;
}tuple_node;

/*函数名:     void LzUncompress( U8* pCompress, U16* pObjData, U32 uByteLen )
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:图片解压缩
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void LzUncompress( U8* pCompress, U16* pObjData, U32 uByteLen )
{
	U32				uLoop1, uLoop2, uSrcIndex;
	tuple_node		tuple;

	uLoop1		= 0;
	uLoop2		= 0;	
	uSrcIndex	= 0;

	do{
#if 0
		ReadFile( fOpen, &(tuple.uOffset), 4, &uAct, NULL );
		ReadFile( fOpen, &(tuple.uSize), 2, &uAct, NULL );
#else
		tuple.uOffset	= ( pCompress[uSrcIndex + 3] << 24 ) + ( pCompress[uSrcIndex + 2] << 16 ) + ( pCompress[uSrcIndex + 1] << 8 ) + pCompress[uSrcIndex];
		uSrcIndex += 4;
		tuple.uSize	= ( pCompress[uSrcIndex + 1] << 8 ) + pCompress[uSrcIndex];
		uSrcIndex += 2;
#endif

		if( tuple.uOffset == 0 && tuple.uSize == 0 )
		{
			tuple.uData			= ( pCompress[uSrcIndex + 1] << 8 ) + pCompress[uSrcIndex];
			uSrcIndex			+= 2;
			pObjData[uLoop1++]	= tuple.uData;
		}
		else
		{
#if 0
			for( uLoop2 = tuple.uOffset; uLoop2 < tuple.uOffset + tuple.uSize - 1; uLoop2 ++ )
			{
				pObjData[uLoop1++] = pObjData[uLoop2];
			}
#else
			memcpy( &pObjData[uLoop1], &pObjData[tuple.uOffset], (tuple.uSize - 1)*2 );
			uLoop1 += (tuple.uSize - 1);
#endif
		}
	}while( uLoop1 < uByteLen/2 );

	return;
}

/*
  * Func: CH6_GetPicIndex
  * Desc: 通过图片名字得到索引
  * In:	cPicName	-> 图片名字
  * Out:	index	-> 索引
  		-1		-> 没有找到
  */
S16 CH6_GetPicIndex( char* cPicName )
{
	S16 uLoop;
	U16 uLen;


	uLen = strlen( cPicName );

	for( uLoop = 0; uLoop < USED_PIC_FILE_NUM; uLoop ++ )
	{
		if( memcmp( GlobalFlashFileInfo[uLoop].FlashFIleName, cPicName, uLen ) == 0 )
			return uLoop;
	}

	/* 没找到 */
	return -1;
}
/*函数名:     boolean CH_AllocBitmap(STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:分配显示空间
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
boolean CH_AllocBitmap(STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
 {
	 
	 ST_ErrorCode_t ErrCode;
	 STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
	 STAVMEM_AllocBlockParams_t    AllocParams;
	 
	 STAVMEM_BlockHandle_t   BlockHandle;
	 STGXOBJ_Bitmap_t* pBMP;
	 int i;
	 pBMP=Chbmp;
	 
	 
	 pBMP->PreMultipliedColor=FALSE;
	 pBMP->ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	 pBMP->AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
	 pBMP->Width=BMPWidth;
	 pBMP->Height=BMPHeight;
	 pBMP->Pitch=0;
	 pBMP->Offset=0;
	 pBMP->Data1_p=NULL;
	 pBMP->Size1=0;
	 pBMP->Data2_p=NULL;
	 pBMP->Size2=0;
	 pBMP->SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	 pBMP->BigNotLittle=FALSE;
	 
	 
	 ErrCode=STLAYER_GetBitmapAllocParams(LayerHandle,pBMP,&BitmapParams1,&BitmapParams2);
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STLAYER_GetBitmapAllocParams()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 AllocParams.PartitionHandle= AVMEMHandle[0];
	 
	 
	 
	 AllocParams.Size=BitmapParams1.AllocBlockParams.Size+BitmapParams2.AllocBlockParams.Size;
	 
	 AllocParams.Alignment=BitmapParams1.AllocBlockParams.Alignment;
	 AllocParams.AllocMode= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
	 AllocParams.NumberOfForbiddenRanges=0;
	 AllocParams.ForbiddenRangeArray_p=(STAVMEM_MemoryRange_t*)NULL;
	 AllocParams.NumberOfForbiddenBorders=0;
	 AllocParams.ForbiddenBorderArray_p=NULL;
	 
	 
	 pBMP->Pitch=BitmapParams1.Pitch;
	 pBMP->Offset=BitmapParams1.Offset;
	 pBMP->Size1=BitmapParams1.AllocBlockParams.Size;
	 
	 pBMP->Size2=BitmapParams2.AllocBlockParams.Size;
	 
	 
	 ErrCode=STAVMEM_AllocBlock((const STAVMEM_AllocBlockParams_t*)&AllocParams,
		 (STAVMEM_BlockHandle_t *)&BlockHandle); 
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 ErrCode=STAVMEM_GetBlockAddress(BlockHandle, (void **)&(pBMP->Data1_p));
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 if(pBMP->BitmapType==STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
	 {
		 pBMP->Data2_p=(U8*)pBMP->Data1_p + pBMP->Size1;
	 }
	 
	 
	 
	 if(pBMP->ColorType==STGXOBJ_COLOR_TYPE_CLUT8)
	 {
		 memset( pBMP->Data1_p, 0x0, pBMP->Size1);
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0, pBMP->Size2);
	 }
	 else
	 {
		 memset( pBMP->Data1_p, 0xff, pBMP->Size1);
		 /*		memset( pBMP->Data1_p, 0xAA ,pBMP->Size1);*/
		 
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0f, pBMP->Size2);
		 i=0;
		 do
		 {
			 *((U8*)pBMP->Data1_p+i+1)=0x0f;
			 if(pBMP->Data2_p!=NULL) *((U8*)pBMP->Data2_p+i)=0x0f;
			 
			 i+=2;
			 
		 }while(i<=pBMP->Size1);
	 }				
	 return FALSE;		
}

/*函数名:     ST_ErrorCode_t CH6_OpenViewPortStill
(char *PICFileName,CH_VideoOutputAspect Ratio,CH_VideoOutputMode HDMode)
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:打开静止层
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
ST_ErrorCode_t CH6_OpenViewPortStill(char *PICFileName,CH_VideoOutputAspect Ratio,CH_VideoOutputMode HDMode)
{
	ST_ErrorCode_t				error;
	STAVMEM_AllocBlockParams_t  	AllocBlockParams; /* Allocation param*/
	STAVMEM_BlockHandle_t 		BlockHandle, tStillBlockHandle, tPaletteBlockHandle;
	void							*Unpitched_Data_p, *cStillBuff;
	STLAYER_ViewPortParams_t		LayerVpParams;
	STLAYER_GlobalAlpha_t			GloabalAlpha;
	STLAYER_ViewPortSource_t		VPSource;
	U16							uLoop;
	char							cBuff1[50];
	STGXOBJ_PaletteAllocParams_t  	PaletteParams;					/* 调色板分配参数 */
	
	STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
	U8* 							pBuffPalette;
	STGXOBJ_Bitmap_t    *Bitmap_p;
	S16 FileIndex;
	STAVMEM_FreeBlockParams_t   FreeBlockParams;
	STGXOBJ_Palette_t PaletteTemp;
	int PosXTemp;
	int PosYTemp;
	int WidthTemp;
	int HeightTemp;

	 STGXOBJ_AspectRatio_t AspectRatio;
	STVTG_TimingMode_t TimingMode[2];
	LAYER_Type LAYERType[2];
	CH_VideoOutputMode Mode[2];
	int i;

      LAYERType[0]=LAYER_GFX3;
	LAYERType[1]=LAYER_GFX3;

	Mode[0]=HDMode; 
	Mode[1]=MODE_576I50;
	


	AspectRatio=CH_GetSTScreenARFromCH(Ratio);

	for(i=0;i<2;i++)
	{
		TimingMode[i]=CH_GetSTTimingModeFromCH(HDMode);
	
	
		switch(TimingMode[i])	
		{
		case STVTG_TIMING_MODE_1080I60000_74250:
			PosXTemp=1;
			PosYTemp=41;
			break;
		case STVTG_TIMING_MODE_1080I50000_74250_1:/* 20070805 modify*/
			PosXTemp=46-10-10-4;
			PosYTemp=25-3-5-2;
			break;
		case STVTG_TIMING_MODE_720P60000_74250:
			PosXTemp=1;
			PosYTemp=11;
			break;
		case STVTG_TIMING_MODE_720P50000_74250:
			PosXTemp=1+30-3-5-8;
			PosYTemp=11+6-10+1;
			break;
		case STVTG_TIMING_MODE_576P50000_27000:/* cqj 20070726 modify*/
			PosXTemp=19-9;
			PosYTemp=14-10+3;	
			break;
		case STVTG_TIMING_MODE_576I50000_13500:
				
			PosXTemp=16+3-10;/*zxg 20070517 add +15*/
			PosYTemp=10+3-4;/*zxg 20070517 add +16*/
			break;
		default:
			PosXTemp=1;
			PosYTemp=11;
			break;  
		}

				if(LAYERType[i]==LAYER_GFX3)
		{
			WidthTemp=gVTGModeParam.ActiveAreaWidth-1;
			HeightTemp=gVTGModeParam.ActiveAreaHeight-1;
		}
		else 
		{
			WidthTemp=gVTGAUXModeParam.ActiveAreaWidth-1;
			HeightTemp=gVTGAUXModeParam.ActiveAreaHeight-1;
			
		}	
			
	
	

	
	
	AllocBlockParams.PartitionHandle            		= AVMEMHandle[1];
	AllocBlockParams.Size                       			= 0;
	AllocBlockParams.AllocMode                  		= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
	AllocBlockParams.NumberOfForbiddenRanges    	= 0;
	AllocBlockParams.ForbiddenRangeArray_p      	= (STAVMEM_MemoryRange_t *)NULL;
	AllocBlockParams.NumberOfForbiddenBorders   	= 0;
	AllocBlockParams.ForbiddenBorderArray_p     	= NULL;
	
	AllocBlockParams.Alignment 					= /*4*/2; /* * 32-bits aligned       */     
	

	gStillBitmap.PreMultipliedColor=FALSE;
	gStillBitmap.ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	gStillBitmap.AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
	
	gStillBitmap.Pitch=0;
	gStillBitmap.Offset=0;
	gStillBitmap.Data1_p=NULL;
	gStillBitmap.Size1=0;
	gStillBitmap.Data2_p=NULL;
	gStillBitmap.Size2=0;
	gStillBitmap.SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	gStillBitmap.BigNotLittle=FALSE;
	gStillBitmap.ColorType    	= STGXOBJ_COLOR_TYPE_ARGB1555;
	gStillBitmap.BitmapType=STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;

	CH_AllocBitmap(LAYERHandle[LAYER_GFX3],&gStillBitmap,719,575);

	
	memset((void *)&LayerVpParams, 0, sizeof(STLAYER_ViewPortParams_t));
	memset((void *)&VPSource, 0, sizeof(STLAYER_ViewPortSource_t));
	


    
	FileIndex=CH6_GetPicIndex(PICFileName);
	
	if(FileIndex==-1)
	{
		do_report(0,"get back.gam failed!\n");
		/*return 1; */
	}
	#if 1
/*	if(gFlashFileInfo[FileIndex].uBmpType!=PIC_TYPE_YCBCR422) 
		return 1; */
	if(FileIndex!=-1)
	{
		if(GlobalFlashFileInfo[FileIndex].ZipSign!=0) 
		{
			LzUncompress((U8*)GlobalFlashFileInfo[FileIndex].FlasdAddr, 
				(U16*)gStillBitmap.Data1_p,
				719*2*575);
		}
		else 
		{
			
			memcpy((void*)gStillBitmap.Data1_p,
				(const void*)GlobalFlashFileInfo[FileIndex].FlasdAddr,
				GlobalFlashFileInfo[FileIndex].FileLen);
			
		}
	}
	
	#endif
	LayerVpParams.Source_p				= &VPSource;
	LayerVpParams.Source_p->Data.BitMap_p	= &gStillBitmap;
	LayerVpParams.Source_p->Palette_p		= NULL;
	LayerVpParams.Source_p->SourceType	= STLAYER_GRAPHIC_BITMAP;
	
	/*=======Set the input rectangle param======================== */
	LayerVpParams.InputRectangle.PositionX	= 0;				/*! Louie(5/18/2004): 40, must be a value divisible by 8 for CLUT8 */
	LayerVpParams.InputRectangle.PositionY	= 0;
	LayerVpParams.InputRectangle.Width		= 719;
	LayerVpParams.InputRectangle.Height 	= 575;
	
	/*=======Set the output rectangle param======================== */
	LayerVpParams.OutputRectangle.PositionX 	= PosXTemp;		
	LayerVpParams.OutputRectangle.PositionY 	= PosYTemp;
	LayerVpParams.OutputRectangle.Width 	= WidthTemp;
	LayerVpParams.OutputRectangle.Height	= HeightTemp;
	
	
	/*===============Open VP1================================*/
	error = STLAYER_OpenViewPort(LAYERHandle[LAYERType[i]],&LayerVpParams, &gVPStill[i] );
	if ( error != ST_NO_ERROR)
	{
		STTBX_Print(( "\n[GRA]->>STLAYER_OpenViewPort()=%s",GetErrorText(error) ));
		return error;
	}

	
	GloabalAlpha.A0 = 128;
	GloabalAlpha.A1 = 128;
	error = STLAYER_SetViewPortAlpha( gVPStill[i], &GloabalAlpha);
	if( error != ST_NO_ERROR )
	{
		STTBX_Print(( "\n[GRA]->>STLAYER_OpenViewPort()=%s",GetErrorText(error) ));
		return error;
	}
	

	error=STLAYER_SetViewPortSource(gVPStill[i],&VPSource);
	if(error!=ST_NO_ERROR)
	{
		STTBX_Print(("STLAYER_SetViewPortSource()=%s",GetErrorText(error)));
		return TRUE;		
	}

       STLAYER_EnableViewPort(gVPStill[i]);
	}
	return ST_NO_ERROR;
}


/*
  * Func: CH_MEM_OSD_Malloc
  * Desc: OSD区域的分配函数
  * In:	ri_Size	-> 要分配的大小
  * Out:	分配好的区域指针
  */
void * CH_MEM_OSD_Malloc ( int ri_Size )
{
	return memory_allocate ( CHSysPartition, ri_Size );
}

/*
  * Func: CH_MEM_OSD_Free
  * Desc: OSD区域的释放函数
  * In:	要释放的指针
  * Out:	none
  */
void CH_MEM_OSD_Free( U8* rp_Mem )
{
	if ( NULL != rp_Mem )
	{
		memory_deallocate ( CHSysPartition, rp_Mem );
		rp_Mem = NULL;
	}

	return;
}

/*
  * Func: CH_MEM_Static_Malloc
  * Desc: Static区域的分配函数
  * In:	ri_Size	-> 要分配的大小
  * Out:	分配好的区域指针
  */
void * CH_MEM_Static_Malloc( int ri_Size )
{
	return memory_allocate ( SystemPartition, ri_Size );
}

/*
  * Func: CH_MEM_Static_Free
  * Desc: Static区域的释放函数
  * In:	要释放的指针
  * Out:	none
  */
void CH_MEM_Static_Free( U8* rp_Mem )
{
	if ( NULL != rp_Mem )
	{
		memory_deallocate ( SystemPartition, rp_Mem );
		rp_Mem = NULL;
	}

	return;
}

/*
  * Func: CH_MEM_Appl_Malloc
  * Desc: Appl区域的分配函数
  * In:	ri_Size	-> 要分配的大小
  * Out:	分配好的区域指针
  */
void * CH_MEM_Appl_Malloc( int ri_Size )
{
	return memory_allocate ( appl_partition, ri_Size );
}

/*
  * Func: CH_MEM_Appl_Free
  * Desc: Appl区域的释放函数
  * In:	要释放的指针
  * Out:	none
  */
void CH_MEM_Appl_Free( U8* rp_Mem )
{
	if ( NULL != rp_Mem )
	{
		memory_deallocate ( appl_partition, rp_Mem );
		rp_Mem = NULL;
	}

	return;
}


/*int check_memStatus(void)
{
	partition_status_t Status;
	partition_status(CHSysPartition, &Status,0);
	STTBX_Print(("\nCAMEMPartition>>total=%d,used=%d,free=%d\n ",Status.partition_status_size,Status.partition_status_used,Status.partition_status_free));
}/*

/*zc added for SYSset 080303*/
/*-------------------------------------------------------------------------
 * Function : VID_ChangeAspectRatio
 *            Change the layer parameters to set the aspect ratio.
 * Input    :
 * Output   :
 * Return   : ErrorCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t VID_ChangeAspectRatio(STGXOBJ_AspectRatio_t *Aspect_p)
{
    ST_ErrorCode_t        ST_ErrorCode;
    STVMIX_ScreenParams_t ScreenParams;
    STLAYER_LayerParams_t LayerParams;

    ST_ErrorCode = STLAYER_GetLayerParams(LAYERHandle[LAYER_VIDEO2], &LayerParams);
    if (ST_ErrorCode != ST_NO_ERROR)
        return ST_ErrorCode;

    ST_ErrorCode = STVMIX_GetScreenParams(VMIXHandle[VMIX_AUX], &ScreenParams);
    if (ST_ErrorCode != ST_NO_ERROR)
        return ST_ErrorCode;

    LayerParams.AspectRatio  = *Aspect_p;
    ScreenParams.AspectRatio = *Aspect_p;

    ST_ErrorCode = STLAYER_SetLayerParams(LAYERHandle[LAYER_VIDEO2], &LayerParams);
    if (ST_ErrorCode != ST_NO_ERROR)
        return ST_ErrorCode;

    ST_ErrorCode = STVMIX_SetScreenParams(VMIXHandle[VMIX_AUX], &ScreenParams);

    return ST_ErrorCode;
}


/*End of file*/

BOOL X_SettingVideoAspect( CH_VideoOutputType Type,STGXOBJ_AspectRatio_t Aspect_Index )
{
	ST_ErrorCode_t ErrCode;
	STVMIX_ScreenParams_t ScreenParams;

       if(Type==TYPE_CVBS)
	{
		if(Aspect_Index)
		{
			ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
		}
		else
		{
			ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
		}
		
		ScreenParams.ScanType    = gVTGAUXModeParam.ScanType;
		ScreenParams.FrameRate   = gVTGAUXModeParam.FrameRate;
		ScreenParams.Width       = gVTGAUXModeParam.ActiveAreaWidth;
		ScreenParams.Height      = gVTGAUXModeParam.ActiveAreaHeight;
		ScreenParams.XStart      = gVTGAUXModeParam.DigitalActiveAreaXStart;

#ifdef REMOVE_BEIJING_HANDLE/*20061109*/
	       /*修改为了瞒足入网测试，把300行位置挪动*/
	       ScreenParams.YStart      = gVTGAUXModeParam.DigitalActiveAreaYStart+2; 
#else
		ScreenParams.YStart      = gVTGAUXModeParam.DigitalActiveAreaYStart; 
#endif

		if (ScreenParams.YStart%2) /* if odd */
			ScreenParams.YStart++;	

		ErrCode = STVMIX_SetScreenParams(VMIXHandle[VMIX_AUX], &ScreenParams);
		if (ErrCode!= ST_NO_ERROR)
		{
			STTBX_Print(("VMIX_SetScreenParams(VMIX_AUX)=%s\n", GetErrorText(ErrCode) ));
			return TRUE;

		}
	}
	else
	{
		if(Aspect_Index)
		{
			ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
		}
		else
		{
			ScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
		}

		ScreenParams.ScanType    = gVTGModeParam.ScanType;
		ScreenParams.FrameRate   = gVTGModeParam.FrameRate;
		ScreenParams.Width       = gVTGModeParam.ActiveAreaWidth;
		ScreenParams.Height      = gVTGModeParam.ActiveAreaHeight;
		ScreenParams.XStart      = gVTGModeParam.DigitalActiveAreaXStart;
		ScreenParams.YStart      = gVTGModeParam.DigitalActiveAreaYStart; 
	
		if (ScreenParams.YStart%2) /* if odd */
			ScreenParams.YStart++;	

		ErrCode = STVMIX_SetScreenParams(VMIXHandle[VMIX_MAIN], &ScreenParams);

		if (ErrCode!= ST_NO_ERROR)
		{
			STTBX_Print(("VMIX_SetScreenParams=%s\n", GetErrorText(ErrCode) ));
			return TRUE;
		}
	}
	return false;
}

/*
  * Func: CH_JS_Factory_ModeCheck
  * Desc: 工厂设置-> 判断是否处于工厂模式
  * flash 备份块起始位置0x7F000400
  */
boolean CH_JS_Factory_ModeCheck ( void )
{
	/*U8* p_Addr;

	p_Addr = (U8*)(JL_FACTORY_STORE_START + 8100);

	if ( 'L' == p_Addr[0] && 'u' == p_Addr[1] && 'o' == p_Addr[2] && 'j' == p_Addr[3] )
	{
		return true;
	}*/

	return false;
}
#ifdef CH_MUTI_RESOLUTION_RATE

//#define USE_BMP_SCALE

#include "stavmem.h"

#define        HISOLUTION_WITH   720 
#define        HISOLUTION_HIGH   576
 boolean CH_CreateViewPort_HighSolution(CH_RESOLUTION_MODE_e renum_mode,CH_VideoOutputAspect rch_Ratio,CH_VideoOutputMode rch_HDMode);
#ifdef USE_BMP_SCALE
void bmscaler(int dest_width,int dest_height,int source_width, int source_height,
			  U32 *DesBitmap, U32 *bitmap);
#endif
/*当前的OSD 屏幕分辨率*/
CH_RESOLUTION_MODE_e   genum_Resolution_Rate = CH_1280X720_MODE ;
  
STLAYER_ViewPortHandle_t 	g_VPOSD_HighSolution[2];                          /*高清OSD菜单Viewport*/
STGXOBJ_Bitmap_t		g_OSDBitmap_HighSolution[2];	             /*高清OSD菜单OSD菜单*/
STAVMEM_BlockHandle_t      g_OSDBitmap_BlockHandle[2];


STGXOBJ_Bitmap_t		g_TempOSDBitmap_HighSolution[2];	             /*用于单独的切换*/
STAVMEM_BlockHandle_t      g_TempOSDBitmap_BlockHandle[2];


/************************函数说明***********************************/
/*函数名:vvoid CH_SetReSolutionRate(CH_RESOLUTION_MODE_e renum_Mode)                 */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:设置屏幕分辨率模式                                                    */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
void CH_SetReSolutionRate(CH_RESOLUTION_MODE_e renum_Mode)
{
    genum_Resolution_Rate = renum_Mode;
}

/************************函数说明***********************************/
/*函数名:CH_RESOLUTION_MODE_e CH_GetReSolutionRate(void )                                    */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:设置屏幕分辨率模式                                                    */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
CH_RESOLUTION_MODE_e CH_GetReSolutionRate(void )     
{
   return  genum_Resolution_Rate ;
}

/*函数名:     boolean CH_AllocBitmap_HighSolution(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:分配显示空间
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
boolean CH_AllocBitmap_HighSolution(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
 {
	 
	 ST_ErrorCode_t ErrCode;
	 STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
	 STAVMEM_AllocBlockParams_t    AllocParams;
	 
	 STGXOBJ_Bitmap_t* pBMP;
	 int i;
	 pBMP=Chbmp;
	 
	 
	 pBMP->PreMultipliedColor=FALSE;
	 pBMP->ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	 pBMP->AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
	 pBMP->Width=BMPWidth;
	 pBMP->Height=BMPHeight;
	 pBMP->Pitch=0;
	 pBMP->Offset=0;
	 pBMP->Data1_p=NULL;
	 pBMP->Size1=0;
	 pBMP->Data2_p=NULL;
	 pBMP->Size2=0;
	 pBMP->SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	 pBMP->BigNotLittle=FALSE;
	 
	 
	 ErrCode=STLAYER_GetBitmapAllocParams(LayerHandle,pBMP,&BitmapParams1,&BitmapParams2);
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STLAYER_GetBitmapAllocParams()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 AllocParams.PartitionHandle= AVMEMHandle[0];
	 
	 
	 
	 AllocParams.Size=BitmapParams1.AllocBlockParams.Size+BitmapParams2.AllocBlockParams.Size;
	 
	 AllocParams.Alignment=BitmapParams1.AllocBlockParams.Alignment;
	 AllocParams.AllocMode= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
	 AllocParams.NumberOfForbiddenRanges=0;
	 AllocParams.ForbiddenRangeArray_p=(STAVMEM_MemoryRange_t*)NULL;
	 AllocParams.NumberOfForbiddenBorders=0;
	 AllocParams.ForbiddenBorderArray_p=NULL;
	 
	 
	 pBMP->Pitch=BitmapParams1.Pitch;
	 pBMP->Offset=BitmapParams1.Offset;
	 pBMP->Size1=BitmapParams1.AllocBlockParams.Size;
	 
	 pBMP->Size2=BitmapParams2.AllocBlockParams.Size;
	 
	 
	 ErrCode=STAVMEM_AllocBlock((const STAVMEM_AllocBlockParams_t*)&AllocParams,
		 (STAVMEM_BlockHandle_t *)&g_OSDBitmap_BlockHandle[ri_LayerIndex]); 
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 ErrCode=STAVMEM_GetBlockAddress(g_OSDBitmap_BlockHandle[ri_LayerIndex], (void **)&(pBMP->Data1_p));
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 if(pBMP->BitmapType==STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
	 {
		 pBMP->Data2_p=(U8*)pBMP->Data1_p + pBMP->Size1;
	 }
	 
	 
	 
	 if(pBMP->ColorType==STGXOBJ_COLOR_TYPE_CLUT8)
	 {
		 memset( pBMP->Data1_p, 0x0, pBMP->Size1);
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0, pBMP->Size2);
	 }
	 else
	 {
		 memset( pBMP->Data1_p, 0xff, pBMP->Size1);
		 /*		memset( pBMP->Data1_p, 0xAA ,pBMP->Size1);*/
		 
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0f, pBMP->Size2);
		 i=0;
		 do
		 {
			 *((U8*)pBMP->Data1_p+i+1)=0x0f;
			 if(pBMP->Data2_p!=NULL) *((U8*)pBMP->Data2_p+i)=0x0f;
			 
			 i+=2;
			 
		 }while(i<=pBMP->Size1);
	 }				
	 return FALSE;		
}


/************************函数说明*******************************************/
/*函数名:boolean CH_CreateViewPort_HighSolution(CH_VideoOutputAspect rch_Ratio,CH_VideoOutputMode rch_HDMode) */
/*开发人和开发时间:                                                                                          */
/*函数功能描述:创建高清OSD菜单                                                              */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
boolean CH_CreateViewPort_HighSolution(CH_RESOLUTION_MODE_e renum_mode,CH_VideoOutputAspect rch_Ratio,CH_VideoOutputMode rch_HDMode)
{
	ST_ErrorCode_t				error;
	STAVMEM_AllocBlockParams_t  	AllocBlockParams; /* Allocation param*/
	STLAYER_ViewPortParams_t		LayerVpParams;
	STLAYER_GlobalAlpha_t			GloabalAlpha;
	STLAYER_ViewPortSource_t		VPSource;
	
	int                                              PosXTemp;
	int                                              PosYTemp;
	int                                              WidthTemp;
	int                                              HeightTemp;

	 STGXOBJ_AspectRatio_t             AspectRatio;
	STVTG_TimingMode_t                 TimingMode[2];
	LAYER_Type                               LAYERType[2];
	CH_VideoOutputMode                  Mode[2];
	int                                             i;

       /*设置对应的GFX LAYER和输出模式*/
       LAYERType[0] = LAYER_GFX1;
	LAYERType[1] = LAYER_GFX2;

	Mode[0] = rch_HDMode; 
	Mode[1] = MODE_576I50;
	
      /******************************************/

	AspectRatio = CH_GetSTScreenARFromCH(rch_Ratio);

	for(i=0;i<2;i++)
	{
		TimingMode[i]=CH_GetSTTimingModeFromCH(rch_HDMode);
		switch(TimingMode[i])	
		{
		case STVTG_TIMING_MODE_1080I60000_74250:
			PosXTemp=1;
			PosYTemp=41;
			break;
		case STVTG_TIMING_MODE_1080I50000_74250_1: 
			PosXTemp=46-10-10-4;
			PosYTemp=25-3-5-2;
			break;
		case STVTG_TIMING_MODE_720P60000_74250:
			PosXTemp=1;
			PosYTemp=11;
			break;
		case STVTG_TIMING_MODE_720P50000_74250:
			PosXTemp=1+30-3-5-8;
			PosYTemp=11+6-10+1;
			break;
		case STVTG_TIMING_MODE_576P50000_27000: 
			PosXTemp=19-9;  
			PosYTemp=14-10+3;	 
			break;
		case STVTG_TIMING_MODE_576I50000_13500:
				
			PosXTemp=16+3-10; 
			PosYTemp=10+3-4; 
			break;
		default:
			PosXTemp=1;
			PosYTemp=11;
			break;  
		}

		if(LAYERType[i] == LAYER_GFX1)/*高清输出*/
		{
		       PosXTemp =  /*gVTGModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGModeParam.ActiveAreaWidth;
			HeightTemp = gVTGModeParam.ActiveAreaHeight;
		}
		else                                       /*标清输出*/
		{
		       PosXTemp =  /*gVTGAUXModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGAUXModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGAUXModeParam.ActiveAreaWidth;
			HeightTemp = gVTGAUXModeParam.ActiveAreaHeight;
			
		}	
			
		AllocBlockParams.PartitionHandle            		= AVMEMHandle[1];
		AllocBlockParams.Size                       			= 0;
		AllocBlockParams.AllocMode                  		= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
		AllocBlockParams.NumberOfForbiddenRanges    	= 0;
		AllocBlockParams.ForbiddenRangeArray_p      	= (STAVMEM_MemoryRange_t *)NULL;
		AllocBlockParams.NumberOfForbiddenBorders   	= 0;
		AllocBlockParams.ForbiddenBorderArray_p     	= NULL;
		
		AllocBlockParams.Alignment 					= 4; /*  32-bits aligned       */     
		
	      
		g_OSDBitmap_HighSolution[i].PreMultipliedColor=FALSE;
		g_OSDBitmap_HighSolution[i].ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
		g_OSDBitmap_HighSolution[i].AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
		
		g_OSDBitmap_HighSolution[i].Pitch=0;
		g_OSDBitmap_HighSolution[i].Offset=0;
		g_OSDBitmap_HighSolution[i].Data1_p=NULL;
		g_OSDBitmap_HighSolution[i].Size1=0;
		g_OSDBitmap_HighSolution[i].Data2_p=NULL;
		g_OSDBitmap_HighSolution[i].Size2=0;
		g_OSDBitmap_HighSolution[i].SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
		g_OSDBitmap_HighSolution[i].BigNotLittle=FALSE;
		g_OSDBitmap_HighSolution[i].ColorType    	= STGXOBJ_COLOR_TYPE_ARGB8888;
		g_OSDBitmap_HighSolution[i].BitmapType=STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;

              
		memset((void *)&LayerVpParams, 0, sizeof(STLAYER_ViewPortParams_t));
		memset((void *)&VPSource, 0, sizeof(STLAYER_ViewPortSource_t));
			    
		LayerVpParams.Source_p				= &VPSource;
		LayerVpParams.Source_p->Palette_p		= NULL;
		LayerVpParams.Source_p->SourceType	= STLAYER_GRAPHIC_BITMAP;
		
		/*=======输入数据是720P格式======================== */
		if(renum_mode == CH_720X576_MODE)
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			LayerVpParams.InputRectangle.Width		= act_src_w;
			LayerVpParams.InputRectangle.Height 	= act_src_h;
			CH_AllocBitmap_HighSolution(i,LAYERHandle[LAYERType[i] ],&g_OSDBitmap_HighSolution[i],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );

		}
	      else
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			#ifdef USE_BMP_SCALE
			if(i == 0)
				{
			LayerVpParams.InputRectangle.Width		= 1279;
			LayerVpParams.InputRectangle.Height 	= 719;
				}
			else
				{
	              LayerVpParams.InputRectangle.Width		= 719;
			LayerVpParams.InputRectangle.Height 	= 575;
				}
			#else
			LayerVpParams.InputRectangle.Width		= 1279;
			LayerVpParams.InputRectangle.Height 	= 719;

			#endif
			CH_AllocBitmap_HighSolution(i,LAYERHandle[LAYERType[i] ],&g_OSDBitmap_HighSolution[i],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );

		}
		 LayerVpParams.Source_p->Data.BitMap_p	= &g_OSDBitmap_HighSolution[i];

		/*=======输出分别是1080I/720P和576I======================== */
		LayerVpParams.OutputRectangle.PositionX 	= PosXTemp;		
		LayerVpParams.OutputRectangle.PositionY 	= PosYTemp;
		LayerVpParams.OutputRectangle.Width 	= WidthTemp;
		LayerVpParams.OutputRectangle.Height	= HeightTemp;
		
		
		/*===============Open VP1================================*/
		error = STLAYER_OpenViewPort(LAYERHandle[LAYERType[i]],&LayerVpParams, &g_VPOSD_HighSolution[i] );
		if ( error != ST_NO_ERROR)
		{
			STTBX_Print(( "\n[GRA]->>STLAYER_OpenViewPort()=%s",GetErrorText(error) ));
			return error;
		}

	       error = STLAYER_EnableViewPortFilter(g_VPOSD_HighSolution[i],LAYERHandle[LAYERType[i] ]);
	
		GloabalAlpha.A0 = 128;
		GloabalAlpha.A1 = 128;
		error = STLAYER_SetViewPortAlpha( g_VPOSD_HighSolution[i], &GloabalAlpha);
		if( error != ST_NO_ERROR )
		{
			STTBX_Print(( "\n[GRA]->>STLAYER_OpenViewPort()=%s",GetErrorText(error) ));
			return error;
		}
		

		error=STLAYER_SetViewPortSource(g_VPOSD_HighSolution[i],&VPSource);
		if(error!=ST_NO_ERROR)
		{
			STTBX_Print(("STLAYER_SetViewPortSource()=%s",GetErrorText(error)));
			return TRUE;		
		}
       
			  
	       error = STLAYER_EnableViewPort(g_VPOSD_HighSolution[i]);
	
	
		CH_ClearFUll_HighSolution();
	}
	return ST_NO_ERROR;
}
/************************函数说明*******************************************/
/*函数名:  boolean CH_PutRectangle_HighSolution( int ri_SourcePosX, int ri_SourcePosY,int ri_Width,int ri_Height,U8* rup_pBMPData,int ri_DesPosX, int ri_DesPosY )*/
/*开发人和开发时间:                                                                                          */
/*函数功能描述:创建高清OSD菜单                                                              */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
 
 boolean CH_PutRectangle_HighSolution( int ri_SourcePosX, int ri_SourcePosY,int ri_Width,int ri_Height,U8* rup_pBMPData,int ri_DesPosX, int ri_DesPosY )
 {
      int i_ErrCode;
      STGXOBJ_Rectangle_t st_Rect;
      STGXOBJ_Bitmap_t    st_BMPT;
	  STGXOBJ_Rectangle_t rect1;
	    STGXOBJ_Rectangle_t rect2;
	static    U32 *puc_bmpData =NULL;
      st_Rect.PositionX = ri_SourcePosX;
      st_Rect.PositionY = ri_SourcePosY;  
      st_Rect.Width = ri_Width;
      st_Rect.Height = ri_Height;

	  
      if(rup_pBMPData == NULL || ri_SourcePosX < 0 || ri_SourcePosY < 0 || ri_Width < 0 || ri_Height < 0)
      	{
           return true;
      	}
       memcpy((void*)&st_BMPT,(const void*)&g_OSDBitmap_HighSolution[0],sizeof(STGXOBJ_Bitmap_t));

	   
      	st_BMPT.Pitch  = 1280*4;
	st_BMPT.Size1 = 1280*720*4;	
	st_BMPT.Width = 1280;
	st_BMPT.Height = 720;
	st_BMPT.Data2_p = NULL;

	st_BMPT.Data1_p = rup_pBMPData;


      do_report(0,"PositionX %d,ri_SourcePosY %d,Width %d Height %d ri_DesPosX %d ri_DesPosY %d %x\n",ri_SourcePosX,ri_SourcePosY,ri_Width,ri_Height,ri_DesPosX,ri_DesPosY,rup_pBMPData);
     // CH_SwitchIOOsd();
     /*首先更新高清OSD*/
      i_ErrCode   =  STGFX_CopyArea(GFXHandle,&st_BMPT,&g_OSDBitmap_HighSolution[0],&gGC,&st_Rect,ri_DesPosX,ri_DesPosY);
      i_ErrCode   =  STGFX_Sync(GFXHandle);
      /*再次更新标清OSD*/
#ifdef USE_BMP_SCALE
    if(puc_bmpData== NULL)
    	{
            puc_bmpData = memory_allocate(CHSysPartition,719*575*4);
    	}
      bmscaler(719,575,1280, 720,puc_bmpData,rup_pBMPData);
     	st_BMPT.Pitch  = 719*4;
	st_BMPT.Size1 = 719*575*4;	
	st_BMPT.Width = 719;
	st_BMPT.Height = 575;
	st_BMPT.Data2_p = NULL;

	st_BMPT.Data1_p = puc_bmpData;
#endif	  
     i_ErrCode   =  STGFX_CopyArea(GFXHandle,&st_BMPT,&g_OSDBitmap_HighSolution[1],&gGC,&st_Rect,ri_DesPosX,ri_DesPosY);
      i_ErrCode   =  STGFX_Sync(GFXHandle);
     //CH_SwitchIOOsd();
   
     // STLAYER_GetViewPortIORectangle(g_VPOSD_HighSolution[0],&rect1,&rect2);

	  

     /************************/
     return false;
	  
 }


/************************函数说明*******************************************/
/*函数名: boolean CH_ClearFUll_HighSolution( void )*/
/*开发人和开发时间:                                                                                          */
/*函数功能描述:清除OSD                                                                                    */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
 boolean CH_ClearFUll_HighSolution( void )
 {
	int i_ErrCode;
	STGXOBJ_Rectangle_t st_Rect;
	STGXOBJ_Color_t       st_Color;
	st_Color.Type =  STGXOBJ_COLOR_TYPE_ARGB8888;
	st_Color.Value.ARGB8888.Alpha = 0x00;
	st_Color.Value.ARGB8888.R = 0xFF;
	st_Color.Value.ARGB8888.G = 0xFF;
	st_Color.Value.ARGB8888.B = 0xFF;


 /*首先更新高清OSD*/
      i_ErrCode = STGFX_Clear(GFXHandle, &g_OSDBitmap_HighSolution[0], &st_Color);
      i_ErrCode   =  STGFX_Sync(GFXHandle);
      /*再次更新标清OSD*/
      
      i_ErrCode = STGFX_Clear(GFXHandle, &g_OSDBitmap_HighSolution[1], &st_Color);
      i_ErrCode   =  STGFX_Sync(GFXHandle);       /************************/
        return false;
     
 }


/************************函数说明*******************************************/
/*函数名void CH_CreateOSDViewPort_HighSolution(void)                                                  */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:创建OSD Viewport                                                                       */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
void CH_CreateOSDViewPort_HighSolution(void)
{
     boolean i_res;
     CH_RESOLUTION_MODE_e enum_Mode;

     
	 
     enum_Mode = CH_GetReSolutionRate() ;
    if(enum_Mode == CH_OLD_MOE)
    {
        CH_SetupViewPort(LAYER_GFX1,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
  	 CH_SetViewPortAlpha(CH6VPOSD,GLOBAL_VIEWPORT_ALPHA);
	if(CH6_ShowViewPort(&CH6VPOSD))
	{ 
		STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is not successful"));
	}
	else	
		STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is successful"));
    }
     else 
     {
            CH_CreateViewPort_HighSolution(enum_Mode,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
     }

     return;
}
/************************函数说明*******************************************/
/*函数名void CH_DeleteOSDViewPort_HighSolution(void)                                                  */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:删除OSD Viewport                                                                       */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
void CH_DeleteOSDViewPort_HighSolution(void)
{
     STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
     boolean i_res = 0;
     int i;
     CH_RESOLUTION_MODE_e enum_Mode;
     enum_Mode = CH_GetReSolutionRate() ;
     if(enum_Mode == CH_OLD_MOE)
     {
           CH6_DeleteViewPort(CH6VPOSD);
     }
    else 	 
    {
	 for(i = 0;i < 2;i++)
	 {
	    i_res = STLAYER_CloseViewPort(g_VPOSD_HighSolution[i]);
	 }
	/*释放相关内存*/
	 for(i = 0;i < 2;i++)
	 {
	    st_FreeBlockParas.PartitionHandle = AVMEMHandle[0];
	    if(g_OSDBitmap_HighSolution[i].Data1_p != NULL)
	    {
	       i_res =  STAVMEM_FreeBlock(&st_FreeBlockParas,&g_OSDBitmap_BlockHandle[i]);
	    }
	}
   }
}

boolean  CH_SetViewPortAlpha_HighSolution(int AlphaValue)
{
	ST_ErrorCode_t				error;
	STLAYER_GlobalAlpha_t			GloabalAlpha;
	int                                             i;

	for(i=0;i<2;i++)
	{
		GloabalAlpha.A0 = 0x80; 
		GloabalAlpha.A1 = AlphaValue;
		error = STLAYER_SetViewPortAlpha( g_VPOSD_HighSolution[i], &GloabalAlpha);
		if( error != ST_NO_ERROR )
		{
			STTBX_Print(( "\n[GRA]->>STLAYER_OpenViewPort()=%s",GetErrorText(error) ));
			return error;
		}
		
	}
	CH_SwitchIOOsd();/*恢复IO控制参数*/
}

/*函数名:     boolean CH_AllocBitmap_ChangeHighSolution(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:分配显示空间
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
boolean CH_AllocBitmap_ChangeHighSolution(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
 {
	 
	 ST_ErrorCode_t ErrCode;
	 STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
	 STAVMEM_AllocBlockParams_t    AllocParams;
	 
	 STGXOBJ_Bitmap_t* pBMP;
	 int i;
	 pBMP=Chbmp;
	 
	 
	 pBMP->PreMultipliedColor=FALSE;
	 pBMP->ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	 pBMP->AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
	 pBMP->Width=BMPWidth;
	 pBMP->Height=BMPHeight;
	 pBMP->Pitch=0;
	 pBMP->Offset=0;
	 pBMP->Data1_p=NULL;
	 pBMP->Size1=0;
	 pBMP->Data2_p=NULL;
	 pBMP->Size2=0;
	 pBMP->SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	 pBMP->BigNotLittle=FALSE;
	 
	 
	 ErrCode=STLAYER_GetBitmapAllocParams(LayerHandle,pBMP,&BitmapParams1,&BitmapParams2);
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STLAYER_GetBitmapAllocParams()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 AllocParams.PartitionHandle= AVMEMHandle[0];
	 
	 
	 
	 AllocParams.Size=BitmapParams1.AllocBlockParams.Size+BitmapParams2.AllocBlockParams.Size;
	 
	 AllocParams.Alignment=BitmapParams1.AllocBlockParams.Alignment;
	 AllocParams.AllocMode= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
	 AllocParams.NumberOfForbiddenRanges=0;
	 AllocParams.ForbiddenRangeArray_p=(STAVMEM_MemoryRange_t*)NULL;
	 AllocParams.NumberOfForbiddenBorders=0;
	 AllocParams.ForbiddenBorderArray_p=NULL;
	 
	 
	 pBMP->Pitch=BitmapParams1.Pitch;
	 pBMP->Offset=BitmapParams1.Offset;
	 pBMP->Size1=BitmapParams1.AllocBlockParams.Size;
	 
	 pBMP->Size2=BitmapParams2.AllocBlockParams.Size;
	 
	 
	 ErrCode=STAVMEM_AllocBlock((const STAVMEM_AllocBlockParams_t*)&AllocParams,
		 (STAVMEM_BlockHandle_t *)&g_TempOSDBitmap_BlockHandle[ri_LayerIndex]); 
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 
	 ErrCode=STAVMEM_GetBlockAddress(g_TempOSDBitmap_BlockHandle[ri_LayerIndex], (void **)&(pBMP->Data1_p));
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 if(pBMP->BitmapType==STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
	 {
		 pBMP->Data2_p=(U8*)pBMP->Data1_p + pBMP->Size1;
	 }
	 
	 
	 
	 if(pBMP->ColorType==STGXOBJ_COLOR_TYPE_CLUT8)
	 {
		 memset( pBMP->Data1_p, 0x0, pBMP->Size1);
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0, pBMP->Size2);
	 }
	 else
	 {
		 memset( pBMP->Data1_p, 0xff, pBMP->Size1);
		 /*		memset( pBMP->Data1_p, 0xAA ,pBMP->Size1);*/
		 
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0f, pBMP->Size2);
		 i=0;
		 do
		 {
			 *((U8*)pBMP->Data1_p+i+1)=0x0f;
			 if(pBMP->Data2_p!=NULL) *((U8*)pBMP->Data2_p+i)=0x0f;
			 
			 i+=2;
			 
		 }while(i<=pBMP->Size1);
	 }				
	 return FALSE;		
}



/************************函数说明*******************************************/
/*函数名void CH_ChangeOSDViewPort_HighSolution(void)                                                  */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:改变OSD Viewport                                                                       */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
void CH_ChangeOSDViewPort_HighSolution(CH_RESOLUTION_MODE_e renum_mode,CH_VideoOutputAspect rch_Ratio,CH_VideoOutputMode rch_HDMode)
{
	ST_ErrorCode_t				error;
	STAVMEM_AllocBlockParams_t  	AllocBlockParams; /* Allocation param*/
	STLAYER_ViewPortParams_t		LayerVpParams;
	STLAYER_GlobalAlpha_t			GloabalAlpha;
	STLAYER_ViewPortSource_t		VPSource;
	
	int                                              PosXTemp;
	int                                              PosYTemp;
	int                                              WidthTemp;
	int                                              HeightTemp;

	 STGXOBJ_AspectRatio_t             AspectRatio;
	STVTG_TimingMode_t                 TimingMode[2];
	LAYER_Type                               LAYERType[2];
	CH_VideoOutputMode                  Mode[2];
	int                                             i;
           STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
       /*设置对应的GFX LAYER和输出模式*/
       LAYERType[0] = LAYER_GFX1;
	LAYERType[1] = LAYER_GFX2;

	Mode[0] = rch_HDMode; 
	Mode[1] = MODE_576I50;
	
      /******************************************/

	AspectRatio = CH_GetSTScreenARFromCH(rch_Ratio);

	for(i=0;i<2;i++)
	{
		TimingMode[i]=CH_GetSTTimingModeFromCH(rch_HDMode);
		switch(TimingMode[i])	
		{
		case STVTG_TIMING_MODE_1080I60000_74250:
			PosXTemp=1;
			PosYTemp=41;
			break;
		case STVTG_TIMING_MODE_1080I50000_74250_1: 
			PosXTemp=46-10-10-4;
			PosYTemp=25-3-5-2;
			break;
		case STVTG_TIMING_MODE_720P60000_74250:
			PosXTemp=1;
			PosYTemp=11;
			break;
		case STVTG_TIMING_MODE_720P50000_74250:
			PosXTemp=1+30-3-5-8;
			PosYTemp=11+6-10+1;
			break;
		case STVTG_TIMING_MODE_576P50000_27000: 
			PosXTemp=19-9;  
			PosYTemp=14-10+3;	 
			break;
		case STVTG_TIMING_MODE_576I50000_13500:
				
			PosXTemp=16+3-10; 
			PosYTemp=10+3-4; 
			break;
		default:
			PosXTemp=1;
			PosYTemp=11;
			break;  
		}

		if(LAYERType[i] == LAYER_GFX1)/*高清输出*/
		{
		       PosXTemp =  /*gVTGModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGModeParam.ActiveAreaWidth;
			HeightTemp = gVTGModeParam.ActiveAreaHeight;
		}
		else                                       /*标清输出*/
		{
		       PosXTemp =  /*gVTGAUXModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGAUXModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGAUXModeParam.ActiveAreaWidth;
			HeightTemp = gVTGAUXModeParam.ActiveAreaHeight;
			
		}	
			
		AllocBlockParams.PartitionHandle            		= AVMEMHandle[1];
		AllocBlockParams.Size                       			= 0;
		AllocBlockParams.AllocMode                  		= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
		AllocBlockParams.NumberOfForbiddenRanges    	= 0;
		AllocBlockParams.ForbiddenRangeArray_p      	= (STAVMEM_MemoryRange_t *)NULL;
		AllocBlockParams.NumberOfForbiddenBorders   	= 0;
		AllocBlockParams.ForbiddenBorderArray_p     	= NULL;
		
		AllocBlockParams.Alignment 					= 4; /*  32-bits aligned       */     
		
	      

		memset((void *)&VPSource, 0, sizeof(STLAYER_ViewPortSource_t));
			    
		
		/*=======输入数据是720P格式======================== */
		if(renum_mode == CH_720X576_MODE)
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			LayerVpParams.InputRectangle.Width		= act_src_w;
			LayerVpParams.InputRectangle.Height 	= act_src_h;
			CH_AllocBitmap_ChangeHighSolution(i,LAYERHandle[LAYERType[i] ],&g_TempOSDBitmap_HighSolution[i],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );

		}
	      else
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			LayerVpParams.InputRectangle.Width		= 1279;
			LayerVpParams.InputRectangle.Height 	= 719;
			CH_AllocBitmap_ChangeHighSolution(i,LAYERHandle[LAYERType[i] ],&g_TempOSDBitmap_HighSolution[i],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );


		}

               /*=======输出分别是1080I/720P和576I======================== */
		LayerVpParams.OutputRectangle.PositionX 	= PosXTemp;		
		LayerVpParams.OutputRectangle.PositionY 	= PosYTemp;
		LayerVpParams.OutputRectangle.Width 	= WidthTemp;
		LayerVpParams.OutputRectangle.Height	= HeightTemp;
		
		
		/*===============Open VP1================================*/
		  
             VPSource.SourceType = STLAYER_GRAPHIC_BITMAP; 
		VPSource.Palette_p   = NULL;
		VPSource.Data.BitMap_p= &g_TempOSDBitmap_HighSolution[i];

              CH_SwitchRestoreOsd(i);


		STLAYER_SetViewPortIORectangle(g_VPOSD_HighSolution[i], &LayerVpParams.InputRectangle, &LayerVpParams.OutputRectangle);
              STLAYER_EnableViewPortFilter(g_VPOSD_HighSolution[i],NULL);
	       /*改变VIERPORT数据源*/
		error=STLAYER_SetViewPortSource(g_VPOSD_HighSolution[i],&VPSource);
		if(error!=ST_NO_ERROR)
		{
			STTBX_Print(("STLAYER_SetViewPortSource()=%s",GetErrorText(error)));
			return TRUE;		
		}
              //STLAYER_SetViewPortParams(g_VPOSD_HighSolution[i],&LayerVpParams);

           /*释放相关内存*/
	
	    st_FreeBlockParas.PartitionHandle = AVMEMHandle[0];
	    if(g_OSDBitmap_HighSolution[i].Data1_p != NULL)
	    {
	       STAVMEM_FreeBlock(&st_FreeBlockParas,&g_OSDBitmap_BlockHandle[i]);
	    }
	    /*更新相应的内存数值*/
           memcpy(&g_OSDBitmap_HighSolution[i],&g_TempOSDBitmap_HighSolution[i],sizeof(STGXOBJ_Bitmap_t));
	    g_OSDBitmap_BlockHandle[i] = g_TempOSDBitmap_BlockHandle[i];
	   /********************************/
 
	}
  
	return ST_NO_ERROR;
}



/************************函数说明*******************************************/
/*函数名:  boolean CH_PutRectangle_TempHighSolution( int ri_SourcePosX, int ri_SourcePosY,int ri_Width,int ri_Height,U8* rup_pBMPData,int ri_DesPosX, int ri_DesPosY )*/
/*开发人和开发时间:                                                                                          */
/*函数功能描述:创建高清OSD菜单                                                              */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
 boolean CH_PutRectangle_TempHighSolution(int ri_index, int ri_SourcePosX, int ri_SourcePosY,int ri_Width,int ri_Height,U8* rup_pBMPData,int ri_DesPosX, int ri_DesPosY )
 {
      int i_ErrCode;
      STGXOBJ_Rectangle_t st_Rect;
      STGXOBJ_Bitmap_t    st_BMPT;
	  
      st_Rect.PositionX = ri_SourcePosX;
      st_Rect.PositionY = ri_SourcePosY;  
      st_Rect.Width = ri_Width;
      st_Rect.Height = ri_Height;

	  
      if(rup_pBMPData == NULL || ri_SourcePosX < 0 || ri_SourcePosY < 0 || ri_Width < 0 || ri_Height < 0)
      	{
           return true;
      	}
       memcpy((void*)&st_BMPT,(const void*)&g_TempOSDBitmap_HighSolution[0],sizeof(STGXOBJ_Bitmap_t));
      	st_BMPT.Pitch  = 1280*4;
	st_BMPT.Size1 = 1280*720*4;	
	st_BMPT.Width = 1280;
	st_BMPT.Height = 720;
	st_BMPT.Data1_p = rup_pBMPData;
	
     /*首先更新高清OSD*/
      i_ErrCode   =  STGFX_CopyArea(GFXHandle,&st_BMPT,&g_TempOSDBitmap_HighSolution[ri_index],&gGC,&st_Rect,ri_DesPosX,ri_DesPosY);
      i_ErrCode   =  STGFX_Sync(GFXHandle);
      /*再次更新标清OSD*/
      
     // i_ErrCode   =  STGFX_CopyArea(GFXHandle,&st_BMPT,&g_TempOSDBitmap_HighSolution[1],&gGC,&st_Rect,ri_DesPosX,ri_DesPosY);
     // i_ErrCode   =  STGFX_Sync(GFXHandle);

     /************************/
     return false;
	  
 }


/************************函数说明*******************************************/
/*函数名void CH_ChangeOSDIO_HighSolution(void)                                                  */
/*开发人和开发时间:                                                                                           */
/*函数功能描述:改变OSD Viewport                                                                       */
/*函数原理说明:                                                                                                      */
/*使用的全局变量、表和结构：                                                                 */
/*调用的主要函数列表：                                                                                 */
/*返回值说明：无                                                                                                  */
void CH_ChangeOSDIO_HighSolution(CH_RESOLUTION_MODE_e renum_mode,CH_VideoOutputAspect rch_Ratio,CH_VideoOutputMode rch_HDMode)
{
	ST_ErrorCode_t				error;
	STAVMEM_AllocBlockParams_t  	AllocBlockParams; /* Allocation param*/
	STLAYER_ViewPortParams_t		LayerVpParams;
	STLAYER_GlobalAlpha_t			GloabalAlpha;
	STLAYER_ViewPortSource_t		VPSource;
	
	int                                              PosXTemp;
	int                                              PosYTemp;
	int                                              WidthTemp;
	int                                              HeightTemp;

	 STGXOBJ_AspectRatio_t             AspectRatio;
	STVTG_TimingMode_t                 TimingMode[2];
	LAYER_Type                               LAYERType[2];
	CH_VideoOutputMode                  Mode[2];
	int                                             i;
           STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
       /*设置对应的GFX LAYER和输出模式*/
       LAYERType[0] = LAYER_GFX1;
	LAYERType[1] = LAYER_GFX2;

	Mode[0] = rch_HDMode; 
	Mode[1] = MODE_576I50;
	
      /******************************************/

	AspectRatio = CH_GetSTScreenARFromCH(rch_Ratio);

	for(i=0;i<2;i++)
	{
		TimingMode[i]=CH_GetSTTimingModeFromCH(rch_HDMode);
		switch(TimingMode[i])	
		{
		case STVTG_TIMING_MODE_1080I60000_74250:
			PosXTemp=1;
			PosYTemp=41;
			break;
		case STVTG_TIMING_MODE_1080I50000_74250_1: 
			PosXTemp=46-10-10-4;
			PosYTemp=25-3-5-2;
			break;
		case STVTG_TIMING_MODE_720P60000_74250:
			PosXTemp=1;
			PosYTemp=11;
			break;
		case STVTG_TIMING_MODE_720P50000_74250:
			PosXTemp=1+30-3-5-8;
			PosYTemp=11+6-10+1;
			break;
		case STVTG_TIMING_MODE_576P50000_27000: 
			PosXTemp=19-9;  
			PosYTemp=14-10+3;	 
			break;
		case STVTG_TIMING_MODE_576I50000_13500:
				
			PosXTemp=16+3-10; 
			PosYTemp=10+3-4; 
			break;
		default:
			PosXTemp=1;
			PosYTemp=11;
			break;  
		}

		if(LAYERType[i] == LAYER_GFX1)/*高清输出*/
		{
		       PosXTemp =  /*gVTGModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGModeParam.ActiveAreaWidth;
			HeightTemp = gVTGModeParam.ActiveAreaHeight;
		}
		else                                       /*标清输出*/
		{
		       PosXTemp =  /*gVTGAUXModeParam.ActiveAreaXStart*/0;
			PosYTemp =  /*gVTGAUXModeParam.ActiveAreaYStart*/0;
			WidthTemp  = gVTGAUXModeParam.ActiveAreaWidth;
			HeightTemp = gVTGAUXModeParam.ActiveAreaHeight;
			
		}	
			
		
		
		/*=======输入数据是720P格式======================== */
		if(renum_mode == CH_720X576_MODE)
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			LayerVpParams.InputRectangle.Width		= act_src_w;
			LayerVpParams.InputRectangle.Height 	= act_src_h;

		}
	      else
		{
			LayerVpParams.InputRectangle.PositionX	= PosXTemp;				
			LayerVpParams.InputRectangle.PositionY	= PosYTemp;
			LayerVpParams.InputRectangle.Width		= 1279;
			LayerVpParams.InputRectangle.Height 	= 719;


		}

               /*=======输出分别是1080I/720P和576I======================== */
		LayerVpParams.OutputRectangle.PositionX 	= PosXTemp;		
		LayerVpParams.OutputRectangle.PositionY 	= PosYTemp;
		LayerVpParams.OutputRectangle.Width 	= WidthTemp;
		LayerVpParams.OutputRectangle.Height	= HeightTemp;
		
		


		STLAYER_SetViewPortIORectangle(g_VPOSD_HighSolution[i], &LayerVpParams.InputRectangle, &LayerVpParams.OutputRectangle);

 
	}
  
	return ST_NO_ERROR;
}

/*======================= Bitmap Scaling Code =============================*/
/*                       位图放大缩小                                       */
void bmscaler(int dest_width,int dest_height,int source_width, int source_height,
			  U16 *DesBitmap, U16 *bitmap)
			  
{
	/*declare local variables*/
	U32 color;
	int error_term_x,error_term_y,source_x,source_y,screen_x,screen_y;
	int destination_width,destination_height;
	
	/*initialize local variables*/
	source_x=0;
	source_y=0;
	error_term_x=0;
	error_term_y=0;
	screen_x=0;	/*starting x coordinate to draw*/
	screen_y=0;	/*starting y coordinate to draw*/
	destination_width=dest_width;
	destination_height=dest_height;
	
	/*here we go, into the scaling stuff.  Pick which case we have and do it*/
	/*case #1: fatter and taller destination bitmap*/
	if((destination_width  > source_width) &&
		(destination_height > source_height))
	{
		do	/*destination fatter (x-axis) than source bitmap*/
		{
			error_term_x+=source_width;
			if(error_term_x > destination_width)
			{
				error_term_x-=destination_width;
				source_x++;
			}
			screen_x++;
			screen_y=0;
			source_y=0;
			do	/*destination taller (y-axis) than source bitmap*/
			{
				color=bitmap[source_y*source_width+source_x];
				if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/
					*(DesBitmap+screen_y*destination_width+screen_x)=color;
				error_term_y+=source_height;
				if(error_term_y > destination_height)
				{
					error_term_y-=destination_height;
					source_y++;
				}
				screen_y++;
			} while(screen_y < (destination_height));
		} while(screen_x < (destination_width));
	}
	else
		/*case #2: fatter and shorter destination bitmap*/
		if((destination_width > source_width) &&
			(destination_height < source_height))
		{
			do	/*destination fatter (x-axis) than source bitmap*/
			{
				error_term_x+=source_width;
				if(error_term_x > destination_width)
				{
					error_term_x-=destination_width;
					source_x++;
				}
				screen_x++;
				screen_y=0;
				source_y=0;
				do	/*destination shorter (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/	
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=destination_height;
					if(error_term_y > source_height)
					{
						error_term_y-=source_height;
						screen_y++;
					}
					source_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width));
		}
		/*case #3: skinnier and taller destination bitmap*/
		if((destination_width < source_width) &&
			(destination_height > source_height))
		{
			do	/*destination skinnier (x-axis) than source bitmap*/
			{
				error_term_x+=destination_width;
				if(error_term_x > source_width)
				{
					error_term_x-=source_width;
					screen_x++;
				}
				source_x++;
				screen_y=0;
				source_y=0;
				do	/*destination taller (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/		
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=source_height;
					if(error_term_y > destination_height)
					{
						error_term_y-=destination_height;
						source_y++;
					}
					screen_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width-2));
		}
		/*case#4: skinnier and shorter destination bitmap*/
		if((destination_width <= source_width) &&
			(destination_height <= source_height))
		{
			do	/*destination skinnier (x-axis) than source bitmap*/
			{
				error_term_x+=destination_width;
				if(error_term_x > source_width)
				{
					error_term_x-=source_width;
					screen_x++;
				}
				source_x++;
				screen_y=0;
				source_y=0;
				do	/*destination shorter (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/			
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=destination_height;
					if(error_term_y > source_height)
					{
						error_term_y-=source_height;
						screen_y++;
					}
					source_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width-2));
		}
		
		/*end of the four cases*/
}
#endif
