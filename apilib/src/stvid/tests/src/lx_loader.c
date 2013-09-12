#if !defined(ST_OSLINUX) || !defined(MODULE)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#endif

#ifndef STUB_FMW_ST40
#include <mme.h>
#include <embx.h>
#include <embxmailbox.h>
#include <embxshm.h>
#endif /* STUB_FMW_ST40 */

#include "lx_loader.h"

#include "stos.h"

#ifdef ST_OSLINUX

#ifdef MODULE
#include "ioclxload.h"
#endif

#else

#include "stlite.h"
#endif

#include "stsys.h"
#include "sttbx.h"
#include "stdevice.h"
#include "testtool.h"
#include "memory_map.h"
#include "stavmem.h" /* for MME_Init / MME_Term */

#if !defined(ST_OSLINUX)
#include "clavmem.h"    /* for AvmemPartitionHandle[] */
#endif

/* --- Constants and static variables ------------------ */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#    define EMBX_SHARED_MEMORY_SIZE     (512*1024)
#    if defined(ST_OS20) && (defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE))
extern int PpEmbxDispatcher_interrupt_init(void);
#    endif /* defined(ST_OS20)  && (defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)) */
#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    #define MAX_LX_NUMBER   1
    #define MEMORY_START    DELTAPHI_HE_SDRAM_BASE_ADDRESS
    #define MEMORY_SIZE     DELTAPHI_HE_SDRAM_SIZE
    static  void           *address_boot = (void*)DELTAPHI_HE_LX_BOOT_ADDRESS;
    static  void           *address_load = (void*)DELTAPHI_HE_LX_LOAD_ADDRESS;
    static  size_t          maxsize[MAX_LX_NUMBER] = {DELTAPHI_HE_LX_CODE_SIZE};
    static  long bigendian_enable_reg = CPUPLUG_BASE_ADDRESS + 0x14;
    static  long debug_enable_reg     = CPUPLUG_BASE_ADDRESS + 0x10;
    static  long boot_reg             = CPUPLUG_BASE_ADDRESS + 0x18;
    static  long periph_reg           = CPUPLUG_BASE_ADDRESS + 0x1c;
    static  long rst_delta_n          = CPUPLUG_BASE_ADDRESS + 0x20;
#elif defined (ST_7100) || defined (ST_7109)
    #ifndef LMI_SYS_BASE
    #define LMI_SYS_BASE                    tlm_to_virtual(0xA4000000)
    #endif

    #ifndef LMI_VID_BASE
    #define LMI_VID_BASE                    tlm_to_virtual(0xB0000000)
    #endif

    #ifndef SYS_CONF_BASE
    #define SYS_CONF_BASE                   tlm_to_virtual(0xB8000000 + 0x01001000)
    #endif

    #define ST40_LX_ADDRESS_MASK            0xE0000000

    #define LX_VIDEO_CODE_OFFSET            0x00000000      /* Video LX code placed at the very beginning of the LMI_SYS */
    #ifndef LX_VIDEO_CODE_MEMORY_SIZE
        #if defined (ST_7109)
            #if defined(DVD_SECURED_CHIP)
                #define LX_VIDEO_CODE_MEMORY_SIZE   (2*1024*1024)   /*  2 MBytes */
            #else
                #define LX_VIDEO_CODE_MEMORY_SIZE   (3*1024*1024)   /*  3 MBytes */
            #endif /* Secure 7109 */
        #else
            #define LX_VIDEO_CODE_MEMORY_SIZE   (2*1024*1024)   /*  2 MBytes */
        #endif /* defined (ST_7109) */
    #endif /* ifndef LX_VIDEO_CODE_MEMORY_SIZE */
    #define LX_VIDEO_CODE_ADDRESS               (LMI_SYS_BASE + LX_VIDEO_CODE_OFFSET)

    #define LX_AUDIO_CODE_OFFSET                LX_VIDEO_CODE_MEMORY_SIZE      /* Audio LX code placed just after Video LX code in the beginiing of LMI_SYS */
    #ifndef LX_AUDIO_CODE_MEMORY_SIZE
        #define LX_AUDIO_CODE_MEMORY_SIZE       (2*1024*1024)   /*  2 MBytes */
    #endif /* #ifndef LX_AUDIO_CODE_MEMORY_SIZE */
    #define LX_AUDIO_CODE_ADDRESS               (LMI_SYS_BASE + LX_AUDIO_CODE_OFFSET)

    #define MAX_LX_NUMBER           2
    #define MEMORY_START            LMI_SYS_BASE
    #define MEMORY_SIZE             (LMI_VID_BASE+(LMI_VID_SIZE) - LMI_SYS_BASE)
    #define periph_addr_lx1         0x1A000000
    #define periph_addr_lx2         0x1A200000
    #define system_config9          0x124
    #define system_config26         0x168
    #define system_config27         0x16c
    #define system_config28         0x170
    #define system_config29         0x174
    #if defined(DVD_SECURED_CHIP)
        #define LX_BOOT_ENABLE          3
    #else
        #define LX_BOOT_ENABLE          1
    #endif /* Secure 7109 */
    #define LX_RESET                1
    #define system_config9_value    0x50000a8c
#   if !defined(NATIVE_CORE)
    static  EMBX_UINT transport_config[MAX_LX_NUMBER][8]            = {{(MULTICOM_COMPANION_CONFIG & 0x01),
                                                                        (MULTICOM_COMPANION_CONFIG & 0x02) >> 1,
                                                                        (MULTICOM_COMPANION_CONFIG & 0x04) >> 2,
                                                                        (MULTICOM_COMPANION_CONFIG & 0x08) >> 3,
                                                                        0, 0, 0, 0},
                                                                       {0, 0, 0, 0, 0, 0, 0, 0}};
    static  void    *address[MAX_LX_NUMBER]                     = {(void*)LX_VIDEO_CODE_ADDRESS,
                                                                   (void*)LX_AUDIO_CODE_ADDRESS};
    static  size_t  maxsize[MAX_LX_NUMBER]                      = {LX_VIDEO_CODE_MEMORY_SIZE,
                                                                   LX_AUDIO_CODE_MEMORY_SIZE}; /* size allocated in valid mapping */
    static  long    periph_addr[MAX_LX_NUMBER]                  = {periph_addr_lx1,
                                                                   periph_addr_lx2};
    static  long    system_config9_reg                          =  CFG_BASE_ADDRESS + system_config9;
    static  long    boot_addr_reg[MAX_LX_NUMBER]                = {CFG_BASE_ADDRESS + system_config28,
                                                                   CFG_BASE_ADDRESS + system_config26};
    static  long    reset_and_periph_addr_reg[MAX_LX_NUMBER]    = {CFG_BASE_ADDRESS + system_config29,
                                                                   CFG_BASE_ADDRESS + system_config27};
#   endif /* #if !defined(NATIVE_CORE) ... */
#elif defined(ST_7200)
    #define MAX_LX_NUMBER           4
    #define MEMORY_START            LMI0_BASE
    #define MEMORY_SIZE             ((LMI1_BASE+LMI1_SIZE) - LMI0_BASE)
    #define system_config9          0x124
    #define system_config26         0x168
    #define system_config27         0x16c
    #define system_config28         0x170
    #define system_config29         0x174
    #define system_config34         0x188
    #define system_config35         0x18C
    #define system_config36         0x190
    #define system_config37         0x194
    #define LX_BOOT_ENABLE          1
    #define LX_RESET                1
    #define system_config9_value    0x10000a8c
#   if !defined(NATIVE_CORE)
    static  EMBX_UINT transport_config[MAX_LX_NUMBER][8] = {{1, 1, 0, 0, 0, 0, 0, 0},
                                                            {1, 0, 1, 0, 0, 0, 0, 0},
                                                            {1, 0, 0, 1, 0, 0, 0, 0},
                                                            {1, 0, 0, 0, 1, 0, 0, 0}};

    static  void    *address[MAX_LX_NUMBER]                     = {(void*)LX_AUDIO1_CODE_ADDRESS,
                                                                   (void*)LX_VIDEO1_CODE_ADDRESS,
                                                                   (void*)LX_AUDIO2_CODE_ADDRESS,
                                                                   (void*)LX_VIDEO2_CODE_ADDRESS};
    static  size_t  maxsize[MAX_LX_NUMBER]                      = {LX_AUDIO1_CODE_MEMORY_SIZE,
                                                                   LX_VIDEO1_CODE_MEMORY_SIZE,
                                                                   LX_AUDIO2_CODE_MEMORY_SIZE,
                                                                   LX_VIDEO2_CODE_MEMORY_SIZE}; /* size allocated in valid mapping */
    static long system_config9_reg                              = CFG_BASE_ADDRESS + system_config9;
    static long boot_addr_reg[MAX_LX_NUMBER]                    = {CFG_BASE_ADDRESS + system_config26,
                                                                   CFG_BASE_ADDRESS + system_config28,
                                                                   CFG_BASE_ADDRESS + system_config34,
                                                                   CFG_BASE_ADDRESS + system_config36};
    static long reset_reg[MAX_LX_NUMBER]                        = {CFG_BASE_ADDRESS + system_config27,
                                                                   CFG_BASE_ADDRESS + system_config29,
                                                                   CFG_BASE_ADDRESS + system_config35,
                                                                   CFG_BASE_ADDRESS + system_config37};
#   endif /* #if !defined(NATIVE_CORE) ... */
#else
#error "Platform not supported. Please check compilation options"
#endif

BOOL MMEAlreadyInitialised = FALSE;

#if !defined(ST_OSLINUX)
/* variables for MME init stuff */
typedef struct BlockHandleCtx_s
{
#   if defined(ST_7100) || defined(ST_7109)
    STAVMEM_BlockHandle_t  MailboxVideoDataHdl;
#   elif defined (ST_7200)
    STAVMEM_BlockHandle_t  MailboxAudio1DataHdl;
    STAVMEM_BlockHandle_t  MailboxVideo1DataHdl;
    STAVMEM_BlockHandle_t  MailboxAudio2DataHdl;
    STAVMEM_BlockHandle_t  MailboxVideo2DataHdl;
#   endif
} BlockHandleCtx_t;
static BlockHandleCtx_t BlockHandleCtx;
typedef struct FactoryCtx_s
{
#   if defined(ST_7100) || defined(ST_7109)
    EMBX_FACTORY hFactoryVideo;
#   elif defined (ST_7200)
    EMBX_FACTORY hFactoryAudio1;
    EMBX_FACTORY hFactoryVideo1;
    EMBX_FACTORY hFactoryAudio2;
    EMBX_FACTORY hFactoryVideo2;
#   endif
} FactoryCtx_t;
static FactoryCtx_t FactoryCtx;
#endif  /* ST_OSLINUX */


#if defined(ST_OSLINUX) && defined(MODULE)
#define PrintTrace(a)   printk a
#else
#define PrintTrace(a)   printf a
#endif

/* --- Functions ---------------------------------------- */

#if !defined ST_OSLINUX
#ifdef STRESS_VALID_PLATFORM
BOOL Mymemcmp(U32 *Dest, U32 *Src, U32 SizeInByte, U32 *Counter)
{
	U32 Loop;
	BOOL Failed = FALSE;
	U32 ReadDest;
	U32 ReadSrc;
	U32 SizeInWord;

	SizeInWord = SizeInByte / 4;

	for(Loop = 0; Loop < SizeInWord ; Loop++)
	{
		ReadDest = Dest[Loop];
		ReadSrc = Src[Loop];
		if (ReadDest != ReadSrc)
		{
			Failed = TRUE;
			PrintTrace(("Failed at Counter = %d: Read %x instead of %x\n", *Counter, ReadDest, ReadSrc));
		}
	}

	return(Failed);
}

void StressValidPlatform(void)
{
	U32 StressTable[] = {	0x2fcd069c, 0x50c08aca, 0xe23ab6ba, 0xb13e3e41,
						0xc6f81ebe, 0x6aba342d, 0xe1265bda, 0x03d1a02b,
						0x8e6bf9a4, 0xb3c075cd, 0x2b17b77a, 0x780ca4d0,
						0x3fd993e9, 0x608821e8, 0x4a67bb32, 0x9808b640,
						0x48b89a86, 0x027fcf43, 0xaf67f285, 0x811d0893,
						0xd28361ea, 0xc92c6555, 0x5107febb, 0x31f95327,
						0x17827c9e, 0x888015be, 0x6e74fd90, 0x9d950c77,
						0x31cc6913, 0xa463d0cb, 0xcf0a26c4, 0xd345ce08,
						0x61f42097, 0xa5037fa3, 0x2f04d22a, 0x84fd527f,
						0xc8e5b6df, 0xd3655fc5, 0x19c03585, 0x8765483c,
						0x0136ee41, 0x2b523c5a, 0xf28fc03b, 0x09c87d3f,
						0x56b18ec1, 0xbc1b764f, 0x8ea66883, 0x1172a280,
						0x3835657a, 0x1a22e6b2, 0xe2dddca8, 0x44c3c63f,
						0x7c021196, 0xb1b4b28e, 0xb4626766, 0x5651702b,
						0x8e9a9378, 0xafb0659e, 0x85b98d58, 0x665a6871,
						0xf1a32caf, 0x24556b38, 0x3cb0546f, 0x2c3f2833,
						0x26758812, 0x647d3b8c, 0xa24b63ee, 0xa8407ce5,
						0xcc255171, 0xbda2bd24, 0x67a464c1, 0x7002338b,
						0x5e808fd3, 0xe7839f07, 0x850e532e, 0x42338919,
						0xffa26ae4, 0x2a3141cc, 0x124252ad, 0xb6cce05b,
						0xbe5e62d9, 0xdfb1b714, 0xc433a2d9, 0xfc69b6ed,
						0x15fd0028, 0xc00a00e7, 0xe3b784dc, 0x9f637a6b,
						0x2467e69e, 0xa66a08ca, 0xbfc7a367, 0x471e0fcb,
						0x9d8c669d, 0x1f455677, 0xc6e621cf, 0x8d13577a,
						0x05ac9413, 0xe159465a, 0x1f328ec6, 0xf5906c64,
						0xb8622ac7, 0x89a610f5, 0xc7e3dea3, 0x39217dee,
						0xd985e2b5, 0x6741a6dd, 0x96f55fd6, 0xe04cd954,
						0x81008f98, 0x3adeced1, 0x03f35bbc, 0x237f7953,
						0x98fcdc3d, 0xa8b516b8, 0xf27cde9e, 0x65b1aeaa,
						0x89a57dee, 0x431e085e, 0x72c6f7b5, 0x50ae8d95,
						0x2cd17973, 0x9fd1f15c, 0x097b6d6e, 0x5eab0f85,
						0x56859df6, 0x8cf37b00, 0xc4bf2e7e, 0xe54de558,
						0xd0ba0b01, 0x10b06d19, 0x92ee9201, 0x8f36c5ee,
						0xe49f9b8c, 0x14e550df, 0xfd8bf064, 0x48a971b6,
						0xab851f95, 0xcf1d9e0e, 0x7ee536a2, 0xbebbf16c,
						0x0a387f9f, 0x6e953a00, 0xacb4da4d, 0x08bf3bff,
						0xb2d4e34f, 0xe61441ac, 0x1364d1a9, 0x8b3b0af8,
						0x8bc53d22, 0x28e97467, 0x46214ee2, 0x87f0e75e,
						0xea683159, 0xe1db5784, 0xf9a542e3, 0x304f4fee,
						0xf6950c53, 0x9ca6ecb1, 0x5ba8ee15, 0x1c974009,
						0x4af2b462, 0x493cbe5c, 0x183c8542, 0x13595f0a,
						0x1858b595, 0x13733868, 0x7e18ddca, 0xd8ee6bbc,
						0x7eeb50bd, 0x1552ee58, 0x3ff09b8e, 0x4ab85d65,
						0xeeb37c40, 0x3d420761, 0x0393baac, 0x1906f450,
						0x381063c4, 0x31bd3f6d, 0x4fda2ddf, 0x54173687,
						0x039f6f1a, 0x69228d1f, 0x05b8aaab, 0x454b7168,
						0x590dfd7d, 0x43fac454, 0x84469b9b, 0x33092fa6,
						0xcebc6f75, 0x4ee1250d, 0xdb4b76bf, 0x7e131fd8,
						0x7943e870, 0xb621df28, 0xcf07de27, 0xad69f21d,
						0x7313d099, 0xb9fb8ee0, 0x727e7a5d, 0xa53eff73,
						0xfd1646b7, 0x01b88d01, 0xac64709c, 0x4cfbb506,
						0x2330cacf, 0x90bb25ad, 0xc9b83198, 0x05a57fe1
		};

    U32 * Dest1;
    U32 * Dest2;
    U32 * Counter;

    Dest1 = (U32 *)SDRAM_BASE_ADDRESS; /* in emulated LMI */
    Dest2 = (U32 *)SDRAM_BASE_ADDRESS + 0x1000; /* in emulated LMI */
	Counter = (U32 *)0xA3A11004; /* PP_READ_STOP: PP_READ_START is used by the LX in the test */
	(*Counter) = 0;
    while(1)
    {
        memset(Dest1, 0xA5, sizeof(StressTable));
        memset(Dest2, 0x5A, sizeof(StressTable));
        memcpy(Dest1, StressTable, sizeof(StressTable));
        memcpy(Dest2, Dest1, sizeof(StressTable));

        Mymemcmp(Dest2, StressTable, sizeof(StressTable), Counter);
        (*Counter)++;
		if ((*Counter) == 0x70000000)
		{
			(*Counter) = 0;
		}
	}
}
#endif /* STRESS_VALID_PLATFORM */
#endif /* ! ST_OSLINUX */




/***********************************************
                  LX setup for boot
***********************************************/
/*
  LxBootSetup()
*/

#if !defined(ST_OSLINUX) || defined(MODULE)
void LxBootSetup(S32 lx_number, BOOL debug_enable)
{
#if defined(NATIVE_CORE)
#   if   defined(ST_7100) || defined(ST_7109)
    void  *address[MAX_LX_NUMBER]                   = {(void*)LX_VIDEO_CODE_ADDRESS,
                                                       (void*)LX_AUDIO_CODE_ADDRESS};
    long   periph_addr[MAX_LX_NUMBER]               = {periph_addr_lx1,
                                                       periph_addr_lx2};
    long   system_config9_reg                       = CFG_BASE_ADDRESS + system_config9;
    long   boot_addr_reg[MAX_LX_NUMBER]             = {CFG_BASE_ADDRESS + system_config28,
                                                       CFG_BASE_ADDRESS + system_config26};
    long   reset_and_periph_addr_reg[MAX_LX_NUMBER] = {CFG_BASE_ADDRESS + system_config29,
                                                       CFG_BASE_ADDRESS + system_config27};
#   elif defined(ST_7200)
    void *address[MAX_LX_NUMBER]                    = {(void*)LX_AUDIO1_CODE_ADDRESS,
                                                       (void*)LX_VIDEO1_CODE_ADDRESS,
                                                       (void*)LX_AUDIO2_CODE_ADDRESS,
                                                       (void*)LX_VIDEO2_CODE_ADDRESS};
    long system_config9_reg           = CFG_BASE_ADDRESS + system_config9;
    long boot_addr_reg[MAX_LX_NUMBER] = {CFG_BASE_ADDRESS + system_config26,
                                         CFG_BASE_ADDRESS + system_config28,
                                         CFG_BASE_ADDRESS + system_config34,
                                         CFG_BASE_ADDRESS + system_config36};
    long reset_reg[MAX_LX_NUMBER]     = {CFG_BASE_ADDRESS + system_config27,
                                         CFG_BASE_ADDRESS + system_config29,
                                         CFG_BASE_ADDRESS + system_config35,
                                         CFG_BASE_ADDRESS + system_config37};
#   endif /* #elif defined(7200) ... */
#endif /* #if defined(NATIVE_CORE) ... */

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    /* enable reset lx_dh */
    STSYS_WriteRegDev32LE(rst_delta_n , 0);              /* reset on deltaphi is activated */
    /* set to little endian */
    STSYS_WriteRegDev32LE(bigendian_enable_reg , 0);     /* set to little endian */
    /* set periph_address */
    STSYS_WriteRegDev32LE(periph_reg , 0xac0);           /* because lx register [31 down to 20] */
    /* set boot address */
    STSYS_WriteRegDev32LE(boot_reg , ((U32)address_boot)/4);  /* because lx register [31 down to 2] */
    /* set debug_enable */
    STSYS_WriteRegDev32LE(debug_enable_reg, debug_enable);
#elif defined (ST_7100) || defined (ST_7109)
    /* At this stage, Both LX have their reset deasserted by chip reset sequence but can't
       execute instructions because boot_enable is not asserted which prevents
       LX from accessing STBUS */

    /* Reset-out bypass for LX Deltaphi, Audio in order to avoid resetting logic
       following LXs in reset chain when we will assert LX reset */
    STSYS_WriteRegDev32LE(system_config9_reg, system_config9_value);
    /* assert LX reset + set perih addr */
    STSYS_WriteRegDev32LE(reset_and_periph_addr_reg[lx_number], (periph_addr[lx_number] >> 19) | LX_RESET);
    /* set boot address and assert boot_enable */
    STSYS_WriteRegDev32LE(boot_addr_reg[lx_number], (((U32)address[lx_number] - NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET) | LX_BOOT_ENABLE));
#elif defined(ST_7200)
    /* At this stage, all LXs have their reset deasserted by chip reset sequence but can't
       execute instructions because boot_enable is not asserted which prevents
       LX from accessing STBUS */

    /* Reset-out bypass for LX Deltas, Audios in order to avoid resetting logic
       following LXs in reset chain when we will assert LX reset */
    STSYS_WriteRegDev32LE(system_config9_reg, system_config9_value);
    /* assert LX reset + set perih addr */
    STSYS_WriteRegDev32LE(reset_reg[lx_number], LX_RESET);
    /* set boot address and assert boot_enable */
    STSYS_WriteRegDev32LE(boot_addr_reg[lx_number], ((U32)(address[lx_number]-NCACHE_SH4_TO_STBUS_ADDRESS_OFFSET) | LX_BOOT_ENABLE));
#endif

    PrintTrace(("LX %d set in %s mode\n", lx_number+1, debug_enable ? "DEBUG" : "NON-DEBUG"));

} /* LxBootSetup */
#endif  /* !LINUX | MODULE */


/*
  LxReboot()
*/

#if !defined(ST_OSLINUX) || defined(MODULE)
void LxReboot(S32 lx_number, BOOL debug_enable)
{
#if defined(NATIVE_CORE)
#   if defined(ST_7100) || defined(ST_7109)
    long periph_addr[MAX_LX_NUMBER]               = {periph_addr_lx1,
                                                     periph_addr_lx2};
    long reset_and_periph_addr_reg[MAX_LX_NUMBER] = {CFG_BASE_ADDRESS + system_config29,
                                                     CFG_BASE_ADDRESS + system_config27};
#   elif defined(ST_7200)
    long reset_reg[MAX_LX_NUMBER] = {CFG_BASE_ADDRESS + system_config27,
                                     CFG_BASE_ADDRESS + system_config29,
                                     CFG_BASE_ADDRESS + system_config35,
                                     CFG_BASE_ADDRESS + system_config37};
#   endif /* #elif defined(ST_7200) */
#endif /* #if defined(NATIVE_CORE) ... */

    UNUSED_PARAMETER(debug_enable);

    PrintTrace(("Starting LX %d by disabling reset\n", lx_number+1));
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    STSYS_WriteRegDev32LE(rst_delta_n , 1);              /* reset on deltaphi is de-activated */
#elif defined(ST_7100) || defined(ST_7109)
    /* deassert LX reset -> boot (+ set periph address again...) */
    STSYS_WriteRegDev32LE(reset_and_periph_addr_reg[lx_number], (periph_addr[lx_number] >> 19));
#elif defined(ST_7200)
    /* deassert LX reset -> boot */
    STSYS_WriteRegDev32LE(reset_reg[lx_number], 0);
#endif /* #elif defined(ST_7200) */
} /* LxReboot */
#endif  /* !LINUX | MODULE */


/* ========================================================================
   testool command : load a code into memory
   ======================================================================== */
#ifdef ST_OSLINUX
#if !defined(MODULE)
static BOOL setup_lx (STTST_Parse_t * pars_p, char *result_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    return(FALSE);
}
#endif  /* !MODULE */

#else   /* LINUX */

static BOOL setup_lx (STTST_Parse_t * pars_p, char *result_p)
{
    BOOL            error = FALSE;
    char            filename[640];
    char            MSG_String[256];
    size_t  transfer;
    FILE    *fd;
    S32     debug_enable;
    S32     lx_number;
    BOOL    load_code;


#ifdef NATIVE_CORE
    printf("SETUP LX IS DUMMY IN NATIVE EXECUTION\n");
    return(FALSE);
#endif

    UNUSED_PARAMETER(result_p);

/* Retrieve command line parameters */
    if (STTST_GetString(pars_p, "file", filename, 640) ||
        (strlen (filename) == 0))
    {
        tag_current_line (pars_p, "Illegal file name");
        return(TRUE);
    }

    if(STTST_GetInteger( pars_p, 1, &lx_number) ||
       (lx_number < 1) ||
       (lx_number > MAX_LX_NUMBER))
    {
        sprintf(MSG_String,"expected lx_number 1..%d", MAX_LX_NUMBER);
        STTST_TagCurrentLine( pars_p, MSG_String);
        return(TRUE);
    }
    lx_number--;

    if(STTST_GetInteger( pars_p, 0, &debug_enable))
    {
        STTST_TagCurrentLine( pars_p, "expected debug_enable flag (0|FALSE=disable, 1|TRUE=enable" );
        return(TRUE);
    }

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    if(debug_enable)
    {
        debug_enable = 1;
    }
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
   debug_enable = 0; /* On 7100, the debug_enable pin is tied to ground */
#endif

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    /* In case of palladium platform, we always lod the code from host,
       even when using LX debugger */
    load_code = TRUE;
#else /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */
    /* In all other cases, if debugger is used it loads the LX code */
    load_code = !debug_enable;
#endif /* not defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

    if (strlen (filename) == 1)
    {
        LxReboot(lx_number, debug_enable);
    }
    else
    {
        LxBootSetup(lx_number, debug_enable);

        if(load_code)
        {
            fd = fopen (filename, "rb");
            if ( fd == NULL )
            {
                PrintTrace(("unable to open file : %s\n",filename));
                return (TRUE );
            }
            PrintTrace(("loading file %s into memory\n", filename));

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
            transfer = fread(address_load, 1, maxsize[lx_number], fd);
#else
            transfer = fread(address[lx_number], 1, maxsize[lx_number], fd);
#endif
            fclose (fd);
            if ( transfer == maxsize[lx_number] )
            {
                PrintTrace(("ERROR !!!! LX %d CODE SIZE TOO LARGE Max is %d, actual size is %d\n", lx_number+1, (int)(maxsize[lx_number]), (int)transfer ));
            }


#if 0     /* code removed as it is not needed anymore to patch the audio firmware */
#if defined(ST_7100) || defined(ST_7109)
           /* Modify CPU synchro for audio firmware : byte based at offset 0x2000 */

            if (lx_number == 1)

          {
#   if   (EMBXSHM_MAX_CPUS == 4)
                *((unsigned char*)address[lx_number]+0x2000) = (0x20 | (MULTICOM_COMPANION_CONFIG & 0xf));
#   elif (EMBXSHM_MAX_CPUS == 8)
                *((unsigned char*)address[lx_number]+0x2000) = (MULTICOM_COMPANION_CONFIG & 0xf);
                *((unsigned char*)address[lx_number]+0x2001) = 0x20;
#   endif
            }

#endif
#endif /* 0 */

            PrintTrace(("%d bytes have been transfered into memory\n", transfer ));
        }

        LxReboot(lx_number, debug_enable);

#ifdef STRESS_VALID_PLATFORM
        task_lock();
        interrupt_lock();
        StressValidPlatform();
        interrupt_unlock();
        task_unlock();
#endif
    }

    return (error);
}
#endif  /* ST_OSLINUX */


#ifndef STUB_FMW_ST40
/*******************************************************************************
Name        : MailboxInit
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : EMBX_FACTORY *hFactory
*******************************************************************************/
#if !defined(ST_OSLINUX)
static ST_ErrorCode_t MailboxInit(const EMBX_CHAR * MMETransportName, EMBX_VOID *sharedAddr, EMBX_UINT sharedSize, EMBX_VOID *warpRangeAddr, EMBX_UINT warpRangeSize, EMBX_FACTORY *hFactory_p)
{
#if defined(NATIVE_CORE)
#   if defined(ST_7100) || defined(ST_7109)
    EMBX_UINT transport_config[MAX_LX_NUMBER][8] = { {(MULTICOM_COMPANION_CONFIG & 0x01),
                                                      (MULTICOM_COMPANION_CONFIG & 0x02) >> 1,
                                                      (MULTICOM_COMPANION_CONFIG & 0x04) >> 2,
                                                      (MULTICOM_COMPANION_CONFIG & 0x08) >> 3,
                                                      0, 0, 0, 0},  /* participants */
                                                     {0, 0, 0, 0, 0, 0, 0, 0}};

#   elif defined(ST_7200)
    EMBX_UINT transport_config[MAX_LX_NUMBER][8] = {{1, 1, 0, 0, 0, 0, 0, 0},
                                                    {1, 0, 1, 0, 0, 0, 0, 0},
                                                    {1, 0, 0, 1, 0, 0, 0, 0},
                                                    {1, 0, 0, 0, 1, 0, 0, 0}};
#   endif
#endif

#if defined(ST_OSWINCE)
	DWORD MB_LX_Sysintr;
#endif /* defined(ST_OSWINCE) */

    /* TODO (LD) : error management */
    EMBX_ERROR ErrorCode = EMBX_SUCCESS;
    int transport_index;

    EMBXSHM_MailboxConfig_t config =

        {
            "",              /* name of the transport */
            0,                  /* cpuID --> 0 means Master */

#   if   (EMBXSHM_MAX_CPUS == 4)
            {0, 0, 0, 0},       /* participants */
#   elif (EMBXSHM_MAX_CPUS == 8)
            {0, 0, 0, 0, 0, 0, 0, 0},    /* participants */
#   endif

/* Currently we keep the old setting when using NATIVE_CORE (VSOC) */
#ifdef NATIVE_CORE
            0x50000000,         /* pointerWarp --> LMI_VID is seen at 0xB0000000 by ST40 and 0x10000000 by LX (see Mailbox doc) */
#else /* #if defined(NATIVE_CORE) ... */
#   if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#       if defined(ST_OS20)
            0xC0000000,         /* pointerWarp --> ST20 : 0x0 -
                                       0x4000.0000 (LMI_VID: on palladium)--> 0xC0000000*/
#       else  /* #if defined(ST_OS20) ... */
            0x60000000,         /* pointerWarp --> ST40 : 0x0 -
                                       0xA000.0000 --> 0x60000000 */
#       endif /* #if defined(ST_OS20) ... #else ... */

#   elif defined (ST_7100) || defined (ST_7109) || defined(ST_7200)
            0x60000000,         /* pointerWarp --> LMI_SYS is seen at 0xA4000000 by ST40 and 0x04000000 by LX (see Mailbox doc)*/
#   else
            #error "Platform not supported. Please check compilation options"
#   endif
#endif /* #if defined(NATIVE_CORE) ... #else ... */

            0,                  /* maxPorts */
            16,                 /* maxObjects */
            16,                 /* freeListSize */
            sharedAddr,         /* CPU Shared address */
            sharedSize,         /* Size of shared memory */
            warpRangeAddr,      /* warpRangeAddr */
            warpRangeSize       /* warpRangeSize */
        };

    /* copy transport name */
    strncpy(config.name, MMETransportName, strlen(MMETransportName)<=EMBX_MAX_TRANSPORT_NAME?strlen(MMETransportName):EMBX_MAX_TRANSPORT_NAME);

#if defined(ST_7100) || defined(ST_7109)
    if(strcmp(MME_AUDIO_TRANSPORT_NAME, MME_VIDEO_TRANSPORT_NAME))
    {
        PrintTrace(("Multicom configuration with different transports for audio and video not supported\n"));
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Init() failed; err=%d\n", ST_ERROR_BAD_PARAMETER));
        return(ST_ERROR_BAD_PARAMETER);
    }

    transport_index = 0;
#elif defined(ST_7200)
    /* Set array of participants */
    if (strncmp(config.name, MME_AUDIO1_TRANSPORT_NAME, strlen(MME_AUDIO1_TRANSPORT_NAME)) == 0)
    {
        transport_index = 0;
    }
    else if (strncmp(config.name, MME_VIDEO1_TRANSPORT_NAME, strlen(MME_VIDEO1_TRANSPORT_NAME)) == 0)
    {
        transport_index = 1;
    }
    else if (strncmp(config.name, MME_AUDIO2_TRANSPORT_NAME, strlen(MME_AUDIO2_TRANSPORT_NAME)) == 0)
    {
        transport_index = 2;
    }
    else if (strncmp(config.name, MME_VIDEO2_TRANSPORT_NAME, strlen(MME_VIDEO2_TRANSPORT_NAME)) == 0)
    {
        transport_index = 3;
    }
    else
    {
        return MME_EMBX_ERROR;
    }
#endif
    /* In case of a multicom compiled with EMBXSHM_MAX_CPUS set to 8, then copy the whole array,
       else copy juste the necessary cells to the config structure */
    memcpy(config.participants, transport_config[transport_index], EMBXSHM_MAX_CPUS * sizeof(EMBX_UINT));

    ErrorCode = EMBX_Mailbox_Init();
    if ((ErrorCode != EMBX_SUCCESS) &&
        (ErrorCode != EMBX_ALREADY_INITIALIZED))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Init() failed; err=%d\n", ErrorCode));
        return(ErrorCode);
    }

    if (MMEAlreadyInitialised == FALSE)
    {
/* mailbox registering */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#ifdef ST_OS20
    /* Init external interrupt dispatcher for PP0 & MBX */
    if (PpEmbxDispatcher_interrupt_init())
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Interrupt Displatcher inti failed; err=%d\n",ErrorCode));
        return(1);
    }
    ErrorCode = EMBX_Mailbox_Register((void *)DELTAPHI_HE_MAILBOX_BASE_ADDRESS, OS20_MBX_SOFTWARE_IT, 5, EMBX_MAILBOX_FLAGS_SET2);
#else
    ErrorCode = EMBX_Mailbox_Register((void *)DELTAPHI_HE_MAILBOX_BASE_ADDRESS,OS21_INTERRUPT_IRL_ENC_5, -1, EMBX_MAILBOX_FLAGS_SET2);
#endif
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
#elif defined(ST_7100)
    ErrorCode = EMBX_Mailbox_Register((void *)ST7100_MAILBOX_BASE_ADDRESS, OS21_INTERRUPT_MB_LX_DPHI, -1, EMBX_MAILBOX_FLAGS_SET2);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 1 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
    ErrorCode = EMBX_Mailbox_Register((void *)ST7100_MAILBOX_1_BASE_ADDRESS, OS21_INTERRUPT_MB_LX_AUDIO, -1, EMBX_MAILBOX_FLAGS_SET2);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 2 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
#elif defined(ST_7109)
#if defined(ST_OSWINCE)
    MB_LX_Sysintr = GetNewSysintr(ST7109_MAILBOX_LX_DELTAPHI);
    ErrorCode = EMBX_Mailbox_Register((void *)ST7109_MAILBOX_BASE_ADDRESS, MB_LX_Sysintr, -1, EMBX_MAILBOX_FLAGS_SET2);
#else /* defined(ST_OSWINCE) */
    ErrorCode = EMBX_Mailbox_Register((void *)ST7109_MAILBOX_BASE_ADDRESS, OS21_INTERRUPT_MB_LX_DPHI, -1, EMBX_MAILBOX_FLAGS_SET2);
#endif /* defined(ST_OSWINCE) */
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 1 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
#if defined(ST_OSWINCE)
    MB_LX_Sysintr = GetNewSysintr(ST7109_MAILBOX_LX_AUDIO);
    ErrorCode = EMBX_Mailbox_Register((void *)ST7109_MAILBOX_1_BASE_ADDRESS, MB_LX_Sysintr, -1, EMBX_MAILBOX_FLAGS_SET2);
#else  /* defined(ST_OSWINCE) */
    ErrorCode = EMBX_Mailbox_Register((void *)ST7109_MAILBOX_1_BASE_ADDRESS, OS21_INTERRUPT_MB_LX_AUDIO, -1, EMBX_MAILBOX_FLAGS_SET2);
#endif /* defined(ST_OSWINCE) */
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 2 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
#elif defined(ST_7200)
    ErrorCode = EMBX_Mailbox_Register((void *)ST7200_MAILBOX_0_LX_AUDIO_0_BASE_ADDRESS, -1, -1, 0);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 2 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
    ErrorCode = EMBX_Mailbox_Register((void *)ST7200_MAILBOX_1_LX_DELTAMU_0_BASE_ADDRESS, ST7200_MB_LX_DPHI0_INTERRUPT, -1, EMBX_MAILBOX_FLAGS_SET2);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 1 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
    ErrorCode = EMBX_Mailbox_Register((void *)ST7200_MAILBOX_2_LX_AUDIO_1_BASE_ADDRESS, -1, -1, 0);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 2 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
    ErrorCode = EMBX_Mailbox_Register((void *)ST7200_MAILBOX_3_LX_DELTAMU_1_BASE_ADDRESS, -1, -1, 0);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_Mailbox_Register 2 failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
#endif /* defined(7200) */
/* End of mailbox registering */
    }

    ErrorCode = EMBX_RegisterTransport(EMBXSHM_mailbox_factory, &config, sizeof(config), hFactory_p);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_RegisterTransport() failed; err=%d\n",ErrorCode));
        return(ErrorCode);
    }
    /*
     * Initialise the EMBX library, this will attempt to create
     * a transport based on the factory we just registered.
     */
    ErrorCode = EMBX_Init();
    if ((ErrorCode != EMBX_SUCCESS) || (ErrorCode != EMBX_ALREADY_INITIALIZED))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX initialisation failed err = %d\n",ErrorCode));
        return(ErrorCode);
    }

    return (ErrorCode);
} /* end of MailboxInit */
#endif  /* !ST_OSLINUX */

/*******************************************************************************
Name        : MailboxTerm
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#if !defined(ST_OSLINUX)
static ST_ErrorCode_t MailboxTerm(EMBX_FACTORY hFactory)
{
    EMBX_ERROR ErrorCode = EMBX_SUCCESS;

    if((ErrorCode = EMBX_Deinit()) != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX termination failed err = %d\n",ErrorCode));
        return(ST_ERROR_TIMEOUT);
    }

    ErrorCode = EMBX_UnregisterTransport(hFactory);
    if (ErrorCode != EMBX_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EMBX_UnregisterTransport() failed, error=%d\n", ErrorCode));
        return(ST_ERROR_TIMEOUT);
    }

/* MailBox Deregistering */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    EMBX_Mailbox_Deregister((void *)DELTAPHI_HE_MAILBOX_BASE_ADDRESS);
#elif defined(ST_7100)
    EMBX_Mailbox_Deregister((void *)ST7100_MAILBOX_BASE_ADDRESS);
    EMBX_Mailbox_Deregister((void *)ST7100_MAILBOX_1_BASE_ADDRESS);
#elif defined(ST_7109)
    EMBX_Mailbox_Deregister((void *)ST7109_MAILBOX_BASE_ADDRESS);
    EMBX_Mailbox_Deregister((void *)ST7109_MAILBOX_1_BASE_ADDRESS);
#elif defined(ST_7200)
    EMBX_Mailbox_Deregister((void *)ST7200_MAILBOX_0_LX_AUDIO_0_BASE_ADDRESS);
    EMBX_Mailbox_Deregister((void *)ST7200_MAILBOX_1_LX_DELTAMU_0_BASE_ADDRESS);
    EMBX_Mailbox_Deregister((void *)ST7200_MAILBOX_2_LX_AUDIO_1_BASE_ADDRESS);
    EMBX_Mailbox_Deregister((void *)ST7200_MAILBOX_3_LX_DELTAMU_1_BASE_ADDRESS);
#endif
/* End of mailbox De_registering */

    return(ST_NO_ERROR);
} /* end of MailboxTerm */
#endif  /* !ST_OSLINUX */
#endif /* STUB_FMW_ST40 */


/*******************************************************************************
Name        : MMEStart_Init
Description :
Parameters  : input:  AVMEMPartition
              output: MailboxDataHdl_p, CompanionDataSharedMemorySize, CompanionDataSharedMemoryAddress, hFactory_p
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#if !defined(ST_OSLINUX)
ST_ErrorCode_t MMEStart_Init(const EMBX_CHAR * MMETransportName, STAVMEM_PartitionHandle_t AVMEMPartition, STAVMEM_BlockHandle_t  *MailboxDataHdl_p, EMBX_FACTORY *hFactory_p)
{
    ST_ErrorCode_t                          ErrorCode = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t              AllocBlockParams;
    void                                    * VirtualAddress, * DeviceAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    U32                                     CompanionDataSharedMemorySize;
    void                                    * CompanionDataSharedMemoryAddress;

    void                                    * CPUAddress;
    void                                    * WarpRangeAddress;
    U32                                     WarpRangeSize;

    /* MailBox needs some memory to run: allocate a block specific for mailbox, sized enough to hold:
     * - the mailbox datastructure: size is TODO!
     * - the MME parameters (computed above)
     */
    AllocBlockParams.PartitionHandle          = AVMEMPartition;
    AllocBlockParams.Alignment                = 4;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    AllocBlockParams.Size                     = EMBX_SHARED_MEMORY_SIZE;
    if(STAVMEM_AllocBlock(&AllocBlockParams, MailboxDataHdl_p) != ST_NO_ERROR)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    if(STAVMEM_GetBlockAddress(*MailboxDataHdl_p, (void **)&VirtualAddress) != ST_NO_ERROR)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    DeviceAddress  = STAVMEM_VirtualToDevice(VirtualAddress,&VirtualMapping);
    VirtualAddress = VirtualMapping.VirtualBaseAddress_p;
    CPUAddress = STAVMEM_VirtualToCPU(VirtualAddress,&VirtualMapping);

    /* store size & address of shared memory between ST40 & LX --> used by MailboxInit() */
#ifndef STUB_FMW_ST40
    CompanionDataSharedMemorySize = AllocBlockParams.Size;
    CompanionDataSharedMemoryAddress = STAVMEM_DeviceToCPU(DeviceAddress, &VirtualMapping);
#else /* not STUB_FMW_ST40 */
    CompanionDataSharedMemorySize = 0;
    CompanionDataSharedMemoryAddress = NULL;
#endif /* STUB_FMW_ST40 */

#if defined(STUB_PP) && defined(STUB_FMW_ST40)
    ErrorCode = MMESTUB_Init(NULL);
#else
#ifndef STUB_FMW_ST40
    WarpRangeAddress = CPUAddress;
#   if   defined(ST_7100) || defined(ST_7109)
    WarpRangeSize = 0x20000000;
#   elif defined(ST_7200)
    WarpRangeSize = 0x20000000;
#   endif

    ErrorCode = MailboxInit(MMETransportName, (EMBX_VOID *)CompanionDataSharedMemoryAddress, (EMBX_UINT)CompanionDataSharedMemorySize, (EMBX_VOID *)WarpRangeAddress, (EMBX_UINT)WarpRangeSize, hFactory_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        if (ErrorCode == EMBX_ALREADY_INITIALIZED)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MailboxInit() failed, result=$%lx (EMBX_ALREADY_INITIALIZED)", ErrorCode));
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MailboxInit() failed, result=$%lx", ErrorCode));
            return (ErrorCode);
        }
    }

#endif /* !STUB_FMW_ST40 */

    if (MMEAlreadyInitialised == FALSE)
    {
        ErrorCode = MME_Init();
#endif /*  #if defined(STUB_PP) && defined(STUB_FMW_ST40) */
        if (ErrorCode != MME_SUCCESS)
        {
            if (ErrorCode == MME_DRIVER_ALREADY_INITIALIZED)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MME_Init() succeeded, result=$%lx (ALREADY_INITIALIZED)", ErrorCode));
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MME_Init() failed, result=$%lx", ErrorCode));
                return (ErrorCode);
            }
        }
        MMEAlreadyInitialised = TRUE;
    }

    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"MME API initialized"));

    ErrorCode = MME_RegisterTransport(MMETransportName);
    if (ErrorCode != MME_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"ERROR: cannot register transport to external processors (%d)\n", ErrorCode));
        return (ErrorCode);
    }
    return (ST_NO_ERROR);
} /* end of MMEStart_Init */
#endif /* !ST_OSLINUX */

/*******************************************************************************
Name        : MMEStart_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
#if !defined(ST_OSLINUX)
ST_ErrorCode_t MMEStart_Term(STAVMEM_PartitionHandle_t AVMEMPartition, STAVMEM_BlockHandle_t *MailboxDataHdl_p, EMBX_FACTORY hFactory)
{
    ST_ErrorCode_t ErrorCode        = ST_NO_ERROR;
    STAVMEM_FreeBlockParams_t   	FreeParams;

#if defined(STUB_FMW_ST40) && defined(STUB_PP)
	MMESTUB_Term("");
#else
    MME_Term();
#ifndef STUB_FMW_ST40
    MailboxTerm(hFactory);
#endif /* !STUB_FMW_ST40 */
#endif

    FreeParams.PartitionHandle = AVMEMPartition;
    ErrorCode = STAVMEM_FreeBlock(&FreeParams, MailboxDataHdl_p);

    return (ErrorCode);

} /* MMEStart_Term */
#endif /* !ST_OSLINUX */

/*-----------------------------------------------------------
                     Testool command
------------------------------------------------------------*/
#ifdef ST_OSLINUX
#if !defined(MODULE)
static BOOL enable_deltaphi (STTST_Parse_t * pars_p, char *result_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    return(FALSE);
}
#endif  /* !MODULE */

#else   /* LINUX */
static BOOL enable_deltaphi (STTST_Parse_t * pars_p, char *result_p)
{
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
	long rst_delta_n = CPUPLUG_BASE_ADDRESS + 0x20;
    S32    enable;
    int     RetErr;

    RetErr = STTST_GetInteger( pars_p, 1, &enable);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected enable flag (0|FALSE=disable, 1|TRUE=enable" );
        return(TRUE);
    }
    if(enable)
    {
        enable = 1;
    }
    STSYS_WriteRegDev32LE(rst_delta_n , (U32)enable); /* reset on deltaphi is de-activated */
#else
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);
#endif
    return 0;
}
#endif  /* ST_OSLINUX */

/*-----------------------------------------------------------
                     Testool command
------------------------------------------------------------*/
#ifdef ST_OSLINUX
#if !defined(MODULE)
static BOOL lxloader_mmeinit (STTST_Parse_t * pars_p, char *result_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    return(FALSE);
}
#endif  /* !MODULE */

#else    /* LINUX */

static BOOL lxloader_mmeinit (STTST_Parse_t * pars_p, char *result_p)
{
    ST_ErrorCode_t ErrorCode;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    PrintTrace(("Start MME_Init...\n"));

#if defined(ST_7100) || defined(ST_7109)
    ErrorCode = MMEStart_Init(MME_VIDEO_TRANSPORT_NAME, AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideoDataHdl, &FactoryCtx.hFactoryVideo);
    if (ErrorCode != ST_NO_ERROR)
    {
        PrintTrace(("MME_StartInit returned error = 0x%x\n", ErrorCode));
        return(TRUE);
    }
    else
    {
        PrintTrace(("MME_Init successfully complete\n"));
        return(FALSE);
    }
#elif defined(ST_7200)
#   if ((MULTICOM_COMPANION_CONFIG & 0x02) != 0)
    ErrorCode = MMEStart_Init(MME_AUDIO1_TRANSPORT_NAME, AvmemPartitionHandle[0], &BlockHandleCtx.MailboxAudio1DataHdl, &FactoryCtx.hFactoryAudio1);
    if (ErrorCode != ST_NO_ERROR)
    {
        PrintTrace(("Transport %s: MME_StartInit returned error = 0x%x\n", MME_AUDIO1_TRANSPORT_NAME, ErrorCode));
        return(TRUE);
    }
    else
    {
        PrintTrace(("Transport %s: MME_Init successfully complete\n", MME_AUDIO1_TRANSPORT_NAME));
    }
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x04) != 0)
        ErrorCode = MMEStart_Init(MME_VIDEO1_TRANSPORT_NAME, AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideo1DataHdl, &FactoryCtx.hFactoryVideo1);
        if (ErrorCode != ST_NO_ERROR)
        {
            PrintTrace(("Transport %s: MME_StartInit returned error = 0x%x\n", MME_VIDEO1_TRANSPORT_NAME, ErrorCode));
            return(TRUE);
        }
        else
        {
            PrintTrace(("Transport %s: MME_Init successfully complete\n", MME_VIDEO1_TRANSPORT_NAME));
        }
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x08) != 0)
    ErrorCode = MMEStart_Init(MME_AUDIO2_TRANSPORT_NAME, AvmemPartitionHandle[0], &BlockHandleCtx.MailboxAudio2DataHdl, &FactoryCtx.hFactoryAudio2);
    if (ErrorCode != ST_NO_ERROR)
    {
        PrintTrace(("Transport %s: MME_StartInit returned error = 0x%x\n", MME_AUDIO2_TRANSPORT_NAME, ErrorCode));
        return(TRUE);
    }
    else
    {
        PrintTrace(("Transport %s: MME_Init successfully complete\n", MME_AUDIO2_TRANSPORT_NAME));
    }
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x10) != 0)
    ErrorCode = MMEStart_Init(MME_VIDEO2_TRANSPORT_NAME, AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideo2DataHdl, &FactoryCtx.hFactoryVideo2);
    if (ErrorCode != ST_NO_ERROR)
    {
        PrintTrace(("Transport %s: MME_StartInit returned error = 0x%x\n", MME_VIDEO2_TRANSPORT_NAME, ErrorCode));
        return(TRUE);
    }
    else
    {
        PrintTrace(("Transport %s: MME_Init successfully complete\n", MME_VIDEO2_TRANSPORT_NAME));
    }
#   endif
#endif

    return(FALSE);
}
#endif  /* ST_OSLINUX */

/*-----------------------------------------------------------
                     Testool command
------------------------------------------------------------*/
#ifdef ST_OSLINUX
#if !defined(MODULE)
static BOOL lxloader_mmeterm (STTST_Parse_t * pars_p, char *result_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    return(FALSE);
}
#endif  /* !MODULE */

#else   /* LINUX */

static BOOL lxloader_mmeterm (STTST_Parse_t * pars_p, char *result_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_p);

    PrintTrace(("Terminate MME\n"));

#if defined(ST_7100) || defined(ST_7109)
    MMEStart_Term(AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideoDataHdl, FactoryCtx.hFactoryVideo);
#elif defined(ST_7200)
#   if ((MULTICOM_COMPANION_CONFIG & 0x02) != 0)
    MMEStart_Term(AvmemPartitionHandle[0], &BlockHandleCtx.MailboxAudio1DataHdl, FactoryCtx.hFactoryAudio1);
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x04) != 0)
    MMEStart_Term(AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideo1DataHdl, FactoryCtx.hFactoryVideo1);
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x08) != 0)
    MMEStart_Term(AvmemPartitionHandle[0], &BlockHandleCtx.MailboxAudio2DataHdl, FactoryCtx.hFactoryAudio2);
#   endif
#   if ((MULTICOM_COMPANION_CONFIG & 0x10) != 0)
    MMEStart_Term(AvmemPartitionHandle[0], &BlockHandleCtx.MailboxVideo2DataHdl, FactoryCtx.hFactoryVideo2);
#   endif
#endif
    return(FALSE);
}
#endif  /* ST_OSLINUX */

/*-----------------------------------------------------------
                     Testool command
------------------------------------------------------------*/
BOOL LxLoaderInitCmds( void )
{
#if !defined(ST_OSLINUX) || !defined(MODULE)
    STTST_RegisterCommand("SETUP_LX" , setup_lx,"<file name> <LX number 1..NbLXs, default=1> <debug_enable TRUE|FALSE, default=FALSE> load lx code from file and setup LX.");
    STTST_RegisterCommand("DELTAPHI_ENABLE" , enable_deltaphi,"<TRUE(default))|FALSE> Enable/disable deltaphi using its reset");
    STTST_RegisterCommand("MME_INIT" , lxloader_mmeinit,"Initialize MME mailbox & transports, must be call once after SETUP_LX");
    STTST_RegisterCommand("MME_TERM" , lxloader_mmeterm,"Terminate MME mailbox & transports");
#ifdef STVID_VALID
#if ((MULTICOM_COMPANION_CONFIG & 0x04) != 0)
    STTST_AssignInteger("LX1_COMPANION_USED", 1, FALSE);
#else /* ((MULTICOM_COMPANION_CONFIG & 0x02) != 0) */
    STTST_AssignInteger("LX1_COMPANION_USED", 0, FALSE);
#endif /* not ((MULTICOM_COMPANION_CONFIG & 0x02) != 0) */
#if ((MULTICOM_COMPANION_CONFIG & 0x10) != 0)
    STTST_AssignInteger("LX2_COMPANION_USED", 1, FALSE);
#else /* ((MULTICOM_COMPANION_CONFIG & 0x04) != 0) */
    STTST_AssignInteger("LX2_COMPANION_USED", 0, FALSE);
#endif /* not ((MULTICOM_COMPANION_CONFIG & 0x04) != 0) */
#if ((MULTICOM_COMPANION_CONFIG & 0x08) != 0)
    STTST_AssignInteger("LX3_COMPANION_USED", 1, FALSE);
#else /* ((MULTICOM_COMPANION_CONFIG & 0x08) != 0) */
    STTST_AssignInteger("LX3_COMPANION_USED", 0, FALSE);
#endif /* not ((MULTICOM_COMPANION_CONFIG & 0x08) != 0) */
#if ((MULTICOM_COMPANION_CONFIG & 0x10) != 0)
    STTST_AssignInteger("LX4_COMPANION_USED", 1, FALSE);
#else /* ((MULTICOM_COMPANION_CONFIG & 0x10) != 0) */
    STTST_AssignInteger("LX4_COMPANION_USED", 0, FALSE);
#endif /* not ((MULTICOM_COMPANION_CONFIG & 0x10) != 0) */
#endif /* STVID_VALID */

#endif  /* !ST_OSLINUX || !MODULE */

    return(TRUE);
}
