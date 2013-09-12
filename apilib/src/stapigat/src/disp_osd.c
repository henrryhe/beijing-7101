/*******************************************************************************

File name   : disp_osd.c

Description : module allowing to display a graphic throught OSD
 *            Warning: do not use along with STLAYER nor STAVMEM.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
21 Feb 2002        Created                                           MB
14 Oct 2002        Add support for 5517                              HSdLM
30 Oct 2002        Rename DisplayPicture by OSDDisplay               HSdLM
28 Apr 2003        Remove STAVMEM dependency                         HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include "testcfg.h"

#ifdef USE_DISP_OSD
#include "stdevice.h"

#include "stsys.h"
#include "testtool.h"

#include "overscan.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define AVG_MEMORY_BASE_ADDRESS     SDRAM_BASE_ADDRESS

#define BASE_ADDRESS        0x00
#define OSD_BDW             0x92 /* OSD Boundary Weight Regiter address */
#define OSD_CFG             0x91 /* Register address */
#define OSD_CFG_LLG         0x10 /* double granularity for a 64Mbits space. */
#define OSD_CFG_NOR         0x02 /* Normal display mode mask*/
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517)
#define OSD_CFG_OSD_POL     0x20
#define OSD_CFG_REC_OSD     0x40
#else
#define VID_DCF             0x75 /* Regiter address */
#define VID_DCF_OSDPOL      0x04 /* Pol bit mask */
#endif
#define VID_OUT             0x90 /* Register address */
#define VID_OUT_NOS         0x08 /* Not OSD bit */
#define OSD_ACT             0x3E /* OSD Active Signal Regiter address */
#define OSD_ACT_OAM         0x40 /* Active signal mode mask*/
#define OSD_ACT_OAD         0x3F /* Active signal delay mask [5:0] */
#define OSD_OTP             0x2A  /* OSD Top Field Pointer Regiter address */
#define OSD_OBP             0x2B  /* OSD Bottom Field Pointer Regiter address */
#define OSD_CFG_ENA         0x04 /* Enable OSD plane display mask*/

/* Private Variables (static)------------------------------------------------ */

static U32  AVGMemAddress  = AVG_MEMORY_BASE_ADDRESS;

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : OSD_Display
Description : display a graphic with a hard configuration of OSD
Parameters  : none
Assumptions :
Limitations :
Returns     : none.
*******************************************************************************/
void OSD_Display(void)
{
    U32 * TopHeader;
    U32 * BotHeader;
    U32 * GhostHeader;
    U8  * TopPalette;
    U8  * BotPalette;
    U8  * TopData;
    U8  * BotData;
    U32 y;
    U8   * SrcAddr_p, * DestAddr_p;
    U32 SrcWidth, SrcHeight, SrcPitch, DestPitch;

    /* Allocate Block TOP for header + palette top + data top */
    /* ------------------------------------------------------ */
    /* Constraints: alignement 256, do not allocate from 0 to 3, CPU must see memory area */
    AVGMemAddress += 256;

    TopHeader  = (U32 *)AVGMemAddress;
    TopPalette = (U8 * )((U32)TopHeader + 2 * sizeof(U32));
    TopData    = (U8 * )((U32)TopPalette + sizeof(ThePaletteData));

    AVGMemAddress += 2 * sizeof(U32);         /* header  */
    AVGMemAddress += sizeof(ThePaletteData);  /* palette */
    AVGMemAddress += sizeof(TheBitmapData)/2; /* data */

    /* Allocate Block BOT for header + palette bot + data bot */
    /* ------------------------------------------------------ */
    AVGMemAddress = (AVGMemAddress + 255) & ~255; /* align on 256 bytes */

    BotHeader  = (U32 *)AVGMemAddress;
    BotPalette = (U8 * )((U32)BotHeader + 2 * sizeof(U32));
    BotData    = (U8 * )((U32)BotPalette + sizeof(ThePaletteData));

    AVGMemAddress += 2 * sizeof(U32);         /* header  */
    AVGMemAddress += sizeof(ThePaletteData);  /* palette */
    AVGMemAddress += sizeof(TheBitmapData)/2; /* data */

    GhostHeader= (U32 *)AVGMemAddress;

    /* copy palette/data top */
    /* --------------------- */
    memcpy(TopPalette, (void *)&(ThePaletteData[0]), sizeof(ThePaletteData));
    SrcAddr_p  = (U8 *)&(TheBitmapData[0]);
    SrcWidth   = BITMAP_WIDTH / 2;  /* 2 pix per byte (16 colors) */
    SrcHeight  = BITMAP_HEIGHT / 2; /* a line for two : BOTTOM only */
    SrcPitch   = BITMAP_WIDTH;      /* (BITMAP_WIDTH / 2) * 2 */
    DestAddr_p = TopData;
    DestPitch  = BITMAP_WIDTH / 2;
    for (y = 0; y < SrcHeight; y++)
    {
        memcpy( (void *) (DestAddr_p + (DestPitch * y)),
                (void *) (SrcAddr_p  + (SrcPitch  * y)),
                SrcWidth);
    }

    /* copy palette/data bot */
    /* --------------------- */
    memcpy(BotPalette, (void *)&(ThePaletteData[0]), sizeof(ThePaletteData));
    SrcAddr_p  = (U8 *)&(TheBitmapData[BITMAP_WIDTH / 2]);
    SrcWidth   = BITMAP_WIDTH / 2;  /* 2 pix per byte (16 colors) */
    SrcHeight  = BITMAP_HEIGHT / 2; /* a line for two : BOTTOM only */
    SrcPitch   = BITMAP_WIDTH;      /* (BITMAP_WIDTH / 2) * 2 */
    DestAddr_p = BotData;
    DestPitch  = BITMAP_WIDTH / 2;
    for (y = 0; y < SrcHeight; y++)
    {
        memcpy( (void *) (DestAddr_p + (DestPitch * y)),
                (void *) (SrcAddr_p  + (SrcPitch  * y)),
                SrcWidth);
    }

    /* set ghost header */
    /* ---------------- */
    * GhostHeader       = 0xffffffff;
    * (GhostHeader + 1) = 0xffffffff;

    /* set top and bottom headers */
    /* -------------------------- */
    {
    U32 YStart,YStop,NextHard,XStart,XStop;
    U32 word1, word2;
    NextHard = ((U32)GhostHeader & 0x0fffffff) / 16;
    XStart = 107;
    XStop = XStart + BITMAP_WIDTH -1;
    YStart = 18;
    YStop = YStart + (BITMAP_HEIGHT-1)/2;
    word1  = (0x25                      << 26)
            |(0                         << 25)
            |(YStart                    << 16)
            |(0                         << 12)
            |(((NextHard >> 4) & 0x07)  << 9)
            |(YStop                     << 0);
    word2  = (((NextHard >> 7) & 0x3F)  << 26)
            |(XStart                    << 16)
            |(((NextHard >> 13) & 0x3F) << 10)
            |(XStop                     << 0);
    STSYS_WriteRegMem32BE((void*)(TopHeader),word1);
    STSYS_WriteRegMem32BE((void*)((U8*)TopHeader + sizeof(U32)),word2);
    STSYS_WriteRegMem32BE((void*)(BotHeader),word1);
    STSYS_WriteRegMem32BE((void*)((U8*)BotHeader + sizeof(U32)),word2);
    }

    /* enable the display */
    /* ------------------ */
    {
    U8 Tmp;
    U8 Tmp1;
    U8 Tmp2;
    U32 Tmp32;

    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + OSD_BDW),32);
    Tmp = STSYS_ReadRegDev8((void *)(BASE_ADDRESS + OSD_CFG));
    Tmp |= OSD_CFG_LLG;
    Tmp |= OSD_CFG_NOR; /* no filter */

#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517)
    Tmp |= OSD_CFG_OSD_POL;
    Tmp |= OSD_CFG_REC_OSD;
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_CFG),Tmp);
#else
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_CFG),Tmp);
    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + VID_DCF),
            STSYS_ReadRegDev8((void *)(BASE_ADDRESS + VID_DCF))| VID_DCF_OSDPOL);
#endif

    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + VID_OUT),
            STSYS_ReadRegDev8((void *)(BASE_ADDRESS + VID_OUT))& ~VID_OUT_NOS);

    Tmp = STSYS_ReadRegDev8((void *)(BASE_ADDRESS + OSD_ACT));
    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + OSD_ACT), Tmp & ~OSD_ACT_OAM);
    Tmp = STSYS_ReadRegDev8((void *)(BASE_ADDRESS + OSD_ACT));
    Tmp1 = 0 & OSD_ACT_OAD;
    Tmp2 = Tmp & ~OSD_ACT_OAD;
    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + OSD_ACT),Tmp1 | Tmp2);
    Tmp32 = (U32)TopHeader &0x0fffffff;
    Tmp32 = Tmp32 / 256;
    Tmp1 = (U8)(Tmp32 >> 8) & 0x3f;
    Tmp2 = (U8)Tmp32 & 0xff;
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_OTP),Tmp1);
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_OTP),Tmp2);
    Tmp32 = (U32)BotHeader &0x0fffffff;
    Tmp32 = Tmp32 / 256;
    Tmp1 = (U8)(Tmp32 >> 8) & 0x3f;
    Tmp2 = (U8)Tmp32 & 0xff;
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_OBP),Tmp1);
    STSYS_WriteRegDev8((void*)(BASE_ADDRESS + OSD_OBP),Tmp2);
    Tmp = STSYS_ReadRegDev8((void *)(BASE_ADDRESS + OSD_CFG));
    STSYS_WriteRegDev8((void *)(BASE_ADDRESS + OSD_CFG),Tmp | OSD_CFG_ENA);
    }
    printf("OSD displayed : Width= %d, Height= %d, at address Top:0x%0x Bot 0x%0x\n",
             BITMAP_WIDTH, BITMAP_HEIGHT,
             (U32)TopHeader, (U32)BotHeader);

} /* end of OSD_Display() */


/*-------------------------------------------------------------------------
 * Function : OSDDisplayCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
BOOL OSDDisplayCmd( STTST_Parse_t *pars_p, char *result_sym_p )
{
    OSD_Display();
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : DispOSD_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DispOSD_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr = STTST_RegisterCommand( "OSDDisplay", OSDDisplayCmd, "OSD Display");
    if (RetErr)
    {
        printf( "DispOSD_RegisterCmd() \t: failed !\n");
    }
    else
    {
        printf( "DispOSD_RegisterCmd() \t: ok\n");
    }
    return(!RetErr);
} /* end of DispOSD_RegisterCmd() */
#endif /* USE_DISP_OSD */

/* End of disp_osd.c */
