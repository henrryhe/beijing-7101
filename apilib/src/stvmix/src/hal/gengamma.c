/*******************************************************************************

File name   : gengamma.c

Description : Generic gamma video mixer driver module file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
27 March 2001      Created                                          BS
13 June  2003      Updated for 5528 generic gamma                   HSdLM
22 Sep   2003      Add support for OS21                             MH
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "gengamma.h"

#include "sttbx.h"
#include "stsys.h"

#include "stdevice.h"

#include "stvtg.h"
#include "stvout.h"

#include "stevt.h"

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define UNUSED(x) (void)(x)


/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif

/* Workaround management ---------------------------------------------------- */
#ifdef WA_GNBvd16602
#ifndef STVMIX_GENGAMMA_7020
#undef WA_GNBvd16602
#endif /* STVMIX_GENGAMMA_7020 */
#endif /* WA_GNBvd16602 */


/* Private Types ------------------------------------------------------------ */

typedef enum
{
    GENGAM_CRB_NOTHING = 0,
    GENGAM_CRB_VID1    = 1,
    GENGAM_CRB_VID2    = 2,
    GENGAM_CRB_GDP1    = 3,
    GENGAM_CRB_GDP2    = 4,
    GENGAM_CRB_GDP3    = 5,
    GENGAM_CRB_GDP4    = 6,
    GENGAM_CRB_GDP5    = 7
   /* TODO: Not yet defined
    GENGAM_CRB_SPD = ?
  */
} stvmix_GenGamma_Input_CRB_e;

typedef enum
{
    GENGAM_TYPE_NOTHING  = 0x000,
    GENGAM_TYPE_BKC      = 0x001,
    GENGAM_TYPE_VID1     = 0x002,
    GENGAM_TYPE_VID2     = 0x004,
    GENGAM_TYPE_GDP1     = 0x008,
    GENGAM_TYPE_GDP2     = 0x010,
    GENGAM_TYPE_GDP3     = 0x020,
    GENGAM_TYPE_GDP4     = 0x040,
    GENGAM_TYPE_GDP5     = 0x080,
    GENGAM_TYPE_ALPHA    = 0x100,
    GENGAM_TYPE_SPD      = 0x200,
    GENGAM_TYPE_CUR      = 0x400,
    GENGAM_TYPE_GDPVBI1  = 0x800,
    GENGAM_TYPE_GDPVBI2  = 0x1000,
    GENGAM_TYPE_VID3     = 0x2000,
    GENGAM_TYPE_RESERVED = 0x4000
} stvmix_GenGamma_Input_Type_e;

typedef struct
{
    STLAYER_Layer_t              LayerType;
    U16                          BBA;
    stvmix_GenGamma_Input_Type_e InputType;
    stvmix_GenGamma_Input_CRB_e  CRBInput;
    U32                          CTLACTEnable;
    BOOL                         ACTEnable;
} stvmix_GenGamma_Block_t;

typedef struct
{
    /* Register to write when VSync occurs */
    U32 RegCTL;
    U32 RegCRB;
    U32 RegACT;

    /* For VSync event registration */
    STEVT_Handle_t EvtHandle;
    BOOL VSyncToProcess;
    BOOL VSyncRegistered;
    semaphore_t * VSyncProcessed_p;
    STVTG_VSYNC_t WriteEventType;
    BOOL TestVsync;
    /* Generic compositor config */
    U32 BlockNb;
    const stvmix_GenGamma_Input_Type_e * GenGammaDepth_p;
    const stvmix_GenGamma_Block_t ** GenGammaBlock_p;
} stvmix_HalGenGamma_t;

/****************************/
/* Register address for ALL */
/****************************/
#define GENGAM_MIX_CTL                 0x00
#define GENGAM_MIX_CRB                 0x34
#define GENGAM_MIX_ACT                 0x38
#define GENGAM_MIX_BKC                 0x04
#define GENGAM_MIX_BCO                 0x0C
#define GENGAM_MIX_BCS                 0x10
#define GENGAM_MIX_AVO                 0x28
#define GENGAM_MIX_AVS                 0x2C

#define GENGAM_MIX_BBA_CURSOR          0x000
#define GENGAM_MIX_BBA_GDP1            0x100
#define GENGAM_MIX_BBA_GDP2            0x200
#define GENGAM_MIX_BBA_GDP3            0x300
#define GENGAM_MIX_BBA_GDP4            0x400
#define GENGAM_MIX_BBA_GDP5            0x500
#define GENGAM_MIX_BBA_ALPHA           0x600
#define GENGAM_MIX_BBA_VID1            0x700
#define GENGAM_MIX_BBA_VID2            0x800
#define GENGAM_MIX_BBA_SPD             0x900
#define GENGAM_MIX_BBA_MIX1            0xC00
#define GENGAM_MIX_BBA_MIX2            0xD00
#define GENGAM_MIX_BBA_MASK            0xFFF


/*****************************/
/* Register contents for ALL */
/*****************************/
#define GENGAM_CTL_ACT_BKC_ENABLE      0x00000001
#define GENGAM_CTL_ACT_VID1_ENABLE     0x00000002
#define GENGAM_CTL_ACT_VID2_ENABLE     0x00000004 /*0x00000004*/
#define GENGAM_CTL_ACT_GDP1_ENABLE     0x00000008
#define GENGAM_CTL_ACT_GDP2_ENABLE     0x00000010
#define GENGAM_CTL_ACT_GDP3_ENABLE     0x00000020 /*0x00000020*/
#define GENGAM_CTL_ACT_GDP4_ENABLE     0x00000040
#define GENGAM_CTL_ACT_GDP5_ENABLE     0x00000080
#define GENGAM_CTL_ACT_SPD_ENABLE      0x00000100
#define GENGAM_CTL_ACT_CUR_ENABLE      0x00000200
#define GENGAM_CTL_ACT_ALPHA_ENABLE    0x00008000
#define GENGAM_CTL_VID1_NOTVID2_SEL    0x00100000
#define GENGAM_CTL_709NOT601           0x02000000
#define GENGAM_CTL_CFORM               0x04000000
#define GENGAM_AV_BC_MASK              0xF800F000

/**************************/
/* Configuration for ALL  */
/**************************/
/* Depth 0 is background : Position is fixed */
/* Depth 1->7 : Position is chip dependant   */
/* Depth 8 is cursor : Position is fixed     */
#define GENGAM_BKC_DEPTH  0
#define GENGAM_MAX_DEPTH  8

#define GENGAM_BLOCK_MIXER1    0
#define GENGAM_BLOCK_MIXER2    1
#define GENGAM_BLOCK_MIXER3    2

#define GENGAM_TWO_MIXER_BLOCK 2

#ifdef STVMIX_GENGAMMA_7200

#define GENGAM_THREE_MIXER_BLOCK 3
#define GENGAM_MIX_COMPO_MASK          0xF000
#define GENGAM_MIX_BBA_LOCAL_COMPO    0x1000
#define GENGAM_MIX_BBA_REMOTE_COMPO    0x2000


#endif




/* This structure allows variation parameters between chips if necessary.  */
/* One table should define the common configuration for the general compo. */
/* Important: Following enum should match with GenGammaBlock table */
typedef enum
{
    GENGAM_BLOCK_CURSOR = 0,
    GENGAM_BLOCK_GDP1,
    GENGAM_BLOCK_GDP2,
    GENGAM_BLOCK_GDP3,
    GENGAM_BLOCK_GDP4,
    GENGAM_BLOCK_GDP5,
    GENGAM_BLOCK_ALPHA,
    GENGAM_BLOCK_VID1,
    GENGAM_BLOCK_VID2,
    GENGAM_BLOCK_VID1_SDDISPO2,
    GENGAM_BLOCK_VID2_SDDISPO2,
    GENGAM_BLOCK_VID1_HDDISPO2,
    GENGAM_BLOCK_VID2_HDDISPO2,
    GENGAM_BLOCK_VID1_DISPLAYPIPE,
#ifdef STVMIX_GENGAMMA_7200
    GENGAM_BLOCK_VID2_DISPLAYPIPE,
    GENGAM_BLOCK_VID3_DISPLAYPIPE,
    GENGAM_BLOCK_VID4_DISPLAYPIPE,
#endif
    GENGAM_BLOCK_FILTER,
    GENGAM_BLOCK_GDPVBI1,
    GENGAM_BLOCK_GDPVBI2
  /* TODO: Not yet defined
    GENGAM_BLOCK_SPD = ?
  */
} stvmix_GenGamma_Block_e;

static const stvmix_GenGamma_Block_t GenGammaBlock[] =
{
    { STLAYER_GAMMA_CURSOR,  GENGAM_MIX_BBA_CURSOR, GENGAM_TYPE_CUR,   GENGAM_CRB_NOTHING, GENGAM_CTL_ACT_CUR_ENABLE,   TRUE  },
    { STLAYER_GAMMA_GDP,     GENGAM_MIX_BBA_GDP1,   GENGAM_TYPE_GDP1,  GENGAM_CRB_GDP1,    GENGAM_CTL_ACT_GDP1_ENABLE,  TRUE  },
    { STLAYER_GAMMA_GDP,     GENGAM_MIX_BBA_GDP2,   GENGAM_TYPE_GDP2,  GENGAM_CRB_GDP2,    GENGAM_CTL_ACT_GDP2_ENABLE,  TRUE  },
    { STLAYER_GAMMA_GDP,     GENGAM_MIX_BBA_GDP3,   GENGAM_TYPE_GDP3,  GENGAM_CRB_GDP3,    GENGAM_CTL_ACT_GDP3_ENABLE,  TRUE  },
    { STLAYER_GAMMA_GDP,     GENGAM_MIX_BBA_GDP4,   GENGAM_TYPE_GDP4,  GENGAM_CRB_GDP4,    GENGAM_CTL_ACT_GDP4_ENABLE,  TRUE  },
    { STLAYER_GAMMA_GDP,     GENGAM_MIX_BBA_GDP5,   GENGAM_TYPE_GDP5,  GENGAM_CRB_GDP5,    GENGAM_CTL_ACT_GDP5_ENABLE,  FALSE },
    { STLAYER_GAMMA_ALPHA,   GENGAM_MIX_BBA_ALPHA,  GENGAM_TYPE_ALPHA, GENGAM_CRB_NOTHING, GENGAM_CTL_ACT_ALPHA_ENABLE, FALSE },
    { STLAYER_7020_VIDEO1,   GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID1,  GENGAM_CRB_VID1,    GENGAM_CTL_ACT_VID1_ENABLE,  TRUE  },
    { STLAYER_7020_VIDEO2,   GENGAM_MIX_BBA_VID2,   GENGAM_TYPE_VID2,  GENGAM_CRB_VID2,    GENGAM_CTL_ACT_VID2_ENABLE,  TRUE  },
    { STLAYER_SDDISPO2_VIDEO1, GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID1,  GENGAM_CRB_VID1,    GENGAM_CTL_ACT_VID1_ENABLE,  TRUE },
    { STLAYER_SDDISPO2_VIDEO2, GENGAM_MIX_BBA_VID2,   GENGAM_TYPE_VID2,  GENGAM_CRB_VID2,    GENGAM_CTL_ACT_VID2_ENABLE,  TRUE },
    { STLAYER_HDDISPO2_VIDEO1, GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID1,  GENGAM_CRB_VID1,    GENGAM_CTL_ACT_VID1_ENABLE,  TRUE },
    { STLAYER_HDDISPO2_VIDEO2, GENGAM_MIX_BBA_VID2,   GENGAM_TYPE_VID2,  GENGAM_CRB_VID2,    GENGAM_CTL_ACT_VID2_ENABLE,  TRUE },
    { STLAYER_DISPLAYPIPE_VIDEO1, GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID1,  GENGAM_CRB_VID1,    GENGAM_CTL_ACT_VID1_ENABLE,  TRUE },
#ifdef STVMIX_GENGAMMA_7200
    { STLAYER_DISPLAYPIPE_VIDEO2, GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID1,  GENGAM_CRB_VID1, GENGAM_CTL_ACT_VID1_ENABLE,  TRUE },
    { STLAYER_DISPLAYPIPE_VIDEO3, GENGAM_MIX_BBA_VID2,   GENGAM_TYPE_VID2,  GENGAM_CRB_VID2, GENGAM_CTL_ACT_VID2_ENABLE,  TRUE },
    { STLAYER_DISPLAYPIPE_VIDEO4, GENGAM_MIX_BBA_VID1,   GENGAM_TYPE_VID3,  GENGAM_CRB_VID1, GENGAM_CTL_ACT_VID1_ENABLE,  TRUE },

#endif
 #ifndef STVMIX_GENGAMMA_5528
    { STLAYER_GAMMA_FILTER,  GENGAM_MIX_BBA_GDP3,   GENGAM_TYPE_GDP3,  GENGAM_CRB_GDP3,    GENGAM_CTL_ACT_GDP3_ENABLE,  TRUE  },
 #else  /* 7020  */
    { STLAYER_GAMMA_FILTER,  GENGAM_MIX_BBA_GDP4,   GENGAM_TYPE_GDP4,  GENGAM_CRB_GDP4,    GENGAM_CTL_ACT_GDP4_ENABLE,  TRUE  },
 #endif
   { STLAYER_GAMMA_GDPVBI,     GENGAM_MIX_BBA_GDP1,   GENGAM_TYPE_GDPVBI1,  GENGAM_CRB_GDP1,    GENGAM_CTL_ACT_GDP1_ENABLE,  TRUE  },
   { STLAYER_GAMMA_GDPVBI,     GENGAM_MIX_BBA_GDP2,   GENGAM_TYPE_GDPVBI2,  GENGAM_CRB_GDP2,    GENGAM_CTL_ACT_GDP2_ENABLE,  TRUE  }

    /* TODO: Not yet defined
    { STLAYER_GAMMA_SPD ??,   GENGAM_MIX_BBA_SPD,  GENGAM_TYPE_SPD,   GENGAM_CRB_SPD,     GENGAM_CTL_ACT_SPD_ENABLE,   TRUE }
    */
};

#ifdef STVMIX_GENGAMMA_7020
/**************************/
/* Configuration for 7020 */
/**************************/
/* Registers */
#define GENGAM_7020_DSPCFG_OFFSET   0x00006100
#define GENGAM_7020_DSP_CFG_DSD     0x00000002
#define GENGAM_7020_DSP_CFG_DESL    0x00000004
#define GENGAM_7020_DSP_CFG_NUP     0x00000010
#define GENGAM_7020_DSP_CFG_GFX2    0x00000020
#define GENGAM_7020_DSP_CFG_VP2     0x00000040

/* Hardware block */
#define GENGAM_BLOCK_NB_7020       8

/* Depth config */
static const stvmix_GenGamma_Input_Type_e GenGammaDepth7020[GENGAM_TWO_MIXER_BLOCK][GENGAM_MAX_DEPTH]=
{
    {
        /* 7020 Hardware pb in documentation DEPTH2 to DEPTH4 is programmable & not DEPTH5 */
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_BKC | GENGAM_TYPE_GDP3 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_VID2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED
    }
};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t* GenGammaBlock7020[GENGAM_BLOCK_NB_7020] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_GDP3],
    &GenGammaBlock[GENGAM_BLOCK_ALPHA],
    &GenGammaBlock[GENGAM_BLOCK_VID1],
    &GenGammaBlock[GENGAM_BLOCK_VID2],
    &GenGammaBlock[GENGAM_BLOCK_FILTER]
};
#endif /* STVMIX_GENGAMMA_7020 */


/*IC*/
#if defined STVMIX_GENGAMMA_7020
#define GENGAM_DSPCFG_OFFSET   0x00006100
#define GENGAM_DSP_CFG_DSD     0x00000002
#define GENGAM_DSP_CFG_DESL    0x00000004
#define GENGAM_DSP_CFG_NUP     0x00000010
#define GENGAM_DSP_CFG_GFX2    0x00000020
#define GENGAM_DSP_CFG_VP2     0x00000040
#elif defined (STVMIX_GENGAMMA_7710)||defined (STVMIX_GENGAMMA_7100)||defined (STVMIX_GENGAMMA_7109)
#define GENGAM_DSPCFG_OFFSET           0x70
#define GENGAM_DSP_CFG_UP_SEL          0x08
#define GENGAM_DSP_CFG_PIP_SEL         0x0800
#define GENGAM_DSP_CFG_DVP_REF         0x40
#define GENGAM_DSP_CFG_DESL            0x01
#define GENGAM_DSP_CFG_NUP             0x04
#endif

#ifdef STVMIX_GENGAMMA_7710
/**************************/
/* Configuration for 7710 */
/**************************/
/* Registers */

#define FS_CLK_CFG2                    0x380000c8
#define DSPCFG_DISP_HD_CLK_DIV2       0x1
#define DSPCFG_COMP_CLK_HD            0xFFFFFFEF
#define FS_UNLOCK                     0x5AA50000

/* Hardware block */
#define GENGAM_BLOCK_NB_7710       8

/* Depth config */
static const stvmix_GenGamma_Input_Type_e GenGammaDepth7710[GENGAM_TWO_MIXER_BLOCK][GENGAM_MAX_DEPTH]=
{
    {

        GENGAM_TYPE_BKC | GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA| GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA | GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA | GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_VID2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP2| GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED

    }
};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t* GenGammaBlock7710[GENGAM_BLOCK_NB_7710] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_ALPHA],
    &GenGammaBlock[GENGAM_BLOCK_VID1_HDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_VID2_HDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI1],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI2]

};
/* Private Variables (static)------------------------------------------------ */
static BOOL MainSelected = FALSE;

#endif /* STVMIX_GENGAMMA_7710 */

#ifdef STVMIX_GENGAMMA_7100
/**************************/
/* Configuration for 7100 */
/**************************/
/* Registers */
#define CKGB_LOCK               0x010
#define CKGB_CLK_SRC            0x0a8
#define CLK_GDP2_SRC            0x1



/* Hardware block */
#define GENGAM_BLOCK_NB_7100       8

/* Depth config */
static const stvmix_GenGamma_Input_Type_e GenGammaDepth7100[GENGAM_TWO_MIXER_BLOCK][GENGAM_MAX_DEPTH]=
{
    {

        GENGAM_TYPE_BKC | GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_VID2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP2| GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED

    }
};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t* GenGammaBlock7100[GENGAM_BLOCK_NB_7100] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_ALPHA],
    &GenGammaBlock[GENGAM_BLOCK_VID1_HDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_VID2_HDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI1],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI2]
};
/* Private Variables (static)------------------------------------------------ */
static BOOL MainSelected = FALSE;

#endif /* STVMIX_GENGAMMA_7100 */
#ifdef STVMIX_GENGAMMA_7200

/**************************/
/* Configuration for 7200 */
/**************************/
#define CLK_GDP3_SRC            0x400
#define CLK_VIDEO2_SRC          0x800


/* Hardware block */
#define GENGAM_BLOCK_NB_7200       10

/* Depth config */
static stvmix_GenGamma_Input_Type_e GenGammaDepth7200[GENGAM_THREE_MIXER_BLOCK][GENGAM_MAX_DEPTH]=
{
    {
        GENGAM_TYPE_BKC | GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDP3 ,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDP3 ,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDP3 ,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDP3 ,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDP3 ,
        GENGAM_TYPE_RESERVED ,
        GENGAM_TYPE_RESERVED ,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_BKC |GENGAM_TYPE_VID2|GENGAM_TYPE_GDP3 | GENGAM_TYPE_GDP4,
        GENGAM_TYPE_VID2|GENGAM_TYPE_GDP3 | GENGAM_TYPE_GDP4,
        GENGAM_TYPE_VID2|GENGAM_TYPE_GDP3 | GENGAM_TYPE_GDP4,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED


    },
    {
        GENGAM_TYPE_BKC |GENGAM_TYPE_VID3 | GENGAM_TYPE_GDP1,
        GENGAM_TYPE_VID3 | GENGAM_TYPE_GDP1,
        GENGAM_TYPE_RESERVED ,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED


    }

};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t* GenGammaBlock7200[GENGAM_BLOCK_NB_7200] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_GDP3],
    &GenGammaBlock[GENGAM_BLOCK_GDP4],
    &GenGammaBlock[GENGAM_BLOCK_VID2_DISPLAYPIPE],
    &GenGammaBlock[GENGAM_BLOCK_VID3_DISPLAYPIPE],
    &GenGammaBlock[GENGAM_BLOCK_VID4_DISPLAYPIPE],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI1],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI2]
};
/* Private Variables (static)------------------------------------------------ */
static BOOL MainSelected = FALSE;

#endif /* STVMIX_GENGAMMA_7200 */

#ifdef STVMIX_GENGAMMA_7109

/**************************/
/* Configuration for 7109 */
/**************************/
#define CKGB_LOCK               0x010
#define CKGB_CLK_SRC            0x0a8
#define CLK_GDP2_SRC            0x1
#define CLK_GDP3_SRC            0x1

/* Hardware block */
#define GENGAM_BLOCK_NB_7109       9



/* Depth config */
static stvmix_GenGamma_Input_Type_e GenGammaDepth7109[GENGAM_TWO_MIXER_BLOCK][GENGAM_MAX_DEPTH]=
{
    {

        GENGAM_TYPE_BKC | GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2 ,
        GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_RESERVED  ,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_VID2,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP2 | GENGAM_TYPE_GDPVBI2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED


    }
};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t* GenGammaBlock7109[GENGAM_BLOCK_NB_7109] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_GDP3],
    &GenGammaBlock[GENGAM_BLOCK_ALPHA],
    &GenGammaBlock[GENGAM_BLOCK_VID1_DISPLAYPIPE],
    &GenGammaBlock[GENGAM_BLOCK_VID2_HDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI1],
    &GenGammaBlock[GENGAM_BLOCK_GDPVBI2]

};
/* Private Variables (static)------------------------------------------------ */
static BOOL MainSelected = FALSE;


#endif /* STVMIX_GENGAMMA_7109 */



#ifdef STVMIX_GENGAMMA_5528
/**************************/
/* Configuration for 5528 */
/**************************/
#define GENGAM_BLOCK_NB_5528  9

#ifdef ST_OSLINUX
/* For Linux, this value is not an absolute address but an offset relatively to Output Stage registers */
#define VOS_MISC_CTL                0x00000000
#else
/* Caution : VMIX must set some VOUT registers !!! */
#ifdef ST_OS21
    #define ST5528_PERIPH_BASE              0xB8000000
#endif
#ifdef ST_OS20
    #define ST5528_PERIPH_BASE              0x18000000
#endif

#define VOS_MISC_CTL                ST5528_PERIPH_BASE + 0x01011C00
#endif

#define VOS_MISC_CTL_DENC_AUX_SEL   0x00000001
#define VOS_MISC_CTL_DVO_IN_SEL_MSK 0x00000006
#define VOS_MISC_CTL_DVO_IN_SEL_MAI 0x00000000
#define VOS_MISC_CTL_DVO_IN_SEL_AUX 0x00000002
#define VOS_MISC_CTL_DVO_IN_SEL_VID 0x00000004

/* Depth config */
static const stvmix_GenGamma_Input_Type_e GenGammaDepth5528[GENGAM_TWO_MIXER_BLOCK][GENGAM_MAX_DEPTH] =
{
    {
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_BKC | GENGAM_TYPE_GDP1 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP3 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP4 | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_ALPHA,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_CUR
    },
    {
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_BKC,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_VID1 | GENGAM_TYPE_VID2,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED | GENGAM_TYPE_GDP3,
        GENGAM_TYPE_RESERVED,
        GENGAM_TYPE_RESERVED
    }
};

/* Hardawre blocks */
static const stvmix_GenGamma_Block_t * GenGammaBlock5528[GENGAM_BLOCK_NB_5528] =
{
    &GenGammaBlock[GENGAM_BLOCK_CURSOR],
    &GenGammaBlock[GENGAM_BLOCK_GDP1],
    &GenGammaBlock[GENGAM_BLOCK_GDP2],
    &GenGammaBlock[GENGAM_BLOCK_GDP3],
    &GenGammaBlock[GENGAM_BLOCK_GDP4],
    &GenGammaBlock[GENGAM_BLOCK_ALPHA],
    &GenGammaBlock[GENGAM_BLOCK_VID1_SDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_VID2_SDDISPO2],
    &GenGammaBlock[GENGAM_BLOCK_FILTER]
};
#endif /* STVMIX_GENGAMMA_5528 */

/***********************************/
/* Software configuration for ALL  */
/***********************************/
#define MIX_U16_MASK                0xFFFF0000
#define GENGAM_CRB_SHIFT            3
#define GENGAMMA_DEVICEID_MASK      0xFFF
#define GENGAMMA_HORIZONTAL_OFFSET  40
#define GENGAM_BLACK_COLOR          0x00000000

/* Private Constants -------------------------------------------------------- */
#define GENGAMMA_OFFSET_BLACK 16
#define GENGAMMA_GLOBAL_GAIN  859 /* meant as 85.9% (219/255) */

/* Private Function prototypes ---------------------------------------------- */


#ifdef ST_OSLINUX
#if ( defined(ST_7100) || defined(ST_7109 ) || defined(ST_7200 ) )
#define ReadRegBase32LE(Add) STSYS_ReadRegDev32LE((void*)\
                                 ((U32)Device_p->InitParams.DeviceBaseAddress_p \
                                + Add));

#else
#define ReadRegBase32LE(Add) STSYS_ReadRegDev32LE(\
                                (U32)Device_p->InitParams.BaseAddress_p\
                                + Add);
#endif

#define ReadRegDev32LE(Add) STSYS_ReadRegDev32LE((void*) \
                                ((U32)Device_p->InitParams.BaseAddress_p \
                                +(U32)Device_p->InitParams.DeviceBaseAddress_p \
                                + Add));

#else
#define ReadRegBase32LE(Add) STSYS_ReadRegDev32LE(\
                                 (U32)Device_p->InitParams.DeviceBaseAddress_p \
                                + Add);

#define ReadRegDev32LE(Add) STSYS_ReadRegDev32LE( \
                                (U32)Device_p->InitParams.BaseAddress_p \
                                +(U32)Device_p->InitParams.DeviceBaseAddress_p \
                                + Add);

#endif

#define WriteRegDev32LE(Add,Value) STSYS_WriteRegDev32LE( \
                                       (U32)Device_p->InitParams.BaseAddress_p \
                                      +(U32)Device_p->InitParams.DeviceBaseAddress_p \
                                      + Add, Value);

#ifdef ST_OSLINUX

#if ( defined(ST_7100) || defined(ST_7109 ) || defined(ST_7200 ) )
#define WriteRegBase32LE(Add,Value) STSYS_WriteRegDev32LE( \
                                 (U32)Device_p->InitParams.DeviceBaseAddress_p \
                                        + Add, Value);
#else
#define WriteRegBase32LE(Add,Value) STSYS_WriteRegDev32LE( \
                                         (U32)Device_p->InitParams.BaseAddress_p \
                                        + Add, Value);
#endif

#else
#define WriteRegBase32LE(Add,Value) STSYS_WriteRegDev32LE( \
                                        (U32)Device_p->InitParams.DeviceBaseAddress_p \
                                        + Add, Value);
#endif


/* Private functions */

/*******************************************************************************
Name        :+ stvmix_GenGammaWriteControlRegister
Description : Write control registers
Parameters  : Hal_p Pointer on hal
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
static void stvmix_GenGammaWriteControlRegister(const stvmix_Device_t * const Device_p)
{
    stvmix_HalGenGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

    /* Store new value for CTL & CRB */
    WriteRegDev32LE((GENGAM_MIX_CRB), Hal_p->RegCRB);
    WriteRegDev32LE((GENGAM_MIX_ACT), Hal_p->RegACT);
    WriteRegDev32LE((GENGAM_MIX_CTL), Hal_p->RegCTL);

    return;
}

/*******************************************************************************
Name        : stvmix_GenGammaReceiveEvtVSync
Description : Vsync event callback
Parameters  : Reason, RegistrantName, Event, EventData and Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
static void  stvmix_GenGammaReceiveEvtVSync(STEVT_CallReason_t Reason,
                                            const ST_DeviceName_t RegistrantName,
                                            STEVT_EventConstant_t Event,
                                            const void *EventData,
                                            const void *SubscriberData_p)
{

    STVTG_VSYNC_t EventType;
    const stvmix_Device_t* Device_p;
    stvmix_HalGenGamma_t * Hal_p;

    EventType = (*((const STVTG_VSYNC_t*) EventData));
    Device_p = ((const stvmix_Device_t*) SubscriberData_p);

    UNUSED(Reason);
    UNUSED(RegistrantName);
    UNUSED(Event);

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

#ifdef ST_OSLINUX
    if (! Hal_p->VSyncRegistered)
    return;
#endif

    if(Hal_p->VSyncToProcess)
    {

        if(Device_p->ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN)
        {

            if(EventType == Hal_p->WriteEventType)
            {

                if(!Hal_p->TestVsync)
                {

                    stvmix_GenGammaWriteControlRegister(Device_p);
                }

                /* Unlocked by semaphore */
                STOS_SemaphoreSignal(Hal_p->VSyncProcessed_p);

                /* Event processed */
                Hal_p->VSyncToProcess = FALSE;
            }
        }
        else
        {

            if(!Hal_p->TestVsync)
            {

                stvmix_GenGammaWriteControlRegister(Device_p);
            }

            /* Unlocked by semaphore */
            STOS_SemaphoreSignal(Hal_p->VSyncProcessed_p);

            /* Event processed */
            Hal_p->VSyncToProcess = FALSE;
        }
    }
}


/*******************************************************************************
Name        : stvmix_GenGammaSetBackgroundColor
Description : Adjust the RGB color
Parameters  : Hal_p Pointer on hal
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
static void stvmix_GenGammaSetBackgroundColor (stvmix_Device_t * const Device_p)
{

    U32 R,G,B;

  R = Device_p->Background.Color.R * GENGAMMA_GLOBAL_GAIN / 1000 + GENGAMMA_OFFSET_BLACK;
  G = Device_p->Background.Color.G * GENGAMMA_GLOBAL_GAIN / 1000 + GENGAMMA_OFFSET_BLACK;
  B = Device_p->Background.Color.B * GENGAMMA_GLOBAL_GAIN / 1000 + GENGAMMA_OFFSET_BLACK;

 WriteRegDev32LE((GENGAM_MIX_BKC), (R<<16) | (G<<8) | (B));

}


/******************************************************************************/
/******************** BEGIN OF GENGAMMA FUNCTIONS *****************************/
/******************************************************************************/

/*******************************************************************************
Name        : stvmix_GenGammaConnectBackground
Description : Enable background for generic gamma device
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations : This function is called when only background is connected
Returns     : None
*******************************************************************************/
void stvmix_GenGammaConnectBackground(stvmix_Device_t * const Device_p)
{
    U32 RegCTL;
    stvmix_HalGenGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

    /* Is background present on mixer block */
    if(Hal_p->GenGammaDepth_p[GENGAM_BKC_DEPTH] & GENGAM_TYPE_BKC)
    {
        RegCTL = ReadRegDev32LE(GENGAM_MIX_CTL);

        if(Device_p->Background.Enable == TRUE)
        {
            RegCTL |= GENGAM_CTL_ACT_BKC_ENABLE;
        }
        else
        {
            RegCTL &= ~((U32)GENGAM_CTL_ACT_BKC_ENABLE);
        }
#ifdef WA_GNBvd16602
        if(Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_7020)
        {
            /* Background should always be enable  */
            RegCTL |= GENGAM_CTL_ACT_BKC_ENABLE;
            if(Device_p->Background.Enable == TRUE)
            {
                WriteRegDev32LE(GENGAM_MIX_BKC, (Device_p->Background.Color.R<<16) |
                                (Device_p->Background.Color.G<<8) | (Device_p->Background.Color.B));
            }
            else
            {
                WriteRegDev32LE(GENGAM_MIX_BKC, GENGAM_BLACK_COLOR);
            }
        }
#endif /* WA_GNBvd16602 */

        /* Store new value for CTL */
        WriteRegDev32LE((GENGAM_MIX_CTL), RegCTL);
    }
    return;
}

/*******************************************************************************
Name        : stvmix_GenGammaConnectLayerMix
Description : Test and set connection of layer on generic gamma device
Parameters  : Pointer on device, Purpose of connection
Assumptions : Device is valid, Hal pointer is valid
Limitations : In LAYER_SET_REGISTER mode, no error should be returned
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t stvmix_GenGammaConnectLayerMix(stvmix_Device_t * const Device_p,
                                              stvmix_HalLayerConnect Purpose)
{
    ST_ErrorCode_t RetErr = ST_NO_ERROR, RetErr2;
    U32 DeviceId;
    U16 i, j, NbLayers;
    S32 Depth = GENGAM_MAX_DEPTH-1;
    STLAYER_Layer_t Type;
    stvmix_HalGenGamma_t * Hal_p;
    clock_t Time;
#if defined (STVMIX_GENGAMMA_7200) ||   defined (STVMIX_GENGAMMA_7109)
    STVOUT_Handle_t        VoutHandle;
    STVOUT_OpenParams_t    VoutOpenParams;
    STVOUT_OutputParams_t  VoutParams;
#endif


#ifdef STVMIX_GENGAMMA_7020
    U32 DspCfg;
#endif /* STVMIX_GENGAMMA_7020 */
#if defined (STVMIX_GENGAMMA_7710) || defined (STVMIX_GENGAMMA_7100) ||defined (STVMIX_GENGAMMA_7109)||defined (STVMIX_GENGAMMA_7200)

    U32 FsClkCfg;
#endif

    #if defined (STVMIX_GENGAMMA_7109)
        U32 CutId;
        U32 DspCfg;
    #endif

    #if defined (STVMIX_GENGAMMA_7109)||defined (STVMIX_GENGAMMA_7200)
        BOOL IsVid2OnMix1=FALSE;
    #endif

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));


    #if defined (STVMIX_GENGAMMA_7109)
     CutId = ST_GetCutRevision();
    #endif

    /* Reset control register */
    Hal_p->RegCTL = 0;
    Hal_p->RegCRB = 0;
    Hal_p->RegCRB = 0;

    switch(Purpose)
    {
        case LAYER_CHECK:
            i = NbLayers = Device_p->Layers.NbTmp;
            break;
        case LAYER_SET_REGISTER:
            i = NbLayers = Device_p->Layers.NbConnect;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    while(i != 0)
    {
        i--;
        /* Get Layer Type */
        switch(Purpose)
        {
            case LAYER_CHECK:
                Type = Device_p->Layers.TmpArray_p[i].Type;
                DeviceId = Device_p->Layers.TmpArray_p[i].DeviceId;
                break;
            case LAYER_SET_REGISTER:
                Type = Device_p->Layers.ConnectArray_p[i].Type;
                DeviceId = Device_p->Layers.ConnectArray_p[i].DeviceId;
                break;
            default :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }

        /* Search for layer type with corresponding BBA & DEVICEID_MASK = DeviceId */
        for(j=0; (j<Hal_p->BlockNb); j++)
        {
            /* If layer type found, exit now */
            if ((Hal_p->GenGammaBlock_p[j]->LayerType == Type) &&
                (Hal_p->GenGammaBlock_p[j]->BBA == (DeviceId & GENGAMMA_DEVICEID_MASK)))
                break;
            /* TODO : In layer set DeviceId to BBA for Videos & remove the following lines !! */


            if((Hal_p->GenGammaBlock_p[j]->LayerType == Type) &&
               ((Type == STLAYER_7020_VIDEO1) ||
                (Type == STLAYER_7020_VIDEO2) ||
                (Type == STLAYER_SDDISPO2_VIDEO1) ||
                (Type == STLAYER_SDDISPO2_VIDEO2) ||
                (Type == STLAYER_HDDISPO2_VIDEO1) ||
                (Type == STLAYER_HDDISPO2_VIDEO2) ||
                (Type == STLAYER_DISPLAYPIPE_VIDEO1) ||
#ifdef STVMIX_GENGAMMA_7200
                (Type == STLAYER_DISPLAYPIPE_VIDEO2) ||
                (Type == STLAYER_DISPLAYPIPE_VIDEO3) ||
                (Type == STLAYER_DISPLAYPIPE_VIDEO4) ||
#endif
                (Type == STLAYER_GAMMA_FILTER)))
                break;
        }

        /* Layer type not found ? */
        if(j == Hal_p->BlockNb)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }




        /* Search for input type in depth table */
        while (Depth != -1)
        {

#ifdef STVMIX_GENGAMMA_7109

                 /*if ((((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2) || (Hal_p->GenGammaBlock_p[j]->InputType==GENGAM_TYPE_VID2)||(CutId>2))*/
            if ((((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1) && (Hal_p->GenGammaBlock_p[j]->InputType==GENGAM_TYPE_VID2)&&(CutId>=0xC0))
            {
                IsVid2OnMix1 = TRUE;
            }
#endif

#ifdef STVMIX_GENGAMMA_7200

            if ((((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1) && (Hal_p->GenGammaBlock_p[j]->InputType==GENGAM_TYPE_VID2))
            {
                IsVid2OnMix1 = TRUE;
            }
#endif

            if(Hal_p->GenGammaDepth_p[Depth] & Hal_p->GenGammaBlock_p[j]->InputType)
            {
                Hal_p->RegCTL |= Hal_p->GenGammaBlock_p[j]->CTLACTEnable;
                if(Hal_p->GenGammaBlock_p[j]->CRBInput != GENGAM_CRB_NOTHING)
                {
                    if(!(Hal_p->GenGammaDepth_p[Depth] & GENGAM_TYPE_RESERVED))
                    {
                        Hal_p->RegCRB |= ((Hal_p->GenGammaBlock_p[j]->CRBInput)<<(Depth*GENGAM_CRB_SHIFT));
                    }
                    if(Depth !=0)
                        Depth--;
                    /* TODO : Enable active signal for video */
                    /* if((Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE) && */
                    /* (Hal_p->GenGammaBlock_p[j]->ACTEnable == TRUE)) */
                    /* { */
                    /* Hal_p->RegACT |= ((Hal_p->GenGammaBlock_p[j]->CTLACTEnable)<<16); */
                    /* } */
                    /* Enable active signal for graphic */
                    if((Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE) &&
                       (Hal_p->GenGammaBlock_p[j]->ACTEnable == TRUE))
                    {
                        Hal_p->RegACT |= Hal_p->GenGammaBlock_p[j]->CTLACTEnable;
                    }
                }
                break;
            }



                      /* Update DVP registers */

            Depth--;
            #ifdef STVMIX_GENGAMMA_7710   /** program CLK_COMP for GDP2  **/

             if ((Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDP2) || (Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDPVBI2))
               {
                    STOS_InterruptLock();
                    /* Register DSP_CFG to be updated */
                    FsClkCfg = ReadRegBase32LE(FS_CLK_CFG2);
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                     {
                            FsClkCfg |= ( DSPCFG_DISP_HD_CLK_DIV2 << 4);

                     }
                     if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                     {
                        FsClkCfg &= DSPCFG_COMP_CLK_HD ;
                     }
                    FsClkCfg = FsClkCfg | FS_UNLOCK;
                    WriteRegBase32LE(FS_CLK_CFG2, FsClkCfg);
                  /* Unprotect access */
                    STOS_InterruptUnlock();

               }
            #endif
            #if defined (STVMIX_GENGAMMA_7100)  /** program CLK_SRC for GDP2  **/


             if ((Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDP2)|| (Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDPVBI2))
               {
                    STOS_InterruptLock();
                    /* UNLOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc0de);
                    /*CKGB_CLK_SRC to be updated*/
                    FsClkCfg = ReadRegBase32LE( CKG_BASE_ADDRESS + CKGB_CLK_SRC);
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                     {
                            FsClkCfg |= CLK_GDP2_SRC ;
                     }
                     if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                     {
                            FsClkCfg &= ~((U32)CLK_GDP2_SRC) ;
                     }
                     WriteRegBase32LE(CKG_BASE_ADDRESS + CKGB_CLK_SRC, FsClkCfg);
                     /* LOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc1a0);
                  /* Unprotect access */
                    STOS_InterruptUnlock();

               }
            #endif

            #if defined (STVMIX_GENGAMMA_7109)   /** program CLK_SRC for GDP2  **/



     if (CutId < 0xC0)
             {

             if ((Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDP2)|| (Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDPVBI2))
               {
                    STOS_InterruptLock();
                    /* UNLOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc0de);
                    /*CKGB_CLK_SRC to be updated*/
                    FsClkCfg = ReadRegBase32LE( CKG_BASE_ADDRESS + CKGB_CLK_SRC);
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                     {
                            FsClkCfg |= CLK_GDP2_SRC ;
                     }
                     if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                     {
                            FsClkCfg &= ~((U32)CLK_GDP2_SRC) ;
                     }
                     WriteRegBase32LE(CKG_BASE_ADDRESS + CKGB_CLK_SRC, FsClkCfg);
                     /* LOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc1a0);
                  /* Unprotect access */
                    STOS_InterruptUnlock();

               }

    }
    else
    {
             if (Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDP3)
               {
                    STOS_InterruptLock();
                    /* UNLOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc0de);
                    /*CKGB_CLK_SRC to be updated*/
                    FsClkCfg = ReadRegBase32LE( CKG_BASE_ADDRESS + CKGB_CLK_SRC);
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                     {
                            FsClkCfg |= CLK_GDP3_SRC ;
                     }
                     if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                     {
                            FsClkCfg &= ~((U32)CLK_GDP3_SRC) ;
                     }
                     WriteRegBase32LE(CKG_BASE_ADDRESS + CKGB_CLK_SRC, FsClkCfg);
                     /* LOCK clock register*/
                    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS + CKGB_LOCK,0xc1a0);
                  /* Unprotect access */
                    STOS_InterruptUnlock();

               }

    }
    #endif

            #if defined (STVMIX_GENGAMMA_7200)   /** program CLK_SRC for GDP3  **/

               if (Hal_p->GenGammaBlock_p[j]->InputType == GENGAM_TYPE_GDP3)
               {
                    STOS_InterruptLock();

                    FsClkCfg = STSYS_ReadRegDev32LE(ST7200_HDMI_BASE_ADDRESS+0x20);
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                     {
                            FsClkCfg |= CLK_GDP3_SRC ;
                     }
                     if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                     {
                            FsClkCfg &= ~((U32)CLK_GDP3_SRC) ;
                     }
                     STSYS_WriteRegDev32LE((ST7200_HDMI_BASE_ADDRESS+0x20),FsClkCfg);

                  /* Unprotect access */
                    STOS_InterruptUnlock();

               }
            #endif
           }
        if(Depth == -1)
        {
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
        }
    }

#if defined (STVMIX_GENGAMMA_7200) ||   defined (STVMIX_GENGAMMA_7109)

    /*retreive screen parameters from stvtg mode*/
    RetErr = STVOUT_Open( Device_p->Outputs.ConnectArray_p[0].Name, &VoutOpenParams, &VoutHandle);
    if(RetErr != ST_NO_ERROR)
    {
        return(RetErr);
    }
    RetErr = STVOUT_GetOutputParams(VoutHandle, &VoutParams);
    if(RetErr != ST_NO_ERROR)
    {
        return(RetErr);
    }

    if ((VoutParams.Analog.ColorSpace)==STVOUT_ITU_R_709)
    {
        Hal_p->RegCTL |= GENGAM_CTL_709NOT601;
    }

    RetErr = STVOUT_Close(VoutHandle);
     if(RetErr != ST_NO_ERROR)
    {
        return(RetErr);
    }



#endif


#if defined (STVMIX_GENGAMMA_7109)

if (((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
    { /*  GENGAM_DSPCFG_OFFSET is updated only on the case of MIX1*/
    if (IsVid2OnMix1)
    {
        /* Protect access */
        STOS_InterruptLock();

        /* Register DSP_CFG to be updated */
        DspCfg = ReadRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET);

        DspCfg |= (GENGAM_DSP_CFG_PIP_SEL);

        WriteRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET, DspCfg);

        /* Unprotect access */
        STOS_InterruptUnlock();
    }
    else
    {
        /* Protect access */
        STOS_InterruptLock();

        /* Register DSP_CFG to be updated */
        DspCfg = ReadRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET);

        DspCfg &= ~(GENGAM_DSP_CFG_PIP_SEL);

        WriteRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET, DspCfg);

        /* Unprotect access */
        STOS_InterruptUnlock();

    }
   }
#endif


#if defined (STVMIX_GENGAMMA_7200)

 if (((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_COMPO_MASK) == GENGAM_MIX_BBA_LOCAL_COMPO) /* Local Compositor */
{
    if (((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
        { /*  GENGAM_DSPCFG_OFFSET is updated only on the case of MIX1*/
        if (IsVid2OnMix1)
        {
            /* Protect access */
            STOS_InterruptLock();

            FsClkCfg = STSYS_ReadRegDev32LE(ST7200_HDMI_BASE_ADDRESS+0x20);

            FsClkCfg &= ~(CLK_VIDEO2_SRC);

            STSYS_WriteRegDev32LE((ST7200_HDMI_BASE_ADDRESS+0x20),FsClkCfg);

            /* Unprotect access */
            STOS_InterruptUnlock();
        }
        else
        {
            /* Protect access */
            STOS_InterruptLock();

            FsClkCfg = STSYS_ReadRegDev32LE(ST7200_HDMI_BASE_ADDRESS+0x20);

            FsClkCfg |=(CLK_VIDEO2_SRC);

            STSYS_WriteRegDev32LE((ST7200_HDMI_BASE_ADDRESS+0x20),FsClkCfg);

            /* Unprotect access */
            STOS_InterruptUnlock();

        }
    }
}
#endif



    /* Is background present on mixer block */
    if(Hal_p->GenGammaDepth_p[GENGAM_BKC_DEPTH] & GENGAM_TYPE_BKC)
    {
        /* Enable background color */
        if(Device_p->Background.Enable == TRUE)
        {
            /* Enable background which is not an hardware block */
            Hal_p->RegCTL |= GENGAM_CTL_ACT_BKC_ENABLE;
        }

#ifdef WA_GNBvd16602
        if((Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_7020) &&
           (Purpose == LAYER_SET_REGISTER))
        {
            /* Background should always be enable  */
            Hal_p->RegCTL |= GENGAM_CTL_ACT_BKC_ENABLE;
            if(Device_p->Background.Enable == TRUE)
            {
                WriteRegDev32LE(GENGAM_MIX_BKC, (Device_p->Background.Color.R<<16) |
                                (Device_p->Background.Color.G<<8) | (Device_p->Background.Color.B));
            }
            else
            {
                WriteRegDev32LE(GENGAM_MIX_BKC, GENGAM_BLACK_COLOR);
            }
        }
#endif /* WA_GNBvd16602 */
    }

    if(Purpose == LAYER_SET_REGISTER)
    {
#ifdef STVMIX_GENGAMMA_7020
        if(Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_7020)
        {
            /* Protect access */
            STOS_InterruptLock();

            /* Register DSP_CFG to be updated */
            DspCfg = ReadRegBase32LE(GENGAM_DSPCFG_OFFSET);

            if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
            {
                /* For GDP2 */
                if(Hal_p->RegCTL & GENGAM_CTL_ACT_GDP2_ENABLE)
                    DspCfg |= GENGAM_DSP_CFG_GFX2;

                /* For VIDEO2 */
                if(Hal_p->RegCTL & GENGAM_CTL_ACT_VID2_ENABLE)
                    DspCfg |= GENGAM_DSP_CFG_VP2;
            }
            else
            {
                /* For GDP2 */
                if(Hal_p->RegCTL & GENGAM_CTL_ACT_GDP2_ENABLE)
                    DspCfg &= ~((U32)GENGAM_DSP_CFG_GFX2);

                /* For VIDEO2 */
                if(Hal_p->RegCTL & GENGAM_CTL_ACT_VID2_ENABLE)
                    DspCfg &= ~((U32)GENGAM_DSP_CFG_VP2);
            }

            WriteRegBase32LE(GENGAM_DSPCFG_OFFSET, DspCfg);

            /* Unprotect access */
            STOS_InterruptUnlock();
        }
#endif /* STVMIX_GENGAMMA_7020 */

        /* Do not test VSync and write into register */
        Hal_p->TestVsync = FALSE;

        if(!(Device_p->Status & API_STVTG_CONNECT))
        {
            stvmix_GenGammaWriteControlRegister(Device_p);
        }
    }
    else
    {
        /* Test if Vsync are generated in layer check mode */
        Hal_p->TestVsync = TRUE;
    }
    if(Device_p->Status & API_STVTG_CONNECT)
    {
        /* Reset semaphore count */
        while(STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, TIMEOUT_IMMEDIATE) != -1);

        /* Enable layers on vtg bottom event */
        Hal_p->WriteEventType = STVTG_BOTTOM;

        /* Process one event */
        Hal_p->VSyncToProcess = TRUE;

        RetErr = stvmix_GenGammaEvt(Device_p, VSYNC_SUBSCRIBE, Device_p->VTG);
        if(RetErr == ST_NO_ERROR)
        {
            /* VSync Generated ? */
            Time = STOS_time_plus(time_now(), STVMIX_MAX_VSYNC_DURATION );
            if( STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
            {
                /* If here, no vtg connection. Report an error */
                RetErr = STVMIX_ERROR_NO_VSYNC;
            }
            RetErr2 = stvmix_GenGammaEvt(Device_p, VSYNC_UNSUBSCRIBE, Device_p->VTG);
            if(RetErr == ST_NO_ERROR)
            {
                RetErr =  RetErr2;
            }
        }
    }
    return(RetErr);
}

/*******************************************************************************
Name        : stvmix_GenGammaEvt
Description : Subscribe & Unsubscribe to events
Parameters  : Pointer on device, purpose of call
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvmix_GenGammaEvt(stvmix_Device_t * const Device_p,
                                  stvmix_HalActionEvt_e Purpose,
                                  ST_DeviceName_t VTGName)
{
    STEVT_DeviceSubscribeParams_t SubscribeParams;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvmix_HalGenGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));
    switch(Purpose)
    {
        case VSYNC_SUBSCRIBE:
            if(! Hal_p->VSyncRegistered)
            {
                /* VTG Name set when reason is LAYER_SET_REGISTER */
                SubscribeParams.NotifyCallback = stvmix_GenGammaReceiveEvtVSync;
                SubscribeParams.SubscriberData_p = Device_p;
                ErrCode = STEVT_SubscribeDeviceEvent(Hal_p->EvtHandle,
                                                     VTGName,
                                                     STVTG_VSYNC_EVT,
                                                     &SubscribeParams);
                if(ErrCode == ST_NO_ERROR)
                {
                    /* Subscribed to VSync Event */
                    Hal_p->VSyncRegistered = TRUE;
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "Not able to register device event VSync"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

        case VSYNC_UNSUBSCRIBE:
            if(Hal_p->VSyncRegistered)
            {
#ifdef ST_OSLINUX
                Hal_p->VSyncRegistered = FALSE;
                Hal_p->VSyncToProcess = FALSE;
#endif

                /* Unsubscribe to vtg VSyncEvent */
                ErrCode = STEVT_UnsubscribeDeviceEvent(Hal_p->EvtHandle,
                                                       VTGName,
                                                       STVTG_VSYNC_EVT);
                if(ErrCode == ST_NO_ERROR)
                {
                    /* Subscribe to VSync Event if not done already */
                    Hal_p->VSyncRegistered = FALSE;
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "Failed to unregister device %s event VSync",
                                  VTGName));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

        case CHANGE_VTG:
            break;

        default:
            break;
    }
    return (ErrCode);
}

/*******************************************************************************
Name        : stvmix_GenGammaInit
Description : Initialise hal for generic gamma device
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED,
              ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t  stvmix_GenGammaInit(stvmix_Device_t * const Device_p)
{
#ifdef STVMIX_GENGAMMA_5528
    U8  Index=0;
    STVOUT_Handle_t VoutHandle;
    STVOUT_OpenParams_t VoutOpenParams;
#endif
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvmix_HalGenGamma_t * Hal_p;
    STEVT_OpenParams_t OpenParams;
    STEVT_Handle_t EvtHandle;

    #if defined (STVMIX_GENGAMMA_7109)
        U32 CutId;
    #endif





    /* Reserve memory for hal structure */
    Device_p->Hal_p = STOS_MemoryAllocate (Device_p->InitParams.CPUPartition_p,
                                       sizeof(stvmix_HalGenGamma_t));
    AddDynamicDataSize(sizeof(stvmix_HalGenGamma_t));

    if(Device_p->Hal_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));


  /*  Check the 7109 cut ID in order to upodate  GenGammaDepth7109 */

    #if defined (STVMIX_GENGAMMA_7109)
     CutId = ST_GetCutRevision();
    #endif

    #if defined (STVMIX_GENGAMMA_7109) && (!defined (STVMIX_GENGAMMA_7200))



        if (CutId >= 0xC0)
       {
          GenGammaDepth7109[0][0] =  GENGAM_TYPE_BKC | GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1 | GENGAM_TYPE_GDP2 | GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2| GENGAM_TYPE_GDP3| GENGAM_TYPE_VID2;
          GenGammaDepth7109[0][1] =  GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2| GENGAM_TYPE_GDP3 | GENGAM_TYPE_VID2;
          GenGammaDepth7109[0][2] =  GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2| GENGAM_TYPE_GDP3 | GENGAM_TYPE_VID2;
          GenGammaDepth7109[0][3] =  GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2| GENGAM_TYPE_GDP3 | GENGAM_TYPE_VID2;
          GenGammaDepth7109[0][4] =  GENGAM_TYPE_VID1  | GENGAM_TYPE_GDP1|GENGAM_TYPE_GDP2|GENGAM_TYPE_ALPHA|GENGAM_TYPE_GDPVBI1 | GENGAM_TYPE_GDPVBI2| GENGAM_TYPE_GDP3 | GENGAM_TYPE_VID2;
          GenGammaDepth7109[1][0] =  GENGAM_TYPE_BKC |GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP3;
          GenGammaDepth7109[1][1] =  GENGAM_TYPE_VID2 | GENGAM_TYPE_GDP3;
          GenGammaDepth7109[1][2] =  GENGAM_TYPE_RESERVED;
       }
    #endif

    /* Check if device type present */
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GENGAMMA_7020
        case STVMIX_GENERIC_GAMMA_TYPE_7020:
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 7020 mixer 1 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7020;
                    Hal_p->GenGammaDepth_p = GenGammaDepth7020[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (stvmix_GenGamma_Block_t**) GenGammaBlock7020;
                    break;

                case GENGAM_MIX_BBA_MIX2:
                    /* Type is 7020 mixer 2 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7020;
                    Hal_p->GenGammaDepth_p = GenGammaDepth7020[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (stvmix_GenGamma_Block_t**) GenGammaBlock7020;
                    break;

                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;
#endif /* STVMIX_GENGAMMA_7020 */

#ifdef STVMIX_GENGAMMA_7710
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 77710 mixer 1 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7710;
                    Hal_p->GenGammaDepth_p = GenGammaDepth7710[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7710;
                    break;

                case GENGAM_MIX_BBA_MIX2:
                    /* Type is 7710 mixer 2 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7710;
                    Hal_p->GenGammaDepth_p = GenGammaDepth7710[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7710;
                    break;

                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;
#endif /* STVMIX_GENGAMMA_7710 */

#ifdef STVMIX_GENGAMMA_7100
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 7100 mixer 1 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7100;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *)GenGammaDepth7100[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7100;
                           break;

                case GENGAM_MIX_BBA_MIX2:
                    /* Type is 7100 mixer 2 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7100;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *) GenGammaDepth7100[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7100;

                    break;

                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;

#endif /* STVMIX_GENGAMMA_7100 */

#ifdef STVMIX_GENGAMMA_7200

         case STVMIX_GENERIC_GAMMA_TYPE_7200:


        if (((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_COMPO_MASK) == GENGAM_MIX_BBA_LOCAL_COMPO) /* Local Compositor */
        {
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 7200 mixer 1 */

                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7200;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *)GenGammaDepth7200[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7200;
                    break;

                case GENGAM_MIX_BBA_MIX2:
                    /* Type is 7200 mixer 2 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7200;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *) GenGammaDepth7200[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7200;
                    break;

                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
        break;
        }

         if (((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_COMPO_MASK) ==GENGAM_MIX_BBA_REMOTE_COMPO) /* Remote Compositor */
        {
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 7200 mixer 3 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7200;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *)GenGammaDepth7200[GENGAM_BLOCK_MIXER3];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7200;
                    break;


                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;
        }
        else
        {
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;

        }



#endif
#ifdef STVMIX_GENGAMMA_7109

         case STVMIX_GENERIC_GAMMA_TYPE_7109:
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {

                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 7109 mixer 1 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7109;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *)GenGammaDepth7109[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7109;

                    break;

                case GENGAM_MIX_BBA_MIX2:
                    /* Type is 7109 mixer 2 */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_7109;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *)GenGammaDepth7109[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock7109;
                    break;


                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;





#endif /* STVMIX_GENGAMMA_7109 */


#ifdef STVMIX_GENGAMMA_5528
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
            switch ((U32)Device_p->InitParams.BaseAddress_p & (U32)GENGAM_MIX_BBA_MASK)
            {
                case GENGAM_MIX_BBA_MIX1:
                    /* Type is 5528 mixer */
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_5528;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *) GenGammaDepth5528[GENGAM_BLOCK_MIXER1];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock5528;
                   for (Index=0; (Index < Device_p->Outputs.NbConnect)&&(Device_p->Outputs.NbConnect!=0);Index++)
                    {

                        if((Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_CVBS)||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_RGB)||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_YC) ||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_YUV))
                        {

                            ErrCode = STVOUT_Open(Device_p->Outputs.ConnectArray_p[Index].Name, &VoutOpenParams, &VoutHandle);
                            if(ErrCode==ST_NO_ERROR)
                            {

                                ErrCode = STVOUT_SetInputSource(VoutHandle,STVOUT_SOURCE_MAIN);
                                if (ErrCode==ST_NO_ERROR)
                                {

                                    ErrCode = STVOUT_Close(VoutHandle);
                                }
                                else
                                {

                                   ErrCode= ST_ERROR_BAD_PARAMETER;
                                }
                            }
                            else
                            {

                                ErrCode= ST_ERROR_NO_FREE_HANDLES;
                            }
                        }

                    }
                    break;

                case GENGAM_MIX_BBA_MIX2:
                    Hal_p->BlockNb = GENGAM_BLOCK_NB_5528;
                    Hal_p->GenGammaDepth_p = (const stvmix_GenGamma_Input_Type_e *) GenGammaDepth5528[GENGAM_BLOCK_MIXER2];
                    Hal_p->GenGammaBlock_p = (const stvmix_GenGamma_Block_t**) GenGammaBlock5528;

                    for (Index=0; (Index < Device_p->Outputs.NbConnect)&&(Device_p->Outputs.NbConnect!=0);Index++)
                    {


                        if((Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_CVBS)||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_RGB)||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_YC) ||\
                           (Device_p->Outputs.ConnectArray_p[Index].Type==STVOUT_OUTPUT_ANALOG_YUV))
                        {


                            ErrCode = STVOUT_Open(Device_p->Outputs.ConnectArray_p[Index].Name, &VoutOpenParams, &VoutHandle);
                            if(ErrCode==ST_NO_ERROR)
                            {

                                ErrCode = STVOUT_SetInputSource(VoutHandle,STVOUT_SOURCE_AUX);
                                if (ErrCode==ST_NO_ERROR)
                                {

                                     ErrCode = STVOUT_Close(VoutHandle);

                                }
                                else
                                {

                                     ErrCode= ST_ERROR_BAD_PARAMETER;
                                 }
                            }
                            else
                            {

                                ErrCode= ST_ERROR_NO_FREE_HANDLES;
                            }
                        }

                    }
                    break;

                default:
                    ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
            }
            break;
#endif /* STVMIX_GENGAMMA_5528 */

        default:
            ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }

    if(ErrCode != ST_NO_ERROR)
    {
        /* Free reserved memory */
        STOS_MemoryDeallocate (Device_p->InitParams.CPUPartition_p,
                           Device_p->Hal_p);
        /* Reset hal structure */
        Device_p->Hal_p = NULL;
        return(ErrCode);
    }

    /* Init hal structure */
    Hal_p->VSyncToProcess = FALSE;
    Hal_p->VSyncRegistered = FALSE;
    Hal_p->TestVsync = FALSE;

    Hal_p->VSyncProcessed_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    /* Check event handler name  */
    ErrCode = STEVT_Open(Device_p->InitParams.EvtHandlerName, &OpenParams, &EvtHandle);
    if(ErrCode != ST_NO_ERROR)
    {

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Bad name %s for event handler name",
                      Device_p->InitParams.EvtHandlerName));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }
    /* Save EVT Handle */
    Hal_p->EvtHandle = EvtHandle;
    if(ErrCode == ST_NO_ERROR)
    {
        /* Set default values */
        stvmix_GenGammaResetConfig(Device_p);
        ErrCode = stvmix_GenGammaSetScreen(Device_p);
        if (ErrCode == ST_NO_ERROR)
        {
            ErrCode = stvmix_GenGammaSetOutputPath(Device_p);
        }
        if(ErrCode != ST_NO_ERROR)
        {

            STEVT_Close(Hal_p->EvtHandle);
        }
    }


    if(ErrCode != ST_NO_ERROR)
    {
        /* Free semaphore */
        STOS_SemaphoreDelete(NULL,Hal_p->VSyncProcessed_p);

        /* Free reserved memory */
        STOS_MemoryDeallocate (Device_p->InitParams.CPUPartition_p,
                           Device_p->Hal_p);
        /* Reset hal structure */
        Device_p->Hal_p = NULL;
    }
    /* Init capability structure */
    Device_p->Capability.ScreenOffsetHorizontalMin = - GENGAMMA_HORIZONTAL_OFFSET;
    Device_p->Capability.ScreenOffsetHorizontalMax = GENGAMMA_HORIZONTAL_OFFSET;
    Device_p->Capability.ScreenOffsetVerticalMin = - STVMIX_COMMON_VERTICAL_OFFSET;
    Device_p->Capability.ScreenOffsetVerticalMax = STVMIX_COMMON_VERTICAL_OFFSET;
    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_GenGammaResetConfig
Description : Reset configuration for generic gamma registers
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_GenGammaResetConfig(stvmix_Device_t * const Device_p)
{
    ST_ErrorCode_t RetErr = ST_NO_ERROR;
    stvmix_HalGenGamma_t * Hal_p;
    clock_t Time;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));
    /* Reset control register */
    Hal_p->RegCTL = 0;
    Hal_p->RegCRB = 0;
    Hal_p->RegCRB = 0;
#ifdef WA_GNBvd16602
    if(Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_7020)
    {
        Hal_p->RegCTL |= GENGAM_CTL_ACT_BKC_ENABLE;
        WriteRegDev32LE(GENGAM_MIX_BKC, GENGAM_BLACK_COLOR);
    }
#endif /* WA_GNBvd16602 */

    if(Device_p->Status & API_STVTG_CONNECT)
    {
        /* Do not test VSync and write into register */
        Hal_p->TestVsync = FALSE;
        /* Reset semaphore count */
        while(STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, TIMEOUT_IMMEDIATE) != -1);
        /* Enable layers on vtg top event */
        Hal_p->WriteEventType = STVTG_TOP;
        /* Process one event */
        Hal_p->VSyncToProcess = TRUE;

        RetErr = stvmix_GenGammaEvt(Device_p, VSYNC_SUBSCRIBE, Device_p->VTG);

        if(RetErr == ST_NO_ERROR)
        {
            /* VSync Generated ? */

            Time = STOS_time_plus(time_now(), STVMIX_MAX_VSYNC_DURATION);
            if( STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
            {

                /* If here, no vtg connection. Report an error */
                RetErr = STVMIX_ERROR_NO_VSYNC;
            }
            /* Wait for a second one */
            Hal_p->WriteEventType = STVTG_BOTTOM;
            /* Process one event */
            Hal_p->VSyncToProcess = TRUE;

            if( STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
            {
                /* If here, no vtg connection. Report an error */
                RetErr = STVMIX_ERROR_NO_VSYNC;
            }
            stvmix_GenGammaEvt(Device_p, VSYNC_UNSUBSCRIBE, Device_p->VTG);
        }
    }
    if((!(Device_p->Status & API_STVTG_CONNECT)) || (RetErr != ST_NO_ERROR))
    {
        /* Immediate write CTL & CRB */
        stvmix_GenGammaWriteControlRegister(Device_p);
    }
    return;
}

/*******************************************************************************
Name        : stvmix_GenGammaSetBackgroundParameters
Description : Set background color for generic gamma mixer device
Parameters  : Pointer on device, Purpose of call
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_GenGammaSetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                       stvmix_HalLayerConnect Purpose)
{
    stvmix_HalGenGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

    /* Is background present on mixer block */
    if(Hal_p->GenGammaDepth_p[GENGAM_BKC_DEPTH] & GENGAM_TYPE_BKC)
    {
        if((Purpose == LAYER_SET_REGISTER)
#ifdef WA_GNBvd16602
           && (Device_p->InitParams.DeviceType != STVMIX_GENERIC_GAMMA_TYPE_7020)
#endif /* WA_GNBvd16602 */
            )
        {
            stvmix_GenGammaSetBackgroundColor(Device_p);
        }
    }
    else
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvmix_GenGammaSetOutputPath
Description : Check & configure that the internal chip path output
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_GenGammaSetOutputPath(stvmix_Device_t * const Device_p)
{

    U32 i;
#ifdef STVMIX_GENGAMMA_7020

    U32 DspCfg;

    if(Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_7020)
    {
        for(i=0; i<Device_p->Outputs.NbConnect; i++)
        {
            switch(Device_p->Outputs.ConnectArray_p[i].Type)
            {
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(GENGAM_DSPCFG_OFFSET);

                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                      {
                        DspCfg |= GENGAM_DSP_CFG_DSD;
                      }
                    else
                     {
                        DspCfg &= ~((U32)GENGAM_DSP_CFG_DSD);
                     }

                    WriteRegBase32LE(GENGAM_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;

                case STVOUT_OUTPUT_HD_ANALOG_RGB:
                case STVOUT_OUTPUT_HD_ANALOG_YUV:
                case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED:
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_ANALOG_SVM:
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    break;

                    /* Internal 7020 DENC here only */
                case STVOUT_OUTPUT_ANALOG_RGB:
                case STVOUT_OUTPUT_ANALOG_YUV:
                case STVOUT_OUTPUT_ANALOG_YC:
                case STVOUT_OUTPUT_ANALOG_CVBS:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(GENGAM_DSPCFG_OFFSET);

                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                      {
                        DspCfg &= ~((U32)GENGAM_DSP_CFG_DESL);
                      }
                    else
                     {
                        DspCfg |= GENGAM_DSP_CFG_DESL;
                     }
                    WriteRegBase32LE(GENGAM_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;

                default:
                    break;
            }
        }
    }
#endif /* STVMIX_GENGAMMA_7020 */

#ifdef STVMIX_GENGAMMA_5528

   U32   VosMiscCtl;


    if(Device_p->InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_5528)
    {
        for(i=0; i<Device_p->Outputs.NbConnect; i++)
        {
            switch(Device_p->Outputs.ConnectArray_p[i].Type)
            {
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED: /* DVO */
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register VOS_MISC_CTL to be updated */
#ifdef ST_OSLINUX
                    VosMiscCtl = ReadRegBase32LE((void*)(VOS_MISC_CTL));
#else
                    VosMiscCtl = ReadRegBase32LE(VOS_MISC_CTL);
#endif
                    VosMiscCtl &= ~((U32)VOS_MISC_CTL_DVO_IN_SEL_MSK);

                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                    {
                        VosMiscCtl |= VOS_MISC_CTL_DVO_IN_SEL_AUX;
                    }
                    else if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)
                    {
                        VosMiscCtl |= VOS_MISC_CTL_DVO_IN_SEL_MAI;
                    }
                    else
                    {
                        VosMiscCtl |= VOS_MISC_CTL_DVO_IN_SEL_VID;
                    }
#ifdef ST_OSLINUX
                    WriteRegBase32LE((void*)(VOS_MISC_CTL), VosMiscCtl);
#else
                    WriteRegBase32LE(VOS_MISC_CTL, VosMiscCtl);
#endif
                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;
                /* Internal 5528 DENC here only */
                case STVOUT_OUTPUT_ANALOG_RGB:
                case STVOUT_OUTPUT_ANALOG_YUV:
                case STVOUT_OUTPUT_ANALOG_YC:
                case STVOUT_OUTPUT_ANALOG_CVBS:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register VOS_MISC_CTL to be updated */
#ifdef ST_OSLINUX
                    VosMiscCtl = ReadRegBase32LE((void*)(VOS_MISC_CTL));
#else
                    VosMiscCtl = ReadRegBase32LE(VOS_MISC_CTL);
#endif
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                    {
                        VosMiscCtl |= VOS_MISC_CTL_DENC_AUX_SEL;
                    }
                    else
                    {
                        VosMiscCtl &= ~((U32)VOS_MISC_CTL_DENC_AUX_SEL);
                    }
#ifdef ST_OSLINUX
                    WriteRegBase32LE((void*)(VOS_MISC_CTL), VosMiscCtl);
#else
                    WriteRegBase32LE(VOS_MISC_CTL, VosMiscCtl);
#endif
                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;
                default:
                    break;
            }
        }
    }
#endif /* STVMIX_GENGAMMA_5528 */
#if defined (STVMIX_GENGAMMA_7710)||defined (STVMIX_GENGAMMA_7100)||defined (STVMIX_GENGAMMA_7109)
/*IC*/
     U32 DspCfg ;
    for(i=0; i<Device_p->Outputs.NbConnect; i++)
        {
            switch(Device_p->Outputs.ConnectArray_p[i].Type)
            {
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET);

                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                      {
                        DspCfg |= GENGAM_DSP_CFG_UP_SEL | GENGAM_DSP_CFG_DVP_REF;
                      }
                    else
                     {
                        DspCfg &= ~( ( (U32)GENGAM_DSP_CFG_UP_SEL|(U32)GENGAM_DSP_CFG_DVP_REF ) )  ;
                     }

                    WriteRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;

                case STVOUT_OUTPUT_HD_ANALOG_RGB:
                case STVOUT_OUTPUT_HD_ANALOG_YUV:
                case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED:
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_ANALOG_SVM:
                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    break;

                    /* Internal DENC here only */
                case STVOUT_OUTPUT_ANALOG_RGB:
                case STVOUT_OUTPUT_ANALOG_YUV:
                case STVOUT_OUTPUT_ANALOG_YC:
                case STVOUT_OUTPUT_ANALOG_CVBS:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET);

                    if(((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX2)
                      {
                        if(MainSelected)/*denc is already connected to mix 1 => cannot connect to mix2*/
                        {
                           return (ST_ERROR_BAD_PARAMETER);
                        }
                        else
                        {
                            DspCfg &= ~((U32)GENGAM_DSP_CFG_DESL);
                        }
                      }
                    else
                     {
                        DspCfg |= GENGAM_DSP_CFG_DESL;
                        MainSelected = TRUE;
                     }
                    WriteRegBase32LE(VOS_BASE_ADDRESS + GENGAM_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;
                default:
                    break;
            }
        }
#endif
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvmix_GenGammaSetScreen
Description : Configure register for output screen
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t  stvmix_GenGammaSetScreen(stvmix_Device_t * const Device_p)
{
    U32 AV0Y, AV0X, AVSX, AVSY, AVO, AVS;
    stvmix_HalGenGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

    /* Intermediate computed values  */
    AV0X = Device_p->ScreenParams.XStart + Device_p->XOffset;
    AV0Y = Device_p->ScreenParams.YStart + Device_p->YOffset;
    AVSX = AV0X + Device_p->ScreenParams.Width - 1;
    AVSY = AV0Y + Device_p->ScreenParams.Height - 1;

    /* Check that AVS & AVO can be computed */
    if((AV0X & MIX_U16_MASK) ||
       (AV0Y & MIX_U16_MASK) ||
       (AVSX & MIX_U16_MASK) ||
       (AVSY & MIX_U16_MASK))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Compute AVS */
    AVS = (AVSY << 16) + AVSX;
    AVO = (AV0Y << 16) + AV0X;

    if((GENGAM_AV_BC_MASK & AVO) || (GENGAM_AV_BC_MASK & AVS))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Store in register AV0 and AVS */
    WriteRegDev32LE((GENGAM_MIX_AVO), AVO);
    WriteRegDev32LE((GENGAM_MIX_AVS), AVS);

    /* Is background present on mixer block */
    if(Hal_p->GenGammaDepth_p[GENGAM_BKC_DEPTH] & GENGAM_TYPE_BKC)
    {
        /* Same for background color on MIXER1 only */
        WriteRegDev32LE((GENGAM_MIX_BCO), AVO);
        WriteRegDev32LE((GENGAM_MIX_BCS), AVS);
    }
    return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvmix_GenGammaTerm
Description : Terminate hal for generic gamma device
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_GenGammaTerm(stvmix_Device_t * const Device_p)
{
    stvmix_HalGenGamma_t * Hal_p;
#if defined (STVMIX_GENGAMMA_7710)|| defined (STVMIX_GENGAMMA_7100)|| defined (STVMIX_GENGAMMA_7109)||defined (STVMIX_GENGAMMA_7200)
          if((((U32)Device_p->InitParams.BaseAddress_p & GENGAM_MIX_BBA_MASK)== GENGAM_MIX_BBA_MIX1)&&(MainSelected))
          {
                MainSelected=FALSE; /*Denc is nomore connected to a MIX1*/
          }
#endif
    /* Set hal pointer */
    Hal_p = ((stvmix_HalGenGamma_t *)(Device_p->Hal_p));

    /* Free event handler. Should be no error */
    /* If error, nothing can be done. A warning could be emited */
    STEVT_Close(Hal_p->EvtHandle);

    /* Free semaphore */
    STOS_SemaphoreDelete(NULL,Hal_p->VSyncProcessed_p);

    /* Free reserved memory */
    STOS_MemoryDeallocate (Device_p->InitParams.CPUPartition_p, Device_p->Hal_p);

    /* Reset hal structure */
    Device_p->Hal_p = NULL;

    return;
}

/* End of gengamma.c */
