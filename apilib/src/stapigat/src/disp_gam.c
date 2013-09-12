/*******************************************************************************

File name   : disp_gam.c

Description : module allowing to display a graphic throught Gamma
 *            Warning: do not use along with STLAYER nor STAVMEM.
 *            Warning: - do not use along with STLAYER nor STAVMEM.
                       - DISP_GAM_HD and DUAL_DENC haven't to be defined together.

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
08 Mar 2002        Created                                           MB
05 Jul 2002        Use data file name path                           HSdLM
30 Oct 2002        Rename DisplayPicture by GammaDisplay             HSdLM
20 Jan 2003        Add scantype parameter for progressive display    BS
28 Apr 2003        Remove STAVMEM dependency                         HSdLM
03 Jun 2003        Add 5528 support                                  HSdLM
21 Jan 2004        Add espresso support                              AC
15 Sep 2004        Add ST40/OS21 support                             MH
*******************************************************************************/

/* uncomment define below to get some information printed */
#define DEBUG_PRINTS

/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
/*Linux porting */
#include "testcfg.h"

#ifdef USE_DISP_GAM
#include "stdevice.h"
#include "stsys.h"
#include "testtool.h"
#include "api_cmd.h"
#ifdef ST_OSLINUX
#include "iocstapigat.h"
#endif
#include "memory_map.h"

/* Variable ----------------------------------------------------------------- */

/* Private constants -------------------------------------------------------- */

#ifndef GAM_BITMAP_NAME
#define GAM_BITMAP_NAME "merou3.gam"
#endif

#if defined ST_5528
#define DUAL_DENC
#endif

#ifdef ST_OS21
#ifdef ST_5528
#define OS_SHIFT                    0x1C000000      /* ST5528_LMI_BASE Offset = 0xC0000000 - 0xA4000000 */
#else
#define OS_SHIFT                    0x00000000      /* ST5528_LMI_BASE Offset = 0xC0000000 - 0xA4000000 */
#endif
#endif /* ST_OS21 */
#ifdef ST_OS20
#define OS_SHIFT                    0x00000000
#endif /* ST_OS20 */
#ifdef ST_OSLINUX
#define OS_SHIFT                    0x00000000/*0x1C000000*/
#endif

#ifdef ST_OS21
#if defined (ST_7100)|| defined(ST_7109)
#define OS_SHIFT_2                   0xA0000000
#elif defined(ST_7200)
#define OS_SHIFT_2                   NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET
#else
#define OS_SHIFT_2                   0x00000000
#endif
#else
#define OS_SHIFT_2                   0x00000000
#endif /* ST_OS21 */

#if defined (ST_OSLINUX)
#if defined(ST_7200)
#define OS_SHIFT_1                   0xA0000000
#else
#define OS_SHIFT_1                   0x00000000
#endif
#endif

#if defined (mb376)
#ifdef ST_OS21
#define AVG_MEMORY_BASE_ADDRESS     0xA8000000
#endif /* ST_OS21 */
#ifdef ST_OS20
#define AVG_MEMORY_BASE_ADDRESS     0x47800000
#endif /* ST_OS20 */
#elif defined (espresso) || defined(mb391)
#define AVG_MEMORY_BASE_ADDRESS     0xC0800000   /*LMI Memory*/
#elif defined (mb295)
#define AVG_MEMORY_BASE_ADDRESS     0x60800000
#elif defined (mb411)
#define AVG_MEMORY_BASE_ADDRESS     0xA5480000
#elif defined (mb519)
#if defined(ST_OSLINUX)
#define AVG_MEMORY_BASE_ADDRESS     0xABA00000
#else
#define AVG_MEMORY_BASE_ADDRESS     AVMEM0_ADDRESS
#endif
#else
#define AVG_MEMORY_BASE_ADDRESS     SDRAM_BASE_ADDRESS   /* for mb290. Not supported: mb295, mb317 (GX1) */
#endif

#if defined (ST_7015)
#define GX1_BASE_ADDRESS            0x6000a000 /* GFX1 */
#define GX2_BASE_ADDRESS            0x6000a500 /* GFX2 */
#define GAM_MIX1_BASE_ADDRESS       0x6000a700 /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       0x6000a800 /* mixer 2 */
#define DSPCFG_CLK                  0x60006100
#define DISP_GAM_HD

#elif defined (ST_7020) && defined(mb290)
#define GX1_BASE_ADDRESS            0x7000a100 /* GDP1 */
#define GX2_BASE_ADDRESS            0x7000a200 /* GDP2 */
#define GAM_MIX1_BASE_ADDRESS       0x7000ac00 /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       0x7000ad00 /* mixer 2 */
#define DSPCFG_CLK                  0x70006100
#define DISP_GAM_HD

#elif defined (ST_7710)
#define GX1_BASE_ADDRESS            ST7710_GDP1_LAYER_BASE_ADDRESS  /* GDP1 */
#define GX2_BASE_ADDRESS            ST7710_GDP2_LAYER_BASE_ADDRESS  /* GDP2 */
#define GAM_MIX1_BASE_ADDRESS       ST7710_VMIX1_BASE_ADDRESS       /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       ST7710_VMIX2_BASE_ADDRESS      /* mixer 2 */

#ifdef STI7710_CUT2x
#define DSPCFG_CLK                  0x20103070
#else
#define DSPCFG_CLK                  0x20103c70
#endif
#define DISP_GAM_HD


#elif defined (ST_7100)
#define GX1_BASE_ADDRESS            ST7100_GDP1_LAYER_BASE_ADDRESS  /* GDP1 */
#define GX2_BASE_ADDRESS            ST7100_GDP2_LAYER_BASE_ADDRESS  /* GDP2 */
#define GAM_MIX1_BASE_ADDRESS       ST7100_VMIX1_BASE_ADDRESS       /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       ST7100_VMIX2_BASE_ADDRESS      /* mixer 2 */
#define PERIPH_BASE                 0xB8000000
#define VOS_BASE                    (PERIPH_BASE + 0x01005000)
#define DSPCFG_CLK      0x070
#define DISP_GAM_HD

#elif defined (ST_7109)
#define GX1_BASE_ADDRESS            ST7109_GDP1_LAYER_BASE_ADDRESS  /* GDP1 */
#define GX2_BASE_ADDRESS            ST7109_GDP2_LAYER_BASE_ADDRESS  /* GDP2 */
#define GAM_MIX1_BASE_ADDRESS       ST7109_VMIX1_BASE_ADDRESS       /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       ST7109_VMIX2_BASE_ADDRESS      /* mixer 2 */
#define DSPCFG_CLK                  0x70
#define DISP_GAM_HD

#elif defined (ST_7200)
#define DISP_GAM_HD
#if defined (ST_OSLINUX)

#define LINUX_COMPO_BASE_ADDRESS    (U32)CompoMap.MappedBaseAddress
#define LINUX_COMPO1_BASE_ADDRESS   (U32)Compo1Map.MappedBaseAddress
#define GX1_BASE_ADDRESS            (LINUX_COMPO_BASE_ADDRESS + 0x100) /* GDP1 */
#define GX2_BASE_ADDRESS            (LINUX_COMPO_BASE_ADDRESS + 0x200) /* GDP2 */
#define GX3_BASE_ADDRESS            (LINUX_COMPO_BASE_ADDRESS + 0x300) /* GDP3 */
#define GX4_BASE_ADDRESS            (LINUX_COMPO_BASE_ADDRESS + 0x400) /* GDP4 */
#define GX5_BASE_ADDRESS            (LINUX_COMPO1_BASE_ADDRESS + 0x100) /* GDP5 */

#define GAM_MIX1_BASE_ADDRESS       (LINUX_COMPO_BASE_ADDRESS + 0xC00)
#define GAM_MIX2_BASE_ADDRESS       (LINUX_COMPO_BASE_ADDRESS + 0xD00)
#define GAM_MIX3_BASE_ADDRESS       (LINUX_COMPO1_BASE_ADDRESS + 0xC00)      /* mixer 3 */
#else
#define GX1_BASE_ADDRESS            ST7200_GDP1_LAYER_BASE_ADDRESS  /* GDP1 */
#define GX2_BASE_ADDRESS            ST7200_GDP2_LAYER_BASE_ADDRESS  /* GDP2 */
#define GX3_BASE_ADDRESS            ST7200_GDP3_LAYER_BASE_ADDRESS  /* GDP3 */
#define GX4_BASE_ADDRESS            ST7200_GDP4_LAYER_BASE_ADDRESS  /* GDP4 */
#define GX5_BASE_ADDRESS            ST7200_GDP5_LAYER_BASE_ADDRESS  /* GDP5 */
#define GAM_MIX1_BASE_ADDRESS       ST7200_VMIX1_BASE_ADDRESS       /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       ST7200_VMIX2_BASE_ADDRESS      /* mixer 2 */
#define GAM_MIX3_BASE_ADDRESS       ST7200_VMIX3_BASE_ADDRESS      /* mixer 3 */
#endif /*ST_OSLINUX*/

#elif defined (ST_7020) && !defined(mb290) /* mb295 and stem7020 */
#define GX1_BASE_ADDRESS            0x6000a100 /* GDP1 */
#define GX2_BASE_ADDRESS            0x6000a200 /* GDP2 */
#define GAM_MIX1_BASE_ADDRESS       0x6000ac00 /* mixer 1 */
#define GAM_MIX2_BASE_ADDRESS       0x6000ad00 /* mixer 2 */
#define DSPCFG_CLK                  0x60006100
#define DISP_GAM_HD

#elif defined (ST_5528)


#ifdef ST_OSLINUX
extern MapParams_t   CompoMap;

#define LINUX_COMPO_BASE_ADDRESS    CompoMap.MappedBaseAddress
#define GX1_BASE_ADDRESS            (LINUX_COMPO_BASE_ADDRESS + 0x100) /* GDP1 */
#define GAM_MIX1_BASE_ADDRESS       (LINUX_COMPO_BASE_ADDRESS + 0xC00)
#define GAM_MIX2_BASE_ADDRESS       (LINUX_COMPO_BASE_ADDRESS + 0xD00)
#else
#define GX1_BASE_ADDRESS            ST5528_GDP1_LAYER_BASE_ADDRESS /* GDP1 */
#define GAM_MIX1_BASE_ADDRESS       ST5528_VMIX1_BASE_ADDRESS
#define GAM_MIX2_BASE_ADDRESS       ST5528_VMIX2_BASE_ADDRESS
#endif


#endif

#define DISPGAM_COLOR_TYPE_RGB565   0
#define DISPGAM_COLOR_TYPE_ARGB1555 6
#define DISPGAM_COLOR_TYPE_ARGB4444 7

#define GAM_MIX_CTL                 0x00
#define GAM_MIX_AVO                 0x28
#define GAM_MIX_AVS                 0x2C
#define GAM_MIX_BCO                 0x0C
#define GAM_MIX_BCS                 0x10
#define GAM_MIX_BKC                 0x04

/* only for MIX1 */
#define GAM_MIX_CRB                 0x34

#define GAM_GDP_PKZ                 0xFC

/* Macros ------------------------------------------------------------------- */

/* Types -------------------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
void Gamma_Display(BOOL InterlacedMode);
BOOL GammaDisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL DispGam_RegisterCmd(void);

typedef struct Bitmap_s
{
    U32 Width;
    U32 Height;
    U32 Pitch;
    U32 ColorType;
    void * Data1_p;
} Bitmap_t;

typedef struct Node_s
{
    U32 CTL;    /* ConTroL register */
    U32 AGC;    /* Alpha Gain Constant */
    U32 HSRC;   /* Horizontal Sample Rate Converter */
    U32 VPO;    /* View Port Offset */

    U32 VPS;    /* View Port Stop */
    U32 PML;    /* Pixmap Memory Location */
    U32 PMP;    /* Pixmap Memory Pitch */
    U32 SIZE;   /* pixmap memory SIZE */

    U32 VSRC;   /* Vertical Sample Rate Converter */
    U32 NVN;    /* Next Viewport Node */
    U32 KEY1;   /* color KEY 1 */
    U32 KEY2;   /* color KEY 2 */

    U32 HFP;        /* Horizontal Filter Pointer */
    U32 PPT;        /* Viewport properties */
    U32 Reserved14; /* Reserved */
    U32 Reserved15; /* Reserved */

    U32 HFC0;       /*  */
    U32 HFC1;       /*  */
    U32 HFC2;       /*  */
    U32 HFC3;       /*  */

    U32 HFC4;       /*  */
    U32 HFC5;       /*  */
    U32 HFC6;       /*  */
    U32 HFC7;       /*  */

    U32 HFC8;       /*  */
    U32 HFC9;       /*  */
    U32 PKZ;        /*  */
    U32 Reserved6;  /*  */

    U32 Reserved7;       /*  */
    U32 Reserved8;       /*  */
    U32 Reserved9;        /*  */
    U32 Reserved10;  /*  */

} Node_t;

/* Global variables  ------------------------------------------------------- */
#ifdef DISP_GAM_HD
    S32                                  XStartHD, YStartHD, ActiveAreaWidth, ActiveAreaHeigth;
#endif /* DISP_GAM_HD */

/* Private variables  ------------------------------------------------------- */
static BOOL GammaDisplayCalled0nce  = FALSE;
static U32  AVGMemAddress           = AVG_MEMORY_BASE_ADDRESS;
#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)
static U32  GammaStartAddress       = 0x00000000;
#else
static U32  GammaStartAddress       = AVG_MEMORY_BASE_ADDRESS;
#endif

/* Private Function prototypes ---------------------------------------------- */

ST_ErrorCode_t GUTIL_LoadBitmap(char *filename,
                                Bitmap_t * Bitmap_p);


/* Functions ---------------------------------------------------------------- */

void Gamma_Display(BOOL InterlacedMode)
{
    U32                                  XStartSD, YStartSD, Gam1Offset, Gam1Stop;
    char                                 FileName[80];
    static Bitmap_t                      Bitmap;
    static Node_t                        *Node1_p, *Node2_p;
#ifdef DISP_GAM_HD
    U32                                  Gam2Offset, Gam2Stop;
    static Node_t                        *Node3_p, *Node4_p;
#if defined (ST_7200)
    static Node_t                        *Node5_p, *Node6_p;
#endif
#if defined (ST_OSLINUX)
    void * Addr_p;
 #endif
#endif /* DISP_GAM_HD */
#ifdef ST_OSLINUX
ST_ErrorCode_t ErrCode;
#endif
/*    OpenDispGam();*/

    /* In case of recall disable mixer */
    /* MIX1 : 8 for GPD1, 1 for background */
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_CTL), 0x00000000);
#if defined (DISP_GAM_HD) || defined (DUAL_DENC)
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL), 0x00000000);
#endif /* DISP_GAM_HD */

    if(GammaDisplayCalled0nce == FALSE)
    {
        GammaDisplayCalled0nce = TRUE;

        /* Allocate 2 nodes (bot + top) */
        /* ---------------------------- */

#ifdef ST_OSLINUX
      ErrCode = STAPIGAT_AllocData(((6 * sizeof(Node_t)) + 256), 256, &Addr_p);
      AVGMemAddress = (U32)(Addr_p);
#endif

        /* Constraints: alignement 256, do not allocate from 0 to 3, CPU must see memory area */
        AVGMemAddress += 256;
        /* affect pointers */
        Node1_p = (Node_t*)AVGMemAddress;
        Node2_p = (Node_t*)((U8*)(Node1_p) + sizeof(Node_t));
        AVGMemAddress += 2 * sizeof(Node_t);
#ifdef DISP_GAM_HD
        Node3_p = (Node_t*)((U8*)(Node2_p) + sizeof(Node_t));
        Node4_p = (Node_t*)((U8*)(Node3_p) + sizeof(Node_t));
        AVGMemAddress += 2 * sizeof(Node_t);

#if defined (ST_7200)
        Node5_p = (Node_t*)((U8*)(Node4_p) + sizeof(Node_t));
        Node6_p = (Node_t*)((U8*)(Node5_p) + sizeof(Node_t));
        AVGMemAddress += 2 * sizeof(Node_t);
#endif

#endif /* #ifdef DISP_GAM_HD */

        AVGMemAddress += 4 * sizeof(Node_t);

        sprintf(FileName, "%s%s", StapigatDataPath, GAM_BITMAP_NAME);

        GUTIL_LoadBitmap(FileName,&Bitmap);
    }

    /* ... now structure bitmap is filled a so is SDRAM */
    printf("Configure node\n");

    /* configure nodes */
    Node1_p->CTL        = Bitmap.ColorType | ( (U32)1 << 31);       /* wait Vsync */
    Node1_p->AGC        = 0x00808080;
    Node1_p->HSRC       = 0x100;
    /* GAM_GFX_VPO must be equal to GAM_MX_AVO so that (0,0) of layer and mixer are aligned */
    /* VPO = (top left corner X) | ((top left corner Y)<<16) */

#if defined (ST_7015)
    XStartSD = 0x90; /*    h90 = VTG XStart = 144    */
    YStartSD = 0x2e; /*    h2e = VTG YStart = 46     */
#elif defined (ST_7020) || defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    /* To have all VTG mode working, get the biggest XStart, Ystart */
    XStartSD = 0x8a; /*    h8a = VTG XStart = 138    */
    YStartSD = 0x2a; /*    h2a = VTG YStart = 42     */
#elif defined (ST_7200)
    XStartSD = 0x84; /*    h8a = VTG XStart = 138    */
    YStartSD = 0x2e; /*    h2a = VTG YStart = 42     */
#endif

#ifdef DISP_GAM_HD
    Gam1Offset = XStartHD | (YStartHD<<16);
    Node1_p->VPO        = Gam1Offset;
    Node1_p->VPS        = ( XStartHD + Bitmap.Width -1) | (( YStartHD + Bitmap.Height -1)<<16);
    Gam2Offset = XStartSD | (YStartSD<<16);
#else
    Gam1Offset = XStartSD | (YStartSD<<16);
    Node1_p->VPO        = Gam1Offset;
    Node1_p->VPS        = ( XStartSD + Bitmap.Width -1) | (( YStartSD + Bitmap.Height -1)<<16);
#endif /* #ifdef DISP_GAM_HD */



    #if defined (ST_OSLINUX)
        Node1_p->PML        = (U32)(STAPIGAT_UserToKernel( (U32)Bitmap.Data1_p ) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress ;
    #else
        Node1_p->PML        = (U32)(Bitmap.Data1_p)+ OS_SHIFT - GammaStartAddress;
    #endif
#if defined (ST_7015)
    Node1_p->PMP        =Bitmap.Pitch;
    Node1_p->SIZE       =Bitmap.Width | (Bitmap.Height  << 16) ;
#elif defined (ST_7020) || defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    Node1_p->PMP        =Bitmap.Pitch*2;
    Node1_p->SIZE       =Bitmap.Width | (Bitmap.Height /2 << 16);
#elif defined (ST_7200)
    if ( InterlacedMode)
    {
        Node1_p->PMP        = Bitmap.Pitch*2;
        Node1_p->SIZE       =Bitmap.Width | (Bitmap.Height /2 << 16);
    }
    else
    {
        Node1_p->PMP        =Bitmap.Pitch;
        Node1_p->SIZE       =(Bitmap.Width | (Bitmap.Height /2 << 16))*2 ;
    }
#endif

    Node1_p->VSRC       =0x100 ;
    #if defined (ST_OSLINUX)
        Node1_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node2_p) - OS_SHIFT_1)  + OS_SHIFT - GammaStartAddress - OS_SHIFT_2 ;
    #else
        Node1_p->NVN        = (U32)Node2_p  + OS_SHIFT - GammaStartAddress - OS_SHIFT_2 ;
    #endif
    Node1_p->KEY1       = 0;
    Node1_p->KEY2       = 0;
    Node1_p->HFP        = 0;
    Node1_p->PPT        = 0;
    Node1_p->HFC0       = 0;
    Node1_p->HFC1       = 0;
    Node1_p->HFC2       = 0;
    Node1_p->HFC3       = 0;
    Node1_p->HFC4       = 0;
    Node1_p->HFC5       = 0;
    Node1_p->HFC6       = 0;
    Node1_p->HFC7       = 0;
    Node1_p->HFC8       = 0;
    Node1_p->HFC9       = 0;
    Node1_p->PKZ        = 0;
    *Node2_p = *Node1_p;

    #if defined (ST_OSLINUX)
        Node2_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node1_p) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
        Node2_p->PML        = (U32)(STAPIGAT_UserToKernel((U32)Bitmap.Data1_p ) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress + Bitmap.Pitch ;
    #else
        Node2_p->PML        = (U32)Bitmap.Data1_p + OS_SHIFT - GammaStartAddress + Bitmap.Pitch;
        Node2_p->NVN        = (U32)Node1_p + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
    #endif


#ifdef DISP_GAM_HD
    *Node3_p = *Node1_p;
    Node3_p->VPO        = Gam2Offset;
    Node3_p->VPS        = ( XStartSD + Bitmap.Width -1) | (( YStartSD + Bitmap.Height -1)<<16);
    #if defined (ST_OSLINUX)
        Node3_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node4_p)  - OS_SHIFT_1)  + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node3_p->PML        = (U32)(STAPIGAT_UserToKernel( (U32)Bitmap.Data1_p ) - OS_SHIFT_1 )+ OS_SHIFT - GammaStartAddress  ;
    #else
        Node3_p->NVN        = (U32)Node4_p  + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node3_p->PML        = (U32)Bitmap.Data1_p + OS_SHIFT - GammaStartAddress ;
    #endif

    *Node4_p = *Node3_p;
    #if defined (ST_OSLINUX)
        Node4_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node3_p) - OS_SHIFT_1 ) + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node4_p->PML        = (U32)(STAPIGAT_UserToKernel( (U32)Bitmap.Data1_p ) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress + Bitmap.Pitch ;
    #else
        Node4_p->NVN        = (U32)Node3_p  + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node4_p->PML        = (U32)Bitmap.Data1_p + OS_SHIFT - GammaStartAddress + Bitmap.Pitch;
    #endif

#if defined (ST_7200)
    *Node5_p = *Node3_p;
    Node5_p->VPO        = Gam2Offset;
    Node5_p->VPS        = ( XStartSD + Bitmap.Width -1) | (( YStartSD + Bitmap.Height -1)<<16);
    #if defined (ST_OSLINUX)
        Node5_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node6_p) - OS_SHIFT_1 ) + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node5_p->PML        = (U32)(STAPIGAT_UserToKernel( (U32)Bitmap.Data1_p ) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress  ;
    #else
        Node5_p->NVN        = (U32)Node6_p  + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node5_p->PML        = (U32)Bitmap.Data1_p + OS_SHIFT - GammaStartAddress ;
    #endif

    *Node6_p = *Node5_p;
    #if defined (ST_OSLINUX)
        Node6_p->NVN        = (U32)(STAPIGAT_UserToKernel(Node5_p)  - OS_SHIFT_1) + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node6_p->PML        = (U32)(STAPIGAT_UserToKernel( (U32)Bitmap.Data1_p ) - OS_SHIFT_1) + OS_SHIFT - GammaStartAddress + Bitmap.Pitch ;
    #else
        Node6_p->NVN        = (U32)Node5_p  + OS_SHIFT  - GammaStartAddress - OS_SHIFT_2;
        Node6_p->PML        = (U32)Bitmap.Data1_p + OS_SHIFT - GammaStartAddress + Bitmap.Pitch;
    #endif
#endif

#endif /* #ifdef DISP_GAM_HD */

    /* Handle progressive mode */
    if(InterlacedMode == FALSE)
    {
        Node1_p->PMP        =Bitmap.Pitch;
        Node1_p->SIZE       =Bitmap.Width | (Bitmap.Height  << 16) ;
        Node2_p->PMP        =Bitmap.Pitch;
        Node2_p->SIZE       =Bitmap.Width | (Bitmap.Height  << 16) ;
    }

#ifdef DEBUG_PRINTS
    printf("\n\n*-*-*-*-*-*-*- Gamma Display *-*-*-*-*-*-*-*-*-*-*-*-*-\n");

    printf("Bitmap Width     = 0x%08x\n", (U32)Bitmap.Width);
    printf("Bitmap Height    = 0x%08x\n", (U32)Bitmap.Height);
    printf("Bitmap Pitch     = 0x%08x\n", (U32)Bitmap.Pitch);
    printf("Bitmap ColorType = 0x%08x\n", (U32)Bitmap.ColorType);
    printf("Bitmap Data1_p   = 0x%08x\n\n", (U32)Bitmap.Data1_p);

    printf("Gamma Display: Node1_p = 0x%08x\n", (U32)Node1_p);
    printf("Gamma Display: Node2_p = 0x%08x\n", (U32)Node2_p);
    #ifdef DISP_GAM_HD
        printf("Gamma Display: Node3_p = 0x%08x\n", (U32)Node3_p);
        printf("Gamma Display: Node4_p = 0x%08x\n", (U32)Node4_p);
    #if defined (ST_7200)
        printf("Gamma Display: Node5_p = 0x%08x\n", (U32)Node5_p);
        printf("Gamma Display: Node6_p = 0x%08x\n", (U32)Node6_p);
    #endif

    #endif

    printf("ConTroL register                 CTL  = 0x%08x\n", (U32)Node1_p->CTL);
    printf("Alpha Gain Constant              AGC  = 0x%08x\n", (U32)Node1_p->AGC);
    printf("Viewport properties              PPT  = 0x%08x\n", (U32)Node1_p->PPT);
    printf("color KEY 1                      KEY1 = 0x%08x\n", (U32)Node1_p->KEY1);
    printf("color KEY 2                      KEY2 = 0x%08x\n", (U32)Node1_p->KEY2);
    printf("Horizontal Sample Rate Converter HSRC = 0x%08x\n", (U32)Node1_p->HSRC);
    printf("Vertical Sample Rate Converter   VSRC = 0x%08x\n", (U32)Node1_p->VSRC);
    printf("View Port Offset                 VPO  = 0x%08x\n", (U32)Node1_p->VPO);
    printf("View Port Stop                   VPS  = 0x%08x\n", (U32)Node1_p->VPS);
    printf("Pixmap Memory Location           PML  = 0x%08x\n", (U32)Node1_p->PML);
    printf("Pixmap Memory Pitch              PMP  = 0x%08x\n", (U32)Node1_p->PMP);
    printf("pixmap memory size               SIZE = 0x%08x\n", (U32)Node1_p->SIZE);
    printf("Next Viewport Node               NVN  = 0x%08x\n", (U32)Node1_p->NVN);
    printf("Horizontal Filter Pointer        HFP  = 0x%08x\n", (U32)Node1_p->HFP);

    printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n\n");
#endif

#ifdef DISP_GAM_HD
    /* Get the smallest VTG values to have all mode working (640*480P) */
    Gam1Stop   = (XStartHD + ActiveAreaWidth - 1) | (YStartHD + ActiveAreaHeigth - 1)<<16;
    /* Parameters computed for max size display: PAL square 576*768 */
    Gam2Stop   = (XStartSD + 768 - 1) | (YStartSD + 576 - 1)<<16;
#else
    Gam1Stop   = (XStartSD + 768 - 1) | (YStartSD + 576 - 1)<<16;
#endif /* #ifdef DISP_GAM_HD */

    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_AVO), Gam1Offset);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_AVS), Gam1Stop);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BCO), Gam1Offset);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BCS), Gam1Stop);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BKC), 0xff0000);
    /* 7020 : only for MIX1, with GDP1 */
    STSYS_WriteRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_CRB), 0x18);

    printf("GAM_MIX1_AVO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_AVO))) );
    printf("GAM_MIX1_AVS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_AVS))) );
    printf("GAM_MIX1_BCO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BCO))) );
    printf("GAM_MIX1_BCS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BCS))) );
    printf("GAM_MIX1_BKC= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_BKC))) );
    /* 7020 : only for MIX1, with GDP1 */
    printf("GAM_MIX1_CRB= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_CRB))) );

#ifdef DUAL_DENC
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVO), Gam1Offset);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVS), Gam1Stop);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCO), Gam1Offset);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCS), Gam1Stop);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BKC), 0x000000ff);
#elif defined DISP_GAM_HD
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVO), Gam2Offset);
    STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVS), Gam2Stop);
    #if defined (ST_7200)
        STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCO), Gam2Offset);
        STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCS), Gam2Stop);
        STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BKC), 0x00ff00ff);
        STSYS_WriteRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_CRB), 0x180);
        printf("GAM_MIX2_AVO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVO))) );
        printf("GAM_MIX2_AVS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_AVS))) );
        printf("GAM_MIX2_BCO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCO))) );
        printf("GAM_MIX2_BCS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BCS))) );
        printf("GAM_MIX2_BKC= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_BKC))) );
        printf("GAM_MIX2_CRB= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_CRB))) );

        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_AVO, Gam2Offset);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_AVS, Gam2Stop);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BCO, Gam2Offset);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BCS, Gam2Stop);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BKC, 0xff00ff);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_CRB, 0x18);
        printf("GAM_MIX3_AVO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_AVO)) );
        printf("GAM_MIX3_AVS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_AVS)) );
        printf("GAM_MIX3_BCO= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BCO)) );
        printf("GAM_MIX3_BCS= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BCS)) );
        printf("GAM_MIX3_BKC= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_BKC)) );
        printf("GAM_MIX3_CRB= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_CRB)) );

    #endif

#endif /* #ifdef DUAL_DENC */



#if defined (ST_7015)
    STSYS_WriteRegDev32LE(DSPCFG_CLK, 0x00000008); /* INVCLK */
#elif defined (ST_7020) || defined (ST_7710)
    /* let MIX2 out to DENC */
    /* enable AUXPIXCLK when GDP2 is on mixer 2. */
#if defined (ST_7020)
    STSYS_WriteRegDev32LE(DSPCFG_CLK, 0x00000028);
#else
    STSYS_WriteRegDev32LE(DSPCFG_CLK,  0x0000000);
#endif
    STSYS_WriteRegDev32LE((U32)GX1_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
    STSYS_WriteRegDev32LE((U32)GX2_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
#elif defined (ST_7100)|| defined (ST_7109)
    STSYS_WriteRegDev32LE((U32)VOS_BASE_ADDRESS + DSPCFG_CLK, 0x0000000);
    STSYS_WriteRegDev32LE((U32)GX1_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
    STSYS_WriteRegDev32LE((U32)GX2_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
#elif defined (ST_7200)
    STSYS_WriteRegDev32LE((U32)GX4_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
    STSYS_WriteRegDev32LE((U32)GX5_BASE_ADDRESS + GAM_GDP_PKZ, 0); /* reset bit4 : pixclk*2 */
#elif defined (ST_5528)
    STSYS_WriteRegDev32LE((U32)GX1_BASE_ADDRESS + GAM_GDP_PKZ, 0x10); /* pixclk */
#endif

    /* Enable the display */
    #if defined (ST_OSLINUX)
        (((Node_t*)(GX1_BASE_ADDRESS))->NVN) = ((STAPIGAT_UserToKernel((U32)Node2_p) - OS_SHIFT_1 )+ OS_SHIFT - GammaStartAddress - OS_SHIFT_2);
    #else
        (((Node_t*)(GX1_BASE_ADDRESS))->NVN) = (U32)Node2_p + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
    #endif

#ifdef DISP_GAM_HD
  #if defined (ST_7710)
    (((Node_t*)(GX2_BASE_ADDRESS))->NVN) = (U32)Node4_p + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
  #endif
#endif /* #ifdef DISP_GAM_HD */

#if defined (ST_7200)
    #if defined (ST_OSLINUX)
        (((Node_t*)(GX4_BASE_ADDRESS))->NVN) = ((STAPIGAT_UserToKernel((U32)Node4_p) - OS_SHIFT_1 ) + OS_SHIFT - GammaStartAddress - OS_SHIFT_2);
        (((Node_t*)(GX5_BASE_ADDRESS))->NVN) = ((STAPIGAT_UserToKernel((U32)Node6_p) - OS_SHIFT_1 ) + OS_SHIFT - GammaStartAddress - OS_SHIFT_2);
    #else
        (((Node_t*)(GX4_BASE_ADDRESS))->NVN) = (U32)Node4_p + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
        (((Node_t*)(GX5_BASE_ADDRESS))->NVN) = (U32)Node6_p + OS_SHIFT - GammaStartAddress - OS_SHIFT_2;
    #endif

#endif

#if defined (ST_7015)
    STSYS_WriteRegDev32LE((U32)GAM_MIX1_BASE_ADDRESS + GAM_MIX_CTL, 0x00010010);
    STSYS_WriteRegDev32LE((U32)GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL, 0x00000020);
#elif defined (ST_7020) || defined (ST_5528)|| defined (ST_7710) || defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)

 /* MIX1 : 8 for GPD1, 1 for background */
#if defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)
    STSYS_WriteRegDev32LE((U32)GAM_MIX1_BASE_ADDRESS + GAM_MIX_CTL, 0x00000008);
#else
    STSYS_WriteRegDev32LE((U32)GAM_MIX1_BASE_ADDRESS + GAM_MIX_CTL, 0x00000009);
#endif /* ST_7710 || ST_7100 */
#if defined (DUAL_DENC)
/* MIX2 : */
#if defined (ST_7710)|| defined (ST_7100)|| defined (ST_7109)
    STSYS_WriteRegDev32LE((U32)GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL, 0x00000008);
#else
    STSYS_WriteRegDev32LE((U32)GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL, 0x00000009);
#endif /* ST_7710 */
#elif defined (DISP_GAM_HD)
    #if defined (ST_7200)
        STSYS_WriteRegDev32LE((U32)GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL, 0x00000040);
        STSYS_WriteRegDev32LE((U32)GAM_MIX3_BASE_ADDRESS + GAM_MIX_CTL, 0x00000009);
    #else
        STSYS_WriteRegDev32LE((U32)GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL, 0x00000010);
    #endif
#endif /* #ifdef DUAL_DENC */

     printf("GAM_MIX1_CTL= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX1_BASE_ADDRESS + GAM_MIX_CTL))) );
     printf("GAM_MIX2_CTL= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX2_BASE_ADDRESS + GAM_MIX_CTL))) );
#if defined (ST_7200)
     printf("GAM_MIX3_CTL= 0x%x\n", (U32)(STSYS_ReadRegDev32LE((U32)(GAM_MIX3_BASE_ADDRESS + GAM_MIX_CTL))) );
#endif

#endif /* if defined (ST_7020) || defined (ST_5528) */
} /* end of Gamma_Display() */

/* Private types/constants ----------------------------------------- */
/* Gamma File type.                                                  */
#define GAMMA_FILE_ACLUT44   0x8C
#define GAMMA_FILE_ACLUT88   0x8D
#define GAMMA_FILE_ACLUT8    0x8B
#define GAMMA_FILE_CLUT8     0x8E
#define GAMMA_FILE_RGB565    0x80
#define GAMMA_FILE_ARGB1555  0x86
#define GAMMA_FILE_ARGB4444  0x87
#define GAMMA_FILE_YCBCR422R 0x92

/* Header of Bitmap file                                             */
typedef struct GUTIL_GammaHeader_s
{
    U16 Header_size;    /* In 32 bit word      */
    U16 Signature;      /* usualy 0x444F       */
    U16 Type;           /* See previous define */
    U8  Properties;     /* Usualy 0x0          */
    U8  NbPict;         /* Number of picture   */
    U32 width;
    U32 height;
    U32 nb_pixel;       /* width*height        */
    U32 zero;           /* Usualy 0x0          */
} GUTIL_GammaHeader_t;

ST_ErrorCode_t GUTIL_LoadBitmap(char *filename,
                                Bitmap_t * Bitmap_p)
{
    FILE *fstream;                      /* File handle for read operation          */
    U32 ExpectedSize;
    U32  size;                          /* Used to test returned size for a read   */
    U32 dummy[2];                       /* used to read colorkey, but not used     */
    GUTIL_GammaHeader_t Gamma_Header;   /* Header of Bitmap file                   */
    U32 Signature;
    ST_ErrorCode_t ErrCode;
    #if defined (ST_OSLINUX)
    void * Addr_p;
    #endif

    ErrCode   = ST_NO_ERROR;
    Signature = 0x0;

    /* Check parameter*/
    if ( (filename == NULL) ||
         (Bitmap_p == NULL)
        )
    {
        printf("Error: Null Pointer, bad parameter\n");
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Open file handle                                              */
    if (ErrCode == ST_NO_ERROR)
    {
    printf("Opening: %s\n", filename);
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            printf("Unable to open \'%s\'\n", filename );
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read Header                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        printf("Reading file Header ... \n");
        ExpectedSize = sizeof(GUTIL_GammaHeader_t);
        size = fread((void *)&Gamma_Header, 1, ExpectedSize, fstream);
        if (size != ExpectedSize)
        {
            printf("Read error %d byte instead of %d\n",
                          size, ExpectedSize);
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }
    printf ("\n/* Read Header*/\n");
    /* Check Header                                                  */
    if (ErrCode == ST_NO_ERROR)
    {
        if ( ((Gamma_Header.Header_size != 0x6) &&
              (Gamma_Header.Header_size != 0x8)   ) ||
             (Gamma_Header.Properties   != 0x0    ) ||
             (Gamma_Header.zero         != 0x0    ) )
        {
            printf("Read %d waited 0x6 or 0x8\n",
                         Gamma_Header.Header_size);
            printf("Read %d waited 0x0\n",
                         Gamma_Header.Properties);
            printf("Read %d waited 0x0\n",
                         Gamma_Header.zero);
            printf("Not a valid file (Header corrupted)\n");
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
        else
        {
            /* For historical reason, NbPict = 0, means that one      */
            /* picture is on the file, so updae it.                   */
            if (Gamma_Header.NbPict == 0)
            {
                Gamma_Header.NbPict = 1;
            }
        }
    }

    /* If present read colorkey but do not use it.                   */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Header_size == 0x8)
        {
            /* colorkey are 2 32-bits word, but it's safer to use    */
            /* sizeof(dummy) when reading. And to check that it's 4. */
            size = fread((void *)dummy, 1, sizeof(dummy), fstream);
            if (size != 4)
            {
                printf("Read error %d byte instead of %d\n", size,4);
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables          */
    /* And do bitmap allocation                                      */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;

        /* Configure in function of the bitmap type                  */
        switch (Gamma_Header.Type)
        {
        case GAMMA_FILE_RGB565 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = DISPGAM_COLOR_TYPE_RGB565;
            Bitmap_p->Pitch =  Bitmap_p->Width * 2;
            break;
        }
        case GAMMA_FILE_ARGB1555 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = DISPGAM_COLOR_TYPE_ARGB1555;
            Bitmap_p->Pitch =  Bitmap_p->Width * 2;
            break;
        }
        case GAMMA_FILE_ARGB4444 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = DISPGAM_COLOR_TYPE_ARGB4444;
            Bitmap_p->Pitch =  Bitmap_p->Width * 2;
            break;
        }
        default :
        {
            printf("Type not supported 0x%08x\n",
                          Gamma_Header.Type);
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
        }
        } /* switch (Gamma_Header.Type) */
    }

    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Signature !=  Signature)
        {
            printf("Error: Read 0x%08x, Waited 0x%08x\n",
                          Gamma_Header.Signature, Signature);
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }
#if defined (ST_OSLINUX)
                /*
        size = fread (STAVMEM_VirtualToCPU(*DataAdd_p_p,&VirtualMapping), 1, AllocBlockParams.Size, fstream_p);
        if (size != AllocBlockParams.Size)
        {
            LOCAL_PRINT(("Read error %d byte instead of %d !!!\n", size, AllocBlockParams.Size));
            Exit = TRUE;
        }
        else
        {
            LOCAL_PRINT(("ok\n"));
       }
       */
#endif

    /* Read bitmap                                                   */
    printf("Read bitmap");
    if (ErrCode == ST_NO_ERROR)
    {
        ExpectedSize = (Bitmap_p->Pitch)*(Bitmap_p->Height)*Gamma_Header.NbPict;

#ifdef ST_OSLINUX

        ErrCode = STAPIGAT_AllocData((ExpectedSize + 256), 256, &Addr_p);
#endif
        /* align on 256 */
        #if !defined (ST_OSLINUX)
            AVGMemAddress = (AVGMemAddress + 255 ) & ~255;
            Bitmap_p->Data1_p = (void *)AVGMemAddress;
        #else
            AVGMemAddress = ((U32)Addr_p);
            AVGMemAddress = (AVGMemAddress + 255 ) & ~255;
            Bitmap_p->Data1_p = AVGMemAddress;
        #endif
        #if !defined (ST_OSLINUX)
            printf("pitch:%d, height:%d, nbpict:%d -> Expected:%d in 0x%x (0x%x)\n", Bitmap_p->Pitch, Bitmap_p->Height, Gamma_Header.NbPict, ExpectedSize, (U32)Bitmap_p->Data1_p, AVG_MEMORY_BASE_ADDRESS);
        #else
            printf("pitch:%d, height:%d, nbpict:%d -> Expected:%d in 0x%x (0x%x)\n", Bitmap_p->Pitch, Bitmap_p->Height, Gamma_Header.NbPict, ExpectedSize, (U32)Bitmap_p->Data1_p, (U32)STAPIGAT_UserToKernel((U32)Bitmap_p->Data1_p));
        #endif

        size = fread (Bitmap_p->Data1_p, 1, ExpectedSize, fstream);
        Bitmap_p->Data1_p = (void *) ((U8*)Bitmap_p->Data1_p - OS_SHIFT_2);

        if (size != ExpectedSize)
        {
            printf("Read error %d byte instead of %d\n", size, ExpectedSize);
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
        fclose (fstream);
    }
    if (ErrCode == ST_NO_ERROR)
    {
        printf("GUTIL loaded correctly %s file\n",filename);
    }

    return ErrCode;

} /* GUTIL_LoadBitmap () */

/*-------------------------------------------------------------------------
 * Function : GammaDisplayCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
BOOL GammaDisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    S32 ScanType;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger(pars_p, 1, &ScanType);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected interlaced mode (default TRUE/FALSE)" );
        return(TRUE);
    }
#if defined(ST_7020) || defined(ST_7015) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)|| defined (ST_7200)
    RetErr = STTST_GetInteger(pars_p, 280, &XStartHD);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected XStartHD" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 46, &YStartHD);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected YStartHD" );
        return(TRUE);
    }
 #endif /* ST_7020 || ST_7015 || ST_7710 || ST_7100 */

    RetErr = STTST_GetInteger(pars_p, 640, &ActiveAreaWidth);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected ActiveAreaWidth" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 480, &ActiveAreaHeigth);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected ActiveAreaHeigth" );
        return(TRUE);
    }
    Gamma_Display((BOOL)ScanType);
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : DispGam_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DispGam_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr = STTST_RegisterCommand( "GammaDisplay", GammaDisplayCmd, "Gamma Display : <Interlaced mode(def TRUE/FALSE))>");
    if (RetErr)
    {
        printf("DispGam_RegisterCmd() \t: failed !\n");
    }
    else
    {
        printf("DispGam_RegisterCmd() \t: ok\n");
    }
    return(RetErr ? FALSE : TRUE);
} /* end of DispGam_RegisterCmd() */

#endif /* USE_DISP_GAM */

/* end of disp_gam.c */
