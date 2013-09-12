/*****************************************************************************

File name   : hvr_dis2.h

Description : Video display module HAL registers for omega 2 header file

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                 Name
----               ------------                 ----
15/05/00           Created                      Philippe Legeard
01 Feb 2001        Update for digital input     Jerome Audu
*****************************************************************************/

#ifndef __DISPLAY_REGISTERS_OMEGA2_MPEG_H
#define __DISPLAY_REGISTERS_OMEGA2_MPEG_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/*---------------------------------------------------------------------------- */
/* --------------------------- MPEG ChipSet ---------------------------------- */
/*---------------------------------------------------------------------------- */

/*- Main & Auxillary presentation Field Selection ----------------------------*/
#define VIDn_PFLD                      0x38
#define VIDn_PFLD_MASK                 0x0F
#define VIDn_APFLD                     0x3C
#define VIDn_APFLD_MASK                0x0F
/* toggle on chroma */
#define VIDn_TCEN_MASK                 1
#define VIDn_TCEN_EMP                  0
/* toggle on luma */
#define VIDn_TYEN_MASK                 1
#define VIDn_TYEN_EMP                  1
/* bottom/top for chroma */
#define VIDn_BTC_MASK                  1
#define VIDn_BTC_EMP                   2
/* bottom/top for luma */
#define VIDn_BTY_MASK                  1
#define VIDn_BTY_EMP                   3
/*------------------------------------------------------------------------*/

/*- Memory Mode registers ------------------------------------------------*/
#define VIDn_APMOD                     0x8C              /* auxillary presentation Memory mode  */
#define VIDn_APMOD_MASK                0x07FC
#define VIDn_PMOD                      0x84              /* Main Presentation Memory Mode */
#define VIDn_PMOD_MASK                 0x07FF
/* indicates pictures in secondary buffers */
/* Only available for STi7015 chips !!!    */
#define VIDn_MOD_SFB_MASK              1
#define VIDn_MOD_SFB_EMP               8
/* indicates pictures decoded in HD-PIP mode */
/* Only available for STi7020 chips VID1 & 2 */
#define VIDn_MOD_HPD_MASK              1
#define VIDn_MOD_HPD_EMP               9
/* indicates pictures decoded in HD-PIP mode */
/* is progressive                            */
/* Only available for STi7020 chips VID1 & 2 */
#define VIDn_MOD_HPS_MASK              1
#define VIDn_MOD_HPS_EMP               10
/* decimation */
#define VIDn_MOD_DEC_MASK              0xF
#define VIDn_MOD_DEC_EMP               2
#define VIDn_MOD_DEC_NONE              0x00
#define VIDn_MOD_DEC_FACTOR2           0x01
#define VIDn_MOD_DEC_FACTOR4           0x02
/* compression (only available on STi7015)   */
#define VIDn_MOD_COMP_MASK             3
#define VIDn_MOD_COMP_NONE             0x00
#define VIDn_MOD_COMP_LEVEL1           0x01
#define VIDn_MOD_COMP_LEVEL2           0x02
/*  Luma  scan type */
#define VIDn_MOD_PDL_MASK              1
#define VIDn_MOD_PDL_EMP               6
/* Chroma scan type */
#define VIDn_MOD_PDC_MASK              1
#define VIDn_MOD_PDC_EMP               7
/*------------------------------------------------------------------------*/

/*- Presentation Frame pointers registers --------------------------------*/
#define VIDn_APFP_32                   0x30              /* auxillary presentation Luma Frame pointer  */
#define VIDn_APCHP_32                  0x34              /* auxillary presentation chroma frame buffer */
#define VIDn_PFP_32                    0x28              /* Main Presentation Luma Frame Pointer       */
#define VIDn_PCHP_32                   0x2C              /* Main Presentation Chroma Buffer            */
/* mask */
#define VIDn_PRESENTATION_MASK         0x03FFFE00
/*------------------------------------------------------------------------*/

/*- Presentation other registers -----------------------------------------*/
#define VIDn_PFH                       0x70               /* Presentation Frame Height                  */
#define VIDn_PFH_MASK                  0x7F
#define VIDn_PFW                       0x6C               /* Presentation Frame Width                   */
/*------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------- */
/* --------------------------- Digital Input ChipSet ------------------------- */
/*---------------------------------------------------------------------------- */

/* Standard definition */

/*- Main & Auxillary presentation Field Selection ----------------------------*/
#define SDINn_PFLD                      0x38
#define SDINn_PFLD_MASK                 0x0F

/* toggle on chroma */
#define SDINn_TCEN_MASK                 1
#define SDINn_TCEN_EMP                  0
/* toggle on luma */
#define SDINn_TYEN_MASK                 1
#define SDINn_TYEN_EMP                  1
/* bottom/top for chroma */
#define SDINn_BTC_MASK                  1
#define SDINn_BTC_EMP                   2
/* bottom/top for luma */
#define SDINn_BTY_MASK                  1
#define SDINn_BTY_EMP                   3
/*------------------------------------------------------------------------*/

/*- Memory Mode registers ------------------------------------------------*/
#define SDINn_PMOD                      0x84              /* Main Presentation Memory Mode */
#define SDINn_PMOD_MASK                 0x003F
/* decimation */
#define SDINn_MOD_HDEC_MASK             3
#define SDINn_MOD_HDEC_EMP              4
/*------------------------------------------------------------------------*/

/*- Presentation Frame pointers registers --------------------------------*/
#define SDINn_PFP_32                    0x28              /* Main Presentation Luma Frame Pointer       */
#define SDINn_PCHP_32                   0x2C              /* Main Presentation Chroma Buffer            */
/* mask */
#define SDINn_PRESENTATION_MASK         0x03FFFE00
/*------------------------------------------------------------------------*/

/*- Presentation other registers -----------------------------------------*/
#define SDINn_PFH                       0x70               /* Presentation Frame Height                  */
#define SDINn_PFH_MASK                  0x7F
#define SDINn_PFW                       0x6C               /* Presentation Frame Width                   */
#define SDINn_PFW_MASK                  0xFF
/*------------------------------------------------------------------------*/

/* High Definition */

/*- Main & Auxillary presentation Field Selection ----------------------------*/
#define HDIN_PFLD                      0x38
#define HDIN_PFLD_MASK                 0x0F

/* toggle on chroma */
#define HDIN_TCEN_MASK                 1
#define HDIN_TCEN_EMP                  0
/* toggle on luma */
#define HDIN_TYEN_MASK                 1
#define HDIN_TYEN_EMP                  1
/* bottom/top for chroma */
#define HDIN_BTC_MASK                  1
#define HDIN_BTC_EMP                   2
/* bottom/top for luma */
#define HDIN_BTY_MASK                  1
#define HDIN_BTY_EMP                   3
/*------------------------------------------------------------------------*/

/*- Memory Mode registers ------------------------------------------------*/
#define HDIN_PMOD_32                   0x84              /* Main Presentation Memory Mode */
#define HDIN_PMOD_MASK                 0x00FC
/* decimation */
#define HDIN_MOD_VDEC_EMP              2
#define HDIN_MOD_HDEC_EMP              4
#define HDIN_MOD_PDC_EMP               7
#define HDIN_MOD_PDL_EMP               6
/*------------------------------------------------------------------------*/

/*- Presentation Frame pointers registers --------------------------------*/
#define HDIN_PFP_32                    0x28              /* Main Presentation Luma Frame Pointer       */
#define HDIN_PCHP_32                   0x2C              /* Main Presentation Chroma Buffer            */
/* mask */
#define HDIN_PRESENTATION_MASK         0x03FFFE00
/*------------------------------------------------------------------------*/

/*- Presentation other registers -----------------------------------------*/
#define HDIN_PFH                       0x70               /* Presentation Frame Height                  */
#define HDIN_PFH_MASK                  0x7F
#define HDIN_PFW                       0x6C               /* Presentation Frame Width                   */
#define HDIN_PFW_MASK                  0xFF
/*------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DISPLAY_REGISTERS_OMEGA2_MPEG_H */

/* End of hvr_dis2.h */
