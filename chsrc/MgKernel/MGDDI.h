/*==========================================================================
  Mediaguard Conditional Access Kernel - Core Library
 *--------------------------------------------------------------------------

  %name: MGDDI.h %
  %version: 15 %
  %instance: STB_2 %
  %date_created: Fri Jun 17 10:44:45 2005 %

 *--------------------------------------------------------------------------

  DDI.Core.IDD ver. 1.0 Rev. H

 *--------------------------------------------------------------------------

  Device Driver's Interface.

 *==========================================================================*/

#ifndef _MGDDI_H_
#define _MGDDI_H_

/*==========================================================================
  BASIC DEFINITIONS
 *==========================================================================*/

#include "General.h"

/*==========================================================================
  GENERAL DDI DEFINITIONS
 *==========================================================================*/

/* DDI return codes
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDIOK,
	MGDDINoResource,
	MGDDIBadParam,
	MGDDIProtocol,
	MGDDINotFound,
	MGDDIBusy,
	MGDDILocked,
	MGDDINotSupported,
	MGDDIError
} TMGDDIStatus;

/* DDI event codes
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDIEvCardExtract,
	MGDDIEvCardInsert,
	MGDDIEvCardResetRequest,
	MGDDIEvCardReset,
	MGDDIEvCardProtocol,
	MGDDIEvCardReport,
	MGDDIEvCardLockTimeout,

	MGDDIEvSrcDmxFltrReport,
	MGDDIEvSrcDmxFltrTimeout,
	MGDDIEvSrcNoResource,
	MGDDIEvSrcStatusChanged,

	MGDDIEvDelay

} TMGDDIEventCode;

/*==========================================================================
  GENERAL SECTION
 *==========================================================================*/

/* Event registration handle
 *--------------------------------------------------------------------------*/

typedef struct sMGDDIEventSubscriptionHandle*	TMGDDIEventSubscriptionHandle;

/* STB information data structures
 *--------------------------------------------------------------------------*/

#define STBSerialNumberLength		8
#define STBHVNLength				4
#define STBSVNLength				4

typedef struct
{
	struct
	{
		ubyte SerialNumber[STBSerialNumberLength];
	} ID;

	struct
	{
		ubyte	HVN[STBHVNLength];
		ubyte	SVN[STBSVNLength];
	} Ver;

} TMGDDISTBData;

/* General intefaces
 *--------------------------------------------------------------------------*/

extern TMGDDIStatus MGDDIGetSTBData( TMGDDISTBData* pSTBData );
extern void MGDDIInhibitSTB( void );
extern TMGDDIStatus MGDDIUnsubscribe
	( TMGDDIEventSubscriptionHandle hSubscription );


/*==========================================================================
  MEDIAGUARD SMART CARD SECTION
 *==========================================================================*/

/* Smart card reader handles
 *--------------------------------------------------------------------------*/

/* MG Kernel handle */
typedef struct sMGDDICardReaderHandle*		TMGDDICardReaderHandle;

/* Smart card ATR data structure (notification)
 *--------------------------------------------------------------------------*/

/* Type field values */
#define	MGDDICardHistoric	FALSE
#define	MGDDICardATR		TRUE

typedef struct
{
	boolean	bATR;		/* See above for values */
	ubyte	Size;
	ubyte*	pData;
} TMGDDICardATR;

/* Smart card protocol data structure
 *--------------------------------------------------------------------------*/

/* Type field values - MGDDICardTxBit(0) => T=0 protocol */
#define MGDDICardTxBit(x)	BitMask1(x)
typedef enum
{
	MGDDICardT0Ptcl	= MGDDICardTxBit(0),
	MGDDICardT1Ptcl	= MGDDICardTxBit(1)
} TMGDDICardTxPtcl;

typedef struct
{
	TMGDDICardTxPtcl	Type;	/* Only 1 bit valid => current protocol */
	udword				Rate;	/* Current rate */
} TMGDDICardProtocol;

/* Smart card capacities data structure
 *--------------------------------------------------------------------------*/

typedef struct
{
	BitField32	SupportedPtcl;	/* See TMGDDICardProtocol.Type for values */
	udword		Rate;			/* Max. Rate */
} TMGDDICardCaps;

/* Smart card message data structure (notification)
 *--------------------------------------------------------------------------*/

typedef struct
{
	ubyte*	pData;
	uword	Size;
	boolean	bLocked;
} TMGDDICardReport;

/* MGDDIEvCardReport event: ExCode values
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDICardOk,
	MGDDICardProtocol,
	MGDDICardError
} TMGDDIEvCardReportExCode;

/* Smart card interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDIStatus MGDDICardGetMGReaders
	( TMGSysSCRdrHandle* lhReaders, ubyte* nReaders );
extern TMGDDICardReaderHandle MGDDICardOpen( TMGSysSCRdrHandle hReader );
extern TMGDDIStatus MGDDICardClose( TMGDDICardReaderHandle hCard );
extern TMGDDIEventSubscriptionHandle MGDDICardSubscribe
	( TMGDDICardReaderHandle hCard, CALLBACK hCallback );
extern TMGDDIStatus MGDDICardIsEmpty( TMGDDICardReaderHandle hCard );
extern TMGDDIStatus MGDDICardSetExclusive( TMGDDICardReaderHandle hCard );
extern TMGDDIStatus MGDDICardClearExclusive( TMGDDICardReaderHandle hCard );
extern TMGDDIStatus MGDDICardReset( TMGDDICardReaderHandle hCard );
extern TMGDDIStatus MGDDICardGetCaps
	( TMGDDICardReaderHandle hCard, TMGDDICardCaps* pCaps );
extern TMGDDIStatus MGDDICardGetProtocol
	( TMGDDICardReaderHandle hCard, TMGDDICardProtocol* pProtocol );
extern TMGDDIStatus MGDDICardChangeProtocol
	( TMGDDICardReaderHandle hCard, TMGDDICardProtocol* pProtocol );
extern TMGDDIStatus MGDDICardSend(
	TMGDDICardReaderHandle hCard,
	CALLBACK hCallback, ubyte* pMsg, uword Size
);
extern TMGDDIStatus MGDDICardAbort( TMGDDICardReaderHandle hCard );
extern TMGDDIStatus MGDDICardLock
	( TMGDDICardReaderHandle hCard, CALLBACK hCallback, udword LockTime );
extern TMGDDIStatus MGDDICardUnlock( TMGDDICardReaderHandle hCard );

/*==========================================================================
  SOURCE [DESCRAMBLER,DEMUX] SECTION
 *==========================================================================*/

/* Source [descrambler,demux] handles
 *--------------------------------------------------------------------------*/

/* MG Kernel Handles */
typedef struct sMGDDISrcDscrChnlHandle*	TMGDDISrcDscrChnlHandle;
typedef struct sMGDDISrcDmxFltrHandle*	TMGDDISrcDmxFltrHandle;

/* MGDDIEvSrcStatusChanged event: ExCode values
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDISrcConnected,
	MGDDISrcDisconnected,
	MGDDISrcTrpChanged
} TMGDDIEvSrcStatusChangedExCode;

/* MGDDIEvSrcNoResource event: ExCode values
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDIDscrChnl,
	MGDDIDmxFltr
} TMGDDIEvSrcNoResourceExCode;

/* MGDDIEvSrcDmxFltrTimeout event: ExCode values
 *--------------------------------------------------------------------------*/

typedef enum
{
	MGDDISrcDmxNoSection,
	MGDDISrcDmxSctInvalid,
	MGDDISrcDmxTblInvalid,
	MGDDISrcDmxTblInvalidPart,
	MGDDISrcDmxTblNotCompleted
} TMGDDIEvSrcDmxFltrTimeoutExCode;

/* Source - Capacities data structure
 *--------------------------------------------------------------------------*/

/* Type field values */
typedef enum
{
	MGDDIEMMSource	=	BitMask1(0),
	MGDDIECMSource	=	BitMask1(1)
} TMGDDISrcSourceType;

typedef struct
{
	TMGSysSrcHandle	hSource;
	BitField16		Type;		/* Combination of TMGDDISrcSourceType values */
} TMGDDISrcCaps;

/* Descrambleur - Capacities data structure
 *--------------------------------------------------------------------------*/

/* Type field values */
typedef enum
{
	MGDDIDVBCS		=	BitMask1(0),
	MGDDIDES		=	BitMask1(1),
	MGDDITripleDES	=	BitMask1(2),
	MGDDIAES		=	BitMask1(3)
} TMGDDIDscrType;

typedef struct
{
	TMGSysSrcHandle	hSource;
	BitField16		Type;		/* Combination of TMGDDIDscrType values */
	uword			nChannels;
	uword			nPIDPerChannel;
} TMGDDIDscrCaps;

/* Descrambleur - Configuration data structure
 *--------------------------------------------------------------------------*/

typedef struct
{
	TMGDDIDscrType	Type;
	ubyte			nKeys;
	ubyte			KeySize;
} TMGDDIDscrSelect;

/* Descrambleur - Control words data structure
 *--------------------------------------------------------------------------*/

typedef struct
{
	ubyte*	Even;
	ubyte*	Odd;
} TMGDDICW;

typedef struct
{
	TMGDDICW*	CWPeer;
	ubyte		nKeys;
	ubyte		KeySize;
} TMGDDICWPeers;

/* Demux - Filter definition data struture
 *--------------------------------------------------------------------------*/

/* Type field values */
#define MGDDISection		FALSE
#define MGDDITable			TRUE

/* Size of filter */
#define MGDDIFilterSize		8
typedef struct
{
	uword	PID;
	uword	MaxLen;
	ubyte	Value[MGDDIFilterSize], Mask[MGDDIFilterSize];
	boolean	bTable;			/* See values above */
} TMGDDIFilter;

/* Demux - Filtering mode data structure
 *--------------------------------------------------------------------------*/

/* Mode field values */
#define MGDDIPersistent		FALSE
#define MGDDITrigger		TRUE

typedef struct
{
	udword	TmDelay;
	boolean	bTrigger;
} TMGDDIFilterMode;

/* Demux - Filtered section data structure (notification)
 *--------------------------------------------------------------------------*/

typedef struct
{
	ubyte*	pData;
	uword	Size;
	boolean	bTable;		/* See TMGDDIFilter.Type */
} TMGDDIFilterReport;

/* Source [Descrambler, Demux] interfaces
 *--------------------------------------------------------------------------*/

/* Sources ... */
extern TMGDDIStatus MGDDISrcGetSources
	( TMGSysSrcHandle* lSources, uword* nSources );
extern TMGDDIStatus MGDDISrcGetCaps
	( TMGSysSrcHandle hSource, TMGDDISrcCaps* pCaps );
extern TMGDDIStatus MGDDISrcGetStatus( TMGSysSrcHandle hSource );
extern TMGDDIEventSubscriptionHandle MGDDISrcSubscribe
	( TMGSysSrcHandle hSource, CALLBACK hCallback );

/* Descrambler ... */
extern TMGDDIStatus MGDDISrcGetDscrCaps
	( TMGSysSrcHandle hSource, TMGDDIDscrCaps* pCaps );
extern TMGDDIStatus MGDDISrcInitDscr
	( TMGSysSrcHandle hSource, TMGDDIDscrSelect* pSelPrms );
extern TMGDDIStatus MGDDISrcGetFreeDscrChnl
	( TMGSysSrcHandle hSource, uword* nChannel );
extern TMGDDISrcDscrChnlHandle MGDDISrcAllocDscrChnl( TMGSysSrcHandle hSource );
extern TMGDDIStatus MGDDISrcFreeDscrChnl( TMGDDISrcDscrChnlHandle hChannel );
extern TMGDDIStatus MGDDISrcStartDscrChnl( TMGDDISrcDscrChnlHandle hChannel );
extern TMGDDIStatus MGDDISrcStopDscrChnl( TMGDDISrcDscrChnlHandle hChannel );
extern TMGDDIStatus MGDDISrcFlushDscrChnl( TMGDDISrcDscrChnlHandle hChannel );
extern TMGDDIStatus MGDDISrcLoadCWToDscrChnl
	( TMGDDISrcDscrChnlHandle hChannel, TMGDDICWPeers *pCWPeerTable );
extern TMGDDIStatus MGDDISrcAddCmptToDscrChnl
	( TMGDDISrcDscrChnlHandle hChannel, uword PID );
extern TMGDDIStatus MGDDISrcDelCmptFromDscrChnl
	( TMGDDISrcDscrChnlHandle hChannel, uword PID );

/* Demux ... */
extern TMGDDIStatus MGDDISrcGetFreeDmxFltr
	( TMGSysSrcHandle hSource, uword* nFilter );
extern TMGDDISrcDmxFltrHandle MGDDISrcAllocDmxFltr( TMGSysSrcHandle hSource );
extern TMGDDIStatus MGDDISrcFreeDmxFltr( TMGDDISrcDmxFltrHandle hFilter );
extern TMGDDIStatus MGDDISrcSetDmxFltr
	( TMGDDISrcDmxFltrHandle hFilter, TMGDDIFilter* pFilter );
extern TMGDDIStatus MGDDISrcStartDmxFltr(
	TMGDDISrcDmxFltrHandle hFilter,
	CALLBACK hCallback, TMGDDIFilterMode* pMode, TMGDDISrcDscrChnlHandle pDscr
);
extern TMGDDIStatus MGDDISrcStopDmxFltr( TMGDDISrcDmxFltrHandle hFilter );

/*==========================================================================
  RAM MEMORY SECTION
 *==========================================================================*/

/* RAM memory handle
 *--------------------------------------------------------------------------*/

typedef struct sMGDDIMemHandle*		TMGDDIMemHandle;

/* RAM Memory interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDIMemHandle MGDDIMemAlloc( udword Size );
extern TMGDDIStatus MGDDIMemFree( TMGDDIMemHandle hMem );

/*==========================================================================
  FLASH MEMORY SECTION
 *==========================================================================*/

/* MEDIAGUARD FLASH area ID
 *--------------------------------------------------------------------------*/

extern ubyte MGDDIFlashID[8];

/* FLASH memory handle
 *--------------------------------------------------------------------------*/

typedef struct sMGDDIFlashHandle*	TMGDDIFlashHandle;

/* FLASH Memory interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDIFlashHandle MGDDIFlashOpen( ubyte* FlashID );
extern TMGDDIStatus MGDDIFlashClose( TMGDDIFlashHandle hFlash );
extern TMGDDIStatus MGDDIFlashWrite
	( TMGDDIFlashHandle hFlash, udword Offset, ubyte* pBuffer, udword Size );
extern TMGDDIStatus MGDDIFlashRead
	( TMGDDIFlashHandle hFlash, udword Offset, ubyte* pBuffer, udword* pSize );

/*==========================================================================
  TIMER PERIPHERALS SECTION
 *==========================================================================*/

/* Timer handle
 *--------------------------------------------------------------------------*/

typedef struct sMGDDITimerHandle*	TMGDDITimerHandle;

/* Timer interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDITimerHandle MGDDIDelayStart( udword Delay, CALLBACK hCallback );
extern TMGDDIStatus MGDDIDelayCancel( TMGDDITimerHandle hTimer );

/*==========================================================================
  CLOCK SECTION
 *==========================================================================*/

/* Clock interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDIStatus MGDDIGetTime( time_t* pTime );

/*==========================================================================
  MUTEX SECTION
 *==========================================================================*/

/* Mutex handle
 *--------------------------------------------------------------------------*/

typedef struct sMGDDIMutexHandle*	TMGDDIMutexHandle;

/* Timer interfaces
 *--------------------------------------------------------------------------*/

extern TMGDDIMutexHandle MGDDIMutexCreate( void );
extern TMGDDIStatus MGDDIMutexDestroy( TMGDDIMutexHandle hMutex );
extern TMGDDIStatus MGDDIMutexLock( TMGDDIMutexHandle hMutex );
extern TMGDDIStatus MGDDIMutexUnlock( TMGDDIMutexHandle hMutex );

/*==========================================================================
  UTILITIES SECTION
 *==========================================================================*/

/* Utilities interfaces
 *--------------------------------------------------------------------------*/

extern udword MGDDIGetTickTime( void );
extern void MGDDITraceString( ubyte* pStr );
extern void MGDDITraceSetAsync( boolean bAsync );

/*==========================================================================*/

#endif	/* _MGDDI_H_ */

/*==========================================================================
   END OF FILE
 *==========================================================================*/
