/*******************头文件说明注释**********************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：ch_antifuse.h                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-07-20                                             */
/*头文件内容概要描述（功能/变量等）：                              */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/
/*****************条件编译部分**************************************/
#ifndef CH_ANTIFUSE
#define CH_ANTIFUSE
/*******************************************************************/
/**************** #include 部分*************************************/
#include "stddef.h"
#include "stlite.h"
/*******************************************************************/
/**************** #define 部分**************************************/



/*******************************************************************/
/**************** #typedef 申明部分 ********************************/




/*******************************************************************/
/***************  外部变量申明部分 *********************************/
extern ST_Partition_t *NcachePartition ;
/*******************************************************************/
/***************  外部函数申明部分 *********************************/
extern BOOL    CH_Secure_Init( void )   ;
extern boolean CH_EnableBootAuthenticaion( void );
extern boolean CH_EnableJTAGLOCK( void );
extern boolean CH_EnableOTPBootCode( void );
extern boolean CH_BootAuthenticaionStatus( void );
extern boolean CH_JTAGLOCKStatus(void);
extern boolean CH_OTPStatus( void );
extern boolean CH_GetNUID(U8* puc_NuIDKey,U32 *pui_CRC);
extern U32 CH_GetNUIDKey( void );
extern void CH_GetPairCASerial(U8 *CASerial);
extern void make_crc_table_ISO_3309(void);
extern unsigned long crc_ISO_3309( unsigned char *buf, int len);
/*******************************************************************/
#endif

