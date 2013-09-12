/*! Time-stamp: <@(#)OS21WrapperMisc.c   5/11/2005 - 8:45:20 AM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMisc.c
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose :  Implementation of Not classified stuffs
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 * V 0.10  5/20/2005 OLGN : Add KitlIO.lib exported functions
 * V       5/26/2005  JLX  : removed KITL & STTBX 
 *
 *********************************************************************
 */
#include "OS21WrapperMisc.h"

long int debugfilesize (long int FileDescriptor)
{
#if 0
	// file should be open with open stdlib
	unsigned long off = _lseek(FileDescriptor,0,SEEK_CUR);

	struct stat StatBuf;
	
	fstat (FileDescriptor, &StatBuf);
	
	return (StatBuf.st_size);
#endif 
	WCE_NOT_IMPLEMENTED();
	return 0;
}



/*!-------------------------------------------------------------------------------------
 * Must be called from STBOOT, here is the function that init all WCE stuffs 
 *
 * @param none
 *
 * @return void  : 
 */
     
void _WCE_StapiInitialize()
{
	_WCE_MemoryInitilize();
	_WCE_TaskInitilize();
	_WCE_InterruptInitialize();
	
	Init_Console_Connection();
/*	
	_WCE_SetTraceMode(1);
	_WCE_SetMsgLevel(MDL_OSCALL);

	_WCE_SetMessageLogFile("Release/message.log");
	_WCE_SetMsgLevel(MDL_CALLL);
	_WCE_SetMessageLogFile("Release/message.log");
	_WCE_SetConsoleLogFile("Release/Console.log");

  */



}

/*!-------------------------------------------------------------------------------------
 * Must be called from STBOOT, clean up WCE stuffs
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_StapiTerminate()
{
	Term_Console_Connection();
	_WCE_MemoryTerminate();
	_WCE_TaskTerminate();
  _WCE_InterruptTerminate();
}

