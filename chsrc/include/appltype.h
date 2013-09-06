/*
* APPLTYPE.H               ver 0.1
*
* (c) Copyright Highgate Worldwide & Co., 1998
*
* e-mail:     anbuhwc@md2.vsnl.net.in
*             sivahwc@giasmd01.vsnl.net.in
*
* Source file name : APPLTYPE.H
* Author(s)   Parivallal G
*             Sadeesh Kumar K
*             Vijayalakshmi C
*
* Purpose: All the application specific typedefs & enums are defined
*          here
*
* Original Work:  None
*
*
* =======================
* IMPROVEMENTS THOUGHT OF
* =======================
*
* =====================
* MODIFICATION HISTORY:
* =====================
*
* Date        Initials	Modification
* ----        --------	------------
* 20.05.98    GPV, KS,
*             CV       Created
*/
#ifndef  __APPLTYPE_H__

#define  __APPLTYPE_H__

/* general & explicit datatype definitions for interfaces */

#if 0 /*避免与其他地方的定义冲突 sqzow070925*/
#define  CHAR           signed char
#define  BYTE           unsigned char
#define  SHORT          signed short
#define  USHORT         unsigned short
#define  LONG           long
#define  ULONG          unsigned long
#define  INT            signed int
#define  UINT           unsigned int

#define BOOLEAN         CHAR
#else
#ifndef CHAR
#define  CHAR           signed char
#endif
#ifndef BYTE
#define  BYTE           unsigned char
#endif
#ifndef SHORT
#define  SHORT          signed short
#endif
#ifndef USHORT
#define  USHORT         unsigned short
#endif
#ifndef LONG
#define  LONG           long
#endif
#ifndef ULONG
#define  ULONG          unsigned long
#endif
#ifndef INT
#define  INT            signed int
#endif
#ifndef UINT
#define  UINT           unsigned int
#endif
#ifndef BOOLEAN
#define BOOLEAN         CHAR
#endif
#endif

#if 0 /*yxl 2004-11-07 modify this section*/
#define	SLOT_ID        signed char
#else
#define	SLOT_ID        short
#endif/*end yxl 2004-11-07 modify this section*/




/*zxg add for timer handle moudle*/
typedef enum 
{
	 EVENT_ID_NIT_CHANGE,
	 EVENT_ID_SOFTWARE_CHANGE,
	 EVENT_ID_SOFTWARE_CHANGE_FORCE,
	 EVENT_ID_START_NVOD_PLAY,/*NVOD预定节目到了*/		
	 EVENT_ID_END_NVOD_PLAY,  /*NVOD节目结束*/
	 EVENT_ID_EPGTIMER_PLAY_START,
     EVENT_ID_EPGTIMER_PLAY_END
}   EventIDInfo;

typedef struct NotifyEventTag
{
 EventIDInfo EventID;
 SHORT       EventPara;
}NotifyEventInfo;


#ifdef  NAFR_KERNEL 
/*add this on 050405*/
typedef enum
{  
STOP_MGCA,
STOP_PMT,	
START_MGCA,
START_MGCA_EPG,
START_PMT_NCTP,
START_PMT_UPDATE,/*add this on 050512*/
START_PMT_CTP
}PMTStaus;
#endif

typedef enum
{  
  NO_CARD,            /*没有卡*/
  ERROR_CARD,         /*无效智能卡*/  
  NO_SIGNAL,          /*没有信号*/
  SIGNAL_LOCK,        /*signal is locked ,yxl 2006-10-21 add this member*/
  NO_STB_CARD,        /*机卡不配对*/
  Geo_Blackout,
  NO_RIGHT,           /*没有权利*/
  NO_VEDIO_RIGHT,     /*没有VEDIO*/
  NO_AUDIO_RIGHT,     /*No audio*/
  NORMAL_STATUS,
  Parental_Control   /*父母控制add this on item on 050312*/    

}STBStaus;




typedef enum
{
  NEW_MAIL,
  PIN_RESET
}EMMStaus;

typedef struct
{
	EMMStaus CurEMMStaus;
	signed char MailIndex;
}EMMInfo;

/*************************/

/* keyboard selection enumeration */
enum
{
	REMOTE_KEYBOARD,
	FRONT_PANEL_KEYBOARD
};


/* --- */
typedef enum
{
	ELEMENT_FREE,
		ELEMENT_OCCUPIED,
		ELEMENT_DELETED
} element_status_t;

/* --- */
typedef enum
{
	NOT_TUNED,
		TUNED
} tuner_module_state_t;

/* --- */
typedef enum
{
	DBASE_WAITING_FOR_START_SIGNAL,
	DBASE_RUNNING 
} dbase_module_state_t;

#define  DEMUX_INVALID_PID       0x1FFF /*yxl 040615 add */

#if 1 /*yxl 2004-06-15 add this section*/
typedef	enum
{
	BUILDING_NIT,
	BUILDING_NIT_COMPLETED,	
    BUILDING_PAT,
	BUILDING_PMT,
	BUILDING_SDT,
	BUILDING_OTHER_SDT,
	TABLE_BUILDING_DONE,
	PROGRAM_SEARCH_DONE, /*yxl 2004-06-24 add this item,standfor successful find program*/
	PROGRAM_SEARCH_FAIL, /*yxl 2004-07-05 add this item,standfor not find program*/
	BUILDING_STOP,
	NO_USED_INFO
} dbase_table_constuction_state_t;
#endif /*end yxl 2004-06-15 add this section*/

/*yxl 2004-06-16 add this section*/
#define MILLI_DELAY(_ms_) { task_delay(_ms_ * ST_GetClocksPerSecondLow() /1000);}
/*end yxl 2004-06-16 add this section*/

/* --- */


/* --- */
typedef enum
{
	BOX_POWERED_ON,
	BOX_STANDBY
} box_state_t;

/*yxl 2005-09-03 add below section for 7710 HD video output*/
typedef enum
{
	TYPE_CVBS=0,
	TYPE_YUV,
	TYPE_RGB
#ifdef DVI_FUNCTION 
	,TYPE_DVI
#endif
#ifdef HDMI_FUNCTION 
	,TYPE_HDMI/*yxl  2006-03-29 add this line*/
#endif	
	,TYPE_UNCHANGED	
#ifdef MODEY_576I /*yxl 2006-06-26 add this  section*/
	,TYPE_SDYUV
#endif /*end yxl 2006-06-26 add this  section*/
	,TYPE_INVALID /*yxl 2007-03-05 add this line*/
}CH_VideoOutputType;



typedef enum
{
	ASPECTRATIO_4TO3=0,
	ASPECTRATIO_16TO9,
	ASPECTRATIO_UNCHANGED	
}CH_VideoOutputAspect;

typedef enum
{
	DENCMODE_PAL = 0,
	DENCMODE_NTSC,
	DENCMODE_UNCHANGED
}CH_VideoDencMode_t;

typedef enum
{
	MODE_1080I50=0,
	MODE_720P50,
	MODE_720P50_VGA,/*yxl 2005-12-15 add this line*/
	MODE_720P50_DVI,/*yxl 2006-03-27 add this line*/
	MODE_576P50,
	MODE_576I50,
	MODE_1080I60,
	MODE_720P60,
	MODE_UNCHANGED
}CH_VideoOutputMode;

typedef enum
{
	CONVERSION_FULLSCREEN=0,
	CONVERSION_LETTETBOX,
	CONVERSION_PANSCAN,
	CONVERSION_COMBINED,
	CONVERSION_UNCHANGED
}CH_VideoARConversion;


/* --- */
typedef enum
{
	        DEMUX_MODULE,
		   SECTION_FILTERING_MODULE,
		   MPEG_MODULE,
		   GPRIM_MODULE,
		   TUNER_MODULE,
		   DRIVERS_MODULE,
		   KEYBOARD_MODULE,
		   USIF_MODULE,
		   DBASE_MODULE,
		   PMT_TIMEOUT_MOUDLE ,
		   EPG_MOUDLE,
		   NVOD_MOUDLE,
		   MOSAIC_MOUDLE,
		   TIMER_MODULE,
		   STBSTATUS_MODULE,
		   EMM_MODULE,
             CA_MODULE,
			EXIT_STOCK_MODULE /*yxl 2005-01-28 add this line*/
		   
} reporting_module_t;

/* --- */
typedef enum
{
	START_DATA_BASE_BUILDING,
		STOP_DATA_BASE_BUILDING,
		DELETE_DATA_BASE_BASED_ON_TRANSPONDER_SLOT,
		START_ONLY_COLLECT_NIT ,
		MSG_TO_DBASE_CHECK_NIT_VERSION,
		MSG_TO_DBASE_SET_STATE_TO_WAIT_AFTER_COLLECT_NIT_VER,
                MSG_TO_DBASE_SAVE_DATA

		,START_DATA_BASE_BUILDING2 /*m0317*/
		,STOP_DATA_BASE_BUILDING2
		,PARSE_SDT_TABLE			/* LJiang080922 */

} usif_db_cmd_state_t;




typedef unsigned long  opaque_t;

/*
* union which converts two 16bit values into 32bit value or versa.
*/
typedef union
{
	
	unsigned int	uiWord32;
	
	struct
	{
		
		unsigned short   sLo16;
		unsigned short   sHi16;
		
	} unShort;
	
} WORD2SHORT;

typedef	union
{
	unsigned int	uiWord32;
	struct
	{
		unsigned char	ucByte0;	/* LSB */
		unsigned char	ucByte1;
		unsigned char	ucByte2;
		unsigned char	ucByte3;	/* MSB */
	} byte;
} WORD2BYTE;

typedef	union
{
	SHORT	sWord16;
	struct
	{
		unsigned char	ucByte0;	/* LSB */
		unsigned char	ucByte1; /* MSB */
	} byte;
} SHORT2BYTE;




typedef	enum
{
	SECTION_FILTER_FREE,					/* no section filter req for this slot */
	SECTION_FILTER_IN_USE,					/* filtering is	in progress */
	SECTION_FILTER_DONE,			/* filtering is	completed and it is ready to restart or	get killed */
	SECTION_FILTER_CRC_ERROR_DETECTED	/* crc error detected while filtering */

}CH6_FILTER_STATUS_t;

typedef enum
{
    FIRSTTIME_PMT_COME,
    NONFIRSTTIME_PMT_COME,
    PMT_UNALLOCTAED,
    PMT_ALLOCTAED,
    START_BUILDER_PMT,
    STOP_BUILDER_PMT
}t_pmt_monitor_type;

typedef enum
{
    CAT_UNALLOCATED,
    CAT_ALLOCATED,
    START_BUILDER_CAT,
    STOP_BUILDER_CAT
#if 1 /*add this on 031205*/
    ,FIRSTTIME_CAT_COME,
    NONFIRSTTIME_CAT_COME
#endif
}t_cat_monitor_type;



typedef	struct  usif_cmd_struct_tag
{
	
	reporting_module_t		from_which_module;
	
	union
	{
	/*
	* Keyboard module communication format
		*/
		struct
		{
#if 0
			BYTE		scancode;		/* key scan code */
#endif
			unsigned short  scancode;/* yxl 2004-05-26 modify this type, key scan code */
			char     device;			/* device type REMOTE or FRONT_PANEL keyboard */
			BOOLEAN  repeat_status;	/* TRUE if the key is pressed repeatedly */
			
		} keyboard;
		
		tuner_module_state_t	new_tuner_module_state;	/* TUNED or NOT_TUNED */
		
														/*
														* database module communication format
		*/
		struct
		{
			dbase_module_state_t					new_dbase_module_state;	/* status of the DBASE */
			dbase_table_constuction_state_t	table_under_construction;	/* which table is being built now */
		} dbase;	

/*zxg add for mosaic*/
		/*
         *mosaic module
         */
#if 0
        struct
        {
            MosaicBuidstatus mosaic_builder_status;
		}mosaic;
#endif
/*********************/

/*zxg add for timer handle moudle*/
		struct 
		{
          NotifyEventInfo NotifyInfo;
		}Timer;
/*add for STB Status display*/
        struct 
		{
         STBStaus CurSTBStaus;
		 SHORT    ProgID;/*和节目相关的状态的节目ID*/
		}STBInfo;
        
        EMMInfo  CurEmmInfo;

		 /*20050306 add*/
  		struct
		{
			int		iSectionSlotId;	/* reporting section id (0-31) */
			/*sf_slot_status_t	return_status; yxl 2004-06-08 modify this line to next line*/
			CH6_FILTER_STATUS_t return_status;			
		} sfilter;
/**/
 
	} contents;

} usif_cmd_t;

typedef	struct  db_cmd_struct_tag
{
	
	reporting_module_t		from_which_module;
	
	union
	{
		
	/*
	* section filtering module
		*/
		struct
		{
			int		iSectionSlotId;	/* reporting section id (0-31) */
			/*sf_slot_status_t	return_status; yxl 2004-06-08 modify this line to next line*/
			CH6_FILTER_STATUS_t return_status;			
		} sfilter;
		
		/*
		* usif module
		*/
		struct
		{
			int		iTransponderIndex;		/* tuned to which transponder */
			usif_db_cmd_state_t	cmd_received;	/* start or stop database building */
		} usif;


       /* usif module */
		struct
        {
            int     iCurProgIndex;      /* tuned to which program ID */
            int     iCurXpdrIndex;
            t_pmt_monitor_type cmd_received;   /* start or stop database building */
        }pmtmoni ;



       /* usif module */
		struct
        {
            t_pmt_monitor_type cmd_received;   /* start or stop database building */
        }catmoni ;


	} contents;
	
} db_cmd_t;


typedef enum
{
  EIT_ACTUAL_PF,
  EIT_OTHER_PF,
  EIT_ACTUAL_SCHEDULE,
  EIT_ACTUAL_ONE_FRE,
  EIT_OTHER_SCHEDULE,
  STOP_EIT_BUILDER,
  STOP_EIT_PF,
  PARSE_EIT_OTHER,
  NJ_START_SDTACTUAL_CHANNEL,
  NJ_STOP_SDTACTUAL_CHANNEL
}EitBuildType;      /*EIT表BUILDER类型*/

typedef enum
{
  START_BUILD_NVOD_NONIT,
  START_BUILD_NVOD_HAVENIT,
  STOP_NVOD_BUILD,
  STOP_NVOD_BUILD_REFEREVENT,
  STOP_NVOD_BUILD_BAT
}NvodBuildType;

typedef	struct	epg_cmd_struct_tag 
{

	reporting_module_t		from_which_module;

	union  
   {

		/*
		 * section filtering module
		 */
		struct 
      {
	        int		iSectionSlotId;	/* reporting section id	(0-31) */
			CH6_FILTER_STATUS_t return_status;	
	} sfilter;

		/*
		 * usif	module
		 */
		struct 
      {
	        int		iProgramIndex;		    /* witch program */
			EitBuildType	eit_builder_type;	/* eit build  */
	} usif;

	} contents;

} epg_cmd_t;/*EPG消息结构*/

typedef	struct	nvod_cmd_struct_tag 
{

	reporting_module_t		from_which_module;

	union  
   {

		/*
		 * section filtering module
		 */
		struct 
      {
	        int		iSectionSlotId;	/* reporting section id	(0-31) */
			CH6_FILTER_STATUS_t return_status;	
				
	} sfilter;

		/*
		 * usif	module
		 */
		struct 
      {	        
		 NvodBuildType	nvod_builder_type;	/* eit build  */
	} usif;

	} contents;

} nvod_cmd_t;/*nvod消息结构*/

typedef	struct	time_cmd_struct_tag 
{

	reporting_module_t		from_which_module;

	union  
   {

		/*
		 * section filtering module
		 */
		struct 
      {
	        int		iSectionSlotId;	/* reporting section id	(0-31) */
			CH6_FILTER_STATUS_t return_status;	
				
	} sfilter;

	

	} contents;

}time_cmd_t;

typedef enum
{
	APP_MODE_PLAY,
	APP_MODE_HTML,
	APP_MODE_NVOD,
	APP_MODE_EPG,
	APP_MODE_STOCK,
	APP_MODE_MOSAIC,
	APP_MODE_SET,
	APP_MODE_GAME,
	APP_MODE_LIST,
	APP_MODE_SEARCH                 /*搜索数据状态*/
}ApplMode;/*各种应用类型定义*/

#define  C_BAND_LNB_FREQ            5150000         /* (in KHz) LNB freq for C-Band */

/* polarization */
#define TUNER_POL_HORIZ          0x00
#define TUNER_POL_VERT           0x01
#define TUNER_POL_LEFT           0x02
#define TUNER_POL_RIGHT          0x03

#ifdef BUILD_FOR_DVBC
#define         DEFAULT_TRANSPONDER_FREQ   330000              /* in KHz */
#define         DEFAULT_SYMBOL_RATE        6875                /* in KS/Sec */
#define         DEFAULT_QAM_SIZE           2
#else
#define  DEFAULT_TRANSPONDER_FREQ   4000000              /* in KHz */
#define  DEFAULT_SYMBOL_RATE        28125                /* in KS/Sec */
#define  DEFAULT_QAM_SIZE       TUNER_POL_HORIZ
#define  DEFAULT_POLARIZATION       TUNER_POL_HORIZ
#define  DEFAULT_LNB_FREQ           C_BAND_LNB_FREQ
#define  DEFAULT_LNB_SOURCE         LNB1
#endif


#define START_E2PROM_CA                  4520   /*用于CA*/
#ifdef STBID_FROM_E2PROM
#define START_E2PROM_STBID             4540	                  /*用于机号*/
#endif

#define ADD_SMART_CONTROL   1

#endif   /* __APPLTYPE_H__  */
