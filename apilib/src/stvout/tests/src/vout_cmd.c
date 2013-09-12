/************************************************************************

File name   : vout_cmd.c
Description : VOUT macros
Note        : All functions return TRUE if error for CLILIB compatibility

COPYRIGHT (C) STMicroelectronics 2003.

Date          Modification                                    Initials
----          ------------                                    --------
31 Aug 2000   Creation                                        JG
24 Apr 2002   Updates for 5516/7020                           HSdLM
07 Feb 2003   Add 5517 support                                HSdLM
12 May 2003   Add 5528 and 7020 STEM support                  HSdLM
01 Sep 2004   Add 7710 support                                AC
13 Sep 2004   Add ST40/OS21 support                           MH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* uncomment for debugging purpose only */
/*#define VOUT_DEBUG*/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"

#include "stcommon.h"

#include "sttbx.h"

#include "testtool.h"
#include "stdenc.h"
#include "stvout.h"

#ifdef VOUT_DEBUG
#include "../../src/vout_drv.h"
#endif

#include "vout_cmd.h"
#include "api_cmd.h"
#include "startup.h"
#if defined(ST_OSLINUX)
#include "iocstapigat.h"
#endif
#include "vtg_cmd.h"
#include "clevt.h"
#include "clpio.h"
#include "cli2c.h"
#include "clpwm.h"
#include "testcfg.h"

#ifdef STVOUT_HDCP_PROTECTION
#if defined (ST_7100)|| defined (ST_7109)||defined(ST_7710)
#include "hdcp_710x.h"
#endif
#if defined(ST_7200)|| defined(ST_7111)|| defined(ST_7105)
#include "hdcp_7200.h"
#endif
#endif



#if defined (mb376) || defined (espresso)
    #include "sti4629.h"
#endif /*defined mb376 || defined espresso*/

/* Private Types ------------------------------------------------------------ */
typedef U8 halvout_DencVersion_t;

typedef struct VoutTable_s
{
    char Name[15];
    U32  Val;
} VoutTable_t;

/*typedef struct VoutHyperTable_s*/
/*{*/
/*    VoutTable_t *VoutTable_p;*/
/*    U32         Size;*/
/*} VoutHyperTable_t;*/

/* Private Constants -------------------------------------------------------- */

/* Patch for chip with no video like ST40GX1 */
#ifndef VIDEO_BASE_ADDRESS
#define VIDEO_BASE_ADDRESS        0
#endif

#define VOUT_MAX_SUPPORTED     14
#define STRING_DEVICE_LENGTH   80
#define RETURN_PARAMS_LENGTH   50

#define VOUT_MAX_FORMAT_SUPPORTED  4
#define VOUT_MAX_CLDELAY_SUPPORTED 10

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) ||  defined(ST_5518) || defined(ST_5516) || \
    ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))
#define VOUT_BASE_ADDRESS         0
#define VOUT_DEVICE_BASE_ADDRESS  0
#if defined STVOUT_DACS_ENHANCED
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_DENC_ENHANCED
#else
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_DENC
#endif
#define VOUT_TYPE                 "TYPE_DENC"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#define DVO_CTL                   0x32 /* VID_656 */
#define REG_SHIFT                 0

#elif (defined(ST_5528) && !defined(ST_7020))
/* VOUT_BASE_ADDRESS defined in chip */
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5528
#define VOUT_TYPE                 "TYPE_5528"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#endif /** ST_OSLINUX **/
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS
#define DVO_CTL                   0x00 /* DVO_CTL_656 */
#define REG_SHIFT                 2
#ifdef ST_OSLINUX
#define VOUT_WIDTH
#endif
#ifdef ST_OS21
#define VOS_MISC_CTL                         0xB9011C00  /*only for output on mixer2*/
#endif
#ifdef ST_OS20
#define VOS_MISC_CTL                         0x19011C00   /*only for output on mixer2*/
#endif
#define VOS_MISC_CTL_DENC_AUX_SEL            0x1
#if defined(mb376) || defined(espresso)
    #ifdef ST_OS21
    #define ST4629_BASE_ADDRESS STI4629_ST40_BASE_ADDRESS
    #endif /* ST_OS21 */
    #ifdef ST_OS20
    #define ST4629_BASE_ADDRESS  STI4629_BASE_ADDRESS
    #endif /* ST_OS20 */
    #ifdef ST_OSLINUX
    #define ST4629_BASE_ADDRESS  0
    #endif /* ST_OS20 */

    #define STI4629_DNC_BASE_ADDRESS ST4629_DENC_OFFSET
    #define REG_SHIFT_4629             0
#endif /* defined mb376 || defined espresso*/

#elif defined (ST_5100)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13 /* STVOUT_DEVICE_TYPE_V13 For STi5100  */
#define VOUT_TYPE                 "TYPE_5100"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DIGOUT_BASE_ADDRESS       0x20900600
#define DVO_CTL                   0x00 /* DVO_CTL_656 */
#define REG_SHIFT                 2

#elif defined (ST_5105)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define VOUT_TYPE                 "TYPE_5105"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DVO_CTL                   0x0
#define REG_SHIFT                 2

#elif defined (ST_5107)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define VOUT_TYPE                 "TYPE_5107"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DVO_CTL                   0x0
#define REG_SHIFT                 2
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS

#define PIO_CLEAR_PnC0              0x28    /* W   */
#define PIO_PnC1                    0x30    /* R/W */
#define PIO_PnC2                    0x40    /* R/W */
#define PIO_SET_PnC1                0x34    /* W   */
#define PIO_SET_PnC2                0x44    /* W   */
#define INT_CONFIG_CONTROL_F        (ST5107_CFG_BASE_ADDRESS + 0xc)
#define INT_CONFIG_CONTROL_G        (ST5107_CFG_BASE_ADDRESS + 0x10)

#elif defined (ST_5162)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define VOUT_TYPE                 "TYPE_5162"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DVO_CTL                   0x0
#define REG_SHIFT                 2
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS

#define PIO_CLEAR_PnC0              0x28    /* W   */
#define PIO_PnC1                    0x30    /* R/W */
#define PIO_PnC2                    0x40    /* R/W */
#define PIO_SET_PnC1                0x34    /* W   */
#define PIO_SET_PnC2                0x44    /* W   */
#define INT_CONFIG_CONTROL_F        (ST5162_CFG_BASE_ADDRESS + 0xc)
#define INT_CONFIG_CONTROL_G        (ST5162_CFG_BASE_ADDRESS + 0x10)

#elif defined (ST_5188)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5525
#define VOUT_TYPE                 "TYPE_5188"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DVO_CTL                   0x0
#define REG_SHIFT                 2
#elif defined (ST_5525)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5525
#define VOUT_TYPE                 "TYPE_5525"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DVO_CTL                   0x0
#define REG_SHIFT                 2
#define VOUT_BASE_ADDRESS         ST5525_VOUT_0_BASE_ADDRESS
#elif defined (ST_5301)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define VOUT_TYPE                 "TYPE_5301"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DIGOUT_BASE_ADDRESS       0x20900600
#define DVO_CTL                   0x00 /* DVO_CTL_656 */
#define REG_SHIFT                 2

#elif defined(ST_7015)
#define VOUT_BASE_ADDRESS         ST7015_DSPCFG_OFFSET
#define VOUT_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7015
#define VOUT_TYPE                 "TYPE_7015"
#define BASE_ADDRESS_DENC         STI7015_BASE_ADDRESS + ST7015_DENC_OFFSET
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#define REG_SHIFT                 2

#elif defined(ST_7020)
#define VOUT_BASE_ADDRESS         ST7020_DSPCFG_OFFSET
#define VOUT_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7020
#define VOUT_TYPE                 "TYPE_7020"
#define BASE_ADDRESS_DENC         STI7020_BASE_ADDRESS + ST7020_DENC_OFFSET
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#define REG_SHIFT                 2
#if defined (ST_5528)
    #define VOUT_5528_DEVICE_BASE_ADDRESS   0
    #define BASE_ADDRESS_DENC_5528          ST5528_DENC_BASE_ADDRESS
    #define DIGOUT_5528_BASE_ADDRESS        ST5528_DVO_BASE_ADDRESS
    #define DVO_CTL                         0x00 /* DVO_CTL_656 */
    #define VOS_MISC_CTL_DENC_AUX_SEL       0x1
    #define VOS_MISC_CTL                    0x19011C00  /*only for output on mixer2*/
    #define STI4629_DNC_BASE_ADDRESS        ST4629_DENC_OFFSET
    #define REG_SHIFT_4629                  0
#endif /* ST_5528 */
#elif defined(ST_7710)
#define VOUT_BASE_ADDRESS         ST7710_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7710
#define VOUT_TYPE                 "TYPE_7710"
#define BASE_ADDRESS_DENC         DENC_BASE_ADDRESS
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         ST7710_HDMI_BASE_ADDRESS
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         ST7710_HDCP_BASE_ADDRESS
#endif
#define VOUT_INTERRUPT_NUMBER     ST7710_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define DSPCFG_CLK                0x70       /*only for output on mixer2*/
#define DSPCFG_CLK_DENC_MAIN_SEL  0x1

#elif defined(ST_7100)
#define VOUT_BASE_ADDRESS         ST7100_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7100
#define VOUT_TYPE                 "TYPE_7100"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         ST7100_DENC_BASE_ADDRESS
#endif
#define DIGOUT_BASE_ADDRESS       ST7100_VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         0xB9005800
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         0xB9005400
#endif
#define VOUT_INTERRUPT_NUMBER     ST7100_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define DSPCFG_CLK                   0x70
#define DSPCFG_CLK_DENC_MAIN_SEL     0x1

#elif defined(ST_7109)
#define VOUT_BASE_ADDRESS         ST7109_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7100
#define VOUT_TYPE                 "TYPE_7109"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         ST7109_DENC_BASE_ADDRESS
#endif
#define DIGOUT_BASE_ADDRESS       ST7109_VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         0xB9005800
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         0xB9005400
#endif
#define VOUT_INTERRUPT_NUMBER     ST7109_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define DSPCFG_CLK                   0x70
#define DSPCFG_CLK_DENC_MAIN_SEL     0x1

#elif defined (ST_7105)
#define VOUT_BASE_ADDRESS         ST7105_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7105
#define VOUT_TYPE                 "TYPE_7105"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         ST7105_DENC_BASE_ADDRESS
#endif
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         0xfd106000
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         0xfd106800
#endif
#define VOUT_INTERRUPT_NUMBER     ST7105_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define HD_TVOUT_HDF_BASE_ADDRESS ST7105_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined (ST_7111)
#define VOUT_BASE_ADDRESS         ST7111_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7111
#define VOUT_TYPE                 "TYPE_7111"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         ST7111_DENC_BASE_ADDRESS
#endif
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         0xfd106000
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         0xfd106800
#endif
#define VOUT_INTERRUPT_NUMBER     ST7111_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define HD_TVOUT_HDF_BASE_ADDRESS ST7111_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined(ST_7200)
#define VOUT_BASE_ADDRESS         ST7200_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7200
#define VOUT_TYPE                 "TYPE_7200"
#if !defined(ST_OSLINUX)
#define BASE_ADDRESS_DENC         ST7200_DENC_BASE_ADDRESS
#endif
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#ifndef HDMI_BASE_ADDRESS
#define HDMI_BASE_ADDRESS         0xfd106000
#endif
#ifndef HDCP_BASE_ADDRESS
#define HDCP_BASE_ADDRESS         0xfd106800
#endif
#define VOUT_INTERRUPT_NUMBER     ST7200_HDMI_INTERRUPT
#define REG_SHIFT                 2
#define HD_TVOUT_HDF_BASE_ADDRESS ST7200_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined(ST_GX1)
#define VOUT_BASE_ADDRESS_DENCIF1 0xBB440010
#define VOUT_BASE_ADDRESS_DENCIF2 0xBB440020
#define VOUT_BASE_ADDRESS         VOUT_BASE_ADDRESS_DENCIF2
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_GX1
#define VOUT_TYPE                 "TYPE_GX1"
#define BASE_ADDRESS_DENC         STGX1_DENC_BASE_ADDRESS
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS
#define REG_SHIFT                 3

#else
#error Not defined yet
#endif

/* Private variables ------------------------------------------------------ */

static char                     VOUT_Msg[255];  /* text for trace */
static STVOUT_Handle_t          VOUTHndl;
static STVOUT_OutputParams_t    Str_OutParam;
#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)|| defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#ifdef STVOUT_HDCP_PROTECTION
static STVOUT_HDCPParams_t      Str_HdcpParam;
#endif

#define VOUT_MAX_DATA_SIZE      17
static U8 Data [VOUT_MAX_DATA_SIZE] = {2,1,13,28,26,0,0,0,60,1,165,0,0,2,209};

#define VOUT_MAX_KSV_SIZE       5*4
#ifdef STVOUT_HDCP_PROTECTION
static U8 ForbiddenKSV[VOUT_MAX_KSV_SIZE] ={0xd5,0xda,0xd5,0xd5,0x34,
                                            0xe6,0xe6,0xe6,0xe6,0xc5,
                                            0x14,0x14,0x14,0x14,0xe4,
                                            0xa9,0xa9,0xa9,0xa9,0x74};/* Key1:Sharp; Key2:Sony; Key3:Hitachi; Key4:Sagem change VOUT_MAX_KSV_SIZE*/
#endif
#endif
#define VOUT_TYPE_NAME_SIZE VOUT_MAX_SUPPORTED+2
static VoutTable_t VoutTypeName[VOUT_TYPE_NAME_SIZE]={
    { "<", STVOUT_OUTPUT_ANALOG_RGB-1 },
    { "RGB", STVOUT_OUTPUT_ANALOG_RGB },
    { "YUV", STVOUT_OUTPUT_ANALOG_YUV },
    { "YC", STVOUT_OUTPUT_ANALOG_YC },
    { "CVBS", STVOUT_OUTPUT_ANALOG_CVBS },
    { "SVM", STVOUT_OUTPUT_ANALOG_SVM },
    { "YCbCr444", STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS },
    { "YCbCr422_16", STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED },
    { "YCbCr422_8", STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED },
    { "RGB888", STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS },
    { "HD_RGB", STVOUT_OUTPUT_HD_ANALOG_RGB },
    { "HD_YUV", STVOUT_OUTPUT_HD_ANALOG_YUV },
    { "HDMI_RGB888", STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 },
    { "HDMI_YcbCr444", STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 },
    { "HDMI_YCbCr422" , STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 },
    { ">", STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422+1}
};
#if defined(ST_7710)||defined(ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define VOUT_FOMAT_NAME_SIZE VOUT_MAX_FORMAT_SUPPORTED+2
static VoutTable_t  VoutFormatName[VOUT_FOMAT_NAME_SIZE]={
    { "<"   , STVOUT_SD_MODE-1  },
    { "SD"  , STVOUT_SD_MODE    },
    { "ED"  , STVOUT_ED_MODE    },
    { "HD"  , STVOUT_HD_MODE    },
    { "HDRQ", STVOUT_HDRQ_MODE  },
    { ">"   , STVOUT_HDRQ_MODE+1}
};
#endif

#define VOUT_CLDELAY_NAME_SIZE VOUT_MAX_CLDELAY_SUPPORTED+2
static VoutTable_t  VoutCLDelayName[VOUT_CLDELAY_NAME_SIZE]={
    { "<"          , STVOUT_CHROMALUMA_DELAY_M2-1   },
    { "DELAY_M2"   , STVOUT_CHROMALUMA_DELAY_M2     },
    { "DELAY_M1_5" , STVOUT_CHROMALUMA_DELAY_M1_5   },
    { "DELAY_M1"   , STVOUT_CHROMALUMA_DELAY_M1     },
    { "DELAY_M0_5" , STVOUT_CHROMALUMA_DELAY_M0_5   },
    { "DELAY_0"    , STVOUT_CHROMALUMA_DELAY_0      },
    { "DELAY_0_5"  , STVOUT_CHROMALUMA_DELAY_0_5    },
    { "DELAY_P1"   , STVOUT_CHROMALUMA_DELAY_P1     },
    { "DELAY_P1_5" , STVOUT_CHROMALUMA_DELAY_P1_5   },
    { "DELAY_P2"   , STVOUT_CHROMALUMA_DELAY_P2     },
    { "DELAY_P2_5" , STVOUT_CHROMALUMA_DELAY_P2_5   },
    { ">"          , STVOUT_CHROMALUMA_DELAY_P2_5+1 }
};

typedef enum
{
    VOUT_TYPE_NAME,
    TRIDAC_NAME,
    TRIDAC_PATH_NAME,
    TRIDAC_COMPONENT_NAME
} VoutTableList_t;

/* Global Variables --------------------------------------------------------- */

#ifdef VOUT_DEBUG
extern stvout_Unit_t *stvout_DbgPtr;
extern stvout_Device_t *stvout_DbgDev;
#endif

BOOL VoutTest_PrintDotsIfOk;
#ifdef ST_OSLINUX
MapParams_t   OutputStageMap;
MapParams_t   CompoMap;

#if defined (ST_5528)
#define VOS_MISC_CTL            OutputStageMap.MappedBaseAddress + 0xC00   /*only for output on mixer2*/
#define BASE_ADDRESS_DENC       OutputStageMap.MappedBaseAddress
#elif defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
MapParams_t   DencMap;
#define BASE_ADDRESS_DENC         (DencMap.MappedBaseAddress)
#define VOS_BASE_ADDRESS_1        (OutputStageMap.MappedBaseAddress)

#define HDMI_MAPPED_BASE_ADDRESS  ((U32)OutputStageMap.MappedBaseAddress + (U32)0x800)

#define VMIX1_BASE_ADDRESS       ((U32)CompoMap.MappedBaseAddress + (U32)0xC00)
#define VMIX2_BASE_ADDRESS       ((U32)CompoMap.MappedBaseAddress + (U32)0xD00)
#endif
#else
#define VOS_BASE_ADDRESS_1          VOS_BASE_ADDRESS
#endif

#if !defined(ST_OSLINUX)
#if defined(ST_7100) || defined (ST_7109)
#define VMIX1_BASE_ADDRESS       VMIX1_LAYER_BASE_ADDRESS
#define VMIX2_BASE_ADDRESS       VMIX2_LAYER_BASE_ADDRESS
#endif
#endif /** ST_OSLINUX **/
/* Private Macros ----------------------------------------------------------- */
#define stvout_rd32(a)     STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)   STSYS_WriteRegDev32LE((void*)(a),(v))
#define stvout_rd8(a)     STSYS_ReadRegDev8((void*)(a))
#define stvout_wr8(a,v)   STSYS_WriteRegDev8((void*)(a),(v))


#define VOUT_PRINT(x) { \
    /* Check lenght */ \
    if (strlen(x)>sizeof(VOUT_Msg)) \
    { \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* Private Function prototypes ---------------------------------------------- */

static BOOL VOUT_GetRevision(        STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_GetCapability(      STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Init(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Open(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Close(              STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Term(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Enable(             STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_Disable(            STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_GetOption(          STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetOption(          STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_GetParams(          STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetParams(          STTST_Parse_t *pars_p, char *result_sym_p );

#if defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
static BOOL VOUT_GetState(           STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_GetTargetInfo(      STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetActiveVideo(     STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SendData(           STTST_Parse_t *pars_p, char *result_sym_p );
#if defined (STVOUT_HDCP_PROTECTION)
static BOOL VOUT_SetHDCPParams(      STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_GetHDCPSinkParams(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetForbiddenKSVs(  STTST_Parse_t *pars_p, char *result_sym_p );
#endif
static BOOL VOUT_Start(              STTST_Parse_t *pars_p, char *result_sym_p );
#endif

static BOOL VOUT_SetAnalogDacParams( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetAnalogBCSParams( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetEmbeddedSyncro(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetAnalogInverted(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_SetAnalogSVM(       STTST_Parse_t *pars_p, char *result_sym_p );
#if defined (ST_5528)|| defined (ST_5100)|| defined (ST_7710)||defined(ST_7100) ||defined (ST_5301)|| defined (ST_7109)
static BOOL VOUT_SetDencAuxMain(         STTST_Parse_t *pars_p, char *result_sym_p );
#endif
static BOOL VOUT_ReadDac(            STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_ReadBCS(            STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_ReadChroma(         STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_ReadLuma(           STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_ReadDSPCFG(         STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_ReadSVM(            STTST_Parse_t *pars_p, char *result_sym_p );
#ifdef VOUT_DEBUG
static BOOL VOUT_StatusDriver(       STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_AllTermInitDriver(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL DENC_Version(            STTST_Parse_t *pars_p, char *result_sym_p );
#endif


static BOOL VOUT_InitCommand(void);
static halvout_DencVersion_t halvout_DencVersionOnChip(int DevTyp);
static void Get_RGB_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_YUV_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_YC_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_CVBS_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_BCS_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_Chroma_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_Luma_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_Inv_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_Emb_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_HD_Dig_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_SVM_Params( STVOUT_OutputParams_t Params, BOOL Print);
static void Get_Vout_Params( STVOUT_OutputParams_t Params, int out, BOOL Print);
static void Print_Vout_Capabilitity( STVOUT_Capability_t Cp);

/* Functions ---------------------------------------------------------------- */
static int shift( int n)   {   int p;  for ( p=0; n!=0; p++, n>>=1); return(p); }


/*-----------------------------------------------------------------------------
 * Function : VOUT_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL VOUT_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if (API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVOUT_ERROR_DENC_ACCESS:
                strcat( Text, "(stvout error denc access failed) !\n");
                break;
            case STVOUT_ERROR_VOUT_ENABLE:
                strcat( Text, "(stvout error output is already enable) !\n");
                break;
            case STVOUT_ERROR_VOUT_NOT_ENABLE:
                strcat( Text, "(stvout error output is already disable) !\n");
                break;
            case STVOUT_ERROR_VOUT_NOT_AVAILABLE:
                strcat( Text, "(stvout error Output not available) !\n");
                break;
            case STVOUT_ERROR_VOUT_INCOMPATIBLE:
                strcat( Text, "(stvout error Incompatible Output) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VOUT_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
} /* end of VOUT_TextError */

/*-----------------------------------------------------------------------------
 * Function : VOUT_DotTextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL VOUT_DotTextError(ST_ErrorCode_t ErrorCode, char *Text)
{
    BOOL RetErr = FALSE;

    if ((ErrorCode == ST_NO_ERROR) && VoutTest_PrintDotsIfOk)
    {
        STTBX_Print(("."));
    }
    else
    {
        RetErr = VOUT_TextError( ErrorCode, Text);
    }
    return( API_EnableError ? RetErr : FALSE);

} /* end of VOUT_DotTextError */


#define st_rd32(a)      STSYS_ReadRegDev32LE((void*)(a))

#if defined (ST_7015) || defined (ST_7020)|| defined (ST_7710)|| defined(ST_7100)|| defined (ST_7109)
static void vout_read32( void* Address, U32* pData)
{
    U32* Add = (U32*)Address;

    *pData = ( (U32) st_rd32(Add) );
}
#endif

static halvout_DencVersion_t halvout_DencVersionOnChip(int DevTyp)
{

#if defined (ST_7109)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107) || defined (ST_7200)|| defined (ST_5162)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(DevTyp);
        return ( (halvout_DencVersion_t)12);
#else
    U8 IdCell = 0, Iscid , RegisterShift = REG_SHIFT;
    long base = (long)BASE_ADDRESS_DENC;
#define _ISCID  (U16)17    /* test for chid */
#define _CID    (U16)0x18  /* Denc macro-cell identification number   [7:0]   */
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    if ( DevTyp == 1)
    {
        base = (long)ST4629_BASE_ADDRESS +(long)STI4629_DNC_BASE_ADDRESS ;
        RegisterShift = REG_SHIFT_4629 ;
    }
#else /* ((defined (mb376) && !defined(ST_7020)) || defined (espresso))*/
    UNUSED_PARAMETER(DevTyp);
#endif

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || defined(mb376) ||  \
    defined(espresso)||((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

    IdCell = *(volatile U8 *)(base + ((long)_ISCID << RegisterShift));
    *(volatile U8 *)(base + ((long)_ISCID << RegisterShift))=(U8) (~IdCell);
    Iscid  = *(volatile U8 *)(base + ((long)_ISCID << RegisterShift));
 #else
    IdCell = (U8)(*(volatile U32 *)(base + ((long)_ISCID << RegisterShift)));
    *(volatile U32 *)(base + ((long)_ISCID << RegisterShift))=(U32) (~IdCell);
    Iscid  = (U8)(*(volatile U32 *)(base + ((long)_ISCID << RegisterShift)));
 #endif

    if (IdCell != Iscid)    /* Version >=6, else if (IdCell == Iscid) => Version 5 */
    {
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || defined(mb376) || \
    defined (espresso)|| ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

        *(volatile U8 *)(base + ((long)_ISCID << RegisterShift))=(U8) (IdCell);
        IdCell  = *(volatile U8 *)(base + ((long)_CID << RegisterShift));
#else
        *(volatile U32 *)(base + ((long)_ISCID << RegisterShift))=(U32) (IdCell);
        IdCell  = (U8)(*(volatile U32 *)(base + ((long)_CID << RegisterShift)));
#endif
    }
    if (IdCell == 119) /* special case of Stv0119 chip */
    {
        return ( (halvout_DencVersion_t)3);
    }
    else
    {
        return((halvout_DencVersion_t)((IdCell & 0xF0) >> 4));
    }
#endif
}

static void Get_RGB_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P1", Params.Analog.AnalogLevel.RGB.R, FALSE);
    STTST_AssignInteger( "P2", Params.Analog.AnalogLevel.RGB.G, FALSE);
    STTST_AssignInteger( "P3", Params.Analog.AnalogLevel.RGB.B, FALSE);
    if ( Print)
    {
        sprintf( VOUT_Msg, ">>   RGB,           R:%1d  G:%1d  B:%1d\n",
                Params.Analog.AnalogLevel.RGB.R,
                Params.Analog.AnalogLevel.RGB.G,
                Params.Analog.AnalogLevel.RGB.B);
        STTBX_Print(( VOUT_Msg ));
    }
}

static void Get_YUV_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P1", Params.Analog.AnalogLevel.YUV.Y, FALSE);
    STTST_AssignInteger( "P2", Params.Analog.AnalogLevel.YUV.U, FALSE);
    STTST_AssignInteger( "P3", Params.Analog.AnalogLevel.YUV.V, FALSE);
    if ( Print)
    {
        sprintf( VOUT_Msg, ">>   YUV,           Y:%1d  U:%1d  V:%1d\n",
                Params.Analog.AnalogLevel.YUV.Y,
                Params.Analog.AnalogLevel.YUV.U,
                Params.Analog.AnalogLevel.YUV.V);
        STTBX_Print(( VOUT_Msg ));
    }
}

static void Get_YC_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P1", Params.Analog.AnalogLevel.YC.Y, FALSE);
    STTST_AssignInteger( "P2", Params.Analog.AnalogLevel.YC.C, FALSE);
    STTST_AssignInteger( "P3", Params.Analog.AnalogLevel.YC.YCRatio, FALSE);
    if ( Print)
    {
        sprintf( VOUT_Msg, ">>   YC,            Y:%1d  C:%1d  YCRatio:%1d\n",
                Params.Analog.AnalogLevel.YC.Y,
                Params.Analog.AnalogLevel.YC.C,
                Params.Analog.AnalogLevel.YC.YCRatio);
        STTBX_Print(( VOUT_Msg ));
    }
}

static void Get_CVBS_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P1", Params.Analog.AnalogLevel.CVBS.CVBS, FALSE);
    STTST_AssignInteger( "P2", Params.Analog.AnalogLevel.CVBS.YCRatio, FALSE);
    if ( Print)
    {
        sprintf( VOUT_Msg, ">>   CVBS,          CVBS:%1d  YCRatio:%1d\n",
                Params.Analog.AnalogLevel.CVBS.CVBS,
                Params.Analog.AnalogLevel.CVBS.YCRatio);
        STTBX_Print(( VOUT_Msg ));
    }
}

static void Get_BCS_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P4", Params.Analog.BCSLevel.Brightness, FALSE);
    STTST_AssignInteger( "P5", Params.Analog.BCSLevel.Contrast, FALSE);
    STTST_AssignInteger( "P6", Params.Analog.BCSLevel.Saturation, FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   Brightness: %d, Contrast: %d, Saturation: %d \n",
                            Params.Analog.BCSLevel.Brightness,
                            Params.Analog.BCSLevel.Contrast,
                            Params.Analog.BCSLevel.Saturation));
    }
}

static void Get_Chroma_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P10", Params.Analog.ChrLumFilter.Chroma[0], FALSE);
    STTST_AssignInteger( "P11", Params.Analog.ChrLumFilter.Chroma[1], FALSE);
    STTST_AssignInteger( "P12", Params.Analog.ChrLumFilter.Chroma[2], FALSE);
    STTST_AssignInteger( "P13", Params.Analog.ChrLumFilter.Chroma[3], FALSE);
    STTST_AssignInteger( "P14", Params.Analog.ChrLumFilter.Chroma[4], FALSE);
    STTST_AssignInteger( "P15", Params.Analog.ChrLumFilter.Chroma[5], FALSE);
    STTST_AssignInteger( "P16", Params.Analog.ChrLumFilter.Chroma[6], FALSE);
    STTST_AssignInteger( "P17", Params.Analog.ChrLumFilter.Chroma[7], FALSE);
    STTST_AssignInteger( "P18", Params.Analog.ChrLumFilter.Chroma[8], FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   Chroma: %d %d %d %d %d %d %d %d %d\n",
                            Params.Analog.ChrLumFilter.Chroma[0],
                            Params.Analog.ChrLumFilter.Chroma[1],
                            Params.Analog.ChrLumFilter.Chroma[2],
                            Params.Analog.ChrLumFilter.Chroma[3],
                            Params.Analog.ChrLumFilter.Chroma[4],
                            Params.Analog.ChrLumFilter.Chroma[5],
                            Params.Analog.ChrLumFilter.Chroma[6],
                            Params.Analog.ChrLumFilter.Chroma[7],
                            Params.Analog.ChrLumFilter.Chroma[8]));
    }
}

static void Get_Luma_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P20", Params.Analog.ChrLumFilter.Luma[0], FALSE);
    STTST_AssignInteger( "P21", Params.Analog.ChrLumFilter.Luma[1], FALSE);
    STTST_AssignInteger( "P22", Params.Analog.ChrLumFilter.Luma[2], FALSE);
    STTST_AssignInteger( "P23", Params.Analog.ChrLumFilter.Luma[3], FALSE);
    STTST_AssignInteger( "P24", Params.Analog.ChrLumFilter.Luma[4], FALSE);
    STTST_AssignInteger( "P25", Params.Analog.ChrLumFilter.Luma[5], FALSE);
    STTST_AssignInteger( "P26", Params.Analog.ChrLumFilter.Luma[6], FALSE);
    STTST_AssignInteger( "P27", Params.Analog.ChrLumFilter.Luma[7], FALSE);
    STTST_AssignInteger( "P28", Params.Analog.ChrLumFilter.Luma[8], FALSE);
    STTST_AssignInteger( "P29", Params.Analog.ChrLumFilter.Luma[9], FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   Luma  : %d %d %d %d %d %d",
                            Params.Analog.ChrLumFilter.Luma[0],
                            Params.Analog.ChrLumFilter.Luma[1],
                            Params.Analog.ChrLumFilter.Luma[2],
                            Params.Analog.ChrLumFilter.Luma[3],
                            Params.Analog.ChrLumFilter.Luma[4],
                            Params.Analog.ChrLumFilter.Luma[5]));
        STTBX_Print(( " %d %d %d %d\n",
                            Params.Analog.ChrLumFilter.Luma[6],
                            Params.Analog.ChrLumFilter.Luma[7],
                            Params.Analog.ChrLumFilter.Luma[8],
                            Params.Analog.ChrLumFilter.Luma[9]));
    }
}

static void Get_Inv_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P8", Params.Analog.InvertedOutput, FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   InvertedOutput %x\n", Params.Analog.InvertedOutput));
    }
}

static void Get_Emb_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P7", Params.Analog.EmbeddedType, FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   EmbeddedType   %x\n", Params.Analog.EmbeddedType));
    }
}

static void Get_HD_Dig_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P9", Params.Digital.EmbeddedType, FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   EmbeddedType %x\n", Params.Digital.EmbeddedType));
    }
}

static void Get_SVM_Params( STVOUT_OutputParams_t Params, BOOL Print)
{
    STTST_AssignInteger( "P10", Params.SVM.DelayCompensation, FALSE);
    STTST_AssignInteger( "P11", Params.SVM.Shape, FALSE);
    STTST_AssignInteger( "P12", Params.SVM.Gain, FALSE);
    STTST_AssignInteger( "P13", Params.SVM.OSDFactor, FALSE);
    STTST_AssignInteger( "P14", Params.SVM.VideoFilter, FALSE);
    STTST_AssignInteger( "P15", Params.SVM.OSDFilter, FALSE);
    if ( Print)
    {
        STTBX_Print(( ">>   DelayCompensation %x\n", Params.SVM.DelayCompensation));
        STTBX_Print(( ">>   Shape %x\n", Params.SVM.Shape));
        STTBX_Print(( ">>   Gain %x\n", Params.SVM.Gain));
        STTBX_Print(( ">>   OSDFactor %x\n", Params.SVM.OSDFactor));
        STTBX_Print(( ">>   VideoFilter %x\n", Params.SVM.VideoFilter));
        STTBX_Print(( ">>   OSDFilter %x\n", Params.SVM.OSDFilter));
    }
}


static void Get_Vout_Params( STVOUT_OutputParams_t Params, int out, BOOL Print)
{
    switch ( out)
    {
        case 1 :
        case 2 :
        case 3 :
        case 4 :
            if ( Print)
            {
                STTBX_Print(( ">>  Analog :\n" ));
            }
            if ( out == 1) /* Analog RGB */
            {
                Get_RGB_Params( Params, Print);
            }
            if ( out == 2) /* Analog YUV */
            {
                Get_YUV_Params( Params, Print);
            }
            if ( out == 3) /* Analog YC */
            {
                Get_YC_Params( Params, Print);
            }
            if ( out == 4) /* Analog CVBS */
            {
                Get_CVBS_Params( Params, Print);
            }
            Get_BCS_Params( Params, Print);
            Get_Emb_Params( Params, Print);
            Get_Inv_Params( Params, Print);
            Get_Chroma_Params( Params, Print);
            Get_Luma_Params( Params, Print);
            break;

        case 5 :
            if ( Print)
            {
                STTBX_Print(( ">>  SVM\n"));
            }
            Get_SVM_Params( Params, Print);
            break;

        case 6 :
        case 7 :
        case 8 :
        case 9 :
            if ( Print)
            {
                STTBX_Print(( ">>  HD Digital\n"));
            }
            Get_HD_Dig_Params( Params, Print);
            break;

        case 10 :
        case 11 :
            if ( Print)
            {
                STTBX_Print(( ">>  HD Analog\n"));
            }
            Get_Emb_Params( Params, Print);
            break;

        default :
            break;
    }
}

static void Print_Vout_Capabilitity( STVOUT_Capability_t Cp)
{
    int AnalogPictureControlCapable;
    char MsgAOAC[80] = ">> Capability : SupportedAnalogOutputsAdjustable : ";
    char MsgAPCC[80] = ">> Capability : AnalogPictureControlCapable : ";
    char MsgALFC[80] = ">> Capability : AnalogLumaChromaFilteringCapable : ";
    char MsgAnal[80] = ">>  Analog    : ";
    char MsgDigi[80] = ">>  Digital   : ";
    char MsgSele[80] = ">> Capability : Selected : ";
    char MsgSupp[80] = ">>  Supported : ";
    char MsgESCa[80] = ">> Capability : EmbeddedSynchroCapable : ";

    if ( Cp.AnalogOutputsAdjustableCapable)   strcat( MsgAOAC, "Yes\n");
    else                                      strcat( MsgAOAC, "No\n");

    if ( Cp.AnalogPictureControlCapable)      strcat( MsgAPCC, "Yes\n");
    else                                      strcat( MsgAPCC, "No\n");

    if ( Cp.AnalogLumaChromaFilteringCapable) strcat( MsgALFC, "Yes\n");
    else                                      strcat( MsgALFC, "No\n");

    if ( Cp.EmbeddedSynchroCapable)           strcat( MsgESCa, "Yes\n");
    else                                      strcat( MsgESCa, "No\n");

    STTBX_Print(( ">> Capability : SupportedOutputs : \n"));
    if ( Cp.SupportedOutputs == 0x0)
    {
        STTBX_Print(( "0 \n"));
    }
    else
    {
        if ( (Cp.SupportedOutputs & 0x1) == 0x1 )  strcat( MsgAnal, "RGB ");
        if ( (Cp.SupportedOutputs & 0x2) == 0x2 )  strcat( MsgAnal, "YUV ");
        if ( (Cp.SupportedOutputs & 0x4) == 0x4 )  strcat( MsgAnal, "YC ");
        if ( (Cp.SupportedOutputs & 0x8) == 0x8 )  strcat( MsgAnal, "CVBS ");
        if ( (Cp.SupportedOutputs & 0x10)== 0x10)  strcat( MsgAnal, "SVM ");
        if ( (Cp.SupportedOutputs & 0x20)== 0x20)  strcat( MsgDigi, "YCbCr444_24 ");
        if ( (Cp.SupportedOutputs & 0x40)== 0x40)  strcat( MsgDigi, "YCbCr422_16 ");
        if ( (Cp.SupportedOutputs & 0x80)== 0x80)  strcat( MsgDigi, "YCbCr422_8 ");
        if ( (Cp.SupportedOutputs & 0x100)==0x100) strcat( MsgDigi, "RGB888_24 ");
        if ( (Cp.SupportedOutputs & 0x200)==0x200) strcat( MsgDigi, "HD_RGB ");
        if ( (Cp.SupportedOutputs & 0x400)==0x400) strcat( MsgDigi, "HD_YUV ");
        if ( (Cp.SupportedOutputs & 0x800)==0x800) strcat( MsgDigi, "RGB888 ");
        if ( (Cp.SupportedOutputs & 0x1000)==0x1000) strcat( MsgDigi, "YCbCr444 ");
        if ( (Cp.SupportedOutputs & 0x2000)==0x2000) strcat( MsgDigi, "YCbCr422 ");
        strcat( MsgAnal, "\n");
        strcat( MsgDigi, "\n");
        STTBX_Print(( MsgAnal));
        STTBX_Print(( MsgDigi));
    }

    switch ( Cp.SelectedOutput)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :
            strcat( MsgSele, "ANALOG_RGB \n");
            break;
        case STVOUT_OUTPUT_ANALOG_YUV :
            strcat( MsgSele, "ANALOG_YUV \n");
            break;
        case STVOUT_OUTPUT_ANALOG_YC :
            strcat( MsgSele, "ANALOG_YC \n");
            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :
            strcat( MsgSele, "ANALOG_CVBS \n");
            break;
        case STVOUT_OUTPUT_ANALOG_SVM :
            strcat( MsgSele, "ANALOG_SVM \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
            strcat( MsgSele, "DIGITAL_YCBCR444_24BITSCOMPONENTS \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
            strcat( MsgSele, "DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
            strcat( MsgSele, "DIGITAL_YCBCR422_8BITSMULTIPLEXED \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
            strcat( MsgSele, "DIGITAL_RGB888_24BITSCOMPONENTS \n");
            break;
        case STVOUT_OUTPUT_HD_ANALOG_RGB :
            strcat( MsgSele, "HD_ANALOG_RGB \n");
            break;
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            strcat( MsgSele, "HD_ANALOG_YUV \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
            strcat( MsgSele, "DIGITAL_HDMI_RGB888 \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
            strcat( MsgSele, "DIGITAL_HDMI_YCBCR444 \n");
            break;
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
            strcat( MsgSele, "DIGITAL_HDMI_YCBCR422 \n");
            break;
        default :
            strcat( MsgSele, "0 \n");
            break;
    }
    STTBX_Print(( MsgSele));

    STTBX_Print((MsgAOAC));
    if ( Cp.AnalogOutputsAdjustableCapable )
    {
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x1) == 0x1  )
            strcat( MsgSupp, "Y ");
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x2) == 0x2  )
            strcat( MsgSupp, "C ");
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x4) == 0x4  )
            strcat( MsgSupp, "YCRatio ");
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x8) == 0x8  )
            strcat( MsgSupp, "CVBS ");
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x10)== 0x10 )
            strcat( MsgSupp, "RGB ");
        if ( (Cp.SupportedAnalogOutputsAdjustable & 0x20)== 0x20 )
            strcat( MsgSupp, "YUV ");
        strcat( MsgSupp, "\n");
        STTBX_Print(( MsgSupp));
        STTBX_Print(( ">>\tRGB     min %i max %i step %i\n", Cp.RGB.Min, Cp.RGB.Max, Cp.RGB.Step));
        STTBX_Print(( ">>\tYC      min %i max %i step %i\n", Cp.YC.Min, Cp.YC.Max, Cp.YC.Step));
        STTBX_Print(( ">>\tYCRatio min %i max %i step %i\n", Cp.YCRatio.Min, Cp.YCRatio.Max, Cp.YCRatio.Step));
        STTBX_Print(( ">>\tCVBS    min %i max %i step %i\n", Cp.CVBS.Min, Cp.CVBS.Max, Cp.CVBS.Step));
        STTBX_Print(( ">>\tYUV     min %i max %i step %i\n", Cp.YUV.Min, Cp.YUV.Max, Cp.YUV.Step));
    }
    STTBX_Print(( MsgAPCC));
    if ( Cp.AnalogPictureControlCapable )
    {
        AnalogPictureControlCapable = 1;
        STTBX_Print(( ">>\tBrightness min %i max %i step %i\n", Cp.Brightness.Min, Cp.Brightness.Max, Cp.Brightness.Step));
        STTBX_Print(( ">>\tContrast   min %i max %i step %i\n", Cp.Contrast.Min, Cp.Contrast.Max, Cp.Contrast.Step));
        STTBX_Print(( ">>\tSaturation min %i max %i step %i\n", Cp.Saturation.Min, Cp.Saturation.Max, Cp.Saturation.Step));
    }
    STTBX_Print(( MsgALFC));
    STTBX_Print(( MsgESCa));
}

/*#######################################################################*/
/*############################# VOUT COMMANDS ###########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VOUT_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
 ST_Revision_t RevisionNumber;
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    RevisionNumber = STVOUT_GetRevision( );
    sprintf( VOUT_Msg, "STVOUT_GetRevision() : revision=%s\n", RevisionNumber );
    STTBX_Print(( VOUT_Msg ));
    return ( FALSE );
} /* end VOUT_GetRevision */



/*-------------------------------------------------------------------------
 * Function : VOUT_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    STVOUT_Capability_t Capability;
    char DeviceName[80];

    UNUSED_PARAMETER(result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_GetCapability (%s) :", DeviceName );
    ErrorCode = STVOUT_GetCapability(DeviceName, &Capability);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        Print_Vout_Capabilitity( Capability);
        STTST_AssignString( "SELECTEDOUTPUT", (char*)(VoutTypeName[shift(Capability.SelectedOutput)].Name), FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end STVOUT_GetCapability */



/*-------------------------------------------------------------------------
 * Function : VOUT_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * Testtool usage :
 * vout_init "devicename" devicetype=DENC            outputtype=RGB/YC/YUV/CVBS  "dencname"   maxopen
 * vout_init "devicename" devicetype=7015/20         outputtype=RGB/YC/YUV/CVBS  "dencname"   maxopen
 * vout_init "devicename" devicetype=7015/20         outputtype=HDxxx/SVM        Baseaddress  maxopen
 * vout_init "devicename" devicetype=GX1             outputtype=RGB/YC/YUV/CVBS  "dencname"   Baseaddress maxopen
 * vout_init "devicename" devicetype=DIGITAL_OUTPUT  outputtype=YCbCr422_8       Baseaddress  maxopen
 * vout_init "VOUT" typeDENC typeCVBS "DENC" 3 (default values)
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_InitParams_t VOUTInitParams;

#if defined (STVOUT_DACS)
    STVOUT_Handle_t      VoutHandle;
    STVOUT_OpenParams_t  VoutOpenParams;
#endif
    ST_ErrorCode_t      ErrorCode1= ST_NO_ERROR ;
    char DeviceName[STRING_DEVICE_LENGTH], TrvStr[80], IsStr[80],*ptr;
    U32 LVar, Var=0,i;
    #if defined (ST_7710) || defined(ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
      U32 LVar1 ;
    #endif
    U32 Source = 0;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get device type  */
    RetErr = STTST_GetInteger( pars_p, (VOUT_DEVICE_TYPE + 1), (S32*)&LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 1=DENC 2=7015 3=7020 4=GX1 5=5528 6=4629 7=DigOut 8=DencEnhanced 9=5100 10=7710 11=7100 12=5525 13=7200");
        return(TRUE);
    }

    if ( (LVar == 0) || (LVar > 13) )  /* values to test outside device type */
    {
        if ( LVar == 0)
        {
            VOUTInitParams.DeviceType = STVOUT_DEVICE_TYPE_DENC-1;
        }
        if ( LVar > 13)
        {
            VOUTInitParams.DeviceType = STVOUT_DEVICE_TYPE_7200 +1;
        }
    }
    else /* Device Type ok */
    {
        VOUTInitParams.DeviceType = (STVOUT_DeviceType_t)(LVar-1);
    }


    if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)
    {
        /* Get output type */
        STTST_GetItem(pars_p, "YCbCr422_8", TrvStr, sizeof(TrvStr));
    }
    else
    {
        /* Get output type (default: ANALOG CVBS) */
        STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    }
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    /* values to test outside output type */
    if ( (strcmp(TrvStr, "<")==0) || (strcmp(TrvStr, ">")==0) )
    {
        if ( (strcmp(TrvStr, "<")==0) )
        {
            Var = STVOUT_OUTPUT_ANALOG_RGB-1;
        }
        if ( (strcmp(TrvStr, ">")==0) )
        {
            Var = STVOUT_OUTPUT_HD_ANALOG_YUV+1;
        }
        VOUTInitParams.OutputType = (STVOUT_OutputType_t)(Var);
    }
    else
    {
        for (Var=1; Var<VOUT_MAX_SUPPORTED+1; Var++)
        {
            if (   (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
                || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
            {
                break;
            }
        }
        if (Var==VOUT_MAX_SUPPORTED+1)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 13);
            if (TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
                return(TRUE);
            }

            for (i=1; i<VOUT_MAX_SUPPORTED+1; i++)
            {
                if (VoutTypeName[i].Val == Var)
                {
                    Var=i;
                    break;
                }
            }
        }
        VOUTInitParams.OutputType = (STVOUT_OutputType_t)(1<<(Var-1));
    }

    /* Get denc name and/or base Address Add */
    switch (VOUTInitParams.DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DencName, sizeof(ST_DeviceName_t));
            break;
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.EnhancedCell.DencName, sizeof(ST_DeviceName_t));
            break;
        case STVOUT_DEVICE_TYPE_7015 :
            if (VOUTInitParams.OutputType > STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
                if (RetErr)
                {
                    STTST_TagCurrentLine( pars_p, "Expected Base Address");
                    return(TRUE);
                }
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void*)LVar;
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            }
            else
            {
                RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DencName, sizeof(ST_DeviceName_t));
            }
            break;
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DualTriDacCell.DencName, sizeof(ST_DeviceName_t) );
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected DencDeviceName");
                return(TRUE);
            }
             RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
             if (RetErr)
             {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
             }
            VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)LVar;
            break;
        case STVOUT_DEVICE_TYPE_V13 : /* no break */
        case STVOUT_DEVICE_TYPE_5528 :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DualTriDacCell.DencName, sizeof(ST_DeviceName_t));
#if defined (ST_5528)&& defined(ST_7020)
            RetErr = STTST_GetInteger( pars_p, (S32)ST5528_VOUT_BASE_ADDRESS, (S32*)&LVar);
#else
            RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
#endif
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)LVar;
#if defined (ST_5528)&& defined(ST_7020)
            VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_5528_DEVICE_BASE_ADDRESS;
#else
            VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
#endif
            break;

        case STVOUT_DEVICE_TYPE_7020 : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.GenericCell.DencName, sizeof(ST_DeviceName_t));
            RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            VOUTInitParams.Target.GenericCell.BaseAddress_p = (void*)LVar;
            VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            break;
        case STVOUT_DEVICE_TYPE_7710 : /* no break */
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
 #if defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
             switch (VOUTInitParams.OutputType)
             {
                case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                     #if defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)
                     RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DualTriDacCell.DencName, sizeof(ST_DeviceName_t));
                     RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
                     if (RetErr)
                     {
                         STTST_TagCurrentLine( pars_p, "Expected Base Address");
                         return(TRUE);
                     }
                     VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)LVar;
                     VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
                     #else
                     RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.EnhancedDacCell.DencName, sizeof(ST_DeviceName_t));
                     RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, VOUTInitParams.Target.EnhancedDacCell.VTGName, sizeof(ST_DeviceName_t));
                     RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
                     if (RetErr)
                     {
                         STTST_TagCurrentLine( pars_p, "Expected Base Address");
                         return(TRUE);
                     }
                     VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p = (void*)LVar;
                     VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
                     #endif
                     break;
                 case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                 case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                 case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                     RetErr = STTST_GetInteger( pars_p, (S32)HDMI_BASE_ADDRESS, (S32*)&LVar);

                     if (RetErr)
                     {
                         STTST_TagCurrentLine( pars_p, "Expected HDMI Base Address");
                         return(TRUE);
                     }
#ifdef STVOUT_CEC_PIO_COMPARE
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p = (void*)LVar;
#else
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p = (void*)LVar;
#endif
                     RetErr = STTST_GetInteger( pars_p, (S32)HDCP_BASE_ADDRESS, (S32*)&LVar);

                     if (RetErr)
                     {
                         STTST_TagCurrentLine( pars_p, "Expected HDCP Base Address");
                         return(TRUE);
                     }
#ifdef STVOUT_CEC_PIO_COMPARE
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.SecondBaseAddress_p = (void*)LVar;
#else
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void*)LVar;
#endif
                     break;
                default :
                    break;
             }
#endif /*ST_7710*/
             break;
        case STVOUT_DEVICE_TYPE_4629 : /* no break */
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DualTriDacCell.DencName, sizeof(ST_DeviceName_t));
            RetErr = STTST_GetInteger( pars_p, (S32)0, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            if (LVar != 0)
            {
                STTST_TagCurrentLine( pars_p, "Base Adress must be equal to 0");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*)STI4629_DNC_BASE_ADDRESS;
            VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)ST4629_BASE_ADDRESS ;
#endif /* mb376 || espresso*/
            break;
/*#if defined(DVO_TESTS)*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
#if !(defined(ST_5105)|| defined(ST_5188)|| defined(ST_5525))
#if defined(ST_5528) || defined(ST_5100) ||defined (ST_5301) || defined(ST_5107) || defined(ST_5162)
#if defined(ST_5528) && defined(ST_7020)
            RetErr = STTST_GetInteger( pars_p, (S32)DIGOUT_5528_BASE_ADDRESS, (S32*)&LVar);
#else
            RetErr = STTST_GetInteger( pars_p, (S32)DIGOUT_BASE_ADDRESS, (S32*)&LVar);
#endif
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            VOUTInitParams.Target.DVOCell.BaseAddress_p = (void*)LVar;
            VOUTInitParams.Target.DVOCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            break;
#else
            RetErr = STTST_GetInteger( pars_p, (S32)DIGOUT_BASE_ADDRESS, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void*)LVar;
            VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            break;
#endif
#endif
/*#endif  STVOUT_DVO DVO_TESTS*/
        default :
            break;
    }

    /* get max open (default: 3 = STVOUT_MAX_OPEN) */
    RetErr = STTST_GetInteger( pars_p, 3, (S32*)&LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected max open init parameter\n" );
        return(TRUE);
    }
    VOUTInitParams.CPUPartition_p  = DriverPartition_p;
    VOUTInitParams.MaxOpen         = (U32)LVar;


    /* Get Used DACs for output type */
   switch (VOUTInitParams.DeviceType)
   {
      case STVOUT_DEVICE_TYPE_DENC : /* no break */
      case STVOUT_DEVICE_TYPE_7015:  /* no break */
      case STVOUT_DEVICE_TYPE_7020 : /* no break */
      case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
      case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT  :
           break;
      case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
            if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_3), (S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC )
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.EnhancedCell.DacSelect=(STVOUT_DAC_t)LVar;
            break;
      case STVOUT_DEVICE_TYPE_5528:

            if(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_6), (S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_4|STVOUT_DAC_5),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;
            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
             break;
      case STVOUT_DEVICE_TYPE_V13 :
           #if defined (ST_5100) ||defined (ST_5301)
            if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_3), (S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV)
            {
                 RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC )
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }

            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;
            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
          #else
            if(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_6), (S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;
            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
          #endif
             break;
      case STVOUT_DEVICE_TYPE_7710:
            if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_6), (S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC )
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_4|STVOUT_DAC_5),(S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;

            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
             break;
      case STVOUT_DEVICE_TYPE_7100:
            if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_6), (S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC )
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;

            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
             break;
      case STVOUT_DEVICE_TYPE_7200:
            if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_3), (S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC )
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.EnhancedDacCell.DacSelect=(STVOUT_DAC_t)LVar;
            /*SD from Main (mixer 1 is not supported for 7200C1*/
            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)1, (S32*)&Source);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
                return(TRUE);
            }
            break;

      case STVOUT_DEVICE_TYPE_4629:
         #if (defined (mb376) && !defined(ST_7020)) || defined (espresso)
           if(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
           {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_3), (S32*)&LVar);
           }
           else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV))
           {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
           }
           else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC)
           {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);

           }
           if(RetErr)
           {
               STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
               return(TRUE);
           }
           VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;
           RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
           if (RetErr)
           {
               STTST_TagCurrentLine( pars_p, "Expected value for main/aux selection main=0 Aux=1");
               return(TRUE);
           }
        #endif /* mb376 || espresso*/
           break;
      case STVOUT_DEVICE_TYPE_5525:
            if(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_3), (S32*)&LVar);
            }
            else if ((VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_RGB)||(VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YUV))
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3),(S32*)&LVar);
            }
            else if (VOUTInitParams.OutputType== STVOUT_OUTPUT_ANALOG_YC)
            {
                RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_DAC_1|STVOUT_DAC_2),(S32*)&LVar);
            }
            if(RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Dac Selected");
                return(TRUE);
            }
            VOUTInitParams.Target.DualTriDacCell.DacSelect=(STVOUT_DAC_t)LVar;
            RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
            if (RetErr)
            {
               STTST_TagCurrentLine( pars_p, "Expected value for main/aux selection main=0 Aux=1");
               return(TRUE);
            }
            break;
      default :
            break;
   }
 #if defined(ST_7710)||defined(ST_7100)|| defined (ST_7109)|| defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
       switch (VOUTInitParams.DeviceType)
       {
          case STVOUT_DEVICE_TYPE_DENC : /* no break */
          case STVOUT_DEVICE_TYPE_7015 : /* no break */
          case STVOUT_DEVICE_TYPE_7020 : /* no break */
          case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
          case STVOUT_DEVICE_TYPE_5528 : /* no break */
          case STVOUT_DEVICE_TYPE_4629 : /* no break */
          case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
          case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
          case STVOUT_DEVICE_TYPE_V13 :
          case STVOUT_DEVICE_TYPE_5525 :
               break;
          case STVOUT_DEVICE_TYPE_7710 : /* no break */
          case STVOUT_DEVICE_TYPE_7100 :
          case STVOUT_DEVICE_TYPE_7200 :
               switch (VOUTInitParams.OutputType)
               {
                case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /*no break*/
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                     break;
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break*/
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                     VOUTInitParams.Target.HDMICell.InterruptNumber =  VOUT_INTERRUPT_NUMBER;
                     #ifndef ST_7710
                     VOUTInitParams.Target.HDMICell.InterruptLevel  =  1;
                     #else
                       #ifdef WA_GNBvd44290
                       VOUTInitParams.Target.HDMICell.InterruptLevel  =  14;
                       #else
                       VOUTInitParams.Target.HDMICell.InterruptLevel  =  1;
                       #endif
                     #endif
                     /* Get Evt Name */
                     RetErr = STTST_GetString( pars_p, STEVT_DEVICE_NAME, VOUTInitParams.Target.HDMICell.EventsHandlerName, sizeof(ST_DeviceName_t));
                     if (RetErr)
                     {
                         STTST_TagCurrentLine( pars_p, "Expected  EVT DeviceName");
                         return(TRUE);
                     }
                     /* Get VTG Name */
                     RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, VOUTInitParams.Target.HDMICell.VTGName, sizeof(ST_DeviceName_t) );
                     strcpy(VOUTInitParams.Target.HDMICell.VTGName, "VTG");
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected VTG DeviceName");
                        return(TRUE);
                     }
                     /* Get HSync and VSync polarity position*/
                     RetErr = STTST_GetInteger( pars_p, 0,(S32*)&LVar );

                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected HSync positive polarity (TRUE, FALSE)" );
                        return(TRUE);
                     }
                     VOUTInitParams.Target.HDMICell.HSyncActivePolarity = (BOOL)LVar;

                     RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );

                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected VSync positive polarity (TRUE, FALSE)" );
                        return(TRUE);
                     }
                     VOUTInitParams.Target.HDMICell.VSyncActivePolarity = (BOOL)LVar;

                     /* Get I2C Name */
#ifdef STVOUT_CEC_PIO_COMPARE
                     RetErr = STTST_GetString( pars_p, STI2C_BACK_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.I2CDevice, sizeof(ST_DeviceName_t) );
#else
                     RetErr = STTST_GetString( pars_p, STI2C_BACK_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.I2CDevice, sizeof(ST_DeviceName_t) );
#endif
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected I2C DeviceName");
                        return(TRUE);
                     }
                     /* Get PIO Name */
#ifdef STVOUT_CEC_PIO_COMPARE
                     RetErr = STTST_GetString( pars_p, STPIO_2_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.PIODevice , sizeof(ST_DeviceName_t) );
#else
#if !(defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105))
                     RetErr = STTST_GetString( pars_p, STPIO_2_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice , sizeof(ST_DeviceName_t) );
#else
                     RetErr = STTST_GetString( pars_p, STPIO_5_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.PIODevice , sizeof(ST_DeviceName_t) );
#endif
#endif
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected PIO DeviceName");
                        return(TRUE);
                     }
                     /* Get PIO bit configuration*/
#if !(defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105))
                     RetErr = STTST_GetInteger( pars_p, PIO_BIT_2, (S32*)&LVar );
#else
                     RetErr = STTST_GetInteger( pars_p, PIO_BIT_7, (S32*)&LVar );
#endif
                     if ( RetErr )
                     {
                        STTST_TagCurrentLine( pars_p, "Expected PIO Bit configuration");
                        return(TRUE);
                     }
#ifdef STVOUT_CEC_PIO_COMPARE
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.HPD_Bit = (U32)LVar;
#else
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.HPD_Bit = (U32)LVar;
#endif
                     RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );

                     /* Get Encryption status */

                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected Encryption (TRUE, FALSE)" );
                        return(TRUE);
                     }
                     VOUTInitParams.Target.HDMICell.IsHDCPEnable = (BOOL)LVar;

                     /* Get HPD logic */
                     RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected HPD logic (TRUE, FALSE)" );
                        return(TRUE);
                     }
#ifdef STVOUT_CEC_PIO_COMPARE
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.IsHPDInversed = (BOOL)LVar;
#else
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.IsHPDInversed = (BOOL)LVar;
#endif

#ifdef STVOUT_CEC_PIO_COMPARE
                     /* Get PWM DeviceName */
                     RetErr = STTST_GetString( pars_p, STPWM_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.PWMName , sizeof(ST_DeviceName_t) );
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected PWM DeviceName" );
                        return(TRUE);
                     }

                     /* Get CEC PIO DeviceName */
                     RetErr = STTST_GetString( pars_p, STPIO_5_DEVICE_NAME, VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.CECPIOName , sizeof(ST_DeviceName_t) );
                     if (RetErr)
                     {
                        STTST_TagCurrentLine( pars_p, "Expected CEC PIO DeviceName");
                        return(TRUE);
                     }

                     /* Get CEC PIO bit configuration*/
                     RetErr = STTST_GetInteger( pars_p, PIO_BIT_7, (S32*)&LVar );
                     if ( RetErr )
                     {
                        STTST_TagCurrentLine( pars_p, "Expected CEC PIO Bit configuration");
                        return(TRUE);
                     }
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.CECPIO_BitNumber = (U32)LVar;
#endif
                 default :
                    break;

               }
              break;
         default :
              break;
       }
#endif

#if defined(ST_7710)||defined(ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)

   switch (VOUTInitParams.OutputType)
   {
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
            break;
        default:
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
            RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar1);    /* by default HD_Dacs upsampler is used for 7200 */
#else
            RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar1);
#endif
            if(RetErr)
            {
                  STTST_TagCurrentLine( pars_p, "Expected HD_Dacs Selection (0=HD_Dacs upsumpler bypassed , 1=HD_Dacs upsumpler used ");
                  return(TRUE);
            }
         #if defined (ST_7710)||defined(ST_7100)|| defined (ST_7109)
            VOUTInitParams.Target.DualTriDacCell.HD_Dacs = (BOOL)LVar1;
            if(VOUTInitParams.Target.DualTriDacCell.HD_Dacs ==1)
              {
                   switch (VOUTInitParams.OutputType)
                   {
                       case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                       case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                           RetErr = STTST_GetInteger( pars_p, (STVOUT_HD_MODE +1), (S32*)&LVar1);
                           break;
                       default:
                            RetErr = STTST_GetInteger( pars_p, (STVOUT_SD_MODE +1), (S32*)&LVar1);
                           break;
                   }
                   if(RetErr)
                   {
                STTST_TagCurrentLine(pars_p,"Expected Format (1=UPS_SD, 2=UPS_ED, 3=UPS_HD, 4=UPS_HDRQ ");
                           return(TRUE);
                   }
                   if ( (LVar1 == 0) || (LVar1 > 4) )  /* values to test outside Format type */
                   {
                       if ( LVar1 == 0)
                       {
                           VOUTInitParams.Target.DualTriDacCell.Format = STVOUT_SD_MODE-1;
                       }
                       if ( LVar1 > 4)
                       {
                           VOUTInitParams.Target.DualTriDacCell.Format = STVOUT_HDRQ_MODE+1;
                       }
                   }
                   else /* Format Type ok */
                   {
                       VOUTInitParams.Target.DualTriDacCell.Format = (STVOUT_Format_t)(LVar1-1);
                   }
           }
         #else
           VOUTInitParams.Target.EnhancedDacCell.HD_Dacs = (BOOL)LVar1;
            if(VOUTInitParams.Target.EnhancedDacCell.HD_Dacs ==1)
              {
                   switch (VOUTInitParams.OutputType)
                   {
                       case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                       case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                           RetErr = STTST_GetInteger( pars_p, (STVOUT_HD_MODE +1), (S32*)&LVar1);
                           break;
                       default:
                            RetErr = STTST_GetInteger( pars_p, (STVOUT_SD_MODE +1), (S32*)&LVar1);
                           break;
                   }
                   if(RetErr)
                   {
                STTST_TagCurrentLine(pars_p,"Expected Format (1=UPS_SD, 2=UPS_ED, 3=UPS_HD, 4=UPS_HDRQ ");
                           return(TRUE);
                   }
                   if ( (LVar1 == 0) || (LVar1 > 4) )  /* values to test outside Format type */
                   {
                       if ( LVar1 == 0)
                       {
                           VOUTInitParams.Target.EnhancedDacCell.Format = STVOUT_SD_MODE-1;
                       }
                       if ( LVar1 > 4)
                       {
                           VOUTInitParams.Target.EnhancedDacCell.Format = STVOUT_HDRQ_MODE+1;
                       }
                   }
                   else /* Format Type ok */
                   {
                       VOUTInitParams.Target.EnhancedDacCell.Format = (STVOUT_Format_t)(LVar1-1);
                   }
           }
           #endif
           break;
     }
  #endif

        /* init */
        sprintf( VOUT_Msg, "STVOUT_Init(%s) : ", DeviceName );
        ErrorCode1 = STVOUT_Init(DeviceName, &VOUTInitParams);
        RetErr = VOUT_TextError( ErrorCode1, VOUT_Msg);

#if defined (STVOUT_DACS)
        switch (VOUTInitParams.OutputType)
        {
            case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
            case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
            case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
            case STVOUT_OUTPUT_ANALOG_CVBS :
                if (ErrorCode1 == ST_NO_ERROR) ErrorCode1 = STVOUT_Open(DeviceName, &VoutOpenParams, &VoutHandle);
                if (ErrorCode1 == ST_NO_ERROR) ErrorCode1 = STVOUT_SetInputSource(VoutHandle,(STVOUT_Source_t)Source);
                if (ErrorCode1 == ST_NO_ERROR) ErrorCode1 = STVOUT_Close(VoutHandle);
                break;
            default :
                break;
        }
#endif

    switch (VOUTInitParams.DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\"\n", VOUT_TYPE,
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.DencName);
            break;
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\"\n", VOUT_TYPE,
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.EnhancedCell.DencName);
            break;
        case STVOUT_DEVICE_TYPE_7015 :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x\n", VOUT_TYPE,
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.DencName,
                    (U32) VOUTInitParams.Target.HdsvmCell.BaseAddress_p);
            break;
        case STVOUT_DEVICE_TYPE_4629 :
        sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x\n", "Type_STi4269",
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.DualTriDacCell.DencName ,
                    (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p );
        break;
        case STVOUT_DEVICE_TYPE_7020 :
        case STVOUT_DEVICE_TYPE_GX1 :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x\n", VOUT_TYPE,
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.GenericCell.DencName ,
                    (U32) VOUTInitParams.Target.GenericCell.BaseAddress_p );
            break;
        case STVOUT_DEVICE_TYPE_V13 :
        case STVOUT_DEVICE_TYPE_5528 :
             sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x\n",
                    VOUTInitParams.DeviceType==STVOUT_DEVICE_TYPE_V13?"TYPE_5100":"TYPE_5528",
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.DualTriDacCell.DencName ,
                    (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p );
            break;
        case STVOUT_DEVICE_TYPE_5525 :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x\n", VOUT_TYPE,
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    VOUTInitParams.Target.DualTriDacCell.DencName ,
                    (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p );
            break;
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s, Add=0x%x\n", "Digital Output",
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                    (U32) VOUTInitParams.Target.DVOCell.BaseAddress_p);
            break;

        case STVOUT_DEVICE_TYPE_7710 :
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
            switch (VOUTInitParams.OutputType)
            {
                case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
#if defined(ST_7710)||defined(ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#if defined (ST_7710)||defined(ST_7100)|| defined (ST_7109)
                     if(VOUTInitParams.Target.DualTriDacCell.HD_Dacs ==1)
                     {
                        sprintf( VOUT_Msg,"\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x HD_DACS=%d Format=%s\n", VOUT_TYPE,
                            (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                             VOUTInitParams.Target.DualTriDacCell.DencName ,
                            (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p,
                            (U32)VOUTInitParams.Target.DualTriDacCell.HD_Dacs,
                            (LVar1 < VOUT_MAX_FORMAT_SUPPORTED+1) ? VoutFormatName[LVar1].Name : "????");                                                }
                     else
                     {
                        sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x HD_DACS=%d\n", VOUT_TYPE,
                            (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                             VOUTInitParams.Target.DualTriDacCell.DencName ,
                            (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p,
                            (U32)VOUTInitParams.Target.DualTriDacCell.HD_Dacs);
                     }
#else
                     if(VOUTInitParams.Target.EnhancedDacCell.HD_Dacs ==1)
                     {
                        sprintf( VOUT_Msg,"\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x HD_DACS=%d Format=%s\n", VOUT_TYPE,
                            (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                             VOUTInitParams.Target.EnhancedDacCell.DencName ,
                            (U32) VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p,
                            (U32)VOUTInitParams.Target.EnhancedDacCell.HD_Dacs,
                            (LVar1 < VOUT_MAX_FORMAT_SUPPORTED+1) ? VoutFormatName[LVar1].Name : "????");                                                }
                     else
                     {
                        sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x HD_DACS=%d\n", VOUT_TYPE,
                            (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                             VOUTInitParams.Target.EnhancedDacCell.DencName ,
                            (U32) VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p,
                            (U32)VOUTInitParams.Target.EnhancedDacCell.HD_Dacs);
                     }

#endif
#endif
                   break;
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                   sprintf( VOUT_Msg, "\tType=%s, Out=%s, Denc=\"%s\" Add=0x%x \n", VOUT_TYPE,
                       (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
                        VOUTInitParams.Target.DualTriDacCell.DencName ,
                       (U32) VOUTInitParams.Target.DualTriDacCell.BaseAddress_p);
                   break;
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                     sprintf( VOUT_Msg, "\tType=%s, Out=%s, Add_Hdmi=0x%x, Add_Hdcp =0x%x\n", VOUT_TYPE,
                            (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????",
#ifdef STVOUT_CEC_PIO_COMPARE
                            (U32) VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p,
                            (U32) VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCellOne.SecondBaseAddress_p);
#else
                            (U32) VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p,
                            (U32) VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p);
#endif
                    break;
                default :
                    break;
            }
            break;
        default :
            sprintf( VOUT_Msg, "\tType=%s, Out=%s ...\n", "????",
                    (Var < VOUT_MAX_SUPPORTED+1) ? VoutTypeName[Var].Name : "????");
            break;
    }
    VOUT_PRINT (VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_Init */

/*-------------------------------------------------------------------------
 * Function : VOUT_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OpenParams_t VOUTOpenParams;
    ST_ErrorCode_t      ErrorCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    /* Open a VOUT device connection */

    /* get device */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Open(%s): ", DeviceName);
    ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( VOUT_Msg, "\tHandle=%d\n", VOUTHndl);
        VOUT_PRINT(VOUT_Msg);
        STTST_AssignInteger( result_sym_p, VOUTHndl, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_Open */



/*-------------------------------------------------------------------------
 * Function : VOUT_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrorCode;

    UNUSED_PARAMETER(result_sym_p);

  /* Close a VOUT device connection */
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Close(%d): ", VOUTHndl);
    ErrorCode = STVOUT_Close(VOUTHndl);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_Close */



/*-------------------------------------------------------------------------
 * Function : VOUT_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_TermParams_t VOUTTermParams;
    ST_ErrorCode_t      ErrorCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    U32  LVar;

    UNUSED_PARAMETER(result_sym_p);

    /* get device */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
    STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }
    VOUTTermParams.ForceTerminate = (BOOL)LVar;

    sprintf( VOUT_Msg, "STVOUT_Term(%s): ", DeviceName);
    ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_Term */



/*-------------------------------------------------------------------------
 * Function : VOUT_Enable
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Enable( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Enable(%d): ", VOUTHndl);
    ErrorCode = STVOUT_Enable( VOUTHndl);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}  /* end VOUT_Enable */



/*-------------------------------------------------------------------------
 * Function : VOUT_Disable
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Disable( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Disable(%d): ", VOUTHndl);
    ErrorCode = STVOUT_Disable( VOUTHndl);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}  /* end VOUT_Disable */


/*-------------------------------------------------------------------------
 * Function : VOUT_GetParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
  STVOUT_OutputParams_t OutParam;
  ST_ErrorCode_t ErrorCode;
  int LVar;
  int OutTyp = 0;
  BOOL Prnt = 0;
  BOOL RetErr;
  char TrvStr[80], IsStr[80], *ptr;
  U32 Var,i;

        UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG CVBS) */
    STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for (Var=1; Var<VOUT_MAX_SUPPORTED+1; Var++)
    {
        if (   (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
            || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
            break;
    }
    if (Var==VOUT_MAX_SUPPORTED+1)
    {
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
            return(TRUE);
        }

        for (i=1; i<VOUT_MAX_SUPPORTED+1; i++)
        {
            if (VoutTypeName[i].Val == Var)
            {
                Var=i;
                break;
            }
        }
    }

    OutTyp = (int)Var;
    /* get print option */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1))
    {
        STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
        RetErr = TRUE;
    }
    else
    {
        Prnt = (BOOL)LVar;
    }

    if ( !RetErr)
    {
        sprintf( VOUT_Msg, "STVOUT_GetOutputParams (%d) :", VOUTHndl );
        ErrorCode = STVOUT_GetOutputParams(VOUTHndl, &OutParam);
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
        if ( ErrorCode == ST_NO_ERROR)
        {
            Get_Vout_Params( OutParam, OutTyp, Prnt);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetParams */


/*******************************************************************************
Name        : VOUT_GetOption
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL VOUT_GetOption( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_OptionParams_t OptionParams;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p,' ', &LVar );
    if ( RetErr)
    {
        sprintf( VOUT_Msg, "Expected Option: Aux not main=%d,\n Notch filter on luma=%d,\n RGB Sat=%d,\n If delay=%d",
                 STVOUT_OPTION_GENERAL_AUX_NOT_MAIN,STVOUT_OPTION_NOTCH_FILTER_ON_LUMA, STVOUT_OPTION_RGB_SATURATION,
                 STVOUT_OPTION_IF_DELAY);
        STTST_TagCurrentLine( pars_p, VOUT_Msg );
        return(TRUE);
    }
    OptionParams.Option = LVar;

    ErrorCode = STVOUT_GetOption(VOUTHndl, &OptionParams);
    sprintf( VOUT_Msg, "STVOUT_GetOption (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    switch (OptionParams.Option)
    {
        case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:
            STTBX_Print(( ">>\tOption: Aux not main: %s\n", OptionParams.Enable ? "Enable" : "Disabled"));
            break;
        case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:
            STTBX_Print(( ">>\tOption: Notch filter on luma: %s\n", OptionParams.Enable ? "Enable" : "Disabled"));
            break;
        case STVOUT_OPTION_RGB_SATURATION:
            STTBX_Print(( ">>\tOption: RGB saturation: %s\n", OptionParams.Enable ? "Enable" : "Disabled"));
            break;
        case STVOUT_OPTION_IF_DELAY:
            STTBX_Print(( ">>\tOption: IF delay: %s\n", OptionParams.Enable ? "Enable" : "Disabled"));
            break;
        default :
            STTST_TagCurrentLine( pars_p, VOUT_Msg );
            return(TRUE);
            break;
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetOption */


/*******************************************************************************
Name        : VOUT_SetOption
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static BOOL VOUT_SetOption( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_OptionParams_t OptionParams;
    S32  LVar;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p,' ', &LVar );
    if ( RetErr)
    {
        sprintf( VOUT_Msg, "Expected Option: Aux not main=%d,\n Notch filter on luma=%d,\n RGB Sat=%d,\n If delay=%d",
                 STVOUT_OPTION_GENERAL_AUX_NOT_MAIN,STVOUT_OPTION_NOTCH_FILTER_ON_LUMA,STVOUT_OPTION_RGB_SATURATION,
                 STVOUT_OPTION_IF_DELAY);
        STTST_TagCurrentLine( pars_p, VOUT_Msg );
        return(TRUE);
    }
    OptionParams.Option = LVar;

    switch (OptionParams.Option)
    {
        case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:
        case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:
        case STVOUT_OPTION_RGB_SATURATION:
        case STVOUT_OPTION_IF_DELAY:
            RetErr = STTST_GetInteger( pars_p, FALSE, &LVar);
            if (RetErr || (LVar<0) || (LVar>1))
            {
                STTST_TagCurrentLine( pars_p, "Expected boolean value (Enable)");
                return(TRUE);
            }
            OptionParams.Enable = (BOOL)LVar;
            break;
        default :
            STTST_TagCurrentLine( pars_p, VOUT_Msg );
            return(TRUE);
            break;
    }
    ErrorCode = STVOUT_SetOption(VOUTHndl, &OptionParams);
    sprintf( VOUT_Msg, "STVOUT_SetOption (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}  /* end VOUT_SetOption */

/*-------------------------------------------------------------------------
 * Function : VOUT_SetParams
 *        =>  VOUT_SetAnalogDacParams
 *            VOUT_SetAnalogBCSParams
 *            VOUT_SetEmbeddedSyncro
 *            VOUT_SetAnalogInverted
 *            VOUT_SetAnalogSVM
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function : VOUT_SetParams
 *            (set defaults parameters)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    char TrvStr[80], IsStr[80], *ptr;
    U32 Var,i, LVar;
#if defined (ST_5528)|| defined(ST_5100) ||defined (ST_5301)|| defined (ST_5107)|| defined(ST_5162)
    BOOL IsNTSC = TRUE;
#endif

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG CVBS) */
    STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for (Var=1; Var<VOUT_MAX_SUPPORTED+1; Var++)
    {
        if (  (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
           || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
        {
            break;
        }
    }
    if (Var==VOUT_MAX_SUPPORTED+1)
    {
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
            return(TRUE);
        }

        for (i=1; i<VOUT_MAX_SUPPORTED+1; i++)
        {
            if (VoutTypeName[i].Val == Var)
            {
                Var=i;
                break;
            }
        }
    }

    switch (Var)
    {
        case 1 : /* STVOUT_OUTPUT_ANALOG_RGB                               */
        case 2 : /* STVOUT_OUTPUT_ANALOG_YUV                               */
        case 3 : /* STVOUT_OUTPUT_ANALOG_YC                                */
        case 4 : /* STVOUT_OUTPUT_ANALOG_CVBS                              */
            OutParam->Analog.StateAnalogLevel  = STVOUT_PARAMS_DEFAULT;
            OutParam->Analog.StateBCSLevel     = STVOUT_PARAMS_DEFAULT;
            OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
            OutParam->Analog.EmbeddedType      = FALSE;
            OutParam->Analog.InvertedOutput    = FALSE;
            OutParam->Analog.SyncroInChroma    = FALSE;
            OutParam->Analog.ColorSpace        = STVOUT_ITU_R_601;
            break;
        case 5 : /* STVOUT_OUTPUT_ANALOG_SVM                               */
            OutParam->SVM.StateAnalogSVM       = STVOUT_PARAMS_DEFAULT;
            break;
        case 6 : /* STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS        */
        case 7 : /* STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED */
        case 8 : /* STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED        */
        case 9 : /* STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS          */
#if defined(ST_5528)|| defined(ST_5100) ||defined (ST_5301)|| defined (ST_5107)|| defined(ST_5162)
            RetErr = STTST_GetInteger( pars_p, TRUE, (S32*)&IsNTSC);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected boolean (1=NTSC 0=PAL)");
                return(TRUE);
            }
            OutParam->Digital.EmbeddedType              = TRUE;
            OutParam->Digital.EmbeddedSystem            = IsNTSC ? STVOUT_EMBEDDED_SYSTEM_525_60 : STVOUT_EMBEDDED_SYSTEM_625_50;
            if (IsNTSC)
            {
                STTBX_Print(("NTSC mode chosen\n"));
            }
            else
            {
                STTBX_Print(("Pal mode chosen\n"));
            }
#else
            OutParam->Digital.EmbeddedType              = FALSE;
            OutParam->Digital.SyncroInChroma            = TRUE;
            OutParam->Digital.ColorSpace                = STVOUT_SMPTE240M;
#endif
                        break;
        case 10 : /* STVOUT_OUTPUT_HD_ANALOG_RGB                            */
        case 11 : /* STVOUT_OUTPUT_HD_ANALOG_YUV                            */
            OutParam->Analog.EmbeddedType      = FALSE;
            OutParam->Analog.SyncroInChroma    = TRUE;
            OutParam->Analog.ColorSpace        = STVOUT_ITU_R_601;
            break;

        case 12 :  /* STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 */
        case 13 : /* STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 */
        case 14 : /* STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 */
             RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
             if (RetErr)
             {
                 STTST_TagCurrentLine( pars_p, "Expected ForceDVI (1=DVI 0=HDMI)" );
                 return(TRUE);
             }
             OutParam->HDMI.ForceDVI = (BOOL)LVar;
             /* Get Encryption status */

             RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
             if (RetErr)
             {
                 STTST_TagCurrentLine( pars_p, "Expected Encryption (TRUE, FALSE)" );
                 return(TRUE);
             }
             OutParam->HDMI.IsHDCPEnable = (BOOL)LVar;

             RetErr = STTST_GetInteger( pars_p, 48000, (S32*)&LVar );
             if (RetErr)
             {
                 STTST_TagCurrentLine( pars_p, "Expected Audio Sampling Frequency" );
                 return(TRUE);
             }
             OutParam->HDMI.AudioFrequency = (BOOL)LVar;

             break;
        default :
            break;
    }

    ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
    sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_SetParams */



/*-------------------------------------------------------------------------
 * Function : VOUT_SetAnalogDacParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetAnalogDacParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;
    int OutTyp = 0;
    int P1 = 0, P2 = 0, P3 = 0;
    char TrvStr[80], IsStr[80], *ptr;
    U32 Var,i;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG RGB) */
    STTST_GetItem(pars_p, "RGB", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for (Var=1; Var<5; Var++)
    {
        if (   (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
            || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
        {
            break;
        }
    }
    if (Var==5)
    {
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
            return(TRUE);
        }

        for (i=1; i<5; i++)
        {
            if (VoutTypeName[i].Val == Var)
            {
                Var=i;
                break;
            }
        }
    }

    OutTyp = Var;
    /* get 3 dac parameters */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "dac parameter 1" );
        RetErr = TRUE;
    }
    P1 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "dac parameter 2" );
        RetErr = TRUE;
    }
    P2 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "dac parameter 3" );
        RetErr = TRUE;
    }
    P3 = LVar;

    ErrorCode = STVOUT_GetOutputParams(VOUTHndl, OutParam);
    if ( ErrorCode != ST_NO_ERROR )
    {
        RetErr = TRUE;
        STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
    }

    if ( !RetErr )
    {
        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_AFFECTED;
        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;

        switch ( OutTyp)
        {
            case 1 :
                OutParam->Analog.AnalogLevel.RGB.R = P1;
                OutParam->Analog.AnalogLevel.RGB.G = P2;
                OutParam->Analog.AnalogLevel.RGB.B = P3;
                break;

            case 2 :
                OutParam->Analog.AnalogLevel.YUV.Y = P1;
                OutParam->Analog.AnalogLevel.YUV.U = P2;
                OutParam->Analog.AnalogLevel.YUV.V = P3;
                break;

            case 3 :
                OutParam->Analog.AnalogLevel.YC.Y       = P1;
                OutParam->Analog.AnalogLevel.YC.C       = P2;
                OutParam->Analog.AnalogLevel.YC.YCRatio = P3;
                break;

            case 4 :
                OutParam->Analog.AnalogLevel.CVBS.CVBS    = P1;
                OutParam->Analog.AnalogLevel.CVBS.YCRatio = P2;
                break;

            default :
                break;
        }

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetAnalogDacParams*/



/*-------------------------------------------------------------------------
 * Function : VOUT_SetAnalogBCSParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetAnalogBCSParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;
    int P1 = 0, P2 = 0, P3 = 0;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get 3 parameters */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 1" );
        RetErr = TRUE;
    }
    P1 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 2" );
        RetErr = TRUE;
    }
    P2 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 3" );
        RetErr = TRUE;
    }
    P3 = LVar;

    ErrorCode = STVOUT_GetOutputParams(VOUTHndl, OutParam);
    if ( ErrorCode != ST_NO_ERROR )
    {
        RetErr = TRUE;
        STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
    }

    if ( !RetErr)
    {
        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_AFFECTED;
        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;

        OutParam->Analog.BCSLevel.Brightness = P1;
        OutParam->Analog.BCSLevel.Contrast   = P2;
        OutParam->Analog.BCSLevel.Saturation = P3;

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetAnalogBCSParams */



/*-------------------------------------------------------------------------
 * Function : VOUT_SetAnalogChrLumFilterParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetAnalogChrLumFilterParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;
    int Lum = 0, Chr = 0;
    int Inc;
    S32 CoeffLuma[10] = {0x03, 0x01, 0xfff7, 0xfffe, 0x1e, 0x05,0x1a4, 0xfff9, 0x144, 0x206};
    S32 CoeffChroma[9] = {0x0a, 0x06, 0xfff7, 0xffe7, 0xffe9, 0x0d,0x49, 0x86, 0x9a};

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
#if 0
    /* get 3 parameters */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 1" );
        RetErr = TRUE;
    }
    P1 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 2" );
        RetErr = TRUE;
    }
    P2 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "parameter 3" );
        RetErr = TRUE;
    }
    P3 = LVar;
#endif
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "1 = set new coeff Luma" );
        RetErr = TRUE;
    }
    Lum=LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "1 = set new coeff Chroma" );
        RetErr = TRUE;
    }
    Chr=LVar;



    ErrorCode = STVOUT_GetOutputParams(VOUTHndl, OutParam);
    if ( ErrorCode != ST_NO_ERROR )
    {
        RetErr = TRUE;
        STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
    }

    if ( !RetErr)
    {
        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_AFFECTED;
        if(Lum)
        {
            for (Inc = 0; Inc < 10; Inc++)
            {
                OutParam->Analog.ChrLumFilter.Luma[Inc] = CoeffLuma[Inc];
            }
        }
        if(Chr)
        {
            for (Inc = 0; Inc < 9; Inc++)
            {
                OutParam->Analog.ChrLumFilter.Chroma[Inc] = CoeffChroma[Inc];
            }
        }

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetAnalogChrLumFilterParams */

/******************************************************************************** */


/*-------------------------------------------------------------------------
 * Function : VOUT_SetEmbeddedSyncro
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetEmbeddedSyncro( STTST_Parse_t *pars_p, char *result_sym_p )
{
  STVOUT_OutputParams_t OutParam;
  ST_ErrorCode_t ErrorCode;
  BOOL RetErr;
  int LVar;
  int OutTyp = 0;
  BOOL Emb = 0, Syn = 0;
  STVOUT_ColorSpace_t Col = 0;
  char TrvStr[80], IsStr[80], *ptr;
  U32 Var,i;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG CVBS) */
    STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for (Var=1; Var<VOUT_MAX_SUPPORTED+1; Var++)
    {
        if (   (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
            || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
        {
            break;
        }
    }
    if (Var==VOUT_MAX_SUPPORTED+1)
    {
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
            return(TRUE);
        }

        for (i=1; i<VOUT_MAX_SUPPORTED+1; i++)
        {
            if (VoutTypeName[i].Val == Var)
            {
                Var=i;
                break;
            }
        }
    }

    OutTyp = Var;
    /* get Embedded (default: 0 = FALSE) */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "! expected Embedded, 0:FALSE or 1:TRUE\n" );
        RetErr = TRUE;
    }
    else
    {
        Emb = (BOOL)LVar;
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr)
        {
            STTST_TagCurrentLine( pars_p, "! expected SyncroInChroma, 0:FALSE or 1:TRUE\n" );
            RetErr = TRUE;
        }
        else
        {
            Syn = (BOOL)LVar;
            RetErr = STTST_GetInteger( pars_p, 2, &LVar );
            if ( RetErr || (LVar < 0) )
            {
                STTST_TagCurrentLine( pars_p, "! expected ColorSpace type (1=smpte240, 2=itu601, 4=itu709)\n" );
                RetErr = TRUE;
            }
            else
            {
                Col = LVar;
            }
        }
    }

    if ( !RetErr)
    {
        ErrorCode = STVOUT_GetOutputParams(VOUTHndl, &OutParam);
        if ( ErrorCode != ST_NO_ERROR )
        {
            RetErr = TRUE;
            STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
        }
    }

    if ( !RetErr)
    {
        switch ( OutTyp)
        {
            case 1 : /* Analog RGB */
            case 2 : /* Analog YUV */
            case 3 : /* Analog YC */
            case 4 : /* Analog CVBS */
                OutParam.Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
                OutParam.Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
                OutParam.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
                OutParam.Analog.EmbeddedType = FALSE;
                OutParam.Analog.SyncroInChroma = Syn;
                OutParam.Analog.ColorSpace = Col;
                break;

            case 5 : /* Analog SVM */
                break;

            case 6 : /* Digital YCbCr444 24bits */
            case 7 : /* Digital YCbCr422 16bits */
            case 8 : /* Digital YCbCr422 8bits */
            case 9 : /* Digital RGB888 24 bits */
                OutParam.Digital.EmbeddedType = Emb;
                OutParam.Digital.SyncroInChroma = Syn;
                OutParam.Digital.ColorSpace = Col;
                break;

            case 10 : /* Analog HD RGB */
            case 11 : /* Analog HD YUV */
                OutParam.Analog.EmbeddedType = Emb;
                OutParam.Analog.SyncroInChroma = Syn;
                OutParam.Analog.ColorSpace = Col;
                break;

            default :
                break;
        }

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, &OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetEmbeddedSyncro */



/*-------------------------------------------------------------------------
 * Function : VOUT_SetAnalogInverted
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetAnalogInverted( STTST_Parse_t *pars_p, char *result_sym_p )
{
  STVOUT_OutputParams_t OutParam;
  ST_ErrorCode_t ErrorCode;
  BOOL RetErr;
  int LVar;
  BOOL Inv = 0;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get Inverted (default: 0 = FALSE) */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "! expected Inverted, 0:FALSE or 1:TRUE\n" );
        RetErr = TRUE;
    }
    else
    {
        Inv = (BOOL)LVar;
        ErrorCode = STVOUT_GetOutputParams(VOUTHndl, &OutParam);
        if ( ErrorCode != ST_NO_ERROR )
        {
            RetErr = TRUE;
            STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
        }
    }

    if ( !RetErr)
    {
        OutParam.Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam.Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
        OutParam.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
        OutParam.Analog.InvertedOutput = Inv;

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, &OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetAnalogInverted */


/*-------------------------------------------------------------------------
 * Function : VOUT_SetAnalogSVM
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetAnalogSVM( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;
    int P1 = 0, P2 = 0, P3 = 0, P4 = 0, P5 = 0, P6 = 0;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

        /* get SVM parameters :
    U32                       DelayCompensation;
    STVOUT_SVM_ShapeControl_t Shape;
    U32                       Gain;
    STVOUT_SVM_OSD_Factor_t   OSDFactor;
    STVOUT_SVM_Filter_t       VideoFilter;
    STVOUT_SVM_Filter_t       OSDFilter;  */

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Delay Compensation " );
        RetErr = TRUE;
    }
    P1 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Shape " );
        RetErr = TRUE;
    }
    P2 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Gain " );
        RetErr = TRUE;
    }
    P3 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "OSD Factor " );
        RetErr = TRUE;
    }
    P4 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Video Filter " );
        RetErr = TRUE;
    }
    P5 = LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "OSD Filter " );
        RetErr = TRUE;
    }
    P6 = LVar;

    ErrorCode = STVOUT_GetOutputParams(VOUTHndl, OutParam);
    if ( ErrorCode != ST_NO_ERROR )
    {
        RetErr = TRUE;
        STTBX_Print(( ">> pb VOUT_GetOutputParams (%d) err %x\n", VOUTHndl, ErrorCode ));
    }

    if ( !RetErr)
    {
        OutParam->SVM.StateAnalogSVM = STVOUT_PARAMS_AFFECTED;
        OutParam->SVM.DelayCompensation = (U32)P1;
        OutParam->SVM.Shape = (STVOUT_SVM_ShapeControl_t)P2;
        OutParam->SVM.Gain = (U32)P3;
        OutParam->SVM.OSDFactor = (STVOUT_SVM_OSD_Factor_t)P4;
        OutParam->SVM.VideoFilter = (STVOUT_SVM_Filter_t)P5;
        OutParam->SVM.OSDFilter = (STVOUT_SVM_Filter_t)P6;

        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
        sprintf( VOUT_Msg, "STVOUT_SetOutputParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetAnalogSVM */

/*-------------------------------------------------------------------------
 * Function : VOUT_AdjustChromaLumaDelay
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_AdjustChromaLumaDelay( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STVOUT_ChromaLumaDelay_t  Delay=0;
    U32 LVar;

    UNUSED_PARAMETER(result_sym_p);

    /* Get VOUT handle*/
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p,(S32)(STVOUT_CHROMALUMA_DELAY_0_5+1), (S32*)&LVar);
    if(RetErr)
    {
        STTST_TagCurrentLine(pars_p,"Expected CLDelay (DELAY_XX with XX in { M2, M1_5, M1, M0_5, 0, 0_5,P1,P1_5, P2, P2_5");
        return(TRUE);
    }
    if ( (LVar == 0) || (LVar > 10) )  /* values to test outside Format type */
    {
        if ( LVar == 0)
        {
            Delay = STVOUT_CHROMALUMA_DELAY_M2-1;
        }
        if ( LVar > 10)
        {
            Delay = STVOUT_CHROMALUMA_DELAY_P2_5+1;
        }
    }
    else /* CLDelay ok */
    {
        Delay  = (STVOUT_ChromaLumaDelay_t)(LVar-1);
    }

    sprintf( VOUT_Msg, "STVOUT_AdjustChromaLumaDelay(%d) : ", VOUTHndl );
    ErrorCode = STVOUT_AdjustChromaLumaDelay(VOUTHndl, Delay);
    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    sprintf( VOUT_Msg,"\tCLDelay Applied is %s \n",(LVar < VOUT_MAX_CLDELAY_SUPPORTED+1) ? VoutCLDelayName[LVar].Name : "????");
    VOUT_PRINT (VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}

#if defined (ST_5528) || defined (ST_5100) || defined (ST_7710)|| defined(ST_7100) ||defined (ST_5301)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
/*-------------------------------------------------------------------------
 * Function : VOUT_SetDencAuxMain
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_SetDencAuxMain( STTST_Parse_t *pars_p, char *result_sym_p )
 {

    BOOL RetErr;
    #if defined(ST_5528 )||defined(ST_7710)|| defined (ST_7100) || defined (ST_7109)
    U32 Value=0;
    #endif
    ST_ErrorCode_t      ErrorCode=ST_NO_ERROR;

    UNUSED_PARAMETER(result_sym_p);

    /* Get VOUT handle*/
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    sprintf( VOUT_Msg, "VOUT_SetDencAux(%d): ", VOUTHndl);
    #if defined(ST_5528)
     Value = stvout_rd32((U32)VOS_MISC_CTL);
     Value = Value|VOS_MISC_CTL_DENC_AUX_SEL;
     stvout_wr32((U32)VOS_MISC_CTL,Value)

    #elif defined (ST_7710)|| defined(ST_7100) || defined (ST_7109)  /** select Main Mix **/
      Value = stvout_rd32(((U32)VOS_BASE_ADDRESS_1 +(U32)DSPCFG_CLK));
      Value = Value|DSPCFG_CLK_DENC_MAIN_SEL;
      stvout_wr32(((U32)VOS_BASE_ADDRESS_1+(U32)DSPCFG_CLK),Value);
    #endif

    RetErr = VOUT_TextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
  }
#endif

#if defined (ST_7710) || defined (ST_7100) || defined(ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
/*-------------------------------------------------------------------------
 * Function : VOUT_GetState
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetState( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_State_t State;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    ErrorCode = STVOUT_GetState(VOUTHndl, &State);
    sprintf( VOUT_Msg, "STVOUT_GetState (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    if (ErrorCode==ST_NO_ERROR)
    {
        switch (State)
        {
            case STVOUT_DISABLED:
                STTBX_Print(( "<<\tState :Non DVI/HDMI Use: STVOUT_DISABLED\n"));
                break;
            case STVOUT_ENABLED:
                STTBX_Print(( "<<\tState :Non DVI/HDMI Use: STVOUT_ENABLED\n"));
                break;
            case STVOUT_NO_RECEIVER:
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_NO_RECEIVER\n"));
                break;
            case STVOUT_RECEIVER_CONNECTED:
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_RECEIVER_CONNECTED\n"));
                break;
            case STVOUT_NO_ENCRYPTION :
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_NO_ENCRYPTION\n"));
                break;
            case STVOUT_NO_HDCP_RECEIVER :
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_NO_HDCP_RECEIVER\n"));
                break;
            case STVOUT_AUTHENTICATION_IN_PROGRESS :
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_AUTHENTICATION_IN_PROGRESS\n"));
                break;
            case STVOUT_AUTHENTICATION_FAILED :
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_AUTHENTICATION_FAILED\n"));
                break;
            case STVOUT_AUTHENTICATION_SUCCEEDED :
                STTBX_Print(( "<<\tState :With DVI/HDMI Use: STVOUT_AUTHENTICATION_SUCCEEDED"));
                break;
            default :
                STTST_TagCurrentLine( pars_p, VOUT_Msg );
                return(TRUE);
                break;
        }
        STTST_AssignInteger( "STATE", State, FALSE);
     }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetState */
/*-------------------------------------------------------------------------
 * Function : VOUT_GetTargetInfo
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_GetTargetInfo( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_TargetInformation_t TargetInfo;
    U8 i,j,Data1,Checksum=0;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    ErrorCode = STVOUT_GetTargetInformation(VOUTHndl, &TargetInfo);
    sprintf( VOUT_Msg, "STVOUT_GetTargetInformation (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    if (!RetErr)
    {
      STTBX_Print(( "Size Buffer :%d , Add Buffer : 0x%x \n",TargetInfo.SinkInfo.Size,(U32)TargetInfo.SinkInfo.Buffer_p));
      for(i=0;i<TargetInfo.SinkInfo.Size/16;i++)
      {
        if((i)%8==0)
        {
            STTBX_Print(( "Block %d\n",(i/8)+1));
        }
        STTBX_Print(( "0x%2x: ",i*16));
        for(j=0;j<8;j++)
        {
            Data1 = (U8)(*(TargetInfo.SinkInfo.Buffer_p+i*16+j));
            STTBX_Print((  " %2x",  Data1  ));
            Checksum+= Data1;
        }
        STTBX_Print(( " ; "));
        for(j=8;j<16;j++)
        {
            Data1 = (U8)(*(TargetInfo.SinkInfo.Buffer_p+i*16+j));
            STTBX_Print((  " %2x",  Data1  ));
            Checksum+= Data1;
        }
        STTBX_Print(( " \n"));
        if((i+1)%8==0)
        {
            if (Checksum==0)
            {
                STTBX_Print(( "Checksum test OK\n\n"));
            }
            else
            {
                STTBX_Print(( "Checksum Error\n\n"));
            }
            Checksum = 0;
        }
      }
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetTargetInfo  */

/*-------------------------------------------------------------------------
 * Function : VOUT_SendData
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_SendData( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p,0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected InfoFrame Type \n");
        return(TRUE);
    }

    ErrorCode = STVOUT_SendData(VOUTHndl,LVar, Data, VOUT_MAX_DATA_SIZE);
    sprintf( VOUT_Msg, "STVOUT_SendData (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_SendData */

/*-------------------------------------------------------------------------
 * Function : VOUT_HDMI_SetOutputType
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_HDMI_SetOutputType( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int Var;
    char TrvStr[80], IsStr[80], *ptr;

    UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /*{ "HDMI_RGB888", STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 },
    { "HDMI_YcbCr444", STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 },
    { "HDMI_YCbCr422" , STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 },*/
    STTST_GetItem(pars_p, "HDMI_RGB888", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for (Var=12; Var<14+1; Var++)
    {
        if (  (strcmp(VoutTypeName[Var].Name, TrvStr)==0)
           || (strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0))
        {
            break;
        }
    }
    if (Var==14+1)
    {
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected HDMI ouput type");
            return(TRUE);
        }
    }

    ErrorCode = STVOUT_SetHDMIOutputType(VOUTHndl,VoutTypeName[Var].Val);
    sprintf( VOUT_Msg, "STVOUT_HDMI_SetOutputType (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_SendData */
#ifdef STVOUT_HDCP_PROTECTION
/*-------------------------------------------------------------------------
 * Function : VOUT_SetHDCPParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetHDCPParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    U32 IV_0, IV_1, KSV_0,KSV_1,IRate;
    int LVar;
    STVOUT_HDCPParams_t *HDCPParam_p = &Str_HdcpParam;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, IV_0_KEY_VALUE, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected IV0 " );
        RetErr = TRUE;
    }
    IV_0 = (U32)LVar;

    RetErr = STTST_GetInteger( pars_p, IV_1_KEY_VALUE, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected IV1 " );
        RetErr = TRUE;
    }
    IV_1 = (U32)LVar;

    RetErr = STTST_GetInteger( pars_p, KSV_0_KEY_VALUE, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected KSV0 " );
        RetErr = TRUE;
    }
    KSV_0 = (U32)LVar;

    RetErr = STTST_GetInteger( pars_p, KSV_1_KEY_VALUE, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected KSV1 " );
        RetErr = TRUE;
    }
    KSV_1 = (U32)LVar;
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected IRate " );
        RetErr = TRUE;
    }
    IRate = (U32)LVar;

    RetErr = STTST_GetInteger( pars_p,0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Advance Cipher Mode selection (TRUE, FALSE)" );
        return(TRUE);
    }

    if (!RetErr)
    {
        HDCPParam_p->IV_0= IV_0;
        HDCPParam_p->IV_1= IV_1;
        HDCPParam_p->KSV_0= KSV_0;
        HDCPParam_p->KSV_1= KSV_1;
        HDCPParam_p->IRate = IRate;
        HDCPParam_p->IsACEnabled = (BOOL)LVar;
        memcpy(HDCPParam_p->DeviceKeys, DevKeys, sizeof(U32)*80);
        ErrorCode = STVOUT_SetHDCPParams(VOUTHndl, HDCPParam_p);
        sprintf( VOUT_Msg, "STVOUT_SetHDCPParams (%d) :", VOUTHndl );
        RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_SetHDCPParams */
/*-------------------------------------------------------------------------
 * Function : VOUT_GetHDCPSinkParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_GetHDCPSinkParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_HDCPSinkParams_t  HDCPSinkParams;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    ErrorCode = STVOUT_GetHDCPSinkParams(VOUTHndl, &HDCPSinkParams);
    sprintf( VOUT_Msg, "STVOUT_GetHDCPSinkParams (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetHDCPSinkParams */

/*-------------------------------------------------------------------------
 * Function : VOUT_SetForbiddenKSVs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetForbiddenKSVs( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    int LVar;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Number of Forbidden");
        return(TRUE);
    }

    ErrorCode = STVOUT_UpdateForbiddenKSVs(VOUTHndl, ForbiddenKSV, LVar);
    sprintf( VOUT_Msg, "STVOUT_UpdateForbiddenKSVs (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_SetForbiddenKSVs */
/*-------------------------------------------------------------------------
 * Function : VOUT_DisableDefaultOutput
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VOUT_DisableDefaultOutput( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    ErrorCode = STVOUT_DisableDefaultOutput(VOUTHndl);
    sprintf( VOUT_Msg, "STVOUT_DisableDefaultOutput (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_DisableDefaultOutput */
/*-------------------------------------------------------------------------
 * Function : VOUT_DisableDefaultOutput
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VOUT_EnableDefaultOutput( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    STVOUT_DefaultOutput_t        DefaultOutput;
    S32 LVar1,LVar2,LVar3;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 200, (S32*)&LVar1);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Chanel 1 Color");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 20, (S32*)&LVar2);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Chanel 2 Color");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 60, (S32*)&LVar3);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Chanel 3 Color");
        return(TRUE);
    }

    DefaultOutput.DataChannel0 = LVar1;
    DefaultOutput.DataChannel1 = LVar2;
    DefaultOutput.DataChannel2 = LVar3;
    ErrorCode = STVOUT_EnableDefaultOutput(VOUTHndl,&DefaultOutput);
    sprintf( VOUT_Msg, "STVOUT_EnableDefaultOutput (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_DisableDefaultOutput */
#endif /*STVOUT_HDCP_PROTECTION*/


#ifdef STVOUT_CEC_PIO_COMPARE
/*-------------------------------------------------------------------------
 * Function : VOUT_CECSendMessage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VOUT_CECSendMessage( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    BOOL RetErr;
    STVOUT_CECMessage_t CEC_Message;

    S32 Length,i,DestAddress,SourceAddress,CEC_Data;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 15, (S32*)&SourceAddress);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Source Address");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 3, (S32*)&DestAddress);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Destination Address");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&Length);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Data Length");
        return(TRUE);
    }
    if(Length > 14)
    {
        Length = 14;
    }
    for ( i=0;i<Length;i++)
    {
        RetErr = STTST_GetInteger( pars_p, 3, (S32*)&CEC_Data);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected Data i");
            return(TRUE);
        }
        CEC_Message.Data[i] = CEC_Data;
    }

    CEC_Message.DataLength = Length;
    CEC_Message.Source = SourceAddress;
    CEC_Message.Destination = DestAddress ;
    CEC_Message.Retries = 1;


    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    ErrorCode = STVOUT_SendCECMessage(VOUTHndl, &CEC_Message);
    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    sprintf( VOUT_Msg, "STVOUT_SendCECMessage (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_CECSendMessage */


/*-------------------------------------------------------------------------
 * Function : VOUT_CECPhysicalAddressAvailable
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VOUT_CECPhysicalAddressAvailable( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    ErrorCode = STVOUT_CECPhysicalAddressAvailable(VOUTHndl);
    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    sprintf( VOUT_Msg, "STVOUT_CECPhysicalAddressAvailable (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_CECPhysicalAddressAvailable */

/*-------------------------------------------------------------------------
 * Function : VOUT_CECSetAdditionalAddress
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL VOUT_CECSetAdditionalAddress( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    BOOL                RetErr;
    S32                 Role;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 15, (S32*)&Role);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }


    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    ErrorCode = STVOUT_CECSetAdditionalAddress(VOUTHndl, Role);
    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    sprintf( VOUT_Msg, "STVOUT_CECSetAdditionalAddress (%d,%d) :", VOUTHndl, Role );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_CECSetAdditionalAddress */


#endif /*STVOUT_CEC_PIO_COMPARE*/


/*-------------------------------------------------------------------------
 * Function : VOUT_Start
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL VOUT_Start( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    ErrorCode = STVOUT_Start(VOUTHndl);
    sprintf( VOUT_Msg, "STVOUT_Start (%d) :", VOUTHndl );
    RetErr = VOUT_DotTextError( ErrorCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_Start */

 /*-------------------------------------------------------------------------
 * Function : VOUT_SetActiveVideo
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetActiveVideo( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    U32 add;
    int LVar;
    int Xmin = 0, Xmax = 0, Ymin = 0, Ymax = 0;

#if defined (ST_OSLINUX)
    U32 MappedHDMIBase = (U32)HDMI_MAPPED_BASE_ADDRESS ;
#else
    U32 base = HDMI_BASE_ADDRESS;
#endif /* ST_OSLINUX */


    #define _XMIN  0x100
    #define _XMAX  0x104
    #define _YMIN  0x108
    #define _YMAX  0x10C

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Xmin active video " );
        RetErr = TRUE;
    }
    Xmin = LVar;

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Xmax active video " );
        RetErr = TRUE;
    }
    Xmax = LVar;

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Ymin active video " );
        RetErr = TRUE;
    }
    Ymin = LVar;

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Ymax active video " );
        RetErr = TRUE;
    }
    Ymax = LVar;
#if defined(ST_OSLINUX)
    add = MappedHDMIBase + (U32)_XMIN;
    printf("Xmin :  (%x)\n\r", Xmin);
    STAPIGAT_Phys_Write((U32 *)(add), Xmin);

    add = MappedHDMIBase + (U32)_XMAX;
    printf("Xmax :  (%x)\n\r", Xmax);
    STAPIGAT_Phys_Write((U32 *)(add), Xmax);

    add = MappedHDMIBase + (U32)_YMIN;
    printf("Ymin :  (%x)\n\r", Ymin);
    STAPIGAT_Phys_Write((U32 *)(add), Ymin);

    add = MappedHDMIBase + (U32)_YMAX;
    printf("Ymax :  (%x)\n\r", Ymax);
    STAPIGAT_Phys_Write((U32 *)(add), Ymax);

#else
    add = base + (U32)_XMIN;
    *(volatile U32*)add= Xmin;
    add = base + (U32)_XMAX;
    *(volatile U32*)add = Xmax;
    add = base + (U32)_YMIN;
    *(volatile U32*)add = Ymin;
    add = base + (U32)_YMAX;
    *(volatile U32*)add = Ymax;

#endif  /*ST_OSLINUX*/

    STTBX_Print(( ">> Settings HDMI active Video Window :\n"));
    STTBX_Print(( " STVOUT_SetActiveVideo(%d) : Done \n", VOUTHndl));

    return(FALSE);

} /* end VOUT_SetActiveVideo  */
#endif
/*-------------------------------------------------------------------------
 * Function : VOUT_ReadDac
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadDac( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8 tab[6], temp, RegisterShift = REG_SHIFT;
    U32 add;
    int LVar;
    int DevTyp = 0;
    halvout_DencVersion_t   Version;
    BOOL Prnt = 0;
    BOOL RetErr;
  #if defined (ST_5525)
    U32 Source = 0;
  #endif
    U32 base = (U32)BASE_ADDRESS_DENC;
#define _DAC13  0x11  /* Denc gain for tri-dac 1-3                   [7:0]    */
#if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109) \
  ||defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined (ST_7111)|| defined (ST_7105)
    #define _DAC34  0x12  /* Denc gain for tri-dac 3-4               [7:0]    */
    #define _DAC45  0x13  /* Denc gain for tri-dac 4-5               [7:0]    */
    #define _DAC6   0x6B  /* Denc gain for tri-dac 6                 [7:0]    */
    #define _DACC   0x41  /* DACC Gain, Mask of TTX, En/Disab reg 0x42-0x44   */
    #define _DAC2   0x6A  /* Denc gain for tri-dac 2                 [7:0]    */
#elif defined(ST_5188) || defined(ST_5525)
    #define _DAC3   0x12  /* Denc gain for tri-dac 3-4               [7:0]    */
    #define _DAC2   0x6A  /* Denc gain for tri-dac 2                 [7:0]    */
    #define _DAC4   0x6B  /* Denc gain for tri-dac 4                 [7:0]    */
    #define _DACC   0x41  /* DACC Gain, Mask of TTX, En/Disab reg 0x42-0x44   */
#else
    #define _DAC45  0x12  /* Denc gain for tri-dac 4-5               [7:0]    */
    #define _DAC6C  0x13  /* Denc gain for tri-dac 6-C               [7:0]    */
    #define _DAC2   0x41  /* DAC2 Gain, Mask of TTX, En/Disab reg 0x42-0x44   */
#endif

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr|| (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "expected  type  (1=Stv0119/STi4629, 0=OnChip)\n" );
        RetErr = TRUE;
    }
    else
    {
        DevTyp = LVar;
        /* get print option */
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr || (LVar < 0 || LVar > 1))
        {
            STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
            RetErr = TRUE;
        }
        else
        {
            Prnt = (BOOL)LVar;
        }
    }
#if defined (ST_5525)
       RetErr = STTST_GetInteger( pars_p, (STVOUT_Source_t)0, (S32*)&Source);
        if (RetErr)
        {
              STTST_TagCurrentLine( pars_p, "Expected Source Selected main=0 and Aux=1");
              return(TRUE);
        }
        if ( Source==1)
        {
            base = (U32)ST5525_VOS_1_BASE_ADDRESS;
        }
#endif
    STTST_AssignInteger( "DAC1", 0, FALSE);
    STTST_AssignInteger( "DAC2", 0, FALSE);
    STTST_AssignInteger( "DAC3", 0, FALSE);
#if defined  (mb376)|| defined (espresso)
    if ( DevTyp == 0)
    {
    STTST_AssignInteger( "DAC4", 0, FALSE);
    STTST_AssignInteger( "DAC5", 0, FALSE);
    STTST_AssignInteger( "DAC6", 0, FALSE);
    }
#else
    STTST_AssignInteger( "DAC4", 0, FALSE);
    STTST_AssignInteger( "DAC5", 0, FALSE);
    STTST_AssignInteger( "DAC6", 0, FALSE);
#endif /* mb376 || espresso*/
    STTST_AssignInteger( "DACC", 0, FALSE);

    if ( Prnt)
    {
        STTBX_Print(( ">>DAC Level :\n"));
    }
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    if ( DevTyp == 1)
    {
        base = (U32)ST4629_BASE_ADDRESS +(U32)STI4629_DNC_BASE_ADDRESS ;
        RegisterShift = REG_SHIFT_4629 ;
    }
#else
    if ( DevTyp == 1)
    {
        STTBX_Print(( "  not available on STV0119 \n"));
    }
    else
#endif /* mb376 || espresso*/
    {
        Version = halvout_DencVersionOnChip(DevTyp);
        if ( Version < 10)
        {
            STTBX_Print(( "  not available on DENC version<10 (denc %d)\n", Version));
        }
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || defined(mb376) || \
    defined (espresso)|| ((defined(ST_5514) || defined(ST_5528) || defined(ST_5517)) && !defined(ST_7020))

         add = (base + ((U32)_DAC13 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[0] = temp ;
          add =  (base + ((U32)_DAC45 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[1] = temp ;
         add =  (base + ((U32)_DAC6C << RegisterShift));
         temp = *((volatile U8 *) add);
         tab[2] = temp ;
         add = (base + ((U32)_DAC2  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[3] = temp ;

#elif defined(ST_5100) ||defined(mb390)|| defined(ST_7710) ||defined(mb391) ||defined(ST_5105)|| defined(mb400)\
|| defined(ST_7100)|| defined(mb411)||defined (ST_5301)||defined (ST_7109)|| defined(ST_5107)|| defined(mb436)||defined (mb634) \
|| defined(DTT5107)|| defined(ST_5162)|| defined(ST_5162)
         add = (base + ((U32)_DAC13 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[0] = temp ;
         add =  (base + ((U32)_DAC34 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[1] = temp ;
         add =  (base + ((U32)_DAC45 << RegisterShift));
         temp = *((volatile U8 *) add);
         tab[2] = temp ;
         add = (base + ((U32)_DAC6  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[3] = temp ;
         add = (base + ((U32)_DACC  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[4] = temp ;
         add = (base + ((U32)_DAC2  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[5] = temp ;
#elif defined (ST_5188) || defined (ST_5525)|| defined (mb457)|| defined (mb428)
         add = (base + ((U32)_DAC13 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[0] = temp ;
         add =  (base + ((U32)_DAC3 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[1] = temp ;
         add =  (base + ((U32)_DAC4 << RegisterShift));
         temp = *((volatile U8 *) add);
         tab[2] = temp ;
         add = (base + ((U32)_DACC  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[3] = temp ;
         add = (base + ((U32)_DAC2  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[4] = temp ;
#elif defined (ST_7200)||defined (mb519)|| defined (ST_7111)|| defined (mb618)|| defined (ST_7105) || defined (mb680)
         add = (base + ((U32)_DAC13 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[0] = temp ;
         add =  (base + ((U32)_DAC34 << RegisterShift));
         temp = *((volatile U8 *)add);
         tab[1] = temp ;
         add = (base + ((U32)_DAC2  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[2] = temp ;
         add = (base + ((U32)_DACC  << RegisterShift)) ;
         temp = *((volatile U8 *) add );
         tab[3] = temp ;
#else
         add = 0;
         temp = (U8)(*(volatile U32 *)(base + ((U32)_DAC13 << RegisterShift)));
         tab[0] = temp ;
         temp = (U8)(*(volatile U32 *)(base + ((U32)_DAC45 << RegisterShift)));
         tab[1] = temp ;
         temp = (U8)(*(volatile U32 *)(base + ((U32)_DAC6C << RegisterShift)));
         tab[2] = temp ;
         temp = (U8)(*(volatile U32 *)(base + ((U32)_DAC2  << RegisterShift)));
         tab[3] = temp ;
 #endif

        if ( Prnt)
        {
            STTBX_Print(( "  R17 %2.2x, R18 %2.2x, R19 %2.2x",(U32)tab[0],(U32)tab[1],(U32)tab[2] ));
            #if defined (ST_5100) || defined (ST_7710)|| defined(ST_7100) ||defined (ST_5301)|| defined (ST_7109)
                STTBX_Print(( "  R20 %2.2x, R21 %2.2x", (U32)tab[3], (U32)tab[4] ));
                STTBX_Print(( ", R65 %2.2x", (U32)tab[5] ));
                STTBX_Print(( "\n"));
                STTBX_Print(( "  => Dac1 %1.1x ,", ((((U32)tab[0]) & 0xFC) >> 2)   ));
                STTBX_Print(( " Dac2 %1.1x ,", ((((U32)tab[5]) & 0x3F))   ));
                STTBX_Print(( " Dac3 %1.1x ,", (((((U32)tab[0]) & 0x03)<<4) | ((((U32)tab[1]) & 0xF0)>>4))   ));
                STTBX_Print(( " Dac4 %1.1x ,", (((((U32)tab[1]) & 0x0F)<<2) | ((((U32)tab[2]) & 0xC0)>>6))   ));
                STTBX_Print(( " Dac5 %1.1x ,", ((((U32)tab[2]) & 0x3F) )  ));
                STTBX_Print(( " Dac6 %1.1x ,", ((((U32)tab[3]) & 0x3F) )  ));
                STTBX_Print(( " DacC %1.1x \n", ((((U32)tab[4]) & 0xF0)>> 4 )  ));
            #elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
                STTBX_Print(( "  => Dac1 %1.1x ,", ((((U32)tab[0]) & 0xFC) >> 2)   ));
                STTBX_Print(( " Dac2 %1.1x ,", ((((U32)tab[2]) & 0x3F))   ));
                STTBX_Print(( " Dac3 %1.1x ,", (((((U32)tab[0]) & 0x03)<<4) | ((((U32)tab[1]) & 0xF0)>>4))   ));
                STTBX_Print(( " DacC %1.1x \n", ((((U32)tab[3]) & 0xF0)>> 4 )  ));
            #else
                STTBX_Print(( ", R65 %2.2x", tab[3] ));
                STTBX_Print(( "\n"));
                STTBX_Print(( "  => Dac1 %1.1x ,", ((((U32)tab[0]) & 0xF0) >> 4)   ));
                STTBX_Print(( " Dac2 %1.1x ,", ((((U32)tab[3]) & 0xF0) >> 4)   ));
                STTBX_Print(( " Dac3 %1.1x , Dac4 %1.1x,", (((U32)tab[0]) & 0xF), ((((U32)tab[1]) & 0xF0) >> 4) ));
                STTBX_Print(( " Dac5 %1.1x , Dac6 %1.1x , DacC %1.1x\n",(((U32)tab[1]) & 0xF), ((((U32)tab[2]) & 0xF0) >> 4),
                                                                        (((U32)tab[2]) & 0xF) ));
            #endif
        }

        #if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100) ||defined(ST_5301)|| defined(ST_7109)\
         || defined(ST_5107)|| defined(ST_5162)
            STTST_AssignInteger( "DAC1", ((tab[0] & 0xFC) >> 2), FALSE);
            STTST_AssignInteger( "DAC2", (tab[5] & 0x3F) , FALSE);
            STTST_AssignInteger( "DAC3",  (((tab[0] & 0x03)<<4) | ((tab[1] & 0xF0)>>4)),        FALSE);
            STTST_AssignInteger( "DAC4", (((tab[1] & 0x0F)<<2) | ((tab[2] & 0xC0)>>6)), FALSE);
            STTST_AssignInteger( "DAC5",   (tab[2] & 0x3F) ,        FALSE);
            STTST_AssignInteger( "DAC6", (tab[3] & 0x3F) , FALSE);
            STTST_AssignInteger( "DACC",  ( (tab[4] & 0xF0)>> 4 ),        FALSE);
        #elif defined (ST_5188) || defined (ST_5525)
            STTST_AssignInteger( "DAC1", ((tab[0] & 0xFC) >> 2), FALSE);
            STTST_AssignInteger( "DAC2", (tab[4] & 0x3F) , FALSE);
            STTST_AssignInteger( "DAC3",  (((tab[0] & 0x03)<<4) | ((tab[1] & 0xF0)>>4)),        FALSE);
            STTST_AssignInteger( "DAC4", ((tab[2] & 0x3F)), FALSE);
            STTST_AssignInteger( "DACC",  ( (tab[3] & 0xF0)>> 4 ),        FALSE);
        #elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
            STTST_AssignInteger( "DAC1", ((tab[0] & 0xFC) >> 2), FALSE);
            STTST_AssignInteger( "DAC2", (tab[2] & 0x3F) , FALSE);
            STTST_AssignInteger( "DAC3",  (((tab[0] & 0x03)<<4) | ((tab[1] & 0xF0)>>4)),        FALSE);
            STTST_AssignInteger( "DACC",  ( (tab[3] & 0xF0)>> 4 ),        FALSE);
        #else
            STTST_AssignInteger( "DAC1", ((tab[0]& 0xF0) >> 4), FALSE);
            STTST_AssignInteger( "DAC2", ((tab[3]& 0xF0) >> 4), FALSE);
            STTST_AssignInteger( "DAC3",  (tab[0]& 0xF),        FALSE);
            STTST_AssignInteger( "DAC4", ((tab[1]& 0xF0) >> 4), FALSE);
            STTST_AssignInteger( "DAC5",  (tab[1]& 0xF),        FALSE);
            STTST_AssignInteger( "DAC6", ((tab[2]& 0xF0) >> 4), FALSE);
            STTST_AssignInteger( "DACC",  (tab[2]& 0xF),        FALSE);
        #endif
    }
    return(FALSE);
} /* end VOUT_ReadDac */



/*-------------------------------------------------------------------------
 * Function : VOUT_ReadBSC
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadBCS( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8 tab[3], RegisterShift = REG_SHIFT;
    int LVar;
    int DevTyp = 0;
    halvout_DencVersion_t   Version;
    BOOL Prnt = 0;
    BOOL RetErr;
    U32 base = (U32)BASE_ADDRESS_DENC;
#define _BRIGHT 0x45  /* Brightness                                      */
#define _CONTRA 0x46  /* Contrast                                        */
#define _SATURA 0x47  /* Saturation                                      */

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr|| (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "expected  type  (1=Stv0119/STi4629, 0=OnChip)\n" );
        RetErr = TRUE;
    }
    else
    {
        DevTyp = LVar;
        /* get print option */
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr || (LVar < 0 || LVar > 1))
        {
            STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
            RetErr = TRUE;
        }
        else
        {
            Prnt = (BOOL)LVar;
        }
    }

    STTST_AssignInteger( "BRIGHT", 0, FALSE);
    STTST_AssignInteger( "CONTRA", 0, FALSE);
    STTST_AssignInteger( "SATURA", 0, FALSE);
    if ( Prnt)
    {
        STTBX_Print(( ">>B C S :\n"));
    }
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    if ( DevTyp == 1)
    {
        base = (U32)ST4629_BASE_ADDRESS +(U32)STI4629_DNC_BASE_ADDRESS ;
        RegisterShift = REG_SHIFT_4629 ;
    }
#else
    if ( DevTyp == 1)
    {
        STTBX_Print(( "  not available on STV0119 \n"));
    }
    else
    {
#endif /* ((defined (mb376) && !defined(ST_7020)) || defined (espresso)) */
        Version = halvout_DencVersionOnChip(DevTyp);
        if ( Version < 10)
        {
            STTBX_Print(( "  not available on DENC version<10 (denc %d)\n", Version));
        }
#if defined(ST_5510) || defined(ST_5512)|| defined(ST_5508)|| defined(ST_5518)|| defined(ST_5516) || defined(mb376) \
  ||defined(espresso)|| defined(ST_5100)|| defined(ST_5301)|| defined(ST_7710)|| defined(ST_5105) || defined(ST_7100) \
  ||defined(ST_7109) || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined (ST_7200)|| defined(ST_5162) \
  ||((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

        tab[0] = *((volatile U8 *)(base + ( (U32)_BRIGHT << RegisterShift)));
        tab[1] = *((volatile U8 *)(base + ( (U32)_CONTRA << RegisterShift)));
        tab[2] = *(volatile U8 *)(base + ( (U32)_SATURA << RegisterShift));
#else
        tab[0] = (U8)(*(volatile U32 *)(base + ( (U32)_BRIGHT << RegisterShift)));
        tab[1] = (U8)(*(volatile U32 *)(base + ( (U32)_CONTRA << RegisterShift)));
        tab[2] = (U8)(*(volatile U32 *)(base + ( (U32)_SATURA << RegisterShift)));
#endif
        if ( Prnt)
        {
            STTBX_Print(( "   Brightness %2.2x, Contrast %2.2x, Saturation %2.2x\n", (U32)tab[0], (U32)tab[1], (U32)tab[2]));
        }

        STTST_AssignInteger( "BRIGHT", tab[0], FALSE);
        STTST_AssignInteger( "CONTRA", (S8)tab[1], FALSE);
        STTST_AssignInteger( "SATURA", tab[2], FALSE);
#if !((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    }
#endif /* not defined mb376 and espresso*/

    return(FALSE);
} /* end VOUT_ReadBCS */


/*-------------------------------------------------------------------------
 * Function : VOUT_ReadChroma
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadChroma( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8 tab[9], RegisterShift = REG_SHIFT;
    halvout_DencVersion_t   Version;
    int LVar;
    int DevTyp = 0;
    BOOL Prnt = 0;
    BOOL RetErr;
    U32 base = (U32)BASE_ADDRESS_DENC;
#define _CHRM0  0x48  /* Chroma filter coefficient 0                     */
#define _CHRM1  0x49  /* Chroma filter coefficient 1                     */
#define _CHRM2  0x4A  /* Chroma filter coefficient 2                     */
#define _CHRM3  0x4B  /* Chroma filter coefficient 3                     */
#define _CHRM4  0x4C  /* Chroma filter coefficient 4                     */
#define _CHRM5  0x4D  /* Chroma filter coefficient 5                     */
#define _CHRM6  0x4E  /* Chroma filter coefficient 6                     */
#define _CHRM7  0x4F  /* Chroma filter coefficient 7                     */
#define _CHRM8  0x50  /* Chroma filter coefficient 8                     */

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr|| (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "expected  type  (1=Stv0119/STi4629, 0=OnChip)\n" );
        RetErr = TRUE;
    }
    else
    {
        DevTyp = LVar;
        /* get print option */
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr || (LVar < 0 || LVar > 1))
        {
            STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
            RetErr = TRUE;
        }
        else
        {
            Prnt = (BOOL)LVar;
        }
    }

    STTST_AssignInteger( "CHROMA0", 0, FALSE);
    STTST_AssignInteger( "CHROMA1", 0, FALSE);
    STTST_AssignInteger( "CHROMA2", 0, FALSE);
    STTST_AssignInteger( "CHROMA3", 0, FALSE);
    STTST_AssignInteger( "CHROMA4", 0, FALSE);
    STTST_AssignInteger( "CHROMA5", 0, FALSE);
    STTST_AssignInteger( "CHROMA6", 0, FALSE);
    STTST_AssignInteger( "CHROMA7", 0, FALSE);
    STTST_AssignInteger( "CHROMA8", 0, FALSE);
    if ( Prnt)
    {
        STTBX_Print(( ">>Chroma :\n"));
    }
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    if ( DevTyp == 1)
    {
        base = (U32)ST4629_BASE_ADDRESS +(U32)STI4629_DNC_BASE_ADDRESS ;
        RegisterShift = REG_SHIFT_4629 ;
    }
#else
    if ( DevTyp == 1)
    {
        STTBX_Print(( "  not available on STV0119 \n"));
    }
    else
    {
#endif /* ((defined (mb376) && !defined(ST_7020)) || defined (espresso))*/
        Version = halvout_DencVersionOnChip(DevTyp);
        if ( Version < 10)
        {
            STTBX_Print(( "  not available on DENC version<10 (denc %d)\n", Version));
        }
#if defined(ST_5510) || defined(ST_5512)|| defined(ST_5508)|| defined(ST_5518)|| defined(ST_5516) || defined(mb376) \
 || defined(espresso)|| defined(ST_5100)|| defined(ST_5301)|| defined(ST_7710)|| defined(ST_7100) || defined(ST_7109)\
 || defined(ST_5105) || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined (ST_7200)|| defined(ST_5162)|| defined (ST_7111)|| defined (ST_7105) \
 ||((defined(ST_5514)|| defined (ST_5517)) && !defined(ST_7020))

        tab[0] = *(volatile U8 *)(base + ((U32)_CHRM0 << RegisterShift));
        tab[1] = *(volatile U8 *)(base + ((U32)_CHRM1 << RegisterShift));
        tab[2] = *(volatile U8 *)(base + ((U32)_CHRM2 << RegisterShift));
        tab[3] = *(volatile U8 *)(base + ((U32)_CHRM3 << RegisterShift));
        tab[4] = *(volatile U8 *)(base + ((U32)_CHRM4 << RegisterShift));
        tab[5] = *(volatile U8 *)(base + ((U32)_CHRM5 << RegisterShift));
        tab[6] = *(volatile U8 *)(base + ((U32)_CHRM6 << RegisterShift));
        tab[7] = *(volatile U8 *)(base + ((U32)_CHRM7 << RegisterShift));
        tab[8] = *(volatile U8 *)(base + ((U32)_CHRM8 << RegisterShift));

 #else
        tab[0] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM0 << RegisterShift)));
        tab[1] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM1 << RegisterShift)));
        tab[2] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM2 << RegisterShift)));
        tab[3] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM3 << RegisterShift)));
        tab[4] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM4 << RegisterShift)));
        tab[5] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM5 << RegisterShift)));
        tab[6] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM6 << RegisterShift)));
        tab[7] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM7 << RegisterShift)));
        tab[8] = (U8)(*(volatile U32 *)(base + ((U32)_CHRM8 << RegisterShift)));

 #endif
        if ( Prnt)
        {
            STTBX_Print(( "   %2.2x %2.2x %2.2x  %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n",(U32)tab[0], (U32)tab[1],
                        (U32)tab[2], (U32)tab[3], (U32)tab[4], (U32)tab[5],(U32)tab[6],(U32)tab[7],(U32)tab[8]));
        }
        STTST_AssignInteger( "CHROMA0", tab[0], FALSE);
        STTST_AssignInteger( "CHROMA1", tab[1], FALSE);
        STTST_AssignInteger( "CHROMA2", tab[2], FALSE);
        STTST_AssignInteger( "CHROMA3", tab[3], FALSE);
        STTST_AssignInteger( "CHROMA4", tab[4], FALSE);
        STTST_AssignInteger( "CHROMA5", tab[5], FALSE);
        STTST_AssignInteger( "CHROMA6", tab[6], FALSE);
        STTST_AssignInteger( "CHROMA7", tab[7], FALSE);
        STTST_AssignInteger( "CHROMA8", tab[8], FALSE);
#if !((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    }
#endif /* not defined mb376 and espresso */
    return(FALSE);
} /* end VOUT_ReadChroma */

/*-------------------------------------------------------------------------
 * Function : VOUT_ReadLuma
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadLuma( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8 tab[10], RegisterShift= REG_SHIFT ;
    halvout_DencVersion_t   Version;
    int LVar;
    int DevTyp = 0;
    BOOL Prnt = 0;
    BOOL RetErr;
    U32 base = (U32)BASE_ADDRESS_DENC;
#define _LUMA0  0x52  /* Luma filter coefficient 0                       */
#define _LUMA1  0x53  /* Luma filter coefficient 1                       */
#define _LUMA2  0x54  /* Luma filter coefficient 2                       */
#define _LUMA3  0x55  /* Luma filter coefficient 3                       */
#define _LUMA4  0x56  /* Luma filter coefficient 4                       */
#define _LUMA5  0x57  /* Luma filter coefficient 5                       */
#define _LUMA6  0x58  /* Luma filter coefficient 6                       */
#define _LUMA7  0x59  /* Luma filter coefficient 7                       */
#define _LUMA8  0x5A  /* Luma filter coefficient 8                       */
#define _LUMA9  0x5B  /* Luma filter coefficient 9                       */

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr|| (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "expected  type  (1=Stv0119/STi4629, 0=OnChip)\n" );
        RetErr = TRUE;
    }
    else
    {
        DevTyp = LVar;
        /* get print option */
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr || (LVar < 0 || LVar > 1))
        {
            STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
            RetErr = TRUE;
        }
        else
        {
            Prnt = (BOOL)LVar;
        }
    }

    STTST_AssignInteger( "LUMA0", 0, FALSE);
    STTST_AssignInteger( "LUMA1", 0, FALSE);
    STTST_AssignInteger( "LUMA2", 0, FALSE);
    STTST_AssignInteger( "LUMA3", 0, FALSE);
    STTST_AssignInteger( "LUMA4", 0, FALSE);
    STTST_AssignInteger( "LUMA5", 0, FALSE);
    STTST_AssignInteger( "LUMA6", 0, FALSE);
    STTST_AssignInteger( "LUMA7", 0, FALSE);
    STTST_AssignInteger( "LUMA8", 0, FALSE);
    STTST_AssignInteger( "LUMA9", 0, FALSE);
    if ( Prnt)
    {
        STTBX_Print(( ">>LUMA :\n"));
    }
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
    if ( DevTyp == 1)
    {
        base = (U32)ST4629_BASE_ADDRESS +(U32)STI4629_DNC_BASE_ADDRESS ;
        RegisterShift = REG_SHIFT_4629 ;
    }
#else
    if ( DevTyp == 1)
    {
        STTBX_Print(( "  not available on STV0119 \n"));
    }
    else
#endif /* ((defined (mb376) && !defined(ST_7020)) || defined (espresso)) */
    {
        Version = halvout_DencVersionOnChip(DevTyp);
        if ( Version < 10)
        {
            STTBX_Print(( "  not available on DENC version<10 (denc %d)\n", Version));
        }
#if defined (ST_5510) || defined(ST_5512)|| defined(ST_5508)|| defined(ST_5518)|| defined(ST_5516)|| defined(mb376) \
 || defined (espresso)|| defined(ST_5100)|| defined(ST_5301)|| defined(ST_7710)|| defined(ST_7100)|| defined(ST_7109)\
 || defined (ST_5105) || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_5162)||defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105) \
 ||((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

        tab[0] = *(volatile U8 *)(base + ((U32)_LUMA0 << RegisterShift));
        tab[1] = *(volatile U8 *)(base + ((U32)_LUMA1 << RegisterShift));
        tab[2] = *(volatile U8 *)(base + ((U32)_LUMA2 << RegisterShift));
        tab[3] = *(volatile U8 *)(base + ((U32)_LUMA3 << RegisterShift));
        tab[4] = *(volatile U8 *)(base + ((U32)_LUMA4 << RegisterShift));
        tab[5] = *(volatile U8 *)(base + ((U32)_LUMA5 << RegisterShift));
        tab[6] = *(volatile U8 *)(base + ((U32)_LUMA6 << RegisterShift));
        tab[7] = *(volatile U8 *)(base + ((U32)_LUMA7 << RegisterShift));
        tab[8] = *(volatile U8 *)(base + ((U32)_LUMA8 << RegisterShift));
        tab[9] = *(volatile U8 *)(base + ((U32)_LUMA9 << RegisterShift));
  #else
        tab[0] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA0 << RegisterShift)));
        tab[1] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA1 << RegisterShift)));
        tab[2] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA2 << RegisterShift)));
        tab[3] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA3 << RegisterShift)));
        tab[4] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA4 << RegisterShift)));
        tab[5] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA5 << RegisterShift)));
        tab[6] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA6 << RegisterShift)));
        tab[7] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA7 << RegisterShift)));
        tab[8] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA8 << RegisterShift)));
        tab[9] = (U8)(*(volatile U32 *)(base + ((U32)_LUMA9 << RegisterShift)));

  #endif
        if ( Prnt)
        {
            STTBX_Print(( "  %2.2x %2.2x %2.2x %2.2x  %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x\n", (U32)tab[0], (U32)tab[1],
                        (U32)tab[2],(U32)tab[3], (U32)tab[4], (U32)tab[5],(U32)tab[6], (U32)tab[7], (U32)tab[8], (U32)tab[9]));
        }
        STTST_AssignInteger( "LUMA0", tab[0], FALSE);
        STTST_AssignInteger( "LUMA1", tab[1], FALSE);
        STTST_AssignInteger( "LUMA2", tab[2], FALSE);
        STTST_AssignInteger( "LUMA3", tab[3], FALSE);
        STTST_AssignInteger( "LUMA4", tab[4], FALSE);
        STTST_AssignInteger( "LUMA5", tab[5], FALSE);
        STTST_AssignInteger( "LUMA6", tab[6], FALSE);
        STTST_AssignInteger( "LUMA7", tab[7], FALSE);
        STTST_AssignInteger( "LUMA8", tab[8], FALSE);
        STTST_AssignInteger( "LUMA9", tab[9], FALSE);
    }
    return(FALSE);
} /* end VOUT_ReadLuma */


/*-------------------------------------------------------------------------
 * Function : VOUT_ReadDSPCFG
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadDSPCFG( STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined (ST_7015) || defined (ST_7020)|| defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)
#if defined (ST_7015)
    long base = (STI7015_BASE_ADDRESS+ST7015_DSPCFG_OFFSET);
#elif defined (ST_7020)
    long base = (STI7020_BASE_ADDRESS+ST7020_DSPCFG_OFFSET);
#elif defined(ST_7710) || defined (ST_7100)|| defined (ST_7109)
  #ifdef ST_OSLINUX
    long base = (long) VOS_BASE_ADDRESS_1;
  #else
    long base = VOUT_BASE_ADDRESS;
  #endif
#endif
    U32 tab[8];
    int LVar;
    BOOL Prnt = 0;
    BOOL RetErr;
#if defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)
#define _CLK  0x70  /* Display clocks and global configuration */
#define _DIG  0x74  /* Display digital out configuration */
#define _ANA  0x78  /* Main analog display out configuration */
#define _DAC  0xDC  /* Dac configuration */
#else
#define _CLK  0x00  /* Display clocks and global configuration */
#define _DIG  0x04  /* Display digital out configuration */
#define _ANA  0x08  /* Main analog display out configuration */
#define _CAPT 0x0C  /* capture configuration */
#define _TST  0x2000  /* Test configuration */
#define _SVM  0x2004  /* SVM static value */
#define _REF  0x2008  /* REF static value*/
#define _DAC  0x200C  /* Dac configuration */
#endif

    UNUSED_PARAMETER(result_sym_p);
    /* get print option */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1))
    {
        STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
        RetErr = TRUE;
    }
    else
    {
        Prnt = (BOOL)LVar;
    }
#if defined (ST_7015) || defined (ST_7020)
    STOS_InterruptLock();
    vout_read32( (void *)(base + _CLK) , &tab[0]);
    vout_read32( (void *)(base + _DIG) , &tab[1]);
    vout_read32( (void *)(base + _ANA) , &tab[2]);
    vout_read32( (void *)(base + _CAPT), &tab[3]);
    vout_read32( (void *)(base + _TST) , &tab[4]);
    vout_read32( (void *)(base + _SVM) , &tab[5]);
    vout_read32( (void *)(base + _REF) , &tab[6]);
    vout_read32( (void *)(base + _DAC) , &tab[7]);
    STOS_InterruptUnlock();

    if ( Prnt)
    {
        STTBX_Print(( ">>DSPCFG :\n"));
        STTBX_Print(( "   CLK: 0x%08.8x, DIG: 0x%08.8x, ANA: 0x%08.8x, CAPT: 0x%08.8x\n",
                          tab[0], tab[1], tab[2], tab[3]));
        STTBX_Print(( "   TST: 0x%08.8x, SVM: 0x%08.8x, REF: 0x%08.8x, DAC : 0x%08.8x\n",
                          tab[4], tab[5], tab[6], tab[7]));
    }

    STTST_AssignInteger( "DSPCFGCLK", (U8)(tab[0] & 0x1F), FALSE);
    STTST_AssignInteger( "DSPCFGDIG", (U8)(tab[1] & 0xF), FALSE);
    STTST_AssignInteger( "DSPCFGANA", (U8)(tab[2] & 0x7), FALSE);
    STTST_AssignInteger( "DSPCFGCAPT",(U8)(tab[3] & 0x1), FALSE);
    STTST_AssignInteger( "DSPCFGTST", (U8)(tab[4] & 0xFF), FALSE);
    STTST_AssignInteger( "DSPCFGSVM", (U16)(tab[5] & 0xFF), FALSE);
    STTST_AssignInteger( "DSPCFGREF", (U16)(tab[6] & 0x1FF), FALSE);
    STTST_AssignInteger( "DSPCFGDAC", (U32)(tab[7] & 0x1FFFF), FALSE);
 #elif defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)
    STOS_InterruptLock();
    vout_read32( (void *)(base + _CLK) , &tab[0]);
    vout_read32( (void *)(base + _DIG) , &tab[1]);
    vout_read32( (void *)(base + _ANA) , &tab[2]);
    vout_read32( (void *)(base + _DAC) , &tab[3]);
    STOS_InterruptUnlock();

    if ( Prnt)
    {
        STTBX_Print(( ">>DSPCFG :\n"));
        STTBX_Print(( "   CLK: 0x%8.8x, DIG: 0x%8.8x, ANA: 0x%8.8x\n",
                          tab[0], tab[1], tab[2]));
        STTBX_Print(( "   TST: DAC : 0x%8.8x\n",
                          tab[3]));
    }

    STTST_AssignInteger( "DSPCFGCLK", (U8)(tab[0] & 0xFF), FALSE);
    #if defined (ST_7710)
        STTST_AssignInteger( "DSPCFGDIG", (U8)(tab[1] & 0x3), FALSE);
        STTST_AssignInteger( "DSPCFGDAC", (U32)(tab[3] & 0xFF), FALSE);
    #elif defined (ST_7100)|| defined (ST_7109)
        STTST_AssignInteger( "DSPCFGDIG", (U8)(tab[1] & 0x7), FALSE);
        STTST_AssignInteger( "DSPCFGDAC", (U32)(tab[3] & 0x1FF), FALSE);
    #endif
    STTST_AssignInteger( "DSPCFGANA", (U8)(tab[2] & 0xF), FALSE);

#endif
#else
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

#endif
    return(FALSE);
} /* end VOUT_ReadDSPCFG */


/*-------------------------------------------------------------------------
 * Function : VOUT_ReadSVM
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadSVM( STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined (ST_7015) || defined (ST_7020)
#ifdef ST_7015
    long base = (STI7015_BASE_ADDRESS+ST7015_DSPCFG_OFFSET);
#elif defined ST_7020
    long base = (STI7020_BASE_ADDRESS+ST7020_DSPCFG_OFFSET);
#endif
    U32 tab[7];
    int LVar;
    BOOL Prnt = 0;
    BOOL RetErr;
#define _DELAY_COMP    0x2B30  /* Delay compensation for analog video out */
#define _PIX_MAX       0x2B34  /* Nbr of pixel per line */
#define _LIN_MAX       0x2B38  /* Nbr of lines per field */
#define _SHAPE         0x2B3c  /* Modulation shape control */
#define _GAIN          0x2B40  /* Gain control */
#define _OSD_FACTOR    0x2B44  /* OSD gain factor */
#define _FILTER        0x2B48  /* SVM freq response selection for Video and OSD */
#define _EDGE_CNT      0x2B4c  /* Edge count for current frame */

    UNUSED_PARAMETER(result_sym_p);

    /* get print option */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1))
    {
        STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
        RetErr = TRUE;
    }
    else
    {
        Prnt = (BOOL)LVar;
    }

    STOS_InterruptLock();
    vout_read32( (void *)(base + _DELAY_COMP), &tab[0]);
    vout_read32( (void *)(base + _PIX_MAX), &tab[1]);
    vout_read32( (void *)(base + _LIN_MAX), &tab[2]);
    vout_read32( (void *)(base + _SHAPE), &tab[3]);
    vout_read32( (void *)(base + _GAIN), &tab[4]);
    vout_read32( (void *)(base + _OSD_FACTOR), &tab[5]);
    vout_read32( (void *)(base + _FILTER), &tab[6]);
    STOS_InterruptUnlock();

    if ( Prnt)
    {
        STTBX_Print(( ">>SVM Register :\n"));
        STTBX_Print(( "   DelayComp: 0x%08.8x, Shape : 0x%08.8x, Gain: 0x%08.8x\n", tab[0], tab[3], tab[4]));
        STTBX_Print(( "   OSDFactor: 0x%08.8x, Filter: 0x%08.8x\n", tab[5], tab[6]));
        STTBX_Print(( "   PixMax   : 0x%08.8x, LinMax: 0x%08.8x\n", tab[1], tab[2]));
    }

    STTST_AssignInteger( "SVMDELAY", (tab[0] & 0xF), FALSE);
    STTST_AssignInteger( "SVMSHAPE", (tab[3] & 0x3), FALSE);
    STTST_AssignInteger( "SVMGAIN" , (tab[4] & 0x1F), FALSE);
    STTST_AssignInteger( "SVMFACTO", (tab[5] & 0x3), FALSE);
    STTST_AssignInteger( "SVMFILTE", (tab[6] & 0x3), FALSE);

#else
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
#endif
    return(FALSE);
} /* end VOUT_ReadSVM */

#ifdef DVO_TESTS
/*-------------------------------------------------------------------------
 * Function : VOUT_ReadDVO_656
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_ReadDVO_656( STTST_Parse_t *pars_p, char *result_sym_p)
{
    int LVar;
    BOOL Prnt = 0;
    BOOL RetErr;
    U8 RegValue;
    U8* Add_p = (U8*)DIGOUT_BASE_ADDRESS + DVO_CTL;

    UNUSED_PARAMETER(result_sym_p);

    /* get print option */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1))
    {
        STTST_TagCurrentLine( pars_p, "expected print option (0=no, 1=ok)\n" );
        RetErr = TRUE;
    }
    else
    {
        Prnt = (BOOL)LVar;
    }

    STOS_InterruptLock();
    RegValue = STSYS_ReadRegDev8((void*)Add_p);
    STOS_InterruptUnlock();

    if ( Prnt)
    {
        STTBX_Print(( ">>DVO_656 : 0x%08.8x\n", RegValue));
    }

    STTST_AssignInteger( "DVO_656_E6M", (U8)(RegValue & 0x01), FALSE);
    return(FALSE);
} /* end VOUT_ReadDVO_656 */
#endif /* DVO_TESTS */


#ifdef VOUT_DEBUG
#define MAX_DEVICE  10
#define MAX_OPEN    3
/*-------------------------------------------------------------------------
 * Function : VOUT_StatusDriver
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_StatusDriver( STTST_Parse_t *pars_p, char *result_sym_p)
{
    stvout_Unit_t *PtrDbg = stvout_DbgPtr;
    stvout_Device_t *PtrDbgDev  = stvout_DbgDev;
    char DevTyp[20];
    char AccTyp[50];
    char OutTyp[80];
    U16 Tmp = 0;

    STTBX_Print(( ">> Initialised VOUT Driver :\n"));
    STTBX_Print(( "   =======================  \n"));
    for ( Tmp= 0; Tmp < MAX_DEVICE; PtrDbgDev++, Tmp++)
    {
        if ( PtrDbgDev->DeviceName[0] != '\0')
        {
            switch (PtrDbgDev->DeviceType)
            {

               case STVOUT_DEVICE_TYPE_DENC: /*no break*/
               case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
                    sprintf( DevTyp, "DENC      ");
                    sprintf( AccTyp, ">> DencName %s,\tDenc Handle %d,\t  DencVersion %d  ",
                            PtrDbgDev->DencName,
                            PtrDbgDev->DencHandle,
                            PtrDbgDev->DencVersion );
                    break;
               case STVOUT_DEVICE_TYPE_7015:
                    switch ( PtrDbgDev->OutputType) {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                        case STVOUT_OUTPUT_ANALOG_YUV :
                        case STVOUT_OUTPUT_ANALOG_YC :
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            sprintf( DevTyp, "7015 Denc ");
                            sprintf( AccTyp, ">> DencName %s,\tDenc Handle %d,\t  DencVersion %d  ",
                                    PtrDbgDev->DencName,
                                    PtrDbgDev->DencHandle,
                                    PtrDbgDev->DencVersion );
                            break;
                        default:
                            sprintf( DevTyp, "STi7015   ");
                            sprintf( AccTyp, ">> BaseAddress 0x%x,\tDeviceBaseAddress 0x%x  ",
                                    (int)PtrDbgDev->BaseAddress_p,
                                    (int)PtrDbgDev->DeviceBaseAddress_p);
                            break;
                    }
                case STVOUT_DEVICE_TYPE_7020: /* no break */
                case STVOUT_DEVICE_TYPE_5528: /* no break */
                case STVOUT_DEVICE_TYPE_V13 : /* no break */
                case STVOUT_DEVICE_TYPE_5525 :
                    switch ( PtrDbgDev->OutputType) {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                        case STVOUT_OUTPUT_ANALOG_YUV :
                        case STVOUT_OUTPUT_ANALOG_YC :
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            if (PtrDbgDev->DeviceType == STVOUT_DEVICE_TYPE_7020)
                            {
                                sprintf( DevTyp, "7020 Denc ");
                            }
                            elif (PtrDbgDev->DeviceType == STVOUT_DEVICE_TYPE_V13)
                            {
                                #ifdef ST_5100
                                sprintf( DevTyp, "5100 Denc ");
                                #elif ST_5301
                                sprintf( DevTyp, "5301 Denc ");
                                #elif ST_5188
                                sprintf( DevTyp, "5188 Denc ");
                                #elif ST_5525
                                sprintf( DevTyp, "5525 Denc ");
                                #else
                                sprintf( DevTyp, "5105 Denc ");
                                #endif
                            }
                            else /* 5528 */
                            {
                              sprintf( DevTyp, "5528 Denc ");
                            }
                            sprintf( AccTyp, ">> DencName %s,\tDenc Handle %d,\t  DencVersion %d  ",
                                    PtrDbgDev->DencName,
                                    PtrDbgDev->DencHandle,
                                    PtrDbgDev->DencVersion );
                            /* no break */
                        default:
                            sprintf( AccTyp, ">> BaseAddress 0x%x,\tDeviceBaseAddress 0x%x  ",
                                    (int)PtrDbgDev->BaseAddress_p,
                                    (int)PtrDbgDev->DeviceBaseAddress_p);
                            break;
                    }
                    break;
                case STVOUT_DEVICE_TYPE_7710 : /* no break */
                case STVOUT_DEVICE_TYPE_7100 : /* no break */
                case STVOUT_DEVICE_TYPE_7200 :
                      switch (PtrDbgDev->OutputType)
                      {
                          case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                          case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                          case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                          case STVOUT_OUTPUT_ANALOG_CVBS :
                                 sprintf( DevTyp, "7710 Denc ");
                                 sprintf( AccTyp, ">> DencName %s,\tDenc Handle %d,\t  DencVersion %d  ",
                                  PtrDbgDev->DencName,
                                  PtrDbgDev->DencHandle,
                                  PtrDbgDev->DencVersion );
                                break;
                          case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                          case STVOUT_OUTPUT_HD_ANALOG_YUV :  /*no break*/
                          case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /*no break*/
                          case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                          case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                                sprintf( AccTyp, ">> BaseAddress 0x%x,\tDeviceBaseAddress 0x%x  ",
                                    (int)PtrDbgDev->BaseAddress_p,
                                    (int)PtrDbgDev->DeviceBaseAddress_p);
                               break;
                          case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                                sprintf( AccTyp, ">> BaseAddress 0x%x,\tDeviceBaseAddress 0x%x, \tSecondDeviceBaseAddress 0x%x",
                                    (int)PtrDbgDev->BaseAddress_p,
                                    (int)PtrDbgDev->DeviceBaseAddress_p,
                                    (int)PtrDbgDev->SecondBaseAddress_p );
                               break;
                          default :
                               break;
                          }
                    break;
                 case STVOUT_DEVICE_TYPE_GX1 :
                    break;
            }

            STTBX_Print((">> DeviceType    %s,\t  DeviceName '%s' \n",
                        DevTyp,
                        PtrDbgDev->DeviceName ));)
            STTBX_Print((">> CPUPartition  0x%x,\t  MaxOpen %d \n",
                        PtrDbgDev->CPUPartition_p,
                        PtrDbgDev->MaxOpen ));
            STTBX_Print(( "%s\n", AccTyp ));

            switch (PtrDbgDev->OutputType) {
                case STVOUT_OUTPUT_ANALOG_RGB :
                    sprintf( OutTyp, "OutputType    ANALOG_RGB \n");
                    break;
                case STVOUT_OUTPUT_ANALOG_YUV :
                    sprintf( OutTyp, "OutputType    ANALOG_YUV \n");
                    break;
                case STVOUT_OUTPUT_ANALOG_YC :
                    sprintf( OutTyp, "OutputType    ANALOG_YC \n");
                    break;
                case STVOUT_OUTPUT_ANALOG_CVBS :
                    sprintf( OutTyp, "OutputType    ANALOG_CVBS \n");
                    break;
                case STVOUT_OUTPUT_ANALOG_SVM :
                    sprintf( OutTyp, "OutputType    ANALOG_SVM \n");
                    break;
                case STVOUT_OUTPUT_HD_ANALOG_RGB :
                    sprintf( OutTyp, "OutputType    HD_ANALOG_RGB \n");
                    break;
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
                    sprintf( OutTyp, "OutputType    HD_ANALOG_YUV \n");
                    break;
                case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
                    sprintf( OutTyp, "OutputType    DIGITAL_YCBCR444_24BITSCOMPONENTS \n");
                    break;
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                    sprintf( OutTyp, "OutputType    DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED \n");
                    break;
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                    sprintf( OutTyp, "OutputType    DIGITAL_YCBCR422_8BITSMULTIPLEXED \n");
                    break;
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                    sprintf( OutTyp, "OutputType    DIGITAL_RGB888_24BITSCOMPONENTS \n");
                    break;
            }
            STTBX_Print((">> %s \n", OutTyp ));
        }
    }

    STTBX_Print(( ">> Opened VOUT Driver :\n"));
    STTBX_Print(( "   ==================  \n"));
    for ( Tmp= 0; Tmp < (MAX_DEVICE * MAX_OPEN); PtrDbg++, Tmp++)
    {
        if ( PtrDbg->Device_p != NULL)
        {
            sprintf( VOUT_Msg, ">> DeviceName '%s'\t UnitValidity %d\t status %s\t   Handle %d\n",
                            PtrDbg->Device_p->DeviceName,
                            PtrDbg->UnitValidity,
                            ( stvout_DbgDev->VoutStatus )? " TRUE" : " FALSE",
                            (int)PtrDbg);
            STTBX_Print(( VOUT_Msg ));
        }
    }

   return(FALSE);
} /* end VOUT_StatusDriver */


/*-------------------------------------------------------------------------
 * Function : VOUT_AllTermInitDriver
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_AllTermInitDriver( STTST_Parse_t *pars_p, char *result_sym_p)
{
  stvout_Device_t *PtrDbgDev  = stvout_DbgDev;
  STVOUT_TermParams_t VOUTTermParams;
  char DeviceName[80];
  U16 Tmp = 0;
  BOOL RetErr;

    STTBX_Print(( ">> Terminate all VOUT Driver :\n"));
    for ( Tmp= 0; Tmp < MAX_DEVICE; PtrDbgDev++, Tmp++)
    {
        if ( PtrDbgDev->DeviceName[0] != '\0')
        {
            /* get device name */
            sprintf( DeviceName , PtrDbgDev->DeviceName);

            /* Terminate the VOUT with Force Terminaison mode */
            VOUTTermParams.ForceTerminate = TRUE;

            if (STVOUT_Term(DeviceName, &VOUTTermParams) != ST_NO_ERROR)
            {
                STTBX_Print(( "STVOUT_Term  %s Failed !\n", DeviceName));
                RetErr = TRUE;
            }
            else
            {
                STTBX_Print(( "STVOUT_Term  %s Done\n", DeviceName));
                RetErr = FALSE;
            }
        }
    }
    return(FALSE);
} /* end VOUT_AllTermInitDriver */


static BOOL DENC_Version( STTST_Parse_t *pars_p, char *result_sym_p)
{
    STTBX_Print(( "Denc Version = %d\n", halvout_DencVersionOnChip(DevTyp)));
    return(FALSE);
} /* end DENC_Version */
#endif

#if defined (ST_7100) || defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : VOUT_GamSet
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GamSet( STTST_Parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    S32 Mix_Sel;
    U32 CutId;

    CutId = ST_GetCutRevision();
    UNUSED_PARAMETER(result_sym_p);

    ErrorCode = STTST_GetInteger(pars_p, 1, &Mix_Sel);
    if (ErrorCode)
    {
        STTST_TagCurrentLine( pars_p, "Mix_Sel(1/2)" );
        return(TRUE);
    }

   if(Mix_Sel == 2)
   {
    STSYS_WriteRegDev32LE((U32)(VMIX2_BASE_ADDRESS + 0x0), 0x4);
    STSYS_WriteRegDev32LE((U32)(VMIX2_BASE_ADDRESS + 0x28), 0x29007f);
    STSYS_WriteRegDev32LE((U32)(VMIX2_BASE_ADDRESS + 0x2c), 0x208034e);
    #if defined (ST_7109)
        if(CutId >=0xC0 )
        {
            STSYS_WriteRegDev32LE((U32)(VMIX2_BASE_ADDRESS + 0x34), 0x2);/*for cut 3*/
        }
    #endif

    STSYS_WriteRegDev32LE((U32)((U32)VOS_BASE_ADDRESS_1 + (U32)0x70), 0x0);
   }
   else
   {
    STSYS_WriteRegDev32LE((U32)(VMIX1_BASE_ADDRESS + 0x0), 0x3);
    STSYS_WriteRegDev32LE((U32)(VMIX1_BASE_ADDRESS + 0x28), 0x300089);
    STSYS_WriteRegDev32LE((U32)(VMIX1_BASE_ADDRESS + 0x2c), 0x26f0358);
    STSYS_WriteRegDev32LE((U32)(VMIX1_BASE_ADDRESS + 0x34), 0x40);
    STSYS_WriteRegDev32LE((U32)((U32)VOS_BASE_ADDRESS_1 + 0x70), 0x1);
    }
    return ( API_EnableError ? ErrorCode : FALSE );
} /* end VOUT_GamSet   */
#endif /* #ifdef ST_7100 */
#if defined (ST_5107)|| defined(ST_5162)
/*-------------------------------------------------------------------------
 * Function : VOUT_PIOSet
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_PIOSet( STTST_Parse_t *pars_p, char *result_sym_p )
{
    /*configure PIO1*/

    U32         PIOBaseAddress;
    U8          BitNumber;
    U32         AlternateFun;
    U32         X_mask;


    PIOBaseAddress = PIO_1_BASE_ADDRESS;

    for(BitNumber=0; BitNumber <=7 ; BitNumber++)
    {
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0), (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1), (0x1 << BitNumber));
        STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2), (0x1 << BitNumber));

        AlternateFun = 0x0001;
        AlternateFun = AlternateFun << BitNumber;
        X_mask = STSYS_ReadRegDev32LE((void*)(INT_CONFIG_CONTROL_F));
        X_mask &= ~(1<<(BitNumber+8));
        AlternateFun = AlternateFun << 8;
        X_mask |= AlternateFun;
        STSYS_WriteRegDev32LE((void*)(INT_CONFIG_CONTROL_F), X_mask);
    }

    /*configure PIO3*/
    PIOBaseAddress = PIO_3_BASE_ADDRESS;
    /**/
    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0), (0x1 << 1));
    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1), (0x1 << 1));
    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2), (0x1 << 1));

    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_CLEAR_PnC0), (0x1 << 4));
    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC1), (0x1 << 4));
    STSYS_WriteRegDev8((void*)(PIOBaseAddress+PIO_SET_PnC2), (0x1 << 4));

   STSYS_WriteRegDev32LE((void*)(INT_CONFIG_CONTROL_G),  0x120000);

    return(FALSE);

}
#endif


/*-------------------------------------------------------------------------
 * Function : VOUT_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VOUT_InitCommand(void)
{
    BOOL RetErr = FALSE;

/* API functions : */
    RetErr  = STTST_RegisterCommand( "VOUT_Capa",      VOUT_GetCapability, "VOUT Get Capability ('DevName')");
    RetErr |= STTST_RegisterCommand( "VOUT_Close",     VOUT_Close, "VOUT Close ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_Disable",   VOUT_Disable, "Disable VOUT ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_Enable",    VOUT_Enable, "Enable VOUT ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_GetOption", VOUT_GetOption, "VOUT Get Options ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_GetParams", VOUT_GetParams, "VOUT Get Parameters ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_Init",      VOUT_Init, "VOUT Init ('DevName' 'DevType' 'OutType' 'MaxOpen')");
    RetErr |= STTST_RegisterCommand( "VOUT_Open",      VOUT_Open, "VOUT Open ('DevName')");
    RetErr |= STTST_RegisterCommand( "VOUT_Revision",  VOUT_GetRevision, "VOUT Get Revision");
    RetErr |= STTST_RegisterCommand( "VOUT_SetOption", VOUT_SetOption, "VOUT Set Options ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetParams", VOUT_SetParams, "VOUT Set Parameters by default ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_Term",      VOUT_Term, "VOUT Term ('DevName' 'ForceTeminate')");

    RetErr |= STTST_RegisterCommand( "VOUT_SetDac",    VOUT_SetAnalogDacParams, "Adjust Dac Level ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetBCS",    VOUT_SetAnalogBCSParams, "Adjust Brightness Contrast and Saturation ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetChrLumF",VOUT_SetAnalogChrLumFilterParams, "Set Chroma and Luma filter coefficients");
    RetErr |= STTST_RegisterCommand( "VOUT_SetEmb",    VOUT_SetEmbeddedSyncro, "Select Embedded/Syncro/Color Parameter ('Handle' 'Embedded' 'SyncroInChroma' 'ColorSapce')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetInv",    VOUT_SetAnalogInverted, "Select Inverted Parameter ('Handle' 'Inverted')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetSvm",    VOUT_SetAnalogSVM, "Select SVM Parameter ('Handle' 'Parameters')");
    RetErr |= STTST_RegisterCommand( "VOUT_AdjustCLdelay",    VOUT_AdjustChromaLumaDelay, "Adjust Chroma Luma Delay ('Handle' 'CL delay')");


#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710)|| defined (ST_7100)|| defined (ST_5301)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    RetErr |= STTST_RegisterCommand( "VOUTSetDenc",    VOUT_SetDencAuxMain, "Select Aux/Main output of the compositor");
#endif
#if defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    RetErr |= STTST_RegisterCommand( "VOUT_GetState",  VOUT_GetState, "VOUT Get State ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_GetTarInfo",VOUT_GetTargetInfo, "VOUT Get Target Info ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_SendData", VOUT_SendData, "VOUT Send Data ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_HDMISetOut", VOUT_HDMI_SetOutputType, "VOUT HDMI Set Output Type ('Handle','OUTPUTTYPE')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetAv",    VOUT_SetActiveVideo , "VOUT Set Active windows parameters ('Handle','Xmin','Xmax', 'Ymin','Ymax')");
#if defined (STVOUT_HDCP_PROTECTION)
    RetErr |= STTST_RegisterCommand( "VOUT_SetHdcpParams",    VOUT_SetHDCPParams , "VOUT Set HDCP parameters by default('Handle','IV0','IV1', 'KSV0','KSV1')");
    RetErr |= STTST_RegisterCommand( "VOUT_GetHdcpSnkParams", VOUT_GetHDCPSinkParams , "VOUT Get HDCP parameters ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_SetForbiddenKSVs", VOUT_SetForbiddenKSVs , "VOUT set the list of forbiden KSVs('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_DisDefaultOut", VOUT_DisableDefaultOutput , "VOUT disable the default screen output ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_EnDefaultOut", VOUT_EnableDefaultOutput , "VOUT disable the default screen output ('Handle','Ch0','Ch1','Ch2')");
#endif

#ifdef STVOUT_CEC_PIO_COMPARE
    RetErr |= STTST_RegisterCommand( "VOUT_CECSendMessage", VOUT_CECSendMessage , "Send on the HDMI CEC bus ('Handle','Dest Address')");
    RetErr |= STTST_RegisterCommand( "VOUT_CECPhyAdd", VOUT_CECPhysicalAddressAvailable , "Notify the driver that the physical address is obtained ('Handle')");
    RetErr |= STTST_RegisterCommand( "VOUT_CECAddAdr", VOUT_CECSetAdditionalAddress , "Set another address to device ('Handle','Role' )");

#endif

    RetErr |= STTST_RegisterCommand( "VOUT_Start",    VOUT_Start , "VOUT Start the state machine ('Handle')");
#endif
/* utilities : */
    RetErr |= STTST_RegisterCommand( "VOUTrdDac",      VOUT_ReadDac, "Read Dac Level register 'device type' 'print'");
    RetErr |= STTST_RegisterCommand( "VOUTrdBCS",      VOUT_ReadBCS, "Read Control Display register 'device type' 'print'");
    RetErr |= STTST_RegisterCommand( "VOUTrdChroma",   VOUT_ReadChroma, "Read Chroma registers 'device type' 'print'");
    RetErr |= STTST_RegisterCommand( "VOUTrdLuma",     VOUT_ReadLuma, "Read Luma registers 'device type' 'print'");
    RetErr |= STTST_RegisterCommand( "VOUTrdDSPCFG",   VOUT_ReadDSPCFG, "Read 7015/20 Display Output Configuration register 'print'");
    RetErr |= STTST_RegisterCommand( "VOUTrdSVM",      VOUT_ReadSVM, "Read 7015/20 SVM  Configuration register 'print'");

#if defined (ST_7100) || defined (ST_7109)
 RetErr |= STTST_RegisterCommand( "VOUT_GamSet",  VOUT_GamSet, "VOUT_GamSet");
 #endif

#ifdef DVO_TESTS
    RetErr |= STTST_RegisterCommand( "VOUTrdDVO_656",  VOUT_ReadDVO_656, "Read Digital Output CCIR 601/656 Switch 'print'");
#endif

#if defined (ST_5107)
    RetErr |= STTST_RegisterCommand( "VOUT_PIOSet",  VOUT_PIOSet, "configure PIO1 for DVO on 5107");
#endif

#if defined (ST_5162)
    RetErr |= STTST_RegisterCommand( "VOUT_PIOSet",  VOUT_PIOSet, "configure PIO1 for DVO on 5162");
#endif

#ifdef VOUT_DEBUG
    RetErr |= STTST_RegisterCommand( "VOUTStatusDrv",  VOUT_StatusDriver, "VOUT Status driver");
    RetErr |= STTST_RegisterCommand( "VOUTAllTerm",    VOUT_AllTermInitDriver, "Terminate all init driver");
    RetErr |= STTST_RegisterCommand( "DENCVer",        DENC_Version, "Read Denc version !");
#endif

#ifdef ST_OSLINUX
  RetErr |= STTST_AssignString ("DRV_PATH_VOUT", "vout/", TRUE);
#else
  RetErr |= STTST_AssignString ("DRV_PATH_VOUT", "", TRUE);
#endif

    RetErr |= STTST_AssignInteger ("ERR_DENC_ACCESS",        STVOUT_ERROR_DENC_ACCESS       , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VOUT_NOT_AVAILABLE", STVOUT_ERROR_VOUT_NOT_AVAILABLE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VOUT_NOT_ENABLE",    STVOUT_ERROR_VOUT_NOT_ENABLE   , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VOUT_ENABLE",        STVOUT_ERROR_VOUT_ENABLE       , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VOUT_INCOMPATIBLE",  STVOUT_ERROR_VOUT_INCOMPATIBLE , TRUE);

    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_DENC",  STVOUT_DEVICE_TYPE_DENC + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_7015",  STVOUT_DEVICE_TYPE_7015 + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_7020",  STVOUT_DEVICE_TYPE_7020 + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_GX1",   STVOUT_DEVICE_TYPE_GX1 + 1,            TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_DIGOUT",STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT + 1, TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_5528",  STVOUT_DEVICE_TYPE_5528 + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_V13" ,  STVOUT_DEVICE_TYPE_V13 + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_AUTO",  VOUT_DEVICE_TYPE + 1,                  TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_4629",  STVOUT_DEVICE_TYPE_4629 + 1,           TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_DENC_ENHANCED",  STVOUT_DEVICE_TYPE_DENC_ENHANCED + 1,TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_7710",  STVOUT_DEVICE_TYPE_7710 + 1,TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_7100",  STVOUT_DEVICE_TYPE_7100 + 1,TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_5525",  STVOUT_DEVICE_TYPE_5525 + 1,TRUE);
    RetErr |= STTST_AssignInteger ("VOUT_DEVICE_TYPE_7200",  STVOUT_DEVICE_TYPE_7200 + 1,TRUE);

    RetErr |= STTST_AssignInteger ("ANALOG_RGB",             shift(STVOUT_OUTPUT_ANALOG_RGB) , TRUE);
    RetErr |= STTST_AssignInteger ("ANALOG_YUV",             shift(STVOUT_OUTPUT_ANALOG_YUV) , TRUE);
    RetErr |= STTST_AssignInteger ("ANALOG_YC",              shift(STVOUT_OUTPUT_ANALOG_YC)  , TRUE);
    RetErr |= STTST_AssignInteger ("ANALOG_CVBS",            shift(STVOUT_OUTPUT_ANALOG_CVBS), TRUE);
    RetErr |= STTST_AssignInteger ("ANALOG_SVM",             shift(STVOUT_OUTPUT_ANALOG_SVM) , TRUE);
    RetErr |= STTST_AssignInteger ("DIGITAL_YC444_24",       shift(STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS),        TRUE);
    RetErr |= STTST_AssignInteger ("DIGITAL_YC422_16",       shift(STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED), TRUE);
    RetErr |= STTST_AssignInteger ("DIGITAL_YC422_8",        shift(STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED),        TRUE);
    RetErr |= STTST_AssignInteger ("DIGITAL_RGB888_24",      shift(STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS),          TRUE);
    RetErr |= STTST_AssignInteger ("HD_ANALOG_RGB",          shift(STVOUT_OUTPUT_HD_ANALOG_RGB) , TRUE);
    RetErr |= STTST_AssignInteger ("HD_ANALOG_YUV",          shift(STVOUT_OUTPUT_HD_ANALOG_YUV) , TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_RGB888",            shift(STVOUT_OUTPUT_DIGITAL_HDMI_RGB888) ,   TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_YCBCR444",          shift(STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444) , TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_YCBCR422",          shift(STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422) , TRUE);

    RetErr |= STTST_AssignInteger ("GENERAL_AUX_NOT_MAIN",   STVOUT_OPTION_GENERAL_AUX_NOT_MAIN, TRUE);
    RetErr |= STTST_AssignInteger ("NOTCH_FILTER_ON_LUMA",   STVOUT_OPTION_NOTCH_FILTER_ON_LUMA, TRUE);
    RetErr |= STTST_AssignInteger ("RGB_SATURATION",         STVOUT_OPTION_RGB_SATURATION,       TRUE);
    RetErr |= STTST_AssignInteger ("IF_DELAY",               STVOUT_OPTION_IF_DELAY,             TRUE);


    RetErr |= STTST_AssignInteger ("UPS_SD" ,   STVOUT_SD_MODE + 1,  TRUE);
    RetErr |= STTST_AssignInteger ("UPS_ED" ,   STVOUT_ED_MODE + 1,  TRUE);
    RetErr |= STTST_AssignInteger ("UPS_HD" ,   STVOUT_HD_MODE + 1,  TRUE);
    RetErr |= STTST_AssignInteger ("UPS_HDRQ" , STVOUT_HDRQ_MODE + 1,TRUE);


    RetErr |= STTST_AssignString  ("VOUT_NAME", STVOUT_DEVICE_NAME, FALSE);

    return(!RetErr);
} /* end VOUT_InitCommand */



/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VOUT_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VOUT_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "VOUT_RegisterCmd()     \t: ok           %s\n", STVOUT_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "VOUT_RegisterCmd()     \t: failed !\n" ));
    }

    return(RetOk);
}
/* end of file */



