/*******************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 2000

Source file name : 
        bmp2gxobj.c

Author :
        Philippe Legeard    : Creation
        Michel Bruant       : Adatption for STGXOBJ color type

This module converts .bmp file to .h stgxobj files 

*******************************************************************************/

/*------ includes ------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---- define & typedef ------------------------------------------------------*/

#define FALSE !(1==1)
#define TRUE (1==1)
#define BI_RGB       0

/*------ Common unsigned types -----------------------------------------------*/

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int    U32;
typedef int BOOL;

/*------ constants for created names files -----------------------------------*/

static const char data_color_h_file[]     = "data_color.h";
static const char data_bitmap_h_file[]    = "data_bitmap.h";
static const char data_bitmap_bin_file[]  = "data_bitmap.bin";
static const char data_bitmap_h_header[]  = "U8 Data_Bitmap[] ";
static const char data_color_h_header[]   = "STGXOBJ_Color_t Data_Color[]";
static const char data_color_h_color_colortype[]
                               = "STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444 ";

/*----- windows bitmaps types ---------------------------------------------*/

typedef struct {
     U16     bfType;
     U32     bfSize;
     U16     bfReserved1;
     U16     bfReserved2;
     U32     bfOffBits;
} BITMAPFILEHEADER;

typedef struct
{
     U32     biSize;
     U32     biWidth;
     U32     biHeight;
     U16     biPlanes;
     U16     biBitCount;
     U32     biCompression;
     U32     biSizeImage;
     U32     biXPelsPerMeter;
     U32     biYPelsPerMeter;
     U32     biClrUsed;
     U32     biClrImportant;
} BITMAPINFOHEADER;

typedef struct
{
     U8      rgbBlue;
     U8      rgbGreen;
     U8      rgbRed;
     U8      rgbReserved;
} RGBQUAD;

/*------ constants -------------------------------------------------------*/

static const char ac_opening[] = "{";
static const char ac_closing[] = "} ;";
static const char space[] = "     ";
static const char goback[1] = {" "} ;
static const char egal[] = " = ";
static const char virg[] = ",";

/*------ pointers -----------------------------------------------------------*/

static BITMAPFILEHEADER *pBitmapFileHeader;
static BITMAPINFOHEADER *pBitmapInfoHeader;
static RGBQUAD           *pRGBQuad, *pRGBQuad_ex;

/******************************************************************************/
/*                                                                            */
/*  main function                                                             */
/*                                                                            */
/******************************************************************************/

int main (int argc, char *argv[])
{
     /*------ input and output files working ---------------------------------*/
    FILE            *input_file_handle ;
    FILE            *data_color_h_file_handle,*data_bitmap_h_file_handle;
    FILE            *data_bitmap_bin_file_handle;
    U32             bmp_amount, file_h_amount, rgb_amount;
    char            input_file[81];
    size_t          output_amount;
    fpos_t          bmp_position;
    fpos_t          bmp_offset;
    /*------ colors computing ------------------------------------------------*/
    U32             nb_of_colors;
    U16             red, green, blue, y, cr, cb  ;
    /*------ bitmap computing ------------------------------------------------*/
    U32             bmp_size, bmp_width, bmp_height ;
    U8              bmp_nb_pad,rest;
    /*------ for memory allocations ------------------------------------------*/
    U8              *pRawData;
    char            *pCwriting;
    /*------ others ----------------------------------------------------------*/
    U32             i,j,k;
    U16             MixWeight = 63;

    /*----- open input file--------------------------------------------------*/
    if (argc != 2)
    {
        printf ("Bad command ! The command is : \n");
	    printf ("bmp2osd  file.bmp : creates .h files with decimal values\n");
        return FALSE;
    }

    if (strlen(argv[1])>sizeof(input_file))
    {
        printf ("The name of the input file is too long !\n");
	    return FALSE;
    }

    strcpy(input_file,argv[1]);

    input_file_handle = fopen(input_file, "rb");   /* read-only, binary */
    if( input_file_handle == NULL )
    {
       printf("Bitmap %s not found\n",input_file);
       return FALSE;
    }

    /*----  get bitmapfileheader and bitmapinfoheader -----------------------*/

    pBitmapFileHeader = malloc(14);
    pBitmapInfoHeader = malloc(40);
    pRGBQuad = malloc(1024);

    if ( (pRGBQuad == NULL) || ( pBitmapFileHeader == NULL )
            || ( pBitmapInfoHeader == NULL ) )
    {
        printf ("no memory left !\n");
	    return FALSE;
    }

    file_h_amount = fread(pBitmapFileHeader, sizeof(U8), 14, input_file_handle);
    file_h_amount = fread(pBitmapInfoHeader, sizeof(U8), 40, input_file_handle);

    if (pBitmapFileHeader->bfType != 0x4d42)
    {
       printf("%s is not a valid bitmap!\n", input_file);
       return FALSE;
    }

    if ( pBitmapInfoHeader->biCompression != BI_RGB)
    {
       printf("%s is a compressed  bitmap!\n", input_file);
       return FALSE;
    }

    /*---- compute nb of colors ---------------------------------------------*/

    switch (pBitmapInfoHeader->biBitCount)
    {
       case 1:   nb_of_colors=2;
            /* printf("Format not supported--not enough colors!\n");
		    return FALSE; */
            break;
       case 4:   nb_of_colors=16;
            break;
       case 8:   nb_of_colors=256;
             break;
       case 24:  nb_of_colors=0;
            printf("Format not compatible--too many colors!\n");
		    return FALSE;
            break;
       default:  printf("Wrong biBitCount value!\n");
		    return FALSE;
    }

    printf("nb of colors:%d\n",nb_of_colors);

    /*---- open output files ------------------------------------------------*/

    if ( ( data_bitmap_h_file_handle = fopen(data_bitmap_h_file, "wb"))
               == NULL )
    {
        printf ("%s has not been opened\n",data_bitmap_h_file);
        return FALSE;
    }

    /*------ color file -----------------------------------------------------*/
    if ( ( data_color_h_file_handle = fopen(data_color_h_file, "wb")) == NULL )
    {
        printf ("%s has not been opened\n",data_color_h_file);
        return FALSE;
    }

    printf("output files are opened\n");
    pCwriting = malloc(200);

    /*------ data color file header computig -----------------------------*/
    output_amount = fwrite(data_color_h_header,sizeof(char),
            sizeof(data_color_h_header)-1,data_color_h_file_handle);
    output_amount = fwrite(egal,sizeof(char),sizeof(egal)-1,
            data_color_h_file_handle);
    output_amount = fwrite(goback,sizeof(char),2,
            data_color_h_file_handle);
    output_amount = fwrite(ac_opening,sizeof(char),
            sizeof(ac_opening)-1,data_color_h_file_handle);
    output_amount = fwrite(goback,sizeof(char),2,data_color_h_file_handle);
     fprintf (data_color_h_file_handle,"\n");

    /*---- compute YCrCb colors -----------------------------------------*/
    pRGBQuad_ex=pRGBQuad;

    /*------ palette computation ---------------------------------------------*/
    for (i=0; i<nb_of_colors; i++)
    {
       rgb_amount = fread((pRGBQuad++), sizeof(U8), 4, input_file_handle);
    }

    pRGBQuad=pRGBQuad_ex;

    j=0 ;
    for (i=0; i<nb_of_colors; i++)
    {
        red = (pRGBQuad->rgbRed*219/255 + 16)&0xff;
        green = (pRGBQuad->rgbGreen*219/255 + 16)&0xff;
        blue = (pRGBQuad->rgbBlue*219/255 + 16)&0xff;

        y=(U8)(((77*red + 150*green + 29*blue)/256));
        cb=(U8)((-44*red - 87*green + 131*blue)/256 + 128 ) ;
        cr=(U8)((131*red - 110*green - 21*blue)/256 +  128 ) ;

        output_amount = fwrite(space,sizeof(U8),sizeof(space)-1,
                data_color_h_file_handle);
        output_amount = fwrite(ac_opening,sizeof(U8),sizeof(ac_opening)-1,
                data_color_h_file_handle);
        output_amount = fwrite(data_color_h_color_colortype,sizeof(U8),
               sizeof(data_color_h_color_colortype)-1,data_color_h_file_handle);
	    output_amount = fwrite(virg,sizeof(U8),sizeof(virg)-1,
                data_color_h_file_handle);

	    if (i!=nb_of_colors-1)
	    {
		      fprintf (data_color_h_file_handle,"   %4d,%4d,%4d,%4d },\n",
                      MixWeight,y,cb,cr);
	    }
	    else
	    {
		         fprintf (data_color_h_file_handle,"   %4d,%4d,%4d,%4d }\n",
                         MixWeight,y,cb,cr);
	    }

        output_amount = fwrite(goback,sizeof(char),2,data_color_h_file_handle);
        pRGBQuad++;
    }

    /*------ data color file end ---------------------------------------------*/
    output_amount = fwrite(ac_closing,sizeof(char),
            sizeof(ac_closing)-1,data_color_h_file_handle);
    output_amount = fwrite(goback,sizeof(char),2,data_color_h_file_handle);
    printf("colors computed\n");

    /*------ compute size of bitmap data in bytes ----------------------------*/
    bmp_width = pBitmapInfoHeader->biWidth;
    bmp_height = pBitmapInfoHeader->biHeight;

    switch (nb_of_colors)
    {
       case 2:
            bmp_width/=8;
            break;
       case 16:
            rest = pBitmapInfoHeader->biWidth%2;
            bmp_width=pBitmapInfoHeader->biWidth/2+rest;
            break;
	  case  256 :
            rest = 0;
            break ;
       default:
            break;
    }

    /*--- bmp_nb_pad is the num of bytes of he bitmap file with stg inside -*/
    bmp_nb_pad = (4-(bmp_width%4))%4;
    printf("nb octets padding = %d\n",bmp_nb_pad);

    /*------ data bitmap file header ----------------------------------------*/
    bmp_size = bmp_width * bmp_height;

    printf("width: %i, height: %i, size: %i\n",
            pBitmapInfoHeader->biWidth, bmp_height, bmp_size);

    output_amount = fwrite(data_bitmap_h_header,sizeof(char),
               sizeof(data_bitmap_h_header)-1,data_bitmap_h_file_handle);
    output_amount = fwrite(egal,sizeof(char),sizeof(egal)-1,
               data_bitmap_h_file_handle);
    output_amount = fwrite(goback,sizeof(char),2,data_bitmap_h_file_handle);
    output_amount = fwrite(ac_opening,sizeof(char),
               sizeof(ac_opening)-1,data_bitmap_h_file_handle);
    output_amount = fwrite(goback,sizeof(char),2,data_bitmap_h_file_handle);
    output_amount = fwrite(space,sizeof(char),
               sizeof(space)-1,data_bitmap_h_file_handle);

    bmp_offset = 14+40+nb_of_colors*4 ;

    /*---- bitmap datas computing --------------------------------------------*/
    pRawData = malloc(bmp_width);
    if (pRawData == NULL)
    {
       printf("Memory allocation pb pRawData\n");
       return FALSE;
    }
    fprintf(data_bitmap_h_file_handle,"\n");
    output_amount = fwrite(goback,sizeof(char),2,data_bitmap_h_file_handle);
    output_amount = fwrite(space,sizeof(char),sizeof(space)-1,
                        data_bitmap_h_file_handle);
    for (i=0; i<bmp_height ; i++)
    {
        bmp_position = bmp_offset + (bmp_height-i-1)*(bmp_width+bmp_nb_pad) ;
        if(fsetpos(input_file_handle, &bmp_position) != 0)
        {
            printf ("bitmap file reading problem ....\n");
            return FALSE;
        }
        bmp_amount = fread(pRawData, sizeof(U8), bmp_width, input_file_handle);
        for (k=0;k<bmp_width;k++)
        {
            /*------ if last element of the array, no ',' after -------*/
            if ((i==bmp_height-1)&&(k==bmp_width-1))
            {
                sprintf(pCwriting,"%3d",pRawData[k]) ;
                output_amount = fwrite(pCwriting,sizeof(U8),3,
                        data_bitmap_h_file_handle);
            }
            else
            {
		        sprintf(pCwriting,"%3d,",pRawData[k]) ;
                output_amount = fwrite(pCwriting,sizeof(char),4,
                        data_bitmap_h_file_handle);
            }
            j++;
            if (j==16)
			{
                j=0;
                fprintf(data_bitmap_h_file_handle,"\n");
                output_amount = fwrite(goback,sizeof(char),2,
                        data_bitmap_h_file_handle);
                output_amount = fwrite(space,sizeof(char),sizeof(space)-1,
                        data_bitmap_h_file_handle);
			}
        }
    }

    /*------ data bitmap file end -----------------------------------*/
    fprintf(data_bitmap_h_file_handle,"\n");
    output_amount = fwrite(goback,sizeof(char),2,data_bitmap_h_file_handle);
    output_amount = fwrite(ac_closing,sizeof(char),sizeof(ac_closing)-1,
               data_bitmap_h_file_handle);
    fprintf(data_bitmap_h_file_handle,"\n");
    output_amount = fwrite(goback,sizeof(char),2,data_bitmap_h_file_handle);

    /*---- close the files and free memory ------------------------------*/
    fclose(input_file_handle);
    fprintf (data_bitmap_h_file_handle,"\n\n");
    fclose(data_bitmap_h_file_handle);
    fprintf (data_color_h_file_handle,"\n\n");
    fclose(data_color_h_file_handle);

    free(pBitmapFileHeader);
    free(pBitmapInfoHeader);
    free(pRawData);
    printf("Normal termination");
}

