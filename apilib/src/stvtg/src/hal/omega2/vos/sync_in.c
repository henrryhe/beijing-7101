/************************************************************************
File Name   :   sync_in.c

Description :   HAL for the Output Stage of the STi7015/20, for embbeded syncs

COPYRIGHT (C) STMicroelectronics 2002

Date               Modification                                     Name
----               ------------                                     ----
06 Mar 2001        Created                                          HSdLM
28 Jun 2001        Fix issue : byte offsets added to an (U32 *)     HSdLM
17 Sep 2002        Replace DHDO_MIDLOW by DHDO_MIDLOWL to match     HSdLM
 *                 datasheet, add support of EIA-770.2
20 Jan 2003        Fix DDTS GNBvd17823                              BS
16 May 2007        update DHDOSyncInLevel table                     ICH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes -----------------------------------------------------------*/

#include "stsys.h"


#include "stddefs.h"
#include "vos_reg.h"
#include "sync_in.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    U16     C656_BeginActive;
    U16     C656_EndActive;
    U16     C656_BlankLength;
    U16     C656_ActiveLength;
    BOOL    C656_PALnotNTSC;
} HALVTG_C656SyncInSettings_t;

typedef struct
{
    U16     HDO_ABroad;
    U16     HDO_BBroad;
    U16     HDO_CBroad;
    U16     HDO_BActive;
    U16     HDO_CActive;
    U16     HDO_BroadLength;
    U16     HDO_BlankLength;
    U16     HDO_ActiveLength;
    BOOL    HDO_IsTriLevel;
} HALVTG_DHDOSyncInSettings_t;

/* Private Constants -------------------------------------------------------- */

typedef enum
{
    C656_MODE_PAL,
    C656_MODE_NTSC,
    C656_MODE_NONE
} C656Mode_t;

typedef enum
{
    DHDO_MODE_274M_I,
    DHDO_MODE_274M_P,
    DHDO_MODE_7703_P,
    DHDO_MODE_295M_I,
    DHDO_MODE_1358_P,
    DHDO_MODE_7702_P,
    DHDO_MODE_XGA75_P,
    DHDO_MODE_NONE
} DHDOMode_t;

/* Private Variables (static)------------------------------------------------ */
/*{C656_BeginActive, C656_EndActive, C656_BlankLength, C656_ActiveLength, C656_PALnotNTSC}*/
static const HALVTG_C656SyncInSettings_t C656SyncInSettings[] =
{
  {131, 850, 22, 288, 1}, /* Y-Cb-Cr  8 bits 4:2:2 PAL mode */
  {121, 840, 16, 244, 0}  /* Y-Cb-Cr  8 bits 4:2:2 NTSC mode */
};

/*{HDO_ABroad, HDO_BBroad, HDO_CBroad, HDO_BActive, HDO_CActive, HDO_BroadLength, HDO_BlankLength, HDO_ActiveLength, HDO_IsTriLevel}*/
static const HALVTG_DHDOSyncInSettings_t DHDOSyncInSettings[] =
{
  {44, 132, 1012, 192, 2112, 5, 11,  544, TRUE }, /* Y-Cb-Cr SMPTE274M & EIA 770.3 Interlaced  */
  {44, 132, 1012, 192, 2112, 5, 36, 1080, TRUE }, /* Y-Cb-Cr SMPTE274M Progressive */
  {40, 260, 1540, 228, 1540, 5, 16,  724, TRUE }, /* Y-Cb-Cr EIA 770.3 Progressive */
  {66, 309,  903, 309, 2229, 1, 76,  544, TRUE }, /* Y-Cb-Cr SMPTE295M Interlaced  */
  {64, 796,  858, 132,  851, 5, 39,  576, FALSE }, /* Y-Cb-Cr ITU-R BT.1358 */
  {64, 796,  858, 123,  843, 6, 30,  483, FALSE },  /* Y-Cb-Cr EIA-770.2 Progressive */
  {112,352, 1024, 288, 1312, 4, 35,  768, FALSE }  /* Y-Cb-Cr XGA Progressive */
};

/*{Mode},{HDO_ZeroL, HDO_MidL, HDO_HighL, HDO_MidLowL, HDO_LowL, HDO_YOff, HDO_COff, HDO_YMult, HDO_CMult}
 */
/*static HALVTG_DHDOSyncInLevel_t DHDOSyncInLevel[] =*/
static   STVTG_SyncLevel_t  DHDOSyncInLevel[] =
{
#if defined (ST_7109)

     /*HD modes*/
    {STVTG_TIMING_MODE_1080I60000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080I59940_74176,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080I50000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25d, 0x24c}},
    {STVTG_TIMING_MODE_1080I50000_74250_1,{0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25d, 0x24c}},
    {STVTG_TIMING_MODE_720P60000_74250,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    {STVTG_TIMING_MODE_720P59940_74176,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    {STVTG_TIMING_MODE_720P50000_74250,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    /*ED modes*/
    {STVTG_TIMING_MODE_480P59940_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x25b, 0x250}},
    {STVTG_TIMING_MODE_480P60000_27027,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x25b, 0x250}},
    {STVTG_TIMING_MODE_576P50000_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x25b, 0x250}},
    /*VGA modes*/  /*put same values as 480P*/
    {STVTG_TIMING_MODE_480P60000_25200,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x25b, 0x250}},
    {STVTG_TIMING_MODE_480P59940_25175,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x25b, 0x250}},
    /*XGA modes*/
    {STVTG_TIMING_MODE_768P75000_78750,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x24b}},
    {STVTG_TIMING_MODE_768P60000_65000,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    {STVTG_TIMING_MODE_768P85000_94500,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    /*modes not validated : put default values from 1080I@60*/
    {STVTG_TIMING_MODE_1080P60000_148500, {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P59940_148352, {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P50000_148500, {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P30000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P29970_74176,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P25000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P24000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1080P23976_74176,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1035I60000_74250,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
    {STVTG_TIMING_MODE_1035I59940_74176,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBE, 0x262, 0x253}},
      /*modes not validated : put default values from 720p@60*/
    {STVTG_TIMING_MODE_720P30000_37125,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    {STVTG_TIMING_MODE_720P29970_37088,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    {STVTG_TIMING_MODE_720P24000_29700,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    {STVTG_TIMING_MODE_720P23976_29670,   {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25b, 0x24d}},
    /*Australian modes*/
    {STVTG_TIMING_MODE_1080I50000_72000,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25d, 0x24c}},
    {STVTG_TIMING_MODE_1152I50000_48000,  {0xE5, 0x155, 0x1C5, 0x72, 0, 0xBF, 0xBF, 0x25d, 0x24c}},

    {STVTG_TIMING_MODE_COUNT,             {0,    0,     0,     0,    0, 0,    0,    0,     0}}
#elif defined (ST_7710)
     /*HD modes*/
    {STVTG_TIMING_MODE_1080I60000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080I59940_74176,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080I50000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2BC, 0x2A8}},
    {STVTG_TIMING_MODE_1080I50000_74250_1,{0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2BC, 0x2A8}},
    {STVTG_TIMING_MODE_720P60000_74250,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    {STVTG_TIMING_MODE_720P59940_74176,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    {STVTG_TIMING_MODE_720P50000_74250,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    /*ED modes*/
    {STVTG_TIMING_MODE_480P59940_27000,   {0x104, 0x180, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B8, 0x2A8}},
    {STVTG_TIMING_MODE_480P60000_27027,   {0x104, 0x180, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B8, 0x2A8}},
    {STVTG_TIMING_MODE_576P50000_27000,   {0x104, 0x180, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B8, 0x2A8}},
    /*VGA modes*/
    {STVTG_TIMING_MODE_480P60000_25200,   {0x104, 0x180, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B8, 0x2A8}},
    {STVTG_TIMING_MODE_480P59940_25175,   {0x104, 0x180, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B8, 0x2A8}},
    /*XGA modes*/ /*validated only for 7109 : take same values as 7109*/
    {STVTG_TIMING_MODE_768P75000_78750,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x24b}},
    {STVTG_TIMING_MODE_768P60000_65000,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    {STVTG_TIMING_MODE_768P85000_94500,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    /*modes not validated : put default values from 1080I@60*/
    {STVTG_TIMING_MODE_1080P60000_148500, {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P59940_148352, {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P50000_148500, {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P30000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P29970_74176,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P25000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P24000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1080P23976_74176,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1035I60000_74250,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1035I59940_74176,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
      /*modes not validated : put default values from 720p@60*/
    {STVTG_TIMING_MODE_720P30000_37125,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    {STVTG_TIMING_MODE_720P29970_37088,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    {STVTG_TIMING_MODE_720P24000_29700,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    {STVTG_TIMING_MODE_720P23976_29670,   {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xA8, 0x2B6, 0x2A8}},
    /*Australian modes*/
    {STVTG_TIMING_MODE_1080I50000_72000,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_1152I50000_48000,  {0x104, 0x18b, 0x208, 0x82, 0, 0xD8, 0xE0, 0x2B8, 0x2A6}},
    {STVTG_TIMING_MODE_COUNT,             {0,    0,     0,     0,    0, 0,    0,    0,     0}}
#elif defined (ST_7100)
     /*HD modes*/
    {STVTG_TIMING_MODE_1080I60000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I59940_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I50000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I50000_74250_1,{0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P60000_74250,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P59940_74176,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P50000_74250,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    /*ED modes*/
    {STVTG_TIMING_MODE_480P59940_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_480P60000_27027,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_576P50000_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    /*VGA modes*/
    {STVTG_TIMING_MODE_480P60000_25200,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_480P59940_25175,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    /*XGA modes*/ /*validated only for 7109 : take same values as 7109*/
    {STVTG_TIMING_MODE_768P75000_78750,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x24b}},
    {STVTG_TIMING_MODE_768P60000_65000,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    {STVTG_TIMING_MODE_768P85000_94500,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    /*modes not validated : put default values from 1080I@60*/
    {STVTG_TIMING_MODE_1080P60000_148500, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P59940_148352, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P50000_148500, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P30000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P29970_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P25000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P24000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P23976_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1035I60000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1035I59940_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
      /*modes not validated : put default values from 720p@60*/
    {STVTG_TIMING_MODE_720P30000_37125,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P29970_37088,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P24000_29700,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P23976_29670,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    /*Australian modes*/
    {STVTG_TIMING_MODE_1080I50000_72000,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1152I50000_48000,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_COUNT,             {0,    0,     0,     0,    0, 0,    0,    0,     0}}
#else
    /* Put some default values - Need adjustements */
     /*HD modes*/
    {STVTG_TIMING_MODE_1080I60000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I59940_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I50000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080I50000_74250_1,{0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P60000_74250,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P59940_74176,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P50000_74250,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    /*ED modes*/
    {STVTG_TIMING_MODE_480P59940_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_480P60000_27027,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_576P50000_27000,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    /*VGA modes*/
    {STVTG_TIMING_MODE_480P60000_25200,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    {STVTG_TIMING_MODE_480P59940_25175,   {0xE5, 0x20d, 0x335, 0x72, 0, 0xBE, 0xBE, 0x268, 0x250}},
    /*XGA modes*/ /*validated only for 7109 : take same values as 7109*/
    {STVTG_TIMING_MODE_768P75000_78750,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x24b}},
    {STVTG_TIMING_MODE_768P60000_65000,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    {STVTG_TIMING_MODE_768P85000_94500,   {0xE5, 0xE5,  0xE5,  0x72, 0, 0xBD, 0xBA, 0x23d, 0x246}},
    /*modes not validated : put default values from 1080I@60*/
    {STVTG_TIMING_MODE_1080P60000_148500, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P59940_148352, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P50000_148500, {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P30000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P29970_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P25000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P24000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1080P23976_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1035I60000_74250,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1035I59940_74176,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
      /*modes not validated : put default values from 720p@60*/
    {STVTG_TIMING_MODE_720P30000_37125,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P29970_37088,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P24000_29700,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_720P23976_29670,   {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    /*Australian modes*/
    {STVTG_TIMING_MODE_1080I50000_72000,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_1152I50000_48000,  {0xE4, 0x155, 0x1C5, 0x72, 0, 0xBD, 0xBF, 0x268, 0x25A}},
    {STVTG_TIMING_MODE_COUNT,             {0,    0,     0,     0,    0, 0,    0,    0,     0}}

#endif
    };
/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define halvtg_Write32(a, v)  STSYS_WriteRegDev32LE((a), (v))
#define halvtg_Read32(a)      STSYS_ReadRegDev32LE((a))

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : stvtg_HALSetSyncInSettings
Description : set settings for embbeded synchronizations
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_HALSetSyncInSettings(const stvtg_Device_t * const Device_p)
{
    U8 * BaseAddress_p; /* must be a U8 * because offset in bytes are added */
    U32 DHDO_Acfg;
    C656Mode_t C656Mode = C656_MODE_NONE;
    DHDOMode_t DHDOMode = DHDO_MODE_NONE;
    const HALVTG_C656SyncInSettings_t * C656ptr;
    const HALVTG_DHDOSyncInSettings_t * DHDOptr;
    STVTG_SyncInLevel_t DHDOSync = {0, 0, 0, 0, 0, 0, 0, 0, 0};


    U8 Index = 0;

    switch (Device_p->TimingMode)
    {
        case STVTG_TIMING_MODE_480I60000_13514 :    /* NTSC 60 Hz */
        case STVTG_TIMING_MODE_480I59940_13500 :    /* NTSC, PAL M */
        case STVTG_TIMING_MODE_480I60000_12285 :    /* NTSC 60Hz square */
        case STVTG_TIMING_MODE_480I59940_12273 :    /* NTSC square, PAL M square */
            C656Mode = C656_MODE_NTSC;
            break;
        case STVTG_TIMING_MODE_576P50000_27000 :
            DHDOMode = DHDO_MODE_1358_P;
            break;
        case STVTG_TIMING_MODE_480P59940_27000 :    /* ATSC P59.94 = EIA770.2 #6&8 = SMPTE293M */
        case STVTG_TIMING_MODE_480P60000_27027 :    /* ATSC P60 = EIA770.2 #5&7 */
        case STVTG_TIMING_MODE_480P60000_24570 :    /* ATSC P60 square*/
	    case STVTG_TIMING_MODE_480P60000_25200 :
		case STVTG_TIMING_MODE_480P59940_25175 :
        case STVTG_TIMING_MODE_480P30000_13514 :    /* ATSC P30 */
        case STVTG_TIMING_MODE_480P30000_12285 :    /* ATSC P30 square*/
        case STVTG_TIMING_MODE_480P24000_10811 :    /* ATSC P24 */
        case STVTG_TIMING_MODE_480P24000_9828  :    /* ATSC P24 square*/
        case STVTG_TIMING_MODE_480P29970_13500 :    /* ATSC P29.97 */
        case STVTG_TIMING_MODE_480P23976_10800 :    /* ATSC P23.98 */
            C656Mode = C656_MODE_NTSC;
            DHDOMode = DHDO_MODE_7702_P;
            break;
        case STVTG_TIMING_MODE_576I50000_13500 :    /* PAL B,D,G,H,I,N, SECAM */
        case STVTG_TIMING_MODE_576I50000_14750 :    /* PAL B,D,G,H,I,N SECAM square */
            C656Mode = C656_MODE_PAL;
            break;
        case STVTG_TIMING_MODE_1080P60000_148500 :  /* SMPTE 274M #1  P60 */
        case STVTG_TIMING_MODE_1080P59940_148352 :  /* SMPTE 274M #2  P60 /1.001 */
        case STVTG_TIMING_MODE_1080P50000_148500 :  /* SMPTE 274M #3  P50 */
            DHDOMode = DHDO_MODE_274M_P;
            break;
        case STVTG_TIMING_MODE_1080I60000_74250 :   /* EIA770.3 #3 I60 = SMPTE274M #4 I60 */
        case STVTG_TIMING_MODE_1080I59940_74176 :   /* EIA770.3 #4 I60 /1.001 = SMPTE274M #5 I60 /1.001 */
        case STVTG_TIMING_MODE_1035I60000_74250 :   /* SMPTE 240M #1  I60 */
        case STVTG_TIMING_MODE_1035I59940_74176 :   /* SMPTE 240M #2  I60 /1.001 */
        case STVTG_TIMING_MODE_1080I50000_72000 :   /* AS 4933-1 200x 1080i(1250) */
        case STVTG_TIMING_MODE_1080I50000_74250_1 : /* SMPTE 274M   I50 */
        case STVTG_TIMING_MODE_1080P30000_74250 :   /* SMPTE 274M #7  P30 */
        case STVTG_TIMING_MODE_1080P29970_74176 :   /* SMPTE 274M #8  P30 /1.001 */
        case STVTG_TIMING_MODE_1080P25000_74250 :   /* SMPTE 274M #9  P25 */
        case STVTG_TIMING_MODE_1080P24000_74250 :   /* SMPTE 274M #10 P24 */
        case STVTG_TIMING_MODE_1080P23976_74176 :   /* SMPTE 274M #11 P24 /1.001 */
            DHDOMode = DHDO_MODE_274M_I;
            break;
        case STVTG_TIMING_MODE_1152I50000_48000 :
        case STVTG_TIMING_MODE_1080I50000_74250 :   /* SMPTE 295M #2  I50 */
            DHDOMode = DHDO_MODE_295M_I;
            break;
        case STVTG_TIMING_MODE_720P50000_74250 :
        case STVTG_TIMING_MODE_720P60000_74250 :    /* EIA770.3 #1 P60 = SMPTE 296M #1 P60 */
        case STVTG_TIMING_MODE_720P59940_74176 :    /* EIA770.3 #2 P60 /1.001= SMPTE 296M #2 P60 /1.001 */
        case STVTG_TIMING_MODE_720P30000_37125 :    /* ATSC 720x1280 30P */
        case STVTG_TIMING_MODE_720P29970_37088 :    /* ATSC 720x1280 30P/1.001 */
        case STVTG_TIMING_MODE_720P24000_29700 :    /* ATSC 720x1280 24P */
        case STVTG_TIMING_MODE_720P23976_29670 :    /* ATSC 720x1280 24P/1.001 */
            DHDOMode = DHDO_MODE_7703_P;
            break;
        case STVTG_TIMING_MODE_768P75000_78750 :
        case STVTG_TIMING_MODE_768P60000_65000 :
        case STVTG_TIMING_MODE_768P85000_94500 :
            DHDOMode = DHDO_MODE_XGA75_P;
            break;
        case STVTG_TIMING_MODE_SLAVE :
        case STVTG_TIMING_MODE_480P59940_24545 :    /* ATSC P59.94 square*/
        case STVTG_TIMING_MODE_480P29970_12273 :    /* ATSC P29.97 square*/
        case STVTG_TIMING_MODE_480P23976_9818  :    /* ATSC P23.98 square*/

        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    if (C656Mode != C656_MODE_NONE)
    {
        C656ptr = &C656SyncInSettings[C656Mode];
        BaseAddress_p = Device_p->C656BaseAddress_p;
        halvtg_Write32((U32)(BaseAddress_p + C656_BACT),   C656ptr->C656_BeginActive);
        halvtg_Write32((U32)(BaseAddress_p + C656_EACT),   C656ptr->C656_EndActive);
        halvtg_Write32((U32)(BaseAddress_p + C656_BLANKL), C656ptr->C656_BlankLength);
        halvtg_Write32((U32)(BaseAddress_p + C656_ACTL),   C656ptr->C656_ActiveLength);
        halvtg_Write32((U32)(BaseAddress_p + C656_PAL),    C656ptr->C656_PALnotNTSC);
    }

    if (DHDOMode != DHDO_MODE_NONE)
    {
        DHDOptr = &DHDOSyncInSettings[DHDOMode];
        BaseAddress_p = Device_p->DHDOBaseAddress_p;
        DHDO_Acfg = halvtg_Read32 ((U32)BaseAddress_p + DHDO_ACFG);
        if((DHDOMode == DHDO_MODE_7702_P) || (DHDOMode == DHDO_MODE_1358_P))
        {
            /*  DDTS GNBvd17823 : Incorrect 480/576 progressive mode with embedded synchro */
            DHDO_Acfg |= DHDO_PROG293M_MASK;
        }
        else
        {
            DHDO_Acfg &= (U32)~DHDO_PROG293M_MASK;
        }
        halvtg_Write32((U32)(BaseAddress_p + DHDO_ACFG),    DHDO_Acfg & (U32)~DHDO_PROG295M_MASK);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_ABROAD),  DHDOptr->HDO_ABroad);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_BBROAD),  DHDOptr->HDO_BBroad);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_CBROAD),  DHDOptr->HDO_CBroad);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_BACTIVE), DHDOptr->HDO_BActive);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_CACTIVE), DHDOptr->HDO_CActive);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_BROADL),  DHDOptr->HDO_BroadLength);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_BLANKL),  DHDOptr->HDO_BlankLength);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_ACTIVEL), DHDOptr->HDO_ActiveLength);

        while(DHDOSyncInLevel[Index].TimingMode != STVTG_TIMING_MODE_COUNT)
        {
            if (DHDOSyncInLevel[Index].TimingMode == Device_p->TimingMode)
            {
                DHDOSync = DHDOSyncInLevel[Index].Sync;
                break;
            }
            Index++;
        }

        /* Set the dacs levels */
        halvtg_Write32((U32)BaseAddress_p + DHDO_LOWL,    (DHDOSync.Sync_LowLevel | DHDOSync.Sync_LowLevel << 16));
        halvtg_Write32((U32)BaseAddress_p + DHDO_MIDLOWL, ( DHDOSync.Sync_MidLowLevel | DHDOSync.Sync_MidLowLevel << 16) );
        halvtg_Write32((U32)BaseAddress_p + DHDO_ZEROL,   ( DHDOSync.Sync_ZeroLevel | DHDOSync.Sync_ZeroLevel << 16) );

        halvtg_Write32((U32)(BaseAddress_p + DHDO_MIDL), ( DHDOSync.Sync_MidLevel| ( DHDOSync.Sync_MidLevel << 16)));
        halvtg_Write32((U32)(BaseAddress_p + DHDO_HIGHL),( DHDOSync.Sync_HighLevel| ( DHDOSync.Sync_HighLevel << 16)));

        halvtg_Write32((U32)(BaseAddress_p + DHDO_YOFF), DHDOSync.Sync_YOff);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_COFF), DHDOSync.Sync_COff);

        halvtg_Write32((U32)(BaseAddress_p + DHDO_YMLT), DHDOSync.Sync_YMult);
        halvtg_Write32((U32)(BaseAddress_p + DHDO_CMLT), DHDOSync.Sync_CMult);

        /* if(DHDOSyncInLevel[Index].TimingMode == STVTG_TIMING_MODE_COUNT)
        *  nothing to do
        */

    }
    return(ST_NO_ERROR);
} /* end of stvtg_HALSetSyncInSettings() function */

/*******************************************************************************
Name        : stvtg_HALSetLevelSettings
Description : set settings for embbeded synchronizations
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_HALSetLevelSettings(const stvtg_Device_t * const Device_p, STVTG_SyncLevel_t *SyncLevel_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U8 Index = 0;

    while(DHDOSyncInLevel[Index].TimingMode != STVTG_TIMING_MODE_COUNT)
    {
        if (DHDOSyncInLevel[Index].TimingMode == SyncLevel_p->TimingMode)
        {
            DHDOSyncInLevel[Index].Sync = SyncLevel_p->Sync;
        }
        if(SyncLevel_p->TimingMode == Device_p->TimingMode)
        {
            ErrCode = stvtg_HALSetSyncInSettings(Device_p);
            break;
        }
        Index++;
    }
    /* if(DHDOSyncInLevel[Index].TimingMode == STVTG_TIMING_MODE_COUNT)
     *  nothing to do
     */
    return (ErrCode);

}/*stvtg_HALSetLevelSettings*/

/*******************************************************************************
Name        : stvtg_HALGetLevelSettings
Description : set settings for embbeded synchronizations
Parameters  : Device_p : IN/OUT : Device associated data to be updated
Assumptions : Device_p not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvtg_HALGetLevelSettings(const stvtg_Device_t * const Device_p,  STVTG_SyncLevel_t * SyncLevel_p)
{
   /* STVTG_DHDOSyncInLevel_t DHDOSync;*/
    U8 Index = 0;

    while(DHDOSyncInLevel[Index].TimingMode != STVTG_TIMING_MODE_COUNT)
    {
        if (DHDOSyncInLevel[Index].TimingMode == Device_p->TimingMode)
        {
            SyncLevel_p->Sync = DHDOSyncInLevel[Index].Sync ;
            SyncLevel_p->TimingMode = DHDOSyncInLevel[Index].TimingMode ;
            break;
        }
        Index++;
    }
    if(DHDOSyncInLevel[Index].TimingMode == STVTG_TIMING_MODE_COUNT)
                return (ST_ERROR_FEATURE_NOT_SUPPORTED);

    return (ST_NO_ERROR);

}/*stvtg_HALGetLevelSettings*/


/* ----------------------------- End of file ------------------------------ */

