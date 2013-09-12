/************************************************************************

File name   : vmix_test.c

Description : VMIX specific test macros

COPYRIGHT (C) STMicroelectronics 2003

Date          Modification                                    Initials
----          ------------                                    --------
14 May 2001   Creation                                          JG
13 May 2003   Add support for Stem7020                          HS
10 Jun 2003   Add support for 5528                              HS
01 Apr 2004   Add support for 5100                              TM
01 Oct 2004   Add support for OS21                              MH
30 Dec 2004   Add support for 5105                              NM
19 Jan 2006   Add support for 5188                              MKBA
21 Feb 2006   Add support for 5525                              SBEBA
 *
************************************************************************/

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
#include "stgxobj.h"
#include "stvmix.h"

#include "clevt.h"
#include "api_cmd.h"
#include "vmix_cmd.h"
#include "startup.h"

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)\
|| defined (ST_5162)
#include "clavmem.h"
#endif

/* Private Types ------------------------------------------------------------ */

typedef struct TaskParams_s
{
    STVMIX_InitParams_t VMIXInitParams;
    STVMIX_Capability_t VMIXCapability;
    STVMIX_OpenParams_t VMIXOpenParams;
    STVMIX_Handle_t VMIXHndl;
    char VTGName[10];
    STVMIX_LayerDisplayParams_t  *VMIXLayerArray[5];
    STGXOBJ_ColorRGB_t RGB;
    BOOL Enable;
    STVMIX_ScreenParams_t Screen_p;
    S8 Horizontal;
    S8 Vertical;
    STVMIX_TermParams_t TermVMIXParam;
    STVMIX_LayerDisplayParams_t VMIXLayerDisplayParams;
} TaskParams_t;

/* Private Constants -------------------------------------------------------- */

/* --- Constants (default values) -------------------------------------- */
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || \
    ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))
#define MIXER_BASE_ADDRESS (VIDEO_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  0
#define MIXER_DEVICE_TYPE  (STVMIX_OMEGA1_SD)
#elif defined (ST_5100)
#define MIXER_COMPO_BASE_ADDRESS        (ST5100_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5100_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5100_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5100_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#elif defined (ST_5162)
#define MIXER_COMPO_BASE_ADDRESS        (ST5162_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5162_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5162_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5162_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (8 * 1024)
#elif defined (ST_5301)
#define MIXER_COMPO_BASE_ADDRESS        (ST5301_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5301_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5301_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5301_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#elif defined (ST_5525)
#define MIXER_COMPO_BASE_ADDRESS        (ST5525_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5525_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5525_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5525_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)

#elif defined (ST_5105)
#define MIXER_COMPO_BASE_ADDRESS        (ST5105_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5105_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5105_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
/*#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5105_TILE_RAM_BASE_ADDRESS)*/
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#elif defined (ST_5188)
#define MIXER_COMPO_BASE_ADDRESS        (ST5188_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5188_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5188_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#elif defined (ST_5107)
#define MIXER_COMPO_BASE_ADDRESS        (ST5107_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA_BASE_ADDRESS         (ST5107_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5107_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               (STVMIX_COMPOSITOR)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#elif defined (ST_5528)
#define MIXER_BASE_ADDRESS    0
#define MIXER_BASE_OFFSET  ST5528_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_5528)
#elif defined (ST_7015)
#define MIXER_BASE_ADDRESS (STI7015_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7015_GAMMA_OFFSET+ST7015_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  (STVMIX_GAMMA_TYPE_7015)
#elif defined (ST_GX1)
#define MIXER_BASE_ADDRESS (0xBB410000)
#define MIXER_DEVICE_TYPE  (STVMIX_GAMMA_TYPE_GX1)
#define MIXER_BASE_OFFSET  0x500
#elif defined (ST_7020)
#define MIXER_BASE_ADDRESS (STI7020_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7020_GAMMA_OFFSET+ST7020_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_7020)
#elif defined (ST_7710)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET   ST7710_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_7710)
#elif defined (ST_7100)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET   ST7100_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_7100)
#elif defined (ST_7109)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET   ST7109_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_7109)
#elif defined(ST_7200)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET   ST7200_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE  (STVMIX_GENERIC_GAMMA_TYPE_7200)

#else
#error Not defined address & type for mixer
#endif

/* Private Variables (static)------------------------------------------------ */
#if defined (ST_7020)
#define MAX_LAYER 7
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"GDP3", "VID1", "VID2", "GDP1", "GDP2", "ALPHA", "CURS"};
/*   Layer numbers              1       2       3        4      5        6       7    */
#elif defined (ST_7710)
#define MAX_LAYER 7
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"VID1", "GDP1", "GDP2", "ALPHA", "CURS", "GDPVBI1", "GDPVBI2"};
/*   Layer numbers              1       2       3        4      5          6       7  */
#elif defined (ST_7100)
#define MAX_LAYER 5
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"VID1", "GDP1", "GDP2", "ALPHA", "CURS"};
/*   Layer numbers              1       2       3        4      5    */
#elif defined (ST_7109)
#define MAX_LAYER 7
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"VID1", "GDP1", "GDP2","GDP3", "GDP4", "ALPHA", "CURS"};
/*   Layer numbers              1       2       3        4      5       6       7 */
#elif defined (ST_7200)
#define MAX_LAYER 7
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"VID1", "VID2", "GDP1", "GDP2","GDP3", "GDP4", "CURS"};
/*   Layer numbers              1       2       3        4      5       6       7 */

#elif defined (ST_5528)
#define MAX_LAYER 8
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"GDP1", "GDP2", "VID1", "VID2", "GDP3", "GDP4", "ALPHA", "CURS"};
/*   Layer numbers              1       2       3        4      5        6       7        8   */

#elif defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107)\
   || defined (ST_5162)
#define MAX_LAYER 3
static STVMIX_Handle_t VMIXHndlConnect;
static char Tab_Connect[MAX_LAYER][10] = {"GFX1", "VID1", "GFX2"};
/*   Layer numbers              1       2       3   */

#elif defined (ST_5510) || defined (ST_5512) || defined (ST_5514) || defined (ST_5516) || defined (ST_5517)
#define MAX_LAYER 5
static char Tab_Connect[MAX_LAYER][10]  = {"BCKG", "STIL", "VID1", "LOSD", "CURS"};

#elif defined (ST_5508) || defined (ST_5518)
#define MAX_LAYER 4
static char Tab_Connect[MAX_LAYER][10] = {"BCKG", "VID1", "LOSD", "CURS"};

#elif defined (ST_7015)
#define MAX_LAYER 6
static char Tab_Connect[MAX_LAYER][10] = {"BCKG", "GFX2", "VID1", "VID2", "GFX1", "CURS"};

#elif defined (ST_GX1)
#define MAX_LAYER 6
static char Tab_Connect[MAX_LAYER][10] = {"BCKG", "GDP1", "GDP2", "GDP3", "GDP4", "CURS"};
#endif

#if defined (ST_7015)
static int tab1[] = {
0,0,0,0,0};
static int tab2[] = {
0,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,8,8,8,8};
static int tab3[] = {
0,0,0,0,0,0,0,0,0,8,8,8,8,8,8,8,0,0,8,0,\
0,8,8,8,8,8,8,8,0,0,8,0,0,8,8,8,8,8,8,8,\
0,0,8,0,0,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static int tab4[] = {
0,0,0,0,8,8,0,0,0,0,8,8,0,0,0,0,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,0,8,8,8,8,8,0,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,8,8,\
8,8,8,0,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,\
8,0,8,8,8,8,8,0,8,8,8,8,8,8,8,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static int tab5[] = {
0,8,0,8,8,8,0,8,0,8,8,8,0,8,0,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,\
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
#endif

#if defined (ST_GX1)
static int tab1[] = {
0,0,0,0,0};
static int tab2[] = {
0,0,0,0,8,0,0,0,8,8,0,0,8,8,8,0,8,8,8,8};
static int tab3[] = {
0,0,0,8, 0,0,8,8, 0,8,8,8, 8,8,8,8, 0,0,8,8,
0,8,8,8, 8,8,8,8, 8,8,8,8, 0,8,8,8, 8,8,8,8,
8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8};
static int tab4[] = {
0,0,8,0,8,8, 8,8,8,0,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,0,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8};
static int tab5[] = {
0,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8,
8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8, 8,8,8,8,8,8};
#endif


/* Global Variables --------------------------------------------------------- */

char    MIX_Msg[200];            /* text for trace */
char    MIX_Msg1[100];           /* text for trace */

STVMIX_LayerDisplayParams_t   LayerParam[5];

#if defined (ST_5100)  || defined (ST_5301) || defined (ST_5525) || defined (ST_5162)

extern STGXOBJ_Bitmap_t  VMIXTEST_CacheBitmap;
#endif

#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
STAVMEM_BlockHandle_t              CacheBlockHandle;
extern STGXOBJ_Bitmap_t            VMIXTEST_CacheBitmap;
#endif

/* Private Macros ----------------------------------------------------------- */

#define stvmix_read(a)      STSYS_ReadRegMem32LE((void*)(a))
#define stvmix_write(a,v)   STSYS_WriteRegMem32LE((void*)(a),(v))

/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
#endif


/* Private Function prototypes ---------------------------------------------- */
static BOOL VMIX_NullPointersTest( parse_t *pars_p, char *result_sym_p );
static BOOL VMIX_TestScreenParams( parse_t *pars_p, char *result_sym_p );
static BOOL VMIX_TestStartParams( parse_t *pars_p, char *result_sym_p );
static BOOL VMIX_TestOffsetParams( parse_t *pars_p, char *result_sym_p );
#if defined (ST_7015) || defined (ST_GX1)
static BOOL VMIX_ConnectLayersTest( parse_t *pars_p, char *result_sym_p );
#endif


BOOL Test_CmdStart(void);
void os20_main(void *ptr);

#ifdef ARCHITECTURE_ST20
static void ReportError(int Error, char *FunctionName);

static void test_overhead(void *dummy);
static void test_typical(void *dummy);

/* Functions ---------------------------------------------------------------- */

static void ReportError(int Error, char *FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned %d\n", FunctionName, Error );
    }
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;
#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    void*                       Cache_p;
    STAVMEM_AllocBlockParams_t  AllocParams;
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#endif /* defined(ST_5105) || defined(ST_5188) || defined (ST_5107) */


    Params_p = (TaskParams_t *) dummy;
    Err = ST_NO_ERROR;

#if defined (ST_5100) || defined (ST_5301) || defined (ST_5525) || defined (ST_5162)
    Params_p->VMIXInitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    Params_p->VMIXInitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)MIXER_GDMA_BASE_ADDRESS;
    Params_p->VMIXInitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType                           = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion                = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p                             = (void*)MIXER_TILE_RAM_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.Size1                               = MIXER_TILE_RAM_SIZE;
    Params_p->VMIXInitParams.CacheBitmap_p                   = &VMIXTEST_CacheBitmap;
    Params_p->VMIXInitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
#if defined (ST_5100)
#if defined (USE_AVMEMCACHED_PARTITION)
    Params_p->VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[1];
#else
    Params_p->VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif
#else  /* defined (ST_5100)*/
    Params_p->VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif
#elif defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Allocate a block to be used as a cache in the composition operations */
    AllocParams.PartitionHandle          = AvmemPartitionHandle[0];
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.Size                     = MIXER_TILE_RAM_SIZE;
    AllocParams.Alignment                = 1024;

    Err = STAVMEM_AllocBlock (&AllocParams, &CacheBlockHandle);
    if ( Err != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't allocate cache in shared memory : %s\n",Err));
        return;
    }
    Err = STAVMEM_GetBlockAddress (CacheBlockHandle, (void **)&(Cache_p));
    if ( Err != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't get the block address for the allocated cache : %s\n",Err));
        /* Free the allocated block before leaving */
        FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
        Err = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
        if(Err != ST_NO_ERROR)
        {
            STTBX_Print (("Error : Can't deallocate the cache block : %s\n",Err));
        }
        return;
    }
    Params_p->VMIXInitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    Params_p->VMIXInitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)MIXER_GDMA_BASE_ADDRESS;
    Params_p->VMIXInitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType                 = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion      = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p                   = (void*)Cache_p;
    VMIXTEST_CacheBitmap.Size1                     = MIXER_TILE_RAM_SIZE;
    Params_p->VMIXInitParams.CacheBitmap_p = &VMIXTEST_CacheBitmap;
    Params_p->VMIXInitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
    Params_p->VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];

#else
    Params_p->VMIXInitParams.DeviceBaseAddress_p = (void *)MIXER_BASE_ADDRESS;
    Params_p->VMIXInitParams.BaseAddress_p = (void *)MIXER_BASE_OFFSET;
#endif

    Params_p->VMIXInitParams.CPUPartition_p  = SystemPartition_p;
    Params_p->VMIXInitParams.DeviceType = MIXER_DEVICE_TYPE;
    Params_p->VMIXInitParams.MaxOpen = 1;
    Params_p->VMIXInitParams.MaxLayer = 5;
    strcpy( Params_p->VMIXInitParams.VTGName, "");
    strcpy( Params_p->VMIXInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    Params_p->VMIXInitParams.OutputArray_p = NULL;

    Err = STVMIX_Init( STVMIX_DEVICE_NAME, &(Params_p->VMIXInitParams));
    ReportError(Err, "STVMIX_Init");

    Err = STVMIX_GetCapability(STVMIX_DEVICE_NAME, &(Params_p->VMIXCapability));
    ReportError(Err, "STVMIX_GetCapability");

    Err = STVMIX_Open(STVMIX_DEVICE_NAME, &(Params_p->VMIXOpenParams), &(Params_p->VMIXHndl));
    ReportError(Err, "STVMIX_Open");

    strcpy( Params_p->VTGName, "VTG");
    Err = STVMIX_SetTimeBase(Params_p->VMIXHndl, Params_p->VTGName);
    ReportError(Err, "STVMIX_SetTimeBase");

    strcpy(LayerParam[0].DeviceName, Tab_Connect[1]);
    LayerParam[0].ActiveSignal = FALSE;
    Params_p->VMIXLayerArray[0] = &LayerParam[0];
    strcpy(LayerParam[1].DeviceName, Tab_Connect[2]);
    LayerParam[1].ActiveSignal = FALSE;
    Params_p->VMIXLayerArray[1] = &LayerParam[1];
    strcpy(LayerParam[2].DeviceName, Tab_Connect[3]);
    LayerParam[2].ActiveSignal = FALSE;
    Params_p->VMIXLayerArray[2] = &LayerParam[2];
    strcpy(LayerParam[3].DeviceName, Tab_Connect[4]);
    LayerParam[3].ActiveSignal = FALSE;
    Params_p->VMIXLayerArray[3] = &LayerParam[3];
    strcpy(LayerParam[4].DeviceName, Tab_Connect[5]);
    LayerParam[4].ActiveSignal = FALSE;
    Params_p->VMIXLayerArray[4] = &LayerParam[4];
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5516) || ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))
    Err = STVMIX_ConnectLayers(Params_p->VMIXHndl, (const STVMIX_LayerDisplayParams_t**)Params_p->VMIXLayerArray, 4);
#elif defined (ST_5508) || defined (ST_5518) || defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) \
    || defined (ST_5525) || defined (ST_5107) || defined (ST_5162)
    Err = STVMIX_ConnectLayers(Params_p->VMIXHndl, (const STVMIX_LayerDisplayParams_t**)Params_p->VMIXLayerArray, 3);
#elif defined (ST_7015) || defined (ST_7020) || defined (ST_GX1) || defined (ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) \
    || defined (ST_7200)
    Err = STVMIX_ConnectLayers(Params_p->VMIXHndl, (const STVMIX_LayerDisplayParams_t**)Params_p->VMIXLayerArray, 5);
#endif
    ReportError(Err, "STVMIX_ConnectLayers");

    Err = STVMIX_GetConnectedLayers(Params_p->VMIXHndl, 1 , &Params_p->VMIXLayerDisplayParams );
    ReportError(Err, "STVMIX_GetConnectedLayers");


    Err = STVMIX_Enable( Params_p->VMIXHndl);
    ReportError(Err, "STVMIX_Enable");

#if defined (ST_7015) || defined (ST_GX1)
    Params_p->RGB.R = 128;
    Params_p->RGB.G = 128;
    Params_p->RGB.B = 128;
    Params_p->Enable = TRUE,
    Err = STVMIX_SetBackgroundColor( Params_p->VMIXHndl, Params_p->RGB, Params_p->Enable);
    ReportError(Err, "STVMIX_SetBackgroundColor");
#endif

    Params_p->Screen_p.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    Params_p->Screen_p.ScanType = STGXOBJ_INTERLACED_SCAN;
    Params_p->Screen_p.FrameRate  = 59964;
    Params_p->Screen_p.Width = 720;
    Params_p->Screen_p.Height = 480;
    Params_p->Screen_p.XStart = 0;
    Params_p->Screen_p.YStart = 0;
    Err = STVMIX_SetScreenParams( Params_p->VMIXHndl, &Params_p->Screen_p);
    ReportError(Err, "STVMIX_SetScreenParams");

    Params_p->Horizontal = 0;
    Params_p->Vertical = 0;
    Err = STVMIX_SetScreenOffset( Params_p->VMIXHndl, Params_p->Horizontal, Params_p->Vertical);
    ReportError(Err, "STVMIX_SetScreenOffset");

    /* */

#if defined (ST_7015) || defined (ST_GX1)
    Err = STVMIX_GetBackgroundColor( Params_p->VMIXHndl, &Params_p->RGB, &Params_p->Enable);
    ReportError(Err, "STVMIX_GetBackgroundColor");
#endif

    Err = STVMIX_GetScreenParams( Params_p->VMIXHndl, &Params_p->Screen_p);
    ReportError(Err, "STVMIX_GetScreenParams");

    Err = STVMIX_GetScreenOffset( Params_p->VMIXHndl, &Params_p->Horizontal, &Params_p->Vertical);
    ReportError(Err, "STVMIX_GetScreenOffset");

    Err = STVMIX_GetTimeBase(Params_p->VMIXHndl, (ST_DeviceName_t*)&Params_p->VTGName);
    ReportError(Err, "STVMIX_GetTimeBase");

    Err = STVMIX_Disable( Params_p->VMIXHndl);
    ReportError(Err, "STVMIX_Disable");

    Err = STVMIX_DisconnectLayers(Params_p->VMIXHndl);
    ReportError(Err, "STVMIX_DisconnectLayers");

    Err = STVMIX_Close(Params_p->VMIXHndl);
    ReportError(Err, "STVMIX_Close");

    Params_p->TermVMIXParam.ForceTerminate = FALSE;
    Err = STVMIX_Term( STVMIX_DEVICE_NAME, &(Params_p->TermVMIXParam));
    ReportError(Err, "STVMIX_Term");
#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Free the allocated block before leaving */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    Err = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
    if(Err != ST_NO_ERROR)
    {
        STTBX_Print (("Error : Can't deallocate the cache block : %s\n",Err));
    }
#endif
}

void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;
    Err = ST_NO_ERROR;

    Params_p = NULL;

   ReportError(Err, "test_overhead");
}
#endif /* ARCHITECTURE_ST20 */

/*-------------------------------------------------------------------------
 * Function : ReturnCode
 * Description :  compare Error code value
 * Input    : ST_ErrorCode_t ErrCode, Expected
 * Output   :
 * Return   : U16, 0 or 1 if test failed
 * ----------------------------------------------------------------------*/
static U16 ReturnCode( ST_ErrorCode_t ErrCode, ST_ErrorCode_t Expected)
{
    U16 Err=0;

    if ( ErrCode != Expected )
    {
        STTBX_Print(( " unexpected return code=%d !\n", ErrCode ));
        Err++;
    }
    else
    {
        STTBX_Print(( " ok\n"));
    }
    return(Err);
}

/*-------------------------------------------------------------------------
 * Function : VMIX_NullPointersTest
 * Description : Bad parameter test : function call with null pointer
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_NullPointersTest(parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    STVMIX_InitParams_t VMIXInitParams;
    STVMIX_OpenParams_t VMIXOpenParams;
    STVMIX_Capability_t VMIXCapability;
    STVMIX_Handle_t VMIXHndl1;
    STVMIX_TermParams_t VMIXTermParams;
    STGXOBJ_ColorRGB_t* Rgb888=NULL;
    S8* Horizontal=NULL;
    S8* Vertical=NULL;
    STVMIX_ScreenParams_t VMIXScreenParams;
    BOOL* Enable=NULL;
    char VTGName[60];
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;
#if !defined(ST_5105) && !defined(ST_5188) && !defined (ST_5107) && !defined (ST_5162)
    int DeviceType = 0;
#endif

#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    void*                         Cache_p;
    STAVMEM_AllocBlockParams_t  AllocParams;
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#endif /* defined(ST_5105) || defined(ST_5188) || defined (ST_5107) */

    STTBX_Print(( "Call API functions with null pointers...\n" ));
    NbErr = 0;


    /* get DeviceType */
#if !defined(ST_5105) && !defined(ST_5188) && !defined (ST_5107) && !defined (ST_5162)
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&DeviceType );
#else
    RetErr = STTST_GetInteger( pars_p, MIXER_DEVICE_TYPE, (S32 *)&VMIXInitParams.DeviceType );
#endif

    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Device Type" );
        return(TRUE);
    }

    VMIXInitParams.CPUPartition_p = SystemPartition_p;
#if !defined(ST_5105) && !defined(ST_5188) && !defined (ST_5107) && !defined (ST_5162)
    VMIXInitParams.DeviceType = (STVMIX_DeviceType_t)DeviceType;
#endif
    VMIXInitParams.MaxOpen = 1;
#if defined (ST_5100)|| defined (ST_5301)|| defined (ST_5525) || defined (ST_5162)
    VMIXInitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)MIXER_GDMA_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType                 = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion      = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p                   = (void*)MIXER_TILE_RAM_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.Size1                     = MIXER_TILE_RAM_SIZE;
    VMIXInitParams.CacheBitmap_p                   = &VMIXTEST_CacheBitmap;
    VMIXInitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
#if defined (ST_5100)
#if defined (USE_AVMEMCACHED_PARTITION)
    VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[1];
#else
    VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif
#else  /* defined (ST_5100)*/
    VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif
#elif defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Allocate a block to be used as a cache in the composition operations */
    AllocParams.PartitionHandle          = AvmemPartitionHandle[0];
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.Size                     = MIXER_TILE_RAM_SIZE;
    AllocParams.Alignment                = 1024;

    ErrCode = STAVMEM_AllocBlock (&AllocParams, &CacheBlockHandle);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't allocate cache in shared memory : %s\n",ErrCode));
        return ( API_EnableError ? TRUE : FALSE );
    }
    ErrCode = STAVMEM_GetBlockAddress (CacheBlockHandle, (void **)&(Cache_p));
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't get the block address for the allocated cache : %s\n",ErrCode));
        /* Free the allocated block before leaving */
        FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
        ErrCode = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print (("Error : Can't deallocate the cache block : %s\n",ErrCode));
        }
        return ( API_EnableError ? TRUE : FALSE );
    }
    VMIXInitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)MIXER_GDMA_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType                 = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion      = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p                   = (void*)Cache_p;
    VMIXTEST_CacheBitmap.Size1                     = MIXER_TILE_RAM_SIZE;
    VMIXInitParams.CacheBitmap_p = &VMIXTEST_CacheBitmap;
    VMIXInitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
    VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#else
    VMIXInitParams.DeviceBaseAddress_p = (void *)MIXER_BASE_ADDRESS;
    VMIXInitParams.BaseAddress_p = (void *)MIXER_BASE_OFFSET;
#endif
    VMIXInitParams.MaxLayer = 1;
    strcpy( VMIXInitParams.VTGName, "");
    strcpy(VMIXInitParams.EvtHandlerName, STEVT_DEVICE_NAME);
    VMIXInitParams.OutputArray_p = NULL;


    /* Test STVMIX_Init with a NULL pointer parameter */
    DeviceName[0] = '\0';
    STTBX_Print(( "---STVMIX_Init(NULL,...) :" ));
    ErrCode = STVMIX_Init(DeviceName, &VMIXInitParams);
    NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

    /* Test STVMIX_Init with a NULL pointer parameter */
    sprintf(DeviceName , STVMIX_DEVICE_NAME);
    STTBX_Print(( "---STVMIX_Init(%s, NULL) :", STVMIX_DEVICE_NAME ));
    ErrCode = STVMIX_Init(DeviceName, NULL);
    NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

    /* Init with Max Open = 0 */
    VMIXInitParams.CPUPartition_p = SystemPartition_p;
    VMIXInitParams.MaxOpen = 0;
    STTBX_Print(( "---STVMIX_Init(%s ... maxopen=0 ...) :", STVMIX_DEVICE_NAME ));
    ErrCode = STVMIX_Init(DeviceName, &VMIXInitParams);
    NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

    VMIXInitParams.MaxOpen = 1;
    /* Init a device to for further tests needing an init device */
    STTBX_Print(( "---STVMIX_Init() :"));
    ErrCode = STVMIX_Init(DeviceName, &VMIXInitParams);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( " unexpected return code=%d \n", ErrCode ));
        NbErr++;
    }
    else
    {
        STTBX_Print(( " ok\n"));
        /* Test STVMIX_Init with a too long DeviceName */
        sprintf (DeviceName, "thisdeviceistoolong");
        STTBX_Print(( "---STVMIX_Init(Toolongdevicename,...) :" ));
        ErrCode = STVMIX_Init(DeviceName, &VMIXInitParams);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_GetCapability with a NULL pointer parameter */
        DeviceName[0] = '\0';
        STTBX_Print(( "---STVMIX_GetCapability(NULL,...) :" ));
        ErrCode = STVMIX_GetCapability(DeviceName, &VMIXCapability);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_GetCapability with a NULL pointer parameter */
        sprintf(DeviceName ,STVMIX_DEVICE_NAME);
        STTBX_Print(( "---STVMIX_GetCapability(%s, NULL,...) :", STVMIX_DEVICE_NAME ));
        ErrCode = STVMIX_GetCapability(DeviceName, NULL);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_GetCapability with a too long DeviceName */
        sprintf (DeviceName, "thisdeviceistoolong");
        STTBX_Print(( "---STVMIX_GetCapability(Toolongdevicename,...) :" ));
        ErrCode = STVMIX_GetCapability(DeviceName, &VMIXCapability);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_Open with a NULL pointer parameter */
        DeviceName[0] = '\0';
        STTBX_Print(( "---STVMIX_Open(NULL,...) :" ));
        ErrCode = STVMIX_Open(DeviceName, &VMIXOpenParams, &VMIXHndl1);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_Open with a NULL pointer parameter */
        sprintf(DeviceName ,STVMIX_DEVICE_NAME);
        STTBX_Print(( "---STVMIX_Open(%s, NULL, ...) :", STVMIX_DEVICE_NAME ));
        ErrCode = STVMIX_Open(DeviceName, NULL, &VMIXHndl1);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_Open with a NULL pointer parameter */
        STTBX_Print(( "---STVMIX_Open(%s, ..., NULL) :", STVMIX_DEVICE_NAME ));
        ErrCode = STVMIX_Open(DeviceName, &VMIXOpenParams, NULL);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Open device to get a good handle for further tests */
        STTBX_Print(( "---STVMIX_Open() :" ));
        ErrCode = STVMIX_Open(DeviceName, &VMIXOpenParams, &VMIXHndl1);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(( "unexpected return code=%d \n", ErrCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( " ok\n"));
            /* Test STVMIX_Open with a too long DeviceName */
            sprintf (DeviceName, "thisdeviceistoolong");
            STTBX_Print(( "---STVMIX_Open(Toolongdevicename,...) :" ));
            ErrCode = STVMIX_Open(DeviceName, &VMIXOpenParams, &VMIXHndl1);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            /* function with pointer parameter */
            STTBX_Print(( "---STVMIX_GetBackgroundColor(Hnd, NULL, Enable) :" ));
            ErrCode = STVMIX_GetBackgroundColor( VMIXHndl1, NULL, Enable);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetBackgroundColor(Hnd, Rgb888, NULL) :" ));
            ErrCode = STVMIX_GetBackgroundColor( VMIXHndl1, Rgb888, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetConnectedLayers(Hnd, 1, NULL) :" ));
            ErrCode = STVMIX_GetConnectedLayers( VMIXHndl1, 1, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetScreenOffset(Hnd, NULL, Vertical) :" ));
            ErrCode = STVMIX_GetScreenOffset( VMIXHndl1, NULL, Vertical);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetScreenOffset(Hnd, Horizontal, NULL) :" ));
            ErrCode = STVMIX_GetScreenOffset( VMIXHndl1, Horizontal, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetScreenParams(Hnd, NULL) :" ));
            ErrCode = STVMIX_GetScreenParams( VMIXHndl1, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_GetTimeBase(Hnd, NULL) :" ));
            ErrCode = STVMIX_GetTimeBase( VMIXHndl1, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            STTBX_Print(( "---STVMIX_SetScreenParams(Hnd, NULL) :" ));
            ErrCode = STVMIX_SetScreenParams( VMIXHndl1, NULL);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            VMIXScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE + 1;
            VMIXScreenParams.ScanType = STGXOBJ_INTERLACED_SCAN;
            STTBX_Print(( "---STVMIX_SetScreenParams(Hnd, NULL) :" ));
            ErrCode = STVMIX_SetScreenParams( VMIXHndl1, &VMIXScreenParams);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            VMIXScreenParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
            VMIXScreenParams.ScanType = STGXOBJ_INTERLACED_SCAN + 1;
            STTBX_Print(( "---STVMIX_SetScreenParams(Hnd, NULL) :" ));
            ErrCode = STVMIX_SetScreenParams( VMIXHndl1, &VMIXScreenParams);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            /* Test STVMIX_SetTimeBase with a NULL pointer parameter and too long devicename */
            VTGName[0] = '\0';
            STTBX_Print(( "---STVMIX_SetTimeBase(Hnd, NULL) :" ));
            ErrCode = STVMIX_SetTimeBase( VMIXHndl1, VTGName);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

            sprintf( VTGName, "VTGtoolongdevicename");
            STTBX_Print(( "---STVMIX_SetTimeBase(Hnd, toolongdevicename) :" ));
            ErrCode = STVMIX_SetTimeBase( VMIXHndl1, VTGName);
            NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        }
        /* Test STVMIX_Term with a NULL pointer parameter */
        DeviceName[0] = '\0';
        STTBX_Print(( "---STVMIX_Term(NULL, ...) :" ));
        ErrCode = STVMIX_Term(DeviceName, &VMIXTermParams);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Test STVMIX_Term with a NULL pointer parameter */
        sprintf(DeviceName ,STVMIX_DEVICE_NAME);
        STTBX_Print(( "---STVMIX_Term(%s, NULL) :", STVMIX_DEVICE_NAME ));
        ErrCode = STVMIX_Term(DeviceName, NULL);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);

        /* Terminate device use */
        VMIXTermParams.ForceTerminate = 1;
        STTBX_Print(( "---STVMIX_Term() :" ));
        ErrCode = STVMIX_Term(DeviceName, &VMIXTermParams);
        NbErr += ReturnCode( ErrCode, ST_NO_ERROR );

        /* Test STVMIX_Term with a too long DeviceName */
        sprintf (DeviceName, "thisdeviceistoolong");
        STTBX_Print(( "---STVMIX_Term(Toolongdevicename,...) :" ));
        ErrCode = STVMIX_Term(DeviceName, &VMIXTermParams);
        NbErr += ReturnCode( ErrCode, ST_ERROR_BAD_PARAMETER);
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VMIX_NullPointersTest() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VMIX_NullPointersTest() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
        RetErr = TRUE;
    }
#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Free the allocated block before leaving */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    ErrCode = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("Error : Can't deallocate the cache block : %s\n",ErrCode));
    }
#endif
    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VMIX_NullPointersTest() */



/*-------------------------------------------------------------------------
 * Function : VMIX_TestScreenParams
 * Description : Screen parameter test : function test all cases
 *              (C code program because macro are not fast for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * Warning  : Max Horizontal and Vertical must be X10
 * ----------------------------------------------------------------------*/
static BOOL VMIX_TestScreenParams(parse_t *pars_p, char *result_sym_p)
{
    STVMIX_Handle_t MixerHandle;
    STVMIX_ScreenParams_t VMIXScreenParams;
    U16 NbErr=0;
    int Horizontal, Vertical, MaxHorizontal, MinHorizontal, MaxVertical, MinVertical;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    STTBX_Print(( "Call API STVMIX_Set/GetScreenParams functions ...\n" ));
    NbErr = 0;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&MixerHandle );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MinHorizontal );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min H " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MaxHorizontal );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max H " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MinVertical );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min V " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MaxVertical );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max V " );
        return(TRUE);
    }

    ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
    if ( ErrCode != ST_NO_ERROR) NbErr++;

    for ( Horizontal = MinHorizontal; Horizontal <= MaxHorizontal; Horizontal+=MaxHorizontal/10 )
    {
        for ( Vertical = MinVertical; Vertical <= MaxVertical; Vertical++ )
        {
            VMIXScreenParams.Width  = Horizontal;
            VMIXScreenParams.Height = Vertical;
            ErrCode = STVMIX_SetScreenParams( MixerHandle, &VMIXScreenParams);
            if ( ErrCode != ST_NO_ERROR)
            {
                NbErr++;
                if(NbErr < 20)
                {
                    sprintf(MIX_Msg, "\tSTVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): Failed !!\n", \
                            MixerHandle,
                            (VMIXScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
                            (VMIXScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                            VMIXScreenParams.FrameRate,
                            VMIXScreenParams.Width, VMIXScreenParams.Height,
                            VMIXScreenParams.XStart, VMIXScreenParams.YStart);
                    STTBX_Print((MIX_Msg));
                }
            }
            else
            {
                ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
                if (( ErrCode != ST_NO_ERROR) ||
                    ( VMIXScreenParams.Width != (U32)Horizontal) ||
                    (VMIXScreenParams.Height != (U32)Vertical))
                {
                    NbErr++;
                    if(NbErr < 20)
                    {
                        if(ErrCode)
                        {
                            STTBX_Print(("\tSTVMIX_GetScreenParams(%d): Failed!!\n",MixerHandle));
                        }
                        else
                        {
                            STTBX_Print(("\tRead != Expected\n\tWidth %d & %d\n\tHeight %d & %d\n",
                                         Horizontal, VMIXScreenParams.Width, Vertical, VMIXScreenParams.Height));
                        }
                    }
                }
            }
        }
    }
    for ( Vertical = MinVertical; Vertical <= MaxVertical; Vertical+=MaxVertical/10 )
    {
        for ( Horizontal = MinHorizontal; Horizontal <= MaxHorizontal; Horizontal++ )
        {
            VMIXScreenParams.Width  = Horizontal;
            VMIXScreenParams.Height = Vertical;
            ErrCode = STVMIX_SetScreenParams( MixerHandle, &VMIXScreenParams);
            if ( ErrCode != ST_NO_ERROR)
            {
                NbErr++;
                if(NbErr < 20)
                {
                    sprintf(MIX_Msg, "\tSTVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): Failed !!\n", \
                            MixerHandle,
                            (VMIXScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
                            (VMIXScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                            VMIXScreenParams.FrameRate,
                            VMIXScreenParams.Width, VMIXScreenParams.Height,
                            VMIXScreenParams.XStart, VMIXScreenParams.YStart);
                    STTBX_Print((MIX_Msg));
                }
            }
            else
            {
                ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
                if (( ErrCode != ST_NO_ERROR) ||
                    ( VMIXScreenParams.Width != (U32)Horizontal) ||
                    (VMIXScreenParams.Height != (U32)Vertical))
                {
                    NbErr++;
                    if(NbErr < 20)
                    {
                        if(ErrCode)
                        {
                            STTBX_Print(("\tSTVMIX_GetScreenParams(%d): Failed!!\n",MixerHandle));
                        }
                        else
                        {
                            STTBX_Print(("\tRead != Expected\n\tWidth %d & %d\n\tHeight %d & %d\n",
                                         Horizontal, VMIXScreenParams.Width, Vertical, VMIXScreenParams.Height));
                        }
                    }
                }
            }
        }
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : ok\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : failed ! %d times\n", NbErr ));
        RetErr = TRUE;
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VMIX_TestScreenParams() */



/*-------------------------------------------------------------------------
 * Function : VMIX_TestStartParams
 * Description : Screen parameter (X/Y Start) test : function test all cases
 *              (C code program because macro are not fast for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_TestStartParams(parse_t *pars_p, char *result_sym_p)
{
    STVMIX_Handle_t MixerHandle;
    STVMIX_ScreenParams_t VMIXScreenParams;
    U16 NbErr=0;
    U32 XStart, YStart, MaxXStart, MinXStart, MaxYStart, MinYStart;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    STTBX_Print(( "Call API STVMIX_Set/GetStartParams functions ...\n" ));
    NbErr = 0;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&MixerHandle );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MinXStart );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min XStart " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MaxXStart );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max XStart " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MinYStart );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min YStart " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MaxYStart );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max YStart " );
        return(TRUE);
    }

    ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
    if ( ErrCode != ST_NO_ERROR) NbErr++;

    for ( XStart = MinXStart; XStart <= MaxXStart; XStart+=(abs((int)((MaxXStart-MinXStart)/10))) )
    {
        for ( YStart = MinYStart; YStart <= MaxYStart; YStart++ )
        {
            VMIXScreenParams.XStart = XStart;
            VMIXScreenParams.YStart = YStart;
            ErrCode = STVMIX_SetScreenParams( MixerHandle, &VMIXScreenParams);
            if ( ErrCode != ST_NO_ERROR)
            {
                NbErr++;
                if(NbErr < 20)
                {
                    sprintf(MIX_Msg, "\tSTVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): Failed !!", \
                            MixerHandle,
                            (VMIXScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
                            (VMIXScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                            VMIXScreenParams.FrameRate,
                            VMIXScreenParams.Width, VMIXScreenParams.Height,
                            VMIXScreenParams.XStart, VMIXScreenParams.YStart);
                    STTBX_Print((MIX_Msg));
                }
            }
            else
            {
                ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
                if (( ErrCode != ST_NO_ERROR) ||
                    ( VMIXScreenParams.XStart != XStart) ||
                    (VMIXScreenParams.YStart != YStart))
                {
                    NbErr++;
                    if(NbErr < 20)
                    {
                        if(ErrCode)
                        {
                            STTBX_Print(("\tSTVMIX_GetScreenParams(%d): Failed!!\n",MixerHandle));
                        }
                        else
                        {
                            STTBX_Print(("\tRead != Expected\n\tXStart %d & %d\n\tYStart %d & %d\n",
                                         XStart, VMIXScreenParams.XStart, YStart, VMIXScreenParams.YStart));
                        }
                    }
                }
            }
        }
    }
    for ( YStart = MinYStart; YStart <= MaxYStart; YStart+=(abs((int)((MaxYStart-MinYStart)/10))) )
    {
        for ( XStart = MinXStart; XStart <= MaxXStart; XStart++ )
        {
            VMIXScreenParams.XStart = XStart;
            VMIXScreenParams.YStart = YStart;
            ErrCode = STVMIX_SetScreenParams( MixerHandle, &VMIXScreenParams);
            if ( ErrCode != ST_NO_ERROR)
            {
                NbErr++;
                if(NbErr < 20)
                {
                    sprintf(MIX_Msg, "\tSTVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): Failed !!", \
                            MixerHandle,
                            (VMIXScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
                            (VMIXScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                            (VMIXScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                            VMIXScreenParams.FrameRate,
                            VMIXScreenParams.Width, VMIXScreenParams.Height,
                            VMIXScreenParams.XStart, VMIXScreenParams.YStart);
                    STTBX_Print((MIX_Msg));
                }
            }
            else
            {
                ErrCode = STVMIX_GetScreenParams( MixerHandle, &VMIXScreenParams);
                if (( ErrCode != ST_NO_ERROR) ||
                    ( VMIXScreenParams.XStart != XStart) ||
                    (VMIXScreenParams.YStart != YStart))
                {
                    NbErr++;
                    if(NbErr < 20)
                    {
                        if(ErrCode)
                        {
                            STTBX_Print(("\tSTVMIX_GetScreenParams(%d): Failed!!\n",MixerHandle));
                        }
                        else
                        {
                            STTBX_Print(("\tRead != Expected\n\tXStart %d & %d\n\tYStart %d & %d\n",
                                         XStart, VMIXScreenParams.XStart, YStart, VMIXScreenParams.YStart));
                        }
                    }
                }
            }
        }
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : ok\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : failed ! %d times\n", NbErr ));
        RetErr = TRUE;
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VMIX_TestStartParams() */

/*-------------------------------------------------------------------------
 * Function : VMIX_TestOffsetParams
 * Description : Screen Offset parameter test : function test all cases
 *              (C code program because macro are not fast for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_TestOffsetParams(parse_t *pars_p, char *result_sym_p)
{
    STVMIX_Handle_t MixerHandle;
    U16 NbErr=0;
    S32 HOff, VOff, MinHOff, MaxHOff, MinVOff, MaxVOff;
    S8  HOff_p, VOff_p;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    STTBX_Print(( "Call API STVMIX_Set/GetOffsetParams functions ...\n" ));
    NbErr = 0;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&MixerHandle );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, &MinHOff );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min Horizontal Offset " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, &MaxHOff );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max Horizontal Offset " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, &MinVOff );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Min Vertical Offset " );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 0, &MaxVOff );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Max Vertical Offset " );
        return(TRUE);
    }

    for ( HOff = MinHOff; HOff <= MaxHOff; HOff++ )
    {
        for ( VOff = MinVOff; VOff <= MaxVOff; VOff++ )
        {
            ErrCode = STVMIX_SetScreenOffset( MixerHandle, (S8)HOff,  (S8)VOff);
            if ( ErrCode != ST_NO_ERROR ) NbErr++;
            ErrCode = STVMIX_GetScreenOffset( MixerHandle, &HOff_p, &VOff_p);
            if ( ErrCode != ST_NO_ERROR ) NbErr++;
            if ( HOff_p != HOff || VOff_p != VOff ) NbErr++;
        }
    }

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : ok\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VMIX_TestScreenParams() result : failed ! %d times\n", NbErr ));
        RetErr = TRUE;
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VMIX_TestOffsetParams() */


#if defined (ST_7015) || defined (ST_GX1)
/*-------------------------------------------------------------------------
 * Function : VMIX_ConnectLayersTest
 * Description : all connect layers capabilties tested
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_ConnectLayersTest(parse_t *pars_p, char *result_sym_p)
{
    STVMIX_Handle_t VMIXHndl1;
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrCode;
    STVMIX_LayerDisplayParams_t*  LayerArray[5];
    STVMIX_LayerDisplayParams_t   LayerParam[5];
    STVMIX_LayerDisplayParams_t   CheckConnectParams;
    int I, I1, I2, I3, I4, I5, J,NBcase;
    int itab2=0, itab3=0, itab4=0, itab5=0;
    char MIXMsg[500];

    STTBX_Print(( "Call API STVMIX_ConnectLayers() function ...\n" ));
    NbErr = 0;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&VMIXHndl1 );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    for ( I=1; I<=5; I++) {
      for ( I1=1; I1<=5; I1++) {
        if ( I > 1) {
          for ( I2=1; I2<=5; I2++) {
            if ( I1 != I2) {
              if ( I > 2) {
                for ( I3=1; I3<=5; I3++) {
                  if ( I1 != I3 && I2 != I3) {
                    if ( I > 3) {
                      for ( I4=1; I4<=5; I4++) {
                        if ( I1 != I4 && I2 != I4 && I3 != I4) {
                          if ( I > 4) {
                            for ( I5=1; I5<=5; I5++) {
                              if (  I1 != I5 && I2 != I5 && I3 != I5 && I4 != I5) {
                                strcpy(LayerParam[0].DeviceName, Tab_Connect[I1][0]);
                                LayerParam[0].ActiveSignal = FALSE;
                                LayerArray[0] = &LayerParam[0];
                                strcpy(LayerParam[1].DeviceName, Tab_Connect[I2][0]);
                                LayerParam[1].ActiveSignal = FALSE;
                                LayerArray[1] = &LayerParam[1];
                                strcpy(LayerParam[2].DeviceName, Tab_Connect[I3][0]);
                                LayerParam[2].ActiveSignal = FALSE;
                                LayerArray[2] = &LayerParam[2];
                                strcpy(LayerParam[3].DeviceName, Tab_Connect[I4][0]);
                                LayerParam[3].ActiveSignal = FALSE;
                                LayerArray[3] = &LayerParam[3];
                                strcpy(LayerParam[4].DeviceName, Tab_Connect[I5][0]);
                                LayerParam[4].ActiveSignal = FALSE;
                                LayerArray[4] = &LayerParam[4];
                                ErrCode = STVMIX_ConnectLayers(VMIXHndl1, (const STVMIX_LayerDisplayParams_t**)LayerArray, 5);
                                if ( ErrCode != tab5[itab5]) {
                                  NbErr++;
                                  sprintf( MIXMsg, "STVMIX_ConnectLayers(Hndl, ");
                                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[0]->DeviceName );
                                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[1]->DeviceName );
                                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[2]->DeviceName );
                                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[3]->DeviceName );
                                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[4]->DeviceName );
                                  sprintf ( MIXMsg, "%s) : %d expected %d\n", MIXMsg, ErrCode, tab5[itab5]);
                                  STTBX_Print((MIXMsg));
                                }
                                else
                                {
                                    if(ErrCode == ST_NO_ERROR)
                                    {
                                        /* Check connected layers only for this part */
                                        for(J=0;J<5;J++)
                                        {
                                            ErrCode = STVMIX_GetConnectedLayers(VMIXHndl1, J+1, &CheckConnectParams);
                                            if(ErrCode != ST_NO_ERROR)
                                            {
                                                NbErr++;
                                                sprintf( MIXMsg,
                                                         "STVMIX_GetConnectedLayers(Hndl, %d): returned %d !!\n", J, ErrCode);
                                                STTBX_Print((MIXMsg));
                                            }
                                            else
                                            {
                                                if((CheckConnectParams.ActiveSignal != FALSE) ||
                                                   (strcmp(CheckConnectParams.DeviceName, LayerArray[J]->DeviceName)))
                                                {
                                                    NbErr++;
                                                    sprintf( MIXMsg,
                                                             "STVMIX_GetConnectedLayers(Hndl, %d): ok\nLayerName = "
                                                             "\"%s\" ActiveSignal = %d  But expected layer %s\n", J,
                                                             CheckConnectParams.DeviceName,
                                                             CheckConnectParams.ActiveSignal,
                                                             LayerArray[J]->DeviceName);
                                                    STTBX_Print((MIXMsg));
                                                }
                                            }
                                        }
                                    }
                                }
                                NBcase++;
                                itab5++;
                              } /* end */
                            } /* end for I5 ... */
                          } /* end if ( I > 4) */
                          else
                          {
                            strcpy(LayerParam[0].DeviceName, Tab_Connect[I1][0]);
                            LayerParam[0].ActiveSignal = FALSE;
                            LayerArray[0] = &LayerParam[0];
                            strcpy(LayerParam[1].DeviceName, Tab_Connect[I2][0]);
                            LayerParam[1].ActiveSignal = FALSE;
                            LayerArray[1] = &LayerParam[1];
                            strcpy(LayerParam[2].DeviceName, Tab_Connect[I3][0]);
                            LayerParam[2].ActiveSignal = FALSE;
                            LayerArray[2] = &LayerParam[2];
                            strcpy(LayerParam[3].DeviceName, Tab_Connect[I4][0]);
                            LayerParam[3].ActiveSignal = FALSE;
                            LayerArray[3] = &LayerParam[3];
                            ErrCode = STVMIX_ConnectLayers(VMIXHndl1, (const STVMIX_LayerDisplayParams_t**)LayerArray, 4);
                            if ( ErrCode != tab4[itab4] ) {
                              NbErr++;
                              sprintf( MIXMsg, "STVMIX_ConnectLayers(Hndl, ");
                              sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[0]->DeviceName );
                              sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[1]->DeviceName );
                              sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[2]->DeviceName );
                              sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[3]->DeviceName );
                              sprintf ( MIXMsg, "%s) : %d expected %d\n", MIXMsg, ErrCode, tab4[itab4]);
                              STTBX_Print((MIXMsg));
                            }
                            NBcase++;
                            itab4++;
                          }
                        }  /* end if ( I1 != I4 ... */
                      } /* end for I4 ... */
                    } /* end if ( I > 3) */
                    else
                    {
                      strcpy(LayerParam[0].DeviceName, Tab_Connect[I1][0]);
                      LayerParam[0].ActiveSignal = FALSE;
                      LayerArray[0] = &LayerParam[0];
                      strcpy(LayerParam[1].DeviceName, Tab_Connect[I2][0]);
                      LayerParam[1].ActiveSignal = FALSE;
                      LayerArray[1] = &LayerParam[1];
                      strcpy(LayerParam[2].DeviceName, Tab_Connect[I3][0]);
                      LayerParam[2].ActiveSignal = FALSE;
                      LayerArray[2] = &LayerParam[2];
                      ErrCode = STVMIX_ConnectLayers(VMIXHndl1, (const STVMIX_LayerDisplayParams_t**)LayerArray, 3);
                      if ( ErrCode != tab3[itab3] ) {
                        NbErr++;
                        sprintf( MIXMsg, "STVMIX_ConnectLayers(Hndl, ");
                        sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[0]->DeviceName );
                        sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[1]->DeviceName );
                        sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[2]->DeviceName );
                        sprintf ( MIXMsg, "%s) : %d expected %d\n", MIXMsg, ErrCode, tab3[itab3]);
                        STTBX_Print((MIXMsg));
                      }
                      NBcase++;
                      itab3++;
                    }
                  } /* end if ( I1 != I3 ... */
                } /* end for I3 ... */
              } /* end if ( I > 2) */
              else
              {
                strcpy(LayerParam[0].DeviceName, Tab_Connect[I1][0]);
                LayerParam[0].ActiveSignal = FALSE;
                LayerArray[0] = &LayerParam[0];
                strcpy(LayerParam[1].DeviceName, Tab_Connect[I2][0]);
                LayerParam[1].ActiveSignal = FALSE;
                LayerArray[1] = &LayerParam[1];
                ErrCode = STVMIX_ConnectLayers(VMIXHndl1, (const STVMIX_LayerDisplayParams_t**)LayerArray, 2);
                if ( ErrCode != tab2[itab2] ) {
                  NbErr++;
                  sprintf( MIXMsg, "STVMIX_ConnectLayers(Hndl, ");
                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[0]->DeviceName );
                  sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[1]->DeviceName );
                  sprintf ( MIXMsg, "%s) : %d expected %d\n", MIXMsg, ErrCode, tab2[itab2]);
                  STTBX_Print((MIXMsg));
                }
                NBcase++;
                itab2++;
              }
            } /* end if ( I1 != I2) ... */
          } /* end for I2 ... */
        } /* end if ( I > 1) ... */
        else
        {
          sprintf( MIXMsg, "STVMIX_ConnectLayers(Hndl, ");
          strcpy(LayerParam[0].DeviceName, Tab_Connect[I1][0]);
          LayerParam[0].ActiveSignal = FALSE;
          LayerArray[0] = &LayerParam[0];

          ErrCode = STVMIX_ConnectLayers(VMIXHndl1, (const STVMIX_LayerDisplayParams_t**)LayerArray, 1);
          if ( ErrCode != tab1[I1 - 1] ) {
            NbErr++;
            sprintf( MIXMsg, "%s \"%s\"", MIXMsg, LayerArray[0]->DeviceName );
            sprintf ( MIXMsg, "%s) : %d expected %d\n", MIXMsg, ErrCode, tab1[I1 - 1]);
            STTBX_Print((MIXMsg));
          }
          NBcase++;
        }
      } /* end for I1 ... */
    } /* end for I ... */

    /* test result */
    if ( NbErr == 0 )
    {
        STTBX_Print(( "### VMIX_ConnectLayersTest() result : ok, ST_ERROR_FEATURE_NOT_SUPPORTED returned each time as expected\n" ));
        RetErr = FALSE;
    }
    else
    {
        STTBX_Print(( "### VMIX_ConnectLayersTest() result : failed ! ST_ERROR_FEATURE_NOT_SUPPORTED not returned %d times\n", NbErr ));
        RetErr = TRUE;
    }

    STTST_AssignInteger( result_sym_p, NbErr, FALSE);

    return(API_EnableError ? RetErr : FALSE );

} /* end of VMIX_ConnectLayersTest() */
#endif

#if defined (ST_7020) || defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_7710)|| defined (ST_7100)\
   || defined (ST_5301)|| defined (ST_7109) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107)|| defined (ST_7200)\
   || defined (ST_5162)
/*-------------------------------------------------------------------------
 * Function : VMIX_SubVmixCheckConnect
 * Descript : Sub function for connect layer capabilities
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static U8 VMIX_SubVmixCheckConnect(U8* LayersFixed)
{
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107)\
 || defined (ST_5162)
   BOOL DetectGFX1 = FALSE, DetectVID1=FALSE, DetectGFX2=FALSE;
#else
    BOOL DetectCUR = FALSE;
    #if !defined (ST_7710) && !defined (ST_7100) && !defined (ST_7109) && !defined (ST_7200)
        BOOL DetectGDP3=FALSE;
    #endif
#if defined (ST_7020)
    BOOL DetectVIDOrGDP1 = FALSE, DetectGDP2=FALSE;
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)
 BOOL DetectVID1OrGDP = FALSE;
#elif defined (ST_5528)
    BOOL DetectVIDOrGDP2 = FALSE, DetectGDP4 = FALSE, DetectGDP1=FALSE;
#endif
#endif /* defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162) */
    U8 Count = 0;

    while(1)
    {
        switch(LayersFixed[Count])
        {
            case 0:
                /* No error */
                return(0);
                break;

            case 1:
#if defined (ST_7020)
                if(DetectVIDOrGDP1 || DetectGDP2 || DetectCUR)
                    return(Count);
                DetectGDP3++;
#elif defined (ST_5528)
                if(DetectVIDOrGDP2 || DetectGDP3 || DetectGDP4 || DetectCUR)
                    return(Count);
                DetectGDP1++;
#elif defined (ST_5100) || defined(ST_5105) || defined (ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined (ST_5107)\
   || defined (ST_5162)
                DetectGFX1++;

#elif defined (ST_7710) || defined(ST_7100) || defined(ST_7109)|| defined(ST_7200)
                if(DetectVID1OrGDP || DetectCUR)
                    return(Count);
#endif

                break;

            case 2:
#if defined (ST_5100) || defined(ST_5105) || defined (ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined (ST_5107)\
 || defined (ST_5162)
                DetectVID1++;
                break;
#endif

            case 3:
#if defined (ST_5100) || defined(ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5107) || defined (ST_5162)
                DetectGFX2++;
                break;
#endif


            case 4:
#if defined (ST_7020)
                if(DetectCUR || DetectGDP2)
                    return(Count);
                DetectVIDOrGDP1++;

#elif defined (ST_5528)
                if(DetectCUR || DetectGDP3 || DetectGDP4)
                    return(Count);
                DetectVIDOrGDP2++;
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)
                if(DetectCUR)
                    return(Count);
#endif
                break;

            case 5:
#if defined (ST_7020)
                if(DetectCUR)
                    return(Count);
                DetectGDP2++;

#elif defined (ST_5528)
                if(DetectCUR || DetectGDP4)
                    return(Count);
                DetectGDP3++;
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
                DetectCUR++;
#endif
                break;

            case 6:
#if defined (ST_7020)
                if(DetectCUR)
                    return(Count);

#elif defined (ST_5528)
                if(DetectCUR)
                    return(Count);
                DetectGDP4++;
#endif
                break;

            case 7:
#if defined (ST_7020)
                DetectCUR++;
#elif defined (ST_5528)
                if(DetectCUR)
                    return(Count);
#endif
                break;
#if defined (ST_5528)
            case 8:
                DetectCUR++;
                break;
#endif

            default:
                return(Count);
        }
        Count++;
    }
}

/*-------------------------------------------------------------------------
 * Function : VMIX_SubConnect
 * Descript : Sub function for vmix connection
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VMIX_SubConnect(STVMIX_LayerDisplayParams_t* LayerParameter, U8* LayersFixed)
{
    U8 j=0, PbPos;
    STVMIX_LayerDisplayParams_t* LayerArray[MAX_LAYER];
    ST_ErrorCode_t ErrCode;

    PbPos = VMIX_SubVmixCheckConnect(LayersFixed);

    while(LayersFixed[j]!=0)
    {
        LayerArray[j] = &LayerParameter[LayersFixed[j]-1];
        j++;
    }
    ErrCode = STVMIX_ConnectLayers(VMIXHndlConnect, (const STVMIX_LayerDisplayParams_t**)LayerArray, j);
    if ((((PbPos != 0) && (ErrCode == ST_NO_ERROR)) ||
         ((PbPos == 0) && (ErrCode != ST_NO_ERROR)))
        && (API_ErrorCount < 25))
    {
        j=0;
        STTBX_Print(("Failed with layer "));
        while(LayersFixed[j]!=0)
        {
            STTBX_Print(("%s ", LayerArray[j]->DeviceName));
            j++;
        }
        STTBX_Print ((". Got error 0x%x and expected %d ", ErrCode, (PbPos == 0) ? 0 : 8));
        if (PbPos != 0)
        {
            STTBX_Print(("(may be due to layer %d", PbPos));
        }
        STTBX_Print(("\n"));
        API_ErrorCount++;
    }
}

/*-------------------------------------------------------------------------
 * Function : VMIX_SubTreeConnect
 * Descript : Exploring all possible connection of layers
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void VMIX_SubTreeConnect(STVMIX_LayerDisplayParams_t* LayerParameter,
                                U8* LayersFixed, U8* LayersFree)
{
    U8 i=0,k=0,l=0;
    U8 SubFixedTable[MAX_LAYER+1];
    U8 SubFreeTable[MAX_LAYER+1];

    while(LayersFixed[k] != 0)
    {
        SubFixedTable[k] = LayersFixed[k];
        k++;
    }
    SubFixedTable[k] = 0;

    while(LayersFree[l] != 0)
        l++;

    while(LayersFree[i] != 0)
    {
        SubFixedTable[k] = LayersFree[i];
        SubFixedTable[k+1] = 0;
        memcpy(SubFreeTable, LayersFree, sizeof(U8)*i);
        memcpy(&SubFreeTable[i], &LayersFree[i+1], sizeof(U8)*(l-i));
/*printf("%3d - i=%3d k=%3d l=%3d\n", NestedLevel, i, k, l);*/
        VMIX_SubTreeConnect(LayerParameter, SubFixedTable, SubFreeTable);
        i++;
    }
    VMIX_SubConnect(LayerParameter, LayersFixed);
}


/*-------------------------------------------------------------------------
 * Function : VMIX_ConnectLayersTest
 * Description : all connect layers capabilties tested
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_ConnectLayersTest(parse_t *pars_p, char *result_sym_p)
{
#if defined (ST_7020)
    U8 EmptyTable[]    = { 0, 0, 0, 0, 0, 0, 0, 0};
    U8 CompleteTable[] = { 1, 2, 3, 4, 5, 6, 7, 0};
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)
    U8 EmptyTable[]    = { 0, 0, 0, 0, 0, 0};
    U8 CompleteTable[] = { 1, 2, 3, 4, 5, 0};
#elif defined (ST_7200)
    U8 EmptyTable[]    = { 0, 0, 0, 0, 0, 0, 0, 0};
    U8 CompleteTable[] = { 1, 2, 3, 4, 5, 6, 7, 0};
#elif defined (ST_5528)
    U8 EmptyTable[]    = { 0, 0, 0, 0, 0, 0, 0, 0, 0};
    U8 CompleteTable[] = { 1, 2, 3, 4, 5, 6, 7, 8, 0};
#elif defined (ST_5100) || defined(ST_5105) || defined (ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined (ST_5107)\
   || defined (ST_5162)
    U8 EmptyTable[]    = { 0, 0, 0, 0};
    U8 CompleteTable[] = { 1, 2, 3, 0};
#endif
    U8 i;
    BOOL RetErr;
    STVMIX_LayerDisplayParams_t LayerParameter[MAX_LAYER];
    U32 TickPerSecond, ClockElapsed, TimeElapsed;
    osclock_t ClockBegin, ClockEnd;

    STTBX_Print(( "Call API STVMIX_ConnectLayers() function ...\n" ));
#if defined (ST_5528)
    STTBX_Print(( " ! This test may take a rather long time (like 3 minutes)...\n" ));
#endif
    TickPerSecond = ST_GetClocksPerSecond();
    ClockBegin = time_now();
    API_ErrorCount = 0;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&VMIXHndlConnect );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    for(i=0;i<MAX_LAYER;i++)
    {
        LayerParameter[i].ActiveSignal = FALSE;
        strcpy(LayerParameter[i].DeviceName, &Tab_Connect[i][0]);
    }
    VMIX_SubTreeConnect(LayerParameter, EmptyTable, CompleteTable);

    ClockEnd = time_now();
    ClockElapsed = (U32)((ClockEnd >= ClockBegin) ? (ClockEnd - ClockBegin) : (0xFFFF - ClockEnd + ClockBegin));
    TimeElapsed = (U32)((long)ClockElapsed/(long)TickPerSecond);
    STTBX_Print(( "This test took %d s...\n", TimeElapsed ));

    STTST_AssignInteger( result_sym_p, API_ErrorCount, FALSE);

    return(API_EnableError ? RetErr : FALSE );
}  /* end of VMIX_ConnectLayersTest() */
#endif /* ST_7020  || ST_5528 || ST_5100 || ST_5105 || ST_5188 || defined (ST_5107) || defined (ST_5162) */


#ifdef ARCHITECTURE_ST20
/*******************************************************************************
Name        : VMIX_StackUsage
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static BOOL VMIX_StackUsage(parse_t *pars_p, char *result_sym_p)
{
    #define STACK_SIZE          4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    char *funcname[]= {
        "test_overhead",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;
    TaskParams_t TaskParams_p;

/*
#if defined (ST_5510) || defined (ST_5512)
    layinit "VID1" 7
    layinit "LOSD" 8
    layinit "CURS" 6
    layinit "STIL" 9
#elif defined (ST_5508) || defined (ST_5518)
    layinit "VID1" 7
    layinit "LOSD" 8
    layinit "CURS" 6
#elif defined (ST_7015)
    layinit "GFX1" 2 hA000 720 480
    layinit "GFX2" 2 ha500 720 480
    layinit "CURS" 0 hA200 720 480 ,, 1
    layinit "VID1" 4 720 480 0
    layinit "VID2" 5 720 480 0
#endif
*/
    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        task_p = task_create(func, (void *)&TaskParams_p, STACK_SIZE, MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);

        /* Wait for task to complete */
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, 1);
        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", funcname[i], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", funcname[i], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        task_delete(task_p);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");

/*
#if defined (ST_5510) || defined (ST_5512)
        layterm "VID1"
        layterm "LOSD"
        layterm "CURS"
        layterm "STIL"
#elif defined (ST_5508) || defined (ST_5518)
        layterm "VID1"
        layterm "LOSD"
        layterm "CURS"
#elif defined (ST_7015)
        layterm "GFX1"
        layterm "GFX2"
        layterm "CURS"
        layterm "VID1"
        layterm "VID2"
#endif
*/
    return(FALSE );
}
#endif /* ARCHITECTURE_ST20 */

#ifdef TRACE_DYNAMIC_DATASIZE
/*******************************************************************************
Name        : VMIX_GetDynamicDataSize
Description : Get DynamicDatasize allocated for the driver
Parameters  : None
Returns     : None
*******************************************************************************/
static BOOL VMIX_GetDynamicDataSize(STTST_Parse_t *pars_p, char *result_sym_p)
{

    STVMIX_InitParams_t InitParam;
    STVMIX_TermParams_t TermParams;
#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    void*                       Cache_p;
    STAVMEM_AllocBlockParams_t  AllocParams;
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#endif /* defined(ST_5105) || defined(ST_5188) || defined (ST_5107) */
    InitParam.CPUPartition_p  = SystemPartition_p;
    #if defined (ST_5100) || defined (ST_5301)|| defined (ST_5525) || defined (ST_5162)
    InitParam.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    InitParam.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType             = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion  = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p               = (void*)MIXER_TILE_RAM_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.Size1                 = MIXER_TILE_RAM_SIZE;
    InitParam.CacheBitmap_p                   = &VMIXTEST_CacheBitmap;
    InitParam.AVMEM_Partition                 = AvmemPartitionHandle[0];
#if defined (ST_5100)
#if defined (USE_AVMEMCACHED_PARTITION)
    InitParam.AVMEM_Partition2                = AvmemPartitionHandle[1];
#else
    InitParam.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif
#else  /* defined (ST_5100)*/
    InitParam.AVMEM_Partition2                = AvmemPartitionHandle[0];
#endif

    InitParam.RegisterInfo.GdmaBaseAddress_p  = (void*)(U32)MIXER_GDMA_BASE_ADDRESS;
#elif defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Allocate a block to be used as a cache in the composition operations */
    AllocParams.PartitionHandle          = AvmemPartitionHandle[0];
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.Size                     = MIXER_TILE_RAM_SIZE;
    AllocParams.Alignment                = 1024;

    ErrCode = STAVMEM_AllocBlock (&AllocParams, &CacheBlockHandle);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't allocate cache in shared memory : %s\n",ErrCode));
        return ( API_EnableError ? TRUE : FALSE );
    }
    ErrCode = STAVMEM_GetBlockAddress (CacheBlockHandle, (void **)&(Cache_p));
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print (("Error : Can't get the block address for the allocated cache : %s\n",ErrCode));
        /* Free the allocated block before leaving */
        FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
        ErrCode = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print (("Error : Can't deallocate the cache block : %s\n",ErrCode));
        }
        return ( API_EnableError ? TRUE : FALSE );
    }
    VMIXInitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)MIXER_GDMA_BASE_ADDRESS;
    VMIXInitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType                 = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion      = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p                   = (void*)Cache_p;
    VMIXTEST_CacheBitmap.Size1                     = MIXER_TILE_RAM_SIZE;
    VMIXInitParams.CacheBitmap_p = &VMIXTEST_CacheBitmap;
    VMIXInitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
    VMIXInitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
#else
    InitParam.DeviceBaseAddress_p = (void *)MIXER_BASE_ADDRESS;
    InitParam.BaseAddress_p = (void *)MIXER_BASE_OFFSET;
#endif
    InitParam.DeviceType = MIXER_DEVICE_TYPE;
    InitParam.MaxOpen = 1;
    InitParam.MaxLayer = 1;
    strcpy( InitParam.VTGName, "");
    strcpy( InitParam.EvtHandlerName, STEVT_DEVICE_NAME);
    InitParam.OutputArray_p = NULL;


    printf("*******************************************\n");
    printf("* Dynamic data size calculation beginning *\n");
    printf("*******************************************\n");

    /*** Get Size for 1 Layers***/

    DynamicDataSize=0;
    STVMIX_Init( STVMIX_DEVICE_NAME, &InitParam);
    printf("** Size of Dynamic Data for 1Layer= %d bytes **\n", DynamicDataSize);
    TermParams.ForceTerminate = TRUE;
    STVMIX_Term(STVMIX_DEVICE_NAME, &TermParams);

    /*** Get Size for 10 Layers***/
    InitParam.MaxLayer = 10;
    DynamicDataSize=0;
    STVMIX_Init( STVMIX_DEVICE_NAME, &InitParam);
    printf("** Size of Dynamic Data for 10Layer= %d bytes **\n", DynamicDataSize);
    TermParams.ForceTerminate = TRUE;
    STVMIX_Term(STVMIX_DEVICE_NAME, &TermParams);

#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107)
        /* Free the allocated block before leaving */
        FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
        ErrCode = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print (("Error : Can't deallocate the cache block : %s\n",ErrCode));
        }
#endif

    printf("*******************************************\n");
    printf("*   Dynamic data size calculation end     *\n");
    printf("*******************************************\n");


    return(FALSE );
}
#endif
/*-------------------------------------------------------------------------
 * Function : VMIX_InitCommand2
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
static BOOL VMIX_InitCommand2(void)
{
  BOOL RetErr;

/* API functions : */
    RetErr  = FALSE;
/* utilities : */
    RetErr |= STTST_RegisterCommand( "VMIX_Nullpt"    ,VMIX_NullPointersTest,"Call API functions to test null pointers ");
    RetErr |= STTST_RegisterCommand( "VMIX_TScreenP"  ,VMIX_TestScreenParams,"Call API functions to test screen (Width/Height) parameters ");
    RetErr |= STTST_RegisterCommand( "VMIX_TStartP"   ,VMIX_TestStartParams,"Call API functions to test screen (X/Y Start) parameters ");
    RetErr |= STTST_RegisterCommand( "VMIX_TOffsetP"  ,VMIX_TestOffsetParams,"Call API functions to test Offset parameters ");
#if defined (ST_7015) || defined(ST_7020) || defined (ST_GX1) || defined (ST_5528) || defined (ST_5100) || defined(ST_5105)\
    || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined(ST_5188) || defined (ST_5107)|| defined (ST_7200)\
    || defined (ST_5162)
    RetErr |= STTST_RegisterCommand( "VMIX_TConnect"  ,VMIX_ConnectLayersTest,"Call API functions to test all connect capabilities ");
#endif
#ifdef STI7710_CUT2x
RetErr |= STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
#elif defined (STI7109_CUT2)
RetErr |= STTST_AssignString ("CHIP_CUT", "STI7109_CUT2", TRUE);
#else
RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif

#ifdef ARCHITECTURE_ST20
    RetErr |= STTST_RegisterCommand( "VMIX_StackUsage",VMIX_StackUsage,"Call stack usage task");
#endif /* ARCHITECTURE_ST20 */
#ifdef TRACE_DYNAMIC_DATASIZE
RetErr |= STTST_RegisterCommand( "VMIX_GetDynamicDataSize",VMIX_GetDynamicDataSize,"Calculate Dynamic Data Size allocated for the driver");
#endif
    return(!RetErr);

} /* end VMIX_InitCommand2 */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart()
{
    BOOL RetOk=FALSE;

    RetOk |= VMIX_InitCommand2();

    if ( RetOk )
    {
        sprintf( MIX_Msg, "VMIX_TestCommand() \t: ok\n");
    }
    else
    {
        sprintf( MIX_Msg, "VMIX_TestCommand() \t: failed !\n" );
    }
    STTST_Print(( MIX_Msg ));

    return(RetOk);
} /* end of Test_CmdStart() */

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    UNUSED_PARAMETER(ptr);
    STAPIGAT_Init();
    STAPIGAT_Term();

} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

UNUSED_PARAMETER(argc);
UNUSED_PARAMETER(argv);
#ifdef ST_OS21
#ifdef ST_OSWINCE
    SetFopenBasePath("/dvdgr-prj-stvmix/tests/src/objs/ST40");
#endif
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
    os20_main(NULL);
#else
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}
/* end of vmix_test.c */
