/*! Time-stamp: <@(#)OS21WrapperConsole.h   6/6/2005 - 4:00:35 PM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperConsole.h
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 6/6/2005
 *
 *  Purpose :  Overload of Console functions
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  6/6/2005  WH : First Revision
 *
 *********************************************************************
 */

#ifndef __OS21WrapperConsole__
#define __OS21WrapperConsole__

#include "KitlIO\Kitl_IO.h"
#include "UDPIO\UDP_IO.h"
#include "SERIALIO\Serial_IO.h"


/* Input functions */



// if master, no overload of Console, will be implemented by Stdio

#ifndef WINCE_MASTER
		
	#ifndef WCE_USE_CONSOLE
	#define WCE_USE_CONSOLE 0  // defaul is serial
	#endif

	#if WCE_USE_CONSOLE == 0
#ifndef STAPI_QUIET
		#pragma message ("Use_Serial")
#endif
		#define	Init_Console_Connection	wce_InitDebugSerial
		#define	Term_Console_Connection()		{};
		#define printf					wce_sdio_printf
		#define Wce_Console_Getchar		wce_sdio_getchar
	#endif


	#if WCE_USE_CONSOLE == 1
#ifndef STAPI_QUIET
		#pragma message ("Use_Kitl")
#endif
		#define	Init_Console_Connection	Wce_KitlIO_Init
		#define	Term_Console_Connection	Wce_KitlIO_Term
		#define printf					Wce_KitlIO_Printf	
		#define Wce_Console_Getchar		Wce_KitlIO_Getchar


	#endif

	#if WCE_USE_CONSOLE == 2
#ifndef STAPI_QUIET
		#pragma message ("Use UDP")
#endif
		#define	Init_Console_Connection	Wce_UDPIO_Init
		#define	Term_Console_Connection	Wce_UDPIO_Term
		#define printf					Wce_UDPIO_Printf
		#define Wce_Console_Getchar		Wce_UDPIO_Getchar

	#endif
#else
#ifndef STAPI_QUIET
		#pragma message ("Master Mode")
#endif
		#define	Init_Console_Connection()	
		#define	Term_Console_Connection()
		#define Wce_Console_Getchar(a) (0)
#endif


#endif
