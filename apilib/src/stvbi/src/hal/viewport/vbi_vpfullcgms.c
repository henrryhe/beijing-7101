/*******************************************************************************
File name   : vbi_vpfullcgms.c

Description : vbi hal source file for FULL_CGMS use

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
02 Jan 2006        Created                                           SBEBA
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "vbi_drv.h"
#include "vbi_vpreg.h"
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <string.h>
#endif


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : stvbi_HalAllocResource
Description : Allocate FULLCGMS Bitmap data on Layer
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvbi_HalAllocResource( VBIDevice_t * const Device_p)
{

        ST_ErrorCode_t Err = ST_NO_ERROR;

        STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;/* needed for Bitmaps initialisation */
        STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;/* needed for Bitmaps initialisation */
        STAVMEM_AllocBlockParams_t  AllocParams;       /* needed for CgmsBitmaps allocation */
        U32 * Cont  ;
        U32 taille  ; /* used to fill memory */




        /* Initilisation of Bitmap */

        Device_p->CgmsBitmap.ColorType=STGXOBJ_COLOR_TYPE_ARGB8888;

        Device_p->CgmsBitmap.Width  = CGMS_VIEWPORT_MAX_WIDTH;
        Device_p->CgmsBitmap.Height = CGMS_VIEWPORT_MAX_HEIGHT;
        Device_p->CgmsBitmap.Data1_p = NULL;
        Device_p->CgmsBitmap.Size1   = 0;

    /* get CgmsBitmap Params */
#ifdef ST_OSLINUX
               Err = STLAYER_GetBitmapAllocParams(Device_p->LayerHandle, &Device_p->CgmsBitmap, &BitmapAllocParams1,&BitmapAllocParams2 );
#else
               Err = STGXOBJ_GetBitmapAllocParams(&Device_p->CgmsBitmap,(STGXOBJ_GAMMA_BLITTER |  STGXOBJ_GAMMA_GDP_PIPELINE | STGXOBJ_GAMMA_CURS_PIPELINE),
                                            &BitmapAllocParams1,
                                            &BitmapAllocParams2);
#endif

            /* Update CgmsBitmap Params */
        Device_p->CgmsBitmap.Size1   = BitmapAllocParams1.AllocBlockParams.Size;
        Device_p->CgmsBitmap.Pitch   = BitmapAllocParams1.Pitch;
        Device_p->CgmsBitmap.Offset  = BitmapAllocParams1.Offset;
        Device_p->CgmsBitmap.Size2   = 0;
        Device_p->CgmsBitmap.Data2_p = NULL;
        Device_p->CgmsBitmap.BitmapType=STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        Device_p->CgmsBitmap.PreMultipliedColor=0;
        Device_p->CgmsBitmap.ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
        Device_p->CgmsBitmap.AspectRatio=STGXOBJ_ASPECT_RATIO_16TO9;
        Device_p->CgmsBitmap.SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
        Device_p->CgmsBitmap.BigNotLittle=0;

                        /* Allocate memory for CGMS-HD Data needed for CgmsBitmap */
        AllocParams.PartitionHandle          = Device_p->AVMemPartitionHandle;
        AllocParams.Size                     = (U32)((Device_p->CgmsBitmap.Size1 + 15) & (~15));
        AllocParams.Alignment                = 16;
        AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocParams.NumberOfForbiddenRanges  = 0;
        AllocParams.ForbiddenRangeArray_p    = NULL;
        AllocParams.NumberOfForbiddenBorders = 0;
        AllocParams.ForbiddenBorderArray_p   = NULL;

        Err = STAVMEM_AllocBlock (&AllocParams, &Device_p->AVMemBlockHandle);
        if ( Err != ST_NO_ERROR )
        {
             return Err;
        }
        Err = STAVMEM_GetBlockAddress (Device_p->AVMemBlockHandle, (void **)&(Device_p->CgmsBitmap.Data1_p));
        if ( Err != ST_NO_ERROR )
        {
              return Err;
        }

                /* write defaullt value */

            Cont   =  (U32 *) ((U32)(Device_p->CgmsBitmap.Data1_p)) ;
            taille=0;

            /*  Write Cgms-HD Start Symbol Data */
            while(taille< AllocParams.Size )
            {
                *(Cont)=(U32)(0x00000000);
                Cont++;
                taille++;
            }




    return (Err);
} /* end of stvbi_HalAllocResource() function */


/*******************************************************************************
Name        : stvbi_HalFreeResource
Description : Free FULLCGMS Bitmap data on Layer
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvbi_HalFreeResource( VBIDevice_t * const Device_p)
{

        ST_ErrorCode_t Err = ST_NO_ERROR;
        STAVMEM_FreeBlockParams_t   FreeParams;


        FreeParams.PartitionHandle = Device_p->AVMemPartitionHandle;
        Err = STAVMEM_FreeBlock(&FreeParams,&(Device_p->AVMemBlockHandle));



        return (Err);
} /* end of stvbi_HalFreeResource() function */


/*******************************************************************************
Name        : stvbi_HalInitFullCgmsViewport
Description : Init Viewport used for FullCgms
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HalInitFullCgmsViewport( VBI_t * const Vbi_p)
{

        ST_ErrorCode_t Err = ST_NO_ERROR;



        STLAYER_ViewPortParams_t VPParam;
        STLAYER_ViewPortSource_t source;
        STGXOBJ_BitmapAllocParams_t BitmapAllocParams1;/* needed for Bitmaps initialisation */
        STGXOBJ_BitmapAllocParams_t BitmapAllocParams2;/* needed for Bitmaps initialisation */

                /* Update Bitmaps Params */

        switch (Vbi_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

            case STVBI_CGMS_HD_1080I60000 :
            case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                                            Vbi_p->Device_p->CgmsBitmap.Width=CGMS_1080I_WITH;
                                            Vbi_p->Device_p->CgmsBitmap.Height=CGMS_1080I_HEIGHT;
                                            break;
            case STVBI_CGMS_HD_720P60000 :
            case STVBI_CGMS_TYPE_B_HD_720P60000 :
                                            Vbi_p->Device_p->CgmsBitmap.Width=CGMS_720P_WITH;
                                            Vbi_p->Device_p->CgmsBitmap.Height=CGMS_720P_HEIGHT;
                                            break;
            case STVBI_CGMS_SD_480P60000 :
            case STVBI_CGMS_TYPE_B_SD_480P60000 :
                                            Vbi_p->Device_p->CgmsBitmap.Width=CGMS_480P_WITH;
                                            Vbi_p->Device_p->CgmsBitmap.Height=CGMS_480P_HEIGHT;
                                            break;
            default:
                                            break;
        }



            /* get CgmsBitmap Params */
#ifdef ST_OSLINUX
               Err = STLAYER_GetBitmapAllocParams(Vbi_p->Device_p->LayerHandle, &Vbi_p->Device_p->CgmsBitmap, &BitmapAllocParams1,&BitmapAllocParams2 );
#else

              Err = STGXOBJ_GetBitmapAllocParams(&Vbi_p->Device_p->CgmsBitmap,(STGXOBJ_GAMMA_BLITTER |  STGXOBJ_GAMMA_GDP_PIPELINE | STGXOBJ_GAMMA_CURS_PIPELINE),
                                            &BitmapAllocParams1,
                                            &BitmapAllocParams2);
#endif

            /* Update CgmsBitmap Params */
        Vbi_p->Device_p->CgmsBitmap.Size1   = BitmapAllocParams1.AllocBlockParams.Size;
        Vbi_p->Device_p->CgmsBitmap.Pitch   = BitmapAllocParams1.Pitch;
        Vbi_p->Device_p->CgmsBitmap.Offset  = BitmapAllocParams1.Offset;


            /* source Params for CgmsBitmap */
        source.SourceType = STLAYER_GRAPHIC_BITMAP;
        source.Palette_p = 0;
        source.Data.BitMap_p =&Vbi_p->Device_p->CgmsBitmap;
        source.Data.BitMap_p->BigNotLittle = FALSE;

                         /* set Viewport Params */

        switch (Vbi_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

            case STVBI_CGMS_HD_1080I60000 :
            case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                                            VPParam.InputRectangle.Width =     CGMS_1080I_WITH;
                                            VPParam.InputRectangle.Height =    CGMS_1080I_HEIGHT;
                                            VPParam.InputRectangle.PositionX = CGMS_1080I_POSXIN;
                                            VPParam.InputRectangle.PositionY = CGMS_1080I_POSYIN;
                                            break;
            case STVBI_CGMS_HD_720P60000 :
            case STVBI_CGMS_TYPE_B_HD_720P60000 :

                                            VPParam.InputRectangle.Width =     CGMS_720P_WITH;
                                            VPParam.InputRectangle.Height =    CGMS_720P_HEIGHT;
                                            VPParam.InputRectangle.PositionX = CGMS_720P_POSXIN;
                                            VPParam.InputRectangle.PositionY = CGMS_720P_POSYIN;
                                            break;
            case STVBI_CGMS_SD_480P60000 :
            case STVBI_CGMS_TYPE_B_SD_480P60000 :
                                            VPParam.InputRectangle.Width =     CGMS_480P_WITH;
                                            VPParam.InputRectangle.Height =    CGMS_480P_HEIGHT;
                                            VPParam.InputRectangle.PositionX = CGMS_480P_POSXIN;
                                            VPParam.InputRectangle.PositionY = CGMS_480P_POSYIN;
                                            break;
            default:
                                            break;
        }




       VPParam.OutputRectangle=VPParam.InputRectangle;



               switch (Vbi_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

            case STVBI_CGMS_HD_1080I60000 :
                                            VPParam.OutputRectangle.PositionX  =  CGMS_1080I_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  =  CGMS_1080I_POSYOUT;
                                            break;
            case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                                            VPParam.OutputRectangle.PositionX  =  CGMS_TYPE_B_1080I_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  =  CGMS_TYPE_B_1080I_POSYOUT;
                                            break;
            case STVBI_CGMS_HD_720P60000 :
                                            VPParam.OutputRectangle.PositionX =  CGMS_720P_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  = CGMS_720P_POSYOUT;
                                            break;

            case STVBI_CGMS_TYPE_B_HD_720P60000 :
                                            VPParam.OutputRectangle.PositionX =  CGMS_TYPE_B_720P_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  = CGMS_TYPE_B_720P_POSYOUT;
                                            break;
            case STVBI_CGMS_SD_480P60000 :
                                            VPParam.OutputRectangle.PositionX =  CGMS_480P_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  = CGMS_480P_POSYOUT;
                                            break;
            case STVBI_CGMS_TYPE_B_SD_480P60000 :

                                            VPParam.OutputRectangle.PositionX =  CGMS_TYPE_B_480P_POSXOUT;
                                            VPParam.OutputRectangle.PositionY  = CGMS_TYPE_B_480P_POSYOUT;
                                            break;
            default:
                                            break;
        }



    VPParam.Source_p = &source;

             /* Open an GDP Viewport for CGMS-HD on the specified LAYER Handle */
    Err = STLAYER_OpenViewPort( Vbi_p->Device_p->LayerHandle, &VPParam, &Vbi_p->Device_p->VPHandle ) ;


    return (Err);
} /* end of stvbi_HalInitFullCgmsViewport() function */




/*******************************************************************************
Name        : stvbi_CloseViewport
Description : Write HDCGMS data on Layer
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HalCloseFullCgmsViewport( VBI_t * const Vbi_p)
{

        ST_ErrorCode_t Err = ST_NO_ERROR;

          /* Close an GDP Viewport */
    Err = STLAYER_CloseViewPort( Vbi_p->Device_p->VPHandle ) ;


 return (Err);

}  /* end of stvbi_HalCloseFullCgmsViewport() function */

/*******************************************************************************
Name        : stvbi_HalWriteDataOnFullCgmsViewport
Description : Write HDCGMS data on Layer
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR7
*******************************************************************************/
ST_ErrorCode_t stvbi_HalWriteDataOnFullCgmsViewport( VBI_t * const Vbi_p, const U8* const Data_p)
{

     ST_ErrorCode_t Err = ST_NO_ERROR;


     U32      Val = 0;
     U32 * Cont  ;
     U32 * Cont1 ;
     int taille=0;
     int ByteCount=0;
     int StartSymb=0;
     int HeaderWith=0;
     U32 StartSymbol =  CGMS_START_SYMBOL ;

         /*   Insert Header  CGMS + CGMS Data   */

     Val = StartSymbol + ((Data_p[0] & 0x0F) << 16) + ((Data_p[1] & 0xFF) << 8) +((Data_p[2] & 0xFF) << 0);


     /* Initilate  Cont,Cont1 used for both Bottom and Top field */

     Cont   =  (U32 *) ((U32)(Vbi_p->Device_p->CgmsBitmap.Data1_p) + Vbi_p->Device_p->CgmsBitmap.Offset) ;
     Cont1  =  (U32 *)((U32)Cont+Vbi_p->Device_p->CgmsBitmap.Pitch);


        switch (Vbi_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

            case STVBI_CGMS_HD_1080I60000 :
                                            StartSymb=CGMS_1080I_START_SYM_POS;
                                            HeaderWith=CGMS_1080I_HEADER_SYM_WIDTH;
                                            break;
            case STVBI_CGMS_HD_720P60000 :
                                            StartSymb=CGMS_720P_START_SYM_POS;
                                            HeaderWith=CGMS_720P_HEADER_SYM_WIDTH;
                                            Cont1= Cont;

                                            break;
            case STVBI_CGMS_SD_480P60000 :
                                            StartSymb=CGMS_480P_START_SYM_POS;
                                            HeaderWith=CGMS_480P_HEADER_SYM_WIDTH;
                                            Cont1= Cont;
                                            break;
            default:
                                            break;
        }


            taille=0;
       /*  Write Cgms-HD Start Symbol Data */
     while(taille< StartSymb )
     {
       *(Cont)=(U32)CGMS_NO_DATA_VALUE;
       *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
       Cont++;
       Cont1++;
       taille++;
     }

         /*  Write Cgms Data */

   for(ByteCount=CGMS_DATA_LENGTH-1;ByteCount>-1;ByteCount--)
     {
      if (((Val >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }


    return (Err);
} /* end of stvbi_HalWriteDataOnFullCgmsViewport() function */



/*******************************************************************************
Name        : stvbi_HalWriteDataOnFullCgmsViewportTypeB
Description : Write HDCGMS data on Layer
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR7
*******************************************************************************/
ST_ErrorCode_t stvbi_HalWriteDataOnFullCgmsViewportTypeB( VBI_t * const Vbi_p, const U8* const Data_p)
{

     ST_ErrorCode_t Err = ST_NO_ERROR;


     U32      Val=0,Val1=0,Val2,Val3=0,Val4= 0;

     U32 * Cont  ;
     U32 * Cont1 ;
     int taille=0;
     int ByteCount=0;
     int StartSymb=0;
     int HeaderWith=0;
     U32 StartSymbol =  CGMS_TYPE_B_START_SYMBOL ;

         /*   Insert Header  CGMS + CGMS Data   */

     Val = StartSymbol + ((Data_p[0] & 0x3F) << 24)   + ((Data_p[1] & 0xFF) << 16) + ((Data_p[2] & 0xFF) << 8) +((Data_p[3] & 0xFF) << 0);
     Val1 = ((Data_p[4] & 0xFF) << 24)   + ((Data_p[5] & 0xFF) << 16) + ((Data_p[6] & 0xFF) << 8) +((Data_p[7] & 0xFF) << 0);
     Val2 = ((Data_p[8] & 0xFF) << 24)   + ((Data_p[9] & 0xFF) << 16) + ((Data_p[10] & 0xFF) << 8) +((Data_p[11] & 0xFF) << 0);
     Val3 = ((Data_p[12] & 0xFF) << 24)   + ((Data_p[13] & 0xFF) << 16) + ((Data_p[14] & 0xFF) << 8) +((Data_p[15] & 0xFF) << 0);
     Val4 =  ((Data_p[16] & 0xFF) << 0);

     /* Initilate  Cont,Cont1 used for both Bottom and Top field */

     Cont   =  (U32 *) ((U32)(Vbi_p->Device_p->CgmsBitmap.Data1_p) + Vbi_p->Device_p->CgmsBitmap.Offset) ;
     Cont1  =  (U32 *)((U32)Cont+Vbi_p->Device_p->CgmsBitmap.Pitch);


        switch (Vbi_p->Params.Conf.Type.CGMSFULL.CGMSFULLStandard) {

            case STVBI_CGMS_TYPE_B_HD_1080I60000 :
                                            StartSymb=CGMS_TYPE_B_1080I_START_SYM_POS;
                                            HeaderWith=CGMS_TYPE_B_1080I_HEADER_SYM_WIDTH;
                                            break;
            case STVBI_CGMS_TYPE_B_HD_720P60000 :
                                            StartSymb=CGMS_TYPE_B_720P_START_SYM_POS;
                                            HeaderWith=CGMS_TYPE_B_720P_HEADER_SYM_WIDTH;
                                            Cont1= Cont;

                                            break;
            case STVBI_CGMS_TYPE_B_SD_480P60000 :
                                            StartSymb=CGMS_TYPE_B_480P_START_SYM_POS;
                                            HeaderWith=CGMS_TYPE_B_480P_HEADER_SYM_WIDTH;
                                            Cont1= Cont;
                                            break;
            default:
                                            break;
        }


            taille=0;
       /*  Write Cgms-HD Start Symbol Data */
     while(taille< StartSymb )
     {
       *(Cont)=(U32)CGMS_NO_DATA_VALUE;
       *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
       Cont++;
       Cont1++;
       taille++;
     }

         /*  Write Cgms Data  byte 0 -> byte 3 */

   for(ByteCount=31;ByteCount>-1;ByteCount--)
     {
      if (((Val >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }
     /*  Write Cgms Data  byte 4 -> byte 7 */

   for(ByteCount=31;ByteCount>-1;ByteCount--)
     {
      if (((Val1 >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }

     /*  Write Cgms Data  byte 8 -> byte 11 */
   for(ByteCount=31;ByteCount>-1;ByteCount--)
     {
      if (((Val2 >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }

     /*  Write Cgms Data  byte 12 -> byte 15 */
   for(ByteCount=31;ByteCount>-1;ByteCount--)
     {
      if (((Val3 >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }

     /*  Write Cgms Data  byte 16  */
   for(ByteCount=7;ByteCount>-1;ByteCount--)
     {
      if (((Val4 >> ByteCount) & (0x01)) == 1)
     {
                taille=0;

                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_DATA_VALUE  ;
                    Cont++;
                    *(Cont1)=(U32)CGMS_DATA_VALUE  ;
                    Cont1++;
                    taille++;
                }

            }
            else
            {
                taille=0;
                while(taille< HeaderWith )
                {
                    *(Cont)=(U32)CGMS_NO_DATA_VALUE;
                    Cont++;
                    *(Cont1)=(U32)CGMS_NO_DATA_VALUE;
                    Cont1++;
                    taille++;
                }
           }
     }

    return (Err);
} /* end of stvbi_HalWriteDataOnFullCgmsViewport() function */



/* End of vbi_vpfullcgms.c */
