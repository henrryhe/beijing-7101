/*****************************************************************************

File name   : blt_cmd.c

Description : STBLIT macros

COPYRIGHT (C) STMicroelectronics 2004.

Date                     Modification                  Name
----                     ------------                  ----
20 September 2003        Created                       HE
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <string.h>
#ifdef ST_OS21
    #include "os21debug.h"
    #include <sys/stat.h>
    #include <fcntl.h>
#endif
#ifdef OS_20
    #include <debug.h>
#endif
#include <stdio.h>
#include "blt_cmd.h"



/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */

static char SrcFileName[30]="C:/Scripts/suzie.gam";
static char DstFileName[30]="C:/Scripts/merou3.gam";
static char UserTagString[]="BLIT";
static STGXOBJ_Bitmap_t        SourceBitmap, TargetBitmap;


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Extern Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : BlitCompletedHandler
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
void BlitCompletedHandler (STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event,
                           const void*            EventData_p,
                           const void*            SubscriberData_p)
{
    semaphore_signal(&BlitCompletionTimeout);
}

/*---------------------------------------------------------------------
 * Function : GUTIL_Allocate
 *            Allocate a ptr using STAVMEM mecanism,
 *            Manage internaly corespondance between ptr and
 *            BlockHandlde. See STAVMEM documentation for
 *            STAVMEM_AllocBlockParams_t definition.
 * Input    : STAVMEM_AllocBlockParams_t *AllocBlockParams_p
 * Output   : void **ptr  Pointer to allocated address
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_Allocate (STAVMEM_AllocBlockParams_t *AllocBlockParams_p, void **ptr)
{
    ST_ErrorCode_t ErrCode;

    ErrCode = ST_NO_ERROR;
    *ptr = NULL;

    /* Allocate on more space for storing BlockHandle                */
    if(Allocate_Number == (1 << (8*sizeof(Allocate_Number_t))))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Allocate Error : no more allocation allowed\n"));
        return ST_ERROR_NO_MEMORY;
    }
    Allocate_Number ++;

    Allocate_KeepInfo =
    memory_reallocate(SystemPartition_p,
			  (void *)Allocate_KeepInfo,
			  (U32)(Allocate_Number*sizeof(Allocate_KeepInfo_t)));

    if ((void *)Allocate_KeepInfo == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
		      "GUTIL_Allocate Error : Allocation error\n"));
        return ST_ERROR_NO_MEMORY;
    }

    /* Space for storing BlockHandle is done, allocate it            */
    ErrCode = STAVMEM_AllocBlock(AllocBlockParams_p,&(Allocate_KeepInfo[Allocate_Number-1].BlockHandle));
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
              "GUTIL_Allocate, STAVMEM_Allocate(0x%08x,0x%08x)=%d\n",
		      Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
		      Allocate_KeepInfo[Allocate_Number-1].ptr,
              ErrCode));
    }
    else
    {
        /* Retrieve address from BlockHandle                         */
        ErrCode = STAVMEM_GetBlockAddress
	    (Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
	     &(Allocate_KeepInfo[Allocate_Number-1].ptr));
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
              "GUTIL_Allocate, STAVMEM_GetBlockAddress(0x%08x,0x%08x)=%d\n",
			  Allocate_KeepInfo[Allocate_Number-1].BlockHandle,
              Allocate_KeepInfo[Allocate_Number-1].ptr,ErrCode));
        }
        else
        {
            *ptr = Allocate_KeepInfo[Allocate_Number-1].ptr;
        }
    }

    return ErrCode;
} /* GUTIL_Allocate() */


/*---------------------------------------------------------------------
 * Function : GUTIL_Free
 *            Free a ptr previously allocated with GUTIL_Allocate.
 *            Mecanism used was STAVMEM, internaly we manage the
 *            corespondance between ptr and BlockHandlde.
 * Input    : void *ptr
 * Output   :
 * Return   : ErrCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t GUTIL_Free(void *ptr)
{
    ST_ErrorCode_t ErrCode;
    STAVMEM_FreeBlockParams_t FreeBlockParams;
    Allocate_Number_t loop;
    void *tmp_p;

    ErrCode = ST_NO_ERROR;
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];

    /* Search ptr into previously allocated by GUTIL_Allocate        */
    for(loop=0; loop<Allocate_Number;loop ++)
    {
        if (ptr == Allocate_KeepInfo[loop].ptr)
        {
            break;
        }
    } /* for(loop=0; loop<Allocate_Number;loop ++) */
    if (loop == Allocate_Number)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Free error : Bad pointer, not allocated by GxAllocate\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Free the ptr, its handle is used to do that with STAVMEM      */
    ErrCode = STAVMEM_FreeBlock(&FreeBlockParams, &(Allocate_KeepInfo[loop].BlockHandle));
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "GUTIL_Free error : Unable to free with STAVMEM the ptr\n"));
    }
    else
    {
        /* Now, we must clean our Allocate_KeepInfo array, and       */
        /* ralloc it smaller.                                        */
        Allocate_Number--;
        if (loop != Allocate_Number)
        {
            Allocate_KeepInfo[loop] = Allocate_KeepInfo[Allocate_Number];
        }
        /* If equal, this is the last on the tabs,                   */
        /* do not need to reorganise.                                */

        /* Clean last before unallocating                            */
        Allocate_KeepInfo[Allocate_Number].ptr = NULL;
        Allocate_KeepInfo[Allocate_Number].BlockHandle = STAVMEM_INVALID_BLOCK_HANDLE;

        tmp_p = memory_reallocate(SystemPartition_p,(void *)Allocate_KeepInfo,Allocate_Number*sizeof(Allocate_KeepInfo_t));
        if ( ((void *)tmp_p == NULL) && (Allocate_Number != 0) )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"GUTIL_Free Error : Reallocation error\n"));
            return ST_ERROR_NO_MEMORY;
        }
        Allocate_KeepInfo = (Allocate_KeepInfo_t *)tmp_p;
    }

    return ErrCode;

} /* GUTIL_Free() */

/*-----------------------------------------------------------------------------
 * Function : ConvertGammaToBitmap
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t ConvertGammaToBitmap(char*               filename,
                                    STGXOBJ_Bitmap_t*   Bitmap_p,
                                    STGXOBJ_Palette_t*  Palette_p)
{
    STAVMEM_AllocBlockParams_t  AllocBlockParams; /* Allocation param*/
    ST_ErrorCode_t              ErrCode;         /* Error management */
    FILE*                       fstream;        /* File handle for read operation          */
    void*                       buffer_p;       /* Temporay pointer storage for allocation */
    U32                         size;            /* Used to test returned size for a read   */
    U32                         dummy[2];         /* used to read colorkey, but not used     */
    GUTIL_GammaHeader           Gamma_Header; /* Header of Bitmap file         */
    int Rest;
    STAVMEM_MemoryRange_t  RangeArea[2];
    U8                      NbForbiddenRange;

    ErrCode = ST_NO_ERROR;

    /* Block allocation parameter */
    NbForbiddenRange = 1;
    if (VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((VirtualMapping.VirtualWindowOffset + VirtualMapping.VirtualWindowSize) !=
         VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                             (U32)(VirtualMapping.VirtualWindowOffset) +
                                             (U32)(VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(VirtualMapping.VirtualBaseAddress_p) +
                                                  (U32)(VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }

    AllocBlockParams.PartitionHandle            = AvmemPartitionHandle[0];
    AllocBlockParams.Size                       = 0;
    AllocBlockParams.AllocMode                  = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges    = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p      = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders   = 0;
    AllocBlockParams.ForbiddenBorderArray_p     = NULL;

    /* Check parameter */
    if ((filename == NULL) || (Bitmap_p == NULL) ||(Palette_p == NULL))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error: Null Pointer, bad parameter\n"));
        ErrCode = ST_ERROR_BAD_PARAMETER;
    }

    /* Check if a bitmap was previously allocated here               */
    /* Check is based on the NULL value of the pointer, so init      */
    /* is very important.                                            */
    if (ErrCode == ST_NO_ERROR)
    {
        if (Bitmap_p->Data1_p != NULL)
        {
            /* Deallocate buffer before changing the pointer */
            /* GUTIL_Free(Bitmap_p->Data1_p);*/
            Bitmap_p->Data1_p = NULL;
        }
        if (Palette_p->Data_p != NULL)
        {
            /* Deallocate buffer before changing the pointer  */
            /*GUTIL_Free(Palette_p->Data_p);*/
            Palette_p->Data_p = NULL;
        }
    }

    /* Open file handle */
    if (ErrCode == ST_NO_ERROR)
    {
        fstream = fopen(filename, "rb");
        if( fstream == NULL )
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to open \'%s\'\n", filename ));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Read Header */
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(("Reading file Header ... \n"));
        size = fread((void *)&Gamma_Header, 1,sizeof(GUTIL_GammaHeader), fstream);
        if (size != sizeof(GUTIL_GammaHeader))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,sizeof(GUTIL_GammaHeader)));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
        }
    }

    /* Check Header */
    if (ErrCode == ST_NO_ERROR)
    {
        if (((Gamma_Header.Header_size != 0x6)      &&
             (Gamma_Header.Header_size != 0x8))     ||
            ((Gamma_Header.Signature    != 0x444F)  &&
             (Gamma_Header.Signature    != 0x422F)) ||
             (Gamma_Header.zero         != 0x0))
        {
            STTBX_Print(("Read %d waited 0x6 or 0x8\n",Gamma_Header.Header_size));
            STTBX_Print(("Read %d waited 0x44F or 0x22F\n",Gamma_Header.Signature));
            STTBX_Print(("Read %d waited 0x0\n",Gamma_Header.Properties));
            STTBX_Print(("Read %d waited 0x0\n",Gamma_Header.zero));
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Not a valid file (Header corrupted)\n"));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            fclose (fstream);
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
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n", size,4));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                fclose (fstream);
            }
        }
    }

    /* In function of bitmap type, configure some variables   */
    /* And do bitmap allocation                               */
    if (ErrCode == ST_NO_ERROR)
    {

        /* Configure in function of the bitmap type           */
        switch (Gamma_Header.Type)
        {
            case GAMMA_FILE_ACLUT8 :
            {
                AllocBlockParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                Bitmap_p->Pitch            = Gamma_Header.width;
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x8B)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT8 bitmap  */
                    AllocBlockParams.Size = 256*4;
                    GUTIL_Allocate(&AllocBlockParams, &buffer_p);
                    if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 8;
                    Palette_p->Data_p = buffer_p;
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(256*4), fstream);
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (256*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(256*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Bitmap_p->ColorType = STGXOBJ_COLOR_TYPE_CLUT8;
                Bitmap_p->Size1     = Gamma_Header.nb_pixel;
                break;
            }
            case GAMMA_FILE_ACLUT4 :
            {
                AllocBlockParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x8A)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT4 bitmap  */
                    AllocBlockParams.Size = 16*4;
                    GUTIL_Allocate(&AllocBlockParams, &buffer_p);
                    if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 4;
                    Palette_p->Data_p = buffer_p;
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(16*4), fstream);
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (16*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(16*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Rest = Gamma_Header.width % 2;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 2;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 2) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocBlockParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_CLUT4;
                break;





            }

            case GAMMA_FILE_ACLUT2 :
            {
                AllocBlockParams.Alignment = 16; /* CLUT is 16 bytes aligned  */
                /* Allocate memory for palette                           */
                if (Gamma_Header.Type == 0x89)
                {
                    STTBX_Print(("Allocate for palette ...\n"));
                    /* ARGB8888 palette for a CLUT4 bitmap  */
                    AllocBlockParams.Size = 4*4;
                    GUTIL_Allocate(&AllocBlockParams, &buffer_p);
                    if (buffer_p == NULL)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for palette\n"));
                        ErrCode = ST_ERROR_NO_MEMORY;
                        fclose (fstream);
                    }
                }

                /* Read palette if necessary */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("Read palette ...\n"));
                    Palette_p->ColorType   = STGXOBJ_COLOR_TYPE_ARGB8888;
                    Palette_p->PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT;
                    Palette_p->ColorDepth  = 2;
                    Palette_p->Data_p = buffer_p;
                    size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Palette_p->Data_p,&VirtualMapping)), 1,(4*4), fstream);
                    STTBX_Print(("\tPalette_p->Data_p : 0X%08x\n",Palette_p->Data_p));
                    STTBX_Print(("\tAllocate_Number : %d\n",Allocate_Number));
                    STTBX_Print(("\tAllocate_KeepIndo : 0x%08x\n",Allocate_KeepInfo));

                    if (size != (4*4))
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,(4*4)));
                        ErrCode = ST_ERROR_BAD_PARAMETER;
                        fclose (fstream);
                        GUTIL_Free(Palette_p->Data_p);
                    }
                }

                Rest = Gamma_Header.width % 4;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 4;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 4) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocBlockParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_CLUT2;
                break;





            }



            case GAMMA_FILE_ALPHA1 :
            {
                Rest = Gamma_Header.width % 8;
                if (Rest == 0)
                {
                    Bitmap_p->Pitch = Gamma_Header.width / 8;
                }
                else
                {
                    Bitmap_p->Pitch = (Gamma_Header.width / 8) + 1;

                }
                if (Gamma_Header.Properties == 0)
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
                }
                else
                {
                    Bitmap_p->SubByteFormat = STGXOBJ_SUBBYTE_FORMAT_RPIX_LSB;
                }
                Bitmap_p->Size1 = Bitmap_p->Pitch * Gamma_Header.height;
                AllocBlockParams.Alignment = 1; /* Byte aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ALPHA1;
                break;
            }
            case GAMMA_FILE_RGB565 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocBlockParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_RGB565;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_ARGB8888 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*4;
                AllocBlockParams.Alignment = 4; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB8888;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*4;
                break;
            }

            case GAMMA_FILE_ARGB1555 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocBlockParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB1555;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_ARGB4444 :
            {
                Bitmap_p->Pitch            = Gamma_Header.width*2;
                AllocBlockParams.Alignment = 2; /* 16-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_ARGB4444;
                Bitmap_p->Size1            = Gamma_Header.nb_pixel*2;
                break;
            }
            case GAMMA_FILE_YCBCR422R :
            {
                if ((Gamma_Header.width % 2) == 0)
                {
                    Bitmap_p->Pitch = (U32)(Gamma_Header.width * 2);
                }
                else
                {
                    Bitmap_p->Pitch = (U32)((Gamma_Header.width - 1) * 2 + 4);
                }


                AllocBlockParams.Alignment = 4; /* 32-bits aligned       */
                Bitmap_p->ColorType        = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_422;
                Bitmap_p->Size1            = Bitmap_p->Pitch * Gamma_Header.height;
                break;
            }
            default :
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Type not supported 0x%08x\n",Gamma_Header.Type));
                ErrCode = ST_ERROR_BAD_PARAMETER;
                break;
            }
        } /* switch (Gamma_Header.Type) */
    }

    if (ErrCode == ST_NO_ERROR)
    {
        /* Allocate memory for bitmap                                */
        STTBX_Print(("Allocate for bitmap ...\n"));
        AllocBlockParams.Size = (U32)(Bitmap_p->Size1);
        GUTIL_Allocate(&AllocBlockParams, &buffer_p);
        if (buffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate for bitmap %d bytes\n",Bitmap_p->Size1));
            ErrCode = ST_ERROR_NO_MEMORY;
            fclose (fstream);
            if (Palette_p->Data_p != NULL)
            {
                GUTIL_Free(Palette_p->Data_p);
            }
        }
    }

    /* Read bitmap                                                   */
    if (ErrCode == ST_NO_ERROR)
    {
        Bitmap_p->BitmapType             = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        Bitmap_p->PreMultipliedColor     = FALSE;
        Bitmap_p->ColorSpaceConversion   = STGXOBJ_ITU_R_BT601;
        Bitmap_p->AspectRatio            = STGXOBJ_ASPECT_RATIO_SQUARE;
        Bitmap_p->Width                  = Gamma_Header.width;
        Bitmap_p->Height                 = Gamma_Header.height;
        Bitmap_p->Data1_p                = buffer_p;
        Bitmap_p->Offset = 0;
        Bitmap_p->BigNotLittle = 0;
        /* Data2 not needed, we are in RASTER_PROGRESSIVE            */
        /* Bitmap_p->Data2_p             =                           */
        /* Bitmap_p->Size2               =                           */

        STTBX_Print(("Reading file Bitmap data ... \n"));
        size = fread ((void *)(STAVMEM_VirtualToCPU((void*)Bitmap_p->Data1_p,&VirtualMapping)), 1,(Bitmap_p->Size1), fstream);
        if (size != (Bitmap_p->Size1))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Read error %d byte instead of %d\n",size,Bitmap_p->Size1));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            GUTIL_Free(Palette_p->Data_p);
            if (Palette_p->Data_p != NULL)
            {
                GUTIL_Free(Palette_p->Data_p);
            }

        }
        fclose (fstream);
    }

    return ErrCode;

}

/*-------------------------------------------------------------------------
 * Function : WaitForBlitterComplite
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t WaitForBlitterComplite()
{
    clock_t time;

    time = time_plus(time_now(), 15625*5);
    return semaphore_wait_timeout(&BlitCompletionTimeout,&time);
}



/*-------------------------------------------------------------------------
 * Function : BLIT_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL BLIT_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err;
    STBLIT_InitParams_t     InitParams;

    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#ifdef ST_5100
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
/*    InitParams.SingleBlitNodeBuffer_p             = NULL;*/

    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
/*    InitParams.JobBlitNodeBuffer_p                = NULL;*/

    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
/*    InitParams.JobBlitBuffer_p        = NULL;*/

    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
/*    InitParams.JobBuffer_p                        = NULL;*/

    InitParams.WorkBufferUserAllocated              = FALSE;
/*    InitParams.WorkBuffer_p                       = NULL;*/
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);

#ifndef STBLIT_EMULATOR
#if defined (ST_5528) || (ST_5100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif
#endif


    Err = STBLIT_Init(STBLIT_DEVICE_NAME,&InitParams);
    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STBLIT_Init() : OK\n"));
    }
    else
    {
        STTBX_Print (("STBLIT_Init() : %d\n",Err));
        return (TRUE);
    }

    return Err;
}

/*-------------------------------------------------------------------------
 * Function : BLIT_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL BLIT_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t          Err;
    STBLIT_TermParams_t     TermParams;

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(STBLIT_DEVICE_NAME,&TermParams);
    if (Err == ST_NO_ERROR)
    {
        STTBX_Print (("STBLIT_Term() : OK\n"));
    }
    else
    {
        STTBX_Print (("STBLIT_Term() : %d\n",Err));
        return (TRUE);
    }

    return(Err);
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortSource
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL BLIT_SetSource (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL Err;

    Err = STTST_GetString( pars_p,"C:/Scripts/suzie.gam", SrcFileName, sizeof(SrcFileName) );

    return Err;
}



/*-------------------------------------------------------------------------
 * Function : BLIT_Display_Test
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL BLIT_Display_Test (STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL Err;
    STBLIT_InitParams_t     InitParams;
    STBLIT_OpenParams_t     OpenParams;
    STBLIT_TermParams_t     TermParams;
    STBLIT_Handle_t         Handle;
    STBLIT_BlitContext_t    Context;
    STBLIT_Source_t         Src;
    STBLIT_Destination_t    Dst;

    STGXOBJ_Rectangle_t     Rectangle, ClearRectangle;
    STGXOBJ_Color_t         Color, ClearColor;
    STGXOBJ_Palette_t       Palette;

    STEVT_OpenParams_t      EvtOpenParams;
    STEVT_Handle_t          EvtHandle;
    STEVT_DeviceSubscribeParams_t EvtSubscribeParams;

    STEVT_SubscriberID_t    SubscriberID;
    int                     i,j;
    char                    Value;
    int                     Step = 2;


    /* ------------ Blit Device Init ------------ */
    InitParams.CPUPartition_p                       = SystemPartition_p;
    InitParams.DeviceType                           = CURRENT_DEVICE_TYPE;
    InitParams.BaseAddress_p                        = CURRENT_BASE_ADDRESS;
    InitParams.AVMEMPartition                       = AvmemPartitionHandle[0];
    InitParams.SharedMemoryBaseAddress_p            = (void*)TEST_SHARED_MEM_BASE_ADDRESS;
    InitParams.MaxHandles                           = 1;

    InitParams.SingleBlitNodeBufferUserAllocated    = FALSE;
#ifdef ST_5100
    InitParams.SingleBlitNodeMaxNumber              = 1000;
#else
    InitParams.SingleBlitNodeMaxNumber              = 30;
#endif
    InitParams.JobBlitNodeBufferUserAllocated       = FALSE;
    InitParams.JobBlitNodeMaxNumber                 = 30;
    InitParams.JobBlitBufferUserAllocated = FALSE;
    InitParams.JobBlitMaxNumber       = 30;
    InitParams.JobBufferUserAllocated               = FALSE;
    InitParams.JobMaxNumber                         = 2;
    InitParams.WorkBufferUserAllocated              = FALSE;
    InitParams.WorkBufferSize             = WORK_BUFFER_SIZE;
    strcpy(InitParams.EventHandlerName,STEVT_DEVICE_NAME);
#if defined (ST_5528) || (ST_5100)
    InitParams.BlitInterruptLevel   = BLITTER_INT_LEVEL ;
    InitParams.BlitInterruptNumber  = BLITTER_INT_EVENT;
#else
    InitParams.BlitInterruptEvent = BLITTER_INT_EVENT;
    strcpy(InitParams.BlitInterruptEventName,STINTMR_DEVICE_NAME);
#endif

    Err = STBLIT_Init(STBLIT_DEVICE_NAME,&InitParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print (("Err Init : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Open ------------ */
    Err = STBLIT_Open(STBLIT_DEVICE_NAME,&OpenParams,&Handle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print (("Err Open : %d\n",Err));
        return (TRUE);
    }


    /* ------------ Initialize global semaphores ------------ */
	#ifdef ST40_OS21
		BlitCompletionTimeout= semaphore_create_fifo(0);
	#else
		semaphore_init_fifo_timeout(&BlitCompletionTimeout, 0);
	#endif /*End of ST40_OS21*/

    /* ------------ Open Event handler -------------- */
    Err = STEVT_Open(STEVT_DEVICE_NAME, &EvtOpenParams, &EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print (("Err Event Open : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Subscribe to Blit Completed event ---------------- */
    EvtSubscribeParams.NotifyCallback     = BlitCompletedHandler;
    EvtSubscribeParams.SubscriberData_p   = NULL;
    Err = STEVT_SubscribeDeviceEvent(EvtHandle,STBLIT_DEVICE_NAME,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print (("Err Subscribe Blit completion : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Get Subscriber ID ------------ */
    STEVT_GetSubscriberID(EvtHandle,&SubscriberID);
    Context.EventSubscriberID   = SubscriberID;
    Context.UserTag_p   = &UserTagString;

    /* ------------ Set Src ------------ */
    Err = ConvertGammaToBitmap(SrcFileName,&SourceBitmap,&Palette);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print (("Err Convert Gamma to Bitmap : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Set ClearColor ------------ */
    ClearColor.Type               = STGXOBJ_COLOR_TYPE_RGB565 ;
    ClearColor.Value.RGB565.R     = 0x50 ;
    ClearColor.Value.RGB565.G     = 0x50 ;
    ClearColor.Value.RGB565.B     = 0x50 ;

    /* ------------ Set ClearRectangle ------------ */
    ClearRectangle.PositionX = 0;
    ClearRectangle.PositionY = 0;
    ClearRectangle.Width     = BitmapUnknown.Width;
    ClearRectangle.Height    = BitmapUnknown.Height;

    /* ------------ Set General Context ------------ */
    Context.ColorKeyCopyMode         = STBLIT_COLOR_KEY_MODE_NONE;
    Context.AluMode                  = STBLIT_ALU_COPY;
    Context.EnableClipRectangle      = FALSE;
    Context.EnableMaskWord           = FALSE;
    Context.EnableMaskBitmap         = FALSE;
    Context.EnableColorCorrection    = FALSE;
    Context.Trigger.EnableTrigger    = FALSE;
    Context.EnableFlickerFilter      = FALSE;
    Context.JobHandle                = STBLIT_NO_JOB_HANDLE;
    Context.NotifyBlitCompletion     = TRUE;
    Context.GlobalAlpha              = 60;

    /*
     * Test 1 : Test Copy feature
     * ==========================
     */

    /* ------------ Print Infos ------------ */
    STTBX_Print(("***********************************\n"));
    STTBX_Print((" Test 1 : Copy feature\n"));
    STTBX_Print(("***********************************\n"));

    /* ------------ Reset Bitmap ------------ */
    Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&ClearRectangle,&ClearColor,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait Blit Complit ------------ */
    Err = WaitForBlitterComplite();
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : Time Out\n"));
        return (TRUE);
    }

    for ( j=20;j<=200;j+=Step)
    {
        for ( i=20;i<BitmapUnknown.Width-40;i+=Step)
        {
            /* ------------ Set Rectangle ------------ */
            Rectangle.PositionX = i;
            Rectangle.PositionY = j;
            Rectangle.Width     = 20;
            Rectangle.Height    = 20;

            /* ------------ Blit ------------ */
            Err = STBLIT_CopyRectangle(Handle,&SourceBitmap,&Rectangle,&BitmapUnknown,Rectangle.PositionX,Rectangle.PositionY,&Context );
            if (Err != ST_NO_ERROR)
            {
                STTBX_Print(("Rectangle=(%d,%d,%d,%d) STBLIT_CopyRectangle(...)=%d",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height,Err));
                return (TRUE);
            }

            /* ------------ Wait Blit Complit ------------ */
            Err = WaitForBlitterComplite();
            if (Err != ST_NO_ERROR)
            {
                STTBX_Print(("Rectangle=(%d,%d,%d,%d) STBLIT_CopyRectangle(...)= TIME OUT",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height));
                return (TRUE);
            }
        }
    }

    /* ------------ Wait User confirm ------------ */
    STTBX_Print(("> Is the display OK on the screen ?"));
    STTBX_InputChar(&Value);
    if ( Value!='y' && Value!='Y')
    {
        STTBX_Print(("\nTEST FAILED !\n\n"));
        return (TRUE);
    }
    else
    {
        STTBX_Print(("\nTEST PASSED\n\n"));
    }


    /*
     * Test 2 : Test Clip feature
     * ==========================
     */

    /* ------------ Reset Bitmap ------------ */
    Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&ClearRectangle,&ClearColor,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait Blit Complit ------------ */
    Err = WaitForBlitterComplite();
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : Time Out\n"));
        return (TRUE);
    }

    /* ------------ Init Src & Dst ------------ */
    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = NULL;

    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = BitmapUnknown.Width-40;
    Dst.Rectangle.Height      = BitmapUnknown.Height-40;
    Dst.Bitmap_p              = &BitmapUnknown;
    Dst.Palette_p             = NULL;

    /* ------------ Set Specific Context ------------ */
    Context.EnableClipRectangle      = TRUE;
    Context.WriteInsideClipRectangle = FALSE;

    /* ------------ Print Infos ------------ */
    STTBX_Print(("***********************************\n"));
    STTBX_Print((" Test 2 : Clip feature\n"));
    STTBX_Print(("***********************************\n"));

    for ( i=BitmapUnknown.Width-80;i>100;i-=Step)
    {
        /* ------------ Set Rectangle ------------ */
        Rectangle.PositionX   = 40;
        Rectangle.PositionY   = 40;
        Rectangle.Width       = i;
        Rectangle.Height      = BitmapUnknown.Height-80;
        Context.ClipRectangle = Rectangle;

        /* ------------ Blit ------------ */
        Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("ClipRectangle=(%d,%d,%d,%d) STBLIT_Blit(...)=%d",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height,Err));
            return (TRUE);
        }

        /* ------------ Wait Blit Complit ------------ */
        Err = WaitForBlitterComplite();
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("ClipRectangle=(%d,%d,%d,%d) STBLIT_Blit(...)= TIME OUT",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height));
            return (TRUE);
        }
    }
    for ( j=BitmapUnknown.Height-100;j>=0;j-=Step)
    {
        /* ------------ Set Rectangle ------------ */
        Rectangle.Height      = j;
        Context.ClipRectangle = Rectangle;
        if ( j<=10)
        {
            Context.EnableClipRectangle      = FALSE;
        }


        /* ------------ Blit ------------ */
        Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("ClipRectangle=(%d,%d,%d,%d) STBLIT_Blit(...)=%d",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height,Err));
            return (TRUE);
        }

        /* ------------ Wait Blit Complit ------------ */
        Err = WaitForBlitterComplite();
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("ClipRectangle=(%d,%d,%d,%d) STBLIT_Blit(...)= TIME OUT",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height));
            return (TRUE);
        }
    }

    /* ------------ Set Specific Context ------------ */
    Context.EnableClipRectangle      = FALSE;

    /* ------------ Wait User confirm ------------ */
    STTBX_Print(("> Is the display OK on the screen ?"));
    STTBX_InputChar(&Value);
    if ( Value!='y' && Value!='Y')
    {
        STTBX_Print(("\nTEST FAILED !\n\n"));
        return (TRUE);
    }
    else
    {
        STTBX_Print(("\nTEST PASSED\n\n"));
    }


    /*
     * Test 3 : Test Fill feature
     * ==========================
     */

    /* ------------ Reset Bitmap ------------ */
    Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&ClearRectangle,&ClearColor,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait Blit Complit ------------ */
    Err = WaitForBlitterComplite();
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : Time Out\n"));
        return (TRUE);
    }

    /* ------------ Set Color ------------ */
    Color.Type                 = STGXOBJ_COLOR_TYPE_RGB565 ;
    Color.Value.RGB565.R     = 0x00 ;
    Color.Value.RGB565.G     = 0x00 ;
    Color.Value.RGB565.B     = 0xff ;

    /* ------------ Print Infos ------------ */
    STTBX_Print(("***********************************\n"));
    STTBX_Print((" Test 3 : Fill feature\n"));
    STTBX_Print(("***********************************\n"));

    for ( j=20;j<=200;j+=Step)
    {
        for ( i=20;i<BitmapUnknown.Width-40;i+=Step)
        {
            /* ------------ Set Rectangle ------------ */
            Rectangle.PositionX = i;
            Rectangle.PositionY = j;
            Rectangle.Width     = 20;
            Rectangle.Height    = 20;

            /* ------------ Set Color ------------ */
            Color.Value.RGB565.R     += j ;
            Color.Value.RGB565.G     += i ;
            Color.Value.RGB565.B     -= j ;


            /* ------------ Blit ------------ */
            Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&Rectangle,&Color,&Context );
            if (Err != ST_NO_ERROR)
            {
                STTBX_Print(("Rectangle=(%d,%d,%d,%d) STBLIT_FillRectangle(...)=%d",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height,Err));
                return (TRUE);
            }

            /* ------------ Wait Blit Complit ------------ */
            Err = WaitForBlitterComplite();
            if (Err != ST_NO_ERROR)
            {
                STTBX_Print(("Rectangle=(%d,%d,%d,%d) STBLIT_FillRectangle(...)= TIME OUT",Rectangle.PositionX,Rectangle.PositionY,Rectangle.Width,Rectangle.Height));
                return (TRUE);
            }
        }
    }

    /* ------------ Wait User confirm ------------ */
    STTBX_Print(("> Is the display OK on the screen ?"));
    STTBX_InputChar(&Value);
    if ( Value!='y' && Value!='Y')
    {
        STTBX_Print(("\nTEST FAILED !\n\n"));
        return (TRUE);
    }
    else
    {
        STTBX_Print(("\nTEST PASSED\n\n"));
    }

    /*
     * Test 4 : Test Resize feature
     * ============================
     */

    /* ------------ Reset Bitmap ------------ */
    Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&ClearRectangle,&ClearColor,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait Blit Complit ------------ */
    Err = WaitForBlitterComplite();
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : Time Out\n"));
        return (TRUE);
    }


    /* ------------ Init Src & Dst ------------ */
    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = NULL;

    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Height      = 150;
    Dst.Bitmap_p              = &BitmapUnknown;
    Dst.Palette_p             = NULL;



    /* ------------ Print Infos ------------ */
    STTBX_Print(("***********************************\n"));
    STTBX_Print((" Test 4 : Resize feature\n"));
    STTBX_Print(("***********************************\n"));

    for ( i=5;i<BitmapUnknown.Width-40;i+=Step)
    {
        /* ------------ Set Dst ------------ */
        Dst.Rectangle.Width       = i;

        /* ------------ Blit ------------ */
        Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Destination=(%d,%d,%d,%d) STBLIT_Blit(...)=%d",Dst.Rectangle.PositionX,Dst.Rectangle.PositionY,Dst.Rectangle.Width,Dst.Rectangle.Height,Err));
            return (TRUE);
        }

        /* ------------ Wait Blit Complit ------------ */
        Err = WaitForBlitterComplite();
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("Destination=(%d,%d,%d,%d) STBLIT_Blit(...)= TIME OUT",Dst.Rectangle.PositionX,Dst.Rectangle.PositionY,Dst.Rectangle.Width,Dst.Rectangle.Height));
            return (TRUE);
        }
    }

    for ( i=155;i<BitmapUnknown.Height-40;i+=Step)
    {
        /* ------------ Set Dst ------------ */
        Dst.Rectangle.Height      = i;

        /* ------------ Blit ------------ */
        Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("STBLIT_BLIT(%d,%d,%d,%d)=%d",Dst.Rectangle.PositionX,Dst.Rectangle.PositionY,Dst.Rectangle.Width,Dst.Rectangle.Height,Err));
            return (TRUE);
        }

        /* ------------ Wait Blit Complit ------------ */
        Err = WaitForBlitterComplite();
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("STBLIT_BLIT(%d,%d,%d,%d)= TIME OUT",Dst.Rectangle.PositionX,Dst.Rectangle.PositionY,Dst.Rectangle.Width,Dst.Rectangle.Height));
            return (TRUE);
        }
    }

    /* ------------ Wait User confirm ------------ */
    STTBX_Print(("> Is the display OK on the screen ?"));
    STTBX_InputChar(&Value);
    if ( Value!='y' && Value!='Y')
    {
        STTBX_Print(("\nTEST FAILED !\n\n"));
        return (TRUE);
    }
    else
    {
        STTBX_Print(("\nTEST PASSED\n\n"));
    }

    /*
     * Test 5 : Test Blend feature
     * ==========================
     */

    /* ------------ Reset Bitmap ------------ */
    Err = STBLIT_FillRectangle(Handle,&BitmapUnknown,&ClearRectangle,&ClearColor,&Context );
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Wait Blit Complit ------------ */
    Err = WaitForBlitterComplite();
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Blit Fill : Time Out\n"));
        return (TRUE);
    }

    /* ------------ Init Src & Dst ------------ */
    Src.Type                 = STBLIT_SOURCE_TYPE_BITMAP;
    Src.Data.Bitmap_p        = &SourceBitmap;
    Src.Rectangle.PositionX  = 0;
    Src.Rectangle.PositionY  = 0;
    Src.Rectangle.Width      = SourceBitmap.Width;
    Src.Rectangle.Height     = SourceBitmap.Height;
    Src.Palette_p            = NULL;

    Dst.Rectangle.PositionX   = 20;
    Dst.Rectangle.PositionY   = 20;
    Dst.Rectangle.Width       = BitmapUnknown.Width-40;
    Dst.Rectangle.Height      = BitmapUnknown.Height-40;
    Dst.Bitmap_p              = &BitmapUnknown;
    Dst.Palette_p             = NULL;

    /* ------------ Set Specific Context ------------ */
    Context.AluMode                 = STBLIT_ALU_ALPHA_BLEND;

    /* ------------ Print Infos ------------ */
    STTBX_Print(("***********************************\n"));
    STTBX_Print((" Test 5 : Blend feature\n"));
    STTBX_Print(("***********************************\n"));

    i=0;
    for ( j=0;j<=500;j++)
    {
        if ( j%5==0 )
        {
            i++;
        }

        /* ------------ Set GlobalAlpha ------------ */
        Context.GlobalAlpha       = i;

        /* ------------ Blit ------------ */
        Err = STBLIT_Blit(Handle,NULL,&Src,&Dst,&Context );
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("GlobalAlpha=%d STBLIT_Blit(...)=%d",Context.GlobalAlpha,Err));
            return (TRUE);
        }

        /* ------------ Wait Blit Complit ------------ */
        Err = WaitForBlitterComplite();
        if (Err != ST_NO_ERROR)
        {
            STTBX_Print(("GlobalAlpha=%d STBLIT_Blit(...)= TIME OUT",Context.GlobalAlpha));
            return (TRUE);
        }
    }

    /* ------------ Wait User confirm ------------ */
    STTBX_Print(("> Is the display OK on the screen ?"));
    STTBX_InputChar(&Value);
    if ( Value!='y' && Value!='Y')
    {
        STTBX_Print(("\nTEST FAILED !\n\n"));
        return (TRUE);
    }
    else
    {
        STTBX_Print(("\nTEST PASSED\n\n"));
    }



    /* --------------- Free Bitmap ------------ */
/*    GUTIL_Free(SourceBitmap.Data1_p);*/
/*    GUTIL_Free(TargetBitmap.Data1_p);*/

    /* --------------- Close Evt ----------*/
    Err = STEVT_Close(EvtHandle);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Event Close : %d\n",Err));
        return (TRUE);
    }

    /* ------------ Blit Term ------------ */
    TermParams.ForceTerminate = TRUE;
    Err = STBLIT_Term(STBLIT_DEVICE_NAME,&TermParams);
    if (Err != ST_NO_ERROR)
    {
        STTBX_Print(("Err Term : %d\n",Err));
        return (TRUE);
    }

    return ( Err );
}




/*-------------------------------------------------------------------------
 * Function : BLIT_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : FALSE if error, TRUE if success
 * ----------------------------------------------------------------------*/
static BOOL BLIT_InitCommand(void)
{
    BOOL RetErr = FALSE;

    RetErr  = STTST_RegisterCommand( "BLIT_Init", BLIT_Init, "BLIT Init");
    RetErr |= STTST_RegisterCommand( "BLIT_Term", BLIT_Term, "BLIT Term");
    RetErr |= STTST_RegisterCommand( "BLIT_Display_Test", BLIT_Display_Test, "Test display with BLIT");
    RetErr |= STTST_RegisterCommand( "BLIT_SetSource", BLIT_SetSource, "Set Blit Source");

   return(!RetErr);
}

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL BLIT_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = BLIT_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "BLIT_RegisterCmd()     \t: ok           %s\n", STBLIT_GetRevision()));
    }
    else
    {
        STTBX_Print(( "BLIT_RegisterCmd()     \t: failed !\n" ));
    }
    return(RetOk);
}
