/*****************************************************************************

File Name: blit.h

Description: BLIT configuration

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BLIT_H
#define __BLIT_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"
#include "stcommon.h"
#include "stgxobj.h"
#include "stblit.h"
#include "stos.h"
#include "sttbx.h"

/* Constants ------------------------------------------------------ */

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

#if defined (ST_5100)
#define TICKS_MILLI_SECOND   15625
#elif defined (ST_5105) || defined (ST_5188)
#define TICKS_MILLI_SECOND   6250000
#elif defined (ST_5301)
#define TICKS_MILLI_SECOND   1002345
#elif defined (ST_7109)
#define TICKS_MILLI_SECOND   259277
#elif defined (ST_7100)
#define TICKS_MILLI_SECOND   259277
#elif defined (ST_7710)
#define TICKS_MILLI_SECOND   259277
#elif defined (ST_7200)
#define TICKS_MILLI_SECOND   259277
#endif


#define STBLIT_DEVICE_NAME             "BLITTER"

#if defined(mb317) || defined(mb317a) || defined(mb317b)
	#define TEST_SHARED_MEM_BASE_ADDRESS           NULL	/*For mb317 The SDRAM_BASE_ADDRESS is not defined*/
#else
	#define TEST_SHARED_MEM_BASE_ADDRESS           SDRAM_BASE_ADDRESS
#endif
#define WORK_BUFFER_SIZE   (40 * 16) /* The workbuffer is used in the driver for storing 5-TAP filter coefficients */


#if defined (STBLIT_SOFTWARE)

#define CURRENT_BASE_ADDRESS    (void*)BlitterRegister
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_SOFTWARE;

#elif defined (STBLIT_EMULATOR)
#define CURRENT_BASE_ADDRESS    (void*)BlitterRegister
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION/*STBLIT_DEVICE_TYPE_BDISP_5100*/;

#elif defined (ST_GX1)
#define CURRENT_BASE_ADDRESS    (void*)(0xbb400a00)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_ST40GX1;

#elif defined (ST_7020)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7020;
#define CURRENT_BASE_ADDRESS    (void*)(STI7020_BASE_ADDRESS + ST7020_GAMMA_OFFSET + ST7020_GAMMA_BLIT_OFFSET);

#elif defined (ST_5528)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_5528;
#define CURRENT_BASE_ADDRESS    (void*)(ST5528_BLITTER_BASE_ADDRESS);

#elif defined (ST_5100)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5100;
#define CURRENT_BASE_ADDRESS    (void*)(ST5100_BLITTER_BASE_ADDRESS);

#elif defined (ST_5105)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5105;
#define CURRENT_BASE_ADDRESS    (void*)(ST5105_BLITTER_BASE_ADDRESS);

#elif defined (ST_5301)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5301;
#define CURRENT_BASE_ADDRESS    (void*)(ST5301_BLITTER_BASE_ADDRESS);

#elif defined (ST_5525)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_5525;
#define CURRENT_BASE_ADDRESS    (void*)(ST5525_BLITTER_BASE_ADDRESS);

#elif defined (ST_7100)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7100;
#define CURRENT_BASE_ADDRESS    (void*)(ST7100_BLITTER_BASE_ADDRESS);

#elif defined (ST_7109)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_7109;
#define CURRENT_BASE_ADDRESS    (void*)(ST7109_BLITTER_BASE_ADDRESS);

#elif defined (ST_7200)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_BDISP_7200;
#define CURRENT_BASE_ADDRESS    (void*)(ST7200_BLITTER_BASE_ADDRESS);

#else
#define CURRENT_BASE_ADDRESS    (void*)(0x6000AA00)
#define CURRENT_DEVICE_TYPE     STBLIT_DEVICE_TYPE_GAMMA_7015;

#endif


#if defined (ST_7015)
#define BLITTER_INT_EVENT ST7015_BLITTER_INT
#elif defined (ST_7020)
#define BLITTER_INT_EVENT ST7020_BLITTER_INT
#elif defined (ST_GX1)
#define BLITTER_INT_EVENT 0
#elif defined (ST_5528)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT ST5528_BLITTER_INTERRUPT
#elif defined (ST_5100)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5100_BLIT_AQ1_INTERRUPT
#elif defined (ST_5105)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5105_BLIT_AQ1_INTERRUPT
#elif defined (ST_5301)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5301_BLIT_AQ1_INTERRUPT
#elif defined (ST_5525)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST5525_BLIT_AQ1_INTERRUPT
#elif defined(ST_7109)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST7109_BLITTER_INTERRUPT
#elif defined(ST_7200)
#define BLITTER_INT_LEVEL 5
#define BLITTER_INT_EVENT ST7200_BLITTER_AQ1_INTERRUPT
#elif defined (ST_7710)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT ST7710_BLT_REG_INTERRUPT
#elif defined (ST_7100)
#define BLITTER_INT_LEVEL 0
#define BLITTER_INT_EVENT  ST7100_BLITTER_INTERRUPT
#endif



/* Exported Variables ----------------------------------------------------- */
/* to control Blit mode */
BOOL WaitForBlitCompletetion;

/* Global variables----------------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t DemoBlit_Setup(ST_Partition_t *SystemPartition_p, STAVMEM_PartitionHandle_t AVMEMPartition,
                              ST_DeviceName_t EvtDeviceName, ST_DeviceName_t IntTMRDeviceName, STEVT_SubscriberID_t ExternSubscriberID);
ST_ErrorCode_t DemoBlit_Term(void);
ST_ErrorCode_t DemoBlit_FillRectangle(STGXOBJ_Bitmap_t *DstBitmap,int x, int y, int w, int h, STGXOBJ_Color_t Color);
ST_ErrorCode_t DemoBlit_CopyRectangle(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int src_x,int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h);
ST_ErrorCode_t DemoBlit_ReseizeBitmap(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int dst_x,int dst_y,int dst_w,int dst_h);
ST_ErrorCode_t DemoBlit_ReseizeRectangle(STGXOBJ_Bitmap_t SrcBitmap,STGXOBJ_Bitmap_t *DstBitmap,int src_x,int src_y, int src_w,int src_h,int dst_x,int dst_y,int dst_w,int dst_h);
ST_ErrorCode_t DemoBlit_Term(void);

#endif /* __BLIT_H */

/* EOF --------------------------------------------------------------------- */
