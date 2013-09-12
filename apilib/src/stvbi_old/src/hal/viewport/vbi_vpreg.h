/*******************************************************************************
File Name   : vbihd_reg.h

Description : DENC V3/V5/V6/V7/V8/V9/V10/V11 registers

COPYRIGHT (C) STMicroelectronics 2000

Date               Modification                                      Name
----               ------------                                      ----
02 Dec 2005        Created                                           SBEBA
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VBIHD_REG_H
#define __VBIHD_REG_H

#include "vbi_drv.h"

/* Includes ----------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */


#define CGMS_DATA_VALUE                     0x80F0F0F0
#define CGMS_NO_DATA_VALUE                  0x800F0F0F
#define CGMS_DATA_LENGTH                    22
#define CGMS_TYPE_B_DATA_LENGTH             136

#define CGMS_VIEWPORT_MAX_WIDTH             1920
#define CGMS_VIEWPORT_MAX_HEIGHT            2
#define CGMS_START_SYMBOL                   0x00200000
#define CGMS_TYPE_B_START_SYMBOL            0x80000000


                /* 1080I mode */

#define CGMS_1080I_WITH                     1920          /* With of 1080I viewport               */
#define CGMS_1080I_HEIGHT                   2             /* Height of 1080I viewport             */
#define CGMS_1080I_START_SYM_POS            117           /* Start  Symbol Position               */
#define CGMS_1080I_HEADER_SYM_WIDTH         77            /* Height of 1080I viewport             */
#define CGMS_1080I_POSXIN                   0             /* Position  Start Xin                  */
#define CGMS_1080I_POSYIN                   0             /* Position  Start Yin                  */
#define CGMS_1080I_POSXOUT                  0            /* Position  Start Xout                */
#define CGMS_1080I_POSYOUT                  -4             /* Position  Start Yout                */


                /* 720P mode */
#define CGMS_720P_WITH                      1280          /* With of 1080I viewport               */
#define CGMS_720P_HEIGHT                    1             /* Height of 1080I viewport             */
#define CGMS_720P_START_SYM_POS             4           /* Start  Symbol Position               */
#define CGMS_720P_HEADER_SYM_WIDTH          58            /* Height of 1080I viewport             */
#define CGMS_720P_POSXIN                    0           /* Position  Start X                    */
#define CGMS_720P_POSYIN                    0             /* Position  Start Y                    */
#define CGMS_720P_POSXOUT                   0          /* Position  Start Xout                 */
#define CGMS_720P_POSYOUT                   -2             /* Position  Start Yout                */

                /* 480P mode */
#define CGMS_480P_WITH                      720          /* With of 1080I viewport               */
#define CGMS_480P_HEIGHT                    1             /* Height of 1080I viewport             */
#define CGMS_480P_START_SYM_POS             33           /* Start  Symbol Position               */
#define CGMS_480P_HEADER_SYM_WIDTH          26            /* Height of 1080I viewport             */
#define CGMS_480P_POSXIN                    0           /* Position  Start X                    */
#define CGMS_480P_POSYIN                    0             /* Position  Start Y                    */
#define CGMS_480P_POSXOUT                   0          /* Position  Start Xout                 */
#define CGMS_480P_POSYOUT                   -2             /* Position  Start Yout                */



                /* 1080I mode type B */

#define CGMS_TYPE_B_1080I_WITH                     1920          /* With of 1080I viewport               */
#define CGMS_TYPE_B_1080I_HEIGHT                   2             /* Height of 1080I viewport             */
#define CGMS_TYPE_B_1080I_START_SYM_POS            117           /* Start  Symbol Position               */
#define CGMS_TYPE_B_1080I_HEADER_SYM_WIDTH         10            /* Height of 1080I viewport             */
#define CGMS_TYPE_B_1080I_POSXIN                   0             /* Position  Start Xin                  */
#define CGMS_TYPE_B_1080I_POSYIN                   0             /* Position  Start Yin                  */
#define CGMS_TYPE_B_1080I_POSXOUT                  0            /* Position  Start Xout                */
#define CGMS_TYPE_B_1080I_POSYOUT                  -6             /* Position  Start Yout                */


                /* 720P mode type B */
#define CGMS_TYPE_B_720P_WITH                      1280          /* With of 1080I viewport               */
#define CGMS_TYPE_B_720P_HEIGHT                    1             /* Height of 1080I viewport             */
#define CGMS_TYPE_B_720P_START_SYM_POS             4           /* Start  Symbol Position               */
#define CGMS_TYPE_B_720P_HEADER_SYM_WIDTH          8            /* Height of 1080I viewport             */
#define CGMS_TYPE_B_720P_POSXIN                    0           /* Position  Start X                    */
#define CGMS_TYPE_B_720P_POSYIN                    0             /* Position  Start Y                    */
#define CGMS_TYPE_B_720P_POSXOUT                   0          /* Position  Start Xout                 */
#define CGMS_TYPE_B_720P_POSYOUT                   -3             /* Position  Start Yout                */

                /* 480P mode type B */
#define CGMS_TYPE_B_480P_WITH                      720          /* With of 1080I viewport               */
#define CGMS_TYPE_B_480P_HEIGHT                    1             /* Height of 1080I viewport             */
#define CGMS_TYPE_B_480P_START_SYM_POS             33           /* Start  Symbol Position               */
#define CGMS_TYPE_B_480P_HEADER_SYM_WIDTH          4            /* Height of 1080I viewport             */
#define CGMS_TYPE_B_480P_POSXIN                    0           /* Position  Start X                    */
#define CGMS_TYPE_B_480P_POSYIN                    0             /* Position  Start Y                    */
#define CGMS_TYPE_B_480P_POSXOUT                   0          /* Position  Start Xout                 */
#define CGMS_TYPE_B_480P_POSYOUT                  -3             /* Position  Start Yout                */




/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

        /* FullCGMS */
ST_ErrorCode_t stvbi_HalAllocResource( VBIDevice_t * const Device_p);
ST_ErrorCode_t stvbi_HalFreeResource( VBIDevice_t * const Device_p);
ST_ErrorCode_t stvbi_HalInitFullCgmsViewport( VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HalCloseFullCgmsViewport( VBI_t * const Vbi_p);
ST_ErrorCode_t stvbi_HalWriteDataOnFullCgmsViewport( VBI_t * const Vbi_p, const U8* const Data_p);
ST_ErrorCode_t stvbi_HalWriteDataOnFullCgmsViewportTypeB( VBI_t * const Vbi_p, const U8* const Data_p);



#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __VBI_DENC_REG_H */

/* End of vbihd_reg.h */


