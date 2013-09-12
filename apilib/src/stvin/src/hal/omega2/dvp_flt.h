/*******************************************************************************

File name   : dvp_flt.h

Description : Contains filters tables

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                               Name
----               ------------                               ----
28 Nov 2001       Creation                                   Michel Bruant

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DVP_FLT_H
#define __DVP_FLT_H

/* Note :   */
/* filters coefs may be re-calculated for optimized ratio using the         */
/* following tool : stvin/tools/filters.c, the source neede to be commpiled */
/* and ran on your host */
/* If the coef has been changed, you need also to recode the filer selection */
/* in file hv_dvp.c                                                          */

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define NB_VSRC_FILTERS         5
#define NB_VSRC_PHASES          8
#define NB_VSRC_TAPS            3
#define NB_VSRC_COEFFS          (NB_VSRC_TAPS * NB_VSRC_PHASES)

#define NB_HSRC_FILTERS         5
#define NB_HSRC_PHASES          8
#define NB_HSRC_TAPS            5
#define NB_HSRC_COEFFS          (NB_HSRC_TAPS * NB_HSRC_PHASES)

/* Exported Variables ------------------------------------------------------- */
const S8 stvin_VSRC_Coeffs[NB_VSRC_FILTERS * NB_VSRC_COEFFS]  =
{
    /* calculated with resize = x1.00 */
    0 ,     64,     0, 
    9 ,     61,     -6,
    19,     55,     -10,
    29,     47,     -12,
    38,     38,     -12,
    47,     27,     -10,
    54,     17,     -7,
    59,     8 ,     -3,
    /* calculated with resize = x0.70 */
    9 ,      46,      9   ,
    15,      46,      3   ,
    22,      43,      -1  ,
    29,      40,      -5  ,
    37,      35,      -8  ,
    44,      30,      -10 ,
    51,      24,      -11 ,
    57,      17,      -10 ,
    /* calculated with resize = x0.50*/
    16,      32,      16 ,
    19,      32,      13 ,
    23,      31,      10 ,
    26,      31,      7  ,
    30,      30,      4  ,
    34,      29,      1  ,
    38,      27,      -1 ,
    43,      25,      -4 ,
    /* calculated with resize = x0.25 */
    20,      24,      20 ,
    21,      24,      19 ,
    22,      23,      19 ,
    23,      23,      18 ,
    23,      24,      17 ,
    24,      24,      16 ,
    25,      24,      15 ,
    26,      23,      15 ,
    /* calculated with resize = x0.10 */
    21,      22,      21 ,
    21,      22,      21 ,
    21,      22,      21 ,
    22,      21,      21 ,
    22,      21,      21 ,
    22,      21,      21 ,
    22,      22,      20 ,
    22,      22,      20 
};

const S8 stvin_HSRC_Coeffs[NB_HSRC_FILTERS * NB_HSRC_COEFFS]  =
{
    /* calculated with resize = x1.00 */
    0  ,     0 ,      64,      0  ,     0 ,
    -3 ,     9 ,      60,      -6 ,     4 ,
    -7 ,     19,      57,      -11,     6 ,
    -11,     31,      49,      -13,     8 ,
    -13,     42,      40,      -13,     8 ,
    -13,     52,      29,      -11,     7 ,
    -11,     59,      18,      -7 ,     5 ,
    -6 ,     63,      7 ,      -3 ,     3 ,
    /* calculated with resize = x0.70 */
    -9 ,     11,      60,      11 ,     -9 ,
    -11,     19,      58,      4  ,     -6 ,
    -12,     28,      52,      -2 ,     -2 ,
    -11,     35,      46,      -6 ,     0  ,
    -9 ,     41,      38,      -9 ,     3  ,
    -6 ,     45,      30,      -10,     5  ,
    -1 ,     48,      21,      -10,     6  ,
    3  ,     49,      15,      -9 ,     6  ,
    /* calculated with resize = x0.50 */
    -5,      19,      36,      19,      -5 ,
    -3,      23,      35,      15,      -6 ,
    -1,      26,      35,      11,      -7 ,
    1 ,      29,      33,      8 ,      -7 ,
    4 ,      32,      31,      4 ,      -7 ,
    7 ,      34,      28,      1 ,      -6 ,
    11,      35,      24,      -1,      -5 ,
    15,      36,      20,      -3,      -4 ,
    /* calculated with resize = x0.25 */
    9 ,      15,      16,      15,      9 ,
    10,      15,      17,      14,      8 ,
    10,      16,      17,      14,      7 ,
    11,      16,      18,      13,      6 ,
    12,      17,      18,      12,      5 ,
    13,      18,      17,      12,      4 ,
    14,      18,      17,      11,      4 ,
    15,      19,      17,      10,      3 ,
    /* calculated with resize = x0.10 */
    12,      13,      14,      13,      12 ,
    12,      13,      14,      13,      12 ,
    12,      13,      14,      13,      12 ,
    13,      13,      13,      13,      12 ,
    13,      13,      13,      13,      12 ,
    13,      13,      14,      13,      11 ,
    13,      14,      13,      13,      11 ,
    13,      14,      13,      13,      11 
};

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_DVP_H */
