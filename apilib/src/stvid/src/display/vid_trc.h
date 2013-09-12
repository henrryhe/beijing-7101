/*******************************************************************************

File name   : vid_trc.h

Description :

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 02 Jan 2003        Created                                   Michel Bruant
*******************************************************************************/

/*
   if the STTBX_PRINT is set then
        TraceInfo and TraceError display text
        trought STTBX_Print function defined in sttbx driver and
        TraceBuffer function do nothing
   else
        TraceInfo, TraceError and TraceBuffer functions display text
        trought TraceBuffer function defined in "trace.h"
*/

#ifdef STTBX_PRINT
    #include "sttbx.h"
    #define TraceInfo(x)  STTBX_Print(x)
    #define TraceError(x)\
            STTBX_Print(("%s,line %d: ERROR: 0x%p\r\n",__FILE__, __LINE__,x))
    #define TraceBuffer(x)
#else
    #include "../tests/src/trace.h"
    #define TraceInfo(x) TraceBuffer(x)
    #define TraceError(x)  \
            TraceBuffer(("%s,line %d: ERROR: 0x%p\r\n",__FILE__, __LINE__,x))
#endif


/******************************************************************************/

