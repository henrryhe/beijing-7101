/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

File name   : lay_cmd.c
Description : Definition of layer commands

Note        :

Date          Modification
----          ------------

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef ST_OS20
#include <debug.h>
#endif
#include "stddefs.h"
#include "stdevice.h"

#ifdef ST_OSLINUX
#include <sys/ioctl.h>
#endif

#include "sttbx.h"
#include "stos.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"
#include "clevt.h"
#include "stlayer.h"
#include "stsys.h"
#include "lay_cmd.h"
#include "lay_data.h"
#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include "os21debug.h"
#endif /* ST_OS20 */
#ifndef ST_OSLINUX
#include "clavmem.h"
#endif
#if defined ST_7015 || defined(ST_7020)
#include "clintmr.h"
#endif /* 7015 and 7020 */

#if defined(ST_OSLINUX)
#include "iocstapigat.h"
#endif

#define STLAYER_SECURE_ON    0x40000000


/* Private constants -------------------------------------------------------- */

#define MAX_LAYER_TYPE              23 /* 22 +  (GDP_VBI type)*/
#define MAX_ASPECTRATIO_TYPE        4
#define MAX_SCANTYPE_TYPE           2
#define MAX_ALLOCATED_BLOCK         30
#define RETURN_PARAMS_LENGTH        50

#if defined (ST_7020)
#define LAYER_DEVICE_BASE_ADDRESS   STI7020_BASE_ADDRESS
#define LAYER_BASE_ADDRESS          (ST7020_GAMMA_OFFSET+ST7020_GAMMA_GDP1_OFFSET)
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2
#if defined (ST_5528)
    #define LAYER_5528_DEVICE_BASE_ADDRESS   0
    #define LAYER_5528_BASE_ADDRESS          ST5528_GDP1_LAYER_BASE_ADDRESS
#endif

#elif defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || \
      defined (ST_5518) || defined (ST_5514) || defined (ST_5516) || \
      defined (ST_5517)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          VIDEO_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_VID_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "OMEGA1_VIDEO"
#define LAYER_MAX_VIEWPORT          1
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2

#elif defined (ST_7015)
#define LAYER_DEVICE_BASE_ADDRESS   STI7015_BASE_ADDRESS
#define LAYER_BASE_ADDRESS     (ST7015_GAMMA_OFFSET+ST7015_GAMMA_GFX1_OFFSET)
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_BKL"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         1920
#define LAYER_DEFAULT_HEIGHT        1080
#define LAYER_MAX_OPEN_HANDLES      2

#elif defined (ST_GX1)
#define LAYER_DEVICE_BASE_ADDRESS   0xbb410000
#ifdef LAYER_BASE_ADDRESS
#undef LAYER_BASE_ADDRESS
#endif
#define LAYER_BASE_ADDRESS          0x100
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2

#elif defined (ST_5100) || defined (ST_5105) ||  defined (ST_5301) || defined (ST_5188) || defined (ST_5525) || defined (ST_5107)\
   || defined(ST_5162)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          0
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "COMPOSITOR"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      4

#elif defined (ST_5528)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          ST5528_GDP1_LAYER_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2

#elif defined (ST_7710)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          ST7710_GDP1_LAYER_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2
#elif defined (ST_7100)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          ST7100_GDP1_LAYER_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2
#elif defined (ST_7109)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          ST7109_GDP1_LAYER_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2
#elif defined (ST_7200)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          ST7200_GDP1_LAYER_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#define LAYER_MAX_OPEN_HANDLES      2


#else
#error Not defined processor
#endif

#define GAMMA_FILE_RGB565           0x80
#define GAMMA_FILE_RGB888           0x81
#define GAMMA_FILE_ARGB8565         0x84
#define GAMMA_FILE_ARGB8888         0x85
#define GAMMA_FILE_ARGB1555         0x86
#define GAMMA_FILE_ARGB4444         0x87
#define GAMMA_FILE_CLUT2            0x89
#define GAMMA_FILE_CLUT4            0x8A
#define GAMMA_FILE_CLUT8            0x8B
#define GAMMA_FILE_ACLUT44          0x8C
#define GAMMA_FILE_ACLUT88          0x8D
#define GAMMA_FILE_ACLUT8           0x8E
#define GAMMA_FILE_YCBCR888         0x90
#define GAMMA_FILE_YCBCR422R        0x92
#define GAMMA_FILE_YCBCR420MB       0x94
#define GAMMA_FILE_AYCBCR8888       0x96
#define GAMMA_FILE_ALPHA1           0x98

#ifdef ST_OSLINUX                         /* for test */
#undef SDRAM_BASE_ADDRESS
#define SDRAM_BASE_ADDRESS 0xa4305000
#else
#ifndef SDRAM_BASE_ADDRESS
#define SDRAM_BASE_ADDRESS 0
#endif

#endif

#ifdef ST_OSLINUX
extern ST_ErrorCode_t STLAYER_AllocData( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p );
extern ST_ErrorCode_t STLAYER_FreeData( STLAYER_Handle_t  LayerHandle, void *Address_p );
#endif

#if defined (ST_7109) && !defined(ST_OSLINUX)
/* For Video Display on 7109*/
static char FileNameVid1[80];
static void * LoadedVidAdd1_p;
#define DISP_VID_OMEGA1_TYPE_MB_420 0x00
#define DISP_VID_OMEGA2_TYPE_MB_420 0x94
#define DISP_VID_5528_TYPE_MB_420   0x01
#define DISP_VID_SIGNATURE          0x420F
#define DISP_VIDEO_FILENAME "tektro1080i.gam"
#endif

/* 55xx Variables ----------------------------------------------------------- */
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508)||defined(ST_5518)\
 || defined(ST_5516)|| ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

static STGXOBJ_Bitmap_t                BitmapCursor, BitmapOSD, BitmapStill;
static STGXOBJ_BitmapAllocParams_t     BitmapOSDParams1,
                                       BitmapOSDParams2;
static STGXOBJ_Palette_t               PaletteCursor, PaletteOSD;
static STGXOBJ_PaletteAllocParams_t    PaletteOSDParams ;
#endif

static STLAYER_FlickerFilterMode_t      FFMode;
static STGXOBJ_ColorARGB_t              ColorFill;

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109)|| defined(ST_5100) || defined(ST_5105) ||  defined(ST_5301) || defined(ST_5188) || defined(ST_5525)\
 || defined(ST_5107)|| defined(ST_7200) || defined(ST_5162)

static STGXOBJ_Bitmap_t    Bitmapmerou;
static STGXOBJ_Bitmap_t    Bitmapeye;
static STGXOBJ_Bitmap_t    BitmapFormat;
static STGXOBJ_Bitmap_t    Bitmapface;
static STGXOBJ_Palette_t   FacePalette;
STLAYER_ViewPortHandle_t   VPHandle;
#endif

static STGXOBJ_BitmapAllocParams_t      BitmapLCParams1,
                                        BitmapLCParams2;
static STGXOBJ_PaletteAllocParams_t     PaletteLCParams ;

#if defined(ST_5516) || (defined(ST_5517) && !defined(ST_7020))
static STGXOBJ_Bitmap_t    Bitmapmerou;
#endif /* ST_7015 ST_7020 ST_GX1 */

#ifdef ST_OSLINUX
static STGXOBJ_Bitmap_t    BitmapTest;
#endif

STGXOBJ_Bitmap_t           BitmapUnknown; /* not static, may be exported */

static STGXOBJ_Bitmap_t  TempBitmap;      /*  Needed By STLAYER_GetViewPortSource() */
static STGXOBJ_Palette_t TempPalette;

#if defined (ST_7109) && !defined(ST_OSLINUX)

static void VIDEO_Disp(void);


#endif

ST_ErrorCode_t GUTIL_AllocBuffer(U32 Size, U32 Alignment, STAVMEM_BlockHandle_t* BlockHandle_p, void **BufferAddress_p);


/* All chips Variable ------------------------------------------------------- */
#ifndef ST_OSLINUX
STAVMEM_SharedMemoryVirtualMapping_t VM;

#endif


/* --- Types -------------------------------------------------------------- */

typedef struct
{
    const char String[16];
    const U32 Value;
} LAYER_StringToEnum_t;


typedef struct {
    STLAYER_ViewPortHandle_t VPHndl;
#ifdef ST_OSLINUX
    void    *BitmapAddress_p;
    void    *PaletteAddress_p;
#else
    STAVMEM_BlockHandle_t BitmapHndl;
    STAVMEM_BlockHandle_t PaletteHndl;
#endif
} AllocatedBlockHandle_t;

typedef struct
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
} GUTIL_GammaHeader;



/* Private Function prototypes ---------------------------------------------- */

#if defined (ST_7109) && !defined(ST_OSLINUX)
/*   Load A video plane source and update display params */
static void VIDEO_Disp()
{

    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_BlockHandle_t BlockHandle;


    void * AddDevice_p;
    FILE * fstream_p;

    U32 size;
    GUTIL_GammaHeader Header;
    BOOL Exit = FALSE;

    /*sprintf(FileNameVid1, "%s%s", StapigatDataPath, DISP_VIDEO_FILENAME);*/

    printf("Looking for file %s ... ", FileNameVid1);
    fstream_p = fopen(FileNameVid1, "rb");
    if( fstream_p == NULL )
    {
        printf("Not unable to open !!!\n");
        Exit = TRUE;
    }

    if(!Exit)
    {
        printf("ok\n");
        printf("Reading header : ");
        size = fread (&Header, 1, sizeof(GUTIL_GammaHeader), fstream_p);
        if((size != sizeof(GUTIL_GammaHeader)) ||
           (Header.Header_size != 0x6) ||
           (Header.Signature != DISP_VID_SIGNATURE))
        {
            printf("Wrong header !!!\n");
            Exit = TRUE;
        }
        else
        {
            switch(Header.Type)
            {
                case DISP_VID_OMEGA1_TYPE_MB_420:
                    printf("Type OMEGA1 MB 4:2:0 width=%d height=%d\n",
                                 Header.width, Header.height );
                    break;

                case DISP_VID_OMEGA2_TYPE_MB_420:
                    printf("Type OMEGA2 MB 4:2:0 width=%d height=%d\n",
                                 Header.width, Header.height );
                   break;

                case DISP_VID_5528_TYPE_MB_420:
                    printf("Type 5528 MB 4:2:0 width=%d height=%d\n",
                                 Header.width, Header.height );
                   break;

                default:
                    printf("Unknown type !!!\n");
                    Exit = TRUE;
                    break;
            }
        }
    }

    if(!Exit)
    {


        AllocBlockParams.PartitionHandle          = AvmemPartitionHandle[0];
        AllocBlockParams.Size                     = Header.nb_pixel + Header.zero;
        AllocBlockParams.Alignment                = 2048;
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        if(STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle)) != ST_NO_ERROR)
        {
            printf("Cannot allocate in avmem !! \n");
            Exit = TRUE;
        }


    }

    if(!Exit)
    {
        STAVMEM_GetBlockAddress( BlockHandle, &LoadedVidAdd1_p);
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

        printf("Reading data ...");
        size = fread (STAVMEM_VirtualToCPU(LoadedVidAdd1_p,&VirtualMapping), 1, AllocBlockParams.Size, fstream_p);
        if (size != AllocBlockParams.Size)
        {
            printf("Read error %d byte instead of %d !!!\n", size, AllocBlockParams.Size);
            Exit = TRUE;
        }
        else
        {
            printf("ok\n");
       }

    }

    if( fstream_p != NULL )
    {
        fclose (fstream_p);
    }

    if(!Exit)
    {

        AddDevice_p = (void*) STAVMEM_VirtualToDevice(LoadedVidAdd1_p, &VirtualMapping);

        printf("%s loaded at avmem dev add 0x%x \n", FileNameVid1, (U32)AddDevice_p);

        STSYS_WriteRegMem32LE((U32 *) 0xb9002018, (U32)((U32) AddDevice_p +8));
        STSYS_WriteRegMem32LE((U32 *) 0xb9002024, (U32)((U32) AddDevice_p + Header.nb_pixel+8));

    }


}

#endif



#define LAYER_PRINT(x)                                          \
{                                                               \
    /* Check lenght */                                          \
    if(strlen(x)>sizeof(LAY_Msg))                               \
    {                                                           \
        sprintf(x, "Message too long (%d)!!\n", strlen(x));     \
        STTBX_Print((x));                                       \
        return(TRUE);                                           \
    }                                                           \
    STTBX_Print((x));                                           \
}




/* --- Local variables ----------------------------------------------------- */

static char LAY_Msg[250];
/*static */STLAYER_Handle_t LayHndl;    /* used in tt_util.c */
static STLAYER_ViewPortHandle_t VPHndl;
static AllocatedBlockHandle_t AllocatedBlockTable[MAX_ALLOCATED_BLOCK];
static const LAYER_StringToEnum_t LayerType[MAX_LAYER_TYPE] =
    {
        { "GAMMA_CURSOR", STLAYER_GAMMA_CURSOR },
        { "GAMMA_GDP", STLAYER_GAMMA_GDP },
        { "GAMMA_BKL", STLAYER_GAMMA_BKL },
        { "GAMMA_ALPHA", STLAYER_GAMMA_ALPHA },
        { "GAMMA_FILTER", STLAYER_GAMMA_FILTER },
        { "OMEGA2_VIDEO1", STLAYER_OMEGA2_VIDEO1 },
        { "OMEGA2_VIDEO2", STLAYER_OMEGA2_VIDEO2 },
        { "7020_VIDEO1", STLAYER_7020_VIDEO1 },
        { "7020_VIDEO2", STLAYER_7020_VIDEO2 },
        { "OMEGA1_CURSOR", STLAYER_OMEGA1_CURSOR },
        { "OMEGA1_VIDEO", STLAYER_OMEGA1_VIDEO },
        { "OMEGA1_OSD", STLAYER_OMEGA1_OSD },
        { "OMEGA1_STILL", STLAYER_OMEGA1_STILL },
        { "COMPOSITOR", STLAYER_COMPOSITOR},
        { "SDDISPO2_VIDEO1", STLAYER_SDDISPO2_VIDEO1 },
        { "SDDISPO2_VIDEO2", STLAYER_SDDISPO2_VIDEO2 },
        { "HDDISPO2_VIDEO1", STLAYER_HDDISPO2_VIDEO1 },
        { "HDDISPO2_VIDEO2", STLAYER_HDDISPO2_VIDEO2 },
        { "VDP_VIDEO1", STLAYER_DISPLAYPIPE_VIDEO1 },
        { "VDP_VIDEO2", STLAYER_DISPLAYPIPE_VIDEO2 },
        { "VDP_VIDEO3", STLAYER_DISPLAYPIPE_VIDEO3 },
        { "VDP_VIDEO4", STLAYER_DISPLAYPIPE_VIDEO4 },
        { "GAMMA_GDPVBI", STLAYER_GAMMA_GDPVBI }
};
static const char AspectRatioType[MAX_ASPECTRATIO_TYPE][7] =
    {
        "16/9", "4/3", "221/01", "SQUARE"
    };

static const char ScanTypeType[MAX_SCANTYPE_TYPE][6] =
    {
        "Prog", "Int  "
    };

/* --- Externals ----------------------------------------------------------- */

#if ((!defined(ST_7109)) && (!defined(ST_7100)))
extern ST_Partition_t *             SystemPartition_p;
#endif
#ifdef ST_XVP_ENABLE_FLEXVP
extern ST_Partition_t *             NCachePartition_p;
#endif


/* Functions ---------------------------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : API_TagExpected
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void API_TagExpected(const LAYER_StringToEnum_t * UnionType,
        const U32 MaxValue, const char * DefaultValue)
{
    U32  i;

    sprintf(LAY_Msg, "Expected enum (");
    for(i=0; i<MaxValue; i++)
    {
        if(!strcmp(UnionType[i].String, DefaultValue))
        {
            strcat(LAY_Msg,"def ");
        }
        sprintf(LAY_Msg, "%s%d=%s,", LAY_Msg, UnionType[i].Value,
                UnionType[i].String);
    }
    /* assumes that one component is listed at least */
    strcat(LAY_Msg,"\b)");
    if(strlen(LAY_Msg) >= sizeof(LAY_Msg))
    {
        sprintf(LAY_Msg, "String is too long\n");
    }
}
#ifndef ST_OSLINUX
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) \
 || defined(ST_5516) || ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

/*-----------------------------------------------------------------------------
 * Function : GetBitmap,  55xx only
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void GetBitmap(STGXOBJ_Bitmap_t * Pixmap,STAVMEM_BlockHandle_t* BlockHandle_p)
{
    ST_ErrorCode_t  RepError;
    STAVMEM_AllocBlockParams_t AllocParams;
    long int  FileDescriptor;    /* File pointer */
    char * FileName;
    U32  Size;
    void * Unpitched_Data_p;
    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualM;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualM);

    FileName =STOS_MemoryAllocate(SystemPartition_p,strlen(StapigatDataPath)+10);


    Pixmap->Width      = 768;
    Pixmap->Height     = 576;
    Pixmap->Pitch      = Pixmap->Width * 2; /* 2bits per pixel */
    if (Pixmap->Pitch % 128 != 0)
    {
        Pixmap->Pitch = Pixmap->Pitch -(Pixmap->Pitch % 128) + 128;
    }
    Pixmap->ColorType  = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    Pixmap->BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM;
    AllocParams.PartitionHandle = AvmemPartitionHandle[0];
    AllocParams.Size =Pixmap->Pitch * Pixmap->Height ;
    AllocParams.Alignment = 128;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *) NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p = (void **) NULL;
    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams, BlockHandle_p);
    if (RepError)
    {
        STTBX_DirectPrint(( "Unable to allocate mem!!!\n"));
        return;
    }

    STAVMEM_GetBlockAddress( *BlockHandle_p, (void **)&(Pixmap->Data1_p));
    Pixmap->Data2_p = (U8*)Pixmap->Data1_p + AllocParams.Size/2;
    STTBX_DirectPrint(("Pixmap allocated\n" ));
    /* ------------------------- */
    /* Pixmap loading from file */
    /* ------------------------- */

    STTBX_DirectPrint(("Frame buffer loading , please wait ...\n"));
    sprintf(FileName, "%sZdf.yuv", StapigatDataPath);
    FileDescriptor = debugopen( (char *)FileName, "rb");
    if (FileDescriptor == -1)
    {
        STTBX_DirectPrint(( "Unable to open file !!!\n"));
        return;
    }

    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams,
                                   (STAVMEM_BlockHandle_t *) &(BlockHandle));
    if (RepError)
    {
        STTBX_DirectPrint(( "Unable to allocate mem!!!\n"));
        return;
    }
    STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Unpitched_Data_p));
    Unpitched_Data_p = STAVMEM_VirtualToCPU(Unpitched_Data_p,&VirtualM);
    Size =(U32)debugread(FileDescriptor,(U8*)Unpitched_Data_p,AllocParams.Size);
    if(Size != AllocParams.Size)
    {
        STTBX_DirectPrint(( "Unable to read bitmap file !!!\n"));
        return;
    }

    STTBX_DirectPrint(( "Top Bottom (768x576) Picture loaded\n"));

    debugclose(FileDescriptor);

    STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
                        (Pixmap->Width * 2),
                        (Pixmap->Height / 2),
                        (Pixmap->Width * 2),
                        (void *)STAVMEM_VirtualToCPU(Pixmap->Data1_p,&VirtualM),
                        (Pixmap->Pitch) );
    STAVMEM_CopyBlock2D ( (void *)((U32)(Unpitched_Data_p)+
                                   (AllocParams.Size/2)),
                        (Pixmap->Width * 2),
                        (Pixmap->Height / 2),
                        (Pixmap->Width * 2),
                        (void *)STAVMEM_VirtualToCPU(Pixmap->Data2_p,&VirtualM),
                          (Pixmap->Pitch) );
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandle);

    return ;
}
/*-----------------------------------------------------------------------------
 * Function : GetBitmapProg,  55xx only
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void GetBitmapProg(STGXOBJ_Bitmap_t * Pixmap,STAVMEM_BlockHandle_t* BlockHandle_p)
{
    ST_ErrorCode_t  RepError;
    STAVMEM_AllocBlockParams_t AllocParams;
    long int  FileDescriptor;    /* File pointer */
    char * FileName;
    U32  Size;
    void * Unpitched_Data_p;
    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualM;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualM);

    FileName = STOS_MemoryAllocate(SystemPartition_p,60);

    Pixmap->Width      = 768;
    Pixmap->Height     = 576;
    Pixmap->Pitch      = Pixmap->Width * 2; /* 2bits per pixel */
    if (Pixmap->Pitch % 128 != 0)
    {
        Pixmap->Pitch = Pixmap->Pitch -(Pixmap->Pitch % 128) + 128;
    }
    Pixmap->ColorType  = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    Pixmap->BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
    AllocParams.PartitionHandle = AvmemPartitionHandle[0];
    AllocParams.Size =Pixmap->Pitch * Pixmap->Height ;
    AllocParams.Alignment = 128;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *) NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p = (void **) NULL;
    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams, BlockHandle_p);
    if (RepError)
    {
        STTBX_DirectPrint(( "Unable to allocate mem!!!\n"));
        return;
    }

    STAVMEM_GetBlockAddress( *BlockHandle_p, (void **)&(Pixmap->Data1_p));
    STTBX_DirectPrint(("Pixmap allocated\n" ));
    /* ------------------------- */
    /* Pixmap loading from file */
    /* ------------------------- */

    STTBX_DirectPrint(("Frame buffer loading , please wait ...\n"));
    sprintf(FileName, "../../../scripts/kiss.yuv");
    FileDescriptor = debugopen( (char *)FileName, "rb");
    if(FileDescriptor == -1){
        STTBX_DirectPrint(( "Unable to open file !!!\n"));
        return;
    }

    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams,
                                   (STAVMEM_BlockHandle_t *) &(BlockHandle));
    if (RepError)
    {
        STTBX_DirectPrint(( "Unable to allocate mem!!!\n"));
        return;
    }
    STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Unpitched_Data_p));
    Unpitched_Data_p = STAVMEM_VirtualToCPU(Unpitched_Data_p,&VirtualM);
    Size =(U32)debugread(FileDescriptor,(U8*)Unpitched_Data_p,AllocParams.Size);
    if(Size != AllocParams.Size)
    {
        STTBX_DirectPrint(( "Unable to read bitmap file !!!\n"));
        return;
    }

    debugclose(FileDescriptor);

    STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
                        (Pixmap->Width * 2),
                        (Pixmap->Height),
                        (Pixmap->Width * 2),
                        (void *)STAVMEM_VirtualToCPU(Pixmap->Data1_p,&VirtualM),
                        (Pixmap->Pitch) );
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandle);

    return ;
}
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5516) (ST_5514 S/A) (ST_5517 S/A) */

#endif /* ST_OSLINUX */
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)\
  || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)\
  || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_5100) || defined (ST_5105)\
  || defined (ST_5301) || defined (ST_5188)|| defined (ST_5525) || defined(ST_5107) || defined(ST_5162)
#ifndef ST_OSLINUX
/*******************************************************************************
Name        : GUTIL_AllocBuffer
Description : Allocate buffer in AVMEM
Returns     : Error code
*******************************************************************************/
ST_ErrorCode_t GUTIL_AllocBuffer( U32 Size, U32  Alignment, STAVMEM_BlockHandle_t* BlockHandle_p, void **BufferAddress_p )
{
    ST_ErrorCode_t                              ErrCode = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t                  AllocBlockParams;

    memset((void *)&AllocBlockParams, 0,
           sizeof(STAVMEM_AllocBlockParams_t));

    AllocBlockParams.PartitionHandle          = AvmemPartitionHandle[0];
    AllocBlockParams.Size                     = Size ;
    AllocBlockParams.Alignment                = Alignment ;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = 0;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;
    ErrCode = STAVMEM_AllocBlock (&AllocBlockParams, BlockHandle_p);
    if (ErrCode == ST_NO_ERROR )
    {
        STAVMEM_GetBlockAddress( *BlockHandle_p, BufferAddress_p );
        STAVMEM_GetSharedMemoryVirtualMapping(&VM);
        *BufferAddress_p = STAVMEM_VirtualToCPU(*BufferAddress_p,&VM);
    }
    return ErrCode;
} /* End of GUTIL_AllocBuffer() */

#endif
/*---------------------------------------------------------------------
 * Function : GUTIL_LoadBitmap
 *            Load a Bitmap and his palette.
 *            Store information into provided structure.
 * Warning  : This function is responsible for the memory allocation of
 *            bitmap and palette buffer. It would first unallocate buffer
 *            if the provided data pointer are not NULL. Be very carefull
 *            with this !
 * Input    : char *filename,
 *            STGXOBJ_Bitmap_t *Bitmap_p
 *            STGXOBJ_Palette_t *Palette_p
 * Info     : Following bitmap type are supported :
 *            CLUT8, ACLUT88, ACLUT44 with ARGB8888 palette,
 *            RGB565, ARGB1555, ARGB4444, YCbCR 4:2:2 Raster
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
#ifdef ST_OSLINUX
static ST_ErrorCode_t GUTIL_LoadBitmap1(char *filename,
                                STGXOBJ_Bitmap_t *Bitmap_p,
                                STGXOBJ_Palette_t *Palette_p )
#else
static ST_ErrorCode_t GUTIL_LoadBitmap(char *filename,
                                STGXOBJ_Bitmap_t *Bitmap_p,
                                STGXOBJ_Palette_t *Palette_p,
                                STAVMEM_BlockHandle_t* BitmapHandle_p,
                                STAVMEM_BlockHandle_t* PaletteHandle_p)
#endif
{
#ifdef ST_OSLINUX
    STLAYER_AllocDataParams_t LayAllocData;
#endif

    ST_ErrorCode_t ErrCode;/* Error management                       */
    FILE *fstream=NULL;        /* File handle for read operation          */
    U32  size=0, size2=0;            /* Used to test returned size for a read   */
    U32 dummy[2];         /* used to read colorkey, but not used     */
    GUTIL_GammaHeader   Gamma_Header;     /* Header of Bitmap file                   */
    BOOL IsPalette;       /* Palette is present into the current file*/
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;
    U32 Signature;

    ErrCode   = ST_NO_ERROR;
    Signature = 0x0;
    IsPalette = FALSE;


#ifndef ST_OSLINUX
    /*UNUSED_PARAMETER(BitmapHandle_p);*/
    UNUSED_PARAMETER(PaletteHandle_p);
#endif

    /* Init structure                                                */
    memset((void *)&BitmapAllocParams1, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));
    memset((void *)&BitmapAllocParams2, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));

    /* Check parameter                                               */
    if ( (filename == NULL) ||
         (Bitmap_p == NULL)
        )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    STTBX_Print(( "Loading file %s...\n", filename));
    /* Open file handle                                              */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Unable to open \'%s\'\n", filename ));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read Header                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Reading file Header ... \n"));
        size = fread((void *)&Gamma_Header, 1,
                     sizeof(GUTIL_GammaHeader), fstream);
        if (size != sizeof(GUTIL_GammaHeader))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Read error %d byte instead of %d\n",
                          size,sizeof(GUTIL_GammaHeader)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

    /* Check Header                                                  */
    if (ErrCode == ST_NO_ERROR)
    {
        if ( ( ( Gamma_Header.Header_size != 0x6 ) && ( Gamma_Header.Header_size != 0x8 ) ) ||
             ( ( Gamma_Header.zero != 0x0 ) && ( Gamma_Header.Type != GAMMA_FILE_YCBCR420MB ) ) )

        {
            STTBX_Print(("Read %d waited 0x6 or 0x8\n",
                         Gamma_Header.Header_size));
            STTBX_Print(("Read %d waited 0x0\n",
                         Gamma_Header.Properties));
            STTBX_Print(("Read %d waited 0x0\n",
                         Gamma_Header.zero));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Not a valid file (Header corrupted)\n"));
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Read error %d byte instead of %d\n", size,4));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables          */
    /* And do bitmap allocation                                      */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        Bitmap_p->PreMultipliedColor     = FALSE;
        Bitmap_p->ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        Bitmap_p->AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;

        /* Configure in function of the bitmap type                  */
        switch (Gamma_Header.Type)
        {
        case GAMMA_FILE_CLUT2 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_CLUT2;
                if (Gamma_Header.width % 4 == 0)
                {
                    Bitmap_p->Pitch    = Gamma_Header.width / 4;
                }
                else
                {
                    Bitmap_p->Pitch    = (Gamma_Header.width / 4) + 1;
                }
                Bitmap_p->Size1        =  Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 2;
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                IsPalette = TRUE;
            }
            break;
        }
        case GAMMA_FILE_CLUT4 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_CLUT4;
                if ( Gamma_Header.width % 2 == 0)
                {
                    Bitmap_p->Pitch    = Gamma_Header.width / 2;
                }
                else
                {
                    Bitmap_p->Pitch    = (Gamma_Header.width / 2) + 1;
                }
                Bitmap_p->Size1        = Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 4;
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                IsPalette = TRUE;
            }
            break;
        }
        case GAMMA_FILE_ACLUT8 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_CLUT8;
                Bitmap_p->Pitch        =  Bitmap_p->Width;
                Bitmap_p->Size1        =  Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB4444;
                Palette_p->PaletteType =STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 8;
                IsPalette = TRUE;
            }
            break;
        }
        case GAMMA_FILE_CLUT8 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_CLUT8;
                Bitmap_p->Pitch        = Bitmap_p->Width;
                Bitmap_p->Size1        = Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 8;
                IsPalette = TRUE;
            }
            break;
        }
        case GAMMA_FILE_ACLUT88 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_ACLUT88;
                Bitmap_p->Pitch        = Bitmap_p->Width * 2;
                Bitmap_p->Size1        = Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 8;
                IsPalette              = TRUE;
            }
            break;
        }
        case GAMMA_FILE_ACLUT44 :
        {
            Signature = 0x444F;
            if (Palette_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_ACLUT44;
                Bitmap_p->Pitch        = Bitmap_p->Width;
                Bitmap_p->Size1        = Bitmap_p->Pitch * Bitmap_p->Height;
                Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                Palette_p->ColorDepth  = 4;
                IsPalette              = TRUE;
            }
            break;
        }
        case GAMMA_FILE_ALPHA1 :
        {
            Signature                   = 0x444F;
            Bitmap_p->ColorType         = STGXOBJ_COLOR_TYPE_ALPHA1;
            if (Gamma_Header.width % 8 == 0)
            {
                Bitmap_p->Pitch         = Gamma_Header.width / 8;
            }
            else
            {
                Bitmap_p->Pitch         = (Gamma_Header.width / 8) + 1;

            }
            Bitmap_p->Size1     =  Bitmap_p->Pitch * Bitmap_p->Height;
            if (Gamma_Header.Properties == 0)
            {
                Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
            }
            else
            {
                Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
            }
            break;
        }
        case GAMMA_FILE_RGB565 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_RGB565;
            Bitmap_p->Pitch     =  Bitmap_p->Width * 2;
            Bitmap_p->Size1     =  Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_RGB888:
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_RGB888;
            Bitmap_p->Pitch         =  Bitmap_p->Width * 3;
            Bitmap_p->Size1     =  Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_ARGB1555 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB1555;
            Bitmap_p->Pitch     =  Bitmap_p->Width * 2;
            Bitmap_p->Size1     =  Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_ARGB4444 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB4444;
            Bitmap_p->Pitch     =  Bitmap_p->Width * 2;
            Bitmap_p->Size1     =  Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_ARGB8565 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB8565;
            Bitmap_p->Pitch     = Bitmap_p->Width * 3;
            Bitmap_p->Size1     = Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_ARGB8888 :
        {
            Signature           = 0x444F;
            Bitmap_p->Pitch     = Gamma_Header.width*4;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB8888;
            Bitmap_p->Size1     = Gamma_Header.nb_pixel*4;
            break;
        }
        case GAMMA_FILE_YCBCR888 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
            break;

            break;
        }
        case GAMMA_FILE_YCBCR422R :
        {
            Signature           = 0x422F;
            if ((Gamma_Header.width % 2) == 0)
            {
                Bitmap_p->Pitch = (U32)(Gamma_Header.width * 2);
            }
            else
            {
                Bitmap_p->Pitch = (U32)((Gamma_Header.width - 1) * 2 + 4);
            }
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
            Bitmap_p->Size1     = Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_AYCBCR8888 :
        {
            Signature           = 0x444F;
            Bitmap_p->Pitch     = Gamma_Header.width * 4;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888;
            Bitmap_p->Size1     = Bitmap_p->Pitch * Bitmap_p->Height;
            break;
        }
        case GAMMA_FILE_YCBCR420MB :
        {
            if ( Gamma_Header.Signature == 0x422F )
            {
                Signature           = 0x422F;
                Bitmap_p->BitmapType       = STGXOBJ_BITMAP_TYPE_MB_TOP_BOTTOM;
            }

            if ( Gamma_Header.Signature == 0x420F )
            {
                Signature           = 0x420F;
                Bitmap_p->BitmapType       = STGXOBJ_BITMAP_TYPE_MB;
            }

            if ((Gamma_Header.width % 16) == 0)   /* MB size multiple */
            {
                Bitmap_p->Pitch        = Gamma_Header.width;
            }
            else
            {
                Bitmap_p->Pitch        = (Gamma_Header.width/16 + 1) * 16;
            }
            Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
            Bitmap_p->Size1            = Gamma_Header.nb_pixel;
            Bitmap_p->Size2            = Gamma_Header.zero;
            break;
        }
        default :
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Type not supported 0x%08x\n",
                          Gamma_Header.Type));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
        }
        } /* switch (Gamma_Header.Type) */
    }

	printf("Size:%x Pitch:%x\n",Bitmap_p->Size1, Bitmap_p->Pitch);

    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Signature !=  Signature)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Error: Read 0x%08x, Waited 0x%08x\n",
                          Gamma_Header.Signature, Signature));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* If necessary palette management                               */
    if ( (ErrCode == ST_NO_ERROR) && (IsPalette) )
    {
        /* Read palette if necessary                              */
        if (ErrCode == ST_NO_ERROR)
        {
            STTBX_Print(("Read palette ...\n"));

            STLAYER_GetPaletteAllocParams( LayHndl, Palette_p, &PaletteLCParams );
#ifdef ST_OSLINUX
            LayAllocData.Size = PaletteLCParams.AllocBlockParams.Size ;
            LayAllocData.Alignment = PaletteLCParams.AllocBlockParams.Alignment ;
            STLAYER_AllocData( LayHndl, &LayAllocData,
                            &Palette_p->Data_p );
            if ( Palette_p->Data_p == NULL )
            {
                STTBX_Print(("Error Allocating data for palette...\n"));
                return -1;
            }
            size = fread ((void *)Palette_p->Data_p, 1, (PaletteLCParams.AllocBlockParams.Size), fstream );
#else
            GUTIL_AllocBuffer( PaletteLCParams.AllocBlockParams.Size, PaletteLCParams.AllocBlockParams.Alignment,
                            BitmapHandle_p,&Palette_p->Data_p );

            size = fread ((void *)STAVMEM_VirtualToCPU(Palette_p->Data_p,&VM)
                    , 1,(PaletteLCParams.AllocBlockParams.Size), fstream);
#endif
            if (size != (PaletteLCParams.AllocBlockParams.Size))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Read error %d byte instead of %d\n",size,
                              (PaletteLCParams.AllocBlockParams.Size)));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }


        }
    }


    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Read bitmap ...\n"));
        if ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB )
        {
            BitmapLCParams1.AllocBlockParams.Size = Bitmap_p->Size1;
            BitmapLCParams1.AllocBlockParams.Alignment = 16;
            BitmapLCParams2.AllocBlockParams.Size = Bitmap_p->Size2;
            BitmapLCParams2.AllocBlockParams.Alignment = 16;
        }
        else
        {
            if ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 )
            {
                STGXOBJ_BitmapAllocParams_t     BitmapParams1, BitmapParams2;

#ifdef ST_OSLINUX
                STLAYER_GetBitmapAllocParams(LayHndl, Bitmap_p, &BitmapParams1,
                &BitmapParams2 );
#else
                STGXOBJ_GetBitmapAllocParams(Bitmap_p, STGXOBJ_GAMMA_BLITTER,&BitmapParams1, &BitmapParams2);
#endif
                Bitmap_p->Pitch                            = BitmapParams1.Pitch;
                Bitmap_p->Offset                           = BitmapParams1.Offset;
                Bitmap_p->Size1                            = BitmapParams1.AllocBlockParams.Size;
                BitmapLCParams1.AllocBlockParams.Size      = BitmapParams1.AllocBlockParams.Size;
                BitmapLCParams1.AllocBlockParams.Alignment = BitmapParams1.AllocBlockParams.Alignment;
            }
            else
            {
                STLAYER_GetBitmapAllocParams(LayHndl, Bitmap_p, &BitmapLCParams1,
                &BitmapLCParams2 );
            }
        }
        STTBX_Print(("Size: 0x%x Alignment:0x%x\n", BitmapLCParams1.AllocBlockParams.Size, BitmapLCParams1.AllocBlockParams.Alignment ));

#ifdef ST_OSLINUX
        LayAllocData.Size = BitmapLCParams1.AllocBlockParams.Size ;
        LayAllocData.Alignment = BitmapLCParams1.AllocBlockParams.Alignment ;
        STLAYER_AllocData( LayHndl, &LayAllocData,
                            &Bitmap_p->Data1_p );
        if ( Bitmap_p->Data1_p == NULL )
        {
            STTBX_Print(("Error Allocating data for bitmap...\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        STTBX_Print(("Reading file Bitmap data Address data bitmap = 0x%x... \n", Bitmap_p->Data1_p ));
        size = fread ((void*)(Bitmap_p->Data1_p),1,
                      (Bitmap_p->Size1)*Gamma_Header.NbPict, fstream);
#else
        GUTIL_AllocBuffer( BitmapLCParams1.AllocBlockParams.Size, BitmapLCParams1.AllocBlockParams.Alignment,
                            BitmapHandle_p,&Bitmap_p->Data1_p );
        STTBX_Print(("Reading file Bitmap data (V2C:0x%lx, V2D:0x%lx) ... \n", STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM), STAVMEM_VirtualToDevice(Bitmap_p->Data1_p,&VM)));
        if ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 )
        {
            U32 BitmapDataBA;
            U32 i;
            U32 PixelsBuffer;
            U8 TempBuffer;
            U8* AddDst;
            U8 Y,Cb,Cr;

            BitmapDataBA = (U32)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM));

            for ( i=1 ; i<=Gamma_Header.nb_pixel ; i++ )
            {
                fread (&PixelsBuffer, 3, 1, fstream);
                fread (&TempBuffer, 1, 1, fstream);

                Y  = ( ( PixelsBuffer & 0xFF ) );
                Cb = ( ( PixelsBuffer & 0xFF00 ) >> 8 );
                Cr = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );

                AddDst = ((U8*)BitmapDataBA) + Bitmap_p->Offset + 3*(i-1);
                *((U8 *)AddDst )     = Cb ;
                *((U8 *)AddDst + 1 ) = Y ;
                *((U8 *)AddDst + 2 ) = Cr ;
            }
        }
        else if ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888)
        {
            U32 BitmapDataBA;
            U32 i;
            U32 PixelsBuffer;
            U8* AddDst;
            U8 Y,Cb,Cr,A;

            BitmapDataBA = (U32)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM));

            for ( i=1 ; i<=Gamma_Header.nb_pixel ; i++ )
            {
                fread (&PixelsBuffer, 4, 1, fstream);

                Y  = ( ( PixelsBuffer & 0xFF ) );
                Cb = ( ( PixelsBuffer & 0xFF00 ) >> 8 );
                Cr = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );
                A =  ( ( PixelsBuffer & 0xFF000000 ) >> 24 );

                AddDst = ((U8*)BitmapDataBA) + Bitmap_p->Offset + 4*(i-1);
                *((U8 *)AddDst )     = Cb ;
                *((U8 *)AddDst + 1 ) = Y ;
                *((U8 *)AddDst + 2 ) = Cr ;
                *((U8 *)AddDst + 3 ) = A ;
            }
        }
        else if ( Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_ARGB8565)
        {
            U32 BitmapDataBA;
            U32 i;
            U32 PixelsBuffer;
            U8 TempBuffer;
            U8* AddDst;
            U8 BG,RG,A;

            BitmapDataBA = (U32)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM));

            for ( i=1 ; i<=Gamma_Header.nb_pixel ; i++ )
            {
                fread (&PixelsBuffer, 3, 1, fstream);
                fread (&TempBuffer, 1, 1, fstream);

                BG  = ( ( PixelsBuffer & 0xFF ) );
                RG  = ( ( PixelsBuffer & 0xFF00 ) >> 8 );
                A   = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );

                AddDst = ((U8*)BitmapDataBA) + Bitmap_p->Offset + 3*(i-1);
                *((U8 *)AddDst )     = BG ;
                *((U8 *)AddDst + 1 ) = RG ;
                *((U8 *)AddDst + 2 ) = A ;
            }
        }
        else if (Bitmap_p->ColorType == STGXOBJ_COLOR_TYPE_RGB888)
        {
            U32 BitmapDataBA;
            U32 i;
            U32 PixelsBuffer;
            U8 TempBuffer;
            U8* AddDst;
            U8 B,R,G;

            BitmapDataBA = (U32)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM));

            for ( i=1 ; i<=Gamma_Header.nb_pixel ; i++ )
            {
                fread (&PixelsBuffer, 3, 1, fstream);
                fread (&TempBuffer, 1, 1, fstream);

                /*swap the colors*/
                B  = ( ( PixelsBuffer & 0xFF ) );
                G  = ( ( PixelsBuffer & 0xFF00 ) >> 8 );
                R  = ( ( PixelsBuffer & 0xFF0000 ) >> 16 );

                AddDst = ((U8*)BitmapDataBA) + Bitmap_p->Offset + 3*(i-1);
                *((U8 *)AddDst )     = B ;
                *((U8 *)AddDst + 1 ) = G ;
                *((U8 *)AddDst + 2 ) = R ;
            }
        }
        else
        {
            size = fread ((void*)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM)),1,
                      (Bitmap_p->Size1)*Gamma_Header.NbPict, fstream);

            if ( Bitmap_p->BitmapType == STGXOBJ_BITMAP_TYPE_MB )
            {
                GUTIL_AllocBuffer( BitmapLCParams2.AllocBlockParams.Size, BitmapLCParams2.AllocBlockParams.Alignment,BitmapHandle_p, &Bitmap_p->Data2_p );
                STTBX_Print(("Reading file Bitmap chroma data (V2C:0x%lx, V2D:0x%lx) ... \n", STAVMEM_VirtualToCPU(Bitmap_p->Data2_p,&VM), STAVMEM_VirtualToDevice(Bitmap_p->Data2_p,&VM)));
                size2 = fread ((void*)(STAVMEM_VirtualToCPU(Bitmap_p->Data2_p,&VM)),1,(Bitmap_p->Size2)*Gamma_Header.NbPict, fstream);
            }
        }
#endif

        if ( Bitmap_p->ColorType != STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444 )
        {
            if (size != ((Bitmap_p->Size1)*Gamma_Header.NbPict) )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "Read error %d byte instead of %d\n", size,
                            (Bitmap_p->Size1)*Gamma_Header.NbPict));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }

            if (size2 != ((Bitmap_p->Size2)*Gamma_Header.NbPict) )
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "Read error %d byte instead of %d\n", size2,
                            (Bitmap_p->Size2)*Gamma_Header.NbPict));
                ErrCode = ST_ERROR_BAD_PARAMETER;
            }
        }

        fclose (fstream);
    }

    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                      "GUTIL loaded correctly %s file\n",filename));

    }


#if defined (DVD_SECURED_CHIP) && defined(STLAYER_NO_STMES)

    Bitmap_p->Data1_p = (U32)((U32)Bitmap_p->Data1_p | STLAYER_SECURE_ON) ;
#endif

    /*Bitmap_p->ColorSpaceConversion=STGXOBJ_SMPTE_170M;  */

    return ErrCode;

}

#ifdef STLAYER_CONVERT_PALETTE_TO_YCBCR888
/*---------------------------------------------------------------------
 * Function : GUTIL_GetYCbCr888Palette
 *            Generate a YCbCr888 Palette
 *            Store information into provided structure.
 * Warning  : This function is responsible for the memory allocation of
 *            palette buffer. It would first unallocate buffer
 *            if the provided data pointer are not NULL. Be very carefull
 *            with this !
 * Input    : STGXOBJ_Palette_t *Palette_p
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
static ST_ErrorCode_t GUTIL_GetYCbCr888Palette(STGXOBJ_Palette_t *Palette_p)
{
    ST_ErrorCode_t ErrCode   = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;  /* Allocation param                       */
    STAVMEM_BlockHandle_t  BlockHandle;


    if (Palette_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
        Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
        Palette_p->ColorDepth  = 8;
    }

    /* If necessary palette management                               */
        if (ErrCode == ST_NO_ERROR)
        {
/*            STLAYER_GetPaletteAllocParams( LayHndl, Palette_p, &PaletteLCParams );*/
            AllocBlockParams.PartitionHandle = AvmemPartitionHandle[0];

            AllocBlockParams.Size            = 1024 /*PaletteLCParams.AllocBlockParams.Size*/;
            AllocBlockParams.Alignment       = 16 /*PaletteLCParams.AllocBlockParams.Alignment*/;

            AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges= 0;
            AllocBlockParams.ForbiddenRangeArray_p    = 0;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle));
            STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Palette_p->Data_p));
        }

    return ErrCode;

}
#endif /* STLAYER_CONVERT_PALETTE_TO_YCBCR888 */

#ifdef STLAYER_CONVERT_PALETTE_TO_AYCBCR8888
/*---------------------------------------------------------------------
 * Function : GUTIL_GetAYCbCr8888Palette
 *            Generate a AYCbCr8888 Palette
 *            Store information into provided structure.
 * Warning  : This function is responsible for the memory allocation of
 *            palette buffer. It would first unallocate buffer
 *            if the provided data pointer are not NULL. Be very carefull
 *            with this !
 * Input    : STGXOBJ_Palette_t *Palette_p
 * Output   : N/A
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
static ST_ErrorCode_t GUTIL_GetAYCbCr8888Palette(STGXOBJ_Palette_t *Palette_p)
{
    ST_ErrorCode_t ErrCode   = ST_NO_ERROR;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;  /* Allocation param                       */
    STAVMEM_BlockHandle_t  BlockHandle;


    if (Palette_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888;
        Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
        Palette_p->ColorDepth  = 8;
    }

    /* If necessary palette management                               */
        if (ErrCode == ST_NO_ERROR)
        {
/*            STLAYER_GetPaletteAllocParams( LayHndl, Palette_p, &PaletteLCParams );*/
            AllocBlockParams.PartitionHandle = AvmemPartitionHandle[0];

            AllocBlockParams.Size            = 1024 /*PaletteLCParams.AllocBlockParams.Size*/;
            AllocBlockParams.Alignment       = 16 /*PaletteLCParams.AllocBlockParams.Alignment*/;

            AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges= 0;
            AllocBlockParams.ForbiddenRangeArray_p    = 0;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle));
            STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Palette_p->Data_p));
        }

    return ErrCode;

}
#endif /* STLAYER_CONVERT_PALETTE_TO_AYCBCR8888 */


#endif /* ST_7015 ST_7020 ST_GX1 or ST_5516/ST_5517 for RGB16*/

/*-----------------------------------------------------------------------------
 * Function : LAYER_TextError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL LAYER_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    if (Error != ST_NO_ERROR)
    {
        RetErr = TRUE;
        API_ErrorCount++;
    }
    /* Error not found in common ones ? */
    if (!(API_TextError(Error, Text)))
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STLAYER_ERROR_INVALID_INPUT_RECTANGLE:
                strcat( Text, "(stlayer error invalid input rectangle !)\n");
                break;
            case STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE:
                strcat( Text, "(stlayer error invalid output rectangle !)\n");
                break;
            case STLAYER_ERROR_NO_FREE_HANDLES:
                strcat( Text, "(stlayer no free handles !)\n");
                break;
            case STLAYER_SUCCESS_IORECTANGLES_ADJUSTED:
                strcat( Text, "(stlayer success iorectangles adjusted !)\n");
                break;
            case STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE:
                strcat( Text,"(stlayer error iorectangles not adjustable !)\n");
                break;
            case STLAYER_ERROR_INVALID_LAYER_TYPE:
                strcat( Text, "(stlayer error invalid layer type !)\n");
                break;
            case STLAYER_ERROR_USER_ALLOCATION_NOT_ALLOWED:
                strcat( Text,"(stlayer error user allocation not allowed !)\n");
                break;
            case STLAYER_ERROR_OVERLAP_VIEWPORT:
                strcat( Text, "(stlayer error overlap viewport !)\n");
                break;
            case STLAYER_ERROR_NO_AV_MEMORY :
                strcat( Text, "(stlayer error no av memory !)\n");
                break;
            case STLAYER_ERROR_OUT_OF_LAYER:
                strcat( Text, "(stlayer error out of layer !)\n");
                break;
            case STLAYER_ERROR_OUT_OF_BITMAP:
                strcat( Text, "(stlayer error out of bitmap !)\n");
                break;
            case STLAYER_ERROR_INSIDE_LAYER:
                strcat( Text, "(stlayer error inside layer !)\n");
                break;
            case STLAYER_ERROR_EVENT_REGISTRATION:
                strcat( Text, "(stlayer error event registration !)\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    LAYER_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/* Private Function prototypes ---------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : LAYER_AdjustIORectangle
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_AdjustIORectangle (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STGXOBJ_Rectangle_t InputRectangle;
    STGXOBJ_Rectangle_t OutputRectangle;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the layer handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected handle" );
        return(TRUE);
    }
    LayHndl= (STLAYER_Handle_t)Lvar;

    /* get the X position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected X" );
        return(TRUE);
    }
    InputRectangle.PositionX = Lvar;

    /* get the Y position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y" );
        return(TRUE);
    }
    InputRectangle.PositionY = Lvar;

    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 20, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Width" );
        return(TRUE);
    }
    InputRectangle.Width = Lvar;

    /* get the Height input rectangle */
    RetErr = STTST_GetInteger( pars_p, 20, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Height" );
        return(TRUE);
    }
    InputRectangle.Height = Lvar;

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected X" );
        return(TRUE);
    }
    OutputRectangle.PositionX = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y" );
        return(TRUE);
    }
    OutputRectangle.PositionY = Lvar;

    /* get the Width output rectangle */
    RetErr = STTST_GetInteger( pars_p, 20, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Width" );
        return(TRUE);
    }
    OutputRectangle.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 20, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Height" );
        return(TRUE);
    }
    OutputRectangle.Height = Lvar;

    sprintf(LAY_Msg, "STLAYER_AdjustIORectangle(%d): ",LayHndl);
    ErrCode = STLAYER_AdjustIORectangle( LayHndl, &InputRectangle,
                                         &OutputRectangle );
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    if((ErrCode ==  STLAYER_SUCCESS_IORECTANGLES_ADJUSTED)
       || (ErrCode == ST_NO_ERROR))
    {
        RetErr = FALSE;
        sprintf(LAY_Msg, " Rectangles adjusted\n"
                "Xin:%d\nYin:%d\nWidthin:%d\nHeightin:"
                "%d\nXout:%d\nYout:%d\nWidthout:%d\nHeightout:%d\n",
                InputRectangle.PositionX,
                InputRectangle.PositionY,
                InputRectangle.Width,
                InputRectangle.Height,
                OutputRectangle.PositionX,
                OutputRectangle.PositionY,
                OutputRectangle.Width,
                OutputRectangle.Height);
        STTBX_Print((  LAY_Msg ));
    }
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_AdjustViewPortParams
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_AdjustViewPortParams (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_ViewPortParams_t VPParams;
    STLAYER_ViewPortSource_t source;
    STLAYER_StreamingVideo_t VideoStream;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);
    /* get the handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected layer Handle" );
        return(TRUE);
    }

    /* get the X position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input X (default 0)" );
        return(TRUE);
    }
    VPParams.InputRectangle.PositionX = Lvar;

    /* get the Y position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Y (default 0)" );
        return(TRUE);
    }
    VPParams.InputRectangle.PositionY = Lvar;

    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Width (default 640)" );
        return(TRUE);
    }
    VPParams.InputRectangle.Width = Lvar;

    /* get the Height input rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Height (default 480)" );
        return(TRUE);
    }
    VPParams.InputRectangle.Height = Lvar;

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output X (default 0)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.PositionX = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Y (default 0)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.PositionY = Lvar;

    /* get the Width output rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Width (default 640)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Height (default 480)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.Height = Lvar;

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected source num" );
        return(TRUE);
    }
    VideoStream.SourceNumber =  Lvar;

    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected bitmap width" );
        return(TRUE);
    }

    VideoStream.BitmapParams.Width = Lvar;

    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected bitmap height" );
        return(TRUE);
    }
    VideoStream.BitmapParams.Height = Lvar;

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected source address" );
        return(TRUE);
    }
    VideoStream.BitmapParams.Data1_p = (void *)Lvar;

    VideoStream.BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
    VideoStream.BitmapParams.PreMultipliedColor = FALSE;
    VideoStream.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    VideoStream.BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
    VideoStream.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    VideoStream.BitmapParams.Pitch = VideoStream.BitmapParams.Width;
    VideoStream.BitmapParams.Offset = 0;
    VideoStream.BitmapParams.Size1 = VideoStream.BitmapParams.Width *
        VideoStream.BitmapParams.Height;
    VideoStream.BitmapParams.Size2 = VideoStream.BitmapParams.Width *
        VideoStream.BitmapParams.Height / 2;
    VideoStream.BitmapParams.Data2_p =
        (void *)((U32)VideoStream.BitmapParams.Data1_p +
                 VideoStream.BitmapParams.Size1);
    VideoStream.BitmapParams.SubByteFormat =
        STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
    VideoStream.ScanType = STGXOBJ_INTERLACED_SCAN;
    VideoStream.CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
    VideoStream.HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
    VideoStream.VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
    source.SourceType = STLAYER_STREAMING_VIDEO;
    source.Data.VideoStream_p = &VideoStream;
    source.Palette_p = NULL;
    VPParams.Source_p = &source;

    sprintf(LAY_Msg, "STLAYER_AdjustViewPortParams(%d,InX=%d,InY=%d,InW=%d,"
            "InH=%d,OuX=%d,OuH=%d,OuH=%d,OuW=%d): ",
            LayHndl,
            VPParams.InputRectangle.PositionX,
            VPParams.InputRectangle.PositionY,
            VPParams.InputRectangle.Width,
            VPParams.InputRectangle.Height,
            VPParams.OutputRectangle.PositionX,
            VPParams.OutputRectangle.PositionY,
            VPParams.OutputRectangle.Width,
            VPParams.OutputRectangle.Height
        );
    ErrCode = STLAYER_AdjustViewPortParams ( LayHndl, &VPParams);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_AttachAlphaViewPort
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_AttachAlphaViewPort( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    ST_ErrorCode_t ErrCode;
    STLAYER_Handle_t HdlMaskedLayer;

    UNUSED_PARAMETER(result_sym_p);
    /* Get the Layer Handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VP Handle" );
        return(TRUE);
    }

    /* Get the Layer Handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&HdlMaskedLayer);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Handle" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_AttachAlphaViewPort(%d,%d): ",LayHndl,
                        HdlMaskedLayer);
    ErrCode = STLAYER_AttachAlphaViewPort(VPHndl, HdlMaskedLayer);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end LAYER_Close */


/*-----------------------------------------------------------------------------
 * Function : LAYER_Close
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    ST_ErrorCode_t ErrCode;

    UNUSED_PARAMETER(result_sym_p);
    /* Get the Layer Handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Handle" );
        return(TRUE);
    }
    sprintf(LAY_Msg, "STLAYER_Close(%d): ", LayHndl);
    ErrCode = STLAYER_Close(LayHndl);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end LAYER_Close */


/*-----------------------------------------------------------------------------
 * Function : LAYER_CloseViewPort
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_CloseViewPort (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    STLAYER_ViewPortSource_t VPSource;
#ifndef ST_OSLINUX
    STAVMEM_FreeBlockParams_t FreeBlockParams;
#endif
    LAY_Msg[0]='\0';

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_CloseViewPort(%d): ", VPHndl);

    ErrCode = STLAYER_CloseViewPort( VPHndl ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {

               /* Free allocated block if any */
        for(Lvar=0; (Lvar<MAX_ALLOCATED_BLOCK)
                && (AllocatedBlockTable[Lvar].VPHndl!= VPHndl); Lvar++);
        if(Lvar != MAX_ALLOCATED_BLOCK)
        {
#ifdef ST_OSLINUX
            if(AllocatedBlockTable[Lvar].BitmapAddress_p != NULL )
            {
                ErrCode=STLAYER_FreeData ( LayHndl, AllocatedBlockTable[Lvar].BitmapAddress_p );
            }
            if(AllocatedBlockTable[Lvar].PaletteAddress_p != 0)
            {
                ErrCode|=STLAYER_FreeData ( LayHndl, AllocatedBlockTable[Lvar].PaletteAddress_p );
            }
#else
            FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
            if(AllocatedBlockTable[Lvar].BitmapHndl != 0)
            {
                ErrCode=STAVMEM_FreeBlock (&FreeBlockParams,
                        &AllocatedBlockTable[Lvar].BitmapHndl);
            }
            if(AllocatedBlockTable[Lvar].PaletteHndl != 0)
            {
                ErrCode|=STAVMEM_FreeBlock (&FreeBlockParams,
                        &AllocatedBlockTable[Lvar].PaletteHndl);
            }
#endif

            if(ErrCode != ST_NO_ERROR)
            {
                sprintf(LAY_Msg, "Problem while freeing memory !!!\n" );
                LAYER_PRINT(LAY_Msg);
            }
            AllocatedBlockTable[Lvar].VPHndl=0;
        }
    }
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_DisableViewPort
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_DisableViewPort (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;


    UNUSED_PARAMETER(result_sym_p);
    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected handle" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_DisableViewPort(%d): ", VPHndl);
    ErrCode = STLAYER_DisableViewPort( VPHndl ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_EnableViewPort
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_EnableViewPort (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    UNUSED_PARAMETER(result_sym_p);
    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected handle" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_EnableViewPort(%d): ", VPHndl);
    ErrCode = STLAYER_EnableViewPort( VPHndl ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_Init
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    STLAYER_InitParams_t InitParams;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    S32 Lvar, DevNb;
    char TrvStr[80], IsStr[80], *ptr;
#if defined (ST_7020) && defined (ST_5528)
    S32 Chip;
#endif

   UNUSED_PARAMETER(result_sym_p);
   /* Get device */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME, DeviceName,
                              sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    /* get LayerType */
    STTST_GetItem( pars_p, LAYER_DEVICE_TYPE, TrvStr, sizeof(TrvStr));;
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!(RetErr))
    {
        strcpy(TrvStr, IsStr);
    }
    for(DevNb=0; DevNb<MAX_LAYER_TYPE; DevNb++)
    {
        if((strcmp(LayerType[DevNb].String, TrvStr) == 0 ) ||
           (strncmp(LayerType[DevNb].String, TrvStr+1,
                    strlen(LayerType[DevNb].String)) == 0 ))
            break;
    }
    if (DevNb == MAX_LAYER_TYPE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, &DevNb, 10);
        if(RetErr)
        {
            DevNb = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                API_TagExpected(LayerType, MAX_LAYER_TYPE, LAYER_DEVICE_TYPE);
                STTST_TagCurrentLine(pars_p, LAY_Msg);
                return(TRUE);
            }
        }
        InitParams.LayerType = DevNb;

        for (DevNb=0; DevNb<MAX_LAYER_TYPE; DevNb++)
        {
            if(LayerType[DevNb].Value == InitParams.LayerType)
                break;
        }
    }
    else
    {
        InitParams.LayerType = LayerType[DevNb].Value;
    }

    switch (InitParams.LayerType)
    {
        case STLAYER_COMPOSITOR :
            break;
        case STLAYER_OMEGA2_VIDEO1 :  /* OMEGA2_VIDEO1 */
#if defined (ST_7015)
            InitParams.BaseAddress_p = (void*)(ST7015_GAMMA_OFFSET
                                            + ST7015_GAMMA_VID1_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7015_DISPLAY_OFFSET;
#endif
            break;
        case STLAYER_7020_VIDEO1:
#if defined (ST_7020)
            InitParams.BaseAddress_p = (void*)(ST7020_GAMMA_OFFSET
                                            + ST7020_GAMMA_VID1_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7020_DISPLAY_OFFSET;
            InitParams.VideoDisplayInterrupt = ST7020_DIS_INT;
            strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
#endif
            break;
        case STLAYER_OMEGA2_VIDEO2 :  /* OMEGA2_VIDEO2 */
#if defined (ST_7015)
            InitParams.BaseAddress_p = (void*)(ST7015_GAMMA_OFFSET
                                            + ST7015_GAMMA_VID2_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7015_PIP_DISPLAY_OFFSET;
#endif
            break;
        case STLAYER_7020_VIDEO2 :
#if defined (ST_7020)
            InitParams.BaseAddress_p = (void*)(ST7020_GAMMA_OFFSET
                                            + ST7020_GAMMA_VID2_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7020_PIP_DISPLAY_OFFSET;
            InitParams.VideoDisplayInterrupt = ST7020_DIP_INT;
            strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
#endif
            break;
        case  STLAYER_SDDISPO2_VIDEO1 :
#if defined (ST_5528)
            InitParams.BaseAddress_p = (void *)(ST5528_VID1_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST5528_DISP0_BASE_ADDRESS);
#endif
            break;
        case  STLAYER_HDDISPO2_VIDEO1 :
#if defined (ST_7710)
            InitParams.BaseAddress_p = (void *)(ST7710_VID1_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7710_DISP0_BASE_ADDRESS);
#elif defined (ST_7100)
            InitParams.BaseAddress_p = (void *)(ST7100_VID1_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7100_DISP0_BASE_ADDRESS);



#endif
            break;
        case  STLAYER_SDDISPO2_VIDEO2 :
#if defined (ST_5528)
            InitParams.BaseAddress_p = (void *)(ST5528_VID2_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST5528_DISP1_BASE_ADDRESS);
#endif
            break;
       case  STLAYER_HDDISPO2_VIDEO2 :
#if defined (ST_7710)
            InitParams.BaseAddress_p = (void *)(ST7710_VID2_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7710_DISP1_BASE_ADDRESS);
#elif defined (ST_7100)
            InitParams.BaseAddress_p = (void *)(ST7100_VID2_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7100_DISP1_BASE_ADDRESS);
#elif defined (ST_7109)
            InitParams.BaseAddress_p = (void *)(ST7109_VID2_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7109_DISP1_BASE_ADDRESS);
#endif
            break;
		case STLAYER_DISPLAYPIPE_VIDEO1 :
#if defined (ST_7109)
            InitParams.BaseAddress_p = (void *)(ST7109_VID1_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7109_DISP0_BASE_ADDRESS);
#endif  /* ST_7109 */
            break;
        case STLAYER_DISPLAYPIPE_VIDEO2 :
#if defined (ST_7200)
            InitParams.BaseAddress_p = (void *)(ST7200_VID1_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7200_VDP_MAIN_BASE_ADDRESS);
#endif
           break;
           case STLAYER_DISPLAYPIPE_VIDEO3 :
#if defined (ST_7200)
            InitParams.BaseAddress_p = (void *)(ST7200_VID2_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7200_VDP_AUX1_BASE_ADDRESS);
#endif
           break;

        case STLAYER_DISPLAYPIPE_VIDEO4 :
#if defined (ST_7200)
            InitParams.BaseAddress_p = (void *)(ST7200_VID3_LAYER_BASE_ADDRESS);
            InitParams.BaseAddress2_p =  (void *)(ST7200_VDP_AUX2_BASE_ADDRESS);
#endif
           break;

        case STLAYER_GAMMA_CURSOR :  /* GAMMA_CURSOR, */
        case STLAYER_GAMMA_GDP :     /* GAMMA_GDP,    */
        case STLAYER_GAMMA_GDPVBI :     /* GAMMA_GDP,    */
        case STLAYER_GAMMA_BKL :     /* GAMMA_BKL,    */
        case STLAYER_OMEGA1_CURSOR : /* OMEGA1_CURSOR */
        case STLAYER_OMEGA1_VIDEO :  /* OMEGA1_VIDEO, */
        case STLAYER_OMEGA1_OSD :    /* OMEGA1_OSD,   */
        case STLAYER_OMEGA1_STILL :  /* OMEGA1_STILL  */
        case STLAYER_GAMMA_ALPHA :   /* GAMMA_ALPHA, */
        case STLAYER_GAMMA_FILTER :   /* GAMMA_FILTER, */
            /* Get base address */
            RetErr = STTST_GetInteger( pars_p, LAYER_BASE_ADDRESS, &Lvar);
            if (RetErr){
                STTST_TagCurrentLine( pars_p,
                                        "Expected base address of layer type");
                return(TRUE);
            }
            InitParams.BaseAddress_p = (void*)Lvar;
            InitParams.BaseAddress2_p = (void*)0;

            /* PATCH */
            if (InitParams.LayerType == STLAYER_OMEGA1_CURSOR)
            {
#ifdef ST_5518
                InitParams.BaseAddress_p = (void*)0x400;
#else
                InitParams.BaseAddress_p = (void*)0x1000;
#endif
            }
            break;
        default:
            InitParams.BaseAddress_p = (void*)0;
            InitParams.BaseAddress2_p = (void*)0;
            break;
    }

    /* Get Width */
    RetErr = STTST_GetInteger( pars_p, LAYER_DEFAULT_WIDTH, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    LayerParams.Width = (U32)Lvar;

    /* Get Height */
    RetErr = STTST_GetInteger( pars_p, LAYER_DEFAULT_HEIGHT, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Height" );
        return(TRUE);
    }
    LayerParams.Height = (U32)Lvar;

    /* Get Scan Type */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type (default Interlaced)" );
        return(TRUE);
    }
    LayerParams.ScanType = (U32)Lvar;

    /* Get Aspect Ratio */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio (default 4:3)" );
        return(TRUE);
    }
    LayerParams.AspectRatio = (U32)Lvar;

    /* get MaxViewPorts */
    RetErr = STTST_GetInteger( pars_p, LAYER_MAX_VIEWPORT, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxViewPorts" );
        return(TRUE);
    }
    InitParams.MaxViewPorts = (U32)Lvar;
#if defined (ST_7020) && defined (ST_5528)
    /* Get chip */
    RetErr = STTST_GetInteger( pars_p, 7020, &Chip);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Current chip: 7020, 5528, etc." );
        return(TRUE);
    }
    if (Chip == 5528 )
    {
        InitParams.DeviceBaseAddress_p = (void *)(LAYER_5528_DEVICE_BASE_ADDRESS);
    }
    else
#endif
    {
        InitParams.DeviceBaseAddress_p = (void *)(LAYER_DEVICE_BASE_ADDRESS);
    }
    InitParams.CPUPartition_p            = SystemPartition_p;
#ifdef ST_XVP_ENABLE_FLEXVP
    InitParams.NCachePartition_p         = NCachePartition_p;
#endif /* ST_XVP_ENABLE_FLEXVP */
    InitParams.MaxHandles                = LAYER_MAX_OPEN_HANDLES;
#if defined (ST_7020) && defined (ST_5528)
    if (Chip == 5528 )
    {
        InitParams.AVMEM_Partition           = AvmemPartitionHandle[1];
    }
    else
#endif
#ifdef ST_OSLINUX
    {
        InitParams.AVMEM_Partition           = (STAVMEM_PartitionHandle_t)NULL;
    }
#else /* !ST_OSLINUX */
    #if defined(DVD_SECURED_CHIP) && defined (ST_7109)
        #if defined(ST_OS21) && !defined(ST_OSWINCE)
            {
                /* Secured/7109/OS21 => provide the reserved partiton handle */
                InitParams.AVMEM_Partition           = AvmemPartitionHandle[3];
            }
        #else /* defined(ST_OS21) && !defined(ST_OSWINCE) */
            {
                /* Secured/7109/WinCE => provide the reserved partiton handle */
                InitParams.AVMEM_Partition           = AvmemPartitionHandle[3];
            }
        #endif /* defined(ST_OS21) && !defined(ST_OSWINCE) */
    #else /* defined(DVD_SECURED_CHIP) && defined (ST_7109) */
        /* Non-secure platforms */
        {
            InitParams.AVMEM_Partition           = AvmemPartitionHandle[0];
        }
#endif /* !defined(DVD_SECURED_CHIP) || !defined (ST_7109) */
#endif /* !ST_OSLINUX */


    InitParams.CPUBigEndian              = FALSE;
    InitParams.ViewPortNodeBuffer_p      = NULL;
    InitParams.ViewPortBuffer_p          = NULL;
    InitParams.ViewPortBufferUserAllocated  = FALSE;
    InitParams.NodeBufferUserAllocated      = FALSE;
#ifdef ST_OSLINUX                       /* Not used */
    InitParams.SharedMemoryBaseAddress_p = (void *)NULL;
#else

#if defined (ST_7020) && defined (ST_5528)
    if (Chip == 5528 )
    {
        InitParams.SharedMemoryBaseAddress_p = (void *)MB376_SDRAM_BASE_ADDRESS;
    }
    else
#endif
    {
        InitParams.SharedMemoryBaseAddress_p = (void *)SDRAM_BASE_ADDRESS;
    }
#endif
    strcpy(InitParams.EventHandlerName, STEVT_DEVICE_NAME);
#ifdef mb295
    InitParams.SharedMemoryBaseAddress_p = (void *)SDRAM_WINDOW_BASE_ADDRESS;
#endif
    InitParams.LayerParams_p = &LayerParams;


    sprintf(LAY_Msg, "STLAYER_Init(%s,Type=%s): ",
            DeviceName,
            (DevNb<MAX_LAYER_TYPE) ? LayerType[DevNb].String : "????" );
    ErrCode = STLAYER_Init( DeviceName, &InitParams ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    sprintf(LAY_Msg, "\tAdd=%x, Add2=%x, Devbaseadd=%x\n",
            (U32)InitParams.BaseAddress_p, (U32)InitParams.BaseAddress2_p,
            (U32)InitParams.DeviceBaseAddress_p);
#ifdef ST_OSLINUX
    sprintf(LAY_Msg, "%s\tcpup=%x, shmem=%x, evt='%s'\n",
            LAY_Msg,
            (U32)InitParams.SharedMemoryBaseAddress_p,
            (U32)InitParams.AVMEM_Partition, InitParams.EventHandlerName);
#else
    sprintf(LAY_Msg, "%s\tcpup=%x, shmem=%x, evt='%s'\n",
            LAY_Msg,
            (U32)InitParams.SharedMemoryBaseAddress_p,
            (U32)InitParams.AVMEM_Partition, InitParams.EventHandlerName);
#endif
    sprintf(LAY_Msg, "%s\tnodebuf=%d, vpnode=%x, vpbufuse=%d, vpbuf=%x\n",
            LAY_Msg,
            InitParams.NodeBufferUserAllocated,
            (U32)InitParams.ViewPortNodeBuffer_p,
            InitParams.ViewPortBufferUserAllocated,
            (U32)InitParams.ViewPortBuffer_p);
    sprintf(LAY_Msg, "%s\tmaxhd=%d, maxvp=%d\n",
            LAY_Msg,
            InitParams.MaxHandles, InitParams.MaxViewPorts);
    sprintf(LAY_Msg, "%s\tWid=%d, Hei=%d, AspRat=%s, ScanT=%s\n",
            LAY_Msg,
            LayerParams.Width,
            LayerParams.Height,
            (LayerParams.AspectRatio<MAX_ASPECTRATIO_TYPE) ?
                        AspectRatioType[LayerParams.AspectRatio] : "???" ,
            (LayerParams.ScanType<MAX_SCANTYPE_TYPE) ?
                        ScanTypeType[LayerParams.ScanType] : "???"  );
    LAYER_PRINT(LAY_Msg);
    return ( API_EnableError ? RetErr : FALSE );
} /* end LAYER_Init */


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetLayerParams
 *            Get driver layer parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetLayerParams (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    char StrParams[80];


    /* Get the handle of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Viewport" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_GetLayerParams(%d): ", VPHndl);
    ErrCode = STLAYER_GetLayerParams( VPHndl, &LayerParams );
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if(ErrCode == ST_NO_ERROR)
    {
        sprintf(LAY_Msg,"\tScanType=%s AspectRatio=%s\n\tWidth=%d Height=%d\n",\
           (LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : \
           (LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" : "????",
           (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
           (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
           (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
           (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
           (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                                                                 "????",
           LayerParams.Width, LayerParams.Height);
        LAYER_PRINT(LAY_Msg);
        sprintf( StrParams, "%d %d %d %d", \
                 LayerParams.ScanType, LayerParams.AspectRatio,
                 LayerParams.Width, LayerParams.Height);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end LAYER_GetLayerParams */


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortIORectangle
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortIORectangle (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STGXOBJ_Rectangle_t InputRectangle;
    STGXOBJ_Rectangle_t OutputRectangle;
    char StrParams[RETURN_PARAMS_LENGTH];

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf( LAY_Msg, "STLAYER_GetViewPortIORectangle(%d): ", VPHndl );
    ErrCode = STLAYER_GetViewPortIORectangle ( VPHndl , &InputRectangle,
            &OutputRectangle ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( LAY_Msg, "\tInX0=%d InY0=%d InWidth=%d InHeight=%d\n",
                    InputRectangle.PositionX, InputRectangle.PositionY,
                    InputRectangle.Width, InputRectangle.Height);
        sprintf( LAY_Msg, "%s\tOutX0=%d OutY0=%d OutWidth=%d OutHeight=%d\n",
                    LAY_Msg,OutputRectangle.PositionX,OutputRectangle.PositionY,
                    OutputRectangle.Width, OutputRectangle.Height);
        LAYER_PRINT(LAY_Msg);

        sprintf(StrParams, "%d %d %d %d %d %d %d %d",
                InputRectangle.PositionX, InputRectangle.PositionY,
                InputRectangle.Width, InputRectangle.Height,
                OutputRectangle.PositionX, OutputRectangle.PositionY,
                OutputRectangle.Width, OutputRectangle.Height);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_Open
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Open (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_OpenParams_t OpenParams;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;

    /* get device name */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME,
            DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
        return (TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_Open(%s): ", DeviceName);
    ErrCode = STLAYER_Open( DeviceName, &OpenParams, &LayHndl) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( LAY_Msg, "\thnd=%d\n", LayHndl);
        LAYER_PRINT(LAY_Msg);
        STTST_AssignInteger ( result_sym_p, (S32)LayHndl, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_OpenViewPort
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_OpenViewPort (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_ViewPortParams_t    VPParam;
    ST_ErrorCode_t ErrCode;
    STLAYER_ViewPortSource_t *   source_p;
#ifndef ST_OSLINUX
    STAVMEM_BlockHandle_t       BitmapHandle=0;
    STAVMEM_BlockHandle_t       PaletteHandle=0;
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#else
    void  *BitmapAddress_p=0;
    void  *PaletteAddress_p=0;
#endif
    STLAYER_StreamingVideo_t    VideoStream;
    S32 Lvar, Lvar2;

#ifdef ST_OSLINUX
	unsigned long i;
#endif

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5517)\
 || defined(ST_5516) || defined(ST_5528) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)|| defined (ST_5100)\
 || defined (ST_5105) ||  defined (ST_5301) || defined (ST_5188)|| defined (ST_5525) || defined(ST_5107) || defined(ST_5162)

    char * FileName;
#endif /* ST_7015 ST_7020 ST_GX1 or ST_5517/ST_5516 for RGB16 */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) \
|| defined(ST_5516) || ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))
    STAVMEM_AllocBlockParams_t  AllocParams;
    int i;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516) (ST_5517)*/

#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5517)\
 || defined(ST_5516) || defined(ST_5528) || defined(ST_7710)  || defined(ST_7100) || defined(ST_7109)|| defined(ST_7200)|| defined(ST_5100)\
 || defined(ST_5105) || defined(ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined(ST_5107) || defined(ST_5162)

#ifdef ST_OSLINUX
    sprintf(StapigatDataPath, "./scripts/data/");
#endif

    FileName=STOS_MemoryAllocate(SystemPartition_p,strlen(StapigatDataPath)+20);
#endif /* ST_7015 ST_7020 ST_GX1 or ST_5517/ST_5516 for RGB16 */

    /* get the layer handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected handle" );
        return(TRUE);
    }
    source_p=STOS_MemoryAllocate(SystemPartition_p, sizeof(STLAYER_ViewPortSource_t));

    /* Default Inits */
    VPParam.Source_p = source_p;
    source_p->SourceType = STLAYER_GRAPHIC_BITMAP;

    RetErr = STTST_GetInteger( pars_p, 2, &Lvar );
    if ( RetErr )
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105)|| defined(ST_5107)|| defined(ST_5301) \
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)

        STTST_TagCurrentLine( pars_p, "expected Source 0=for video  1=facecursor "
                              "2=merou 3=susie 4=format 5=AnyFile" );
#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5508) \
 || defined(ST_5518) || defined(ST_5514) || defined (ST_5516) || defined(ST_5517)
        STTST_TagCurrentLine( pars_p,"expected Source 1=Cursor 2=OSD 3=Still Top/Bot 4=OSDinRGB16(5516/5517 only) 5=Still Prog" );
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516) (ST_5517) */
        return(TRUE);
    }
    if(Lvar == 0)
    {
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected source num" );
            return(TRUE);
        }
        VideoStream.SourceNumber =  Lvar2;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected width" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Width = Lvar2;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected height" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Height = Lvar2;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected source address" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Data1_p = (void *)Lvar2;
#ifdef ST_OSLINUX
        printf(" VideoStream.BitmapParams.Data1_p : 0x%x\n", (U32)VideoStream.BitmapParams.Data1_p );
#endif
        VideoStream.BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
        VideoStream.BitmapParams.PreMultipliedColor = FALSE;
        VideoStream.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
        VideoStream.BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
        VideoStream.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
        VideoStream.BitmapParams.Pitch = VideoStream.BitmapParams.Width;
        VideoStream.BitmapParams.Offset = 0;
        VideoStream.BitmapParams.Size1 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height;
        VideoStream.BitmapParams.Size2 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height / 2;
        VideoStream.BitmapParams.Data2_p =
            (void *)((U32)VideoStream.BitmapParams.Data1_p +
                     VideoStream.BitmapParams.Size1);
#ifdef ST_OSLINUX
        printf("OVP: Data1:%x\nData2:%x\n",(U32)VideoStream.BitmapParams.Data1_p,(U32)VideoStream.BitmapParams.Data2_p);
#endif
        VideoStream.BitmapParams.SubByteFormat =
            STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
        VideoStream.ScanType = STGXOBJ_INTERLACED_SCAN;
        VideoStream.CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
        VideoStream.HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
        VideoStream.VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;

        source_p->SourceType = STLAYER_STREAMING_VIDEO;
        source_p->Data.VideoStream_p = &VideoStream;
        source_p->Palette_p = NULL;

        VPParam.InputRectangle.PositionX    = 0;
        VPParam.InputRectangle.PositionY    = 0;
        VPParam.InputRectangle.Width        = VideoStream.BitmapParams.Width;
        VPParam.InputRectangle.Height       = VideoStream.BitmapParams.Height;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
    }

    if(Lvar ==1)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)  || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105)|| defined(ST_5107) || defined (ST_5301)\
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)


                sprintf(FileName, "%sface_128x128.gam", StapigatDataPath);


#ifdef ST_OSLINUX
        GUTIL_LoadBitmap1(FileName,&Bitmapface,
                        &FacePalette);
#else
        GUTIL_LoadBitmap(FileName,&Bitmapface,
                        &FacePalette,&BitmapHandle,&PaletteHandle);
#endif
        VPParam.InputRectangle.PositionX    = 0;
        VPParam.InputRectangle.PositionY    = 0;
        VPParam.InputRectangle.Width        = Bitmapface.Width;
        VPParam.InputRectangle.Height       = Bitmapface.Height;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
        source_p->Palette_p                    = &FacePalette;
        source_p->Data.BitMap_p                = &Bitmapface;
#ifdef ST_OSLINUX
        BitmapAddress_p = Bitmapface.Data1_p;
        PaletteAddress_p = FacePalette.Data_p;
#endif
#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5508) \
 || defined(ST_5518) || defined(ST_5514) || defined (ST_5516)\
 || defined(ST_5517)
        BitmapCursor.ColorType              = STGXOBJ_COLOR_TYPE_CLUT2;
        BitmapCursor.PreMultipliedColor     = FALSE;
        BitmapCursor.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        BitmapCursor.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        BitmapCursor.Width                  = 40;
        BitmapCursor.Height                 = 40;
        BitmapCursor.Pitch                  = 10;
        BitmapCursor.Data1_p                = (void*)Cursor_Bitmap;
        PaletteCursor.ColorType             = STGXOBJ_COLOR_TYPE_ARGB8888;
        PaletteCursor.ColorDepth            = 2;
        PaletteCursor.Data_p                = (void*)Cursor_Color;
        source_p->Data.BitMap_p                = &BitmapCursor;
        source_p->Palette_p                    = &PaletteCursor;
        VPParam.InputRectangle.PositionX    = 0;
        VPParam.InputRectangle.PositionY    = 0;
        VPParam.InputRectangle.Width        = 40;
        VPParam.InputRectangle.Height       = 40;
        VPParam.OutputRectangle.PositionX   = 200;
        VPParam.OutputRectangle.PositionY   = 200;
        VPParam.OutputRectangle.Width       = 40;
        VPParam.OutputRectangle.Height      = 40;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516)*/

    }
    if(Lvar ==2)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)  || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100)|| defined(ST_5105) || defined(ST_5107) || defined(ST_5301) \
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)

        sprintf(FileName, "%smerou3.gam", StapigatDataPath);
#ifdef ST_OSLINUX
        GUTIL_LoadBitmap1(FileName,&Bitmapmerou,0);
#else
        GUTIL_LoadBitmap(FileName,&Bitmapmerou,0,
                &BitmapHandle,0);
#endif
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width =  Bitmapmerou.Width;
        VPParam.InputRectangle.Height = Bitmapmerou.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source_p->Palette_p = 0;
        source_p->Data.BitMap_p = &Bitmapmerou;
#ifdef ST_OSLINUX
        BitmapAddress_p = Bitmapmerou.Data1_p;
#endif
#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518)\
 || defined(ST_5514) || defined (ST_5516)\
 || defined(ST_5517)
        /* Creation Palettes */
        PaletteOSD.ColorType        =STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
        PaletteOSD.ColorDepth       = 8;
        STLAYER_GetPaletteAllocParams(LayHndl,&PaletteOSD,&PaletteOSDParams);
        AllocParams.PartitionHandle = AvmemPartitionHandle[0];
        AllocParams.Size            = PaletteOSDParams.AllocBlockParams.Size;
        AllocParams.Alignment      =PaletteOSDParams.AllocBlockParams.Alignment;
        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *)0;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.ForbiddenBorderArray_p = (void **)0;
        STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                            &AllocParams,
                            (STAVMEM_BlockHandle_t *) &(PaletteHandle));
        STAVMEM_GetBlockAddress( PaletteHandle, (void **)&(PaletteOSD.Data_p));

        /* Palettes fill */
        /*---------------*/
        for (i=0; i<256; i++)
        {
            STGXOBJ_SetPaletteColor(&PaletteOSD,i,&(OSD_Color[i]));
        }
        /* Creation Bitmap */
        BitmapOSD.ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
        BitmapOSD.BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        BitmapOSD.Width = 136;
        BitmapOSD.Height = 210;
        STLAYER_GetBitmapAllocParams(LayHndl,&BitmapOSD,&BitmapOSDParams1,
                &BitmapOSDParams2);
        AllocParams.Size = BitmapOSDParams1.AllocBlockParams.Size;
        AllocParams.Alignment = BitmapOSDParams1.AllocBlockParams.Alignment;
        BitmapOSD.Pitch = BitmapOSDParams1.Pitch;
        BitmapOSD.Offset = BitmapOSDParams1.Offset;
        STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                            &AllocParams,(STAVMEM_BlockHandle_t *)
                            &(BitmapHandle));
        STAVMEM_GetBlockAddress( BitmapHandle, (void **)&(BitmapOSD.Data1_p));

        /* fill bitmap */
        /*-------------*/
        STAVMEM_CopyBlock2D ( (void *) &(OSD_Bitmap[0]),
                              BitmapOSD.Width,
                              BitmapOSD.Height,
                              BitmapOSD.Width,
                              (void *)((U32)(BitmapOSD.Data1_p)
                                       +BitmapOSD.Offset),
                              BitmapOSD.Pitch );
        VPParam.InputRectangle.PositionX    = 0;
        VPParam.InputRectangle.PositionY    = 0;
        VPParam.InputRectangle.Width        = BitmapOSD.Width;
        VPParam.InputRectangle.Height       = BitmapOSD.Height;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
        VPParam.OutputRectangle.PositionY   = 200;
        VPParam.OutputRectangle.PositionX   = 200;
        source_p->Palette_p                    = &PaletteOSD;
        source_p->Data.BitMap_p                = &BitmapOSD;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516) */

    }
    if(Lvar ==3)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)  || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105)|| defined(ST_5107) || defined(ST_5301) \
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)

        sprintf(FileName, "%ssuzie.gam", StapigatDataPath);
#ifdef ST_OSLINUX
        GUTIL_LoadBitmap1(FileName,&Bitmapeye,0);
#else
        GUTIL_LoadBitmap(FileName,&Bitmapeye,0,
                &BitmapHandle,0);
#endif
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapeye.Width;
        VPParam.InputRectangle.Height = Bitmapeye.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source_p->Palette_p = 0;
        source_p->Data.BitMap_p = &Bitmapeye;

#ifdef ST_OSLINUX
        BitmapAddress_p = Bitmapeye.Data1_p ;
#endif

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5508) \
 || defined(ST_5518) || defined(ST_5514) || defined (ST_5516)\
 || defined(ST_5517)
        GetBitmap(&BitmapStill, &BitmapHandle);
        VPParam.InputRectangle.PositionX    = 50;
        VPParam.InputRectangle.PositionY    = 50;
        VPParam.InputRectangle.Width        = 250;
        VPParam.InputRectangle.Height       = 250;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
        VPParam.OutputRectangle.PositionY   = 0;
        VPParam.OutputRectangle.PositionX   = 0;
        source_p->Palette_p                    = 0;
        source_p->Data.BitMap_p                = &BitmapStill;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516)*/
    }
    if(Lvar ==4)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)  || defined(ST_5528) || defined(ST_7710) || defined(ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105)|| defined(ST_5107) || defined(ST_5301)\
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)

        sprintf(FileName, "%sformat.gam", StapigatDataPath);
#ifdef ST_OSLINUX
        GUTIL_LoadBitmap1(FileName,&BitmapFormat,0);
#else
        GUTIL_LoadBitmap(FileName,&BitmapFormat,
                0,&BitmapHandle,0);
#endif
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = BitmapFormat.Width;
        VPParam.InputRectangle.Height = BitmapFormat.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source_p->Palette_p = 0;
        source_p->Data.BitMap_p = &BitmapFormat;
#ifdef ST_OSLINUX
        BitmapAddress_p = BitmapFormat.Data1_p ;
#endif

#elif defined(ST_5517)  ||  defined(ST_5516)
        sprintf(FileName, "../../../scripts/rgb565.gam");
        GUTIL_LoadBitmap(FileName,&Bitmapmerou,0,
                &BitmapHandle,0);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapmerou.Width;
        VPParam.InputRectangle.Height = Bitmapmerou.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source_p->Palette_p = 0;
        source_p->Data.BitMap_p = &Bitmapmerou;
#endif
    }
    if(Lvar ==5)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)  || defined(ST_5528) || defined (ST_7710) || defined (ST_7100)\
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5301)\
 || defined(ST_5188) || defined(ST_5525) || defined(ST_5162)

        STGXOBJ_Palette_t Palette;

#ifdef STLAYER_CONVERT_PALETTE_TO_YCBCR888
        STGXOBJ_Palette_t Palette2;
        STGXOBJ_ColorSpaceConversionMode_t ConvMode = STGXOBJ_ITU_R_BT601;
#endif

#ifdef STLAYER_CONVERT_PALETTE_TO_AYCBCR8888
        STGXOBJ_Palette_t Palette2;
        STGXOBJ_ColorSpaceConversionMode_t ConvMode = STGXOBJ_ITU_R_BT601;
#endif

        memset( FileName, 0, sizeof(FileName));
        RetErr = STTST_GetString( pars_p, "", FileName, strlen(StapigatDataPath)+20);
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected FileName" );
            return (TRUE);
        }

#ifdef ST_OSLINUX
        GUTIL_LoadBitmap1(FileName,&BitmapUnknown,&Palette);
#else
        GUTIL_LoadBitmap(FileName,&BitmapUnknown,&Palette,&BitmapHandle,&PaletteHandle);
#endif

#ifdef STLAYER_CONVERT_PALETTE_TO_YCBCR888
    switch (BitmapUnknown.ColorType )
    {
            case STGXOBJ_COLOR_TYPE_CLUT8:
            case STGXOBJ_COLOR_TYPE_CLUT4:
            case STGXOBJ_COLOR_TYPE_CLUT2:
            case STGXOBJ_COLOR_TYPE_CLUT1:
            case STGXOBJ_COLOR_TYPE_ACLUT88:
            case STGXOBJ_COLOR_TYPE_ACLUT44:
            STTBX_Print(("--> Converting src palette to YCbCr888 \n"));

            RetErr = GUTIL_GetYCbCr888Palette(&Palette2);
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "error in YCbCr888 Palette creation " );
                return (TRUE);
            }

            RetErr = STGXOBJ_ConvertPalette(&Palette,&Palette2,ConvMode);
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "error in YCbCr888 Palette conversion " );
                return (TRUE);
            }

            Palette = Palette2;
            break;

            default:
                STTBX_Print(("--> Bitmap color type is not a CLUT: Palette not converted to YCbCr888\n"));
                break;
    }
#endif

#ifdef STLAYER_CONVERT_PALETTE_TO_AYCBCR8888
    switch (BitmapUnknown.ColorType )
    {
            case STGXOBJ_COLOR_TYPE_CLUT8:
            case STGXOBJ_COLOR_TYPE_CLUT4:
            case STGXOBJ_COLOR_TYPE_CLUT2:
            case STGXOBJ_COLOR_TYPE_CLUT1:
            case STGXOBJ_COLOR_TYPE_ACLUT88:
            case STGXOBJ_COLOR_TYPE_ACLUT44:
            STTBX_Print(("--> Converting src palette to AYCbCr8888 \n"));

            RetErr = GUTIL_GetAYCbCr8888Palette(&Palette2);
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "error in AYCbCr8888 Palette creation " );
                return (TRUE);
            }

            /* Set the same color depth param */
            Palette2.ColorDepth = Palette.ColorDepth;

            RetErr = STGXOBJ_ConvertPalette(&Palette,&Palette2,ConvMode);
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "error in AYCbCr8888 Palette conversion " );
                return (TRUE);
            }

            Palette = Palette2;
            break;

            default:
                STTBX_Print(("--> Bitmap color type is not a CLUT: Palette not converted to AYCbCr8888\n"));
                break;
    }
#endif

        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = BitmapUnknown.Width;
        VPParam.InputRectangle.Height = BitmapUnknown.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source_p->Palette_p = &Palette;
        source_p->Data.BitMap_p = &BitmapUnknown;

#ifdef LAYER_DEBUG
        {
            U8 Value8;
            U32 i;
            void * Addr_p;

            printf("Read User bitmap data:\n");
            for ( i=0; i<20; i++)
            {

                    Addr_p = (U8 *) (((U8 *) BitmapUnknown.Data1_p) + i);
                    Value8 =  *((U8*)Addr_p);
                    printf( "%x: %x\n ", Addr_p, Value8 );
            }
            printf("\n");
        }
#endif

#ifdef ST_OSLINUX
        BitmapAddress_p = BitmapUnknown.Data1_p ;
        PaletteAddress_p = Palette.Data_p ;
#endif


#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5508) \
 || defined(ST_5518) || defined(ST_5514) || defined (ST_5516)\
 || defined(ST_5517)
        GetBitmapProg(&BitmapStill, &BitmapHandle);
        VPParam.InputRectangle.PositionX    = 50;
        VPParam.InputRectangle.PositionY    = 50;
        VPParam.InputRectangle.Width        = 250;
        VPParam.InputRectangle.Height       = 250;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
        VPParam.OutputRectangle.PositionY   = 0;
        VPParam.OutputRectangle.PositionX   = 0;
        source_p->Palette_p                    = 0;
        source_p->Data.BitMap_p                = &BitmapStill;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) (ST_5516) (ST_5517) */
    }

    /*  Used to display dynamic video layer plane */
#if defined (ST_7109) && !defined(ST_OSLINUX)
      if(Lvar ==6)
            {



        memset( FileNameVid1, 0, sizeof(FileNameVid1));
        RetErr = STTST_GetString( pars_p, DISP_VIDEO_FILENAME, FileNameVid1, strlen(StapigatDataPath)+20);
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected FileName" );
            return (TRUE);
        }


        VIDEO_Disp() ;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected width" );
            return(TRUE);
        }

        VideoStream.BitmapParams.Width = Lvar2;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar2 );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected height" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Height = Lvar2;


        VideoStream.BitmapParams.Data1_p = (void *)LoadedVidAdd1_p;
#ifdef ST_OSLINUX
        printf(" VideoStream.BitmapParams.Data1_p : 0x%x\n", VideoStream.BitmapParams.Data1_p );
#endif
        VideoStream.BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
        VideoStream.BitmapParams.PreMultipliedColor = FALSE;
        VideoStream.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
        VideoStream.BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
        VideoStream.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
        VideoStream.BitmapParams.Pitch = VideoStream.BitmapParams.Width;
        VideoStream.BitmapParams.Offset = 0;
        VideoStream.BitmapParams.Size1 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height;
        VideoStream.BitmapParams.Size2 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height / 2;
        VideoStream.BitmapParams.Data2_p =
            (void *)((U32)VideoStream.BitmapParams.Data1_p +
                     VideoStream.BitmapParams.Size1);
#ifdef ST_OSLINUX
	    printf("OVP: Data1:%x\nData2:%x\n",VideoStream.BitmapParams.Data1_p,VideoStream.BitmapParams.Data2_p);
#endif
        VideoStream.BitmapParams.SubByteFormat =
            STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
        VideoStream.ScanType = STGXOBJ_INTERLACED_SCAN;
        VideoStream.CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
        VideoStream.HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
        VideoStream.VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;

        source_p->SourceType = STLAYER_STREAMING_VIDEO;
        source_p->Data.VideoStream_p = &VideoStream;
        source_p->Palette_p = NULL;

        VPParam.InputRectangle.PositionX    = 0;
        VPParam.InputRectangle.PositionY    = 0;
        VPParam.InputRectangle.Width        = VideoStream.BitmapParams.Width;
        VPParam.InputRectangle.Height       = VideoStream.BitmapParams.Height;
        VPParam.OutputRectangle             = VPParam.InputRectangle;


            }
#endif

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected X Position (default 0)" );
        return(TRUE);
    }
    VPParam.OutputRectangle.PositionX = Lvar;

    /* get the Y position Output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Y Position (default 0)" );
        return(TRUE);

    }
    VPParam.OutputRectangle.PositionY  = Lvar;
    source_p->Data.BitMap_p->BigNotLittle = FALSE;

    ErrCode = STLAYER_OpenViewPort( LayHndl, &VPParam, &VPHndl ) ;
    sprintf( LAY_Msg, "STLAYER_OpenViewPort(%d,X=%d,Y=%d) : ", \
             LayHndl, VPParam.OutputRectangle.PositionX,
             VPParam.OutputRectangle.PositionY);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    sprintf( LAY_Msg, "\thnd=%d\n", VPHndl);
    LAYER_PRINT(LAY_Msg);

    STTST_AssignInteger(result_sym_p, VPHndl, FALSE);

    for(Lvar=0; (Lvar<MAX_ALLOCATED_BLOCK)
            && (AllocatedBlockTable[Lvar].VPHndl!=0); Lvar++);
    if(Lvar==MAX_ALLOCATED_BLOCK)
    {
        sprintf( LAY_Msg, "Too many block allocated !!!\n" );
        LAYER_PRINT(LAY_Msg);
        return(TRUE);
    }

#ifdef ST_OSLINUX
    /* Store allocated block */
    if ( ErrCode == ST_NO_ERROR)
    {
        AllocatedBlockTable[Lvar].BitmapAddress_p = BitmapAddress_p;
        AllocatedBlockTable[Lvar].PaletteAddress_p = PaletteAddress_p;
        AllocatedBlockTable[Lvar].VPHndl = VPHndl;
    }
    else
    {
        ErrCode=STLAYER_FreeData ( LayHndl, AllocatedBlockTable[Lvar].BitmapAddress_p );
        ErrCode|=STLAYER_FreeData ( LayHndl, AllocatedBlockTable[Lvar].PaletteAddress_p );

    }
#else
    /* Store allocated block */
    if ( ErrCode == ST_NO_ERROR)
    {
        AllocatedBlockTable[Lvar].BitmapHndl = BitmapHandle;
        AllocatedBlockTable[Lvar].PaletteHndl = PaletteHandle;
        AllocatedBlockTable[Lvar].VPHndl = VPHndl;
    }
    else
    {
        FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
        ErrCode=STAVMEM_FreeBlock (&FreeBlockParams,
                        &AllocatedBlockTable[Lvar].BitmapHndl);
        ErrCode|=STAVMEM_FreeBlock (&FreeBlockParams,
                        &AllocatedBlockTable[Lvar].PaletteHndl);

    }
#endif

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetLayerParams
 *            Get driver layer parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetLayerParams (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* Get the handle of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected layer Handle" );
        return(TRUE);
    }

    /* get the Scan Type of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type" );
        return(TRUE);
    }
    LayerParams.ScanType=(STGXOBJ_ScanType_t)Lvar;

    /* get the Aspect Ratio of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio " );
        return(TRUE);
    }
    LayerParams.AspectRatio=(STGXOBJ_AspectRatio_t)Lvar;

    /* get the Width of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, 576, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    LayerParams.Width=(U32)Lvar;

    /* get the Height of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Height" );
        return(TRUE);
    }
    LayerParams.Height=(U32)Lvar;

    sprintf(LAY_Msg, "STLAYER_SetLayerParams(%d,%s,%s,W=%d,H=%d): ", \
       LayHndl,
       (LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : \
       (LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" : "????",
       (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
       (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
       (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
       (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
       (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "????"
       ,LayerParams.Width, LayerParams.Height);

    ErrCode = STLAYER_SetLayerParams( LayHndl, &LayerParams );
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end LAYER_SetLayerParams */


/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortAlpha
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortAlpha (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_GlobalAlpha_t Alpha;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Alpha 0" );
        return(TRUE);
    }
    Alpha.A0 = (U8)Lvar;

    /*get the Alpha 1 */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Alpha 1" );
        return(TRUE);
    }
    Alpha.A1 = (U8)Lvar;

    sprintf( LAY_Msg, "STLAYER_SetViewPortAlpha(%d,A0=%d,A1=%d): ",
             VPHndl, Alpha.A0, Alpha.A1);

    ErrCode = STLAYER_SetViewPortAlpha ( VPHndl , &Alpha ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortIORectangle
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortIORectangle (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STGXOBJ_Rectangle_t InputRectangle;
    STGXOBJ_Rectangle_t OutputRectangle;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort Handle" );
        return(TRUE);
    }

    /* get the X position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected X (default 0)" );
        return(TRUE);
    }
    InputRectangle.PositionX = Lvar;

    /* get the Y position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y (default 0)" );
        return(TRUE);
    }
    InputRectangle.PositionY = Lvar;

    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Width (default 640)" );
        return(TRUE);
    }
    InputRectangle.Width = Lvar;

    /* get the Height input rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Height (default 480)" );
        return(TRUE);
    }
    InputRectangle.Height = Lvar;

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected X (default 0)" );
        return(TRUE);
    }
    OutputRectangle.PositionX = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Y (default 0)" );
        return(TRUE);
    }
    OutputRectangle.PositionY = Lvar;

    /* get the Width output rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Width (default 640)" );
        return(TRUE);
    }
    OutputRectangle.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Height (default 480)" );
        return(TRUE);
    }
    OutputRectangle.Height = Lvar;

    sprintf(LAY_Msg, "STLAYER_SetViewPortIORectangle(%d,InX=%d,InY=%d,InW=%d,\
InH=%d - OutX=%d,OutY=%d,OutW=%d,OutH=%d): ",
            VPHndl,
            InputRectangle.PositionX,
            InputRectangle.PositionY,
            InputRectangle.Width,
            InputRectangle.Height,
            OutputRectangle.PositionX,
            OutputRectangle.PositionY,
            OutputRectangle.Width,
            OutputRectangle.Height
        );
    ErrCode = STLAYER_SetViewPortIORectangle ( VPHndl , &InputRectangle,
                                                &OutputRectangle ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortParams
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortParams (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_ViewPortParams_t VPParams;
    STLAYER_ViewPortSource_t source;
    STLAYER_StreamingVideo_t VideoStream;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected viewport Handle" );
        return(TRUE);
    }

    /* get the X position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input X (default 0)" );
        return(TRUE);
    }
    VPParams.InputRectangle.PositionX = Lvar;

    /* get the Y position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Y (default 0)" );
        return(TRUE);
    }
    VPParams.InputRectangle.PositionY = Lvar;

    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Width (default 640)" );
        return(TRUE);
    }
    VPParams.InputRectangle.Width = Lvar;

    /* get the Height input rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected input Height (default 480)" );
        return(TRUE);
    }
    VPParams.InputRectangle.Height = Lvar;

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output X (default 0)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.PositionX = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Y (default 0)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.PositionY = Lvar;

    /* get the Width output rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Width (default 640)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Height (default 480)" );
        return(TRUE);
    }
    VPParams.OutputRectangle.Height = Lvar;

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected source num" );
        return(TRUE);
    }
    VideoStream.SourceNumber =  Lvar;

    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected bitmap width" );
        return(TRUE);
    }

    VideoStream.BitmapParams.Width = Lvar;

    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected bitmap height" );
        return(TRUE);
    }
    VideoStream.BitmapParams.Height = Lvar;

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected source address" );
        return(TRUE);
    }
    VideoStream.BitmapParams.Data1_p = (void *)Lvar;

    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected Field Invertion Type" );
        return(TRUE);
    }

    if ( Lvar==0)
    {
        VideoStream.PresentedFieldInverted =FALSE;
    }
    else
    {
        VideoStream.PresentedFieldInverted =TRUE;
    }


    VideoStream.BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
    VideoStream.BitmapParams.PreMultipliedColor = FALSE;
    VideoStream.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    VideoStream.BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
    VideoStream.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
    VideoStream.BitmapParams.Pitch = VideoStream.BitmapParams.Width;
    VideoStream.BitmapParams.Offset = 0;
    VideoStream.BitmapParams.Size1 = VideoStream.BitmapParams.Width *
        VideoStream.BitmapParams.Height;
    VideoStream.BitmapParams.Size2 = VideoStream.BitmapParams.Width *
        VideoStream.BitmapParams.Height / 2;
    VideoStream.BitmapParams.Data2_p =
        (void *)((U32)VideoStream.BitmapParams.Data1_p +
                 VideoStream.BitmapParams.Size1);
#ifdef ST_OSLINUX
    printf("SVP: Data1:%x\nData2:%x\n",(U32)(VideoStream.BitmapParams.Data1_p),(U32)(VideoStream.BitmapParams.Data2_p));
#endif
    VideoStream.BitmapParams.SubByteFormat =
        STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
    VideoStream.ScanType = STGXOBJ_INTERLACED_SCAN;
    VideoStream.CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
    VideoStream.HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
    VideoStream.VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
    source.SourceType = STLAYER_STREAMING_VIDEO;
    source.Data.VideoStream_p = &VideoStream;
    source.Palette_p = NULL;
    VPParams.Source_p = &source;

    sprintf(LAY_Msg, "STLAYER_SetViewPortParams(%d,InX=%d,InY=%d,InW=%d,"
            "InH=%d,OuX=%d,OuH=%d,OuH=%d,OuW=%d): ",
            VPHndl,
            VPParams.InputRectangle.PositionX,
            VPParams.InputRectangle.PositionY,
            VPParams.InputRectangle.Width,
            VPParams.InputRectangle.Height,
            VPParams.OutputRectangle.PositionX,
            VPParams.OutputRectangle.PositionY,
            VPParams.OutputRectangle.Width,
            VPParams.OutputRectangle.Height
        );
    ErrCode = STLAYER_SetViewPortParams ( VPHndl, &VPParams);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortFlickerFilterMode
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortFlickerFilterMode (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;



    if (!(RetErr))
    {
        RetErr = TRUE;

        ErrCode = STLAYER_GetViewPortFlickerFilterMode ( VPHandle , &FFMode ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);

        if(FFMode==STLAYER_FLICKER_FILTER_MODE_SIMPLE)
        {
            sprintf(LAY_Msg, "STLAYER_GetViewPortFlickerFilterMode(%d)=STLAYER_FLICKER_FILTER_MODE_SIMPLE ", VPHandle);
        }
        else
        {
            if(FFMode==STLAYER_FLICKER_FILTER_MODE_ADAPTIVE)
            {
                sprintf(LAY_Msg, "STLAYER_GetViewPortFlickerFilterMode(%d)=STLAYER_FLICKER_FILTER_MODE_ADAPTIVE ", VPHandle);
            }
            else
            {
                sprintf(LAY_Msg, "STLAYER_GetViewPortFlickerFilterMode(%d)=STLAYER_FLICKER_FILTER_MODE_USING_BLITTER ", VPHandle);
            }
        }
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );

}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetColorFill
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetColorFill (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';
    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;



    if (!(RetErr))
    {
        RetErr = TRUE;

        ErrCode = STLAYER_GetViewportColorFill ( VPHandle , &ColorFill ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);


                    sprintf(LAY_Msg, "STLAYER_GetViewportColorFill(%d)={A=%d,R=%d,G=%d,B=%d}", VPHandle,ColorFill.Alpha,ColorFill.R,ColorFill.G,ColorFill.B);


                 RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );

}




/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortSource
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortSource (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                        RetErr;
    ST_ErrorCode_t              ErrCode;
    S32                         Lvar;
    STLAYER_ViewPortSource_t    Source1;
    STGXOBJ_Palette_t           Palette;
    STLAYER_StreamingVideo_t    VideoStream;
    STGXOBJ_Bitmap_t  *         BitmapParams_p;


    UNUSED_PARAMETER(result_sym_p);
#ifdef ST_OSLINUX
    UNUSED_PARAMETER(BitmapParams_p);
#endif


    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p,VPHndl, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }
    VPHndl= (STLAYER_ViewPortHandle_t)Lvar;
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected source type" );
        return(TRUE);
    }

    if (Lvar == 0) /* gfx source */
    {

#ifndef ST_OSLINUX
       RetErr = STTST_GetInteger(pars_p, 0, &Lvar );

            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "expected bitmap address or 0 for default bitmap " );
                return(TRUE);
            }

        if (Lvar != 0)
        {
            BitmapParams_p = (void*)Lvar ;
            Source1.SourceType    = STLAYER_GRAPHIC_BITMAP;
            Source1.Data.BitMap_p = BitmapParams_p;
            Source1.Palette_p     = &Palette;
        }
        else
        {
            Source1.SourceType    = STLAYER_GRAPHIC_BITMAP;
            Source1.Data.BitMap_p = &BitmapUnknown;
            Source1.Palette_p     = &Palette;
        }
#else
        Source1.SourceType    = STLAYER_GRAPHIC_BITMAP;
        Source1.Data.BitMap_p = &BitmapUnknown;
        Source1.Palette_p     = &Palette;
#endif
    }
    else /* video source */
    {
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected source num" );
            return(TRUE);
        }
        VideoStream.SourceNumber =  Lvar;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected width" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Width = Lvar;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected height" );
            return(TRUE);
        }
        VideoStream.BitmapParams.Height = Lvar;

        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected source address" );
            return(TRUE);
        }
        Source1.Data.VideoStream_p = &VideoStream;
        Source1.Palette_p = &Palette;

        Source1.Data.VideoStream_p->BitmapParams.Data1_p = (void *)Lvar;
        Source1.Data.VideoStream_p->BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420;
        Source1.Data.VideoStream_p->BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;
        Source1.Data.VideoStream_p->BitmapParams.PreMultipliedColor = FALSE;
        Source1.Data.VideoStream_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
        Source1.Data.VideoStream_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
        Source1.Data.VideoStream_p->BitmapParams.Pitch = VideoStream.BitmapParams.Width;
        Source1.Data.VideoStream_p->BitmapParams.Offset = 0;
        Source1.Data.VideoStream_p->BitmapParams.Size1 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height;
        Source1.Data.VideoStream_p->BitmapParams.Size2 = VideoStream.BitmapParams.Width *
            VideoStream.BitmapParams.Height / 2;
        Source1.Data.VideoStream_p->BitmapParams.Data2_p =
            (void *)((U32)VideoStream.BitmapParams.Data1_p +
                     VideoStream.BitmapParams.Size1);
        Source1.Data.VideoStream_p->BitmapParams.SubByteFormat =
            STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
        Source1.Data.VideoStream_p->ScanType = STGXOBJ_INTERLACED_SCAN;
        Source1.Data.VideoStream_p->PresentedFieldInverted = 0;
        Source1.Data.VideoStream_p->CompressionLevel = STLAYER_COMPRESSION_LEVEL_NONE;
        Source1.Data.VideoStream_p->HorizontalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
        Source1.Data.VideoStream_p->VerticalDecimationFactor = STLAYER_DECIMATION_FACTOR_NONE;
        Source1.SourceType = STLAYER_STREAMING_VIDEO;
       }

    sprintf(LAY_Msg, "STLAYER_SetViewPortSource(%d): ",VPHndl);
    ErrCode = STLAYER_SetViewPortSource ( VPHndl, &Source1 ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortSource
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortSource (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                     RetErr;
    ST_ErrorCode_t           ErrCode;
    STLAYER_ViewPortSource_t VPSource;


    /* Init VPSource */
    VPSource.Data.BitMap_p  = &TempBitmap;
    VPSource.Palette_p = &TempPalette;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf( LAY_Msg, "STLAYER_GetViewPortSource(%d): ", VPHndl );
    ErrCode = STLAYER_GetViewPortSource ( VPHndl , &VPSource);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        if (VPSource.SourceType == STLAYER_GRAPHIC_BITMAP)
        {
            sprintf( LAY_Msg, "BitmapHandle=%x\n", (S32)VPSource.Data.BitMap_p );
            STTST_AssignInteger(result_sym_p, (S32)(VPSource.Data.BitMap_p), FALSE);
        }
        else /* video source */
        {
            sprintf( LAY_Msg, "VideoStreamHandle=%x\n", (S32)VPSource.Data.VideoStream_p );
            STTST_AssignInteger(result_sym_p, (S32)(VPSource.Data.VideoStream_p), FALSE);
        }

        LAYER_PRINT(LAY_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_UpdateFromMixer
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_UpdateFromMixer (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_OutputParams_t  OutputParams;
    STLAYER_Handle_t        LayerHandle;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the handle */
    RetErr = STTST_GetInteger( pars_p, LayerHandle, (S32*)&LayerHandle );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected viewport Handle" );
        return(TRUE);
    }

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output X (default 0)" );
        return(TRUE);
    }
    OutputParams.XStart = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Y (default 0)" );
        return(TRUE);
    }
    OutputParams.YStart = Lvar;


    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Width (default 640)" );
        return(TRUE);
    }
    OutputParams.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Height (default 480)" );
        return(TRUE);
    }
    OutputParams.Height = Lvar;

    /* get the output Aspect Ratio  */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio " );
        return(TRUE);
    }
    OutputParams.AspectRatio=(STGXOBJ_AspectRatio_t)Lvar;

    /* get the output Scan Type */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN, &Lvar);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type" );
        return(TRUE);
    }
    OutputParams.ScanType=(STGXOBJ_ScanType_t)Lvar;

    /* get the FrameRate */
    RetErr = STTST_GetInteger( pars_p, 29970, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected output Frame rate (default 29970)" );
        return(TRUE);
    }
    OutputParams.FrameRate = Lvar;

    /* get the VTG name */
    RetErr = STTST_GetString( pars_p, "VTG", OutputParams.VTGName,
                              sizeof(OutputParams.VTGName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected VTG name (default VTG)" );
        return(TRUE);
    }

    OutputParams.DisplayEnable = TRUE;
    OutputParams.DeviceId = 0;
    OutputParams.UpdateReason = STLAYER_VTG_REASON | STLAYER_SCREEN_PARAMS_REASON;
    OutputParams.XOffset = 0;
    OutputParams.YOffset = 0;
    OutputParams.BackLayerHandle = 0;
    OutputParams.FrontLayerHandle = 0;
    OutputParams.DisplayHandle = 0;

    sprintf(LAY_Msg, "STLAYER_UpdateFromMixer(%d,OuX=%d,OuH=%d,OuH=%d,OuW=%d, VTG=%s): ",
            LayerHandle,
            OutputParams.XStart,
            OutputParams.YStart,
            OutputParams.Width,
            OutputParams.Height,
            OutputParams.VTGName);

    ErrCode = STLAYER_UpdateFromMixer(LayerHandle, &OutputParams);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

 /*-----------------------------------------------------------------------------
 * Function : LAYER_DisableFilter
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_DisableFilter(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    LAY_Msg[0]='\0';
    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if ( RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_DisableViewPortFilter(%d): ", VPHandle);
        ErrCode = STLAYER_DisableViewPortFilter( VPHandle ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}
/*-----------------------------------------------------------------------------
 * Function : LAYER_EnableFilter
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_EnableFilter(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    STLAYER_Handle_t  FilterHandle=0;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;
    /* get the Filter handle */
    RetErr = STTST_GetInteger( pars_p, FilterHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    FilterHandle = (STLAYER_Handle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableViewPortFilter(%d): ", VPHandle);
        ErrCode = STLAYER_EnableViewPortFilter( VPHandle, FilterHandle) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortFlickerFilterMode
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_SetViewPortFlickerFilterMode(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar=0;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }

    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    /* Get Scan Type */
    RetErr = STTST_GetInteger( pars_p, STLAYER_FLICKER_FILTER_MODE_SIMPLE, &Lvar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type (default Interlaced)" );
        return(TRUE);
    }

   FFMode = (U32)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_SetFlickerFilterMode(%d,FF_TYPE=%s):",\
        VPHandle,
        (FFMode == STLAYER_FLICKER_FILTER_MODE_SIMPLE) ? "FF_MODE_SIMPLE" : \
        (FFMode== STLAYER_FLICKER_FILTER_MODE_ADAPTIVE) ? "FF_MODE_ADAPTIVE" : \
        (FFMode== STLAYER_FLICKER_FILTER_MODE_USING_BLITTER) ? "FF_MODE_USING_BLITTER" : "????");

        ErrCode = STLAYER_SetViewPortFlickerFilterMode( VPHandle, FFMode) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
}




/*-----------------------------------------------------------------------------
 * Function : LAYER_EnableColorFill
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_EnableColorFill(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    STLAYER_Handle_t  FilterHandle=0;


    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;
    /* get the Filter handle */
    RetErr = STTST_GetInteger( pars_p, FilterHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    FilterHandle = (STLAYER_Handle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableViewPortFilter(%d): ", VPHandle);
        ErrCode = STLAYER_EnableViewportColorFill( VPHandle) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
}



/*-----------------------------------------------------------------------------
 * Function : LAYER_SetColorFill
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_SetColorFill(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;


    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 100, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }

    ColorFill.Alpha=(int)Lvar;

      /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 100, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }

    ColorFill.R=(int)Lvar;

         /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 100, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }

    ColorFill.G=(int)Lvar;


         /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 100, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    ColorFill.B=(int)Lvar;


  /*
    ColorFill.Alpha=20;
    ColorFill.R=255;
    ColorFill.G=255;
    ColorFill.B=0;

    */


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableViewPortFilter(%d): ", VPHandle);
        ErrCode = STLAYER_SetViewportColorFill( VPHandle,&ColorFill) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
}



/*-----------------------------------------------------------------------------
 * Function : LAYER_DisableColorFill
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL  LAYER_DisableColorFill(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    STLAYER_Handle_t  FilterHandle=0;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;
    /* get the Filter handle */
    RetErr = STTST_GetInteger( pars_p, FilterHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    FilterHandle = (STLAYER_Handle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableViewPortFilter(%d): ", VPHandle);
        ErrCode = STLAYER_DisableViewportColorFill( VPHandle) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
}






/*-----------------------------------------------------------------------------
 * Function : LAYER_Term
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    STLAYER_TermParams_t TermParams;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME, DeviceName,
            sizeof(DeviceName));
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

   /* get term param */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p,
                "Expected ForceTerminate (TRUE, default FALSE)" );
        return(TRUE);
    }
    TermParams.ForceTerminate = (BOOL)LVar;

    RetErr = TRUE;
    sprintf(LAY_Msg, "STLAYER_Term(%s,forceterminate=%d): ", \
            DeviceName, TermParams.ForceTerminate);
    ErrCode = STLAYER_Term( DeviceName, &TermParams ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end LAYER_Term */


/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortPosition
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortPosition (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar,PositionX,PositionY;


    UNUSED_PARAMETER(result_sym_p);
    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected ViewPort" );
        return(TRUE);
    }

    /* get the X Position */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected X position" );
        return(TRUE);
    }
    PositionX = Lvar;

    /* get the Y Position */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected Y position" );
        return(TRUE);
    }
    PositionY = Lvar;

    sprintf(LAY_Msg, "STLAYER_SetViewPortPosition(%d,X=%d,Y=%d): ", \
            VPHndl, PositionX, PositionY);
    ErrCode = STLAYER_SetViewPortPosition ( VPHndl , PositionX , PositionY) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

#ifndef ST_OSLINUX
/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortCompositionRecurrence
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortCompositionRecurrence (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_CompositionRecurrence_t CompositionRecurrence;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, STLAYER_COMPOSITION_RECURRENCE_EVERY_VSYNC, &Lvar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expectedComposition Mode" );
        return(TRUE);
    }
    CompositionRecurrence = (U8)Lvar;


    sprintf( LAY_Msg, "STLAYER_SetViewPortCompositionRecurrence(%d,Mode=%d): ",
             VPHndl, CompositionRecurrence);

    ErrCode = STLAYER_SetViewPortCompositionRecurrence ( VPHndl, CompositionRecurrence ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_PerformViewPortComposition
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_PerformViewPortComposition (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }


    UNUSED_PARAMETER(result_sym_p);


    sprintf( LAY_Msg, "STLAYER_PerformViewPortComposition(%d): ", VPHndl);

    ErrCode = STLAYER_PerformViewPortComposition ( VPHndl ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}
#endif


#ifdef ST_OSLINUX_DEBUG
static BOOL LAYER_Read( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr;

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }
	printf("Read at %x: ",Addr);
	printf("%x\n",STSYS_ReadRegDev32LE(Addr));

return RetErr;
}

static BOOL LAYER_Write( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr, Val;

    UNUSED_PARAMETER(result_sym_p);

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Val );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Value" );
        }
    }

	STSYS_WriteRegDev32LE((Addr),(Val));

	printf("Write at %x: ",Addr);
	printf("%x\n",Val);

return RetErr;
}
#endif

#ifdef ST_OSLINUX

static BOOL LAYER_Phys_Read( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr;

    UNUSED_PARAMETER(result_sym_p);

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        STAPIGAT_Phys_Read( (U32*) Addr );
    }

    return RetErr;
}

static BOOL LAYER_Phys_Write( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr, Val;


    UNUSED_PARAMETER(result_sym_p);

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Val );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Value" );
        }
    }

    if (!(RetErr))
    {
        STAPIGAT_Phys_Write( (U32*) Addr, Val );
    }

    return RetErr;
}

static BOOL LAYER_Phys_Dump_Read( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
    S32 Addr,Number;


    UNUSED_PARAMETER(result_sym_p);



    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Number );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Number" );
        }
    }

    if (!(RetErr))
    {
      STAPIGAT_Phys_Dump_Read( (U32)Addr, Number );
	}

    return RetErr;
}

static BOOL LAYER_Phys_Fill( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
    S32  Addr, Number, val;

    UNUSED_PARAMETER(result_sym_p);

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Number );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected length" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &val );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected data" );
        }
    }

    if (!(RetErr))
    {
        STAPIGAT_Phys_Fill( (U32*)Addr, Number, val );

	}

    return RetErr;
}
#endif

/*###########################################################################*/
/*############################# LAYER COMMANDS ##############################*/
/*###########################################################################*/

/*-----------------------------------------------------------------------------
 * Function : LAYER_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/
static BOOL LAYER_InitCommand (void)
{
    BOOL RetErr;

    /*  Command's name, link to C-function, help message     */
    /*  (by alphabetic order of macros = display order)     */
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "layer_aiorect",  LAYER_AdjustIORectangle,
                                     "<Layer Handle><X in><Y in><Width in>"
                                     "<Height in><X out><Y out><Width out>"
                                     "<Height out>: adjust the IO Rectangles");
    RetErr |= STTST_RegisterCommand( "layer_avpparams",  LAYER_AdjustViewPortParams,
                                     "<Handle><X in><Y in><Width in>"
                                     "<Height in><X out><Y out><Width out>"
                                     "<Height out>: adjust the VP params");
    RetErr |= STTST_RegisterCommand( "layer_attvp",
                        LAYER_AttachAlphaViewPort,
                      "<VP handle> <Layer Handle>: Attach alpha to layer");
    RetErr |= STTST_RegisterCommand( "layer_close", LAYER_Close,
                           "<Layer Handle>: Close a layer");
    RetErr |= STTST_RegisterCommand( "layer_closevp", LAYER_CloseViewPort,
                           "<ViewPort Handle>: Close a ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_disable", LAYER_DisableViewPort,
                           "<ViewPort Handle>: Disable VP Function");
    RetErr |= STTST_RegisterCommand( "layer_enable", LAYER_EnableViewPort,
                           "<ViewPort Handle>: Enable VP Function");
    RetErr |= STTST_RegisterCommand( "layer_gparams", LAYER_GetLayerParams,
                           "<Layer Handle>: Retrieves the Layer parameters");
    RetErr |= STTST_RegisterCommand( "layer_gvpiorect",
                                LAYER_GetViewPortIORectangle,
                "<ViewPort Handle>: Display the IO Rectangles of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_init", LAYER_Init,
                 "<DeviceName><LayerType><Base@><Width><Height><ScanType>"
                 "AspectRatio><MaxViewPort>:\n"
                                    "\t\tInitialize a layer and return a handle");
    RetErr |= STTST_RegisterCommand( "layer_open", LAYER_Open,
                 "<DeviceName>: Open an Initialized layer and return a handle");
    RetErr |= STTST_RegisterCommand( "layer_openvp", LAYER_OpenViewPort,
                 "<Layer Handle><Source><X in><Y in>: Open a ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_sparams", LAYER_SetLayerParams,
                                     "<Handle>: Set the Layer parameters");
    RetErr |= STTST_RegisterCommand( "layer_svpalpha", LAYER_SetViewPortAlpha,
       "<ViewPort Handle><A0><A1>: Set the alpha params of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_svpiorect",
                                    LAYER_SetViewPortIORectangle,
       "<ViewPort Handle><X in><Y in><Width in><Height in><X out><Y out>"
       "<Width out>\n"
       "\t\t<Height out>: Set the IO Rectangles of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_svpparams", LAYER_SetViewPortParams,
                                     "<VP Handle><X in><Y in><W in><H in>"
                                     "<X out><Y out><W out><H out>...: Set the"
                                     " parameters of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_svppos", LAYER_SetViewPortPosition,
                 "<ViewPort Handle><X><Y>: Set the Position of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_svpsrc",  LAYER_SetViewPortSource,
                                     "<ViewPort Handle><Source-type><Source-num>:"
                                     "Set the Source of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_gvpsrc",  LAYER_GetViewPortSource,
                                     "<ViewPort Handle>: Get the Source of the ViewPort");
    RetErr |= STTST_RegisterCommand( "layer_term", LAYER_Term,
                 "<DeviceName>: Termination of the specified layer");
    RetErr |= STTST_RegisterCommand( "lay_ena_flt",  LAYER_EnableFilter,
        "<ViewPort Handle>: Enable AntiFlicker");
    RetErr |= STTST_RegisterCommand( "lay_set_flt_mode",  LAYER_SetViewPortFlickerFilterMode,
        "<ViewPort Handle>: Enable AntiFlicker");
    RetErr |= STTST_RegisterCommand( "lay_get_flt_mode",  LAYER_GetViewPortFlickerFilterMode,
        "<ViewPort Handle>: Get FlickerFilter Mode");
   RetErr |= register_command( "lay_discf",  LAYER_DisableColorFill,
        "<ViewPort Handle>: Disable Color Fill Function");
    RetErr |= register_command( "lay_encf",  LAYER_EnableColorFill,
        "<ViewPort Handle>: Enable Color Fill Function");
    RetErr |= register_command( "lay_setcf",  LAYER_SetColorFill,
        "<ViewPort Handle>: Set Color Fill Function");
    RetErr |= register_command( "lay_getcf",  LAYER_GetColorFill,
        "<ViewPort Handle>: Get Color Fill Function");
    RetErr |= STTST_RegisterCommand( "lay_dis_flt",   LAYER_DisableFilter,
        "<ViewPort Handle>: Disable AntiFlicker");
    RetErr |= STTST_RegisterCommand( "layer_updatefrommixer", LAYER_UpdateFromMixer,
                                     "<Layer Handle><Xstart out><Ystart out><Xoffset out><Yoffset out><W out><H out>"
                                     "<AspectRatio out><ScanType out><FrameRate out><VTG Name>: Set the"
                                     " parameters of the layer");



#ifndef ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "layer_SetViewPortCompositionRecurrence", LAYER_SetViewPortCompositionRecurrence,
                                        "<Layer Handle><Mode>");

    RetErr |= STTST_RegisterCommand( "layer_PerformViewPortComposition", LAYER_PerformViewPortComposition,
                                        "<Layer Handle>");
#endif

#ifdef ST_OSLINUX_DEBUG
    RetErr |= STTST_RegisterCommand( "lay_read",   LAYER_Read, "<Address> Read at Address");
    RetErr |= STTST_RegisterCommand( "lay_write",   LAYER_Write, "<Address> <Value> Write at Address");
#endif

#ifdef ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "lay_pwrite",   LAYER_Phys_Write, "<Address> <Value> Write at physical Address (input is virtual)");
    RetErr |= STTST_RegisterCommand( "lay_pread",   LAYER_Phys_Read, "<Address> Read at physical Address (input is virtual)");
    RetErr |= STTST_RegisterCommand( "lay_pdread",   LAYER_Phys_Dump_Read, "<Address> <long> Dump at physical Address (input is virtual) for number of long");
    RetErr |= STTST_RegisterCommand( "lay_pfill",   LAYER_Phys_Fill, "<Address> <long> <Data> Fill at physical Address (input is virtual) for number of long");
#endif
    /* Constants */
    RetErr |= STTST_AssignString("LAYER_NAME", LAYER_USED_DEVICE_NAME, TRUE);
    RetErr |= STTST_AssignInteger ("FF_MODE_ADAPTIVE", STLAYER_FLICKER_FILTER_MODE_ADAPTIVE, TRUE);
    RetErr |= STTST_AssignInteger ("FF_MODE_SIMPLE", STLAYER_FLICKER_FILTER_MODE_SIMPLE, TRUE);
    RetErr |= STTST_AssignInteger ("FF_MODE_USING_BLITTER", STLAYER_FLICKER_FILTER_MODE_USING_BLITTER, TRUE);


#ifdef ST_OSLINUX
    RetErr |= STTST_AssignString ("DRV_PATH_LAYER", "layer/", TRUE);
#else
    RetErr |= STTST_AssignString ("DRV_PATH_LAYER", "", TRUE);
#endif

    return(RetErr ? FALSE : TRUE);

} /* end LAYER_MacroInit */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL LAYER_RegisterCmd()
{
    BOOL RetOk;


    /* Global inits */
    memset(AllocatedBlockTable, 0,
            sizeof(AllocatedBlockHandle_t)*MAX_ALLOCATED_BLOCK);



    RetOk = LAYER_InitCommand();
    if ( RetOk )
    {
        sprintf( LAY_Msg, "LAYER_Command()     \t: ok           %s\n",
                STLAYER_GetRevision() );
    }
    else
    {
        sprintf( LAY_Msg, "LAYER_Command()     \t: failed !\n");
    }
    STTST_Print((LAY_Msg));

    return( RetOk );

} /* end LAYER_CmdStart */

