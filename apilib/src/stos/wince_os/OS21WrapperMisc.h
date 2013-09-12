/*! Time-stamp: <@(#)OS21WrapperMisc.h   5/11/2005 - 8:42:41 AM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMisc.h
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose :  Declaration for not classified stuffs
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 * V 0.10  6/15/2005  WH : clean up 

 *
 *********************************************************************
 */

#ifndef __OS21WrapperMisc__
#define __OS21WrapperMisc__


/* This is *only* okay because we never de-reference it. It'll need to
 * be fixed if we ever do.
 */
#define TIMEOUT_INFINITY    ((osclock_t *)NULL)
#define TIMEOUT_IMMEDIATE   ((osclock_t *)-1)
/* File Name */
#define FILENAME_MAX	_MAX_PATH 



/* For register access */

#define REGION2 					0xa0000000			
#define REGION4 					0xe0000000
#define MASK_STBUS_FROM_ST40		0x1fffffff



/* Module majors */
#define STDENC_MAJOR				240
#define STVTG_MAJOR					241
#define STAVMEM_MAJOR				242
#define STINTMR_MAJOR				243
#define STVOUT_MAJOR				244
#define STLAYER_MAJOR				245
#define STVMIX_MAJOR				246
#define STVID_MAJOR					247
#define STPWM_MAJOR					248
#define STPIO_MAJOR					249
#define STBLIT_MAJOR				250

/* Module minor */
#define STAVMEM_MINOR				0

void _WCE_StapiInitialize();
void _WCE_StapiTerminate();
void _WCE_TaskInitilize();
void _WCE_TaskTerminate();
void _WCE_InterruptInitialize();
void _WCE_InterruptTerminate();


// -------------------------------------------------------------------------------
//
//			Prototype for globals
//		
//typedef int ST_DeviceName_t;		
/* ************** */
/* *************** */




#define setbuf(a,b) 1			// Not used with STTBX_InputChar

// ???? 
#define  kernel_chip(void)  "Id not available"

#endif
