/*******************************************************************************

File name   : blt_cmd.h

Description : STBLIT commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2004.

Date                     Modification                                 Name
----                     ------------                                 ----
20 September 2003        Created                                      HE
30 May 2006              Updated for 7109 device					WinCE Team-ST Noida
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLIT_CMD_H
#define __BLIT_CMD_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stblit.h"
#include "stcommon.h"
#include "stevt.h"
#include "stavmem.h"
#include "stsys.h"
#include "stos.h"
#include "testtool.h"
#include "clevt.h"





/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */
#ifndef STTBX_PRINT
#define STTBX_PRINT 1
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

#define DEFAULT_EOF_VALUE 0x0FFF5FFF





/* Exported Types ----------------------------------------------------------- */
typedef struct
{
    void *ptr;
    STAVMEM_BlockHandle_t BlockHandle;
} Allocate_KeepInfo_t;

typedef U16 Allocate_Number_t;

/* Header of Bitmap file  */
typedef struct
{
    U16 Header_size;    /* In 32 bit word      */
    U16 Signature;      /* usualy 0x444F       */
    U16 Type;           /* See previous define */
    U16 Properties;     /* Usualy 0x0          */
    U32 width;
    U32 height;
    U32 nb_pixel;       /* width*height        */
    U32 zero;           /* Usualy 0x0          */
} GUTIL_GammaHeader;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
BOOL BLIT_RegisterCmd(void);

#endif /* #ifndef __BLIT_CMD_H */

/* End of blit_cmd.h */
