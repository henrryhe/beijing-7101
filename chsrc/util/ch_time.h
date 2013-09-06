/*
 * ch_time.h
 *
 * (c) Copyright  changhong
 *
 * Author(s) zxg    
 *
 * =====================
 * MODIFICATION HISTORY:
 * =====================
 *
 * Date        Modification                Initials
  2003/11/29
 * ----        ------------                --------
 */
#ifndef __CH_TIME_H__
#define __CH_TIME_H__
#include "appltype.h"
#include "stddefs.h"
#include "stdio.h"
#define MAX_EVENT_NAME_LENGTH  40
#define USER_RESERVE_SERVICE   32
#define MAX_LANGUAGE_ID              2
#define TIME_OFFSET 8

typedef struct tagDATE
{
   BYTE  ucWday;
   BYTE  ucDay;
   BYTE  ucMonth;
   SHORT sYear;
} TDTDATE;         /*日期*/

typedef struct tagTIME
{
   BYTE ucHours;
   BYTE ucMinutes;
   BYTE ucSeconds;
} TDTTIME;        /*时间*/




extern TDTDATE current_date;    
extern TDTTIME current_time;

#define TIMER_NAME_LEN    30

#define EPG_UPDATE_HOUR     8 
#define EPG_UPDATE_MINUTES  30

 typedef struct 
{
 		STBStaus CurSTBStaus;
 	U8       Level;
}STBInfo;


typedef struct 
{
	SHORT TargetProgNo;
	SHORT ReferServiceId;
	TDTDATE StartDate;
	TDTDATE EndDate;
	TDTTIME StartTime;
	TDTTIME EndTime;
	boolean TimerType;
	char EventName[TIMER_NAME_LEN+1];
	boolean isIPPV;
}TIMER_EVENT_SYNTHESIS;

#endif /*_CH_TIME_H__*/

