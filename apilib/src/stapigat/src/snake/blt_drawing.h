/*****************************************************************************

File Name  : drawing.h

Description: drawing header

Copyright (C) 2006 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DRAWING_H
#define __DRAWING_H
/* Exported Constants ------------------------------------------------------- */

/* Includes --------------------------------------------------------------- */
#include "stgxobj.h"

#if defined (ST_5100)
#define TICKS_PER_MILLI_SECOND   15625
#elif defined (ST_5105)
#define TICKS_PER_MILLI_SECOND   6250000
#elif defined (ST_5301)
#define TICKS_PER_MILLI_SECOND   1002345
#elif defined (ST_7109)
#define TICKS_PER_MILLI_SECOND   259277
#elif defined (ST_7100)
#define TICKS_PER_MILLI_SECOND   259277
#elif defined (ST_7710)
#define TICKS_PER_MILLI_SECOND   259277
#elif defined (ST_7200)
#define TICKS_PER_MILLI_SECOND   259277
#endif


/* Exported Constants ------------------------------------------------------- */

/* Exported Functions ---------------------------------------------------------- */
ST_ErrorCode_t DemoDraw_DrawMatrix(STGXOBJ_Bitmap_t *DstBitmap);
ST_ErrorCode_t DemoDraw_ClearOSDBitmap(STGXOBJ_Bitmap_t *DstBitmap);

#endif /*__DRAWING_H */

