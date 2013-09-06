/*
  Name:initterm.h
  Description:header file of system initialize section and
	some global variable for changhong 7710 platform
  Authors:yxl
  Date          Remark
  2005-06-24    Created

*/


#include "stpio.h"
#include "stcommon.h"
#include "stapp_os.h"/**/
#include "stavmem.h"
/*#include "staudlt.h"*/
#include "staud.h"
#include "stboot.h"
#include "stclkrv.h"
/*#include "stcount.h"*/
#include "stdenc.h"
#include "ste2p.h"
#include "stevt.h"
#include "stflash.h"
#include "sti2c.h"
#include "stpti.h"
/*#include "stpwm.h"*/
#include "stsmart.h"
#include "sttbx.h"
#include "stuart.h"
#include "stvid.h"
#include "stvtg.h"
#include "stblast.h"
#include "stclkrv.h"

#include "sttuner.h"
#include "stvout.h"
#include "stvmix.h"
#include "stlayer.h"
#include "stgfx.h"
#include "stgxobj.h"
#include "stsys.h"
#include "stfdma.h"
#include "stmerge.h"
#include "sthdmi.h"

#include "graphicbase.h"
#ifndef __INITTERM_H_
#define __INITTERM_H_

#define GLOBAL_VIEWPORT_ALPHA 103 /*yxl 2006-03-20 add this macro for control whole viewport alpha,
									128 is opaque,98 */


typedef struct CH_ColorYUVValue_t_s
{
	U8 Y;
	U8 Cr;
	U8 Cb;
}CH6_ColorYUVValue_t;


/*yxl 2005-09-11 add belowsection*/
typedef enum
{
    VTG_MAIN = 0,
    VTG_AUX = 1
} VTG_Type;

typedef enum
{
    VOUT_MAIN = 0,
    VOUT_AUX = 1,
	VOUT_HDMI = 2
} VOUT_Type;


typedef enum
{
    VMIX_MAIN = 0,
    VMIX_AUX = 1
} VMIX_Device;
/*end yxl 2005-09-11 add belowsection*/

typedef enum
{

	VID_MPEG2 = 0,
       VID_H=1,
       VID_UNKNOW=2
} VID_Device;



/*global variable*/


extern ST_Partition_t* InternalPartition;
extern ST_Partition_t* NcachePartition;
extern ST_Partition_t* SystemPartition;
extern ST_Partition_t* CHSysPartition;/*yxl 2006-04-12 add this line*/

extern ST_Partition_t 	*gp_appl2_partition;

extern ST_DeviceName_t PIO_DeviceName[]; 
extern ST_DeviceName_t UART_DeviceName[]; 


extern STPTI_Buffer_t VIDEOBufHandleTemp;/*20051017 add for get video bufhandle*/
extern ST_ErrorCode_t VideoGetWritePtrFct(void * const Handle_p, void ** const Address_p);
extern void VideoSetReadPtrFct(void * const Handle_p, void * const Address_p);
#if 1
enum
{
	ASC3_RXD_PIO_PORT=4,
	ASC3_TXD_PIO_PORT=4
		
};
#define ASC2_RXD_BIT PIO_BIT_3;
#define ASC2_TXD_BIT PIO_BIT_4;

#else

enum
{
	ASC3_RXD_PIO_PORT=5,
	ASC3_TXD_PIO_PORT=5,
	ASC1_RXD_PIO_PORT=1,
	ASC1_TXD_PIO_PORT=1
		
};
#define ASC3_RXD_BIT PIO_BIT_5
#define ASC3_TXD_BIT PIO_BIT_4
#define ASC1_RXD_BIT PIO_BIT_1
#define ASC1_TXD_BIT PIO_BIT_0
#endif

extern ST_DeviceName_t EVTDeviceName; 
extern STEVT_Handle_t EVTHandle;

/*extern ST_DeviceName_t PWMDeviceName;*/
extern ST_DeviceName_t CLKRVDeviceName;
extern STCLKRV_Handle_t CLKRVHandle;

#define I2C_BACK_BUS 0 /*yxl 2005-07-30 add this line*/ 
extern ST_DeviceName_t I2C_DeviceName[];

extern STAVMEM_PartitionHandle_t AVMEMHandle[2];
extern STFLASH_Handle_t FLASHHandle;
extern STE2P_Handle_t hE2PDevice; 

extern STTUNER_Handle_t TunerHandle;


extern ST_DeviceName_t DENCDeviceName;
extern STDENC_Handle_t DENCHandle;

#if 0 /*yxl 2005-09-11 modify this section*/
extern ST_DeviceName_t VTGMainDeviceName;
extern STVTG_Handle_t VTGHandle;
#else
extern ST_DeviceName_t VTGDeviceName[2];
extern STVTG_Handle_t VTGHandle[2];
#endif 

extern ST_DeviceName_t AUDDeviceName;

extern ST_DeviceName_t LAYER_DeviceName[];
extern STLAYER_Handle_t LAYERHandle[7];
typedef enum /*yxl 2005-09-18 add typedef*/
{
 	LAYER_VIDEO1=0, 
	LAYER_VIDEO2,		
	LAYER_GFX1,
	LAYER_GFX2,
	LAYER_GFX3,
	LAYER_GFX4,
}LAYER_Type;/*yxl 2005-09-18 add "LAYER_Type"*/

enum
{
    LAYER_STILL=0,
	LAYER_VIDEO,
	LAYER_OSD

};



#if 0 /*yxl 2005-09-11 modify this section*/
extern ST_DeviceName_t VOUTMainDeviceName;
extern STVOUT_Handle_t VOUTHandle;
#else
extern ST_DeviceName_t VOUTDeviceName[3];
extern STVOUT_Handle_t VOUTHandle[3];
#endif/*end yxl 2005-09-11 modify this section*/


extern ST_DeviceName_t VMIXDeviceName[2];/*yxl 2005-09-20 add this line*/

#if 0 /*yxl 2005-09-11 modify this section*/
extern STVMIX_Handle_t VMIXHandle;
#else
extern STVMIX_Handle_t VMIXHandle[2];
#endif/*end yxl 2005-09-11 modify this section*/

extern STBLAST_Handle_t BLASTRXHandle;
enum
{
	BLAST_RXD_PIO=3,
	BLAST_TXD_PIO=3
};
#define BLAST_RXD_BIT		PIO_BIT_3



extern semaphore_t* gpSemBLAST;/*用于遥控信号*/

extern ST_DeviceName_t BLITDeviceName;

extern STGFX_Handle_t GFXHandle;



extern ST_DeviceName_t PTIDeviceName;
extern STPTI_Handle_t PTIHandle;
/*extern STPTI_Handle_t PTIHandleTemp;yxl 2005-07-26 add this variable*/
extern STPTI_Slot_t VideoSlot,AudioSlot,PCRSlot;

extern STAUD_Handle_t AUDHandle;
extern STVID_Handle_t VIDHandle[2];
extern STVID_ViewPortHandle_t VIDVPHandle[2][2];
extern STVID_ViewPortHandle_t VIDAUXVPHandle;/*yxl 2005-09-11 add this variable*/

extern ST_ClockInfo_t gClockInfo;


extern CH_ViewPort_t CH6VPOSD;
extern CH_ViewPort_t CH6VPSTILL;






extern STVTG_ModeParams_t gVTGModeParam;

extern STVTG_ModeParams_t gVTGAUXModeParam;/* yxl 2005-09-11 add this line*/

extern STGFX_GC_t gGC;

/*yxl 2004-09-24 add this varaiable for OSD LAYER AND STILL LAYER viewport parameters*/
extern CH6_ViewPortParams_t* pStillVPParams;
extern CH6_ViewPortParams_t* pOSDVPMParams;
extern CH6_ViewPortParams_t* pOSDVPPParams;

/*end yxl 2004-09-24 add this varaiable for OSD LAYER AND STILL LAYER viewport parameters*/



/*yxl 2004-07-14 add this section for PAL,NTSC change*/

enum
{
	MODE_PAL=0,
	MODE_NTSC
};



#if 1
extern U8    gTimeMode; 
#endif
/*end yxl 2004-07-14 add this section for PAL,NTSC change*/


/*yxl 2004-09-22 add this section for pal mode and RGB mode conversion test*/

	
extern ST_DeviceName_t AVMEMDeviceName[];

/*end yxl 2004-09-22 add this section for pal mode and RGB mode conversion test*/



extern BOOL GlobalInit(void);

void CH_Appl2Mem_init(void);

extern ST_ErrorCode_t VID_ChangeAspectRatio(STGXOBJ_AspectRatio_t *Aspect_p);/*zc added for SYSset 080303*/

#endif
#ifdef CH_MUTI_RESOLUTION_RATE
typedef enum 
{
    CH_OLD_MOE,                          /*原有的长虹显示驱动接口*/
    CH_720X576_MODE,
    CH_1280X720_MODE,
}CH_RESOLUTION_MODE_e;

extern void       CH_CreateOSDViewPort_HighSolution(void);
extern   boolean CH_PutRectangle_HighSolution( int ri_SourcePosX, int ri_SourcePosY,int ri_Width,int ri_Height,U8* rup_pBMPData,int ri_DesPosX, int ri_DesPosY );
extern  boolean CH_ClearFUll_HighSolution( void );
#endif


