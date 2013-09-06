/*==========================================================================
  Mediaguard Conditional Access Kernel - Core Library
 *--------------------------------------------------------------------------

  %name: MGAPI.h %
  %version: 15.1.2 %
  %instance: STB_2 %
  %date_created: Mon Oct 03 09:31:19 2005 %

 *--------------------------------------------------------------------------

  API.Core.IDD ver. 1.0 Rev. K

 *--------------------------------------------------------------------------

  Application Programmer's Interface.

 *==========================================================================*/

#ifndef _MGAPI_H_
#define _MGAPI_H_

/*==========================================================================
  BASIC DEFINITIONS
 *==========================================================================*/

#include "General.h"

/*==========================================================================
  GENERAL API DEFINITIONS
 *==========================================================================*/

/* API return codes
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIOk,
	MGAPISysError,
	MGAPIHWError,
	MGAPIInvalidParam,
	MGAPINotFound,
	MGAPIAlreadyExist,
	MGAPINotInitialized,
	MGAPIIdling,
	MGAPIEmpty,
	MGAPIRunning,
	MGAPINotRunning,
	MGAPINoResource,
	MGAPINotReady,
	MGAPILockError,
	MGAPINotEntitled,
	MGAPINotAvailable
} TMGAPIStatus;

/*==========================================================================
  EVENTS DEFINITION
 *==========================================================================*/

/* API event codes
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIEvSysError,
	MGAPIEvNoResource,
	MGAPIEvDumpText,
	MGAPIEvMsgNotify,
	MGAPIEvClose,
	MGAPIEvHalted,
	MGAPIEvExtract,
	MGAPIEvBadCard,
	MGAPIEvUnknown,
	MGAPIEvReady,
	MGAPIEvPassword,
	MGAPIEvCmptStatus,
	MGAPIEvRejected,
	MGAPIEvAcknowledged,
	MGAPIEvContent,
	MGAPIEvAddFuncData,
	MGAPIEvRating,
	MGAPIEvUsrNotify,
	MGAPIEvUpdate
} TMGAPIEventCode;

/*==========================================================================
  BASIC LIBRARY
  --------------------------------------------------------------------------
  GENERIC SECTION ["<M>" = ""]
 *==========================================================================*/

/* Default status of an instance
 *--------------------------------------------------------------------------*/

#define MGAPIIdle				0

/* Interfaces
 *--------------------------------------------------------------------------
 * boolean MGAPI<M>IsRunning();
 * TMGAPIStatus MGAPI<M>Exist( HANDLE hInst );
 * TMGAPIStatus MGAPI<M>GetList( int* N, HANDLE* lhInst );
 * TMGAPIStatus MGAPI<M>CloseAll();
 * TMGAPIStatus MGAPI<M>GetStatus( HANDLE hInst, <int>* Status );
 * TMGAPIStatus MGAPI<M>SetEventMask( HANDLE hInst, BitField32 EvMask )
 */

/* MGAPIEvClose event
 * <No further details>
 *--------------------------------------------------------------------------*/

/* MGAPIEvHalted event
 * <No further details>
 *--------------------------------------------------------------------------*/

/* MGAPIEvExtract event
 * <No further details>
 *--------------------------------------------------------------------------*/

/* MGAPIEvBadCard event
 * <No further details>
 *--------------------------------------------------------------------------*/

/* MGAPIEvReady event
 * EventData: See TMGAPICardContent
 *--------------------------------------------------------------------------*/

/*==========================================================================
  BASIC LIBRARY
  --------------------------------------------------------------------------
  KERNEL ADMINISTRATION SECTION ["<M>" = ""]
 *==========================================================================*/

/* Specific handle declaration for Kernel administration component
 *--------------------------------------------------------------------------*/

/* Event subscription handle */
typedef struct sMGAPISubscriptionHandle*	TMGAPISubscriptionHandle;

/* Event mask */

typedef enum
{
	MGAPIEvSysErrorMask		= BitMask1(0),
	MGAPIEvDumpTextMask		= BitMask1(1),
	MGAPIEvNoResourceMask	= BitMask1(2),
	MGAPIEvPasswordMask		= BitMask1(3),
	MGAPIEvMsgNotifyMask	= BitMask1(4),
	MGAPIEvUnknownMask		= BitMask1(5),
	MGAPIEvUsrNotifyMask	= BitMask1(6)
} TMGAPIEvMask;

/* Kernel module(s) revision data structures
 *--------------------------------------------------------------------------*/

#define MGAPIMaxLib		8

typedef struct
{
	char Name[24];
	char Date[9];
	char Release[15];
} TMGAPIRevision;

typedef struct
{
	TMGAPIRevision		Ver[MGAPIMaxLib];
	char				Copyright[48];
} TMGAPIID;

/* Kernel configuration data structures
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIDVBCS,
	MGAPIDES,
	MGAPITripleDES,
	MGAPIAES
} TMGAPIDscrType;

typedef struct
{
	TMGAPIDscrType	DscrType;

	boolean			bCheckPairing;
	boolean			bCheckBlackList;
	boolean			bCardRatingBypass;

	int				nAdmSubscribers;
	int				nCtrlInst;
	int				nCardInst;
} TMGAPIConfig;

/* Operator list structure
 *--------------------------------------------------------------------------*/

typedef struct
{
	uword*	lOPI;
	int	nOPI;
}TMGAPIOPIList;

/* Interfaces prototypes
 *--------------------------------------------------------------------------*/

extern TMGAPIStatus MGAPIGetID( TMGAPIID* pID );

extern void MGAPIInit( TMGAPIConfig* pInit );
extern void MGAPITerminate( void );

extern boolean MGAPIIsRunning( void );
extern TMGAPIStatus MGAPISetEventMask
	( TMGAPISubscriptionHandle hSubscription, BitField32 EvMask );

extern TMGAPISubscriptionHandle MGAPISubscribe( CALLBACK hCallback );
extern TMGAPIStatus MGAPIUnsubscribe( TMGAPISubscriptionHandle hSubscription );

extern TMGAPIStatus MGAPISetPassword( ubyte* OldPassword, ubyte* NewPassword );
extern TMGAPIStatus MGAPICheckPassword( ubyte* Password );

extern boolean MGAPIMGCardIsReady( void );

/* MGAPIEvSysError event
 * HANDLE: See TMGAPISubscriptionHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvNoResource event
 * HANDLE: See TMGAPISubscriptionHandle.
 *--------------------------------------------------------------------------*/

/* ExCode: Resource type */
typedef enum
{
	MGAPIMemory,
	MGAPIDmxChannel,
	MGAPIDscrChannel
} TMGAPIEvNoResourceExCode;

/*
	EventData: Source of the no more available resource
	=> Systeme source handle (see DDI for Demux and descrambler)
	=> Following codes for memory
 */
typedef enum
{
	MGAPIRAM,
	MGAPIFlash
} TMGAPIEvNoResourceEventData;

/* MGAPIEvMsgNotify event
 * HANDLE: See TMGAPISubscriptionHandle.
 *--------------------------------------------------------------------------*/

/* ExCode: Type of notification */
typedef enum
{
	MGAPIEMMC
} TMGAPIEvMsgNotifyExCode;

/* EventData: Data block */
typedef struct
{
	ubyte*	pData;
	uword	Size;
} TMGAPIBlk;

/* MGAPIEvDumpText event
 * HANDLE: See TMGAPISubscriptionHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvPassword event
 * HANDLE: See TMGAPISubscriptionHandle.
 *--------------------------------------------------------------------------*/

/* EventData: Result of password command */
typedef enum
{
	MGAPIPwdNoCard,
	MGAPIPwdHWFailure,
	MGAPIPwdInvalid,
	MGAPIPwdReset,
	MGAPIPwdValid
} TMGAPIEvPasswordExCode;

/* MGAPIEvUnknown event
 * HANDLE: See TMGAPISubscriptionHandle.
 * EventData: See TMGSysSCRdrHandle
 *--------------------------------------------------------------------------*/

/*==========================================================================
  BASIC LIBRARY
  --------------------------------------------------------------------------
  PROGRAM CONTROLLER SECTION ["<M>" = "Ctrl"]
 *==========================================================================*/

/* Specific handle declaration for program controller component
 *--------------------------------------------------------------------------*/

typedef struct sMGAPICtrlHandle*	TMGAPICtrlHandle;

/* Specific events mask definition for program controller component
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICtrlEvHaltedMask		= BitMask1(0),
	MGAPICtrlEvCloseMask		= BitMask1(1),
	MGAPICtrlEvBadCardMask		= BitMask1(2),
	MGAPICtrlEvReadyMask		= BitMask1(3),
	MGAPICtrlEvCmptStatusMask	= BitMask1(4),
	MGAPICtrlEvRejectedMask		= BitMask1(5),
	MGAPICtrlEvAcknowledgedMask	= BitMask1(6)
} TMGAPICtrlEvMask;

/* Program origin
 *--------------------------------------------------------------------------*/

typedef struct
{
	uword	ONID;
	uword	TSID;
	uword	SvcID;
	uword	EvtID;
} TMGAPIOrigin;

/* Program controller status
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICtrlIdle	= MGAPIIdle,
	MGAPICtrlLoaded,
	MGAPICtrlReady,
	MGAPICtrlRunning
} TMGAPICtrlStatus;

/* Program controller source status
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPISrcDisconnected,
	MGAPISrcConnected
} TMGAPISrcStatus;

/* List of ES PID
 *--------------------------------------------------------------------------*/

typedef struct
{
	uword*	pPID;
	udword	Reserved;
	ubyte	n;
} TMGAPIPIDList;

/* Interfaces prototypes
 *--------------------------------------------------------------------------*/

extern boolean MGAPICtrlIsRunning( void );
extern TMGAPIStatus MGAPICtrlExist( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPICtrlGetList( int* N, TMGAPICtrlHandle* lhCtrl );
extern TMGAPIStatus MGAPICtrlCloseAll( void );
extern TMGAPIStatus MGAPICtrlGetStatus
	( TMGAPICtrlHandle hCtrl, TMGAPICtrlStatus* Status );
extern TMGAPIStatus MGAPICtrlSetEventMask
	( TMGAPICtrlHandle hCtrl, BitField32 EvMask );

extern TMGAPICtrlHandle MGAPICtrlOpen
	( CALLBACK hCallback, TMGSysSrcHandle hSrc );
extern TMGAPIStatus MGAPICtrlClose( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPICtrlLoad(
	TMGAPICtrlHandle hCtrl, TMGAPIOrigin* pOrigin,
	ubyte* hPMT, ubyte nPID, uword* lpPID
);
extern TMGAPIStatus MGAPICtrlChange( TMGAPICtrlHandle hCtrl, ubyte* hPMT );
extern TMGAPIStatus MGAPICtrlFree( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPICtrlAdd( TMGAPICtrlHandle hCtrl, uword PID );
extern TMGAPIStatus MGAPICtrlDel( TMGAPICtrlHandle hCtrl, uword PID );
extern TMGAPIStatus MGAPICtrlStart( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPICtrlHalt( TMGAPICtrlHandle hCtrl );
extern TMGAPIStatus MGAPICtrlProgInfo
	( TMGAPICtrlHandle hCtrl, TMGSysSrcHandle* phSource, TMGAPIOrigin* pOrigin );
extern TMGAPIStatus MGAPICtrlCmptStatus
	( TMGAPICtrlHandle hCtrl, uword PID, boolean bCheck );
extern TMGAPIStatus MGAPICtrlSrcStatus
	( TMGAPICtrlHandle hCtrl, TMGAPISrcStatus* pStatus );

/* MGAPIEvClose event
 * HANDLE: See TMGAPICtrlHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvHalted event
 * HANDLE: See TMGAPICtrlHandle.
 *--------------------------------------------------------------------------*/

/* Program controller specific ExCode for common event MGAPIEvHalted
 *--------------------------------------------------------------------------*/
typedef enum
{
	MGAPICtrlUser,
	MGAPICtrlCard,
	MGAPICtrlSrc
} TMGAPICtrlEvHaltedExCode;

/* MGAPIEvBadCard event
 * HANDLE: See TMGAPICtrlHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvReady event
 * HANDLE: See TMGAPICtrlHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvAcknowledged event
 * HANDLE: See TMGAPICtrlHandle.
 * EventData: Address of a ES PID list
 *--------------------------------------------------------------------------*/

/* ExCode: Acceptance report */

typedef enum
{
	MGAPICMAck,
	MGAPICMOverdraft,
	MGAPICMPreview,
	MGAPICMPPV			= BitMask1(15)
} TMGAPIEvAcknowledgedExCode;

/* MGAPIEvRejected event
 * HANDLE: See TMGAPICtrlHandle.
 * EventData: Address of a ES PID list
 *--------------------------------------------------------------------------*/

/* ExCode: Rejected report */

typedef enum
{
	MGAPICMCardFailure	= BitMask1(0),
	MGAPICMNANoRights	= BitMask1(1),
	MGAPICMNAExpired	= BitMask1(2),
	MGAPICMNABlackout	= BitMask1(3),
	MGAPICMNARating		= BitMask1(4),
	MGAPICMNAPPPV		= BitMask1(5),
	MGAPICMNAIPPV		= BitMask1(6),
	MGAPICMNAIPPT		= BitMask1(7),
	MGAPICMNAMaxShot	= BitMask1(8),
	MGAPICMNANoCredit	= BitMask1(9),
	MGAPICMNAOthers		= BitMask1(10)
} TMGAPIEvRejectedExCode;

/* MGAPIEvCmptStatus event
 * HANDLE: See TMGAPICtrlHandle.
 *--------------------------------------------------------------------------*/

/* ExCode: Entitlement summary => TMGAPICmptStatusList* */
typedef enum
{
	MGAPICmptOk			= BitMask1(0),
	MGAPICmptOverdraft	= BitMask1(1),
	MGAPICmptPreview	= BitMask1(2),
	MGAPICmptNANoCard	= BitMask1(3),
	MGAPICmptNAOperator	= BitMask1(4),
	MGAPICmptNAOffer	= BitMask1(5),
	MGAPICmptNAExpired	= BitMask1(6),
	MGAPICmptNABlackout	= BitMask1(7),
	MGAPICmptNARating	= BitMask1(8),
	MGAPICmptNAMaxShot	= BitMask1(9),
	MGAPICmptNANoCredit	= BitMask1(10),
	MGAPICmptNAPPPV		= BitMask1(11),
	MGAPICmptNAIPPV		= BitMask1(12),
	MGAPICmptNAIPPT		= BitMask1(13),
	MGAPICmptNAUnknown	= BitMask1(14),
	MGAPICmptNAOthers	= BitMask1(15)
} TMGAPIEvCmptStatusExCode;

/* EventData: Adresse of an entitlement list */

typedef struct
{
	ubyte	Offer[8];
	uword	OPI;
	uword	Date;
} TMGAPIOffer;		/* pInfo field of TMGAPICmptCAType structure */

typedef enum
{
	MGAPICATypeOffer,
	MGAPICATypePPV,
	MGAPICATypeUnknown
} TMGAPICmptCATypeID;

typedef struct
{
	TMGAPICmptCATypeID	Type;
	void*				pInfo;
	udword				Reserved;
} TMGAPICmptCAType;

typedef struct
{
	TMGAPICmptCAType*	lpStatus;
	uword				PID;
	ubyte				nStatus;
} TMGAPICmptStatusList;

/*==========================================================================
  BASIC LIBRARY
  --------------------------------------------------------------------------
  MEDIAGUARD USER INTERFACE ["<M>" = "CARD"]
 *==========================================================================*/

/* Specific handle declaration for Mediaguard user interface component
 *--------------------------------------------------------------------------*/

typedef struct sMGAPICardHandle*	TMGAPICardHandle;

/* Specific events mask declaration for Mediaguard user interface component
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICardEvCloseMask		= BitMask1(0),
	MGAPICardEvExtractMask		= BitMask1(1),
	MGAPICardEvBadCardMask		= BitMask1(2),
	MGAPICardEvReadyMask		= BitMask1(3),
	MGAPICardEvContentMask		= BitMask1(4),
	MGAPICardEvAddFuncDataMask	= BitMask1(5),
	MGAPICardEvRatingMask		= BitMask1(6),
	MGAPICardEvUpdateMask		= BitMask1(7)
} TMGAPICardEvMask;

/* Mediaguard user interface status
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICardNotReady	= MGAPIIdle,
	MGAPICardReady
} TMGAPICardStatus;

/* Operator structure
 *--------------------------------------------------------------------------*/

typedef struct
{
	uword	OPI;
	uword	Date;
	ubyte	Name[16];
	ubyte	Offers[8];
	ubyte	Geo;
} TMGAPIOperator;

/* All card command event results
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPICardOk,
	MGAPICardNotFound,
	MGAPICardBadParam,
	MGAPICardNoAnswer,
	MGAPICardFailure
} TMGAPIEvCardCmdReport;

/* Interfaces prototypes
 *--------------------------------------------------------------------------*/

extern boolean MGAPICardIsRunning( void );
extern TMGAPIStatus MGAPICardExist( TMGAPICardHandle hCard );
extern TMGAPIStatus MGAPICardGetList( int* N, TMGAPICardHandle* lhCard );
extern TMGAPIStatus MGAPICardCloseAll( void );
extern TMGAPIStatus MGAPICardGetStatus
	( TMGAPICardHandle hCard, TMGAPICardStatus* Status );
extern TMGAPIStatus MGAPICardSetEventMask
	( TMGAPICardHandle hCard, BitField32 EvMask );

extern TMGAPICardHandle MGAPICardOpen( CALLBACK hCallback );
extern TMGAPIStatus MGAPICardClose( TMGAPICardHandle hCard );
extern TMGAPIStatus MGAPICardContent( TMGAPICardHandle hCard );

extern TMGAPIStatus MGAPICardGetOperator
	( TMGAPICardHandle hCard, uword OPI, TMGAPIOperator* pOperator );
extern TMGAPIStatus MGAPICardGetFirstOperator
	( TMGAPICardHandle hCard, TMGAPIOperator* pOperator );
extern TMGAPIStatus MGAPICardGetNextOperator
	( TMGAPICardHandle hCard, TMGAPIOperator* pOperator );
extern TMGAPIStatus MGAPICardUpdate( TMGAPICardHandle hCard );

extern TMGAPIStatus MGAPICardGetAddFuncData
	( TMGAPICardHandle hCard, uword OPI, ubyte Index );

extern TMGAPIStatus MGAPICardGetRating( TMGAPICardHandle hCard );
extern TMGAPIStatus MGAPICardSetRating
	( TMGAPICardHandle hCard, ubyte Rating, ubyte* Password );
extern TMGAPIStatus MGAPICardCheckRating
	( TMGAPICardHandle hCard, ubyte* Password );
extern TMGAPIStatus MGAPICardUncheckRating
	( TMGAPICardHandle hCard, ubyte* Password );
extern TMGAPIStatus MGAPICardRestrictOPI( TMGAPICardHandle hCard, TMGAPIOPIList* lpOPI );

/* MGAPIEvClose event
 * HANDLE: See TMGAPICardHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvExtract event
 * HANDLE: See TMGAPICardHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvBadCard event
 * HANDLE: See TMGAPICardHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvReady event
 * HANDLE: See TMGAPICardHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvContent event
 * HANDLE: See TMGAPICardHandle.
 * ExCode: See TMGAPIEvCardCmdReport
 *--------------------------------------------------------------------------*/

/* EventData: Software content of the smart card */

typedef enum
{
	MGAPIIDUnknown,
	MGAPIIDMediaguard
} TMGAPICardAppID;

typedef struct
{
	ubyte	ID;			/* See TMGAPICardAppID */
	ubyte	Major;
	ubyte	Minor;
} TMGAPICardApp;

typedef enum
{
	MGAPIUnknown		= 0x00,
	MGAPIMediaguard		= 0x01,
	MGAPIMultiple		= 0x02
} TMGAPICardTypeID;

typedef struct
{
  TMGAPICardApp*	pAppData;
  ubyte				nApp;
  BitField08		Type;	/* TMGAPICardTypeID */
} TMGAPICardType;

#define MGAPICardProductCodeLength		2
#define MGAPICardSerialNumberLength		6

typedef struct
{
	ubyte ProductCode[MGAPICardProductCodeLength];
	ubyte SerialNumber[MGAPICardSerialNumberLength];
} TMGAPICardID;

typedef struct
{
	TMGAPICardID	ID;
	TMGAPICardType	Type;
} TMGAPICardContent;

/* MGAPIEvAddFuncData event
 * HANDLE: See TMGAPICardHandle.
 * ExCode: See TMGAPIEvCardCmdReport
 *--------------------------------------------------------------------------*/

/* EventData: Address of AFD information structure */

typedef struct
{
	uword	OPI;
	ubyte	Data[10];
	ubyte	ID;
} TMGAPIAFD;

/* MGAPIEvRating event
 * HANDLE: See TMGAPICardHandle.
 * ExCode: See TMGAPIEvCardCmdReport
 *--------------------------------------------------------------------------*/

/* EventData: Address of rating information structure */

typedef enum
{
	MGAPIRating			= 0x01,
	MGAPIRatingCheck	= 0x02
} TMGAPIRatingValidBits;

typedef struct
{
	BitField08	Valid;		/* See TMGAPIRatingValidBits */
	ubyte		Rating;
	ubyte		IsChecked;
} TMGAPIRating;

/* MGAPIEvUpdate event
 * HANDLE: See TMGAPICardHandle.
 *--------------------------------------------------------------------------*/

/*==========================================================================*/

#endif	/* _MGAPI_H_ */

/*==========================================================================
   END OF FILE
 *==========================================================================*/
