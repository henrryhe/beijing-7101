#ifndef __MGGENDRV_H__
#define __MGGENDRV_H__

/*****************************************************************************
  include
 *****************************************************************************/
/*#define           DEFINE_BOOLEAN*/
#include             "..\chcabase\ChSys.h"

#if 0
 #include          "Stcommon.h"
 #include          <string.h>
 #include          "stevt.h"              /* for the definition of the evt type*/
 #include          "stsmart.h"            /* for the definition of the smart card type */
 #include          "stddefs.h"            /* for the standard definition (such as U8,U32...) */
 #include          <stdio.h>             /* fot the definition(such as NULL) */


 #include          "app_data.h"
 #include          "sttbx.h"
 #include          "report.h"
 #include          "symbol.h"
 #include          "pti.h"
 #include          "vdbase.h"
 #include           "vsfilter.h"

 #include          "appltype.h"
 #ifdef             STLINK
 #include           "ptilink.h"
 #endif
 #include           "section.h"
#endif


#if 0

/*the Card event from the kernel structure*/
typedef  enum
{
      CARD_INSTANCE,
      CTRL_INSTANCE,
      SYS_INSTANCE,
      OTHER_INSTANCE
}reporting_instance_t;
 

typedef struct  Kernel_EventCmd_struct_tag
{
       reporting_instance_t	from_which_instance;
  	TMGAPIEventCode  	KerEventCode;
       HANDLE                        hInst;
	word		              Result;  
	dword                          ExResult;
}Kernel_Event_Cmd_t;

extern     semaphore_t         *pSemMgApiAccess;

extern boolean  ChSCEventSubscribe( int  IndexCard );


extern   void MgCardControl_Callback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2);


extern  void MgKerPartitionInit(void);

extern  boolean   ChCardProcessInit(void);

extern  void MgSubscribe_Callback
	 (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2);
extern   void MgProgCtrl_Callback
   (HANDLE hSubscription,byte EventCode,word Excode,dword ExCode2);

extern  void MgCardControl_Callback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2);


extern  void  ChGetApiStatusReport (TMGAPIStatus  chApiStatus);

/*extern  void  ChDescramblerInit(void);*/
extern  void  ChProgProcessStop(void);

#endif
#endif                                  /* __MGGENDRV_H__ */
