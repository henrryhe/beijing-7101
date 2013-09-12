/*****************************************************************************

File Name: engine.h

Description:

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __ENGINE_H
#define __ENGINE_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stgxobj.h"

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */
#define DEFAULT_DEMO_WIDTH 200
#define DEFAULT_DEMO_HEIGHT 200
#define DEFAULT_DEMO_OFFSETX 20
#define DEFAULT_DEMO_OFFSETY 20
#define DEFAULT_DEMO_COMPLEXITY 20
#define DEFAULT_DEMO_ALPHA 20
#define DEFAULT_DEMO_NBR_OBSTACLES 5
#define DEFAULT_DEMO_MATRIX_SIZE 100
#define DEFAULT_DEMO_INIT_MODE 0
#define DEFAULT_DEMO_DEVICE_NAME "Blitter"


/* Exported Variables ----------------------------------------------------- */

/* Demo Parameters*/
typedef struct
{
    U32 Width;
    U32 Height;
    U32 OffsetX;
    U32 OffsetY;
    U32 Alpha;
    U32 Complexity;
    U32 NbrObstacles;
	U32 InitMode;
	ST_DeviceName_t DeviceName;
	STGXOBJ_Bitmap_t* TargetBitmap_p;
} SnakeDemoParameters_t;

SnakeDemoParameters_t SnakeDemoParameters;

char DemoEngine_Matrix[DEFAULT_DEMO_MATRIX_SIZE][DEFAULT_DEMO_MATRIX_SIZE];

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
void DemoEngine_Init(void) ;
void DemoEngine_InitTarget(void);
int DemoEngine_MoveSnake(void);
void DemoEngine_Term(void);
#endif /* __ENGINE_H */

/* EOF --------------------------------------------------------------------- */
