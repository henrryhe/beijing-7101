/******************************************************************************
*	COPYRIGHT (C) STMicroelectronics 1998
*
*   File name   : GXOBJ_test.c
*   Description : Macros used to test STGXOBJ_xxx functions
*                 of the Layer Driver
*	Note        : All functions return TRUE if error for CLILIB compatibility
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"
#include "stcommon.h"
#include "stsys.h"
#include "stos.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif
#include "testtool.h"
#include "api_cmd.h"

#include "stgxobj.h"
#include "startup.h"

/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* --- Constants --- */
/* --- Prototypes --- */
void os20_main(void *ptr);
BOOL Test_CmdStart(void);

/* --- Global variables --- */
char GXOBJ_Msg[200];                              /* text for trace */

STGXOBJ_Palette_t   PaletteGen1;
STGXOBJ_Palette_t   PaletteGen2;

STGXOBJ_Color_t     ColorGen;

U32 Data1[256]={0};
U32 Data2[256]={0};


/* --- Externals --- */

/*###############################################################################*/
/*############### STATIC FUNCTIONS : TESTTOOL USER INTERFACE ####################*/
/*###############          (in alphabetic order)             ####################*/
/*###############################################################################*/


/*-----------------------------------------------------------------------------
 * Function : GXOBJ_ConvertPalette
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_ConvertPalette (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    STGXOBJ_ColorSpaceConversionMode_t ConvMode;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);
    ConvMode = 0;
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;

    /* get Conversion mode */
    if ( RetErr == FALSE )
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar);
        if ( RetErr == TRUE )
	    {
            tag_current_line( pars_p, "expected Conversion mode :" );
            tag_current_line( pars_p, "0 : STGXOBJ_ITU_R_BT601");
            tag_current_line( pars_p, "1 : STGXOBJ_ITU_R_BT709");
            tag_current_line( pars_p, "2 : STGXOBJ_ITU_R_BT470_2_M");
            tag_current_line( pars_p, "3 : STGXOBJ_ITU_R_BT470_2_BG");
            tag_current_line( pars_p, "4 : STGXOBJ_SMPTE_170M");
            tag_current_line( pars_p, "5 : STGXOBJ_SMPTE_240M");
            tag_current_line( pars_p, "6 : STGXOBJ_CONVERSION_MODE_UNKNOWN");
        }
        switch ( Lvar )
        {
            case 0 :
                ConvMode=STGXOBJ_ITU_R_BT601;break;
            case 1 :
                ConvMode=STGXOBJ_ITU_R_BT709;break;
            case 2 :
                ConvMode=STGXOBJ_ITU_R_BT470_2_M;break;
            case 3 :
                ConvMode=STGXOBJ_ITU_R_BT470_2_BG;break;
            case 4 :
                ConvMode=STGXOBJ_SMPTE_170M;break;
            case 5 :
                ConvMode=STGXOBJ_SMPTE_240M;break;
            case 6 :
                ConvMode=STGXOBJ_CONVERSION_MODE_UNKNOWN;break;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode=STGXOBJ_ConvertPalette( &PaletteGen1, &PaletteGen2, ConvMode);
        switch ( ErrCode )
        {
	        case ST_NO_ERROR :
                RetErr = FALSE;
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat( GXOBJ_Msg, "STGXOBJ_ConvertPalette , Feature not supported !\n" );
	            break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( GXOBJ_Msg, "STGXOBJ_ConvertPalette Bad parameter !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( GXOBJ_Msg, "%sSTGXOBJ_ConvertPalette unexpected error [%X] !\n",
                        GXOBJ_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  GXOBJ_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_GetPaletteColor
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_GetPaletteColor (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U8 PaletteIndex;
    char ColorType[80];
    ST_ErrorCode_t ErrCode;
    S32 Lvar,Palette;

    UNUSED_PARAMETER(result_sym_p);
    ColorType[0]='\0';
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;

    /* get Index */
    if ( RetErr == FALSE )
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar);
        if ( RetErr == TRUE )
	    {
            tag_current_line( pars_p, "expected Index" );
	    }
        PaletteIndex = (U8)Lvar;
	}

    /* get number of palette */
    if ( RetErr == FALSE )
	{
        RetErr = STTST_GetInteger( pars_p, 1, &Lvar);
        if ( RetErr == TRUE )
	    {
            tag_current_line( pars_p, "expected Index" );
	    }
        Palette = Lvar;
	}

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        switch ( Palette )
        {
            case 2 :
                ErrCode=STGXOBJ_GetPaletteColor( &PaletteGen2, PaletteIndex, &ColorGen );
                break;
            default:
                ErrCode=STGXOBJ_GetPaletteColor( &PaletteGen1, PaletteIndex, &ColorGen );
                break;
        }
        switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
                RetErr = FALSE;
                switch ( ColorGen.Type )
                {
                    case STGXOBJ_COLOR_TYPE_ARGB8888:
                        sprintf( ColorType, "%sARGB8888, Alpha:%d, R:%d, G:%d, B:%d",
              ColorType, ColorGen.Value.ARGB8888.Alpha, ColorGen.Value.ARGB8888.R,
              ColorGen.Value.ARGB8888.G, ColorGen.Value.ARGB8888.B);
                        break;
                    case STGXOBJ_COLOR_TYPE_ARGB4444:
                        sprintf( ColorType, "%sARGB4444, Alpha:%d, R:%d, G:%d, B:%d",
              ColorType, ColorGen.Value.ARGB4444.Alpha, ColorGen.Value.ARGB4444.R,
              ColorGen.Value.ARGB4444.G, ColorGen.Value.ARGB4444.B);
                        break;
                    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444:
                        sprintf( ColorType, "%sUNSIGNED_AYCBCR6888_444, Alpha: %d, Y:%d, Cb:%d, Cr:%d",
              ColorType, ColorGen.Value.UnsignedAYCbCr6888_444.Alpha, ColorGen.Value.UnsignedAYCbCr6888_444.Y,
              ColorGen.Value.UnsignedAYCbCr6888_444.Cb, ColorGen.Value.UnsignedAYCbCr6888_444.Cr);
                        break;
                    case STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888:
                        sprintf( ColorType, "%sUNSIGNED_AYCBCR8888, Alpha: %d, Y:%d, Cb:%d, Cr:%d",
              ColorType, ColorGen.Value.UnsignedAYCbCr8888.Alpha, ColorGen.Value.UnsignedAYCbCr8888.Y,
              ColorGen.Value.UnsignedAYCbCr8888.Cb, ColorGen.Value.UnsignedAYCbCr8888.Cr);
                        break;

                    default:
                        break;
                }

                strcat( GXOBJ_Msg, "STGXOBJ_GetPaletteColor \n" );
                sprintf( GXOBJ_Msg, "%s%s Index:%d\n", GXOBJ_Msg, ColorType, PaletteIndex);
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( GXOBJ_Msg, "STGXOBJ_GetPaletteColor Bad parameter !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( GXOBJ_Msg, "%sSTGXOBJ_GetPaletteColor unexpected error [%X] !\n",
                        GXOBJ_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  GXOBJ_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : GXOBJ_Get Revision
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/
static BOOL GXOBJ_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    UNUSED_PARAMETER(pars_p);
    RevisionNumber = STGXOBJ_GetRevision( );
    sprintf( GXOBJ_Msg, "STGXOBJ_GetRevision() : revision=%s\n", RevisionNumber );
    STTBX_Print((  GXOBJ_Msg ));

    return ( FALSE );
} /* end GXOBJ_GetRevision */

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetPaletteColor
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetPaletteColor (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U8 PaletteIndex;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;

    /* get Index */
    if ( RetErr == FALSE )
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar);
        if ( RetErr == TRUE )
	    {
            tag_current_line( pars_p, "expected Index" );
	    }
        PaletteIndex = (U8)Lvar;
	}

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode=STGXOBJ_SetPaletteColor( &PaletteGen1, PaletteIndex, &ColorGen );
        switch ( ErrCode )
        {
	        case ST_NO_ERROR :
                RetErr = FALSE;
                break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( GXOBJ_Msg, "STGXOBJ_SetPaletteColor Bad parameter !\n" );
                STTBX_Print((  GXOBJ_Msg ));
	            break;
            default:
                API_ErrorCount++;
                sprintf( GXOBJ_Msg, "%sSTGXOBJ_SetPaletteColor unexpected error [%X] !\n",
                        GXOBJ_Msg, ErrCode );
                STTBX_Print((  GXOBJ_Msg ));
	            break;
    	}
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetColorARGB4444
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetColorARGB4444 (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;
    ColorGen.Type = STGXOBJ_COLOR_TYPE_ARGB4444;

    /* get A */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected A ");
    }
    ColorGen.Value.ARGB4444.Alpha = (U8)LVar;

    /* get R */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected R ");
    }
    ColorGen.Value.ARGB4444.R = (U8)LVar;

    /* get G */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected G ");
    }
    ColorGen.Value.ARGB4444.G = (U8)LVar;

    /* get B */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected B ");
    }
    ColorGen.Value.ARGB4444.B = (U8)LVar;

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetColorARGB8888
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetColorARGB8888 (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;
    ColorGen.Type = STGXOBJ_COLOR_TYPE_ARGB8888;

    /* get A */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected A ");
    }
    ColorGen.Value.ARGB8888.Alpha = (U8)LVar;

    /* get R */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected R ");
    }
    ColorGen.Value.ARGB8888.R = (U8)LVar;

    /* get G */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected G ");
    }
    ColorGen.Value.ARGB8888.G = (U8)LVar;

    /* get B */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected B ");
    }
    ColorGen.Value.ARGB8888.B = (U8)LVar;

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetColorAYCbCr6888
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetColorAYCbCr6888 (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;
    ColorGen.Type = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;

    /* get A */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected A ");
    }
    ColorGen.Value.ARGB8888.Alpha = (U8)LVar;

    /* get Y */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Y ");
    }
    ColorGen.Value.UnsignedAYCbCr6888_444.Y = (U8)LVar;

    /* get Cb */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Cb ");
    }
    ColorGen.Value.UnsignedAYCbCr6888_444.Cb = (U8)LVar;

    /* get Cr */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Cr ");
    }
    ColorGen.Value.UnsignedAYCbCr6888_444.Cr = (U8)LVar;

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetColorAYCbCr8888
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetColorAYCbCr8888 (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 LVar;

    UNUSED_PARAMETER(result_sym_p);
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;
    ColorGen.Type = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888;

    /* get A */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected A ");
    }
    ColorGen.Value.UnsignedAYCbCr8888.Alpha = (U8)LVar;

    /* get Y */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Y ");
    }
    ColorGen.Value.UnsignedAYCbCr8888.Y = (U8)LVar;

    /* get Cb */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Cb ");
    }
    ColorGen.Value.UnsignedAYCbCr8888.Cb = (U8)LVar;

    /* get Cr */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Cr ");
    }
    ColorGen.Value.UnsignedAYCbCr8888.Cr = (U8)LVar;

    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_SetPalette
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL GXOBJ_SetPalette (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 Lvar;
    STGXOBJ_Palette_t   Palette;

    UNUSED_PARAMETER(result_sym_p);

    Palette.ColorType = 0;
    Palette.PaletteType = 0;
    GXOBJ_Msg[0]='\0';
    RetErr = FALSE;

    /* get the color type */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Color type: ");
        tag_current_line( pars_p, "0: ARGB4444");
        tag_current_line( pars_p, "1: ARGB8888");
        tag_current_line( pars_p, "2: UNSIGNED_AYCBCR6888_444");
        tag_current_line( pars_p, "3: UNSIGNED_AYCBCR8888");
    }
    switch ( Lvar )
    {
        case 0 :
            Palette.ColorType = STGXOBJ_COLOR_TYPE_ARGB4444;
            break;
        case 1 :
            Palette.ColorType = STGXOBJ_COLOR_TYPE_ARGB8888;
            break;
        case 2 :
            Palette.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR6888_444;
            break;
        case 3 :
            Palette.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_AYCBCR8888;
            break;
    }

    /* get the palette type */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected palette type: ");
        tag_current_line( pars_p, "0: STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT");
        tag_current_line( pars_p, "1: STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT");
    }
    switch ( Lvar )
    {
        case 0 : Palette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_INDEPENDENT; break;
        case 1 : Palette.PaletteType = STGXOBJ_PALETTE_TYPE_DEVICE_DEPENDENT; break;
    }

    /* get the Color depth */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Color Depth");
    }
    Palette.ColorDepth = (U8)Lvar;

    /* get the palette number */
    RetErr = STTST_GetInteger( pars_p, 1, &Lvar );
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Palette number ");
    }
    switch ( Lvar )
    {
        case 1 :
            PaletteGen1=Palette;
            PaletteGen1.Data_p = &Data1;
            break;
        case 2 :
            PaletteGen2=Palette;
            PaletteGen2.Data_p = &Data2;
            break;
        default:
            tag_current_line( pars_p, "expected Palette number ");
            break;
    }

    return ( API_EnableError ? RetErr : FALSE );
}



/*###########################################################################*/
/*############################# GXOBJ COMMANDS ##############################*/
/*###########################################################################*/

/*-----------------------------------------------------------------------------
 * Function : GXOBJ_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/


static BOOL GXOBJ_InitCommand (void)
{
BOOL RetErr;

    /*  Command's name, link to C-function, help message     */
    /*  (by alphabetic order of macros = display order)     */
    RetErr = FALSE;

    RetErr |= register_command( "GXO_CONV",  GXOBJ_ConvertPalette,"<Palette In><Palette Out><Conversion mode>: convert a palette in another type");
    RetErr |= register_command( "GXO_GETPALCOL",  GXOBJ_GetPaletteColor,"<Palette Index><Number of palette>: Get the color of a palette for a given index");
    RetErr |= register_command( "GXO_GETREV",  GXOBJ_GetRevision,"Get the revision of the driver");
    RetErr |= register_command( "GXO_SETPALCOL",  GXOBJ_SetPaletteColor,"<Palette Index>: Set the color of a palette for a given index");

    RetErr |= register_command( "GXO_PAL",  GXOBJ_SetPalette,"<ColorType><PaletteType><ColorDepth><Number of palette>: Create a palette object");
    RetErr |= register_command( "GXO_COL_ARGB4444",  GXOBJ_SetColorARGB4444,"<Alpha><R><G><B>: Create a Color");
    RetErr |= register_command( "GXO_COL_ARGB8888",  GXOBJ_SetColorARGB8888,"<Alpha><R><G><B>: Create a Color");
    RetErr |= register_command( "GXO_COL_AYCBCR6888",  GXOBJ_SetColorAYCbCr6888,"<Alpha><Y><Cb><Cr>: Create a Color");
    RetErr |= register_command( "GXO_COL_AYCBCR8888",  GXOBJ_SetColorAYCbCr8888,"<Alpha><Y><Cb><Cr>: Create a Color");

    if ( RetErr == TRUE )
    {
        STTBX_Print((  "GXOBJ_MacroInit() : macros registrations failure !\n" ));
    }
    else
    {
        STTBX_Print((  "GXOBJ_MacroInit() : macros registrations ok\n" ));
    }
    return( RetErr );

} /* end GXOBJ_MacroInit */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL Test_CmdStart()
{
    BOOL RetErr;

    RetErr = GXOBJ_InitCommand();

    return(RetErr ? FALSE : TRUE);

} /* end GXOBJ_CmdStart */


/*###########################################################################*/


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

/* End of avm_test.c */


