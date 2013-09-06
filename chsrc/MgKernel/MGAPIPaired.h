/*==========================================================================
  Mediaguard Conditional Access Kernel - Pairing custom extension library
 *--------------------------------------------------------------------------

  %name: MGAPIPaired.h %
  %version: 5 %
  %instance: STB_1 %
  %date_created: Fri Jun 17 13:51:15 2005 %

 *--------------------------------------------------------------------------

  API.Core.Paired ver. 1.0 Rev. C

 *--------------------------------------------------------------------------

  Application Programmer's Interface.

 *==========================================================================*/

#ifndef _MGAPIPaired_H_
#define _MGAPIPaired_H_

/*==========================================================================
  BASIC DEFINITIONS
 *==========================================================================*/

#include "MGAPI.h"

/*==========================================================================
  EVENTS DEFINITION
 *==========================================================================*/

/* API event codes
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICtrlPairing		= ( MGAPICtrlSrc + 1 )	
} TMGAPIPairedEvHaltedExCode;

typedef enum
{
	MGAPIPairedStatusChange = 0
} TMGAPIPairedUsrNotifyExCode;


/*==========================================================================
  Pairing LIBRARY
  --------------------------------------------------------------------------
  Pairing SECTION ["<M>" = "Paired"]
 *==========================================================================*/

/* Interfaces prototypes
 *--------------------------------------------------------------------------*/

extern void MGAPIPairedInit( TMGAPIConfig* pInit );
extern void MGAPIPairedTerminate( void );
extern TMGAPICtrlHandle MGAPIPairedCtrlOpen( CALLBACK hCallback, TMGSysSrcHandle hSrc );
extern TMGAPIStatus MGAPIPairedCtrlClose( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPIPairedCtrlStart( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPIPairedCheckFTA(void);
extern TMGAPIStatus MGAPIPairedCtrlSetEventMask( TMGAPICtrlHandle hCtrl, BitField32 EvMask );

/*==========================================================================*/

#endif	/* _MGAPIPaired_H_ */

/*= End of File ============================================================*/
