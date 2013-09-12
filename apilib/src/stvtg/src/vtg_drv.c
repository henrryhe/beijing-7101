/*******************************************************************************

File name   : vtg_drv.c

Description : VTG driver module specific functions source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
29 Sep 2000        Created                                          HSdLM
16 Nov 2000        Improve error tracking                           HSdLM
18 Jan 2001        Unit of VSyncPulseWidth is half line !           HSdLM
21 Mar 2001        Add pixel clock and digital xy start in table,   HSdLM
 *                 remove useless timing modes from table,
 *                 NTSC ActiveAreaHeight : 485 -> 480,
 *                 turn a SMPTE274Mto SMPTE295M,
 *                 new STVTG_<Set/Get> OptionalConfiguration
28 Jun 2001        Enforce handle validity checking                 HSdLM
 *                 Set prefix stvtg_ on exported symbols
 *                 Reorganization of optional parameters management
19 Oct 2001        Fix DDTS GNBvd09294 (3 lines up).                HSdLM
20 Feb 2002        DDTS GNBvd09294 : Display positioning            HSdLM
 *                 calibration tuned for some modes on STi7015
16 Apr 2002        New DeviceType for STi7020                       HSdLM
08 Jul 2002        Add debug statistics management.                 HSdLM
20 Jan 2003        Remove fix of DDTS GNBvd09294 (3 lines up).      BS
16 Apr 2003        Add support of STi5528 (VFE2).                   HSdLM
02 Jun 2003        New format 1080i(1250)                           HSdLM
13 Sep 2004        Add ST40/OS21 support                            MH
16 May 2007        Add sync calibration API for 7710/710x           ICH
 *******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <string.h>
#endif
#include "stddefs.h"
#include "stgxobj.h"
#include "stvtg.h"
#include "vtg_drv.h"
#include "vtg_ihal.h"



/* Private Types ------------------------------------------------------------ */

#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
typedef struct ModeParamsLineTuned_s
{
    STVTG_TimingMode_t   Mode;
    S8                   DeltaDigitalActiveAreaXStart;
    S8                   DeltaDigitalActiveAreaYStart;
} ModeParamsLineTuned_t;
#endif /* #ifdef STVTG_VOS || STVTG_VFE || STVTG_VFE2*/

/* Private Constants -------------------------------------------------------- */

#define SCAN_I STGXOBJ_INTERLACED_SCAN
#define SCAN_P STGXOBJ_PROGRESSIVE_SCAN

/* Private Variables (static)------------------------------------------------ */

/* {Mode},
 * {FrameRate, ScanType, ActiveAreaWidth, ActiveAreaHeight, ActiveAreaXStart, ActiveAreaYStart, DigitalActiveAreaXStart, DigitalActiveAreaYStart},
 * {PixelsPerLine, LinesByFrame, PixelClock, HSyncPolarity, HSyncPulseWidth, VSyncPolarity, VSyncPulseWidth}
*/
static ModeParamsLine_t ModeParamsTable[] =
    {
    { STVTG_TIMING_MODE_SLAVE,             {    0, SCAN_I,    0,    0,   0,  0,   0,   0}, {    0,    0,         0, FALSE,  0, FALSE,  0}},
    /* SD modes */
    { STVTG_TIMING_MODE_480I60000_13514,   {60000, SCAN_I,  720,  480, 122, 20, 119,  38}, {  858,  525,  13513500, FALSE, 62, FALSE,  6}},
    { STVTG_TIMING_MODE_480P60000_27027,   {60000, SCAN_P,  720,  480, 122, 37, 122,  37}, {  858,  525,  27027000, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480P30000_13514,   {30000, SCAN_P,  720,  480, 122, 40, 122,  38}, {  858,  525,  13513500, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480P24000_10811,   {24000, SCAN_P,  720,  480, 122, 40, 122,  38}, {  858,  525,  10810800, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480I59940_13500,   {59940, SCAN_I,  720,  480, 122, 20, 119,  39}, {  858,  525,  13500000, FALSE, 62, FALSE,  6}},
    { STVTG_TIMING_MODE_480P59940_27000,   {59940, SCAN_P,  720,  480, 122, 37, 122,  37}, {  858,  525,  27000000, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480P29970_13500,   {29970, SCAN_P,  720,  480, 122, 40, 138,  38}, {  858,  525,  13500000, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480P23976_10800,   {23976, SCAN_P,  720,  480, 122, 40, 138,  38}, {  858,  525,  10800000, FALSE, 62, FALSE, 12}},
    { STVTG_TIMING_MODE_480I60000_12285,   {60000, SCAN_I,  640,  480, 111, 20, 119,  40}, {  780,  525,  12285000, FALSE, 59, FALSE,  6}},
    { STVTG_TIMING_MODE_480P60000_24570,   {60000, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,  24570000, FALSE, 59, FALSE, 12}},
    { STVTG_TIMING_MODE_480P30000_12285,   {30000, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,  12285000, FALSE, 59, FALSE, 12}},
    { STVTG_TIMING_MODE_480P24000_9828,    {24000, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,   9828000, FALSE, 59, FALSE, 12}},
    { STVTG_TIMING_MODE_480I59940_12273,   {59940, SCAN_I,  640,  480, 111, 20, 119,  40}, {  780,  525,  12272727, FALSE, 59, FALSE,  6}},
    { STVTG_TIMING_MODE_480P59940_24545,   {59940, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,  24545454, FALSE, 59, FALSE, 12}},
    { STVTG_TIMING_MODE_480P29970_12273,   {29970, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,  12272727, FALSE, 59, FALSE, 12}},
    { STVTG_TIMING_MODE_480P23976_9818,    {23976, SCAN_P,  640,  480, 111, 40, 122,  38}, {  780,  525,   9818181, FALSE, 59, FALSE, 12}},
     /*Test 640*480p = Mode VGA*/
    { STVTG_TIMING_MODE_480P60000_25200,   {60000, SCAN_P,  640,  480, 144, 36, 144,  36}, {  800,  525,  25200000, FALSE, 96, FALSE, 4}},
    { STVTG_TIMING_MODE_480P59940_25175,   {59940, SCAN_P,  640,  480, 144, 36, 144,  36}, {  800,  525,  25175000, FALSE, 96, FALSE, 4}},
    /*1024*768P XGA modes */
    { STVTG_TIMING_MODE_768P75000_78750,   {75000, SCAN_P, 1024,  768, 259, 32, 259,  32}, { 1312,  800,  78750000, TRUE,  96,  TRUE, 6}},
    { STVTG_TIMING_MODE_768P60000_65000,   {60000, SCAN_P, 1024,  768, 283, 36, 283,  36}, { 1344,  806,  65000000, TRUE, 136,  TRUE, 12}},
    { STVTG_TIMING_MODE_768P85000_94500,   {85000, SCAN_P, 1024,  768, 292, 40, 292,  40}, { 1376,  808,  94500000, TRUE,  97,  TRUE, 6}},

    { STVTG_TIMING_MODE_576I50000_13500,   {50000, SCAN_I,  720,  576, 132, 23, 132,  46}, {  864,  625,  13500000, FALSE, 63, FALSE,  6}},
    { STVTG_TIMING_MODE_576I50000_14750,   {50000, SCAN_I,  768,  576, 141, 23, 132,  44}, {  944,  625,  14750000, FALSE, 71, FALSE,  6}},
    /* HD modes */
    { STVTG_TIMING_MODE_1080P60000_148500, {60000, SCAN_P, 1920, 1080, 192, 41, 280,  45}, { 2200, 1125, 148500000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P59940_148352, {59940, SCAN_P, 1920, 1080, 192, 41, 280,  45}, { 2200, 1125, 148351648, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P50000_148500, {50000, SCAN_P, 1920, 1080, 192, 41, 720,  45}, { 2640, 1125, 148500000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080I60000_74250,  {60000, SCAN_I, 1920, 1080, 192, 42, 192,  42}, { 2200, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080I59940_74176,  {59940, SCAN_I, 1920, 1080, 192, 20, 192,  42}, { 2200, 1125,  74175824, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080I50000_74250,  {50000, SCAN_I, 1920, 1080, 309, 80, 309, 164}, { 2376, 1250,  74250000, TRUE, 66, TRUE, 10}},
    { STVTG_TIMING_MODE_1080I50000_74250_1,{50000, SCAN_I, 1920, 1080, 192, 42, 192,  42}, { 2640, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P30000_74250,  {30000, SCAN_P, 1920, 1080, 192, 41, 192,  42}, { 2200, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P29970_74176,  {29970, SCAN_P, 1920, 1080, 192, 41, 192,  42}, { 2200, 1125,  74175824, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P25000_74250,  {25000, SCAN_P, 1920, 1080, 192, 41, 192,  42}, { 2640, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P24000_74250,  {24000, SCAN_P, 1920, 1080, 192, 41, 192,  42}, { 2750, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1080P23976_74176,  {23976, SCAN_P, 1920, 1080, 192, 41, 192,  42}, { 2750, 1125,  74175824, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1035I60000_74250,  {60000, SCAN_I, 1920, 1035, 192, 44, 280,  90}, { 2200, 1125,  74250000, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_1035I59940_74176,  {59940, SCAN_I, 1920, 1035, 192, 44, 280,  90}, { 2200, 1125,  74175824, TRUE, 44, TRUE, 10}},
    { STVTG_TIMING_MODE_720P60000_74250,   {60000, SCAN_P, 1280,  720, 260, 26, 260,  26}, { 1650,  750,  74250000, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_720P59940_74176,   {59940, SCAN_P, 1280,  720, 260, 25, 260,  26}, { 1650,  750,  74175824, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_720P30000_37125,   {30000, SCAN_P, 1280,  720, 260, 25, 260,  26}, { 1650,  750,  37125000, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_720P29970_37088,   {29970, SCAN_P, 1280,  720, 260, 25, 260,  26}, { 1650,  750,  37087912, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_720P24000_29700,   {24000, SCAN_P, 1280,  720, 260, 25, 260,  26}, { 1650,  750,  29700000, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_720P23976_29670,   {23976, SCAN_P, 1280,  720, 260, 25, 260,  26}, { 1650,  750,  29670329, TRUE, 40, TRUE, 10}},
    /*Australian modes*/
    { STVTG_TIMING_MODE_1080I50000_72000,  {50000, SCAN_I, 1920, 1080, 192, 41, 384, 85}, { 2304, 1250,  72000000, TRUE , 44, TRUE , 10}},
    { STVTG_TIMING_MODE_720P50000_74250,   {50000, SCAN_P, 1280,  720, 260, 26, 260, 26}, { 1980,  750,  74250000, TRUE, 40, TRUE, 10}},
    { STVTG_TIMING_MODE_576P50000_27000,   {50000, SCAN_P,  720,  576, 132, 45, 132, 45}, {  864,  625,  27000000, FALSE, 64, FALSE, 10}},
    { STVTG_TIMING_MODE_1152I50000_48000,  {50000, SCAN_I, 1280, 1152, 235, 90, 235, 90}, { 1536, 1250,  48000000, TRUE , 44, TRUE , 10}}

    };

#ifdef STVTG_VOS
/* Output window diplay positioning calibration :
 * adjustments done from ModeParamsTable table values, with monitors in H/V Delays
 * and special pattern displayed */
static ModeParamsLineTuned_t ModeParamsTunedTableVos[] =
    {
    { STVTG_TIMING_MODE_SLAVE,               0,   0},
    /* SD modes */
    { STVTG_TIMING_MODE_480I60000_13514,   -11,  -4},
    { STVTG_TIMING_MODE_480P60000_27027,   -11,  -8},
    { STVTG_TIMING_MODE_480P30000_13514,   -11,  -4},
    { STVTG_TIMING_MODE_480P24000_10811,   -11,  -4},
    { STVTG_TIMING_MODE_480I59940_13500,   -11,  -4},
    { STVTG_TIMING_MODE_480P59940_27000,   -16,  -5},
    { STVTG_TIMING_MODE_480P29970_13500,   -11,  -4},
    { STVTG_TIMING_MODE_480P23976_10800,   -11,  -4},
    { STVTG_TIMING_MODE_480I60000_12285,   -18,  -2},
    { STVTG_TIMING_MODE_480P60000_24570,   -18,  -8},
    { STVTG_TIMING_MODE_480P30000_12285,   -18,  -2},
    { STVTG_TIMING_MODE_480P24000_9828,    -18,  -2},
    { STVTG_TIMING_MODE_480I59940_12273,   -18,  -2},
    { STVTG_TIMING_MODE_480P59940_24545,   -18,  -8},
    { STVTG_TIMING_MODE_480P29970_12273,   -18,  -2},
    { STVTG_TIMING_MODE_480P23976_9818,    -18,  -2},
    { STVTG_TIMING_MODE_576I50000_13500,    -7,  -1},
    { STVTG_TIMING_MODE_576I50000_14750,   -17,  -1},
    /* HD modes */
    { STVTG_TIMING_MODE_1080P60000_148500, -89,   0},
    { STVTG_TIMING_MODE_1080P59940_148352, -89,   0},
    { STVTG_TIMING_MODE_1080P50000_148500,   0,   0},
    { STVTG_TIMING_MODE_1080I60000_74250,  -89,  -3},
    { STVTG_TIMING_MODE_1080I59940_74176,  -89,  -3},
    { STVTG_TIMING_MODE_1080I50000_74250,   0,    0},
    { STVTG_TIMING_MODE_1080I50000_74250_1,  0,   0},
    { STVTG_TIMING_MODE_1080P30000_74250,  -89,  -3},
    { STVTG_TIMING_MODE_1080P29970_74176,  -89,  -3},
    { STVTG_TIMING_MODE_1080P25000_74250,    0,  -3},
    { STVTG_TIMING_MODE_1080P24000_74250,    0,  -3},
    { STVTG_TIMING_MODE_1080P23976_74176,    0,  -3},
    { STVTG_TIMING_MODE_1035I60000_74250,    0,  -4},
    { STVTG_TIMING_MODE_1035I59940_74176,    0,  -4},
    { STVTG_TIMING_MODE_720P60000_74250,  -110,  -4},
    { STVTG_TIMING_MODE_720P59940_74176,  -110,  -4},
    { STVTG_TIMING_MODE_720P30000_37125,  -110,  -4},
    { STVTG_TIMING_MODE_720P29970_37088,  -110,  -4},
    { STVTG_TIMING_MODE_720P24000_29700,  -110,  -4},
    { STVTG_TIMING_MODE_720P23976_29670,  -110,  -4},

    /*Australian modes*/
    { STVTG_TIMING_MODE_1080I50000_72000,  0,  0},
    { STVTG_TIMING_MODE_720P50000_74250,   0,  0},
    { STVTG_TIMING_MODE_576P50000_27000,   0,  0},
    { STVTG_TIMING_MODE_1152I50000_48000,  0,  0}/*to be defined*/

    };
#endif /* #ifdef STVTG_VOS */

#ifdef STVTG_VFE
/* Output window diplay positioning calibration :
 * adjustments done from ModeParamsTable table values, with monitors in H/V Delays
 * and special pattern displayed */
static ModeParamsLineTuned_t ModeParamsTunedTableVfe[] =
    {
    { STVTG_TIMING_MODE_SLAVE,               0,   0},
    /* SD modes */
    { STVTG_TIMING_MODE_480I60000_13514,     0,   0},
    { STVTG_TIMING_MODE_480P60000_27027,     0,   0},
    { STVTG_TIMING_MODE_480P30000_13514,     0,   0},
    { STVTG_TIMING_MODE_480P24000_10811,     0,   0},
    { STVTG_TIMING_MODE_480I59940_13500,   -16, -11},
    { STVTG_TIMING_MODE_480P59940_27000,     0,   0},
    { STVTG_TIMING_MODE_480P29970_13500,     0,   0},
    { STVTG_TIMING_MODE_480P23976_10800,     0,   0},
    { STVTG_TIMING_MODE_480I60000_12285,     0,   0},
    { STVTG_TIMING_MODE_480P60000_24570,     0,   0},
    { STVTG_TIMING_MODE_480P30000_12285,     0,   0},
    { STVTG_TIMING_MODE_480P24000_9828,      0,   0},
    { STVTG_TIMING_MODE_480I59940_12273,     0,   0},
    { STVTG_TIMING_MODE_480P59940_24545,     0,   0},
    { STVTG_TIMING_MODE_480P29970_12273,     0,   0},
    { STVTG_TIMING_MODE_480P23976_9818,      0,   0},
    { STVTG_TIMING_MODE_576I50000_13500,   -12,  -1},
    { STVTG_TIMING_MODE_576I50000_14750,     0,   0},
    /* HD modes */
    { STVTG_TIMING_MODE_1080P60000_148500,   0,   0},
    { STVTG_TIMING_MODE_1080P59940_148352,   0,   0},
    { STVTG_TIMING_MODE_1080P50000_148500,   0,   0},
    { STVTG_TIMING_MODE_1080I60000_74250,    0,   0},
    { STVTG_TIMING_MODE_1080I59940_74176,    0,   0},
    { STVTG_TIMING_MODE_1080I50000_74250,    0,   0},
    { STVTG_TIMING_MODE_1080I50000_74250_1,  0,   0},
    { STVTG_TIMING_MODE_1080P30000_74250,    0,   0},
    { STVTG_TIMING_MODE_1080P29970_74176,    0,   0},
    { STVTG_TIMING_MODE_1080P25000_74250,    0,   0},
    { STVTG_TIMING_MODE_1080P24000_74250,    0,   0},
    { STVTG_TIMING_MODE_1080P23976_74176,    0,   0},
    { STVTG_TIMING_MODE_1035I60000_74250,    0,   0},
    { STVTG_TIMING_MODE_1035I59940_74176,    0,   0},
    { STVTG_TIMING_MODE_720P60000_74250,     0,   0},
    { STVTG_TIMING_MODE_720P59940_74176,     0,   0},
    { STVTG_TIMING_MODE_720P30000_37125,     0,   0},
    { STVTG_TIMING_MODE_720P29970_37088,     0,   0},
    { STVTG_TIMING_MODE_720P24000_29700,     0,   0},
    { STVTG_TIMING_MODE_720P23976_29670,     0,   0},
    { STVTG_TIMING_MODE_1080I50000_72000,    0,   0},


    /* non standard modes */
    { STVTG_TIMING_MODE_720P50000_74250,     0,   0},
    { STVTG_TIMING_MODE_576P50000_27000,     0,   0},
    { STVTG_TIMING_MODE_1152I50000_48000,    0,   0}
    };
#endif /* #ifdef STVTG_VFE */

#ifdef STVTG_VFE2
/* Output window diplay positioning calibration :
 * adjustments done from ModeParamsTable table values, with monitors in H/V Delays
 * and special pattern displayed */
static ModeParamsLineTuned_t ModeParamsTunedTableVfe[] =
    {
    { STVTG_TIMING_MODE_SLAVE,               0,   0},
    /* SD modes */
    { STVTG_TIMING_MODE_480I60000_13514,     0,   0},
    { STVTG_TIMING_MODE_480P60000_27027,     0,   0},
    { STVTG_TIMING_MODE_480P30000_13514,     0,   0},
    { STVTG_TIMING_MODE_480P24000_10811,     0,   0},
    { STVTG_TIMING_MODE_480I59940_13500,    -15,  -11},           /*secam 13,  -5},*/
    { STVTG_TIMING_MODE_480P59940_27000,     0,   0},
    { STVTG_TIMING_MODE_480P29970_13500,     0,   0},
    { STVTG_TIMING_MODE_480P23976_10800,     0,   0},
    { STVTG_TIMING_MODE_480I60000_12285,     0,   0},
    { STVTG_TIMING_MODE_480P60000_24570,     0,   0},
    { STVTG_TIMING_MODE_480P30000_12285,     0,   0},
    { STVTG_TIMING_MODE_480P24000_9828,      0,   0},
    { STVTG_TIMING_MODE_480I59940_12273,     0,   0},
    { STVTG_TIMING_MODE_480P59940_24545,     0,   0},
    { STVTG_TIMING_MODE_480P29970_12273,     0,   0},
    { STVTG_TIMING_MODE_480P23976_9818,      0,   0},
    { STVTG_TIMING_MODE_576I50000_13500,    -12,  -3},              /* pal x=-98 y=83(decimal)*/
    { STVTG_TIMING_MODE_576I50000_14750,     0,   0}
    };
#endif /* #ifdef STVTG_VFE2 */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
static U8 GetTunedModeParamsLine( const STVTG_TimingMode_t Mode, const ModeParamsLineTuned_t * const TunedTable_p);
#endif /* #ifdef STVTG_VOS || STVTG_VFE || STVTG_VFE2*/

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stvtg_GetModeParamsLine
Description : Give back pointer on ModeParamsLine_t in table ModeParamsTable
              relative to timing mode given.
Parameters  : Mode (IN) : Timing mode wanted
Assumptions : Mode is ok
Limitations :
Returns     : pointer on ModeParamsLine_t
*******************************************************************************/
ModeParamsLine_t *stvtg_GetModeParamsLine( const STVTG_TimingMode_t Mode)
{
    U8 Index=0;

    for (Index=0; Index < STVTG_TIMING_MODE_COUNT; Index++)
    {
        if (ModeParamsTable[Index].Mode == Mode)
        {
            break;  /* found */
        }
    }
   return(&ModeParamsTable[Index]);

} /* End of stvtg_GetModeParamsLine() function */

#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
/*******************************************************************************
Name        : GetTunedModeParamsLine
Description : Give back index in table TunedTable_p relative to timing mode given.
Parameters  : Mode (IN) : Timing mode wanted
 *            TunedTable_p (IN): Table of tuned values
Assumptions : Mode is ok, TunedTable_p is ok.
Limitations :
Returns     : index in table TunedTable_p
*******************************************************************************/
static U8 GetTunedModeParamsLine( const STVTG_TimingMode_t Mode, const ModeParamsLineTuned_t * const TunedTable_p)
{
    U8 Index=0;

    for (Index=0; Index < STVTG_TIMING_MODE_COUNT; Index++)
    {
        if (TunedTable_p[Index].Mode == Mode)
        {
            break;  /* found */
        }
    }
    return (Index);
} /* End of GetTunedModeParamsLine() function */
#endif /* #ifdef STVTG_VOS || STVTG_VFE || STVTG_VFE2*/

/*
******************************************************************************
API Functions
******************************************************************************
*/

/***************************************************************
Name : STVTG_SetMode
Description : Configure the video timing mode for a VTG
Parameters : Handle (IN) : Handle to the VTG
             Mode (IN) : Timing Mode to be used
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, STVTG_ERROR_INVALID_MODE
 *       ST_ERROR_BAD_PARAMETER, STVTG_ERROR_DENC_ACCESS,
 *       ST_ERROR_FEATURE_NOT_SUPPORTED
****************************************************************/
ST_ErrorCode_t STVTG_SetMode(  const STVTG_Handle_t     Handle
                             , const STVTG_TimingMode_t Mode
                            )
{
    ST_ErrorCode_t ErrorCode;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    /* Protect access */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    if (   (   (Mode < 32)
            && (((1<<Mode)      & Unit_p->Device_p->CapabilityBank1) != 0))
        || (   ((Mode >= 32) && (Mode < STVTG_TIMING_MODE_COUNT))
            && (((1<<(Mode-32)) & Unit_p->Device_p->CapabilityBank2) != 0))
       )
    {
        ErrorCode = stvtg_IHALUpdateTimingParameters(Unit_p->Device_p, Mode);
    }
    else
    {
        ErrorCode = STVTG_ERROR_INVALID_MODE;
    }
    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ErrorCode);
} /* End of STVTG_SetMode() function */

/***************************************************************
Name : STVTG_SetOptionalConfiguration
Description : allow force inhibition of output sync signal
 *            allow embedded synchronization insertion
Parameters : Handle (IN) : Handle to a VTG
             OptionalConfiguration_p (IN) : optional configuration to be set
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE,
 *       ST_ERROR_FEATURE_NOT_SUPPORTED
****************************************************************/
ST_ErrorCode_t STVTG_SetOptionalConfiguration( const STVTG_Handle_t                   Handle,
                                               const STVTG_OptionalConfiguration_t  * const OptionalConfiguration_p
                                             )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_Option_t OldOption;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (OptionalConfiguration_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    /* Protect access */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    /* Check Option validity */
    OldOption = Unit_p->Device_p->Option;
    Unit_p->Device_p->Option = OptionalConfiguration_p->Option;
    ErrorCode = stvtg_IHALCheckOptionValidity(Unit_p->Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        switch (OptionalConfiguration_p->Option)
        {
            case STVTG_OPTION_EMBEDDED_SYNCHRO :
                Unit_p->Device_p->EmbeddedSynchro = OptionalConfiguration_p->Value.EmbeddedSynchro;
                break;
            case STVTG_OPTION_NO_OUTPUT_SIGNAL :
                Unit_p->Device_p->OldNoOutputSignal = Unit_p->Device_p->NoOutputSignal; /* store current value */
                Unit_p->Device_p->NoOutputSignal    = OptionalConfiguration_p->Value.NoOutputSignal;
                break;
            case STVTG_OPTION_HSYNC_POLARITY :
                Unit_p->Device_p->IsHSyncPositive = OptionalConfiguration_p->Value.IsHSyncPositive;
                break;
            case STVTG_OPTION_VSYNC_POLARITY :
                Unit_p->Device_p->IsVSyncPositive = OptionalConfiguration_p->Value.IsVSyncPositive;
                break;
            case STVTG_OPTION_OUT_EDGE_POSITION :
                Unit_p->Device_p->OutEdges = OptionalConfiguration_p->Value.OutEdges;
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = stvtg_IHALSetOptionalConfiguration(Unit_p->Device_p);
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* failed to set option => set back old option */
        Unit_p->Device_p->Option = OldOption;
    }
    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ErrorCode);
} /* End of STVTG_SetOptionalConfiguration() function */


/*************************************************************************
 Name : STVTG_SetSlaveModeParams
 Description : Sets Slave Mode parameters but do not activate slave/master
               change.
 Parameters : Handle (IN) : Handle to the VTG
              SlaveModeParams_p (IN) : slave mode parameters
Assumptions :
Limitations :
 Return : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER or ST_ERROR_INVALID_HANDLE
**************************************************************************/
ST_ErrorCode_t STVTG_SetSlaveModeParams(  const STVTG_Handle_t           Handle
                                        , const STVTG_SlaveModeParams_t  *const SlaveModeParams_p
                                       )
{
    ST_ErrorCode_t ErrorCode;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SlaveModeParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    /* Protect access */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    /* check slave mode parameters are OK */
    ErrorCode = stvtg_IHALCheckSlaveModeParams(Unit_p->Device_p, SlaveModeParams_p);
    if ( ErrorCode == ST_NO_ERROR)
    {
        Unit_p->Device_p->SlaveModeParams = *SlaveModeParams_p; /* whole structure copy */
    }
    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return (ErrorCode);
} /* End of STVTG_SetSlaveModeParams() function */


/***************************************************************
Name : STVTG_GetMode
Description : Retrieves the current vsync signal parameters for a VTG device.
Parameters : Handle (IN) : Handle to a VTG
             SyncParams_p (IN/OUT) : Current vsync params data : memory location
                                     is filled with structure data.
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetModeSyncParams( STVTG_Handle_t       Handle,
                              STVTG_SyncParams_t * const SyncParams_p
                            )
{
    stvtg_Device_t *Device_p;
    ModeParamsLine_t *Line_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((SyncParams_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* reach device */
    Device_p = (stvtg_Device_t *) (((stvtg_Unit_t *)Handle)->Device_p);

    /* Protect access */

    STOS_SemaphoreWait(Device_p->ModeCtrlAccess_p);

    /* Here Handle is valid, that means it has been initialized (STVTG_Init) and opened (STVTG_Open)
     * so Device_p->TimingMode has reliable values.
    */
    /* Get corresponding mode parameters in table */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);

    *SyncParams_p = Line_p->RegParams;

    /* Free access */
    STOS_SemaphoreSignal(Device_p->ModeCtrlAccess_p);

    return(ST_NO_ERROR);
}

/***************************************************************
Name : STVTG_GetMode
Description : Retrieves the current video signal parameters for a VTG device.
Parameters : Handle (IN) : Handle to a VTG
             TimingID_p (IN/OUT) : Current timing mode identifier
             ModeParams_p (IN/OUT) : Current timing mode data : memory location
                                     is filled with structure data.
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetMode(  const STVTG_Handle_t Handle
                             , STVTG_TimingMode_t * const TimingID_p
                             , STVTG_ModeParams_t * const ModeParams_p
                            )
{
    stvtg_Device_t *Device_p;
    ModeParamsLine_t *Line_p;
#if defined (STVTG_VOS) || defined (STVTG_VFE) || defined (STVTG_VFE2)
    U8  Index;
    ModeParamsLineTuned_t *TunedTable_p;
#endif /* #ifdef STVTG_VOS || STVTG_VFE || STVTG_VFE2*/

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    if ((ModeParams_p == NULL) || (TimingID_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* reach device */
    Device_p = (stvtg_Device_t *) (((stvtg_Unit_t *)Handle)->Device_p);

    /* Protect access */

    STOS_SemaphoreWait(Device_p->ModeCtrlAccess_p);

    /* Here Handle is valid, that means it has been initialized (STVTG_Init) and opened (STVTG_Open)
     * so Device_p->TimingMode has reliable values.
    */

    /* give back timing mode */
    *TimingID_p = Device_p->TimingMode;

    /* Get corresponding mode parameters in table */
    Line_p = stvtg_GetModeParamsLine(Device_p->TimingMode);
    *ModeParams_p = Line_p->ModeParams;
#ifdef STVTG_VOS
    /* DDTS GNBvd09294 : Display positioning needs tuning compared to theorical values */
    if ( (Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7015) || (Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7020)
       ||(Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7710) || (Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7100)
       ||(Device_p->DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7200))
    {
        TunedTable_p = ModeParamsTunedTableVos;
        Index = GetTunedModeParamsLine(Device_p->TimingMode, TunedTable_p);

        /*ModeParams_p->DigitalActiveAreaXStart += (S32)TunedTable_p[Index].DeltaDigitalActiveAreaXStart;
          ModeParams_p->DigitalActiveAreaYStart += (S32)TunedTable_p[Index].DeltaDigitalActiveAreaYStart;*/
    }
#endif /* #ifdef STVTG_VOS */
#if defined (STVTG_VFE) || defined (STVTG_VFE2)
    if ( (Device_p->DeviceType == STVTG_DEVICE_TYPE_VFE) || (Device_p->DeviceType == STVTG_DEVICE_TYPE_VFE2))
    {
        TunedTable_p = ModeParamsTunedTableVfe;
        Index = GetTunedModeParamsLine(Device_p->TimingMode, TunedTable_p);

        ModeParams_p->DigitalActiveAreaXStart += (S32)TunedTable_p[Index].DeltaDigitalActiveAreaXStart;
        ModeParams_p->DigitalActiveAreaYStart += (S32)TunedTable_p[Index].DeltaDigitalActiveAreaYStart;
    }
#endif /* #ifdef STVTG_VFE || STVTG_VFE2*/

    /* Free access */
    STOS_SemaphoreSignal(Device_p->ModeCtrlAccess_p);

    return(ST_NO_ERROR);
} /* End of STVTG_GetMode() function */

/***************************************************************
Name : STVTG_GetOptionalConfiguration
Description : get output config
Parameters : Handle (IN) : Handle to a VTG
             OptionalConfiguration_p (OUT) : optional configuration to be got
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_INVALID_HANDLE
 *       ST_ERROR_FEATURE_NOT_SUPPORTED
****************************************************************/
ST_ErrorCode_t STVTG_GetOptionalConfiguration( const STVTG_Handle_t Handle,
                                               STVTG_OptionalConfiguration_t * const OptionalConfiguration_p
                                             )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    stvtg_Unit_t *Unit_p;
    STVTG_Option_t OldOption;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (OptionalConfiguration_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    /* Protect access */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    /* Check Option validity */
    OldOption = Unit_p->Device_p->Option;
    Unit_p->Device_p->Option = OptionalConfiguration_p->Option; /* change temporarily for internal use*/
    ErrorCode = stvtg_IHALCheckOptionValidity(Unit_p->Device_p);

    if (ErrorCode == ST_NO_ERROR)
    {
        switch (OptionalConfiguration_p->Option)
        {
            case STVTG_OPTION_EMBEDDED_SYNCHRO :
                OptionalConfiguration_p->Value.EmbeddedSynchro = Unit_p->Device_p->EmbeddedSynchro;
                break;
            case STVTG_OPTION_NO_OUTPUT_SIGNAL :
                OptionalConfiguration_p->Value.NoOutputSignal = Unit_p->Device_p->NoOutputSignal;
                break;
            case STVTG_OPTION_HSYNC_POLARITY :
                OptionalConfiguration_p->Value.IsHSyncPositive = Unit_p->Device_p->IsHSyncPositive;
                break;
            case STVTG_OPTION_VSYNC_POLARITY :
                OptionalConfiguration_p->Value.IsVSyncPositive = Unit_p->Device_p->IsVSyncPositive;
                break;
            case STVTG_OPTION_OUT_EDGE_POSITION :
                OptionalConfiguration_p->Value.OutEdges = Unit_p->Device_p->OutEdges;
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }
    Unit_p->Device_p->Option = OldOption;

    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ErrorCode);
} /* End of STVTG_GetOptionalConfiguration() function */


/***************************************************************
Name : STVTG_GetSlaveModeParams
Description : Retrieves the current slave mode parameters for a VTG device.
Parameters : Handle (IN) : Handle to a VTG
             SlaveModeParams_p (IN/OUT) : pointer to structure to be filled
             with slave mode parameters informations.
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetSlaveModeParams(  const STVTG_Handle_t    Handle
                                        , STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                       )
{
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SlaveModeParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    /* Protect access */

    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    *SlaveModeParams_p = Unit_p->Device_p->SlaveModeParams;

    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    return(ST_NO_ERROR);
} /* End of STVTG_GetSlaveModeParams() function */


#ifdef STVTG_DEBUG_STATISTICS
/***************************************************************
Name : STVTG_GetStatistics
Description : Gives some statistics for DEBUG
Parameters : Handle (IN) : Handle to a VTG
             Statistics_p (IN/OUT) : pointer to structure to be filled
 *           ResetAll (IN) : Reset statistics if TRUE
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetStatistics(  const STVTG_Handle_t    Handle
                                   , STVTG_Statistics_t * const Statistics_p
                                   , BOOL ResetAll
                                  )
{
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (Statistics_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;
    *Statistics_p = Unit_p->Device_p->Statistics;
    if (ResetAll)
    {
        memset(&(Unit_p->Device_p->Statistics), 0, sizeof(STVTG_Statistics_t));
    }
    return(ST_NO_ERROR);
} /* End of STVTG_GetStatistics() function */
#endif /* #ifdef STVTG_DEBUG_STATISTICS */

/***************************************************************
Name : STVTG_CalibrateSyncLevel
Description :  adjust sync level(sync signal calibration)
Parameters : Handle (IN) : Handle to a VTG
             SyncLevel (IN) : sync level definition for a given HD/ED mode
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_CalibrateSyncLevel( const STVTG_Handle_t    Handle , STVTG_SyncLevel_t   *SyncLevel_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SyncLevel_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;

    #if defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)
        ErrCode = stvtg_HALSetLevelSettings(Unit_p->Device_p, SyncLevel_p);
    #else
       return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    #endif

    return (ErrCode);

}
/***************************************************************
Name : STVTG_GetLevelSettings
Description : retreive current settings of sync signal
Parameters : Handle (IN) : Handle to a VTG
             SyncLevel (IN/OUT) :
Assumptions :
Limitations :
Return : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_FEATURE_NOT_SUPPORTED or ST_ERROR_BAD_PARAMETER
****************************************************************/
ST_ErrorCode_t STVTG_GetLevelSettings( const STVTG_Handle_t   Handle , STVTG_SyncLevel_t   *SyncLevel_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvtg_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (SyncLevel_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (stvtg_Unit_t *)Handle;

    #if defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)
        ErrCode = stvtg_HALGetLevelSettings(Unit_p->Device_p, SyncLevel_p);
    #else
       return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    #endif

    return (ErrCode);

}


/* End of vtg_drv.c */




