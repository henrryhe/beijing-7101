/*******************************************************************************

File name   : 5510.c

Description : Video HAL (Hardware Abstraction Layer) access to hardware source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
07 Aug 2000        Created                                          AN
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include "stddefs.h"
#include "stgxobj.h"
#include "stlayer.h"
#include "sttbx.h"
#include "stsys.h"
#include "halv_lay.h"
#include "5510-12.h"
#include "hv_layer.h"
#include "hv_reg.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Types ------------------------------------------------------------ */

/* Private Macros ----------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Functions ---------------------------------------------------------------- */
void SetInputMargins(const  STLAYER_Handle_t  LayerHandle,
                            S32 * const LeftMargin_p,
                            S32 * const TopMargin_p,
                            const  BOOL Apply);

/* Global Variables --------------------------------------------------------- */

/*******************************************************************************
Name        : SetInputMargins
Description : If APPLY is TRUE : set input margins
              If APPLY is FALSE : check if input margins are OK, and
                                  set new value in return pointers.
Parameters  :
Assumptions : Margins are in units of 1/8th pixels.
Limitations :
Returns     : Nothing
*******************************************************************************/
void SetInputMargins(const STLAYER_Handle_t LayerHandle,
                     S32 * const LeftMargin_p,
                     S32 * const TopMargin_p,
                     const BOOL Apply)
{
    S32 Scan;
    S32 Pan;
    U8  FractionnalPan;
    U16 IntegerPan;
    U16 DFSValue;

    /*  We have to deal with CIF mode wich is into VID_DFS register and we dont
     * want to change the other bits. DFS is a circular register. */
    DFSValue = (U32)(HAL_Read8(
            ((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16)) & VID_DFS_MASK1)<<8;
    DFSValue |= (U32)(HAL_Read8(
            ((U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_DFS_16)) & VID_DFS_MASK2);

    /************************************/
    /* Setup the SCAN vector (vertical) */
    /************************************/
    /* Note :                                               */
    /* The P&S vectors in the stream are in unit of  1/16 p */
    /* 256 =  16 lines of 16 subpixel                       */
    /* The 5510 core handle SCAN in unit of 16 lines        */
    /* Value rounded to the nearest macroblock              */
    /* The 5512 core handle SCAN in unit of 2 lines         */
    /* Value rounded to the nearest line                    */
    if (*TopMargin_p < 0)
    {
        *TopMargin_p = 0;  /* No support for such scan vectors ! */
    }
    Scan = (*TopMargin_p + 128) / 256;

    /* when in CIF mode The 55XX core handle SCAN in unit of DISPLAYED 16 lines */
    /* => we have to take the Y scaling factor into account  */
    if ((DFSValue & VID_DFS_CIF) != 0)
    {
        Scan *= 2;
    }

    if (Scan > LAYER_MAX_SCN)
    {
        Scan = LAYER_MAX_SCN;
    }

    /*************************************/
    /* Setup the PAN vector (horizontal) */
    /*************************************/

    /* Note : The 55XX core handle PAN in 1/8th pixels */
    if (*LeftMargin_p < 0)
    {
        *LeftMargin_p = 0; /* No support for such pan vectors ! */
    }
    Pan = *LeftMargin_p >> 1;

    FractionnalPan = (U8)(Pan & 0x7);
    IntegerPan     = (U16)(Pan >> 3); /* In pixels */
    if (IntegerPan > LAYER_MAX_PAN)
    {
        IntegerPan = LAYER_MAX_PAN;
    }

    if (!Apply)
    {
        /*  16 * 16 : Macroblocks => line,  line =>  1/16 pix */
        *TopMargin_p = Scan * 256;
        if ((DFSValue & VID_DFS_CIF) != 0)
        {
            *TopMargin_p /= 2;
        }

        *LeftMargin_p = IntegerPan * 16; /* in 16th pixel */
    }
    else
    {
        /* Apply settings */
        HAL_Write16(
            (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_PAN_16,
            IntegerPan & VID_PAN_MASK);
        HAL_Write8(
            (U8 *)((HALLAYER_LayerProperties_t *)LayerHandle)->
            VideoDisplayBaseAddress_p + VID_SCN_8 ,
            Scan & VID_SCN_MASK);
    }
}

