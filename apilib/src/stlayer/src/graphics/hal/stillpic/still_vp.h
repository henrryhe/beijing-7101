/*******************************************************************************

File name   : 

Description : definitions used in layer context. 

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STILL_VP_H 
#define __STILL_VP_H 

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgxobj.h"

/* Types -------------------------------------------------------------------- */

typedef struct
{   
    BOOL                    Open;
    BOOL                    Enabled;
    U32                     Alpha;
    STGXOBJ_Rectangle_t     InputRectangle;
    STGXOBJ_Rectangle_t     OutputRectangle;
    STGXOBJ_Bitmap_t        Bitmap;
}still_ViewportDesc;


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef  __STILL_VP_H */

