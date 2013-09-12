/************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : lay_cmd.c
Description : Definition of layer commands

Note        :

Date          Modification
----          ------------

************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"

#include "stlayer.h"
#include "stsys.h"

#include "lay_cmd.h"
#ifdef ST_OS20
#include "debug.h"
#endif
#ifdef ST_OS21
#include "os21debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#endif

/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */
#define MAX_LAYER_TYPE 10
#define MAX_ASPECTRATIO_TYPE 4
#define MAX_SCANTYPE_TYPE 2
#define MAX_ALLOCATED_BLOCK 10

#if defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || defined (ST_5518) || defined (ST_5514)
#define LAYER_DEVICE_BASE_ADDRESS   0
#define LAYER_BASE_ADDRESS          VIDEO_BASE_ADDRESS
#define LAYER_USED_DEVICE_NAME      STLAYER_VID_DEVICE_NAME
#define LAYER_DEVICE_TYPE           STLAYER_OMEGA1_VIDEO
#define LAYER_MAX_VIEWPORT          1
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576
#elif defined (ST_7015)
#define LAYER_DEVICE_BASE_ADDRESS   STI7015_BASE_ADDRESS
#define LAYER_BASE_ADDRESS          (ST7015_GAMMA_OFFSET+ST7015_GAMMA_GFX2_OFFSET)
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           STLAYER_GAMMA_BKL
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         1920
#define LAYER_DEFAULT_HEIGHT        1080
#elif defined (ST_7020)
#define LAYER_DEVICE_BASE_ADDRESS   STI7020_BASE_ADDRESS
#define LAYER_BASE_ADDRESS          (ST7020_GAMMA_OFFSET+ST7020_GAMMA_GFX2_OFFSET)
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           STLAYER_GAMMA_BKL
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         1920
#define LAYER_DEFAULT_HEIGHT        1080
#elif defined (ST_GX1)
#define LAYER_DEVICE_BASE_ADDRESS   0xbb410000
#define LAYER_BASE_ADDRESS          0x100
#define LAYER_USED_DEVICE_NAME      STLAYER_GFX_DEVICE_NAME
#define LAYER_DEVICE_TYPE           STLAYER_GAMMA_GDP
#define LAYER_MAX_VIEWPORT          4
#define LAYER_DEFAULT_WIDTH         720
#define LAYER_DEFAULT_HEIGHT        576

#define SDRAM_BASE_ADDRESS 0 /* !!!!!!! TO BE REMOVED !!!!!!!!!! */

#else
#error Not defined processor
#endif

/* --- Macros ---*/
#define LAYER_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(LAY_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* --- Prototypes --- */
/* --- Local variables --- */
static char LAY_Msg[250];
static STLAYER_Handle_t LayHndl;
static STLAYER_ViewPortHandle_t VPHndl;

typedef struct {
    STLAYER_ViewPortHandle_t VPHndl;
    STAVMEM_BlockHandle_t BitmapHndl;
    STAVMEM_BlockHandle_t PaletteHndl;
} AllocatedBlockHandle_t;
static AllocatedBlockHandle_t AllocatedBlockTable[MAX_ALLOCATED_BLOCK];

static const char LayerType[MAX_LAYER_TYPE][10] = {
    "CURSOR", "GDP", "BKL", "O_ALPHA", "VIDEO1", "VIDEO2", "O_CURSOR", "O_VIDEO", "O_OSD", "O_STILL"
};

static const char AspectRatioType[MAX_ASPECTRATIO_TYPE][7] = {
    "16/9", "4/3", "221/01", "SQUARE"
};

static const char ScanTypeType[MAX_SCANTYPE_TYPE][6] = {
    "Prog", "Int  "
};

/* --- Externals --- */
extern ST_Partition_t *SystemPartition;
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;

#define GAMMA_FILE_ACLUT44   0x8C
#define GAMMA_FILE_ACLUT88   0x8D
#define GAMMA_FILE_ACLUT8    0x8B
#define GAMMA_FILE_CLUT8     0x8E
#define GAMMA_FILE_RGB565    0x80
#define GAMMA_FILE_ARGB1555  0x86
#define GAMMA_FILE_ARGB4444  0x87
#define GAMMA_FILE_YCBCR422R 0x92

/* Header of Bitmap file                                             */
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

STAVMEM_SharedMemoryVirtualMapping_t VM;

/*-----------------------------------------------------------------------------
 * Function : GetBitmap
  * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void GetBitmap(STGXOBJ_Bitmap_t * Pixmap, STAVMEM_BlockHandle_t* BlockHandle_p)
{
    ST_ErrorCode_t  RepError;
    STAVMEM_AllocBlockParams_t AllocParams;
    long int  FileDescriptor;    /* File pointer */
    char FileName[40];
    U32  Size;
    void * Unpitched_Data_p;
    STAVMEM_BlockHandle_t BlockHandle;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualM;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualM);

    Pixmap->Width      = 768;
    Pixmap->Height     = 576;
    Pixmap->Pitch      = Pixmap->Width * 2; /* 2bits per pixel */
    if (Pixmap->Pitch % 128 != 0)
    {
        Pixmap->Pitch = Pixmap->Pitch -(Pixmap->Pitch % 128) + 128;
    }
    Pixmap->ColorType  = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
    Pixmap->BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM;
    AllocParams.PartitionHandle = AvmemPartitionHandle;
    AllocParams.Size =Pixmap->Pitch * Pixmap->Height ;
    AllocParams.Alignment = 128;
    AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges = 0;
    AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *) NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p = (void **) NULL;
    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams, BlockHandle_p);
    STAVMEM_GetBlockAddress( *BlockHandle_p, (void **)&(Pixmap->Data1_p));
    Pixmap->Data2_p = (U8*)Pixmap->Data1_p + AllocParams.Size/2;
    STTBX_DirectPrint(("Pixmap allocated\n" ));
    /* ------------------------- */
    /* Pixmap loading from file */
    /* ------------------------- */

    STTBX_DirectPrint(("Frame buffer loading , please wait ...\n"));
    strcpy (FileName, "../../../scripts/Zdf.yuv");

#ifdef ST_OS20
    FileDescriptor = debugopen( (char *)FileName, "rb");
#endif
#ifdef ST_OS21
        FileDescriptor  = open(FileName, O_RDONLY | O_BINARY  );
#endif /* ST_OS21 */
    if(FileDescriptor == -1){
        STTBX_DirectPrint(( "Unable to open file !!!\n"));
        return;
    }

    RepError = STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                                   &AllocParams,(STAVMEM_BlockHandle_t *) &(BlockHandle));
    STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Unpitched_Data_p));
    Unpitched_Data_p = STAVMEM_VirtualToCPU(Unpitched_Data_p,&VirtualM);

    Size =(U32)debugread(FileDescriptor,(U8*)Unpitched_Data_p,AllocParams.Size);

    STTBX_DirectPrint(( "Top Bottom (768x576) Picture loaded\n"));
    debugclose(FileDescriptor);

    STAVMEM_CopyBlock2D ( (void *)Unpitched_Data_p,
                          (Pixmap->Width * 2),
                          (Pixmap->Height / 2),
                          (Pixmap->Width * 2),
                          (void *)STAVMEM_VirtualToCPU(Pixmap->Data1_p,&VirtualM),
                          (Pixmap->Pitch) );
    STAVMEM_CopyBlock2D ( (void *)((U32)(Unpitched_Data_p)+
                                   (Pixmap->Width * Pixmap->Height)),
                          (Pixmap->Width * 2),
                          (Pixmap->Height / 2),
                          (Pixmap->Width * 2),
                          (void *)STAVMEM_VirtualToCPU(Pixmap->Data2_p,&VirtualM),
                          (Pixmap->Pitch) );
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle;
    STAVMEM_FreeBlock (&FreeBlockParams, &BlockHandle);

    return ;
}

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
ST_ErrorCode_t GUTIL_LoadBitmap(char *filename,
                                STGXOBJ_Bitmap_t *Bitmap_p,
                                STGXOBJ_Palette_t *Palette_p,
                                STAVMEM_BlockHandle_t* BitmapHandle_p,
                                STAVMEM_BlockHandle_t* PaletteHandle_p)
{
    STAVMEM_AllocBlockParams_t  AllocBlockParams;  /* Allocation param                       */

    ST_ErrorCode_t ErrCode;/* Error management                       */
    FILE *fstream;        /* File handle for read operation          */
    /*  void *buffer_p;  */      /* Temporay pointer storage for allocation */
    U32  size;            /* Used to test returned size for a read   */
    U32 dummy[2];         /* used to read colorkey, but not used     */
    GUTIL_GammaHeader
        Gamma_Header;     /* Header of Bitmap file                   */
    BOOL IsPalette;       /* Palette is present into the current file*/
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;
    /* STGXOBJ_PaletteAllocParams_t PaletteAllocParams; */
    U32 Signature;

    ErrCode   = ST_NO_ERROR;
    Signature = 0x0;
    IsPalette = FALSE;

    /* Init structure                                                */
    memset((void *)&BitmapAllocParams1, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));
    memset((void *)&BitmapAllocParams2, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));
    memset((void *)&AllocBlockParams, 0,
           sizeof(STAVMEM_AllocBlockParams_t));

    /* Block allocation parameter                                    */
    AllocBlockParams.PartitionHandle = AvmemPartitionHandle;
    AllocBlockParams.Size            = 0;
    AllocBlockParams.AllocMode       = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
    AllocBlockParams.NumberOfForbiddenRanges  = 0;
    AllocBlockParams.ForbiddenRangeArray_p    = NULL;
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    /* Check parameter                                               */
    if ( (filename == NULL) ||
         (Bitmap_p == NULL)
        )
    {
        STTBX_Print(( "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Open file handle                                              */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            STTBX_Print(( "Unable to open \'%s\'\n", filename ));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    STTBX_Print(( "Loading file %s...\n", filename));
    /* Read Header                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        size = fread((void *)&Gamma_Header, 1,
                     sizeof(GUTIL_GammaHeader), fstream);
        if (size != sizeof(GUTIL_GammaHeader))
        {
            STTBX_Print(( "Header: Read error %d byte instead of %d\n",
                          size,sizeof(GUTIL_GammaHeader)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

    /* Check Header                                                  */
    if (ErrCode == ST_NO_ERROR)
    {
        if ( ((Gamma_Header.Header_size != 0x6) &&
              (Gamma_Header.Header_size != 0x8)   ) ||
             (Gamma_Header.Properties   != 0x0    ) ||
             (Gamma_Header.zero         != 0x0    ) )
        {
            STTBX_Print(("Read %d waited 0x6 or 0x8\n",
                         Gamma_Header.Header_size));
            STTBX_Print(("Read %d waited 0x0\n",
                         Gamma_Header.Properties));
            STTBX_Print(("Read %d waited 0x0\n",
                         Gamma_Header.zero));
            STTBX_Print(( "Not a valid file (Header corrupted)\n"));
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
                STTBX_Print(( "Read error %d byte instead of %d\n", size,4));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables          */
    /* And do bitmap allocation                                      */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->BitmapType           = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        Bitmap_p->PreMultipliedColor     = FALSE;
        Bitmap_p->ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        Bitmap_p->AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;

        /* Configure in function of the bitmap type                  */
        switch (Gamma_Header.Type)
        {
            case GAMMA_FILE_ACLUT8 :
            {
                Signature = 0x444F;
                if (Palette_p == NULL)
                {
                    STTBX_Print(( "Error: Null Pointer, bad parameter\n"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_CLUT8;
                    Bitmap_p->Pitch =  Bitmap_p->Width;
                    Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
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
                    STTBX_Print(( "Error: Null Pointer, bad parameter\n"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_CLUT8;
                    Bitmap_p->Pitch =  Bitmap_p->Width;
                    Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB4444;
                    Palette_p->PaletteType =STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
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
                    STTBX_Print(( "Error: Null Pointer, bad parameter\n"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_ACLUT88;
                    Bitmap_p->Pitch =  Bitmap_p->Width;
                    Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType= STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
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
                    STTBX_Print(( "Error: Null Pointer, bad parameter\n"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    Bitmap_p->ColorType    = STGXOBJ_COLOR_TYPE_ACLUT44;
                    Bitmap_p->Pitch =  Bitmap_p->Width;
                    Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType =STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 4;
                    IsPalette              = TRUE;
                }
                break;
            }
            case GAMMA_FILE_RGB565 :
            {
                Signature           = 0x444F;
                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_RGB565;
                Bitmap_p->Pitch =  Bitmap_p->Width * 2;
                Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                break;
            }
            case GAMMA_FILE_ARGB1555 :
            {
                Signature           = 0x444F;
                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB1555;
                Bitmap_p->Pitch =  Bitmap_p->Width * 2;
                Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                break;
            }
            case GAMMA_FILE_ARGB4444 :
            {
                Signature           = 0x444F;
                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ARGB4444;
                Bitmap_p->Pitch =  Bitmap_p->Width * 2;
                Bitmap_p->Size1 =  Bitmap_p->Pitch * Bitmap_p->Height;
                break;
            }
            case GAMMA_FILE_YCBCR422R :
            {
                Signature           = 0x422F;
                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
                break;
            }
            default :
            {
                STTBX_Print(( "Type not supported 0x%08x\n",
                              Gamma_Header.Type));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                break;
            }
        } /* switch (Gamma_Header.Type) */
    }

    if (ErrCode == ST_NO_ERROR)
    {
        if (Gamma_Header.Signature !=  Signature)
        {
            STTBX_Print(( "Error: Read 0x%08x, Waited 0x%08x\n",
                          Gamma_Header.Signature, Signature));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }


    /* If necessary palette management                               */
    if ( (ErrCode == ST_NO_ERROR) && (IsPalette == TRUE) )
    {

        /* Read palette if necessary                              */
        if (ErrCode == ST_NO_ERROR)
        {
            AllocBlockParams.PartitionHandle = AvmemPartitionHandle;
            AllocBlockParams.Size           = 256*2;
            AllocBlockParams.Alignment              = 256;
            AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges= 0;
            AllocBlockParams.ForbiddenRangeArray_p    = 0;
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,PaletteHandle_p);
            STAVMEM_GetBlockAddress( *PaletteHandle_p, (void **)&(Palette_p->Data_p));
            STAVMEM_GetSharedMemoryVirtualMapping(&VM);

            size = fread ((void *)STAVMEM_VirtualToCPU(Palette_p->Data_p,&VM)
                    , 1,(AllocBlockParams.Size), fstream);

            if (size != (AllocBlockParams.Size))
            {
                STTBX_Print(( "Pallette: Read error %d byte instead of %d\n",size,
                              (AllocBlockParams.Size)));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }


    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        AllocBlockParams.PartitionHandle = AvmemPartitionHandle;
        AllocBlockParams.Size           = (Bitmap_p->Size1)*Gamma_Header.NbPict;
        AllocBlockParams.Alignment              = 256;
        AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges= 0;
        AllocBlockParams.ForbiddenRangeArray_p    = 0;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,BitmapHandle_p);
        STAVMEM_GetBlockAddress( *BitmapHandle_p, (void **)&(Bitmap_p->Data1_p));
        STAVMEM_GetSharedMemoryVirtualMapping(&VM);

        /* Data2 not needed, we are in RASTER_PROGRESSIVE            */
        /* Bitmap_p->Data2_p             =                           */
        /* Bitmap_p->Size2               =                           */
        /* Bitmap_p->SubByteFormat       =                           */

        STTBX_Print(("Reading file Bitmap data ... \n"));
        size = fread ((void*)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM)),1,
                      (Bitmap_p->Size1)*Gamma_Header.NbPict, fstream);
        if (size != ((Bitmap_p->Size1)*Gamma_Header.NbPict) )
        {
            STTBX_Print(("Bitmap: Read error %d byte instead of %d\n", size,
                          (Bitmap_p->Size1)*Gamma_Header.NbPict));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            if (Palette_p->Data_p != NULL)
            {
            }

        }
        fclose (fstream);
    }
    return ErrCode;
}

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

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
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
                strcat( Text, "(stlayer error iorectangles not adjustable !)\n");
                break;
            case STLAYER_ERROR_INVALID_LAYER_TYPE:
                strcat( Text, "(stlayer error invalid layer type !)\n");
                break;
            case STLAYER_ERROR_USER_ALLOCATION_NOT_ALLOWED:
                strcat( Text, "(stlayer error user allocation not allowed !)\n");
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


/*###############################################################################*/
/*############### STATIC FUNCTIONS : TESTTOOL USER INTERFACE ####################*/
/*###############          (in alphabetic order)             ####################*/
/*###############################################################################*/


/*-----------------------------------------------------------------------------
 * Function : LAYER_Close
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Close( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    ST_ErrorCode_t ErrCode;

    /* Get the Layer Handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr == TRUE )
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
static BOOL LAYER_CloseViewPort (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    LAY_Msg[0]='\0';

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
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
        for(Lvar=0; (Lvar<MAX_ALLOCATED_BLOCK) && (AllocatedBlockTable[Lvar].VPHndl!= VPHndl); Lvar++);
        if(Lvar != MAX_ALLOCATED_BLOCK)
        {
            FreeBlockParams.PartitionHandle = AvmemPartitionHandle;
            if(AllocatedBlockTable[Lvar].BitmapHndl != 0)
                ErrCode=STAVMEM_FreeBlock (&FreeBlockParams, &AllocatedBlockTable[Lvar].BitmapHndl);
            if(AllocatedBlockTable[Lvar].PaletteHndl != 0)
                ErrCode|=STAVMEM_FreeBlock (&FreeBlockParams, &AllocatedBlockTable[Lvar].PaletteHndl);
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
static BOOL LAYER_DisableViewPort (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
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
static BOOL LAYER_EnableViewPort (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
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
static BOOL LAYER_Init( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    STLAYER_InitParams_t InitParams;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    S32 Lvar;

    /* Get device */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    /* get LayerType */
    RetErr = STTST_GetInteger( pars_p, LAYER_DEVICE_TYPE, &Lvar);
    if ( RetErr == TRUE ){
        STTST_TagCurrentLine( pars_p,
                              "Expected Layer Type(0:G_CURSOR, 1:GDP, 2:BKL, 3:G_ALPHA, 4:VIDEO1, 5:VIDEO2, 6:O_CURSOR, 7:O_VIDEO, 8:O_OSD, 9:O_STILL)");
        return(TRUE);
    }
    InitParams.LayerType = (STLAYER_Layer_t)Lvar;

    switch (InitParams.LayerType)
    {
        case STLAYER_OMEGA2_VIDEO1 :  /* OMEGA2_VIDEO1 */
            #if defined (ST_7015)
            InitParams.BaseAddress_p = (void*)(ST7015_GAMMA_OFFSET + ST7015_GAMMA_VID1_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7015_DISPLAY_OFFSET;
            #endif
            break;
        case STLAYER_OMEGA2_VIDEO2 :  /* OMEGA2_VIDEO2 */
            #if defined (ST_7015)
            InitParams.BaseAddress_p = (void*)(ST7015_GAMMA_OFFSET + ST7015_GAMMA_VID2_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7015_PIP_DISPLAY_OFFSET;
            #endif
            break;
        case STLAYER_7020_VIDEO1 :    /* STLAYER_7020_VIDEO1 */
            #if defined (ST_7020)
            InitParams.BaseAddress_p = (void*)(ST7020_GAMMA_OFFSET + ST7020_GAMMA_VID1_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7020_DISPLAY_OFFSET;
            #endif
            break;
        case STLAYER_7020_VIDEO2 :    /* STLAYER_7020_VIDEO2 */
            #if defined (ST_7020)
            InitParams.BaseAddress_p = (void*)(ST7020_GAMMA_OFFSET + ST7020_GAMMA_VID2_OFFSET);
            InitParams.BaseAddress2_p = (void*)ST7020_PIP_DISPLAY_OFFSET;
            #endif
            break;
        case STLAYER_GAMMA_CURSOR :  /* GAMMA_CURSOR, */
        case STLAYER_GAMMA_GDP :  /* GAMMA_GDP,    */
        case STLAYER_GAMMA_BKL :  /* GAMMA_BKL,    */
        case STLAYER_OMEGA1_CURSOR :  /* OMEGA1_CURSOR */
        case STLAYER_OMEGA1_VIDEO :  /* OMEGA1_VIDEO, */
        case STLAYER_OMEGA1_OSD :  /* OMEGA1_OSD,   */
        case STLAYER_OMEGA1_STILL :  /* OMEGA1_STILL  */
            /* Get base address */
            RetErr = STTST_GetInteger( pars_p, LAYER_BASE_ADDRESS, &Lvar);
            if ( RetErr == TRUE ){
                STTST_TagCurrentLine( pars_p, "Expected base address of layer type");
                return(TRUE);
            }
            InitParams.BaseAddress_p = (void*)Lvar;
            InitParams.BaseAddress2_p = (void*)0;
            break;
        case STLAYER_GAMMA_ALPHA :  /* GAMMA_ALPHA, */
            /**/
            break;
        default:
            InitParams.BaseAddress_p = (void*)0;
            InitParams.BaseAddress2_p = (void*)0;
            break;
    }

    /* Get Width */
    RetErr = STTST_GetInteger( pars_p, LAYER_DEFAULT_WIDTH, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    LayerParams.Width = (U32)Lvar;

    /* Get Height */
    RetErr = STTST_GetInteger( pars_p, LAYER_DEFAULT_HEIGHT, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Height" );
        return(TRUE);
    }
    LayerParams.Height = (U32)Lvar;

    /* Get Scan Type */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type (default Interlaced)" );
        return(TRUE);
    }
    LayerParams.ScanType = (U32)Lvar;

    /* Get Aspect Ratio */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio (default 4:3)" );
        return(TRUE);
    }
    LayerParams.AspectRatio = (U32)Lvar;

    /* get MaxViewPorts */
    RetErr = STTST_GetInteger( pars_p, LAYER_MAX_VIEWPORT, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxViewPorts" );
        return(TRUE);
    }
    InitParams.MaxViewPorts = (U32)Lvar;

    InitParams.DeviceBaseAddress_p = (void *)(LAYER_DEVICE_BASE_ADDRESS);
    InitParams.CPUPartition_p            = (ST_Partition_t*)SystemPartition;
    InitParams.MaxHandles                = 2;
    InitParams.AVMEM_Partition           = AvmemPartitionHandle;
    InitParams.ViewPortNodeBuffer_p      = NULL;
    InitParams.ViewPortBuffer_p          = NULL;
    InitParams.ViewPortBufferUserAllocated  = FALSE;
    InitParams.NodeBufferUserAllocated      = FALSE;
    InitParams.SharedMemoryBaseAddress_p = (void *)SDRAM_BASE_ADDRESS;
    strcpy(InitParams.EventHandlerName, STEVT_DEVICE_NAME);
#ifdef mb295
    InitParams.SharedMemoryBaseAddress_p = (void *)SDRAM_WINDOW_BASE_ADDRESS;
#endif
    InitParams.LayerParams_p = &LayerParams;

    sprintf(LAY_Msg, "STLAYER_Init(%s,Type=%s): ",
            DeviceName,
            (InitParams.LayerType<MAX_LAYER_TYPE) ? LayerType[InitParams.LayerType] : "????" );
    ErrCode = STLAYER_Init( DeviceName, &InitParams ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    sprintf(LAY_Msg, "\tAdd=%x, Add2=%x, Devbaseadd=%x\n",
            (U32)InitParams.BaseAddress_p, (U32)InitParams.BaseAddress2_p, (U32)InitParams.DeviceBaseAddress_p);
    sprintf(LAY_Msg, "%s\tcpup=%x, shmem=%x, avmemp=%x, evt='%s'\n",
            LAY_Msg,
            (U32)InitParams.CPUPartition_p, (U32)InitParams.SharedMemoryBaseAddress_p,
            (U32)InitParams.AVMEM_Partition, InitParams.EventHandlerName);
    sprintf(LAY_Msg, "%s\tnodebuf=%d, vpnode=%x, vpbufuse=%d, vpbuf=%x\n",
            LAY_Msg,
            InitParams.NodeBufferUserAllocated, (U32)InitParams.ViewPortNodeBuffer_p,
            InitParams.ViewPortBufferUserAllocated, (U32)InitParams.ViewPortBuffer_p );
    sprintf(LAY_Msg, "%s\tmaxhd=%d, maxvp=%d\n",
            LAY_Msg,
            InitParams.MaxHandles, InitParams.MaxViewPorts);
    sprintf(LAY_Msg, "%s\tWid=%d, Hei=%d, AspRat=%s, ScanT=%s\n",
            LAY_Msg,
            LayerParams.Width,
            LayerParams.Height,
            (LayerParams.AspectRatio<MAX_ASPECTRATIO_TYPE) ? AspectRatioType[LayerParams.AspectRatio] : "???" ,
            (LayerParams.ScanType<MAX_SCANTYPE_TYPE) ? ScanTypeType[LayerParams.ScanType] : "???"  );
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
static BOOL LAYER_GetLayerParams (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    char StrParams[80];

    /* Get the handle of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Viewport" );
        return(TRUE);
    }

    sprintf(LAY_Msg, "STLAYER_GetLayerParams(%d): ", VPHndl);
    ErrCode = STLAYER_GetLayerParams( VPHndl, &LayerParams );
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if(ErrCode == ST_NO_ERROR)
    {
        sprintf(LAY_Msg, "\tScanType=%s  AspectRatio=%s\n\tWidth=%d  Height=%d\n", \
                (LayerParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : \
                (LayerParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" : "????",
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                (LayerParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "????"
                ,LayerParams.Width, LayerParams.Height);
        LAYER_PRINT(LAY_Msg);
        sprintf( StrParams, "%d %d %d %d", \
                 LayerParams.ScanType, LayerParams.AspectRatio, LayerParams.Width, LayerParams.Height);
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
static BOOL LAYER_GetViewPortIORectangle (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STGXOBJ_Rectangle_t InputRectangle;
    STGXOBJ_Rectangle_t OutputRectangle;
    char StrParams[RETURN_PARAMS_LENGTH];

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort" );
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf( LAY_Msg, "STLAYER_GetViewPortIORectangle(%d): ", VPHndl );
    ErrCode = STLAYER_GetViewPortIORectangle ( VPHndl , &InputRectangle, &OutputRectangle ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( LAY_Msg, "\tInX0=%d InY0=%d InWidth=%d InHeight=%d\n",
                    InputRectangle.PositionX, InputRectangle.PositionY,
                    InputRectangle.Width, InputRectangle.Height);
        sprintf( LAY_Msg, "%s\tOutX0=%d OutY0=%d OutWidth=%d OutHeight=%d\n",
                    LAY_Msg, OutputRectangle.PositionX, OutputRectangle.PositionY,
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
static BOOL LAYER_Open (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_OpenParams_t OpenParams;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;

    /* get device name */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr == TRUE)
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
static BOOL LAYER_OpenViewPort (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_ViewPortParams_t    VPParam;
    ST_ErrorCode_t ErrCode;
    STLAYER_ViewPortSource_t    source;
    STAVMEM_BlockHandle_t       BitmapHandle=0;
    STAVMEM_BlockHandle_t       PaletteHandle=0;
    S32 Lvar;

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5514)
    STAVMEM_AllocBlockParams_t  AllocParams;
    int i;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) */

    /* get the layer handle */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected handle" );
        return(TRUE);
    }

    /* Default Inits */
    VPParam.Source_p = &source;
    source.SourceType = STLAYER_GRAPHIC_BITMAP;

    RetErr = STTST_GetInteger( pars_p, 2, &Lvar );
    if ( RetErr == TRUE )
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        STTST_TagCurrentLine( pars_p, "expected Source 1=facecursor 2=merou 3=susie 4=format" );
#endif /* ST_7015 ST_7020 ST_GX1 */
#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5514)
        STTST_TagCurrentLine( pars_p, "expected Source 1=Cursor 2=OSD 3=Still" );
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) */
        return(TRUE);
    }
    if(Lvar ==1)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        GUTIL_LoadBitmap("../../../scripts/face_128x128.gam",&Bitmapface,&FacePalette,&BitmapHandle,&PaletteHandle);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapface.Width;
        VPParam.InputRectangle.Height = Bitmapface.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source.Palette_p = &FacePalette;
        source.Data.BitMap_p = &Bitmapface;
#endif /* ST_7015 ST_7020 ST_GX1 */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5514)
        BitmapCursor.ColorType              = STGXOBJ_COLOR_TYPE_CLUT2;
        BitmapCursor.PreMultipliedColor     = FALSE;
        BitmapCursor.ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        BitmapCursor.AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        BitmapCursor.Width                  = 40;
        BitmapCursor.Height                 = 40;
        BitmapCursor.Pitch                  = 10;
        BitmapCursor.Data1_p                = (void*)Cursor_Bitmap;
        PaletteCursor.ColorType     = STGXOBJ_COLOR_TYPE_ARGB8888;
        PaletteCursor.ColorDepth    = 2;
        PaletteCursor.Data_p        = (void*)Cursor_Color;
        source.Data.BitMap_p      = &BitmapCursor;
        source.Palette_p          = &PaletteCursor;

        VPParam.InputRectangle.PositionX   = 0;
        VPParam.InputRectangle.PositionY   = 0;
        VPParam.InputRectangle.Width       = 40;
        VPParam.InputRectangle.Height      = 40;
        VPParam.OutputRectangle.PositionX   = 200;
        VPParam.OutputRectangle.PositionY   = 200;
        VPParam.OutputRectangle.Width       = 40;
        VPParam.OutputRectangle.Height      = 40;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) */

    }
    if(Lvar ==2)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        GUTIL_LoadBitmap("../../../scripts/merou3.gam",&Bitmapmerou,0,&BitmapHandle,0);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapmerou.Width;
        VPParam.InputRectangle.Height = Bitmapmerou.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source.Palette_p = 0;
        source.Data.BitMap_p = &Bitmapmerou;
#endif /* ST_7015 ST_7020 ST_GX1 */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5514)
        /* Creation Palettes */
        PaletteOSD.ColorType  = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
        PaletteOSD.ColorDepth = 8;
        STLAYER_GetPaletteAllocParams(LayHndl,&PaletteOSD,&PaletteOSDParams);
        AllocParams.PartitionHandle = AvmemPartitionHandle;
        AllocParams.Size = PaletteOSDParams.AllocBlockParams.Size;
        AllocParams.Alignment = PaletteOSDParams.AllocBlockParams.Alignment;
        AllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocParams.NumberOfForbiddenRanges = 0;
        AllocParams.ForbiddenRangeArray_p = (STAVMEM_MemoryRange_t *)0;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.ForbiddenBorderArray_p = (void **)0;
        STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                            &AllocParams,(STAVMEM_BlockHandle_t *) &(PaletteHandle));
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
        STLAYER_GetBitmapAllocParams(LayHndl,&BitmapOSD,&BitmapOSDParams1,&BitmapOSDParams2);
        AllocParams.Size = BitmapOSDParams1.AllocBlockParams.Size;
        AllocParams.Alignment = BitmapOSDParams1.AllocBlockParams.Alignment;
        BitmapOSD.Pitch = BitmapOSDParams1.Pitch;
        BitmapOSD.Offset = BitmapOSDParams1.Offset;
        STAVMEM_AllocBlock ((const STAVMEM_AllocBlockParams_t *)
                            &AllocParams,(STAVMEM_BlockHandle_t *) &(BitmapHandle));
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
        source.Palette_p                    = &PaletteOSD;
        source.Data.BitMap_p                = &BitmapOSD;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) */

    }
    if(Lvar ==3)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        GUTIL_LoadBitmap("../../../scripts/suzie.gam",&Bitmapeye,0,&BitmapHandle,0);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapeye.Width;
        VPParam.InputRectangle.Height = Bitmapeye.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source.Palette_p = 0;
        source.Data.BitMap_p = &Bitmapeye;
#endif /* ST_7015 ST_7020 ST_GX1 */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5514)
        GetBitmap(&BitmapStill, &BitmapHandle);
        VPParam.InputRectangle.PositionX    = 50;
        VPParam.InputRectangle.PositionY    = 50;
        VPParam.InputRectangle.Width        = 250;
        VPParam.InputRectangle.Height       = 250;
        VPParam.OutputRectangle             = VPParam.InputRectangle;
        VPParam.OutputRectangle.PositionY   = 0;
        VPParam.OutputRectangle.PositionX   = 0;
        source.Palette_p                    = 0;
        source.Data.BitMap_p                = &BitmapStill;
#endif /* (ST_5510) (ST_5512) (ST_5508) (ST_5518) (ST_5514) */

    }
    if(Lvar ==4)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        GUTIL_LoadBitmap("../../../scripts/format.gam",&BitmapFormat,0,&BitmapHandle,0);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = BitmapFormat.Width;
        VPParam.InputRectangle.Height = BitmapFormat.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source.Palette_p = 0;
        source.Data.BitMap_p = &BitmapFormat;
#endif /* ST_7015 ST_7020 ST_GX1 */
    }

    if(Lvar ==5)
    {
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1)
        GUTIL_LoadBitmap("../../../scripts/guitar.gam",&Bitmapguitar,0,&BitmapHandle,0);
        VPParam.InputRectangle.PositionX = 0;
        VPParam.InputRectangle.PositionY = 0;
        VPParam.InputRectangle.Width = Bitmapguitar.Width;
        VPParam.InputRectangle.Height = Bitmapguitar.Height;
        VPParam.OutputRectangle=VPParam.InputRectangle;
        source.Palette_p = 0;
        source.Data.BitMap_p = &Bitmapguitar;
#endif /* ST_7015 ST_7020 ST_GX1 */
    }

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected X Position (default 0)" );
        return(TRUE);
    }
    VPParam.OutputRectangle.PositionX = Lvar;

    /* get the Y position Output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Y Position (default 0)" );
        return(TRUE);

    }
    VPParam.OutputRectangle.PositionY = Lvar;

    ErrCode = STLAYER_OpenViewPort( LayHndl, &VPParam, &VPHndl ) ;
    sprintf( LAY_Msg, "STLAYER_OpenViewPort(%d,X=%d,Y=%d) : ", \
             LayHndl, VPParam.OutputRectangle.PositionX, VPParam.OutputRectangle.PositionY);
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);
    sprintf( LAY_Msg, "\thnd=%d\n", VPHndl);
    LAYER_PRINT(LAY_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        STTST_AssignInteger(result_sym_p, VPHndl, FALSE);
        /* Store allocated block */
        for(Lvar=0; (Lvar<MAX_ALLOCATED_BLOCK) && (AllocatedBlockTable[Lvar].VPHndl!=0); Lvar++);
        if(Lvar==MAX_ALLOCATED_BLOCK){
            sprintf( LAY_Msg, "Too many block allocated !!!\n" );
            LAYER_PRINT(LAY_Msg);
            return(TRUE);
        }
        AllocatedBlockTable[Lvar].BitmapHndl = BitmapHandle;
        AllocatedBlockTable[Lvar].PaletteHndl = PaletteHandle;
        AllocatedBlockTable[Lvar].VPHndl = VPHndl;
    }

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_SetLayerParams
 *            Get driver layer parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetLayerParams (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STLAYER_LayerParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    /* Get the handle of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, LayHndl, (S32*)&LayHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected layer Handle" );
        return(TRUE);
    }

    /* get the Scan Type of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Scan Type" );
        return(TRUE);
    }
    LayerParams.ScanType=(STGXOBJ_ScanType_t)Lvar;

    /* get the Aspect Ratio of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio " );
        return(TRUE);
    }
    LayerParams.AspectRatio=(STGXOBJ_AspectRatio_t)Lvar;

    /* get the Width of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, 576, &Lvar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    LayerParams.Width=(U32)Lvar;

    /* get the Height of the ViewPort */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar);
    if ( RetErr == TRUE )
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
 * Function : LAYER_SetViewPortIORectangle
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortIORectangle (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STGXOBJ_Rectangle_t InputRectangle;
    STGXOBJ_Rectangle_t OutputRectangle;
    S32 Lvar;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected ViewPort Handle" );
        return(TRUE);
    }

    /* get the X position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected X (default 0)" );
        return(TRUE);
    }
    InputRectangle.PositionX = Lvar;

    /* get the Y position input rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Y (default 0)" );
        return(TRUE);
    }
    InputRectangle.PositionY = Lvar;

    /* get the Width input rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Width (default 640)" );
        return(TRUE);
    }
    InputRectangle.Width = Lvar;

    /* get the Height input rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Height (default 480)" );
        return(TRUE);
    }
    InputRectangle.Height = Lvar;

    /* get the X position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected X (default 0)" );
        return(TRUE);
    }
    OutputRectangle.PositionX = Lvar;

    /* get the Y position output rectangle */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Y (default 0)" );
        return(TRUE);
    }
    OutputRectangle.PositionY = Lvar;

    /* get the Width output rectangle */
    RetErr = STTST_GetInteger( pars_p, 640, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Width (default 640)" );
        return(TRUE);
    }
    OutputRectangle.Width = Lvar;

    /* get the Height output rectangle */
    RetErr = STTST_GetInteger( pars_p, 480, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Height (default 480)" );
        return(TRUE);
    }
    OutputRectangle.Height = Lvar;

    sprintf(LAY_Msg, "STLAYER_SetViewPortIORectangle(%d,InX=%d,InY=%d,InW=%d,InH=%d,OuX=%d,OuH=%d,OuH=%d,OuW=%d): ",
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
    ErrCode = STLAYER_SetViewPortIORectangle ( VPHndl , &InputRectangle, &OutputRectangle ) ;
    RetErr = LAYER_TextError( ErrCode, LAY_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_Term
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_Term( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    STLAYER_TermParams_t TermParams;
    S32 LVar;

    /* Get device name */
    RetErr = STTST_GetString( pars_p, LAYER_USED_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

   /* get term param */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected ForceTerminate (TRUE, default FALSE)" );
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
static BOOL LAYER_SetViewPortPosition (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar,PositionX,PositionY;

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHndl, (S32*)&VPHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected ViewPort" );
        return(TRUE);
    }

    /* get the X Position */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected X position" );
        return(TRUE);
    }
    PositionX = Lvar;

    /* get the Y Position */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
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
    RetErr |= STTST_RegisterCommand( "LayClose", LAYER_Close,
                                     "<Layer Handle>: Close a layer");
    RetErr |= STTST_RegisterCommand( "LayCloseVP", LAYER_CloseViewPort,
                                     "<ViewPort Handle>: Close a ViewPort");
    RetErr |= STTST_RegisterCommand( "LayDisable", LAYER_DisableViewPort,
                                     "<ViewPort Handle>: Disable Border Alpha Function");
    RetErr |= STTST_RegisterCommand( "LayEnable", LAYER_EnableViewPort,
                                     "<ViewPort Handle>: Enable VP Function");
    RetErr |= STTST_RegisterCommand( "LayGParams", LAYER_GetLayerParams,
                                     "<ViewPort Handle>: Retrieves the Layer parameters");
    RetErr |= STTST_RegisterCommand( "LayGVPIoRect", LAYER_GetViewPortIORectangle,
                                     "<ViewPort Handle>: Display the IO Rectangles of the ViewPort");
    RetErr |= STTST_RegisterCommand( "LayInit", LAYER_Init,
                                     "<DeviceName><LayerType><Base@><Width><Height><ScanType><AspectRatio><MaxViewPort>:\n"
                                    "\t\tInitialise a layer return an handle");
    RetErr |= STTST_RegisterCommand( "LayOpen", LAYER_Open,

                                     "<DeviceName>: Open an Initialized layerand return an handle");
    RetErr |= STTST_RegisterCommand( "LayOpenVP", LAYER_OpenViewPort,
                                     "<Layer Handle><Source><X in><Y in>: Open a ViewPort");
    RetErr |= STTST_RegisterCommand( "LaySParams", LAYER_SetLayerParams,
                                     "<Handle>: Set the Layer parameters");
    RetErr |= STTST_RegisterCommand( "LaySVPIoRect", LAYER_SetViewPortIORectangle,
                                     "<ViewPort Handle><X in><Y in><Width in><Height in><X out><Y out><Width out>\n"
                                     "\t\t<Height out>: Set the IO Rectangles of the ViewPort");
    RetErr |= STTST_RegisterCommand( "LaySVPPos", LAYER_SetViewPortPosition,
                                     "<ViewPort Handle><X><Y>: Set the Position of the ViewPort");
    RetErr |= STTST_RegisterCommand( "LayTerm", LAYER_Term,
                                     "<DeviceName>: Termination of the specified layer");

    /* Constants */
    RetErr |= STTST_AssignString("LAYER_NAME", LAYER_USED_DEVICE_NAME, TRUE);

    return(RetErr ? FALSE : TRUE);

} /* end LAYER_MacroInit */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL LAYER_RegisterCmd()
{
    BOOL RetOk;

    /* Global inits */
    memset(AllocatedBlockTable, 0, sizeof(AllocatedBlockHandle_t)*MAX_ALLOCATED_BLOCK);

    RetOk = LAYER_InitCommand();
    if ( RetOk )
    {
        sprintf( LAY_Msg, "LAYER_Command()     \t: ok           %s\n", STLAYER_GetRevision() );
    }
    else
    {
        sprintf( LAY_Msg, "LAYER_Command()     \t: failed !\n");
    }
    STTST_Print((LAY_Msg));

    return( RetOk );

} /* end LAYER_CmdStart */


/*###########################################################################*/

