/*******************************************************************************

File name   : bdisp_slc.c

Description : blitter slice source file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
29 June 2004       Created                                          HE
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <string.h>
#endif
#include "stddefs.h"
#include "stblit.h"
#include "stgxobj.h"

#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "bdisp_com.h"
#include "bdisp_mem.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#define STBLIT_INCREMENT_NB_BITS_PRECISION  10
#define STBLIT_TAP_NUMBER_FILTER            5

/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */



/*
--------------------------------------------------------------------------------
API Get slice buffer function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetSliceRectangleBuffer(
    STBLIT_Handle_t                  Handle,
    STBLIT_SliceRectangleBuffer_t*   Buffer_p)
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Buffer_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*
--------------------------------------------------------------------------------
API Blit slice rectangle function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_BlitSliceRectangle(
    STBLIT_Handle_t           Handle,
    STBLIT_SliceData_t*       Src1_p,
    STBLIT_SliceData_t*       Src2_p,
    STBLIT_SliceData_t*       Dst_p,
    STBLIT_SliceRectangle_t*  SliceRectangle_p,
    STBLIT_BlitContext_t*     BlitContext_p)
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Src1_p);
    UNUSED_PARAMETER(Src2_p);
    UNUSED_PARAMETER(Dst_p);
    UNUSED_PARAMETER(SliceRectangle_p);
    UNUSED_PARAMETER(BlitContext_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}


/*
--------------------------------------------------------------------------------
API Blit Get slice rectangle number function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetSliceRectangleNumber(
    STBLIT_Handle_t                  Handle,
    STBLIT_SliceRectangleBuffer_t*   Buffer_p)
{
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(Buffer_p);

    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}




/* End of blt_slc.c */

