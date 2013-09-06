/******************************************************************************

  Copyright (C), 2001-2011, iPanel Co., Ltd.

 ******************************************************************************
  File Name     : stb_ca_app.h
  Version       : Initial Draft
  Author        : huzh
  Created       : 2009/6/1
  Last Modified :
  Description   : CA上层应用定义头文件，提供给iPanel中间件使用
  Function List :
  History       :
    Modification: Created file

******************************************************************************/
#ifndef __STB_CA_APP_H__
#define __STB_CA_APP_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/*******************************************************************************
*
*     常量定义
*
*******************************************************************************/
/* canal CA最多库个数 */
#define STB_CA_MaxLib                     8

/* 智能卡产品代码长度 */
#define STB_CA_CardProductCodeLength      2

/* 智能卡序列号长度 */
#define STB_CA_CardSerialNumberLength     6

/* 位掩码定义 */
#define STB_CA_BitMask( n )               ( 1U << (n) )

/*******************************************************************************
*
*     类型定义
*
*******************************************************************************/
typedef char            INT8;  
typedef char            CHAR;
typedef unsigned char   UINT8; 
typedef signed short    INT16; 
typedef unsigned short  UINT16;
typedef signed long     INT32; 
typedef unsigned long   UINT32;
//typedef unsigned int    BOOL;

typedef void            VOID;


/*******************************************************************************
*
*     数据结构定义
*
*******************************************************************************/
/* 函数返回状态码 */
typedef enum tagSTB_CA_STATUS_CODE
{
   STB_CA_Ok,
   STB_CA_SysError,
   STB_CA_HWError,
   STB_CA_InvalidParam,
   STB_CA_NotFound,
   STB_CA_AlreadyExist,
   STB_CA_NotInitialized,
   STB_CA_Idling,
   STB_CA_Empty,
   STB_CA_Running,
   STB_CA_NotRunning,
   STB_CA_NoResource,
   STB_CA_NotReady,
   STB_CA_LockError,
   STB_CA_NotEntitled,
   STB_CA_NotAvailable,
   STB_CA_UNKnown
}STB_CA_STATUS_CODE_E;

/* 智能卡状态码 */
typedef enum tagSTB_CA_CARD_REPORT
{
   STB_CA_CardOk,
   STB_CA_CardNotFound,
   STB_CA_CardBadParam,
   STB_CA_CardNoAnswer,
   STB_CA_CardFailure
}STB_CA_CARD_REPORT_E;

/* 智能卡类型码 */
typedef enum tagSTB_CA_CARD_TYPE
{
	STB_CA_Unknown		= 0x00,
	STB_CA_Mediaguard	= 0x01,
	STB_CA_Multiple		= 0x02
} STB_CA_CARD_TYPE_E;

typedef enum tagSTB_CA_CARD_APPID
{
	STB_CA_IDUnknown,
	STB_CA_IDMediaguard
} STB_CA_CARD_APPID_E;

/* 事件ID */
typedef enum tagSTB_CA_EVENTCODE
{
   STB_CA_EvSysError,
   STB_CA_EvNoResource,
   STB_CA_EvDumpText,
   STB_CA_EvMsgNotify,
   STB_CA_EvClose,
   STB_CA_EvHalted,
   STB_CA_EvExtract,
   STB_CA_EvBadCard,
   STB_CA_EvUnknown,
   STB_CA_EvReady,
   STB_CA_EvPassword,
   STB_CA_EvCmptStatus,
   STB_CA_EvRejected,
   STB_CA_EvAcknowledged,
   STB_CA_EvContent,
   STB_CA_EvAddFuncData,
   STB_CA_EvRating,
   STB_CA_EvUsrNotify,
   STB_CA_EvUpdate,
   STB_CA_EvOperator,
   STB_CA_EvGetId,
   STB_CA_EvNewMail,
   STB_CA_EvResetPwd,
   STB_CA_EvPairing
}STB_CA_EVENTCODE_E;

/* 机卡配对检测结果码 */
typedef enum tagSTB_CA_EV_PAIRING_EXCODE
{
    STB_CA_Pair_Yes,    /* 机卡已经匹配 */
    STB_CA_Pair_No      /* 机卡不匹配 */
}STB_CA_EV_PAIRING_EXCODE_E;

/* 密码命令相关的result code */
typedef enum tagSTB_CA_EV_PWD_EXCODE
{
   STB_CA_PwdNoCard,
   STB_CA_PwdHWFailure,
   STB_CA_PwdInvalid,
   STB_CA_PwdReset,
   STB_CA_PwdValid
}STB_CA_EV_PWD_EXCODE_E;

/* 有授权的result code */
typedef enum tagSTB_CA_EV_ACK_EXCODE
{
   STB_CA_CMAck,
   STB_CA_CMOverdraft,
   STB_CA_CMPreview,
   STB_CA_CMPPV                   = STB_CA_BitMask(15)
}STB_CA_EV_ACK_EXCODE_E;

/* 无授权的result code */
typedef enum tagSTB_CA_EV_REJECTED_EXCODE
{
   STB_CA_CMCardFailure           = STB_CA_BitMask(0),
   STB_CA_CMNANoRights            = STB_CA_BitMask(1),
   STB_CA_CMNAExpired             = STB_CA_BitMask(2),
   STB_CA_CMNABlackout            = STB_CA_BitMask(3),
   STB_CA_CMNARating              = STB_CA_BitMask(4),
   STB_CA_CMNAPPPV                = STB_CA_BitMask(5),
   STB_CA_CMNAIPPV                = STB_CA_BitMask(6),
   STB_CA_CMNAIPPT                = STB_CA_BitMask(7),
   STB_CA_CMNAMaxShot             = STB_CA_BitMask(8),
   STB_CA_CMNANoCredit            = STB_CA_BitMask(9),
   STB_CA_CMNAOthers              = STB_CA_BitMask(10)
}STB_CA_EV_REJECTED_EXCODE_E;

/* 运营商信息获取操作类型 */
typedef enum tagSTB_CA_OP_TYPE
{
   STB_CA_OP_GET_ONE,      /* 获取指定OPI的运营商信息 */
   STB_CA_OP_GET_FIRST,    /* 获取第一个运营商信息 */
   STB_CA_OP_GET_NEXT      /* 获取下一个运营商信息 */
}STB_CA_OP_TYPE_E;

/* 成人级检测域的有效性 */
typedef enum tagSTB_CA_RATING_VALID
{
	STB_CA_Rating			= 0x01, /* 当前检测的是rating */
	STB_CA_RatingCheck	    = 0x02  /* 当前检测的是ratingCheck */
} STB_CA_RATING_VALID_E;


/* CA初始化配置 */
typedef  struct  tagSTB_CA_CONFIG
{
   BOOL                 bCheckPairing;      /* 是否需要检查机卡配对 */
   BOOL                 bCheckBlackList;    /* 是否检查黑名单 */
   BOOL                 bCardRatingBypass;  /* 是否不需要成人级 */
}STB_CA_CONFIG_S;

/* CA状态 */
typedef  struct  tagSTB_CA_STATUS
{
   BOOL                 bCAStarted; /* CA状态 TRUE表示CA已经启动 FALSE表示未启动*/
   BOOL                 bIsPaired;  /* 机卡配对状态,TRUE表示已经配对,仅当初始
                                       化为要求检测机卡配对时,该值有效 */
   STB_CA_CARD_REPORT_E enScStatus; /* 智能卡状态 */
}STB_CA_STATUS_S;

/* 运营商信息 */
typedef struct tagSTB_CA_OPERATOR
{
   UINT16               uwOPI;            /* 运营商ID */
   UINT16               uwDate;           /* 过期日期 */
   UINT8                aucName[16];       /* 运营商名称 */
   UINT8                aucOffers[8];      /* 授权号数组 具体含义需参考运营商规定 */
   UINT8                ucGeo;            /* 区域码 */
   UINT8                aucReserved[3];   /* 保留未用 */
} STB_CA_OPERATOR_S;

/* canal CA的版本信息 */
typedef struct tagSTB_CA_REVISION
{
   CHAR         Name[24];       /* CA的库名 */
   CHAR         Date[9];        /* 库创建日期 dd/mm/yy */
   CHAR         Release[15];    /* 库版本号 */
} STB_CA_REVISION_S;

/* canal CA的ID信息结构 */
typedef struct tagSTB_CA_ID
{
   STB_CA_REVISION_S    astVer[STB_CA_MaxLib];  /* 库版本信息 */
   CHAR                 cCopyright[48];         /* 授权信息 */
} STB_CA_ID_S;

/* 智能卡应用数据结构 */
typedef struct tagSTB_CA_CARD_APP
{
   UINT8        ID;             /* 应用ID 参考tagSTB_CA_CARD_APPID */
   UINT8        Major;          /* 主版本号 */
   UINT8        Minor;          /* 副版本号 */
   UINT8        ucReserved;     /* 保留未用 */
} STB_CA_CARD_APP_S; 

/* 智能卡内容结构 */
typedef struct tagSTB_CA_CARD_CONTENT
{
   UINT8              ucProductCode[STB_CA_CardProductCodeLength];    /* 产品代码 */
   UINT8              ucSerialNumber[STB_CA_CardSerialNumberLength];  /* 序列号 */
   UINT8              ucApp;       /* 智能卡应用个数，当前值为1 */
   UINT8              ucType;      /* 智能卡类型 参考 tagSTB_CA_CARD_TYPE */
   UINT16             uwReserved;  /* 保留未用 */
   STB_CA_CARD_APP_S* pstAppData;  /* 智能卡应用数据 */
} STB_CA_CARD_CONTENT_S;

/* 成人级结构 */
typedef struct tagSTB_CA_RATING
{
   UINT8               ucValid;        /* 参考tagSTB_CA_RATING_VALID */
   UINT8               ucRating;       /* 成人级值 */
   UINT8               ucIsChecked;    /* 是否检测成人级 */
   UINT8               ucReserved;     /* 保留未用 */
} STB_CA_RATING_S;

/* 附加功能事件对应的运营商数据信息 */
typedef struct tagSTB_CA_AFD
{
   UINT16       uwOPI;                  /* 运营商ID */
   UINT8        aucData[10];            /* 附加功能日期 */
   UINT8        ucID;                   /* 附加功能ID */
   UINT8        aucReserved[3];             /* 保留未用 */
} STB_CA_AFD_S;

/* 授权信息对应的PID列表结构 */
typedef struct tagSTB_CA_PID_LIST
{
   UINT16*      puwPID;                 /* PID 列表 */
   UINT8        ucNumber;               /* 列表中数据个数 */
   UINT8        aucReserved[3];            /* 保留未用 */
} STB_CA_PID_LIST_S;

/* canal CA邮件头结构 */
typedef struct tagSTB_CA_EMAIL_HEAD
{
    UINT8        ucTID;              /* table_id, 应该为0x86 */
    UINT8        ucReserved;         /* 保留未用 */
    UINT8        ucNS[6];            /* 智能卡序列号 */
    UINT16       uwLong;             /* bit0－bit11: Length on 12 bits starting from the following byte
                                        bit12－bit13: MPEG reserved
                                        bit14: TBD
                                        bit15: Equal to 0
                                     */
    UINT16       uwOPI;              /* 运营商ID */
    UINT16       uwReserved;         /* 保留未用 */
    UINT8        ucPU1;
    UINT8        ucPU2;   
} STB_CA_EMAIL_HEAD_S;

/* canal CA邮件数据结构 */
typedef struct tagSTB_CA_EMAIL_DATA
{
   UINT8        ucType;             /* 邮件优先级. 0x02:普通, 0x0B:紧急 */
   UINT8        ucIndex;            /* 邮件索引 */
   CHAR         szMessage[86];      /* 消息内容 */
} STB_CA_EMAIL_DATA_S;

/* canal CA邮件结构 */
typedef struct tagSTB_CA_EMIAL_INFO
{
   STB_CA_EMAIL_HEAD_S   stEmailHeader;   /* 邮件头 */
   STB_CA_EMAIL_DATA_S   stEmailData;     /* 邮件数据 */
   UINT8                 aucReserved[3];  /* 保留未用 */
} STB_CA_EMIAL_INFO_S;

typedef VOID (*STBCAAppCallback)(UINT32 ulEventId, UINT32 ulResult, UINT32 ulDataLen, void* pData);
/*******************************************************************************
*
*     CA 应用API定义
*
*******************************************************************************/
extern INT32 STBInitCA(STB_CA_CONFIG_S  stConfig, STBCAAppCallback pfnNotify);
extern INT32 STBGetCAStatus(STB_CA_STATUS_S* pstStatus);
extern INT32 STBSetSysPassword(CHAR* szOldPwd, CHAR* szNewPwd);
extern INT32 STBCheckSysPassword(CHAR* szPwd);
extern INT32 STBGetOperatorInfo(STB_CA_OP_TYPE_E enType, UINT16 uwOpi);
extern INT32 STBStartRatingCheck(CHAR* szPwd);
extern INT32 STBStopRatingCheck(CHAR* szPwd);
extern INT32 STBGetRating(CHAR* szPwd);
extern INT32 STBSetRating(UINT8 ucRating, CHAR* szPwd);
extern INT32 STBGetCAId(void);
extern INT32 STBGetCardContent(void);
extern INT32 STBGetFuncData(UINT16 uwOpi, UINT8 ucIndex);
extern INT32 STBCardUpdate(void);
extern INT32 CAEventNotify(UINT32 udwEventId, UINT32 udwResult, UINT32 udwDataLen, void* pData);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* ifndef __STB_CA_APP_H__ */
