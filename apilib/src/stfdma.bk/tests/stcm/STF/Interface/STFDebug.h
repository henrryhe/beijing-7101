///
/// @file STF\Interface\STFDebug.h
///
/// @brief OS-dependent debug-methods 
///
/// @par OWNER: 
/// STF-Team
/// @author GB    
///
/// @par SCOPE:
///	EXTERNAL Header File
///
/// @date       2003-03-03 changed header
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#ifndef _STFDEBUG_
#define _STFDEBUG_

#include "STF/Interface/Types/STFResult.h"
#include "OSSTFDebug.h"
#include "string.h"

void MDebugPrint(const char * szFormat, ...);
#define RDP MDebugPrint

/*! Empty debug function,
    in non-debug prints DP is mapped to this, in debug prints you can manually
	 map DP to this if you want to exclude a specific file from debug prints.
*/
inline void DebugPrintEmpty(const char * szFormat, ...) {}		// empty function (optimized to nothing)

#define DP_EMPTY while(0) DebugPrintEmpty


#if _DEBUG

	void DebugPrint (const char  * szFormat, ...);	// standard prototype for debug output
	#define DP DebugPrint
	void DebugPrintRecord (const char  * szFormat, ...);	// standard prototype for debug output
	#define DPR DebugPrintRecord
	void InitializeDebugRecording (void);
	void WriteDebugRecording (char *filename);

	// Logging allows to "print" according to levels or channels.
	void DebugLog (uint32 id, const char  * szFormat, ...);
	#define DEBUGLOG DebugLog

	#define DPDUMP DebugPrintDump // see STFGenericDebug.cpp
	void DebugPrintDump(unsigned char *bp, int len, int maxlen = 0, bool dumpASCII = false, uint32 startAddr = 0);

	/// Debug print with thread name in front (only call from an STFThread)
	#define DPTN DebugPrintWithThreadName
	void DebugPrintWithThreadName (const char  * szFormat, ...);	// standard prototype for debug output

#else
		#define DP while(0) DebugPrintEmpty
		#define DPF while(0) DebugPrintEmpty
		#define DPR while(0) DebugPrintEmpty
		#define DEBUGLOG while(0) DebugPrintEmpty
		#define DPDUMP while(0) DebugPrintEmpty // DebugPrintEmpty compiles because param1 is a char *
		#define DPTN while(0) DebugPrintEmpty
		inline void InitializeDebugRecording (void) {}
		inline void WriteDebugRecording (char *filename) {}

		//
#endif


// IDs for the logging mechanism.
#define LOGID_ERROR_LOGGING	3		// may become changed
#define LOGID_BLOCK_LOGGING	4		// may become changed


#ifndef _DEBUG
#define BREAKPOINT		do {} while (0)
#endif // !_DEBUG

#ifdef _DEBUG
#define DEBUG_EXECUTE(x) x
#else
#define DEBUG_EXECUTE(x) do {} while (0)
#endif

#ifndef ASSERT
#define ASSERT(cond) if (!(cond)) {DP("ASSERTION FAILED: (%s, %d) : "  #cond "!", __FILE__, __LINE__); BREAKPOINT;} else while (0)
#endif

#if !UPDATE_UTILITY_BUILD
#define DEBUG__(x)		x
#else
#define DEBUG__(x)
#endif


#endif
