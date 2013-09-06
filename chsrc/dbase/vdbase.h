/*
 * VDBASE.H      ver 0.1
 *
 * (c) Copyright SGS-THOMSON Microelectronics Limited 1996. 
 *
 * Source file name : VDBASE.H
 * Author(s)    T.H.Thillai Rajan (tht.rajan@st.com)
 *
 * =======================
 * IMPROVEMENTS THOUGHT OF
 * =======================
 *
 * =====================
 * MODIFICATION HISTORY:
 * =====================
 *
 * Date        Modification                                                                             Initials
 * ----        ------------                                                                             --------
 * 07.09.96             CREATED                                  Author
 * 10-11-2003   changed                                          zxg
 * 15-06-2004   changed                                          yxl
 * 2005-02-     changed   形成一个NORMAL Release的数据库         yxl
						  将IPANEL HTML从BEIJING_DATABASE独立出来

  2005-08-07    changed   1)修改相关数据结构，形成由宏控制        yxl
							DVBC和DVBT兼容的数据库
						  2）删除数据库中与北京歌华特殊要求相关
						  的部分，形成长虹通用版数据库
 */
#ifndef __VDBASE_H__
#define __VDBASE_H__
/*#define      DATABASE_DEBUG */


#include "appltype.h"
#include "..\util\ch_time.h"
#if 1/*def NET_ENABLE*/
#include "../chdrv_net/include/CHDRV_NET.h"
#endif



#ifdef SUMA_SECURITY


#define START_E2PROM_SECURITYFLASHDATA 4559
#define E2PROM_SECURITYFLASHDATA_LEN 2


#define START_E2PROM_SECURITYDATA      6050
#define E2PROM_SECURITYDATA_LEN    2048
#define START_E2PROM_SECURITYDATA_SPARE      (START_E2PROM_SECURITYDATA+E2PROM_SECURITYDATA_LEN)
#define START_TSCAPTURE_PARALIST   4352
#endif

#define  NVM_APPLICATION_MEM_START_DFA       8100


#define IQPWM_INIT					                /*0x80 */0x00/*mjc change*/


#define MPEG1_VIDEO_STREAM              0x01
#define MPEG2_VIDEO_STREAM              0x02
#define MPEG1_AUDIO_STREAM              0x03
#define MPEG2_AUDIO_STREAM              0x04
#define H264_VIDEO_STREAM               27 /*2007-04-13 add this line*/

/*#ifdef CH_DVBC_PLATFORM ,yxl 2005-08-13 add this line,yxl 2005-09-09 cancel this line*/										
#define PRIVATE_DATA_STREAM             0x06
/*#endifyxl 2005-08-13 add this line,yxl 2005-09-09 cancel this line*/

/*#ifdef CH_DVBT_PLATFORM ,yxl 2005-08-13 add below lines,yxl 2005-09-09 cancel this line*/	
#define AC3_AUDIO_STREAM			0x81
#define AC3_AUDIO_STREAM_AUS		0x06
/*#endif end yxl 2005-08-13 add below lines,yxl 2005-09-09 cancel this line*/

#define INVALID_LINK                   -1


#define MAX_NETWORK_NAME_LENGTH         20



#define MAX_AUDIO_CHANNEL_PER_PROGRAM        8

#define NO_OF_BYTES_TILL_SECTION_LENGTH_FIELD   3
#define NO_OF_CRC_DATA_BYTES                                                    4
#define PROGRAM_NOT_FOUND_IN_DATABASE                   -1                    

#define MAX_ISO_639_LANGUAGE_CODE           3 


typedef enum
{
	NOT_BLOCK=0,
	BLOCK_R,           /*R Restricted 18+*/
	BLOCK_AV,          /*AV Adult Audience,Strong violence15+*/
	BLOCK_MA,          /*Mature Adult Audience 15+*/
	BLOCK_M,           /*Audience 15+*/
	BLOCK_PG,          /*Parental Guidance Recommended*/
	BLOCK_G,           /*General*/
	BLOCK_ALL        /*BLOCK ALL*/
}prog_parent_lock_t;
/*end 20050925 add for parent lock*/





typedef struct box_info_t
{
    WORD2SHORT stLastWatchingProg;/*关机前最后一次观看的节目*/
    WORD2SHORT stLastWatchingTvProg;/*关机前最后一次观看的电视节目*/
    WORD2SHORT stLastWatchingRadioProg;/*关机前最后一次观看的广播节目*/
    /*
     * stLastWatchingProg .     sLo16 - Program No (16)
     * stLastWatchingProg .     sHi16 - Transponder No (16)
     */

    INT iNoOfValidServices : 13;  /* 当前节目个数*/

    INT iNoOfValidXpdrs : 12;    /* 当前频点个数*/

    INT abiUnusedBits_WORD_1 : 7;

	INT iHeadServiceIndex : 13;  /* 节目链表头 */

	INT iTailServiceIndex : 13;   /* 节目链表尾 */

	INT abiUnusedBits_WORD_2:6;

  /*21*/  INT iHeadXpdrIndex  : 12;   /* 频点链表头 */

    INT iTailXpdrIndex  : 12;   /* 频点链表尾 */


/*24*/	UINT abiFlashValidity : 1;
					/* FLASH 状态 TRUE -  InValid
					  * FALSE - Valid
					*/

	UINT abiBoxPoweredState : 1;
				   /* 开关机状态
					* 0 - BOX_STANDBY
					* 1 - BOX_POWERED_ON
					*/

	UINT abiOutputDisplayFormat : 1;
				   /*宽高比输出
					*   0 - 4:3
					*   1 - 16:9
					*/
    
	UINT  abiLanguageInUse : 3;
					/*语言选择
					* Language Currently Used by the Application
					* max 8 languge
					*/

	UINT abiTVColor :3;
					/*输出制式选择
					 *  00 = PAL
					 *  01 = NTSC
					 *  10 = AUTO					
					*/
	
	UINT abiBoxLock:1;
	/*机顶盒锁定状态
	* 0 - LCOK OFF
	* 1 - LOCK ON
	*/

	UINT abiTVSaturation :8;
	/*亮度
	TV Saturation
	*/

	UINT abiTVBright:7;
	/*色度
	TV Bright
	*/

	UINT abiTVContrast:7;
	/*对比度
	TV Contrast
	*/



	U8	sta_DigitalType;

	S8  GlobalVolume;                      /*节目全局音量*/
 
#ifdef NAFR_KERNEL 

	SHORT    PinResetIndex;     
	U8              PinCard[6];
/*20060225 add PIN CODE Status*/
	U8         FirstPin;
    TDTDATE         FirstPinDate;
#endif




	int    SPDIFOUT;/*20060922 add for set SPDIFOUT 0,AC-3,1,PCM*/




	  CHDRV_NET_Config_t ipcfg;

	/*U8 NoUse[3];*/
	CH_VideoOutputType/*U8*/ VideoOutputType;/*0->RGB,1->yuv,2->CVBS,3->DVI*/
	CH_VideoOutputMode/*U8*/ VideoOutputMode;
						/*0->1080I60,1->1080I50,2->720P60,3->720P50,
						4->576P60,5->576P50,6->576I60,7->576I50,*/
	CH_VideoOutputAspect VideoOutputAspectRatio;
	CH_VideoARConversion VideoARConversion;/*yxl 2005-09-27 add this member*/

	CH_VideoOutputType SDVideoOutputType;/*stand for analog video output format of SD,TYPE_INVALID stand for no SD video output */ 
	CH_VideoOutputType HDVideoOutputType;/*stand for analog video output format of HD,TYPE_INVALID stand for no HD video output */ 
	CH_VideoOutputType HDDigitalVideoOutputType;/*stand for digital video output format of HD,TYPE_INVALID stand for no HD digital video output*/ 
	CH_VideoOutputMode SDVideoTimingMode;/*stand for video timing mode of SD*/ 
	CH_VideoOutputMode HDVideoTimingMode;/*stand for video timing mode of HD*/ 
       U8                            serach_tag;
	U8                          ucSoftwareVerMajor;     /* software version major (8)*/
	U8                          ucSoftwareVerMinor;     /* software version minor (4)*/
	U8                          upgrade;     /* 记录升级后第一次升级*/
	
} BOX_INFO_STRUCT;


typedef enum
{
	CH_DVBC=0,/*DVBC platform*/
	CH_DVBT   /*DVBT platform*/
}CH_Platform;

typedef struct  transponder_info_table 
{

	CH_Platform DatabaseType;/*数据库类型,CH_DVBC代表DVBC数据库,CH_DVBT代表DVBT数据库*/


	int   iTransponderFreq;        /* transponder freq in KHz */
	int	  iBandWidth;             /* band width in Mhz for DVBT ,no use for DVBC*/
	int   iSymbolRate;             /* symbol rate,no use for DVBt */
	BYTE  ucPwmVal;                /* PWM value to acheive the symbol rate ,no use for DVBT*/



	BYTE ucQAMMode;	/*no use for DVBT,0x00= not defined 
				0x01=16 QAM
				0x02=32 QAM
				0x03=64 QAM
				0x04=128 QAM
				 0x05=256 QAM*/
	   
	boolean abiIQInversion;/*no use for DVBT*/


} TRANSPONDER_INFO_STRUCT;




        

/*
 * bit variables in the ucStatusPolFlag (8bit) variable are defined as:
 */
#define IQ_STATUS_BIT_MASK        0xC0
#define IQ_STATUS_VALID_BIT_MASK  0x80
#define IQ_INV_BIT_MASK           0xC0
#define IQ_NON_INV_BIT_MASK       0x80
#define IMAGE_SIDE_BIT_MASK       0x20
#define POL_BIT_MASK              0x07	 /* org 0x03 Baisy*/



#define CHANGHONG_STOCK_PIDNUM 256       /*股票最大的PID个数*/
#define MAX_COMPONENT_NUM      500       /*COMPONENT最大个数*/



BOOLEAN  DVBDbaseInit ( void );
SHORT CH_GetsCurTransponderId(void);
SHORT CH_GetsCurProgramId(void);
SHORT CH_UpdatesCurTransponderId(SHORT Temp);


extern BOX_INFO_STRUCT	*pstBoxInfo;
extern opaque_t     idNvmBoxInfoId;



extern SHORT sCurTransponderId;        /*当前调谐频点索引*/
extern SHORT sCurProgramId;            /*当前服务索引**/



#endif      

