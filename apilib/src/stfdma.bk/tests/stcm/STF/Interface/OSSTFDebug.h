//
// FILE:      STF\Source\OSAL\OS20\OSSTFDebug.h
// AUTHOR:    Viona
// COPYRIGHT: (c) 1995 Viona Development.  All Rights Reserved.
// CREATED:   04.12.96
//
// PURPOSE:   
//
// HISTORY:	2002-12-09 ported to STF st20lite-version
//

#include <stdio.h>
#include <debug.h>


#if _DEBUG
		//ST20
		#define DPF printf

#endif

//
//  Define breakpoint
//

#ifdef _DEBUG

//#elif ST20LITE
//#define BREAKPOINT		debugbreak()
#define BREAKPOINT		__asm { breakpoint; }
#endif // _DEBUG


