
/****************************************************************************

File Name   : startup.c

Description : Generic tests startup

Copyright (C) ST Microelectronics 2005

References  :

Date                Modification                                     Name
----                ------------                                     ----
10 Apr 2002         Created                                          HSdLM
14 May 2002         Add support of 5516B/mb361                       HSdLM
12 Jun 2002         Make data memory size configurable from outside  HSdLM
04 Jul 2002         Add module to display image in video plane       BS
14 Oct 2002         Add support for 5517                             HSdLM
21 Nov 2002         Add FDMA support                                 HSdLM
02 Apr 2003         Add db573 support and 5528                       HSdLM
21 Jan 2004         Add support of espresso                          AC
03 dec 2004        Port to 5105 + 5301 + 8010, supposing same as 5100 TA+HG
15 Aug 2005        Add dupport for 5525                              IC
8 Sep 2006         Version modified to STAPIGAT-REL_1.0.51_WinCE_1.0 Noida WinCE Team
                   for WinCE platform
****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ST_OSLINUX
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#endif

#include "stos.h"

#include "testcfg.h" /* test specific dependency needs */
#include "stdevice.h"
#include "startup.h"
#include "stcommon.h"
#include "stsys.h"
#include "clboot.h"


#ifdef BENCHMARK_WINCESTAPI /* only ST_OSWINCE */
#define THREAD_INFO

#ifdef THREAD_INFO
#pragma comment(lib,"toolhelp.lib")
#include <tlhelp32.h>
#endif
#endif

#include "memory_map.h"

/* ---------------- Tools -------------- */

#ifndef USE_TSTREG
#ifdef DUMP_REGISTERS
#include "dumpreg.h"
#endif
#endif /* ! USE_TSTREG */

#ifdef USE_TSTMEM
#include "cltstmem.h"
#endif

#ifdef USE_VALID_DHRYSTONE
#include "cldhry.h"
#endif

#ifdef USE_TSTREG
#include "cltstreg.h"
#endif

#ifdef TRACE_UART
#ifdef USE_VALID_TRACE
#include "cltrace.h"
#else
#include "trace.h"
#endif
#endif /* TRACE_UART */

#ifdef USE_API_TOOL
#include "api_cmd.h"
#endif

#ifdef USE_DISP_GAM
#include "disp_gam.h"
#endif

#ifdef USE_DISP_GDMA
#include "disp_gdm.h"
#endif

#ifdef USE_DISP_OSD
#include "disp_osd.h"
#endif

#ifdef USE_DISP_VIDEO
#include "disp_vid.h"
#endif

#ifdef USE_HDD
#include "hdd.h"
#endif

#ifdef USE_SCART_SWITCH_CONFIG
#include "scartswt.h"
#endif

#ifdef USE_STEM_RESET
#include "stemrst.h"
#endif

#ifdef USE_VOUTSIM
#include "simvout.h"
#endif

#ifdef USE_VTGSIM
#include "simvtg.h"
#endif

#ifdef USE_VTGSTUB
#include "vtg_stub.h"
#endif

#ifdef USE_PROCESS_TOOL
#include "tt_task.h"
#endif

/* ---------------- Libraries ---------- */

#ifdef USE_AUDIO
#ifdef ST_7710
#include "aud_cmd.h"
#else
#include "audlx_cmd.h"
#endif
#endif

#ifdef USE_AVMEM
#include "clavmem.h"
#endif

#ifdef USE_BLIT
#include "blt_cmd.h"
#endif

#ifdef USE_CC
#include "cc_cmd.h"
#endif

#ifdef USE_CLKRV
#include "clclkrv.h"
#endif

#ifdef USE_COMPO
#include "compo_cmd.h"
#endif

#ifdef USE_DENC
#include "denc_cmd.h"
#endif

#ifdef USE_DISP
#include "disp_cmd.h"
#endif

#if defined(DVD_SECURED_CHIP)
#include "clmes.h"
#endif

#if defined(USE_EVT) || defined(USE_POLL)
#include "clevt.h"
#endif

#ifdef USE_EXTVIN
#include "evin_cmd.h"
#endif

#ifdef USE_FDMA
#if !defined ST_OSLINUX
#include "clfdma.h"
#endif
#endif

#ifdef USE_GPDMA
#include "clgpdma.h"
#endif

#ifdef USE_I2C
#include "cli2c.h"
#endif

#ifdef USE_INTMR
#include "clintmr.h"
#endif

#ifdef USE_LAYER
#include "lay_cmd.h"
#endif

#ifdef USE_PIO
#include "clpio.h"
#endif

#ifdef USE_PTI
#include "pti_cmd.h"
#endif

#ifdef USE_DEMUX
#include "demux_cmd.h"
#endif

#ifdef USE_PWM
#include "clpwm.h"
#endif

#ifdef USE_STCFG
#include "clcfg.h"
#endif

#ifdef USE_STLLI
#include "cllli.h"
#endif

#ifdef USE_TSMUX
#include "cltsmux.h"
#endif

#ifdef USE_TBX
#include "sttbx.h"
#include "cltbx.h"
#endif

#ifdef USE_TESTTOOL
#include "testtool.h"
#include "cltst.h"
#endif

#ifdef USE_TTX
#include "ttx_cmd.h"
#endif

#ifdef USE_TUNER
#include "tune_cmd.h"
#endif

#ifdef USE_UART
#include "cluart.h"
#endif

#ifdef USE_VBI
#include "vbi_cmd.h"
#endif

#ifdef USE_VIDEO
#include "vid_cmd.h"
#endif

#ifdef USE_VIN
#include "vin_cmd.h"
#endif

#ifdef USE_VMIX
#include "vmix_cmd.h"
#endif

#ifdef USE_VOUT
#include "vout_cmd.h"
#endif

#ifdef USE_VTG
#include "vtg_cmd.h"
#endif

#ifdef USE_HDMI
#include "hdmi_cmd.h"
#endif

#ifdef USE_LXLOADER
#include "lx_loader.h"
#endif

#ifdef ST_OSLINUX
#include "iocstapigat.h"
#endif

#ifdef ST_OSLINUX
#ifdef USE_REG
#include "reg_cmd.h"
#endif
#endif /* ST_OSLINUX */
/* Extern functions prototypes -------------------------------------------- */

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Interconnect registers for STB5118 (low cost STi5105) */
#if defined(SELECT_DEVICE_STB5118) || defined(ST_5188)

    #define   NHD2_CONFIG_BASE        0x20400000
    #define   CLOCK_REG_BASE          0x20300000

    #define   NHD2_INIT_1_PRIORITY    (NHD2_CONFIG_BASE + 0x00)
    #define   NHD2_INIT_2_PRIORITY    (NHD2_CONFIG_BASE + 0x04)
    #define   NHD2_INIT_3_PRIORITY    (NHD2_CONFIG_BASE + 0x08)
    #define   NHD2_INIT_4_PRIORITY    (NHD2_CONFIG_BASE + 0x0C)
    #define   NHD2_INIT_5_PRIORITY    (NHD2_CONFIG_BASE + 0x10)
    #define   NHD2_INIT_6_PRIORITY    (NHD2_CONFIG_BASE + 0x14)
    #define   NHD2_INIT_7_PRIORITY    (NHD2_CONFIG_BASE + 0x18)
    #define   NHD2_INIT_8_PRIORITY    (NHD2_CONFIG_BASE + 0x1C)
    #define   NHD2_INIT_1_LIMIT       (NHD2_CONFIG_BASE + 0x60)
    #define   NHD2_INIT_2_LIMIT       (NHD2_CONFIG_BASE + 0x64)
    #define   NHD2_INIT_3_LIMIT       (NHD2_CONFIG_BASE + 0x68)
    #define   NHD2_INIT_4_LIMIT       (NHD2_CONFIG_BASE + 0x6C)
    #define   NHD2_INIT_5_LIMIT       (NHD2_CONFIG_BASE + 0x70)
    #define   NHD2_INIT_6_LIMIT       (NHD2_CONFIG_BASE + 0x74)
    #define   NHD2_INIT_7_LIMIT       (NHD2_CONFIG_BASE + 0x78)
    #define   NHD2_INIT_8_LIMIT       (NHD2_CONFIG_BASE + 0x7C)
    #define   NHD2_TARG_1_PRIORITY    (NHD2_CONFIG_BASE + 0x80)
    #define   NHD2_TARG_2_PRIORITY    (NHD2_CONFIG_BASE + 0x84)

#endif

#if defined(ST_5188)
#define INTERCONNECT_BASE           0x20402000

#define CONFIG_CTRL_C		        0x00
#define CONFIG_CTRL_D		        0x04
#define CONFIG_CTRL_E		        0x08
#define CONFIG_CTRL_F		        0x0C
#define CONFIG_CTRL_G		        0x10
#define CONFIG_CTRL_H		        0x14

#define CONFIG_CONTROL_C (INTERCONNECT_BASE + CONFIG_CTRL_C)
#define CONFIG_CONTROL_D (INTERCONNECT_BASE + CONFIG_CTRL_D)
#define CONFIG_CONTROL_E (INTERCONNECT_BASE + CONFIG_CTRL_E)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + CONFIG_CTRL_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + CONFIG_CTRL_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + CONFIG_CTRL_H)

#endif

#ifndef NCACHE_PARTITION_SIZE
    #define NCACHE_PARTITION_SIZE   0x0080000
#endif
#ifndef NCACHE_SIZE
    #if defined (ST_5301) ||  defined (ST_5525)
        #define NCACHE_SIZE             0x0800000    /* 8MB */
    #else
        #define NCACHE_SIZE             NCACHE_PARTITION_SIZE    /* As defined in clavmem.h */
    #endif
#endif /* NCACHE_SIZE not defined */

#if defined(mb231)
#define BOARD_NAME                         "mb231"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0100000    /* 1 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0100000    /* 1 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb282b)
#define BOARD_NAME                         "mb282b"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb275)
#define BOARD_NAME                         "mb275"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb275_64)
#define BOARD_NAME                         "mb275_64"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb290)
#define BOARD_NAME                         "mb290"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x1B00000    /* 27 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb295)
#define BOARD_NAME                         "mb295"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x1B00000    /* 27 MBytes */
#endif /* DATA_MEMORY_SIZE */
#ifndef DATA2_MEMORY_SIZE
#define DATA2_MEMORY_SIZE                  0x1EB5000    /* 31 MBytes - 300KBytes */
#endif /* DATA2_MEMORY_SIZE */

#elif defined(mb314)
#define BOARD_NAME                         "mb314"
#ifdef UNIFIED_MEMORY
#ifndef SYSTEM_SIZE
#if defined(ST_7020)
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes for use with 7020 STEM*/
#else
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0100000    /* 1 MBytes */
#endif /* DATA_MEMORY_SIZE */
#else /* UNIFIED_MEMORY */
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */
#endif /* UNIFIED_MEMORY */

#elif defined(mb376)
#define BOARD_NAME                         "mb376"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0380000    /* 3.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#if defined(ST_7020)
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#else
#define DATA_MEMORY_SIZE                   0x0800000    /* 8 MBytes */
#endif
#endif /* DATA_MEMORY_SIZE */

#elif defined(espresso)
#define BOARD_NAME                         "espresso"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0380000    /* 3.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0800000    /* 8 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb390)
#define BOARD_NAME                         "mb390"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb400)
#define BOARD_NAME                         "mb400"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(maly3s)
#define BOARD_NAME                         "maly3s"
#ifdef STVID_USE_REDUCED_SYSTEM_SIZE
#define SYSTEM_SIZE                        0x00E7000    /* 1.0 MBytes */
#define DATA_MEMORY_SIZE                   0x0080000    /* 0.5 MBytes */
#else /* !STVID_USE_REDUCED_SYSTEM_SIZE */
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x01DFF00    /* 1.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0020000    /* 1.0 MBytes */
#endif /* DATA_MEMORY_SIZE */
#endif /* STVID_USE_REDUCED_SYSTEM_SIZE */

#elif defined(SAT5107)
#define BOARD_NAME                         "SAT5107"
#ifdef STVID_USE_REDUCED_SYSTEM_SIZE
#define SYSTEM_SIZE                        0x00C0000    /* 768 KBytes */
#define DATA_MEMORY_SIZE                   0x0010000    /* 0.5 MBytes */
#else /* !STVID_USE_REDUCED_SYSTEM_SIZE */
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x01DFF00    /* 1.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0020000    /* 1.0 MBytes */
#endif /* DATA_MEMORY_SIZE */
#endif /* STVID_USE_REDUCED_SYSTEM_SIZE */


#elif defined(mb436)
#define BOARD_NAME                         "mb436"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(DTT5107)
#define BOARD_NAME                         "DTT5107"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb411)
#define BOARD_NAME                         "mb411"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x00600000   /* 6.0 MBytes */
#endif /* SYSTEM_SIZE */
/*  no DataPartition, we will allocate buffer to load streams in the STAVMEM_SYS_MEMORY */
/*  in vid_util.c instead of memory allocate in DataPartition_p */

#elif defined(mb519)
#define BOARD_NAME                         "mb519"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        SH4_SYSTEM_PARTITION_MEMORY_SIZE
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   NCACHE_PARTITION_MEMORY_SIZE
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb424)
#define BOARD_NAME                         "mb424"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb421)
#define BOARD_NAME                         "mb421"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb428)
#define BOARD_NAME                         "mb428"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif

#elif defined(mb457)
#define BOARD_NAME                         "mb457"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0200000    /* 2.0 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0400000    /* 4.0 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb391)
#define BOARD_NAME                         "mb391"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0600000    /* 6 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0800000    /* 8 MBytes */
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb361)
#define BOARD_NAME                         "mb361"
#ifdef UNIFIED_MEMORY
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0300000    /* 3 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x07FFE00     /* 8 MBytes - 512 bytes */
#endif /* DATA_MEMORY_SIZE */
#else /* UNIFIED_MEMORY */
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */
#endif /* UNIFIED_MEMORY */

#elif defined(mb382)
#define BOARD_NAME                         "mb382"
#ifdef UNIFIED_MEMORY
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0300000    /* 3 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x07FFE00     /* 8 MBytes - 512 bytes */
#endif /* DATA_MEMORY_SIZE */
#else /* UNIFIED_MEMORY */
#ifndef SYSTEM_SIZE
#if defined(ST_7020)
#define SYSTEM_SIZE                        0x0380000    /* 3.5 MBytes for use with 7020 STEM*/
#else
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#if defined(ST_7020)
#define DATA_MEMORY_SIZE                   0x1900000    /* 25 MBytes for use with 7020 STEM */
#else
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif
#endif /* DATA_MEMORY_SIZE */
#endif /* UNIFIED_MEMORY */

#elif defined(mb317a) || defined(mb317b)
#define BOARD_NAME                         "mb317"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0280000    /* 2.5 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0A00000    /* 10 MBytes */
#endif /* DATA_MEMORY_SIZE */
#endif /* mb317a || mb317b */


/*#########################################################################
 *                         Chip specific values
 *#########################################################################*/

#ifdef STB_FRONTEND_5510
#define STB_FRONTEND "STi5510"
#endif
#ifdef STB_BACKEND_5510
#define STB_BACKEND  "STi5510"
#endif

#ifdef STB_FRONTEND_5512
#define STB_FRONTEND "STi5512"
#endif
#ifdef STB_BACKEND_5512
#define STB_BACKEND  "STi5512"
#endif

#ifdef STB_FRONTEND_5508
#define STB_FRONTEND "STi5508"
#endif
#ifdef STB_BACKEND_5508
#define STB_BACKEND  "STi5508"
#endif

#ifdef STB_FRONTEND_5518
#define STB_FRONTEND "STi5518"
#endif
#ifdef STB_BACKEND_5518
#define STB_BACKEND  "STi5518"
#endif

#ifdef STB_FRONTEND_5528
#define STB_FRONTEND "STi5528"
#endif
#ifdef STB_BACKEND_5528
#define STB_BACKEND  "STi5528"
#endif

#ifdef STB_BACKEND_DELTAPHI_HE
#define STB_BACKEND  "Deltaphi_HE"
#endif

#ifdef STB_FRONTEND_5100
#define STB_FRONTEND "STi5100"
#endif
#ifdef STB_BACKEND_5100
#define STB_BACKEND  "STi5100"
#endif

#ifdef STB_FRONTEND_5514
#define STB_FRONTEND "STi5514"
#endif
#ifdef STB_BACKEND_5514
#define STB_BACKEND  "STi5514"
#endif

#ifdef STB_FRONTEND_5516
#define STB_FRONTEND "STi5516"
#endif
#ifdef STB_BACKEND_5516
#define STB_BACKEND  "STi5516"
#endif

#ifdef STB_FRONTEND_5517
#define STB_FRONTEND "STi5517"
#endif
#ifdef STB_BACKEND_5517
#define STB_BACKEND  "STi5517"
#endif

#ifdef STB_FRONTEND_TP3
#define STB_FRONTEND "ST20 TP3"
#endif
#ifdef STB_BACKEND_TP3
#define STB_BACKEND  "ST20 TP3"
#endif

#ifdef STB_FRONTEND_7015
#define STB_FRONTEND "STi7015"
#endif
#ifdef STB_BACKEND_7015
#define STB_BACKEND  "STi7015"
#endif

#ifdef STB_FRONTEND_7020
#define STB_FRONTEND "STi7020"
#endif
#ifdef STB_BACKEND_7020
#define STB_BACKEND  "STi7020"
#endif

#ifdef STB_FRONTEND_7710
#define STB_FRONTEND "STi7710"
#endif
#ifdef STB_BACKEND_7710
#define STB_BACKEND  "STi7710"
#endif

#ifdef STB_FRONTEND_5105
#define STB_FRONTEND "STi5105"
#endif
#ifdef STB_BACKEND_5105
#define STB_BACKEND  "STi5105"
#endif

#ifdef STB_FRONTEND_5188
#define STB_FRONTEND "STi5188"
#endif
#ifdef STB_BACKEND_5188
#define STB_BACKEND  "STi5188"
#endif

#ifdef STB_FRONTEND_5107
#define STB_FRONTEND "STi5107"
#endif
#ifdef STB_BACKEND_5107
#define STB_BACKEND  "STi5107"
#endif

#ifdef STB_FRONTEND_5301
#define STB_FRONTEND "STi5301"
#endif
#ifdef STB_BACKEND_5301
#define STB_BACKEND  "STi5301"
#endif

#ifdef STB_FRONTEND_8010
#define STB_FRONTEND "STi8010"
#endif
#ifdef STB_BACKEND_8010
#define STB_BACKEND  "STi8010"
#endif

#ifdef STB_FRONTEND_5557
#define STB_FRONTEND "STi5557"
#endif
#ifdef STB_BACKEND_5557
#define STB_BACKEND  "STi5557"
#endif

#ifdef STB_FRONTEND_5525
#define STB_FRONTEND "STi5525"
#endif
#ifdef STB_BACKEND_5525
#define STB_BACKEND  "STi5525"
#endif

#ifdef STB_FRONTEND_7100
#define STB_FRONTEND "STi7100"
#endif
#ifdef STB_BACKEND_7100
#define STB_BACKEND  "STi7100"
#endif

#ifdef STB_FRONTEND_7109
#define STB_FRONTEND "STi7109"
#endif
#ifdef STB_BACKEND_7109
#define STB_BACKEND  "STi7109"
#endif

#ifdef STB_FRONTEND_7200
#define STB_FRONTEND "STi7200"
#endif

#ifdef STB_FRONTEND_GX1
#define STB_FRONTEND "GX1"
#endif
#ifdef STB_BACKEND_GX1
#define STB_BACKEND  "GX1"
#endif


/* Private variables (static) --------------------------------------------- */

/* TEST PERFORMANCE AVANT INIT */
#if defined(ST_OS21) || defined(ST_OSWINCE)
static unsigned char*    pDest; /* SYSTEM_SIZE */
static unsigned char*    pSrc;  /* SYSTEM_SIZE */
#ifdef ST_OSWINCE
HANDLE MCThread;
#endif
#endif



/* Allow room for OS20 segments in internal memory */
#ifdef ST_OS20
#if defined(mb290) || (defined(mb314) && defined(ST_7020))
static unsigned char    InternalBlock[ST5514_INTERNAL_MEMORY_SIZE-1200];
#elif defined(mb382) && defined(ST_7020)
static unsigned char    InternalBlock[ST5517_INTERNAL_MEMORY_SIZE-1200];
#elif defined(mb376) && defined(ST_7020)
static unsigned char    InternalBlock[ST5528_INTERNAL_MEMORY_SIZE-1200];
#else
static unsigned char    InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];
#endif
#pragma ST_section      (InternalBlock, "internal_section")
#endif /* ST_OS20 */

#if !defined ST_OSLINUX && !defined ST_OSWINCE
static unsigned char    ExternalBlock[SYSTEM_SIZE];
#pragma ST_section      (ExternalBlock, "system_section")
#elif defined(ST_OSWINCE)
static unsigned char*    ExternalBlock; /* SYSTEM_SIZE */
#endif/* ST_OSWINCE */
#if !defined ST_OSLINUX
#if defined (mb411) || defined (mb519)
    /* no DataBlock definition */
    /* fixed NCacheMemory mapping */
    #if defined(ST_7200)
    /* Check memory mapping in mb519.h */
    static unsigned char    *NCacheMemory = (unsigned char*)(NCACHE_PARTITION_ADDRESS);
    #elif defined(ST_7109)
    /* Check memory mapping in the clavmem.h */
    static unsigned char    *NCacheMemory = (unsigned char*)0xA5500000;
    #elif defined(ST_7100)
    /* Check memory mapping in the clavmem.h */
        static unsigned char    *NCacheMemory = (unsigned char*)0xA5400000;
    #endif /* defined (ST_7200) */
#else
    static unsigned char    NCacheMemory [NCACHE_SIZE];
    #pragma ST_section      (NCacheMemory , "ncache_section")
    #if defined (mb391)
        /* DataBlock is declared here to avoid linker warning: all the data is allocated in ncache_section to
         * be ble to use more memory */
        static unsigned char    DataBlock[1];
    #else
        static unsigned char    DataBlock[DATA_MEMORY_SIZE];
    #endif
    #if defined(mb295) || defined (mb290) || defined (mb376) || defined (espresso) || defined (mb390) || defined (mb391) \
        || defined (mb400) || defined (mb424) || defined (mb421)||defined (mb457) ||defined (mb428) || defined(maly3s) || defined(SAT5107)
        #pragma ST_section      (DataBlock, "data_section")
    #else /* mb295 mb290 mb376 espresso mb390 mb400 mb424 mb421 mb457 maly3s SAT5107 */
        #if (defined(mb361) || defined(mb382)) && defined(UNIFIED_MEMORY)
        /* Use ncache2_section to get 8MB for DataBlock with unified memory */
            #pragma ST_section      (DataBlock, "ncache2_section")
        #else
            #pragma ST_section      (DataBlock, "system_section")
        #endif /* (mb361 || mb382) && UNIFIED_MEMORY */
    #endif /* not(mb295 mb290 mb376 espresso mb390 mb400 mb424 mb421 maly3s SAT5107) */
#endif /* not mb411 */

#if defined (mb295)
static unsigned char    Data2Block[DATA2_MEMORY_SIZE];
#pragma ST_section      (Data2Block, "data2_section")
static partition_t      TheData2Partition;
#endif /* mb295 */

static unsigned char    InternalBlockNoinit[1];
#pragma ST_section      ( InternalBlockNoinit, "internal_section_noinit")

static unsigned char    SystemBlockNoinit[1];
#pragma ST_section      ( SystemBlockNoinit, "system_section_noinit")

#endif /* not ST_OSLINUX */

#if (defined(mb361) || defined(mb382)) && defined(UNIFIED_MEMORY)
static unsigned char    NCache2Memory [1];
#pragma ST_section      ( NCache2Memory , "ncache2_section" )
static partition_t      TheNcache2Partition;
#endif

#ifdef ST_OS20
static partition_t      TheInternalPartition;
static partition_t      TheExternalPartition;
static partition_t      TheNCachePartition;
static partition_t      TheDataPartition;
#endif /* ST_OS20 */


ST_Partition_t          *InternalPartition_p;

#ifdef ST_OSLINUX
static BOOL             STAPIGAT_DeviceOpened = FALSE;
#endif

/* Global variables ------------------------------------------------------- */
#ifdef ST_OS20
ST_Partition_t          *InternalPartition_p;
#endif

ST_Partition_t          *SystemPartition_p;
ST_Partition_t          *DriverPartition_p;
ST_Partition_t          *NCachePartition_p;

/* following 2 variable are needed by OS20.lib (needed when using PTI and some others not identified */
ST_Partition_t          *ncache_partition;

ST_Partition_t          *internal_partition;

ST_Partition_t          *system_partition;

#if !defined(mb411) && !defined(mb519)
ST_Partition_t          *DataPartition_p;
#endif

#if defined(mb295)
ST_Partition_t          *Data2Partition_p;
#endif

/* This is the partition where audio streams will be loaded to */
ST_Partition_t          *AudioPartition_p;
#if !defined(ST_OS21) && !defined(ST_OSWINCE) && !defined(ST_OSLINUX)
unsigned char           *InternalBlock_p   = InternalBlock;
unsigned char           *ExternalBlock_p   = ExternalBlock;
unsigned char           *NCacheBlock_p     = NCacheMemory;
#endif
/* Extern functions prototypes -------------------------------------------- */


/* dependencies */
extern BOOL Test_CmdStart(void);    /* Test driver specific */

/* Private Macros --------------------------------------------------------- */

#ifdef USE_TBX
#define LOCAL_PRINT(x) STTBX_Print(x)
#else
#define LOCAL_PRINT(x) printf x
#endif


/* Private Function prototypes -------------------------------------------- */

#ifdef ST_OS20
static void DisplaySoftwareConfiguration(void);
#endif /* ST_OS20 */

static BOOL StartInitComponents(const char * TTDefaultScript);
static BOOL StartSettingDevices(void);
static BOOL StartSearchVersion(char* FullCutName);
static void StartInterconnectConfiguration(void);

#ifdef USE_TESTTOOL
static BOOL StartRegisterCmds(void);

#ifdef ST_OSLINUX
/* This feature is not in clavmem since under linux no access to AVMEM is allowed from User space */
static BOOL AVMEM_PrintMemoryStatus(STTST_Parse_t *Parse_p, char *ResultSymbol_p);
#endif
#endif  /* USE_TESTTOOL */

#ifdef THREAD_INFO
typedef struct
{
	THREADENTRY32 TInfo;
	DWORD KernelTime;
	DWORD UserTime;
}sThreadInfo;
#endif

/* Functions -------------------------------------------------------------- */

#ifdef ST_OS20
/*------------------------------------------------------------------------------
 Name   : DisplaySoftwareConfiguration
 In     : -
 Out    : -
 Note   :
------------------------------------------------------------------------------*/
static void DisplaySoftwareConfiguration(void)
{
    device_id_t devid;
    char devname[32];

    LOCAL_PRINT(("%s \n", __ICC_VERSION_STRING__));
    LOCAL_PRINT(("OS20 kernel %s \n\n", kernel_version()));
    devid = device_id();
    strcpy(devname, device_name(devid));
    LOCAL_PRINT(("Device_id = %x (%s)  \n\n", devid.id, devname));
}
#endif /* ST_OS20 */

#if defined(ST_OS21) || defined(ST_OSWINCE)
#if defined (ST_OSWINCE) && defined(BENCHMARK_WINCESTAPI)

static DWORD UseCPUTime(LPVOID param)
{
	FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
	int time1=0, time2=0, elapsed=0, sum =0, loop =0, tic_per_ms;
	BOOL* Stop = (BOOL*)param;
	while (!*Stop)
	{
		time1= time_now(); /* starting up injection */
		memcpy(pDest, pSrc, 1024*1024);
		time2= time_now(); /* starting up injection */
		elapsed += (time2 - time1);
		loop++;
	}
	if (elapsed)
	{
		/* ask for time spent in task */
		GetThreadTimes( MCThread, &lpCreationTime, &lpExitTime,
								&lpKernelTime,&lpUserTime);

		tic_per_ms = time_ticks_per_sec()/1000;
		elapsed = elapsed/tic_per_ms;
		printf ("T \t%d\t ms for %d loops Average BW \t%d\t MB/s \n", elapsed, loop, loop*1000/elapsed);
		printf ("Memcpy Task run during %d ms\n", lpUserTime.dwLowDateTime/10000);
	}
	return 0;
}

static void MemCpyTask(BOOL StartNoStop)
{
    static BOOL MemCpyStop;

	if (StartNoStop)
    {
			MemCpyStop = FALSE;

	  		/* Create mempcy task to measure CPU bandwith */
			MCThread = CreateThread(NULL, /* no security options */
					0,    /* default Stack Size */
					UseCPUTime, /* Entry point */
					(LPVOID)&MemCpyStop,  /* Entry point parameters */
					0,
					NULL  /* don't need Thread Id */
					);
			ASSERT(MCThread != NULL);
			SetThreadPriority(MCThread,THREAD_PRIORITY_IDLE);
    }
    else
    {
		/* stop memcpy thread */
        ASSERT(MCThread != NULL);
        MemCpyStop = TRUE;
        WaitForSingleObject(MCThread, 500);
        CloseHandle(MCThread);
        MCThread = NULL;
	}
}

static void ListThreads()
{
	HANDLE hSnapshot;
	THREADENTRY32 CurrentThread;
	task_t* CurrentTask; /* STAPI */
	U32 oldProcPermissions;
	char dashStr[120];
	char* nullName = "";
/*	strcpy( nullName, ""); */
	U32 LastPS=0;
	U32 ProcCnt=0;
	char* curName;
	FILETIME CreationTime;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;


	/* List STAPI/OS21 managed tasks */
	printf ("\n"); printf ("\n");
	printf ("STAPI Threads:\n");
	/* WINCE_DumpTasks(); */
	CurrentTask = task_list_next(0);
	printf("\n");
	printf("%-13s%-31s%6s%9s\n", "ID", "Name", "CePRI", "OS21PRI");
	memset(dashStr, '-', 59);
	dashStr[59]=0;
	printf("%s\n", dashStr);
	while(CurrentTask!=0)
	{
		printf(
			"0x%08X   %-28s   %6d   %6d\n",
			CurrentTask->dwThreadId_opaque,
			CurrentTask->tName,
			CurrentTask->dwCeTaskPriority,
			CurrentTask->dwOs21TaskPriority
		);
		CurrentTask = task_list_next(CurrentTask);
	}

	/* List WinCE Threads */
	printf ("\n"); printf ("\n");
	printf ("All WINCE Threads\n");
	hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0); /* Use current process, include all info in snapshot */
	oldProcPermissions=GetCurrentPermissions();  /* Save procedure's permissions */
	SetProcPermissions(0xFFFFFFFF);  /* Enable access to all processes (for timing info) */
	CurrentThread.dwSize=sizeof(CurrentThread);
	if(!Thread32First(hSnapshot, &CurrentThread)) {
		printf("Error retrieving thread information!\n");
		return;
	}

	do {

		/* Get process EXE name -- TODO */
		if (LastPS != CurrentThread.th32OwnerProcessID)
		{
			LastPS = CurrentThread.th32OwnerProcessID;
			ProcCnt++;
			printf("\n");
			printf("*** Process %d ***\n", ProcCnt);
			printf("%-13s%-31s%-10s%6s%12s%12s\n", "ID", "Name", "PS", "Pri", "KernTime", "UserTime");
			memset(dashStr, '-', 84);
			dashStr[84]=0;
			printf("%s\n", dashStr);
		}


		/* Get name if it is a STAPI/OS21 thread */
		curName=0;
		for (CurrentTask = task_list_next(0); CurrentTask!=0; CurrentTask=task_list_next(CurrentTask)) {
			if (CurrentTask->dwThreadId_opaque==CurrentThread.th32ThreadID) {
				curName=CurrentTask->tName;
			}
		}
		if (!curName) curName=nullName;

		/* Get timing info -- 64 bit values that represent 100-nanosecond intervals (1/10000 millisecond) */
		memset(&CreationTime, 0, sizeof(FILETIME));
		ExitTime=KernelTime=UserTime=CreationTime;
		GetThreadTimes( (HANDLE)CurrentThread.th32ThreadID, &CreationTime, &ExitTime, &KernelTime, &UserTime);

		printf(
			"0x%08x   %-28s   0x%08x   %3d   %7d   %7d\n",
			CurrentThread.th32ThreadID,
			curName,
			CurrentThread.th32OwnerProcessID,
			CurrentThread.tpBasePri + CurrentThread.tpDeltaPri,
			(int)(KernelTime.dwLowDateTime/(double)10000.0 + KernelTime.dwHighDateTime*(double)(1<<16)*((double)(1<<16)/10000.0)),
			(int)(UserTime.dwLowDateTime/  (double)10000.0 + UserTime.dwHighDateTime*(double)(1<<16)*((double)(1<<16)/10000.0))
		);
	} while(Thread32Next(hSnapshot, &CurrentThread));
	SetProcPermissions(oldProcPermissions);
}

static void TimeThread()
{

	HANDLE hSnapshot;
	THREADENTRY32 CurrentThread;
	sThreadInfo Thread[100];
	task_t* CurrentTask; /* STAPI */
	U32 oldProcPermissions;
	char dashStr[120];
	char* nullName = "";
/*	strcpy( nullName, ""); */
	U32 LastPS=0;
	U32 ProcCnt=0;
	char* curName;
	FILETIME CreationTime;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;
	int i=0, j=0;

    /* List WinCE Threads */
	printf ("\n"); printf ("\n");
	printf ("Start Time WINCE Threads\n");
	hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0); /* Use current process, include all info in snapshot */
	oldProcPermissions=GetCurrentPermissions();  /* Save procedure's permissions */
	SetProcPermissions(0xFFFFFFFF);  /* Enable access to all processes (for timing info) */
	CurrentThread.dwSize=sizeof(CurrentThread);
	if(!Thread32First(hSnapshot, &CurrentThread)) {
		printf("Error retrieving thread information!\n");
		return;
	}

	do {

		/* Get timing info -- 64 bit values that represent 100-nanosecond intervals (1/10000 millisecond) */
		memset(&CreationTime, 0, sizeof(FILETIME));
		ExitTime=KernelTime=UserTime=CreationTime;
		GetThreadTimes( (HANDLE)CurrentThread.th32ThreadID, &CreationTime, &ExitTime, &KernelTime, &UserTime);

		/* save start info */
		Thread[i].TInfo = CurrentThread;
		Thread[i].KernelTime = KernelTime.dwLowDateTime;
		Thread[i].UserTime = UserTime.dwLowDateTime;
		i++;
	} while(Thread32Next(hSnapshot, &CurrentThread));

	Sleep (20*1000);
	printf ("Get End Time \n");

	j=i;
	/* Print */
	for (i=0; i< j; i++)
	{
		/* Get timing info -- 64 bit values that represent 100-nanosecond intervals (1/10000 millisecond) */
		memset(&CreationTime, 0, sizeof(FILETIME));
		ExitTime=KernelTime=UserTime=CreationTime;
		GetThreadTimes( (HANDLE)Thread[i].TInfo.th32ThreadID, &CreationTime, &ExitTime, &KernelTime, &UserTime);

		Thread[i].KernelTime = KernelTime.dwLowDateTime - Thread[i].KernelTime;
		Thread[i].UserTime = UserTime.dwLowDateTime - Thread[i].UserTime;
	}
	/* Print */
	for (i=0; i< j; i++)
	{
		/* Get process EXE name -- TODO */
		if (LastPS != Thread[i].TInfo.th32OwnerProcessID)
		{
			LastPS = Thread[i].TInfo.th32OwnerProcessID;
			ProcCnt++;
			printf("\n");
			printf("*** Process %d ***\n", ProcCnt);
			printf("%-13s%-31s%-10s%6s%12s%12s\n", "ID", "Name", "PS", "Pri", "KernTime", "UserTime");
			memset(dashStr, '-', 84);
			dashStr[84]=0;
			printf("%s\n", dashStr);
		}


		/* Get name if it is a STAPI/OS21 thread */
		curName=0;
		for (CurrentTask = task_list_next(0); CurrentTask!=0; CurrentTask=task_list_next(CurrentTask)) {
			if (CurrentTask->dwThreadId_opaque==Thread[i].TInfo.th32ThreadID) {
				curName=CurrentTask->tName;
			}
		}
		if (!curName) curName=nullName;


		printf(
			"0x%08x\t%-28s\t0x%08x\t%3d\t%7d\t%7d\n",
			Thread[i].TInfo.th32ThreadID,
			curName,
			Thread[i].TInfo.th32OwnerProcessID,
			Thread[i].TInfo.tpBasePri + Thread[i].TInfo.tpDeltaPri,
			(int)(Thread[i].KernelTime/(double)10000.0),
			(int)(Thread[i].UserTime/(double)10000.0)
		);

	}
	SetProcPermissions(oldProcPermissions);

}

#endif /* ST_OSWINCE */

void TestPerf(void)
{
	/* Test 1 - CPU power */
	int time1, time2, endtime, elapsed, i, tic_per_ms;
	osclock_t IdleTimeStart, IdleTimeEnd;
	task_t* TaskTab[30];
	int TaskTime[30];
	task_status_t status;

#ifdef BENCHMARK_WINCESTAPI
	void*	Handle;
	static	CRITICAL_SECTION gLocCriticalHandler;
#endif
	pDest= (unsigned char*)malloc(1024*1024);
	pSrc= (unsigned char*)malloc(1024*1024);

/* test memcpy for 10s */
#ifdef ST_OSWINCE
#ifdef BENCHMARK_WINCESTAPI /* only ST_OSWINCE */

	/*ListThreads();
	Sleep(20*1000);
	ListThreads();*/
	TimeThread();

	printf (" PROFILE ONLY 100 micro\n");
	ProfileStart( 100, PROFILE_BUFFER | PROFILE_STARTPAUSED );
	ProfileStart( 100, PROFILE_CONTINUE );
	Sleep(20*1000);
	ProfileStop();/* stop wince profiling */

	printf (" PROFILE ONLY 200 micro\n");
	ProfileStart( 200, PROFILE_BUFFER | PROFILE_STARTPAUSED );
	ProfileStart( 200, PROFILE_CONTINUE );
	Sleep(20*1000);
	ProfileStop();/* stop wince profiling */

	printf (" PROFILE + CUSTOM  100 micro\n");
	ProfileStart( 100, PROFILE_BUFFER | PROFILE_STARTPAUSED );
	ProfileStart( 100, PROFILE_CONTINUE );
	P_Start(); /* start wince profiling */
	Sleep(20*1000);
	P_Stop();
	ProfileStop();/* stop wince profiling */
	P_Print();      /* print wince profiling */

/*	printf (" PROFILE + Memcpy 100 micro\n");
	ProfileStart( 100, PROFILE_BUFFER | PROFILE_STARTPAUSED );
	ProfileStart( 100, PROFILE_CONTINUE );
	MemCpyTask(TRUE);
	Sleep(20*1000);



	MemCpyTask(FALSE);
	ProfileStop();// stop wince profiling*/

#endif /* BENCHMARK_WINCESTAPI */

#else /* OS21 */
	IdleTimeStart = IdleTimeEnd =0;
	IdleTimeStart = kernel_idle();

	/* record start time of each thread */
	i=0;
	TaskTab[i]= task_list_next(NULL);

	while (TaskTab[i]!=NULL)
	{
		task_status(TaskTab[i], &status, 0);
		TaskTime[i] = status.task_time;

	/*	printf ("Task %s time %d \n", task_name(TaskTab[i]), TaskTime[i]); */

		i++;
		TaskTab[i]= task_list_next(TaskTab[i-1]);
	}

	task_delay(10*time_ticks_per_sec());

	i= 0;
	while (TaskTab[i]!=NULL)
	{
		task_status(TaskTab[i], &status, 0);
		elapsed = status.task_time - TaskTime[i];

		printf ("%s\t run \t%d\t", task_name(TaskTab[i]), elapsed);
		printf ("%d \n", elapsed/(time_ticks_per_sec()/1000));

		i++;
	}

	IdleTimeEnd = kernel_idle();
	printf ("OS21 Idle StarTime %d \n", IdleTimeStart);
	printf ("OS21 Idle EndTime %d \n", IdleTimeEnd);
	printf ("Idle Time %d ms\n", time_minus(IdleTimeEnd, IdleTimeStart)/(time_ticks_per_sec()/1000));
	printf ("OS21 Kernel Time %d \n", kernel_time());
#endif /* WINCE */

	i= elapsed = 0;
/*	printf (" PROFILE + Memcpy 100 micro\n");
	ProfileStart( 100, PROFILE_BUFFER | PROFILE_STARTPAUSED );
	ProfileStart( 100, PROFILE_CONTINUE );*/
	time2=time_now();
	endtime = time_plus(time_now(), 20*time_ticks_per_sec());
	while (time_after (endtime, time2))
	{
		time1= time_now();
		memcpy(pDest, pSrc, 1024*1024);
		time2= time_now();
		elapsed += (time2 - time1);
		i++;
	}
/*	ProfileStop();// stop wince profiling */
	if (elapsed)
	{
		/* in ms */
		tic_per_ms = time_ticks_per_sec()/1000;
		elapsed = elapsed/tic_per_ms;
		printf ("T \t%d\t ms for %d loops Average BW \t%d\t MB/s \n", elapsed, i, i*1000/elapsed);
	}

	free (pSrc);
	free (pDest);
}

#endif /* ST_OS21 || ST_OSWINCE */

/*-------------------------------------------------------------------------
 * Function : StartInitComponents
 *            Init, open, set drivers,
 *            according to driver dependencies
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartInitComponents(const char * TTDefaultScript)
{
    BOOL RetOk = TRUE;
    printf ( "----------------------------------------------------------------------\n");
#if !defined ST_OSLINUX
	/* Wince specific profiler init */
#ifdef BENCHMARK_WINCESTAPI
	P_Init();  /* create an empty list of threads */
#endif
#if !defined ST_OSLINUX
    RetOk = BOOT_Init();
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
	/* TestPerf(); // uncomment for test */
#endif

#endif /* !ST_OSLINUX */

#ifdef USE_STCFG
    if (RetOk)
    {
                RetOk = CFG_Init();
    }
#endif

#ifdef USE_PIO
    if (RetOk)
    {
        RetOk = PIO_Init();
    }
#endif

#ifdef USE_UART
    if (RetOk)
    {
        RetOk = UART_Init();
    }
#if defined(TRACE_UART) && !defined(USE_VALID_TRACE)
    if (RetOk)
    {
        RetOk = UART_Open();
    }
#endif
#endif /* #ifdef USE_UART */

#ifdef USE_TBX
    if (RetOk)
    {
        RetOk = TBX_Init();
    }
#endif

#ifdef USE_TESTTOOL
    if (RetOk)
    {
        RetOk = TST_Init(TTDefaultScript);
    }
#endif

#if defined USE_EVT || defined USE_POLL
    if (RetOk)
    {
        RetOk = EVT_Init();
    }
#endif

#ifdef USE_INTMR
    if( RetOk)
    {
        RetOk = INTMR_Init();
    }
#endif

#ifdef USE_FDMA
#if !defined ST_OSLINUX
/* Init is done when loading the module */
    if( RetOk)
    {
        RetOk = FDMA_Init();
    }
#endif
#endif

#ifdef USE_GPDMA
    if( RetOk)
    {
        RetOk = GPDMA_UseInit();
    }
#endif

#ifdef USE_STLLI
    if (RetOk)
    {
        RetOk = LLI_Init();
    }
#endif

#ifdef USE_TSMUX
    if (RetOk)
    {
        RetOk = TSMUX_Init();
    }
#endif

#ifdef USE_AVMEM
    if (RetOk)
    {
        RetOk = AVMEM_Init();
    }
#endif

#ifdef USE_AUDIO
#if (!defined(ST_7710)&&!defined(ST_OSLINUX))
if (RetOk)
    {
       RetOk = MBX_Init();
    }
#endif
#endif

#ifdef USE_PWM
    if (RetOk)
    {
        RetOk = PWM_Init();
    }
#endif

#ifdef USE_I2C
    if (RetOk)
    {
        RetOk = I2C_Init();
    }
#endif

#ifdef USE_PTI
    if (RetOk)
    {
        RetOk = PTI_InitComponent();
    }
#endif

#ifdef USE_DEMUX
    if (RetOk)
    {
        RetOk = DEMUX_InitComponent();
    }
#endif

#ifdef USE_CLKRV
#if defined(ST_7200)
    if (RetOk)
    {
        RetOk = CLKRV_Init(STCLKRV0_DEVICE_NAME, (void *)CLKRV0_BASE_ADDRESS, STCLKRV_DECODE_PRIMARY);
    }
#else /* if !defined(ST_7200) ... */
    if (RetOk)
    {
        RetOk = CLKRV_Init();
    }
#endif /* if !defined(ST_7200) ... else ...*/
#endif /* #ifdef USE_CLKRV ... */

#ifdef USE_AUDIO
    if (RetOk)
    {
#ifndef ST_7710
        RetOk = AUD_InitComponent();
#endif
    }
#endif

#if defined(DVD_SECURED_CHIP)
    if (RetOk)
    {
        RetOk = MES_Init();
    }
#endif

    LOCAL_PRINT(("STSYS\t\t\trevision=%s\n", STSYS_GetRevision() ));
    LOCAL_PRINT(("STAPIGAT\t\trevision=STAPIGAT-REL_1.0.69\n"));

    return( RetOk );
} /* end of StartInitComponents() */

/*-------------------------------------------------------------------------
 * Function : StartSettingDevices
 *            Set some devices
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartSettingDevices(void)
{
    BOOL RetOk;

    RetOk = TRUE;
    LOCAL_PRINT(( "----------------------------------------------------------------------\n" ));
    LOCAL_PRINT(( "Device setting...\n" ));

#ifdef USE_STEM_RESET
    if (RetOk)
    {
        RetOk = StemReset_Init();
    }
#endif

#ifdef USE_SCART_SWITCH_CONFIG
    if (RetOk)
    {
        RetOk = ScartSwitch_Init();
        if ( !RetOk )
        {
            LOCAL_PRINT(("SCART not available !\n"));
        }
        /* Special for SCART: Even if fails, continue normaly */
        RetOk = TRUE;
    }
#endif

#ifdef USE_HDD
    if (RetOk)
    {
        RetOk = HDD_Init();
        if ( !RetOk )
        {
            LOCAL_PRINT(("HDD not available !\n"));
        }
        else
        {
            RetOk = HDD_CmdStart();
            if ( !RetOk )
            {
                LOCAL_PRINT(("HDD_CmdStart() failed !\n"));
            }
        }
        /* Special for HDD: Even if fails, continue normaly */
        RetOk = TRUE;
    }
#endif /* #ifdef USE_HDD */

    /* The following are only here temporary !!! */
    /*         End of temporary !!!              */

    return( RetOk );
} /* end of StartSettingDevices() */

#ifdef USE_TESTTOOL
/*******************************************************************************
Name        : AVMEM_PrintMemoryStatus
Description : Call  STAPIGAT_PrintMemoryStatus()
Parameters  :
Assumptions :
Limitations :
Returns     : False if success
*******************************************************************************/
#ifdef ST_OSLINUX
static BOOL AVMEM_PrintMemoryStatus(STTST_Parse_t *Parse_p, char *ResultSymbol_p)
{
    ST_ErrorCode_t  ErrorCode;
    ST_DeviceName_t DeviceName;
    int LVar;

    UNUSED_PARAMETER(ResultSymbol_p);
    /* get file name */
    memset(DeviceName, 0, sizeof(DeviceName));
    ErrorCode = STTST_GetString( Parse_p, "", DeviceName, sizeof(DeviceName) );
    if ( ErrorCode )
    {
        STTST_TagCurrentLine( Parse_p, "expected DeviceName" );
        return(FALSE);
    }

    if (strcmp(DeviceName, "") == 0)
    {
        strcpy(DeviceName, "AVMEM");
        printf("%s(): empty name, default is '%s'\n", __FUNCTION__, DeviceName);
    }

    ErrorCode = STTST_GetInteger( Parse_p, 0 , &LVar);
    if ( ErrorCode )
    {
        STTST_TagCurrentLine( Parse_p, "expected Parition number " );
        return(FALSE);
    }

    ErrorCode = STAPIGAT_PrintMemoryStatus(DeviceName, LVar);

    return (ErrorCode == ST_NO_ERROR ? FALSE : TRUE);
}
#endif  /* LINUX */

/*-------------------------------------------------------------------------
 * Function : StartRegisterCmds
 *            Create register commands needed for the test
 * Input    :
 * Output   :
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartRegisterCmds(void)
{
    BOOL RetOk = TRUE;
#if defined(TRACE_UART) && !defined(USE_VALID_TRACE)
    ST_ErrorCode_t ErrorCode;
#endif

    LOCAL_PRINT(( "----------------------------------------------------------------------\n" ));
    LOCAL_PRINT(( "Testtool commands registrations...\n" ));

/* ---------------- Tools     ---------- */

    LOCAL_PRINT(( "... tools:\n" ));

#ifdef ST_OSLINUX
#ifdef USE_REG
    if (RetOk)
    {
        RetOk = REG_RegisterCmd();
    }
#endif
#endif /* ST_OSLINUX */

#ifdef DUMP_REGISTERS
    if (RetOk)
    {
        RetOk = DUMPREG_RegisterCmd();
    }
    if( (DUMPREG_Init()) != ST_NO_ERROR)
    {
        RetOk = FALSE;
    }
#endif /* DUMP_REGISTERS */

#ifdef USE_TSTMEM
    if (RetOk)
    {
        RetOk = TSTMEM_Init();
    }
#endif

#ifdef USE_VALID_TRACE
    if (RetOk)
    {
        RetOk = TRC_Init();
    }
#endif

#ifdef USE_VALID_DHRYSTONE
    if (RetOk)
    {
        RetOk = DHRY_Init();
    }
#endif

#ifdef USE_TSTREG
    if (RetOk)
    {
        RetOk = TSTREG_Init();
    }
#endif

#if defined(TRACE_UART) && !defined(USE_VALID_TRACE)
    if (RetOk)
    {
        /* Uart Traces implementation task inits and debug */
        ErrorCode = TraceInit();
        if (ErrorCode != ST_NO_ERROR)
        {
            LOCAL_PRINT(("TraceInit() failed !"));
        }
        /* Testtool's commands for traces */
        ErrorCode = TraceDebug();
        if (ErrorCode != ST_NO_ERROR)
        {
            LOCAL_PRINT(("TraceDebug() failed !"));
        }
    }
#endif

#ifdef USE_API_TOOL
    if (RetOk)
    {
        RetOk = API_RegisterCmd();
    }
#endif

#ifdef USE_DISP_GAM
    if (RetOk)
    {
        RetOk = DispGam_RegisterCmd();
    }
#endif

#ifdef USE_DISP_GDMA
    if (RetOk)
    {
        RetOk = DispGDMA_RegisterCmd();
    }
#endif

#ifdef USE_DISP_OSD
    if (RetOk)
    {
        RetOk = DispOSD_RegisterCmd();
    }
#endif

#ifdef USE_DISP_VIDEO
    if (RetOk)
    {
        RetOk = DispVideo_RegisterCmd();
    }
#endif

#ifdef USE_PROCESS_TOOL
    if (RetOk)
    {
        RetOk = TTTask_RegisterCmd();
    }
#endif

#ifdef USE_VOUTSIM
    if (RetOk)
    {
        RetOk = VoutSim_RegisterCmd();
    }
#endif

#ifdef USE_VTGSIM
    if (RetOk)
    {
        RetOk = VtgSim_RegisterCmd();
    }
#endif

#ifdef USE_VTGSTUB
    if (RetOk)
    {
        RetOk = VtgStub_RegisterCmd();
    }
#endif

#ifdef ST_OSLINUX
    if (RetOk)
    {
        /* This feature is registered in startup.c instead of clavmem.c */
        /* since under linux no access to AVMEM is allowed from User space */
        /* Under other OS, this feature is available with clavmem.c file ... */
        RetOk = !(STTST_RegisterCommand( "AVMEM_GetMemoryStatus",  AVMEM_PrintMemoryStatus, "<Device name>"));
    }
#endif  /* LINUX */


/* ---------------- Libraries ---------- */

    LOCAL_PRINT(( "... libraries:\n" ));

#ifdef USE_PTI
    if (RetOk)
    {
        RetOk = PTI_RegisterCmd();
    }
#endif

#ifdef USE_DEMUX
    if (RetOk)
    {
        RetOk = DEMUX_RegisterCmd();
    }
#endif

#ifdef USE_DENC
    if (RetOk)
    {
        RetOk = DENC_RegisterCmd();
    }
#endif

#ifdef USE_VTG
    if (RetOk)
    {
        RetOk = VTG_RegisterCmd();
    }
#endif

#ifdef USE_VOUT
    if (RetOk)
    {
        RetOk = VOUT_RegisterCmd();
    }
#endif

#ifdef USE_LAYER
    if (RetOk)
    {
        RetOk = LAYER_RegisterCmd();
    }
#endif

#ifdef USE_VMIX
    if (RetOk)
    {
        RetOk = VMIX_RegisterCmd();
    }
#endif


#ifdef USE_VIDEO
    if (RetOk)
    {
        RetOk = VID_RegisterCmd();
    }
#endif

#ifdef USE_AUDIO
    if (RetOk)
    {
        RetOk = AUD_RegisterCmd();
    }
#endif


#ifdef USE_VIN
    if (RetOk)
    {
        RetOk = VIN_RegisterCmd();
    }
#endif

#ifdef USE_EXTVIN
    if (RetOk)
    {
        RetOk = EXTVIN_RegisterCmd();
    }
#endif

#ifdef USE_VBI
    if (RetOk)
    {
        RetOk = VBI_RegisterCmd();
    }
#endif

#ifdef USE_TTX
    if (RetOk)
    {
        RetOk = TTX_RegisterCmd();
    }
#endif

#ifdef USE_CC
    if (RetOk)
    {
        RetOk = CC_RegisterCmd();
    }
#endif

#ifdef USE_TUNER
    if (RetOk)
    {
        RetOk = TUNER_RegisterCmd();
    }
#endif

#ifdef USE_BLIT
    if (RetOk)
    {
        RetOk = BLIT_RegisterCmd();
    }
#endif

#ifdef USE_COMPO
    if (RetOk)
    {
        RetOk = COMPO_RegisterCmd();
    }
#endif

#ifdef USE_DISP
    if (RetOk)
    {
        RetOk = DISP_RegisterCmd();
    }
#endif

#ifdef USE_HDMI
    if (RetOk)
    {
        RetOk = HDMI_RegisterCmd();
    }
#endif

#ifdef USE_LXLOADER
    if (RetOk)
    {
        RetOk = LxLoaderInitCmds();
    }
#endif

#ifdef USE_GFB
    if (RetOk)
    {
       RetOk = GFB_RegisterCmd();
    }
#endif

#if defined(DVD_SECURED_CHIP)
    if (RetOk)
    {
       RetOk = MES_RegisterCmd();
    }
#endif

/* ---------------- Test application --- */

    LOCAL_PRINT(( "... test application:\n" ));

    if (RetOk)
    {
        RetOk = Test_CmdStart();
    }

    LOCAL_PRINT(( "----------------------------------------------------------------------\n" ));

    return(RetOk);
} /* end StartRegisterCmds() */
#endif /* #ifdef USE_TESTTOOL */
#if 0
#ifdef ST_OSLINUX
/*-------------------------------------------------------------------------
 * Function : STAPIGAT_KillApp
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void STAPIGAT_QuitApp(void)
{
    printf("\nQuit signal received...\n");

    printf("Trying to Term all drivers...\n");
    STAPIGAT_Term();

    printf("Ok !\n");

    exit(0);
}

/*-------------------------------------------------------------------------
 * Function : STAPIGAT_CoreError
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void STAPIGAT_CoreError(void)
{
    printf("\nUndefined reference signal received...\n");

    printf("Trying to Term all drivers...\n");
    STAPIGAT_Term();

    printf("Ok !\n");

    exit(0);
}

#endif
#endif

/*-------------------------------------------------------------------------
 * Function : STAPIGAT_Init
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
#ifdef STAPIGAT_USE_NEW_INIT_API
void STAPIGAT_Init(const char * TTDefaultScript)
#else
void STAPIGAT_Init(void)
#endif
{
    BOOL RetOk = TRUE;
    BOOL IsDCacheOn = FALSE;
    char ChipVersion[16];

    /* Set special interconnect configuration */
    StartInterconnectConfiguration();

#if 0
    #ifdef ST_OSLINUX
    /* Branch the application on signals */
    if (signal(SIGINT, STAPIGAT_QuitApp) == SIG_ERR)
    {
        printf("SIGINT not handled !\n");
    }
    if (signal(SIGQUIT, STAPIGAT_QuitApp) == SIG_ERR)
    {
        printf("SIGQUIT not handled !\n");
    }
    if (signal(SIGSEGV, STAPIGAT_CoreError) == SIG_ERR)
    {
        printf("SIGSEGV not handled !\n");
    }
    #endif
#endif
    /* Create memory partitions */
#ifdef ST_OS21
/* This is to avoid a compiler warning */
    InternalBlockNoinit[0] = 0;
    SystemBlockNoinit[0] = 0;
#endif
#ifdef ST_OS20
/* This is to avoid a compiler warning */
    InternalBlockNoinit[0] = 0;
    SystemBlockNoinit[0] = 0;
#if (defined(mb361) || defined(mb382)) && defined(UNIFIED_MEMORY)
    NCache2Memory[0] = 0;
#endif
#if defined(mb391)
    DataBlock[0]=0;
#endif

    InternalPartition_p = &TheInternalPartition;
    partition_init_simple(InternalPartition_p, (void *) InternalBlock, sizeof(InternalBlock));

    SystemPartition_p = &TheExternalPartition;
    partition_init_heap(SystemPartition_p, (void *) ExternalBlock, sizeof(ExternalBlock));

    NCachePartition_p = &TheNCachePartition;
    partition_init_heap(NCachePartition_p, (void *)NCacheMemory, sizeof(NCacheMemory));

    DataPartition_p = &TheDataPartition;

#if !defined(mb391)
    partition_init_heap(DataPartition_p, (void *) DataBlock, sizeof(DataBlock));
#else
    partition_init_heap(DataPartition_p, (void *) NCacheMemory, sizeof(NCacheMemory));
#endif

#if defined(mb295)
    Data2Partition_p = &TheData2Partition;
    partition_init_heap(Data2Partition_p, (void *) Data2Block, sizeof(Data2Block));
#endif
    internal_partition  = InternalPartition_p;
#endif /* ST_OS20 */





/* ---------------------------------------------------- */
#ifdef ST_OSWINCE
/*	DataBlock =(unsigned char*)malloc(DATA_MEMORY_SIZE); */
	ExternalBlock = (unsigned char*)malloc(SYSTEM_SIZE);
/*	NCacheMemory = (unsigned char*)malloc(NCACHE_SIZE); */
    WCE_ASSERT(ExternalBlock != NULL && NCacheMemory != NULL);
	SystemPartition_p = partition_create_heap((void *) ExternalBlock, SYSTEM_SIZE);
    NCachePartition_p = partition_create_heap((void *) MapPhysicalToVirtual(NCacheMemory, NCACHE_SIZE), NCACHE_SIZE);
   /* DataPartition_p = partition_create_heap((void *) DataBlock, DATA_MEMORY_SIZE); */

#elif defined(ST_OS21)

#ifdef NATIVE_CORE
    /* custom partition init and kernel init: */
    #define SYSTEM_STACK_SIZE     530000
    #define SYSTEM_HEAP_SIZE      530000
    static U8 system_heap[ SYSTEM_HEAP_SIZE ];
    static U8 system_stack[ SYSTEM_STACK_SIZE ];
    int i;
    kernel_initialize_t os21_init = { system_stack ,
                                  sizeof(system_stack),
                                  system_heap,
                                  sizeof(system_heap) };
    static U8  ExternalBlock[SYSTEM_SIZE];
    static U8  * NCacheMemory;


#ifdef ST_7200
#ifdef NATIVE_CORE
    NCacheMemory = (U8 *) (tlm_to_virtual((esw_uint64_t)(char*)(0x17000000)));
#else
	NCacheMemory = (U8 *) (tlm_to_virtual((char*)(0x17000000)));
#endif
#else /* ST_7200 */
    NCacheMemory = (U8 *) tlm_to_virtual((char*)(0xA5300000));
#endif /* ST_7200 */

    i = kernel_initialize(&os21_init);
    if (i != OS21_SUCCESS)
    {
        /*printf("kernel_initialize() failed\n");*/
        exit(10);
    }
    i = kernel_start();
    if (i != OS21_SUCCESS)
    {
        /*printf("kernel_start() failed\n");*/
        exit(10);
    }
#endif /* NATIVE_CORE */

    SystemPartition_p = partition_create_heap((void*) ExternalBlock, SYSTEM_SIZE);
    NCachePartition_p = partition_create_heap((void*)NCacheMemory, NCACHE_SIZE);

#if defined (mb411)
    /*  no DataPartition_p, we will allocate buffer to load streams in the STAVMEM_SYS_MEMORY in vid_util.c instead of memory allocate in DataPartition_p */
#elif defined (mb519)
    /*  no DataPartition_p, we will allocate buffer to load streams in the STAVMEM_SYS_MEMORY in vid_util.c instead of memory allocate in DataPartition_p */
#else /* not mb411 */
    DataPartition_p = partition_create_heap((void *) DataBlock, DATA_MEMORY_SIZE);
#endif /* mb411 */
#endif  /* ST_OS21 */
/* ---------------------------------------------------- */

#ifdef ST_OSLINUX
	/* Following pointer must not be NULL but are not used with LINUX */
	SystemPartition_p = InternalPartition_p = NCachePartition_p = (int *) 0xFFFFFFFF;

    internal_partition  = InternalPartition_p;
#if defined (mb231)
    AudioPartition_p    = NCachePartition_p;
#elif (defined(mb361) || defined(mb382)) && defined(UNIFIED_MEMORY)
    AudioPartition_p    = NCachePartition_p;
#else
    AudioPartition_p    = SystemPartition_p;
#endif

    printf("Partitions initialized\n");
#ifndef USE_GFB
    STAPIGAT_DeviceOpened = STAPIGAT_OpenFileDev();
    if (STAPIGAT_DeviceOpened)
    {
        ClockMap.BaseAddress = CLOCK_MAP_BASE_ADDRESS ;
        ClockMap.AddressWidth = CLOCK_MAP_WIDTH ;
        STAPIGAT_MapUser( &ClockMap );
        if ( ClockMap.MappedBaseAddress )
        {
            printf( "Old clock base adress= 0x%lx " ,  ClockMap.BaseAddress );
            printf( "New clock base adress= 0x%lx (linux)\n" ,  (unsigned long) ClockMap.MappedBaseAddress );
        }

        SysCfgMap.BaseAddress = SYSCFG_MAP_BASE_ADDRESS ;
        SysCfgMap.AddressWidth = SYSCFG_MAP_WIDTH ;
        STAPIGAT_MapUser( &SysCfgMap );
        if ( SysCfgMap.MappedBaseAddress )
        {
            printf( "Old SysCfg base adress= 0x%lx " ,  SysCfgMap.BaseAddress );
            printf( "New SysCfg base adress= 0x%lx (linux)\n" ,  (unsigned long) SysCfgMap.MappedBaseAddress );
        }

        OutputStageMap.BaseAddress = OUTPUTSTAGE_MAP_BASE_ADDRESS;
        OutputStageMap.AddressWidth = OUTPUTSTAGE_MAP_WIDTH;
        STAPIGAT_MapUser( &OutputStageMap );
        if ( OutputStageMap.MappedBaseAddress )
        {
            printf( "Old output stage base adress= 0x%lx " ,  OutputStageMap.BaseAddress );
            printf( "New output stage base adress= 0x%lx (linux)\n" ,  (unsigned long) OutputStageMap.MappedBaseAddress );
        }

        DencMap.BaseAddress = DENC_MAP_BASE_ADDRESS;
        DencMap.AddressWidth = DENC_MAP_WIDTH;
        STAPIGAT_MapUser( &DencMap );
        if ( DencMap.MappedBaseAddress )
        {
            printf( "Old denc base adress= 0x%lx " ,  DencMap.BaseAddress );
            printf( "New denc base adress= 0x%lx (linux)\n" ,  (unsigned long) DencMap.MappedBaseAddress );
        }

        CompoMap.BaseAddress = COMPO_MAP_BASE_ADDRESS;
        CompoMap.AddressWidth = COMPO_MAP_WIDTH;
        STAPIGAT_MapUser( &CompoMap );
        if ( CompoMap.MappedBaseAddress )
        {
            printf( "Old compo base adress= 0x%lx " ,  CompoMap.BaseAddress );
            printf( "New compo base adress= 0x%lx (linux)\n" ,  (unsigned long) CompoMap.MappedBaseAddress );
        }
        #if defined (ST_7200)
            Compo1Map.BaseAddress = COMPO1_MAP_BASE_ADDRESS;
            Compo1Map.AddressWidth = COMPO1_MAP_WIDTH;
            STAPIGAT_MapUser( &Compo1Map );
            if ( Compo1Map.MappedBaseAddress )
            {
                printf( "Old compo base adress= 0x%lx " ,  Compo1Map.BaseAddress );
                printf( "New compo base adress= 0x%lx (linux)\n" ,  (unsigned long) Compo1Map.MappedBaseAddress );
            }

            OutputStageSDMap.BaseAddress = OUTPUTSTAGE_SD_MAP_BASE_ADDRESS;
            OutputStageSDMap.AddressWidth = OUTPUTSTAGE_SD_MAP_WIDTH;
            STAPIGAT_MapUser( &OutputStageSDMap );
            if ( OutputStageSDMap.MappedBaseAddress )
            {
                printf( "Old output stage base adress= 0x%lx " ,  OutputStageSDMap.BaseAddress );
                printf( "New output stage base adress= 0x%lx (linux)\n" ,  (unsigned long) OutputStageSDMap.MappedBaseAddress );
            }

            HDMIMap.BaseAddress = HDMI_MAP_BASE_ADDRESS;
            HDMIMap.AddressWidth = HDMI_MAP_WIDTH;
            STAPIGAT_MapUser( &HDMIMap );
            if ( HDMIMap.MappedBaseAddress )
            {
                printf( "Old HDMI base adress= 0x%lx " ,  HDMIMap.BaseAddress );
                printf( "New HDMI base adress= 0x%lx (linux)\n" ,  (unsigned long) HDMIMap.MappedBaseAddress );
            }


        #endif
        Disp0Map.BaseAddress = DISP0_MAP_BASE_ADDRESS;
        Disp0Map.AddressWidth = DISP0_MAP_WIDTH;
        STAPIGAT_MapUser( &Disp0Map );
        if ( Disp0Map.MappedBaseAddress )
        {
            printf( "Disp0 base adress= 0x%lx " ,  Disp0Map.BaseAddress );
            printf( "New Disp0 base adress= 0x%lx (linux)\n" ,  (unsigned long) Disp0Map.MappedBaseAddress );
        }

        Disp1Map.BaseAddress = DISP1_MAP_BASE_ADDRESS;
        Disp1Map.AddressWidth = DISP1_MAP_WIDTH;
        STAPIGAT_MapUser( &Disp1Map );
        if ( Disp1Map.MappedBaseAddress )
        {
            printf( "Disp1 base adress= 0x%lx " ,  Disp1Map.BaseAddress );
            printf( "New Disp1 base adress= 0x%lx (linux)\n" ,  (unsigned long) Disp1Map.MappedBaseAddress );
        }
        #if defined (ST_7200)
        Disp2Map.BaseAddress = DISP2_MAP_BASE_ADDRESS;
        Disp2Map.AddressWidth = DISP2_MAP_WIDTH;
        STAPIGAT_MapUser( &Disp2Map );
        if ( Disp2Map.MappedBaseAddress )
        {
            printf( "Disp2 base adress= 0x%lx " ,  Disp2Map.BaseAddress );
            printf( "New Disp2 base adress= 0x%lx (linux)\n" ,  (unsigned long) Disp2Map.MappedBaseAddress );
        }
        #endif
        /** Mapping TTX base address for USER  **/
        TTXMap.BaseAddress = TTX_MAP_BASE_ADDRESS;
        TTXMap.AddressWidth = TTX_MAP_WIDTH;
        STAPIGAT_MapUser( &TTXMap );
        if ( TTXMap.MappedBaseAddress )
        {
            printf( "Old ttx base adress= 0x%lx " ,  TTXMap.BaseAddress );
            printf( "New ttx base adress= 0x%lx (linux)\n" ,  (unsigned long) TTXMap.MappedBaseAddress );
        }

        /** Mapping Blitter base address for USER **/
        BlitterMap.BaseAddress = BLITTER_MAP_BASE_ADDRESS;
        BlitterMap.AddressWidth = BLITTER_MAP_WIDTH;
        STAPIGAT_MapUser( &BlitterMap );
        if ( BlitterMap.MappedBaseAddress )
        {
            printf( "Old Blitter base adress= 0x%lx " ,  BlitterMap.BaseAddress );
            printf( "New Blitter base adress= 0x%lx (linux)\n" ,  (unsigned long) BlitterMap.MappedBaseAddress );
        }

        /** Mapping DVP base address for USER **/
        DvpMap.BaseAddress = DVP_MAP_BASE_ADDRESS;
        DvpMap.AddressWidth = DVP_MAP_WIDTH;
        STAPIGAT_MapUser( &DvpMap );
        if ( DvpMap.MappedBaseAddress )
        {
            printf( "Old VIN(DVP) base adress= 0x%lx " ,  DvpMap.BaseAddress );
            printf( "New VIN(DVP) base adress= 0x%lx (linux)\n" ,  (unsigned long) DvpMap.MappedBaseAddress );
        }

    }
    else
    {
           printf( "Error loading stapigat file device !!\n" );
           RetOk = FALSE ;
    }
#endif /* !USE_GFB */
#endif /* ST_OSLINUX */
    ncache_partition    = NCachePartition_p;
    system_partition    = SystemPartition_p;
    DriverPartition_p   = SystemPartition_p;

	if (RetOk)
    {
#ifdef STAPIGAT_USE_NEW_INIT_API
    RetOk = StartInitComponents(TTDefaultScript);
#else
    RetOk = StartInitComponents(NULL);
#endif
    }

	if (RetOk)
    {
        RetOk = StartSettingDevices();
    }

#ifdef USE_TESTTOOL
    if (RetOk)
    {
        RetOk = StartRegisterCmds();
    }
#endif

#ifdef USE_AVMEM
    if (RetOk)
    {
        RetOk = AVMEM_Command();
    }
#endif

    if (!RetOk)
    {
        LOCAL_PRINT(("Errors reported during system initialisation\n"));
    }
    else
    {
        LOCAL_PRINT(("No errors during initialisation\n"));
    }

#ifdef ST_OS20
    DisplaySoftwareConfiguration();
#endif /* ST_OS20 */

#ifndef ST_OSLINUX
#ifdef DCACHE_ENABLE
    IsDCacheOn = !( DCacheMap_p[0].StartAddress==NULL && DCacheMap_p[0].EndAddress==NULL );
#else
    IsDCacheOn = FALSE;
#endif
#endif

    LOCAL_PRINT(("======================================================================\n"));
    LOCAL_PRINT(("                          Test Application \n"));
#if defined(DVD_STAPI_DEBUG)
    LOCAL_PRINT(("                       %s - %s \n", __DATE__, __TIME__  ));
#endif /*DVD_STAPI_DEBUG*/
#ifndef REMOVE_GENERIC_ADDRESSES /* way to know if STB_FRONTEND == STB_BACKEND */
    LOCAL_PRINT(("                   Chip %s on board %s\n", STB_FRONTEND, BOARD_NAME ));
#else
    LOCAL_PRINT(("                   Chips %s (f/e) and %s (b/e) on board %s\n", STB_FRONTEND, STB_BACKEND, BOARD_NAME ));
#endif
    if (StartSearchVersion(ChipVersion))
    {
       LOCAL_PRINT(("                    Chip version = %s \n", ChipVersion));
    }

    LOCAL_PRINT(("                    CPU Clock Speed = %d Hz\n", ST_GetClockSpeed() ));

    LOCAL_PRINT(("                    CPU Clock per second = %d clk/sec\n", ST_GetClocksPerSecond() ));


#ifdef ICACHE_ENABLE
    LOCAL_PRINT(("                     ICache = ON  DCache = %s\n", IsDCacheOn ? "ON":"OFF"));
#else
    LOCAL_PRINT(("                     ICache = OFF DCache = %s\n", IsDCacheOn ? "ON":"OFF"));
#endif
    LOCAL_PRINT(("                       UNIFIED_MEMORY: "));
#if defined UNIFIED_MEMORY
    LOCAL_PRINT(("TRUE\n"));
#else
    LOCAL_PRINT(("FALSE\n"));
#endif
    LOCAL_PRINT(("======================================================================\n"));


#ifdef USE_VALID_DHRYSTONE
    task_priority_set( task_id(),MIN_USER_PRIORITY+1);
#else /*  USE_VALID_DHRYSTONE */
    /* Lowest priority for task root */
   #if !defined(ST_OSLINUX)
    task_priority_set( task_id(),MIN_USER_PRIORITY);
   #endif
#endif


#ifdef USE_TESTTOOL
    /* In scripts there 'should' not be any abbreviation ... */
/*    STTST_SetMode(STTST_NO_ABBREVIATION_MODE);*/

#ifdef TESTTOOL_BATCH_MODE
    STTST_SetMode(STTST_BATCH_MODE);
#endif

    STTST_Start();

#endif /* #ifdef USE_TESTTOOL */

} /* end STAPIGAT_Init */

/*-------------------------------------------------------------------------
 * Function : StartSearchVersion
 *            Read the chip cut version
 * Input    : None
 * Output   : a string with the formated name
 * Return   : TRUE if success, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL StartSearchVersion(char* FullCutName)
{
    BOOL Found = FALSE;
#ifdef STB_FRONTEND_5528
    U32  Value = 0;
#endif /* STB_FRONTEND_5528 */
    *FullCutName = 0;

#ifdef STB_FRONTEND_5528

#ifdef ST_OSLINUX /* hardware access not permitted here */
	sprintf(FullCutName,"N/A");
#else
     /* Look for STi5528 version */
    #ifdef ST_OS20
    Value = STSYS_ReadRegDev32LE( 0x19162004 ); /* Bad pokeware ... Use Register name instead! */
    #endif /* ST_OS20 */
    #ifdef ST_OS21
    Value = STSYS_ReadRegDev32LE( CFG_BASE_ADDRESS ); /* Should be the same for ST20 and ST40 ! */
    #endif
    sprintf(FullCutName,"%x.%x",(Value>>4) & 0xF, (Value & 0xF));
#endif /* Linux porting */

    Found = TRUE;
#endif /* STB_FRONTEND_5528 */

    return (Found);
}   /* end of StartSearchVersion */

/*-------------------------------------------------------------------------
 * Function : StartInterconnectConfiguration
 *            Sets interconnect configuration for some chips
 * Input    : None
 * Output   : None
 * Return   : None
 * ----------------------------------------------------------------------*/
static void StartInterconnectConfiguration(void)
{
#ifdef SELECT_DEVICE_STB5118
    /* Interconnect configurations */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_1_PRIORITY, 0x8);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_1_LIMIT, 0x196);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_2_PRIORITY, 0x7);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_2_LIMIT, 0x98);

    STSYS_WriteRegDev32LE((void*)NHD2_INIT_3_PRIORITY, 0x2);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_3_LIMIT, 0x3C1);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_4_PRIORITY, 0x1);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_4_LIMIT, 0x071);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_5_PRIORITY, 0x5);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_5_LIMIT, 0x3B1);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_6_PRIORITY, 0x4);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_6_LIMIT, 0x231);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_7_PRIORITY, 0x3);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_7_LIMIT, 0x1B1);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_8_PRIORITY, 0x6);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_8_LIMIT, 0x321);

    STSYS_WriteRegDev32LE((void*)NHD2_TARG_1_PRIORITY, 0x2);
    STSYS_WriteRegDev32LE((void*)NHD2_TARG_2_PRIORITY, 0x1);

    STSYS_WriteRegDev32LE((void*)(CLOCK_REG_BASE+0x0), 0xFF);
    STSYS_WriteRegDev32LE((void*)(CLOCK_REG_BASE+0x4), 0x05);

    /* CFG_VIDIC */
    STSYS_WriteRegDev32LE((void*)(0x20500010), 0x12);
#endif /* SELECT_DEVICE_STB5118 */
#ifdef ST_5188
    U32 RegValue;

    /* Interconnect configurations */
    RegValue=STSYS_ReadRegDev32LE((void*)(CONFIG_CONTROL_C));
    STSYS_WriteRegDev32LE((void*)(CONFIG_CONTROL_C), (0x00002A80|RegValue));/*To control the blitter priority bt E register*/

    RegValue = STSYS_ReadRegDev32LE((void*)(CONFIG_CONTROL_E));
    STSYS_WriteRegDev32LE((void*)(CONFIG_CONTROL_E),(0x33450000|RegValue));/*CONFIG_CONTROL_E register settings for blitter proirity*/

    STSYS_WriteRegDev32LE((void*)NHD2_INIT_1_PRIORITY,  0x7);       /* GDMA */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_1_LIMIT,     0x00);
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_2_PRIORITY,  0x2);       /* LCMPEG */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_2_LIMIT,     0x17440);   /* 76.2MB/s over 5us*/
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_3_PRIORITY,  0x6);       /* FDMA */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_3_LIMIT,     0x17FF0);   /* 20.3 MB/s over 19us */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_4_PRIORITY,  0x3);       /*  Blitter S1 */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_4_LIMIT,     0xFFC0);    /* 14.0 MB/sec over 18.3 us frame */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_5_PRIORITY,  0x3);       /* Blitter S2 */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_5_LIMIT,     0x18FF0);   /* 21.6 MB/s over 18.5 us frame */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_6_PRIORITY,  0x4);       /* Blitter S3 */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_6_LIMIT,     0x1EAA0);   /* 40.1 MB/s over 12.4 us frame */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_7_PRIORITY,  0x5);       /* Blitter D */
    STSYS_WriteRegDev32LE((void*)NHD2_INIT_7_LIMIT,     0x1F7D0);   /* 56.3 MB/s over 9.1 us frame */

    STSYS_WriteRegDev32LE((void*)(CLOCK_REG_BASE+0x0), 0xFF);
    STSYS_WriteRegDev32LE((void*)(CLOCK_REG_BASE+0x4), 0x05);
#endif

}   /* end of StartInterconnectConfiguration */

/*-------------------------------------------------------------------------
 * Function : STAPIGAT_Term
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void STAPIGAT_Term(void)
{
#if defined(ST_OSLINUX) && !defined(USE_GFB)                        /* user mode unmapping for linux */
    if (STAPIGAT_DeviceOpened)
    {
        STAPIGAT_UnmapUser( &ClockMap );
        STAPIGAT_UnmapUser( &SysCfgMap );
        STAPIGAT_UnmapUser( &OutputStageMap );
        STAPIGAT_UnmapUser( &CompoMap );
        #if defined (ST_7200)
            STAPIGAT_UnmapUser( &Compo1Map );
            STAPIGAT_UnmapUser( &OutputStageSDMap );
        #endif
        STAPIGAT_UnmapUser( &Disp0Map );
        STAPIGAT_UnmapUser( &Disp1Map );
        #if defined (ST_7200)
        STAPIGAT_UnmapUser( &Disp2Map );
        #endif
        STAPIGAT_UnmapUser( &TTXMap );
        STAPIGAT_UnmapUser( &BlitterMap );
        STAPIGAT_UnmapUser( &DencMap );
        STAPIGAT_UnmapUser( &DvpMap );

        STAPIGAT_CloseFileDev();
        STAPIGAT_DeviceOpened = FALSE;
    }
#endif

#ifdef USE_CLKRV
    CLKRV_Term();
#endif

#ifdef USE_PTI
    PTI_TermComponent();
#endif

#ifdef USE_DEMUX
    DEMUX_TermComponent();
#endif

#ifdef USE_PWM
    PWM_Term();
#endif

#ifdef USE_AVMEM
    AVMEM_Term();
#endif

#ifdef USE_STLLI
    LLI_Term();
#endif

#ifdef USE_FDMA
#if !defined ST_OSLINUX
    /* Term is done when unloading the module */
    FDMA_Term();
#endif
#endif

#ifdef USE_GPDMA
    GPDMA_Term();
#endif

#ifdef USE_I2C
    I2C_Term();
#endif

#ifdef USE_INTMR
    INTMR_Term();
#endif

#if defined USE_EVT || defined USE_POLL
    EVT_Term();
#endif

#ifdef USE_TESTTOOL
    TST_Term();
#endif

#ifdef USE_TBX
    TBX_Term();
#endif

#ifdef USE_TSTMEM
    TSTMEM_Term();
#endif

#ifdef USE_VALID_TRACE
    TRC_Term();
#endif

#ifdef USE_VALID_DHRYSTONE
    DHRY_Term();
#endif

#ifdef USE_TSTREG
    TSTREG_Term();
#endif

#ifdef USE_UART
#if defined(TRACE_UART) && !defined(USE_VALID_TRACE)
    UART_Close();
#endif
    UART_Term();
#endif /* #ifdef USE_UART */

#ifdef USE_PIO
    PIO_Term();
#endif

#if defined(DVD_SECURED_CHIP)
    MES_Term();
#endif

#if !defined ST_OSLINUX
    BOOT_Term();
#endif

} /* end STAPIGAT_Term */

/* End of startup.c */
