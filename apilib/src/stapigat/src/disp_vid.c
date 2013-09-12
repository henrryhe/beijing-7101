/*******************************************************************************

File name   : disp_vid.c

Description : module allowing to display a single video bitmap on all chips :
              - Loading a file into avmem. (file must be of type macro blocked
                omega1 or omega2 4:2:0.)
              - Setting video registers for decoding this loaded picture.

              To really see the picture on the output, do the following :
              - Compile VTG with variable STVTG_VSYNC_WITHOUT_VIDEO (for omega1)
              - Initialize & set DENC, VOUT, VMIX & VTG.
              - Then call the followings layer functions:
                * Init, Open, OpenViewport & Enable for video
                * Then AdjustViewPortParams & SetViewPortParams
              - Then connect the layer to the mixer
              - Finally for omega2, call SetViewPortParams again

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
28 June 2002       Created                                           BS
14 Oct 2002        Add support for 5517                              HSdLM
05 May 2003        Add support for 7020 STEM                         HSdLM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include "testcfg.h"

#ifdef USE_DISP_VIDEO
#ifdef ST_OSLINUX
#include "compat.h"
#include "iocstapigat.h"
#endif

#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"
#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
#include "stlayer.h"
#else
#include "stavmem.h"
#include "clavmem.h"
#endif
#include "api_cmd.h"

#include "disp_vid.h"

#ifdef USE_TBX
#include "sttbx.h"
#endif

#ifdef USE_TESTTOOL
#include "testtool.h"
#endif

#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
STLAYER_Handle_t FakeLayHndl = 0x123 ;
#endif


#ifdef USE_DISPVID
#define OS21_SHIFT                         0xA0000000
#define AVG_MEMORY_BASE_ADDRESS            0xB3768000
static U32  AVGMemAddress = AVG_MEMORY_BASE_ADDRESS;
#define GAM_MIX_CTL                 0x00
#define GAM_MIX_AVO                 0x28
#define GAM_MIX_AVS                 0x2C
#define GAM_MIX_CRB                 0x34
#define DSPCFG_CLK                  0x070
#define PERIPH_BASE                 0xB8000000
#define VOS_BASE                    (PERIPH_BASE + 0x01005000)
#if defined (ST_7100)
#define GAM_MIX1_BASE_ADDRESS       ST7100_VMIX1_BASE_ADDRESS       /* mixer 1 */
#elif defined (ST_7109)
#define GAM_MIX1_BASE_ADDRESS       ST7109_VMIX1_BASE_ADDRESS       /* mixer 1 */
#elif defined (ST_7200)
#define GAM_MIX1_BASE_ADDRESS       ST7200_VMIX1_BASE_ADDRESS       /* mixer 1 */

#endif
#endif
/* External Variables --------------------------------------------------------- */

/* Private Constants -------------------------------------------------------- */

#if defined (ST_5510) || defined (ST_5512) || defined (ST_5508) || defined (ST_5518) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define DISP_VIDEO_OMEGA1

#ifndef DISP_VIDEO_FILENAME
#if defined (ST_5516) || defined (ST_5517)
#define DISP_VIDEO_FILENAME "o1pictyuv128.gam"
#else /* ST_5516, ST_5517 */
#define DISP_VIDEO_FILENAME "o1pictyuv.gam"
#endif /* ST_5516, ST_5517 */
#endif /* DISP_VIDEO_FILENAME */
#define HALDISP_Write8(Address_p, Value)  STSYS_WriteRegDev8((void *) (Address_p), (Value))
#define HALDISP_Write16(Address_p, Value) STSYS_WriteRegDev16BE((void *) (Address_p), (Value))
#define HALDISP_Write32(Address_p, Value) STSYS_WriteRegDev32BE((void *) (Address_p), (Value))
#endif /* 55XX */

#if defined (ST_7015) || defined (ST_7020)
#define DISP_VIDEO_OMEGA2
#define DISP_VIDEO_MAX_VIDEO 2
#ifndef DISP_VIDEO_FILENAME
#define DISP_VIDEO_FILENAME "o2pictyuv.gam"
#define HALDISP_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))
#endif /* #ifndef DISP_VIDEO_FILENAME */

#if defined (ST_7015)
const void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(STI7015_BASE_ADDRESS+ST7015_VID1_OFFSET), /* Video 1 base address */
    (void *)(STI7015_BASE_ADDRESS+ST7015_VID2_OFFSET)  /* Video 2 base address */
};
#elif !defined (ST_5528)
const void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(STI7020_BASE_ADDRESS+ST7020_VID1_OFFSET), /* Video 1 base address */
    (void *)(STI7020_BASE_ADDRESS+ST7020_VID2_OFFSET)  /* Video 2 base address */
};
#endif /* ST_7015 */
#define WA_GNBvd06019
#endif /* #if defined (ST_7015) || defined (ST_7020) */

#if defined (ST_5528) || defined (ST_7710) || defined (ST_7100)
    #define DISP_VIDEO_5528
    #define DISP_VIDEO_MAX_VIDEO 2
    #ifndef DISP_VIDEO_FILENAME
    #define DISP_VIDEO_FILENAME "5528pictyuv.gam"
    #define HALDISP_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))
    #endif /* #ifndef DISP_VIDEO_FILENAME */

#elif defined (ST_7109)
    #define DISP_VIDEO_DISPLAYPIPE  /* For MAIN display */
    #define DISP_VIDEO_5528         /* For AUX display */
    #define DISP_VIDEO_MAX_VIDEO 2
    #ifndef DISP_VIDEO_FILENAME
    #define DISP_VIDEO_FILENAME "5528pictyuv.gam"
    #define HALDISP_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))
    #endif /* #ifndef DISP_VIDEO_FILENAME */
#elif defined (ST_7200)
    #define DISP_VIDEO_DISPLAYPIPE  /* For MAIN display */
    #define DISP_VIDEO_MAX_VIDEO 3
    #ifndef DISP_VIDEO_FILENAME
    #define DISP_VIDEO_FILENAME "5528pictyuv.gam"
    #define HALDISP_Write32(Address_p, Value) STSYS_WriteRegDev32LE((void *) (Address_p), (Value))
    #endif /* #ifndef DISP_VIDEO_FILENAME */
#endif /* ST_5528 | ST_7710 | ST_7100 | ST_7109 */

#ifdef ST_OSLINUX
extern MapParams_t              Disp0Map;
extern MapParams_t              Disp1Map;
#if defined (ST_7200)
extern MapParams_t              Disp2Map;
#endif
void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO];

#else
#if defined (ST_5528)
const void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(ST5528_DISP0_BASE_ADDRESS), /* Video Display 1 base address */
    (void *)(ST5528_DISP1_BASE_ADDRESS)  /* Video Display 2 base address */
};
#elif defined (ST_7710)
const void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(ST7710_DISP0_BASE_ADDRESS), /* Video Display 1 base address */
    (void *)(ST7710_DISP1_BASE_ADDRESS)  /* Video Display 2 base address */
};
#elif defined (ST_7100)
void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(ST7100_DISP0_BASE_ADDRESS), /* Video Display 1 base address */
    (void *)(ST7100_DISP1_BASE_ADDRESS)  /* Video Display 2 base address */
};
#elif defined (ST_7109)
void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(ST7109_DISP0_BASE_ADDRESS), /* Video Display 1 base address */
    (void *)(ST7109_DISP1_BASE_ADDRESS)  /* Video Display 2 base address */
};
#elif defined (ST_7200)
const void * VideoDecAdd_p[DISP_VIDEO_MAX_VIDEO] =
{
    (void *)(ST7200_VDP_MAIN_BASE_ADDRESS), /* Video Display 1 base address */
    (void *)(ST7200_VDP_AUX1_BASE_ADDRESS), /* Video Display 2 base address */
    (void *)(ST7200_VDP_AUX2_BASE_ADDRESS)  /* Video Display 3 base address */
};
#endif /* #ifdef 5528-7710-7100-7109 */
#endif  /* ST_OSLINUX */

/* VIDEO OMEGA1 defines */
#define VID_DFP16       0x0C  /* Display Frame pointer [13:0] luma            */
#define VID_XFW         0x28  /* Video displayed frame width, [7:0]           */
#define VID_DFC16       0x58  /* Display Frame pointer [13:0] chroma          */

/* VIDEO OMEGA2 defines */
#define VIDn_PFH_8      0x70  /* Presentation Frame Height                    */
#define VIDn_PFH_MASK   0x7F
#define VIDn_PFW_8      0x6C  /* Presentation Frame Width                     */
#define VIDn_APFP_32    0x30  /* auxillary presentation Luma Frame pointer    */
#define VIDn_APCHP_32   0x34  /* auxillary presentation chroma frame buffer   */
#define VIDn_PFP_32     0x28  /* Main Presentation Luma Frame Pointer         */
#define VIDn_PCHP_32    0x2C  /* Main Presentation Chroma Buffer              */

/* VIDEO DISPLAY 5528 defines */
#define DISP_LUMA_BA    0x084 /* Display Luma   Source Pixmap Memory Location */
#define DISP_CHR_BA     0x088 /* Display Chroma Source Pixmap Memory Location */
#define DISP_PMP        0x08c /* Display Pixmap Memory Pitch */

/* VIDEO DISP_VIDEO_DISPLAYPIPE defines */
#define VDP_DEI_CYF_BA  0x18  /* Current video field, luma buffer base address   */
#define VDP_DEI_CCF_BA  0x24  /* Current video field, chroma buffer base address */

/* Private Macros ----------------------------------------------------------- */
#ifdef USE_TBX
#define LOCAL_PRINT(x) STTBX_Print(x)
#else
#define LOCAL_PRINT(x) printf x
#endif

/* Header definition for file */
#define DISP_VID_OMEGA1_TYPE_MB_420 0x00
#define DISP_VID_OMEGA2_TYPE_MB_420 0x94
#define DISP_VID_5528_TYPE_MB_420   0x01
#define DISP_VID_SIGNATURE          0x420F

typedef struct
{
    U16 Header_size;
    U16 Signature;
    U16 Type;
    U8  Properties;
    U8  AspectRatio;
    U32 Width;
    U32 Height;
    U32 LumaSize;
    U32 ChromaSize;
} DISPVID_Header_t;

/* Private Function prototypes ---------------------------------------------- */

/*******************************************************************************
Name        : VIDDISP_ShowPicture
Description : Show specified picture on display instead of current display
Parameters  : Luma & chroma buffer address, horizontal & vertical picture size
Assumptions :
Limitations : Buffer address are the wrote on main & aux processor.
Returns     : None
*******************************************************************************/
static void DispVideo_ShowPicture(void * const LumaAddress_p,
                                  void * const ChromaAddress_p,
                                  const U32 HorizontalSize,
                                  const U32 VerticalSize,
                                  const U8 LayerIdentity)
{
#ifdef DISP_VIDEO_OMEGA1
    U8 WidthInMacroBlock = (HorizontalSize + 15) / 16;

    /* Beginning of the luma frame buffer to display, in unit of 2 Kbits */
    HALDISP_Write16((U8 *) VID_DFP16, ((U32) LumaAddress_p) / 256);

    /* Beginning of the chroma frame buffer to display, in unit of 2 Kbits */
    HALDISP_Write16((U8 *) VID_DFC16 , ((U32) ChromaAddress_p) / 256);

    /* Write width in macroblocks */
    HALDISP_Write8((U8 *) VID_XFW, WidthInMacroBlock);
#endif /* DISP_VIDEO_OMEGA1 */

#if defined DISP_VIDEO_OMEGA2
    void * RegistersBaseAddress_p;
    U8 WidthInMacroBlock ;
    U8 HeightInMacroBlock ;
    U8 i;

    for(i=0;i<DISP_VIDEO_MAX_VIDEO;i++)
    {
        RegistersBaseAddress_p = (void*) VideoDecAdd_p[i];
        /* main layer */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_PFP_32, (U32) LumaAddress_p);

        /* aux layer */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_APFP_32, (U32) LumaAddress_p);

        /* main layer */
        /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_PCHP_32, (U32) ChromaAddress_p);

        /* aux layer */
        /* Beginning of the chroma frame buffer to display, in unit of 512 bytes */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_APCHP_32, (U32) ChromaAddress_p);

        WidthInMacroBlock = (HorizontalSize + 15) / 16;

        /* Write width in macroblocks */
        HALDISP_Write32((U8 *) VID_XFW, WidthInMacroBlock);

        /* width in macroblocks */
        WidthInMacroBlock = (HorizontalSize + 15) / 16;

        /* Height in macroblocks */
        HeightInMacroBlock = (VerticalSize + 15) / 16;

        /* Write width in macroblocks in presentation width buffer */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_PFW_8, WidthInMacroBlock);

        /* Write Height in macroblocks in presentation Height buffer */
#if defined WA_GNBvd06019
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_PFH_8, HeightInMacroBlock + 1);
#else
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VIDn_PFH_8, HeightInMacroBlock);
#endif /* WA_GNBvd06019 */
    }
#else
    UNUSED_PARAMETER(VerticalSize);
#endif /* DISP_VIDEO_OMEGA2 */

#if defined DISP_VIDEO_5528
#if defined(ST_7109)
    if (LayerIdentity == 1)     /* 7109 AUX display */
#else
    UNUSED_PARAMETER(LayerIdentity);
#endif  /* ST_7109 */
    {
    void * RegistersBaseAddress_p;
    U32 MemoryPitch ;
    U8 Index;

#ifdef ST_OSLINUX
    VideoDecAdd_p[0] = (void *)(Disp0Map.MappedBaseAddress); /* Video Display 1 base address */
    VideoDecAdd_p[1] = (void *)(Disp1Map.MappedBaseAddress); /* Video Display 2 base address */
#endif

    MemoryPitch = (HorizontalSize + 15) & (~15);

#if defined(ST_7109)
        /* For 7109: only AUX display must be written to */
        Index=1;
#else
    for (Index=0; Index<DISP_VIDEO_MAX_VIDEO; Index++)
#endif  /* ST_7109 */
    {
        RegistersBaseAddress_p = (void *) VideoDecAdd_p[Index];

        /* Set Frame Buffer location */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + DISP_LUMA_BA, (U32) LumaAddress_p);
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + DISP_CHR_BA, (U32) ChromaAddress_p);

        /* Write pixmap memory pitch */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + DISP_PMP, (U32) MemoryPitch);
    }
    }
#endif /* DISP_VIDEO_5528 */

#if defined DISP_VIDEO_DISPLAYPIPE
#if defined(ST_7109)
    if (LayerIdentity == 0)
    {
        void * RegistersBaseAddress_p;

#ifdef ST_OSLINUX
        VideoDecAdd_p[0] = (void *)(Disp0Map.MappedBaseAddress); /* Video Display 1 base address */
        VideoDecAdd_p[1] = (void *)(Disp1Map.MappedBaseAddress); /* Video Display 2 base address */
#endif

        /* Only 7109 MAIN display must be written to */
        RegistersBaseAddress_p = (void*) VideoDecAdd_p[0];

        /* Set Frame Buffer location */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VDP_DEI_CYF_BA, (U32) LumaAddress_p + 8);
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VDP_DEI_CCF_BA, (U32) ChromaAddress_p + 8);
    }
#endif  /* 7109 */

#if defined(ST_7200)

       void * RegistersBaseAddress_p;

#ifdef ST_OSLINUX
        VideoDecAdd_p[0] = (void *)(Disp0Map.MappedBaseAddress); /* Video Display 1 base address */
        VideoDecAdd_p[1] = (void *)(Disp1Map.MappedBaseAddress); /* Video Display 2 base address */
        VideoDecAdd_p[2] = (void *)(Disp2Map.MappedBaseAddress); /* Video Display 3 base address */
#endif

        /* Only 7109 MAIN display must be written to */
        RegistersBaseAddress_p = (void*) VideoDecAdd_p[LayerIdentity];

        /* Set Frame Buffer location */
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VDP_DEI_CYF_BA, (U32) LumaAddress_p + 8);
        HALDISP_Write32((U8 *) RegistersBaseAddress_p + VDP_DEI_CCF_BA, (U32) ChromaAddress_p + 8);

#endif  /* 7200 */

#endif /* DISP_VIDEO_DISPLAYPIPE */

} /* End of VIDDISP_ShowPicture() function */


/*******************************************************************************
Name        : DispVideo_Display
Description : Load a macro blocked image in avmem and set video registers
Parameters  : Pointer to return loaded add
Assumptions : STAVMEM is initialized and open.
Limitations :
Returns     : none.
*******************************************************************************/
void VIDEO_Display(void** DataAdd_p_p, U32* BitmapWidth_p, U32* BitmapHeight_p, U8 LayerIdentity, char Disp_Video_FileName[80])
{
#ifndef USE_DISPVID
#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
    STLAYER_AllocDataParams_t LayAllocData;
#else
    STAVMEM_AllocBlockParams_t AllocBlockParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STAVMEM_BlockHandle_t BlockHandle;
#endif  /* LINUX */

#else
U32 ExpectedSize;
#endif  /* USE_DISPVID */

    void * AddDevice_p;
    FILE * fstream_p;
    char FileName[80];
    U32 size;
    DISPVID_Header_t Header;
    BOOL Exit = FALSE;

    /*sprintf(FileName, "%s%s", StapigatDataPath, DISP_VIDEO_FILENAME);*/

    sprintf(FileName, "%s%s", StapigatDataPath, Disp_Video_FileName);

    LOCAL_PRINT(("Looking for file %s ... ", FileName));
    fstream_p = fopen(FileName, "rb");
    if( fstream_p == NULL )
    {
        LOCAL_PRINT(("Not unable to open !!!\n"));
        Exit = TRUE;
    }

    if(!Exit)
    {
        LOCAL_PRINT(("ok\n"));
        LOCAL_PRINT(("Reading header : "));
        size = fread (&Header, 1, sizeof(DISPVID_Header_t), fstream_p);
        if((size != sizeof(DISPVID_Header_t)) ||
           (Header.Header_size != 0x6) ||
           (Header.Signature != DISP_VID_SIGNATURE))
        {
            LOCAL_PRINT(("Wrong header !!!\n"));
            Exit = TRUE;
        }
        else
        {
            switch(Header.Type)
            {
                case DISP_VID_OMEGA1_TYPE_MB_420:
                    LOCAL_PRINT(("Type OMEGA1 MB 4:2:0 width=%d height=%d\n",
                                 Header.Width, Header.Height ));
                    break;

                case DISP_VID_OMEGA2_TYPE_MB_420:
                    LOCAL_PRINT(("Type OMEGA2 MB 4:2:0 width=%d height=%d\n",
                                 Header.Width, Header.Height ));
                   break;

                case DISP_VID_5528_TYPE_MB_420:
                    LOCAL_PRINT(("Type 5528 MB 4:2:0 width=%d height=%d\n",
                                 Header.Width, Header.Height ));
                   break;

                default:
                    LOCAL_PRINT(("Unknown type !!!\n"));
                    Exit = TRUE;
                    break;
            }
        }
    }

    if(!Exit)
    {
#ifndef USE_DISPVID

#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
        LayAllocData.Size = Header.LumaSize + Header.ChromaSize ;
        LayAllocData.Alignment = 256 ;
        STLAYER_AllocData( FakeLayHndl, &LayAllocData,
                        DataAdd_p_p );
        if ( DataAdd_p_p == NULL )
        {
            STTBX_Print(("Error Allocating data ...\n"));
            return;
        }
#else
        AllocBlockParams.PartitionHandle          = AvmemPartitionHandle[0];
        AllocBlockParams.Size                     = Header.LumaSize + Header.ChromaSize;
        AllocBlockParams.Alignment                = 2048;
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        if(STAVMEM_AllocBlock (&AllocBlockParams,&(BlockHandle)) != ST_NO_ERROR)
        {
            LOCAL_PRINT(("Cannot allocate in avmem !! \n"));
            Exit = TRUE;
        }
#endif  /* LINUX */

#endif  /* !USE_DISPVID */
    }

    if(!Exit)
    {
#ifndef USE_DISPVID
#if !defined (ST_OSLINUX) || defined(USE_AVMEM)
        STAVMEM_GetBlockAddress( BlockHandle, DataAdd_p_p);
        STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
#endif   /*ST_OSLINUX || USE_AVMEM*/
        LOCAL_PRINT(("Reading data ..."));
#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
        size = fread ( *DataAdd_p_p, 1, LayAllocData.Size, fstream_p);
        if (size != LayAllocData.Size)
        {
            LOCAL_PRINT(("Read error %d byte instead of %d !!!\n", size, LayAllocData.Size));
            Exit = TRUE;
        }
        else
        {
            LOCAL_PRINT(("ok\n"));
       }

#else  /*ST_OSLINUX && !USE_AVMEM*/
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

#endif  /*ST_OSLINUX && !USE_AVMEM*/
#else   /*!USE_DISPVID*/
        AddDevice_p = (void *)AVGMemAddress;
        size = fread (AddDevice_p, 1, ExpectedSize, fstream_p);
        if (size != ExpectedSize)
        {
            LOCAL_PRINT(("Read error %d byte instead of %d !!!\n", size, ExpectedSize));
        }
        else
        {
            LOCAL_PRINT(("ok\n"));
       }
#endif  /*!USE_DISPVID*/
    }

    if( fstream_p != NULL )
    {
        fclose (fstream_p);
    }

    if(!Exit)
    {
        *BitmapWidth_p = Header.Width;
        *BitmapHeight_p = Header.Height;
#ifndef USE_DISPVID
#if defined(ST_OSLINUX) && !defined(USE_AVMEM)
        AddDevice_p = (void*) ((U32) STLAYER_UserToKernel((U32*)*DataAdd_p_p) & MASK_STBUS_FROM_ST40);
#else  /*ST_OSLINUX && !USE_AVMEM*/
        AddDevice_p = (void*) STAVMEM_VirtualToDevice(*DataAdd_p_p, &VirtualMapping);
#endif  /* LINUX */

#else
       AddDevice_p = (void*)((U8*)AddDevice_p - (U8*)OS21_SHIFT);
#endif  /* DISPVID */
        LOCAL_PRINT(("%s loaded at avmem dev add 0x%x \n", FileName, AddDevice_p));

        /* Call video registers */
        DispVideo_ShowPicture(AddDevice_p,
                              (void*)((U32)AddDevice_p + Header.LumaSize),
                              Header.Width,
                              Header.Height,
                              LayerIdentity);
    }

} /* end of DispVideo_Display() */

/*-------------------------------------------------------------------------
 * Function : VideoDisplayCmd
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
BOOL VIDEODisplayCmd( STTST_Parse_t* pars_p, char* result_sym_p )
{
    void * LoadedDevAdd_p;
    U32 Width, Height;
    char ResultStr[80];
    char Disp_Video_FileName[80];
    U32 LayerIdentity;
    BOOL RetErr;

    /* Grab value of Layer Identity */
    RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LayerIdentity);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected display type: 0=MAIN, 1=AUX");
        return(TRUE);
    }

    /* Get the name of video file to be loaded */
    RetErr = STTST_GetString( pars_p,DISP_VIDEO_FILENAME, Disp_Video_FileName, sizeof(Disp_Video_FileName) );

    VIDEO_Display(&LoadedDevAdd_p, &Width, &Height, (U8)LayerIdentity, Disp_Video_FileName);

    sprintf(ResultStr, "%d %d %d", (U32) LoadedDevAdd_p, Width, Height);

    STTST_AssignString(result_sym_p, ResultStr, FALSE);
    return(FALSE);
}


/* Exported Functions -------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : DispVideo_RegisterCmd
 *            Definition of register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL DispVideo_RegisterCmd(void)
{
    BOOL RetErr = FALSE;

    RetErr = STTST_RegisterCommand( "DisplayVideo", VIDEODisplayCmd, "Video Display");
    if (RetErr)
    {
        LOCAL_PRINT(( "DispVideo_RegisterCmd() : failed !\n"));
    }
    else
    {
        LOCAL_PRINT(( "DispVideo_RegisterCmd() : ok\n"));
    }
    return(!RetErr);
} /* end of DispVideo_RegisterCmd() */

#endif /* USE_DISP_VIDEO */

/* End of disp_vid.c */

