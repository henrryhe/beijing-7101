// This file abstracts Multicom/mme.h for debugging of ACC_Multicom Transformers

#ifndef _ACC_MME_H_
#define _ACC_MME_H_

#define	_ACC_WRAPP_MME_	0
//#define _USB_WRITE_ 1
//#define _USB_BUF_ 1

#if _ACC_WRAPP_MME_
#ifndef _MME_WRAPPER_

#ifndef ST_OSLINUX
#define MME_Init				acc_MME_Init
#endif
#define MME_InitTransformer			acc_MME_InitTransformer
#define MME_SendCommand				acc_MME_SendCommand
#define MME_AbortCommand			acc_MME_AbortCommand
#define MME_TermTransformer			acc_MME_TermTransformer
#define MME_GetTransformerCapability		acc_MME_GetTransformerCapability



#endif
#endif // _ACC_WRAPP_MME_
#if defined(ST_51XX) || defined(ST_5197)
#include "mme_interface.h"
#else
#include "mme.h"
#endif

#endif // _ACC_MME_H_


