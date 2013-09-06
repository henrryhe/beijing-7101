/*==========================================================================
  Mediaguard Conditional Access Kernel - Basic PPV extension
 *--------------------------------------------------------------------------

  %name: MGAPIPPV.h %
  %version: 6 %
  %instance: STB_2 %
  %date_created: Mon Nov 07 15:13:10 2005 %

 *--------------------------------------------------------------------------

  API.PPV.IDD ver. 1.0 Rev. D

 *--------------------------------------------------------------------------

  Application Programmer's Interface.

 *==========================================================================*/

#ifndef _MGAPIPPV_H_
#define _MGAPIPPV_H_

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
	MGAPIEvWallet		= 0x30,
	MGAPIEvProduct,
	MGAPIEvPPV
} TMGAPIPPVEventCode;

/*==========================================================================
  BASIC PPV LIBRARY
  --------------------------------------------------------------------------
  BASIC PPV SECTION ["<M>" = "PPV"]
 *==========================================================================*/

/* Specific handle declaration for basic PPV component
 *--------------------------------------------------------------------------*/

typedef struct sMGAPIPPVHandle* TMGAPIPPVHandle;

/* Specific events mask definition for basic PPV component
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIPPVEvCloseMask		= BitMask1(0),
	MGAPIPPVEvExtractMask	= BitMask1(1),
	MGAPIPPVEvBadCardMask	= BitMask1(2),
	MGAPIPPVEvReadyMask		= BitMask1(3),
	MGAPIPPVEvWalletMask	= BitMask1(4),
	MGAPIPPVEvProductMask	= BitMask1(5),
	MGAPIPPVEvPPVMask		= BitMask1(6)
} TMGAPIPPVEvMask;

/* Basic PPV status
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIPPVIdle	= MGAPIIdle,
	MGAPIPPVReady
} TMGAPIPPVStatus;

/* PPV identification
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGAPIPPV,
	MGAPIPPVSession,
	MGAPIPPPV,
	MGAPIIPPV,
	MGAPIIPPT
} TMGAPIPPVType;

typedef struct
{
	uword	OPI;
	uword	Session;
	ubyte	Type;		/* See TMGAPIPPVType */
	ubyte	Index;
} TMGAPIPPVID;

/* PPV information
 *--------------------------------------------------------------------------*/

typedef struct
{
	TMGAPIPPVID	ID;
	uword		Cost;
	ubyte		Access;
} TMGAPIPPV;

/* PPV configuration data structures
 *--------------------------------------------------------------------------*/

typedef struct
{
	int				nPPVInst;
} TMGAPIPPVConfig;

/* Interfaces prototypes
 *--------------------------------------------------------------------------*/

extern TMGAPIStatus MGAPIPPVInit( TMGAPIPPVConfig* pInit );
extern void MGAPIPPVTerminate( void );

extern boolean MGAPIPPVIsRunning( void );
extern TMGAPIStatus MGAPIPPVExist( TMGAPIPPVHandle hPPV );
extern TMGAPIStatus MGAPIPPVGetList( int* N, TMGAPIPPVHandle* lhPPV );
extern TMGAPIStatus MGAPIPPVCloseAll( void );
extern TMGAPIStatus MGAPIPPVGetStatus
	( TMGAPIPPVHandle hPPV, TMGAPIPPVStatus* Status );
extern TMGAPIStatus MGAPIPPVSetEventMask
	( TMGAPIPPVHandle hPPV, BitField32 EvMask );

extern TMGAPIPPVHandle MGAPIPPVOpen( CALLBACK hCallback );
extern TMGAPIStatus MGAPIPPVClose( TMGAPIPPVHandle hPPV );
extern TMGAPIStatus MGAPIPPVGetWallet( TMGAPIPPVHandle hPPV, uword OPI );
extern TMGAPIStatus MGAPIPPVGetProduct
	( TMGAPIPPVHandle hPPV, uword OPI, uword Session );
extern TMGAPIStatus MGAPIPPVGetFirstProduct( TMGAPIPPVHandle hPPV, uword OPI );
extern TMGAPIStatus MGAPIPPVGetNextProduct( TMGAPIPPVHandle hPPV, uword OPI );
extern TMGAPIStatus MGAPIPPVConsume
	( TMGAPIPPVHandle hPPV, ubyte* Password, TMGAPIPPVID* pPPV );

/* MGAPIEvClose event
 * HANDLE: See TMGAPIPPVHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvExtract event
 * HANDLE: See TMGAPIPPVHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvBadCard event
 * HANDLE: See TMGAPIPPVHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvReady event
 * HANDLE: See TMGAPIPPVHandle.
 *--------------------------------------------------------------------------*/

/* MGAPIEvWallet event
 * HANDLE: See TMGAPIPPVHandle.
 * ExCode: See TMGAPIEvCardCmdReport (MGAPI.h)
 *--------------------------------------------------------------------------*/

/* EventData: Address of a wallet status structure */

typedef struct
{
	dword	Status;
	uword	OPI;
	uword	Credit;
	uword	Date;
	ubyte	MaxOverdraft;
} TMGAPIWallet;

/* MGAPIEvProduct event
 * HANDLE: See TMGAPIPPVHandle.
 * ExCode: See TMGAPIEvCardCmdReport (MGAPI.h)
 *--------------------------------------------------------------------------*/

/* EventData: Address of a product information structure */

typedef struct
{
	TMGAPIPPV	Info;
	uword		Date;
	uword		Summary;
	boolean		lReq;
} TMGAPIProduct;

/* MGAPIEvPPV event
 * HANDLE: See TMGAPIPPVHandle.
 * EventData: See TMGAPIPPVID.
 *--------------------------------------------------------------------------*/

/* ExCode: Result of PPV operation */

typedef enum
{
	MGAPIPPVDone,
	MGAPIPPVHWFailure,
	MGAPIPPVPrgHalted,
	MGAPIPPVWallet,
	MGAPIPPVBlackout,
	MGAPIPPVLowRating,
	MGAPIPPVMaxShots,
	MGAPIPPVAborted
} TMGAPIPPVReport;

/*==========================================================================*/

#endif	/* _MGAPIPPV_H_ */

/*= End of File ============================================================*/
