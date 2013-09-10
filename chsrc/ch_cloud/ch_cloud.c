

/*****************************************************************************

File Name   : ch_cloud.c
Description : 增加视博云而增加的接口

COPYRIGHT (C) ChangHong 2006.

*****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "stdenc.h"
#include "stdevice.h"
#include "sthdmi.h"
#include "stsys.h"
#include "sttbx.h"
#include "stvout.h"
#include "..\main\initterm.h"
#include "ch_cloud.h"
#include "superusbhid_acq.h"
#define DEBUG_PRINT STTBX_Print
/* Private Types -------------------------------------------------------------------- */
#ifndef DU32
typedef volatile unsigned int DU32;
#endif

C_BOOL USBHIDACQ_Start(void)
{
	return CLOUD_OK;
}

void USBHIDACQ_OutputReport(C_U32  handle,
               					C_U8     *output_buffer,
                             	C_U16     output_len)
{
	return;
}

void USBHIDACQ_SetConnectCallback(ConnectCallbackFunc callback )
{
	return;
}

void USBHIDACQ_SetDisconnectCallback(DisconnectCallbackFunc callback )
{
	return;
}

void USBHIDACQ_SetInputReportCallback(InputReportCallbackFunc callback )
{
	return;
}

void USBHIDACQ_SetFlingpcInputCallback(FlingpcInputCallbackFunc callback )
{
	return;
}

void USBHIDACQ_Stop( void )
{
	return;
}





