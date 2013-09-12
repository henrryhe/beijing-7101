/**********************************************************************

File name   : tt_gutil.c

Copyright   : COPYRIGHT (C) STMicroelectronics 2000.

Description : Graphics Utils
 Includes -------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include "stddefs.h"

#ifdef ST_OSLINUX
#include <sys/ioctl.h>
#include "stdevice.h"
#include "stlayer.h"
#else
#include "stlite.h"
#include "stdevice.h"
#endif
#include "sttbx.h"
#include "stos.h"
#include "stgxobj.h"
#include "testtool.h"


/* Private types/constants ----------------------------------------- */
/* Gamma File type.                                                  */
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
#define GAMMA_FILE_YCBCR422R        0x92
#define GAMMA_FILE_YCBCR420MB       0x94
#define GAMMA_FILE_AYCBCR8888       0x96
#define GAMMA_FILE_ALPHA1           0x98


#define MD5AUTOTEST TRUE
#define BLIT_MULTI_OPEN FALSE

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


#ifndef ST_OSLINUX
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;
STAVMEM_SharedMemoryVirtualMapping_t VM;
ST_ErrorCode_t GUTIL_AllocBuffer( U32 Size, U32  Alignment, STAVMEM_BlockHandle_t* BlockHandle_p, void **BufferAddress_p );
#else


extern STLAYER_Handle_t LayHndl;
#endif

ST_ErrorCode_t GUTIL_LoadBitmap(char *filename,
                                STGXOBJ_Bitmap_t *Bitmap_p,
                                STGXOBJ_Palette_t *Palette_p);
/* ================================================================= */
/*                                                                   */
/*                     General functions                             */
/*                                                                   */
/* ================================================================= */

/* ----------------------------------------------------------------- */
/*                      Bitmap functions                             */
/* ----------------------------------------------------------------- */

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
                                STGXOBJ_Palette_t *Palette_p)
{
#ifdef  ST_OSLINUX
    STLAYER_AllocDataParams_t LayAllocData;
#else
    STAVMEM_AllocBlockParams_t AllocBlockParams;  /* Allocation param */
#endif
    U32         SizeAlloc;

    ST_ErrorCode_t ErrCode;/* Error management                       */
    FILE *fstream;        /* File handle for read operation          */
    U32  size;            /* Used to test returned size for a read   */
    U32 dummy[2];         /* used to read colorkey, but not used     */
    GUTIL_GammaHeader
        Gamma_Header;     /* Header of Bitmap file                   */
    BOOL IsPalette;       /* Palette is present into the current file*/
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;
    STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;
    STAVMEM_BlockHandle_t BitmapHandle=0;
    U32 Signature;

    ErrCode   = ST_NO_ERROR;
    Signature = 0x0;
    IsPalette = FALSE;


    /* Init structure                                                */
    memset((void *)&BitmapAllocParams1, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));
    memset((void *)&BitmapAllocParams2, 0,
           sizeof(STGXOBJ_BitmapAllocParams_t));
#ifndef ST_OSLINUX
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
#endif

    /* Check parameter                                               */
    if ( (filename == NULL) ||
         (Bitmap_p == NULL)
        )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

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
		else
        {
            STTBX_Print(("File: \'%s\' opened\n", filename ));
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
        if ( ((Gamma_Header.Header_size != 0x6) &&
              (Gamma_Header.Header_size != 0x8)   ) ||
             (Gamma_Header.zero         != 0x0    ) )
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error: Null Pointer, bad parameter\n"));
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
        case GAMMA_FILE_ALPHA1 :
        {
            Signature           = 0x444F;
            Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_ALPHA1;
            Bitmap_p->Pitch =  Bitmap_p->Width / 8;
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
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Type not supported 0x%08x\n",
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
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Error: Read 0x%08x, Waited 0x%08x\n",
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
            STTBX_Print(("Read palette ...\n"));

            SizeAlloc = 256*2 ;
#ifdef ST_OSLINUX
            LayAllocData.Size = SizeAlloc ;
            LayAllocData.Alignment = 256 ;
            STLAYER_AllocData( LayHndl, &LayAllocData,
                            &Palette_p->Data_p );
            if ( Palette_p->Data_p == NULL )
            {
                STTBX_Print(("Error Allocating data for palette...\n"));
                return ST_ERROR_BAD_PARAMETER;
            }
            size = fread ((void *)Palette_p->Data_p, 1, SizeAlloc, fstream );
#else
            GUTIL_AllocBuffer( SizeAlloc, 256,
                            &BitmapHandle,&Palette_p->Data_p );
            size = fread ((void *)STAVMEM_VirtualToCPU(Palette_p->Data_p,&VM)
                    , 1,SizeAlloc, fstream);
#endif
/*
        AllocBlockParams.PartitionHandle = AvmemPartitionHandle;
        AllocBlockParams.Size           = 256*2;
        AllocBlockParams.Alignment              = 256;
        AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges= 0;
        AllocBlockParams.ForbiddenRangeArray_p    = 0;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle));
        STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Palette_p->Data_p));
        STAVMEM_GetSharedMemoryVirtualMapping(&VM);
            size = fread ((void *)STAVMEM_VirtualToCPU(Palette_p->Data_p,&VM)
                    , 1,(AllocBlockParams.Size), fstream);
*/
            if (size != SizeAlloc)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Read error %d byte instead of %d\n",size,
                              (SizeAlloc)));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Reading file Bitmap data ... \n"));
#ifdef  ST_OSLINUX
        LayAllocData.Size = (Bitmap_p->Size1)*Gamma_Header.NbPict ;
        LayAllocData.Alignment = 256 ;
        STLAYER_AllocData( LayHndl, &LayAllocData,
                            &Bitmap_p->Data1_p );

        if ( Bitmap_p->Data1_p == NULL )
        {
            STTBX_Print(("Error Allocating data for palette...\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        STTBX_Print(("Reading file Bitmap data Address data bitmap = 0x%x... \n", (U32)Bitmap_p->Data1_p ));

        size = fread ((void*)(Bitmap_p->Data1_p),1,
                      (Bitmap_p->Size1)*Gamma_Header.NbPict, fstream);

#else
        GUTIL_AllocBuffer( (Bitmap_p->Size1)*Gamma_Header.NbPict, 256,
                            &BitmapHandle,&Bitmap_p->Data1_p );

        printf("  -> @:0x%x (0x%x), BMSize:%d, NBPict:%d, Size:%d\n", (U32)STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM),
                        (U32)Bitmap_p->Data1_p, Bitmap_p->Size1, Gamma_Header.NbPict, (Bitmap_p->Size1)*Gamma_Header.NbPict);

        size = fread ((void*)(STAVMEM_VirtualToCPU(Bitmap_p->Data1_p,&VM)),1,
                      (Bitmap_p->Size1)*Gamma_Header.NbPict, fstream);

#endif
/*
        AllocBlockParams.PartitionHandle = AvmemPartitionHandle;
        AllocBlockParams.Size           = (Bitmap_p->Size1)*Gamma_Header.NbPict;
        AllocBlockParams.Alignment              = 256;
        AllocBlockParams.AllocMode              = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges= 0;
        AllocBlockParams.ForbiddenRangeArray_p    = 0;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;
        ErrCode = STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle));
        STAVMEM_GetBlockAddress( BlockHandle, (void **)&(Bitmap_p->Data1_p));
        STAVMEM_GetSharedMemoryVirtualMapping(&VM);
*/

        /* Data2 not needed, we are in RASTER_PROGRESSIVE            */
        /* Bitmap_p->Data2_p             =                           */
        /* Bitmap_p->Size2               =                           */
        /* Bitmap_p->SubByteFormat       =                           */





/*#ifdef ST_OSLINUX*/

        if (size != ((Bitmap_p->Size1)*Gamma_Header.NbPict) )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Read error %d byte instead of %d\n", size,
                          (Bitmap_p->Size1)*Gamma_Header.NbPict));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            if (Palette_p->Data_p != NULL)
            {
            }

        }
        fclose (fstream);
    }

    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                      "GUTIL loaded correctly %s file\n",filename));
    }

    return ErrCode;

} /* GUTIL_LoadBitmap () */




