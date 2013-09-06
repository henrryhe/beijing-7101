/* =========================================================== *\
	Title:				STB厂商和CAS系统末端，智能卡公用的数据定义，类型定义等
	Version:			3.0.0.0
	Create Date:		2003.07.28
	Author:				GZY
	Description:		此文件符合ANSI C的语法	
\* =========================================================== */
#ifndef _PUB21EX_ST_H
#define _PUB21EX_ST_H
#define  INCLUDE_ADDRESSING_APP 1  /*OSD和邮件模块*/
#define  INCLUDE_IPPV_APP       1  /*IPPV模块*/
#define  INCLUDE_LOCK_SERVICE   0  /*VOD点播模块*/
#ifdef  __cplusplus
extern "C" {
#endif
#define true           1                                          
#define false          0   

/*所有函数调用的指针指向的空间由调用的管理*/
#define		OUT			    /*被调用初始化并填入内容填充内容*/
#define		IN				/*调用者初始化并填入内容填充内容*/						
#define		INOUT			/*调用者有输入，被调用者有输出*/	

#define		ULONG			unsigned  long   
#define		WORD			unsigned  short  
#define		BYTE			unsigned  char 
#ifndef     bool
	#define		bool			BYTE
#endif
#define		TFDATE			WORD					
#define		TFDATETIME		ULONG
#ifndef		NULL
#define		NULL			0
#endif

/*私有表PID List_Management*/
#define		TFCAS_LIST_OK					0x00
#define		TFCAS_LIST_FIRST				0x01	
#define		TFCAS_LIST_ADD					0x02
#define		TFCAS_LIST_UPDATE				0x03


/*机顶盒CAS的限制*/
#define		TFCA_MAXLEN_PROGRAMNUM			4			/*一个控制字最多解的节目数*/
#define		TFCA_MAXLEN_ECM					8			/*同时接收处理的ECMPID的最大数量*/

/*智能卡的限制*/
#define     TFCA_MAXLEN_PRICE				2			/*最多的IPPV价格个数*/
#define     TFCA_MAXLEN_OPERATOR			4			/*最多的运营商个数*/
#define		TFCA_MAXLEN_PINCODE		   		6			/*PIN码的长度*/     
#define		TFCA_MAXLEN_ACLIST				18			/*智能卡内保存每个运营商的AC的个数*/
#define     TFCA_MAXLEN_SLOT				20			/*卡存储的最大钱包数*/
#define		TFCA_MAXLEN_TVSPRIINFO			20			/*运营商私有信息*/
#define		TFCA_MAXLEN_IPPVP				300			/*智能卡保存最多IPPV节目的个数*/
#define		TFCA_MAXLEN_ENTITL				300			/*智能卡保存最多授权产品的个数*/
	
/*邮件*/
#define		TFCA_MAXLEN_EMAIL_TITLE			30			/*邮件标题的长度*/
#define		TFCA_MAXLEN_EMAIL				100			/*机顶盒保存的最大邮件个数*/
#define		TFCA_MAXLEN_EMAIL_CONTENT		160			/*邮件内容的长度*/
/*邮件图标显示方式*/
#define		Email_IconHide					0x00		/*隐藏邮件通知图标*/
#define		Email_New						0x01		/*新邮件通知，显示新邮件图标*/
#define		Email_SpaceExhaust				0x02		/*有新邮件，但磁盘空间不够；图标闪烁。*/

/*OSD的长度*/
#define		TFCA_MAXLEN_OSD					180			/*OSD内容的最大长度*/
/*OSD类型*/
#define		OSD_TOP							0x01		/*OSD风格：显示在屏幕上方*/
#define		OSD_BOTTOM						0x02		/*OSD风格：显示在屏幕下方*/

/*节目无法播放的提示*/
#define		MESSAGE_CANCEL_TYPE				0x00		/*取消当前的显示*/
#define		MESSAGE_BADCARD_TYPE			0x01		/*无法识别卡，不能使用*/
#define		MESSAGE_EXPICARD_TYPE			0x02		/*智能卡已经过期，请更换新卡*/
#define		MESSAGE_INSERTCARD_TYPE			0x03		/*加扰节目，请插入智能卡*/
#define		MESSAGE_NOOPER_TYPE				0x04		/*卡中不存在节目运营商*/
#define		MESSAGE_BLACKOUT_TYPE			0x05		/*条件禁播*/
#define		MESSAGE_OUTWORKTIME_TYPE		0x06		/*不在工作时段内*/
#define		MESSAGE_WATCHLEVEL_TYPE			0x07		/*节目级别高于设定观看级别*/
#define		MESSAGE_PAIRING_TYPE			0x08		/*节目要求机卡对应*/
#define		MESSAGE_NOENTITLE_TYPE			0x09		/*没有授权*/
#define		MESSAGE_DECRYPTFAIL_TYPE		0x0A		/*节目解密失败*/
#define		MESSAGE_NOMONEY_TYPE			0x0B		/*卡内金额不足*/
#define     MESSAGE_ERRREGION_TYPE          0x0C		/*区域不正确*/
/*节目购买阶段提示*/
#define		IPPV_FREEVIEWED_SEGMENT			0x00        /*免费预览阶段，是否购买*/
#define		IPPV_PAYEVIEWED_SEGMENT			0x01        /*收费阶段，是否购买*/


/************************************************************************/
/*                       Product,IPPV相关                               */
/************************************************************************/
/*IPPV价格类型*/
#define TFCA_IPPVPRICETYPE_TPPVVIEW						0X0
#define TFCA_IPPVPRICETYPE_TPPVVIEWTAPING				0X1
/*IPPV节目的状态*/
#define TFCA_IPPVSTATUS_TOKEN_BOOKING					0x01
#define TFCA_IPPVSTATUS_TOKEN_VIEWED					0x03

/*--------------------------------返回值定义-------------------------------------------------------------*/
#define TFCAS_OK							0x00
#define TFCAS_UNKNOWN						0x01	/*- 未知错误*/
#define TFCAS_POINTER_INVALID				0x02	/*- 指针无效*/
#define TFCAS_CARD_INVALID					0x03	/*- 智能卡无效*/
#define TFCAS_PIN_INVALID					0x04	/*- PIN码无效*/
#define	TFCAS_PIN_LOCK						0x05	/*- PIN码锁定*/
#define TFCAS_DATASPACE_SMALL				0x06	/*- 所给的空间不足*/
#define TFCAS_CARD_PAIROTHER				0x07    /*- 智能卡已经对应别的机顶盒*/
#define TFCAS_DATA_NOT_FIND					0x08    /*- 没有找到所要的数据*/
#define TFCAS_PROG_STATUS_INVALID			0x09    /*- 要购买的节目状态无效*/
#define TFCAS_CARD_NO_ROOM					0x0A    /*- 智能卡没有空间存放购买的节目*/
#define TFCAS_WORKTIME_INVALID				0x0B    /*- 设定的工作时段无效 0～24*/
#define TFCAS_IPPV_CANNTDEL					0x0C    /*- IPPV节目不能被删除*/
#define TFCAS_CARD_NOPAIR					0x0D    /*- 智能卡没有对应任何的机顶盒*/
#define TFCAS_WATCHRATING_INVALID			0x0E    /*- 设定的观看级别无效 4－18*/
/*-----------------------------------信号量定义  各操作系统不一样 -----------------------------------*/

typedef ULONG  TFCA_Semaphore ;

typedef struct _TFCAOperatorInfo{
	char	m_szTVSPriInfo[TFCA_MAXLEN_TVSPRIINFO+1];   /*运营商私有信息*/
}STFCAOperatorInfo;

typedef struct _TFCASServiceInfo{
	WORD	m_wEcmPid;									
	BYTE	m_byServiceNum;
	WORD 	m_wServiceID[TFCA_MAXLEN_PROGRAMNUM];
}STFCASServiceInfo;

#if INCLUDE_IPPV_APP 
	typedef struct _TFCASTVSSlotInfo{
		WORD	m_wCreditLimit;					    /*信用度*/
		WORD	m_wBalance;						    /*已花的点数*/
	}STFCATVSSlotInfo;

	typedef struct _TFCAIPPVPrice{
		BYTE	m_byPriceCode;	
		WORD	m_wPrice;		
	}STFCAIPPVPrice; 

	typedef struct _TFCAIppvBuyInfo{
		WORD			m_wTVSID;					/*运营商ID*/
		BYTE			m_bySlotID;					/*钱包ID*/
		ULONG 			m_dwProductID;               /*节目的ID*/
		BYTE			m_byPriceNum;				/*节目价格个数*/
		STFCAIPPVPrice  m_Price[TFCA_MAXLEN_PRICE]; /*节目价格*/
		TFDATE			m_wExpiredDate;				/*节目过期时间*/
	}STFCAIppvBuyInfo;

	typedef struct _TFCAIppvfo{
		ULONG 			m_dwProductID;               /*节目的ID*/
		BYTE			m_byBookEdFlag;				/*产品状态*/ /*BOOKING VIEWED EXPIRED*/ 
		BYTE			m_bCanTape;					/*是否可以录像，1：可以录像；0：不可以录像*/
		WORD			m_wPrice;					/*节目价格*/
		TFDATE			m_wExpiredDate;				/*节目过期时间*/
	}STFCAIppvInfo;
#endif

typedef struct _TFCAEntitle{
	ULONG 		m_dwProductID;
	BYTE		m_bCanTape;
	TFDATE		m_tExpriData;
}STFCAEntitle;

typedef struct _TFCAEntitles{
	WORD			m_wProductCount;
	STFCAEntitle	m_Entitles[TFCA_MAXLEN_ENTITL];
}STFCAEntitles;

#if INCLUDE_ADDRESSING_APP
	typedef struct _TFCAEmailHead{
		ULONG		m_dwActionID;            /*Email ID  */
		BYTE		m_bNewEmail;            /*0 不是新邮件 1是新邮件*/
		char   		m_szEmailHead[TFCA_MAXLEN_EMAIL_TITLE+1];/*邮件标题，最长为30*/
		WORD 		m_wImportance;			/*重要性, 0：普通，1：重要*/
		ULONG		m_tCreateTime;          /*EMAIL创建的时间*/
	}STFCAEmailHead;

	typedef struct _TFCAEmailContent{
		char		m_szEmail[TFCA_MAXLEN_EMAIL_CONTENT+1];	/*Email的正文*/
	}STFCAEmailContent;
#endif

#ifdef  __cplusplus
}
#endif

#endif		


