/*******************头文件说明注释**********************************/
/*公司版权说明：版权（2009）归长虹网络科技有限公司所有。           */
/*文件名：MGCA_Ipanel.h                                            */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2009-06-24                                             */
/*头文件内容概要描述（功能/变量等）：                              */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/
/*****************条件编译部分**************************************/
#ifndef CH_MGCA_IPANEL
#define CH_MGCA_IPANEL
/*******************************************************************/
/**************** #include 部分*************************************/
#include "stddef.h"
#include "stlite.h"
#include "ipanel_typedef.h"
#include "ca_dmx_api.h"
#include "ca_emm_api.h"
#include "ca_srv_api.h"
#include "stb_ca_app.h"
#include "ca_pmt_api.h"
#include "ipanel_typedef.h"

/*******************************************************************/
/**************** #define 部分**************************************/



/**************** #typedef 申明部分 ********************************/


typedef struct
{
  	int original_network_id;
	int transport_stream_id;
	int service_id;
	int video_pid;
	int audio_pid;
	int pcr_pid;
}CH_Service_Info_t;

typedef enum
{   
  CH_CA2IPANEL_EXTRACT_CARD,
  CH_CA2IPANEL_BAD_CARD,
  CH_CA2IPANEL_UNKNOW_CARD,
  CH_CA2IPANEL_OK_CARD,
  CH_CA2IPANEL_Have_Right,
  CH_CA2IPANEL_NO_Right,
  CH_CA2IPANEL_EXPIRED,
  CH_CA2IPANEL_BLACKOUT,
  CH_CA2IPANEL_RESET_PIN,
  CH_CA2IPANEL_NEW_MAIL,
  CH_CA2IPANEL_PAIR_OK,
  CH_CA2IPANEL_PAIR_FAIL,
}CH_CA2IPANEL_EVT_t;



/***************  外部变量申明部分 *********************************/



/***************  外部函数申明部分 *********************************/

extern boolean  CHCA_CardSendMess2Usif (CH_CA2IPANEL_EVT_t  r_evt,U8 *Data,int ri_len);
extern void CH_MGCA_IpanelNotify(UINT32 ulEventId, UINT32 ulResult, UINT32 ulDataLen, void* pData);
extern void CH_MGCA_IpanelNotify(UINT32 ulEventId, UINT32 ulResult, UINT32 ulDataLen, void* pData);
extern void CH_MGCA_Ipanel_UpdateCardStatus(CH_CA2IPANEL_EVT_t r_evt,U8 *Data,int ri_len);





/*******************************************************************/
#endif

