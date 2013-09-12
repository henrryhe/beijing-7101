/************************************************************************

File name   : mix_cmd.c

Description : Definition of mixer commands

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                     Name
----               ------------                                     ----
Aug 2000           Creation                                          BS
11 Oct 2002        Add support for STi5517                           BS
14 Jan 2003        Correct cast problem for GetInteger               BS
13 May 2003        Add support for Stem7020                          HS
10 Jun 2003        Add support for 5528                              HS
************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21debug.h>
#endif

#ifdef ST_OS20
#include <debug.h>
#endif

#include "stsys.h"

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "stvmix.h"

#include "clevt.h"
#include "api_cmd.h"
#include "stvtg.h"
#include "denc_cmd.h"
#include "vtg_cmd.h"
#include "vmix_cmd.h"
#include "startup.h"
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined (ST_5107)\
 || defined (ST_5162)
#include "clavmem.h"
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define MAX_LAYER 10
#define MAX_VOUT  6
#define MAX_VMIX_TYPE 24
#define STRING_DEVICE_LENGTH 80
#define RETURN_PARAMS_LENGTH 50

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || \
    ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))
#define MIXER_BASE_ADDRESS (VIDEO_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  0
#define MIXER_DEVICE_TYPE  "OMEGA1_SD"

#elif defined (ST_5100)
#define MIXER_COMPO_BASE_ADDRESS        (ST5100_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS        (ST5100_GDMA1_BASE_ADDR)
#define MIXER_GDMA2_BASE_ADDRESS        (ST5100_GDMA2_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5100_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5100"
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5100_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5162)
#define MIXER_COMPO_BASE_ADDRESS        (ST5162_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS        (ST5162_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5162_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5162"
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5162_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (8 * 1024)
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5105)
#define MIXER_COMPO_BASE_ADDRESS        (ST5105_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS        (ST5105_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5105_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5105"
#if defined USE_215K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (215 * 1024)
#elif defined USE_128K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (128 * 1024)
#elif defined USE_64K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (64 * 1024)
#elif defined USE_32K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (32 * 1024)
#else /* Assumption is 16K cache */
#define MIXER_CACHE_SIZE                (16 * 1024)
#endif
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5107)
#define MIXER_COMPO_BASE_ADDRESS        (ST5107_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS        (ST5107_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5107_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5107"
#if defined USE_215K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (215 * 1024)
#elif defined USE_128K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (128 * 1024)
#elif defined USE_64K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (64 * 1024)
#elif defined USE_32K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (32 * 1024)
#else /* Assumption is 16K cache */
#define MIXER_CACHE_SIZE                (16 * 1024)
#endif
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5188)
#define MIXER_COMPO_BASE_ADDRESS        (ST5188_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS        (ST5188_GDMA1_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5188_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_COMPOSITOR_FIELD_COMBINED_SDRAM_422"
#if defined USE_215K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (215 * 1024)
#elif defined USE_128K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (128 * 1024)
#elif defined USE_64K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (64 * 1024)
#elif defined USE_32K_BLITTER_CACHE
#define MIXER_CACHE_SIZE                (32 * 1024)
#else /* Assumption is 16K cache */
#define MIXER_CACHE_SIZE                (16 * 1024)
#endif

#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5301)
#define MIXER_COMPO_BASE_ADDRESS        (ST5301_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS         (ST5301_GDMA1_BASE_ADDR)
#define MIXER_GDMA2_BASE_ADDRESS         (ST5301_GDMA2_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5301_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5301"
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5301_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS

#elif defined (ST_5525)
#define MIXER_COMPO_BASE_ADDRESS        (ST5525_BLITTER_BASE_ADDRESS + 0xA00)
#define MIXER_GDMA1_BASE_ADDRESS         (ST5525_GDMA1_BASE_ADDR)
#define MIXER_GDMA2_BASE_ADDRESS         (ST5525_GDMA2_BASE_ADDR)
#define MIXER_VOUT_BASE_ADDRESS         (ST5525_VOUT_BASE_ADDRESS)
#define MIXER_DEVICE_TYPE               "COMPO_TYPE_5525"
#define MIXER_TILE_RAM_BASE_ADDRESS     (ST5525_TILE_RAM_BASE_ADDRESS)
#define MIXER_TILE_RAM_SIZE             (16 * 1024)
#define MIXER_BASE_OFFSET               MIXER_GDMA1_BASE_ADDRESS



#elif defined (ST_5528) && !defined(ST_7020)
#define MIXER_BASE_ADDRESS    0
#define MIXER_BASE_OFFSET     ST5528_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE     "GAMMA_TYPE_5528"
#define G_CURSOR_BASE_ADDRESS ST5528_CURSOR_LAYER_BASE_ADDRESS
#define G_GDP1_BASE_ADDRESS   ST5528_GDP1_LAYER_BASE_ADDRESS
#define G_GDP2_BASE_ADDRESS   ST5528_GDP2_LAYER_BASE_ADDRESS
#define G_GDP3_BASE_ADDRESS   ST5528_GDP3_LAYER_BASE_ADDRESS
#define G_GDP4_BASE_ADDRESS   ST5528_GDP4_LAYER_BASE_ADDRESS
#define G_ALPHA_BASE_ADDRESS  ST5528_ALP_LAYER_BASE_ADDRESS
#define G_MIXER1_BASE_ADDRESS ST5528_VMIX1_BASE_ADDRESS
#define G_MIXER2_BASE_ADDRESS ST5528_VMIX2_BASE_ADDRESS
#define G_VIDEO1              "SDDISPO2_VIDEO1"
#define G_VIDEO2              "SDDISPO2_VIDEO2"

#define GAMMA_CONSTANTS

#elif defined (ST_7015)
#define MIXER_BASE_ADDRESS (STI7015_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7015_GAMMA_OFFSET+ST7015_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  "GAMMA_TYPE_7015"

#elif defined (ST_7020)
#define MIXER_BASE_ADDRESS (STI7020_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7020_GAMMA_OFFSET+ST7020_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  "GAMMA_TYPE_7020"
#define G_CURSOR_BASE_ADDRESS 0xA000
#define G_GDP1_BASE_ADDRESS   0xA100
#define G_GDP2_BASE_ADDRESS   0xA200
#define G_GDP3_BASE_ADDRESS   0xA300
#define G_ALPHA_BASE_ADDRESS  0xA600
#define G_MIXER1_BASE_ADDRESS 0xAC00
#define G_MIXER2_BASE_ADDRESS 0xAD00
#define G_VIDEO1              "OMEGA2_VIDEO1"
#define G_VIDEO2              "OMEGA2_VIDEO2"
#define GAMMA_CONSTANTS
#if defined (ST_5528)
    #define MIXER_5528_BASE_ADDRESS    0
    #define MIXER_5528_BASE_OFFSET     ST5528_VMIX1_BASE_ADDRESS
#endif

#elif defined (ST_7710)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET            ST7710_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE            "GAMMA_TYPE_7710"
#define G_CURSOR_BASE_ADDRESS         0x38215000
#define G_GDP1_BASE_ADDRESS           0x38215100
#define G_GDP2_BASE_ADDRESS           0x38215200
#define G_ALPHA_BASE_ADDRESS          0x38215600
#define G_MIXER1_BASE_ADDRESS         ST7710_VMIX1_BASE_ADDRESS
#define G_MIXER2_BASE_ADDRESS         ST7710_VMIX2_BASE_ADDRESS
#define G_VIDEO1                      "HDDISPO2_VIDEO1"
#define G_VIDEO2                      "HDDISPO2_VIDEO2"
#define GAMMA_CONSTANTS

#elif defined (ST_7100)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET            ST7100_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE            "GAMMA_TYPE_7100"
#define G_CURSOR_BASE_ADDRESS         ST7100_CURSOR_LAYER_BASE_ADDRESS
#define G_GDP1_BASE_ADDRESS           ST7100_GDP1_LAYER_BASE_ADDRESS
#define G_GDP2_BASE_ADDRESS           ST7100_GDP2_LAYER_BASE_ADDRESS
#define G_ALPHA_BASE_ADDRESS          ST7100_ALP_LAYER_BASE_ADDRESS
#define G_MIXER1_BASE_ADDRESS         ST7100_VMIX1_BASE_ADDRESS
#define G_MIXER2_BASE_ADDRESS         ST7100_VMIX2_BASE_ADDRESS
#define G_VIDEO1                      "HDDISPO2_VIDEO1"
#define G_VIDEO2                      "HDDISPO2_VIDEO2"
#define GAMMA_CONSTANTS

#elif defined (ST_7109)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET            ST7109_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE            "GAMMA_TYPE_7109"
#define G_CURSOR_BASE_ADDRESS         ST7109_CURSOR_LAYER_BASE_ADDRESS
#define G_GDP1_BASE_ADDRESS           ST7109_GDP1_LAYER_BASE_ADDRESS
#define G_GDP2_BASE_ADDRESS           ST7109_GDP2_LAYER_BASE_ADDRESS
#define G_GDP3_BASE_ADDRESS           ST7109_GDP3_LAYER_BASE_ADDRESS
#define G_ALPHA_BASE_ADDRESS          ST7109_ALP_LAYER_BASE_ADDRESS
#define G_MIXER1_BASE_ADDRESS         ST7109_VMIX1_BASE_ADDRESS
#define G_MIXER2_BASE_ADDRESS         ST7109_VMIX2_BASE_ADDRESS
#define G_VIDEO1                      "VDP_VIDEO1"
#define G_VIDEO2                      "HDDISPO2_VIDEO2"
#define GAMMA_CONSTANTS

#elif  defined(ST_7200)
#define MIXER_BASE_ADDRESS  0
#define MIXER_BASE_OFFSET            ST7200_VMIX1_BASE_ADDRESS
#define MIXER_DEVICE_TYPE            "GAMMA_TYPE_7200"
#define G_CURSOR_BASE_ADDRESS         ST7200_CURSOR_LAYER_BASE_ADDRESS
#define G_GDP1_BASE_ADDRESS           ST7200_GDP1_LAYER_BASE_ADDRESS
#define G_GDP2_BASE_ADDRESS           ST7200_GDP2_LAYER_BASE_ADDRESS
#define G_GDP3_BASE_ADDRESS           ST7200_GDP3_LAYER_BASE_ADDRESS
#define G_GDP4_BASE_ADDRESS           ST7200_GDP4_LAYER_BASE_ADDRESS
#define G_GDP5_BASE_ADDRESS           ST7200_GDP5_LAYER_BASE_ADDRESS
#define G_MIXER1_BASE_ADDRESS         ST7200_VMIX1_BASE_ADDRESS
#define G_MIXER2_BASE_ADDRESS         ST7200_VMIX2_BASE_ADDRESS
#define G_MIXER3_BASE_ADDRESS         ST7200_VMIX3_BASE_ADDRESS
#define G_VIDEO1                      "VDP_VIDEO2"
#define G_VIDEO2                      "VDP_VIDEO3"
#define G_VIDEO3                      "VDP_VIDEO4"
#define GAMMA_CONSTANTS





#elif defined (ST_GX1)
#define MIXER_BASE_ADDRESS (0xBB410000)
#define MIXER_DEVICE_TYPE  "GAMMA_TYPE_GX1"
#define MIXER_BASE_OFFSET  0x500
#else
#error Not defined address & type for mixer
#endif

typedef struct VMIX_StringToEnum_s
{
    const char String[50];
    const U32 Value;
} VMIX_StringToEnum_t;

#if defined (ST_5525) || defined (ST_5105) || defined (ST_5188) || defined (ST_5107) || defined (ST_5162)

#define FILEMAXSIZE 1024

#endif




/* Private Constant ----------------------------------------------------------- */
static const VMIX_StringToEnum_t VmixType[MAX_VMIX_TYPE] = {
    { "OMEGA1_SD", STVMIX_OMEGA1_SD },
    { "GAMMA_TYPE_7015", STVMIX_GAMMA_TYPE_7015 },
    { "GAMMA_TYPE_GX1", STVMIX_GAMMA_TYPE_GX1 },
    { "GAMMA_TYPE_7020", STVMIX_GENERIC_GAMMA_TYPE_7020 },
    { "GAMMA_TYPE_5528", STVMIX_GENERIC_GAMMA_TYPE_5528 },
    { "COMPO_TYPE_5100", STVMIX_COMPOSITOR },
    { "COMPO_TYPE_5105", STVMIX_COMPOSITOR },
    { "COMPO_TYPE_5107", STVMIX_COMPOSITOR },
    { "COMPO_TYPE_5162", STVMIX_COMPOSITOR },
    { "COMPO_TYPE_5301", STVMIX_COMPOSITOR },
    { "COMPO_TYPE_5525", STVMIX_COMPOSITOR },
    { "GAMMA_TYPE_7710", STVMIX_GENERIC_GAMMA_TYPE_7710 },
    { "GAMMA_TYPE_7100", STVMIX_GENERIC_GAMMA_TYPE_7100 },
    { "GAMMA_TYPE_7109", STVMIX_GENERIC_GAMMA_TYPE_7109 },
    { "GAMMA_TYPE_7200", STVMIX_GENERIC_GAMMA_TYPE_7200 },
    { "COMPO_TYPE_COMPOSITOR_FIELD", STVMIX_COMPOSITOR_FIELD },
    { "COMPO_TYPE_COMPOSITOR_FIELD_422", STVMIX_COMPOSITOR_FIELD_422 },
    { "COMPO_TYPE_COMPOSITOR_FIELD_SDRAM", STVMIX_COMPOSITOR_FIELD_SDRAM },
    { "COMPO_TYPE_COMPOSITOR_FIELD_SDRAM_422", STVMIX_COMPOSITOR_FIELD_SDRAM_422 },
    { "COMPO_TYPE_COMPOSITOR_FIELD_COMBINED", STVMIX_COMPOSITOR_FIELD_COMBINED },
    { "COMPO_TYPE_COMPOSITOR_FIELD_COMBINED_422", STVMIX_COMPOSITOR_FIELD_COMBINED_422 },
    { "COMPO_TYPE_COMPOSITOR_FIELD_COMBINED_SDRAM", STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM },
    { "COMPO_TYPE_COMPOSITOR_FIELD_COMBINED_SDRAM_422", STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422 },
    { "COMPO_TYPE_COMPOSITOR_422", STVMIX_COMPOSITOR_422 }
};

/* Private Variables (static)------------------------------------------------ */

static STVMIX_Handle_t  MixHndl;                          /* Handle for mixer */
static STVMIX_LayerDisplayParams_t LayerParam[MAX_LAYER]; /* Layer parameters */
static char VMIX_Msg[400];                          /* Text for trace */

/* Global Variables --------------------------------------------------------- */
#if defined (ST_5100) || defined (ST_5301)|| defined (ST_5525) || defined (ST_5162)
STGXOBJ_Bitmap_t  VMIXTEST_CacheBitmap;
#endif
#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
STAVMEM_BlockHandle_t       CacheBlockHandle;
STGXOBJ_Bitmap_t            VMIXTEST_CacheBitmap;
#endif

/* Private Macros ----------------------------------------------------------- */

#define SIZEOF_STRING(x) (sizeof(x)/sizeof(char))

#define VMIX_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>SIZEOF_STRING(VMIX_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \


/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
 * Function : VMIX_TextError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VMIX_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if (API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVMIX_ERROR_LAYER_UNKNOWN:
                strcat( Text, "(Layer unknown by mixer) !\n");
                break;
            case STVMIX_ERROR_LAYER_UPDATE_PARAMETERS:
                strcat( Text, "(Layer cannot support parameters) !\n");
                break;
            case STVMIX_ERROR_LAYER_ALREADY_CONNECTED:
                strcat( Text, "(Layer already connected & VTG are different) !\n");
                break;
            case STVMIX_ERROR_LAYER_ACCESS:
                strcat( Text, "(Mixer can't acces to connected layer) !\n");
                break;
            case STVMIX_ERROR_VOUT_UNKNOWN:
                strcat( Text, "(Vout doesn't exist) !\n");
                break;
            case STVMIX_ERROR_VOUT_ALREADY_CONNECTED:
                strcat( Text, "(Vout already connected to another mixer) !\n");
                break;
            case STVMIX_ERROR_NO_VSYNC:
                strcat( Text, "(VSync doesn't occur) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VMIX_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : API_TagExpected
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void API_TagExpected(const VMIX_StringToEnum_t * UnionType, const U32 MaxValue, const char * DefaultValue)
{
    U32  i;

    sprintf(VMIX_Msg, "Expected enum (");
    for(i=0; i<MaxValue; i++)
    {
        if(!strcmp(UnionType[i].String, DefaultValue))
        {
            strcat(VMIX_Msg,"def ");
        }
        sprintf(VMIX_Msg, "%s%d=%s,", VMIX_Msg, UnionType[i].Value, UnionType[i].String);
    }
    /* assumes that one component is listed at least */
    strcat(VMIX_Msg,"\b)");
    if(strlen(VMIX_Msg) >= SIZEOF_STRING(VMIX_Msg))
    {
        sprintf(VMIX_Msg, "String is too short\n");
    }
}

/*#######################################################################*/
/*###################### MIXER REGISTER COMMANDS ########################*/
/*######################  (in alphabetic order)  ########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VMIX_Close
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

     UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_Close(%d): ", MixHndl );
    ErrCode = STVMIX_Close( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Close() */


/*-------------------------------------------------------------------------
 * Function : VMIX_ConnectLayers
 *            Connect layers to the outputs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_ConnectLayers( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t                ErrCode;
    STVMIX_LayerDisplayParams_t*  LayerArray[MAX_LAYER];
    char               DeviceName[STRING_DEVICE_LENGTH];
    BOOL                          RetErr;
    U16                           i = 0, j;

    DeviceName[0] = '\0';
     UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    do{
        RetErr = STTST_GetString( pars_p, "", DeviceName, SIZEOF_STRING(DeviceName));
        strcpy(LayerParam[i].DeviceName, DeviceName);
        if(LayerParam[i].DeviceName[0] == 0x00)
            RetErr=TRUE;
        else{
            LayerArray[i] = &LayerParam[i];
            i++;
            if(i == MAX_LAYER)
                RetErr=TRUE;
        }
    } while (!RetErr);

    sprintf( VMIX_Msg, "STVMIX_ConnectLayers (%d", MixHndl);
    for(j=0; j<i; j++){
        sprintf(VMIX_Msg, "%s, %s", VMIX_Msg, LayerParam[j].DeviceName);
        LayerParam[j].ActiveSignal = FALSE;
    }
    sprintf( VMIX_Msg, "%s):", VMIX_Msg);
    ErrCode = STVMIX_ConnectLayers(MixHndl, (const STVMIX_LayerDisplayParams_t**)LayerArray, i);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_ConnectLayers() */


/*-------------------------------------------------------------------------
 * Function : VMIX_DisconnectLayers
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_DisconnectLayers( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

     UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_DisconnectLayers(%d): ", MixHndl);
    ErrCode = STVMIX_DisconnectLayers( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Disable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Disable
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Disable( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
     UNUSED_PARAMETER(result_sym_p);


    sprintf( VMIX_Msg, "STVMIX_Disable(%d): ", MixHndl);
    ErrCode = STVMIX_Disable( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Disable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Enable
 *            Enable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Enable( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_Enable(%d): ", MixHndl);
    ErrCode = STVMIX_Enable( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Enable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetBackgroundColor
 *            Get background color
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetBackgroundColor( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STGXOBJ_ColorRGB_t    Color;
    BOOL                  Enable;
    BOOL                  RetErr;
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetBackgroundColor(%d): ", MixHndl);
    ErrCode = STVMIX_GetBackgroundColor( MixHndl, &Color, &Enable);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tEnable=%s  Red=%d Green=%d Blue=%d\n",
                (Enable) ? "TRUE" :
                (!Enable) ?  "FALSE" : "?????",
                Color.R, Color.G, Color.B );
        sprintf(StrParams, "%d %d %d %d",
                Enable, Color.R, Color.G, Color.B);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_GetBackgroundColor */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetCapability
 *            Retreive driver's capabilities
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVMIX_Capability_t Capability;
    char DeviceName[STRING_DEVICE_LENGTH];
    char StrParams[RETURN_PARAMS_LENGTH];
    BOOL RetErr;

    /* get DeviceName */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, SIZEOF_STRING(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetCapability(%s): ",  DeviceName);
    ErrCode = STVMIX_GetCapability(DeviceName, &Capability);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if(ErrCode == ST_NO_ERROR){
        sprintf(VMIX_Msg, "\tOffsetHorizontalMin = %d  OffsetHorizontalMax = %d\n"
                " \tOffsetVerticalMin = %d    OffsetVerticalMax = %d\n",
                Capability.ScreenOffsetHorizontalMin,
                Capability.ScreenOffsetHorizontalMax,
                Capability.ScreenOffsetVerticalMin ,
                Capability.ScreenOffsetVerticalMax);
        VMIX_PRINT(( VMIX_Msg ));
        sprintf(StrParams, "%d %d %d %d",
                Capability.ScreenOffsetHorizontalMin,
                Capability.ScreenOffsetHorizontalMax,
                Capability.ScreenOffsetVerticalMin ,
                Capability.ScreenOffsetVerticalMax);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_GetCapability() */

/*-------------------------------------------------------------------------
 * Function : VMIX_GetConnectedLayer
 *            Retrieve layer connected information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetConnectedLayer( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t  ErrCode;
    BOOL            RetErr;
    char            StrParams[RETURN_PARAMS_LENGTH];
    S32             LayerPosition;
    STVMIX_LayerDisplayParams_t LayerDisplayParams;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 1, &LayerPosition );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected layer number (def 1 = farthest from the eyes)" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetConnectedLayer(%d,LayerNb=%d): ", MixHndl, LayerPosition);
    ErrCode = STVMIX_GetConnectedLayers( MixHndl, (U16)LayerPosition, &LayerDisplayParams );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tLayer=\"%s\" Active=%s\n",
                LayerDisplayParams.DeviceName,
                (LayerDisplayParams.ActiveSignal) ? "TRUE" : "FALSE");
        sprintf(StrParams, "%s %d", LayerDisplayParams.DeviceName, LayerDisplayParams.ActiveSignal );
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VMIX_GetScreenOffset
 *            Retrieve the screen Offset parameter information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetScreenOffset( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t  ErrCode;
    S8              Horizontal, Vertical;
    BOOL            RetErr;
    char            StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetScreenOffset(%d): ", MixHndl);
    ErrCode = STVMIX_GetScreenOffset( MixHndl, &Horizontal, &Vertical );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tHorizontal=%d Vertical=%d\n", Horizontal, Vertical );
        sprintf(StrParams, "%d %d", Horizontal, Vertical );
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetScreenOffset() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetScreenParams
 *            Retrieve the screen parameter information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetScreenParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STVMIX_ScreenParams_t ScreenParams;
    BOOL                  RetErr;
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetScreenParams(%d): ", MixHndl);
    ErrCode = STVMIX_GetScreenParams( MixHndl, &ScreenParams );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tScanType=%s AspectRatio=%s FRate=%d\n",
                (ScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROGRESSIVE" :
                (ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTERLACED" : "?????",
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                ScreenParams.FrameRate);
        sprintf(VMIX_Msg, "%s\tWidth=%d Height=%d XStart=%d YStart=%d\n",
                VMIX_Msg, ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);
        sprintf(StrParams, "%d %d %d %d %d %d %d",
                ScreenParams.ScanType, ScreenParams.AspectRatio, ScreenParams.FrameRate,\
                ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetScreenParams() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetTimeBase
 *            Retrieve the VTG name
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetTimeBase( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    BOOL                  RetErr;
    char                  DeviceName[STRING_DEVICE_LENGTH];
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetTimeBase(%d): ", MixHndl);
    ErrCode = STVMIX_GetTimeBase( MixHndl, (ST_DeviceName_t*)&DeviceName );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tVTG name=\"%s\"\n", DeviceName );
        sprintf(StrParams, "%s", DeviceName);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetTimeBase() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

     UNUSED_PARAMETER(result_sym_p);
      UNUSED_PARAMETER(pars_p);


    RevisionNumber = STVMIX_GetRevision();
    sprintf( VMIX_Msg, "STVMIX_GetRevision(): Ok Revision=%s\n", RevisionNumber );
    VMIX_PRINT(( VMIX_Msg ));
    return ( FALSE );

} /* end VMIX_GetRevision() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Init
 *            Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH], TrvStr[80], IsStr[80];
    char  *ptr;
    STVMIX_InitParams_t InitParams;
    ST_DeviceName_t VoutArray [MAX_VOUT];
    S32 Var, DevNb;
    BOOL RetErr;
    U16  i=0,j ;



#ifdef STVMIX_USE_GLOBAL_DEVICE_TYPE
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188)|| defined(ST_5525) || defined (ST_5107)\
 || defined (ST_5162)
    S32 VmixDeviceType = 0;
#endif /* defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162)*/
#endif /* STVMIX_USE_GLOBAL_DEVICE_TYPE*/
#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    U8*                         Cache_p;
    STAVMEM_AllocBlockParams_t  AllocParams;
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#endif /* defined(ST_5105) || defined(ST_5188) || defined (ST_5107) */

UNUSED_PARAMETER(result_sym_p);
    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get DeviceName */
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, SIZEOF_STRING(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    /* Get DeviceType */
    STTST_GetItem( pars_p, MIXER_DEVICE_TYPE, TrvStr, SIZEOF_STRING(TrvStr));;
    RetErr = STTST_EvaluateString( TrvStr, IsStr, SIZEOF_STRING(IsStr));
    if(RetErr==FALSE)
    {
        strncpy(TrvStr, IsStr, SIZEOF_STRING(TrvStr));
    }
    for(DevNb=0; DevNb<MAX_VMIX_TYPE; DevNb++)
    {
        if((strcmp(VmixType[DevNb].String, TrvStr) == 0 ) ||
           (strncmp(VmixType[DevNb].String, TrvStr+1, strlen(VmixType[DevNb].String)) == 0 ))
            break;
    }
    if(DevNb==MAX_VMIX_TYPE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, &DevNb, 10);
        if(RetErr)
        {
            DevNb = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                API_TagExpected(VmixType, MAX_VMIX_TYPE, MIXER_DEVICE_TYPE);
                STTST_TagCurrentLine(pars_p, VMIX_Msg);
                return(TRUE);
            }
        }
        InitParams.DeviceType = DevNb;

        for (DevNb=0; DevNb<MAX_VMIX_TYPE; DevNb++)
        {
            if(VmixType[DevNb].Value == InitParams.DeviceType)
                break;
        }
    }
    else
    {
        InitParams.DeviceType = VmixType[DevNb].Value;
    }

    /* get BaseAddress */
#if defined(ST_5528) && defined(ST_7020)
    if (InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_5528)
    {
        RetErr = STTST_GetInteger( pars_p, MIXER_5528_BASE_OFFSET, &Var );
    }
    else
#endif /* defined(ST_5528) && defined(ST_7020) */
    {
        RetErr = STTST_GetInteger( pars_p, MIXER_BASE_OFFSET, &Var );
    }
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Base Address" );
        return(TRUE);
    }

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107) || defined (ST_5162)
    InitParams.RegisterInfo.GdmaBaseAddress_p  = (void*)Var;
#else
    InitParams.BaseAddress_p = (void*)Var;
#endif

    /* get VTGName */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, InitParams.VTGName, sizeof(ST_DeviceName_t));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected VTGName" );
        return(TRUE);
    }

    /* get MaxOpen */
    RetErr = STTST_GetInteger( pars_p, 1, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxOpen" );
        return(TRUE);
    }
    InitParams.MaxOpen = Var;

    /* get MaxLayer */
    RetErr = STTST_GetInteger( pars_p, MAX_LAYER , &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxLayer" );
        return(TRUE);
    }
    InitParams.MaxLayer=(S16)Var;

    /* Get Vout Name */
    do{
        RetErr = STTST_GetString( pars_p, "", VoutArray[i], sizeof(ST_DeviceName_t));
        if((VoutArray[i][0] == '\0'))
        {
            if (VoutArray[0][0] == '\0')
                InitParams.OutputArray_p = NULL;
            else{
                InitParams.OutputArray_p = VoutArray;
            }
            break;
        }
        i++;
    } while ((!RetErr) && (i<MAX_VOUT));

    if(i == MAX_VOUT)
    {
        STTST_TagCurrentLine( pars_p, "Max output reach !!!" );
        return(TRUE);
    }

    InitParams.CPUPartition_p = DriverPartition_p;

#ifdef STVMIX_USE_GLOBAL_DEVICE_TYPE
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107) || defined (ST_5162)

    /* get VMIX Device Type */
    RetErr = STTST_EvaluateInteger("G_DeviceType", &VmixDeviceType, 0);
    if (RetErr)
    {
        return(TRUE);
    }

    switch(VmixDeviceType)
    {
        case 0 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR;
            break;

        case 1 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_422;
            break;

        case 2 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD;
            break;

        case 3 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_422;
            break;

        case 4 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_SDRAM;
            break;

        case 5 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_SDRAM_422;
            break;

         case 6 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_COMBINED;
            break;

        case 7 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_COMBINED_422;
            break;

        case 8 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM;
            break;

        case 9 :
            InitParams.DeviceType =  STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422;
            break;

        default:
            InitParams.DeviceType =  STVMIX_COMPOSITOR;
            break;
    }

    DevNb=0;
    while (( VmixType[DevNb].Value != InitParams.DeviceType ) && (DevNb < MAX_VMIX_TYPE))
    {
        DevNb++;
    }
#endif /* defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162)*/
#endif /* STVMIX_USE_GLOBAL_DEVICE_TYPE */
#if defined (ST_5100) || defined (ST_5301) || defined (ST_5525) || defined (ST_5162)
    InitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    InitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType             = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion  = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p               = (void*)MIXER_TILE_RAM_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.Size1                 = MIXER_TILE_RAM_SIZE;
    InitParams.CacheBitmap_p                   = &VMIXTEST_CacheBitmap;
    InitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
 #if defined (ST_5100)
 #if defined (USE_AVMEMCACHED_PARTITION)
    InitParams.AVMEM_Partition2                = AvmemPartitionHandle[1];
 #else
    InitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
 #endif
 #else  /* defined (ST_5100)*/
    InitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];
 #endif
#elif defined (ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Allocate a block to be used as a cache in the composition operations */
    AllocParams.PartitionHandle          = AvmemPartitionHandle[0];
    AllocParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocParams.NumberOfForbiddenRanges  = 0;
    AllocParams.ForbiddenRangeArray_p    = NULL;
    AllocParams.NumberOfForbiddenBorders = 0;
    AllocParams.ForbiddenBorderArray_p   = NULL;
    AllocParams.Size                     = MIXER_CACHE_SIZE;
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
    InitParams.RegisterInfo.CompoBaseAddress_p = (void*)MIXER_COMPO_BASE_ADDRESS;
    InitParams.RegisterInfo.VoutBaseAddress_p  = (void*)MIXER_VOUT_BASE_ADDRESS;
    VMIXTEST_CacheBitmap.ColorType             = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_444;
    VMIXTEST_CacheBitmap.ColorSpaceConversion  = STGXOBJ_ITU_R_BT601;
    VMIXTEST_CacheBitmap.Data1_p               = (void*)Cache_p;
    VMIXTEST_CacheBitmap.Size1                 = MIXER_CACHE_SIZE;
    InitParams.CacheBitmap_p                   = &VMIXTEST_CacheBitmap;
    InitParams.AVMEM_Partition                 = AvmemPartitionHandle[0];
    InitParams.AVMEM_Partition2                = AvmemPartitionHandle[0];

#elif defined(ST_5528) && defined(ST_7020)

    if (InitParams.DeviceType == STVMIX_GENERIC_GAMMA_TYPE_5528)
    {
        InitParams.DeviceBaseAddress_p = (void *)MIXER_5528_BASE_ADDRESS;
    }
    else
    {
        InitParams.DeviceBaseAddress_p = (void *)MIXER_BASE_ADDRESS;
    }
#else
        InitParams.DeviceBaseAddress_p = (void *)MIXER_BASE_ADDRESS;
#endif

    strncpy(InitParams.EvtHandlerName, STEVT_DEVICE_NAME, sizeof(ST_DeviceName_t));

    sprintf( VMIX_Msg, "STVMIX_Init(%s,%s,0x%X,VTG=\"%s\",Open=%d,Lay=%d,Vout=", \
             DeviceName,
             (DevNb < MAX_VMIX_TYPE) ? VmixType[DevNb].String : "????",
             (U32)InitParams.BaseAddress_p, (char*) InitParams.VTGName, InitParams.MaxOpen, InitParams.MaxLayer);
    for(j=0; j<i; j++){
        if(j!=0)
            strcat(VMIX_Msg, ",");
        sprintf( VMIX_Msg, "%s%s", VMIX_Msg, VoutArray[j]);
    }

    strcat(VMIX_Msg, "): ");

    ErrCode = STVMIX_Init( DeviceName, &InitParams);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Init() */



/*-------------------------------------------------------------------------
 * Function : VMIX_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVMIX_OpenParams_t OpenParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;

    /* get device */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, SIZEOF_STRING(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf( VMIX_Msg, "STVMIX_Open(%s): ", DeviceName );
    ErrCode = STVMIX_Open( DeviceName, &OpenParams, &MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
    sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)MixHndl );
    VMIX_PRINT(( VMIX_Msg ));
    if ( ErrCode == ST_NO_ERROR)
    {
        STTST_AssignInteger( result_sym_p, (int)MixHndl, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Open() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetBackgroundColor
 *            Set background color
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetBackgroundColor( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STGXOBJ_ColorRGB_t    Color;
    BOOL                  Enable;
    BOOL                  RetErr;
    S32                   Var;

     UNUSED_PARAMETER(result_sym_p);


    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    /* Get Enable */
    RetErr = STTST_GetInteger( pars_p, TRUE, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Enable (default TRUE)" );
        return(TRUE);
    }
    Enable = (BOOL)Var;

    /* Get Red */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Red (default 127)" );
        return(TRUE);
    }
    Color.R = (U8)Var;

    /* Get Green */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Green (default 127)" );
        return(TRUE);
    }
    Color.G = (U8)Var;

    /* Get Blue */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Blue (default 127)" );
        return(TRUE);
    }
    Color.B = (U8)Var;

    sprintf( VMIX_Msg, "STVMIX_SetBackgroundColor(%d,Enable=%s,R=%d,G=%d,B=%d): ", MixHndl,
             (Enable) ? "TRUE" :
             (!Enable) ?  "FALSE" : "?????",
             Color.R, Color.G, Color.B );

    ErrCode = STVMIX_SetBackgroundColor( MixHndl, Color, Enable);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_SetBackgroundColor() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetScreenOffset
 *            Change screen offsets
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetScreenOffset( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t     ErrCode;
    BOOL               RetErr;
    S32                XOffset, YOffset;

    UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &XOffset );
    if ((RetErr) || (XOffset > 127) || (XOffset < -128))
    {
        STTST_TagCurrentLine( pars_p, "Expected XOffset -128->127(default 0)" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &YOffset );
    if ((RetErr)  || (YOffset > 127) || (YOffset < -128))
    {
        STTST_TagCurrentLine( pars_p, "Expected YOffset -128->127(default 0)" );
        return(TRUE);
    }

    ErrCode = STVMIX_SetScreenOffset( MixHndl, (S8)XOffset, (S8)YOffset);
    sprintf( VMIX_Msg, "STVMIX_SetScreenOffset(%d,XOff=%d,YOff=%d): ", MixHndl, XOffset, YOffset);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_SetScreenOffset() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetScreenParams
 *            Change screen paramaters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetScreenParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t         ErrCode;
    STVMIX_ScreenParams_t  ScreenParams;
    BOOL                   RetErr;
    S32                    Var;

    ErrCode = ST_ERROR_BAD_PARAMETER;


     UNUSED_PARAMETER(result_sym_p);


    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN , &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected ScanType" );
        return(TRUE);
    }
    ScreenParams.ScanType = (STGXOBJ_ScanType_t) Var;

    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected AspectRatio" );
        return(TRUE);
    }
    ScreenParams.AspectRatio = (STGXOBJ_AspectRatio_t) Var;

    RetErr = STTST_GetInteger( pars_p, 50000, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Frame Rate" );
        return(TRUE);
    }
    ScreenParams.FrameRate = Var;

    RetErr = STTST_GetInteger( pars_p, 720, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    ScreenParams.Width = Var;

    RetErr = STTST_GetInteger( pars_p, 576, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Height" );
        return(TRUE);
    }
    ScreenParams.Height = Var;

    RetErr = STTST_GetInteger( pars_p, 132, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Xstart" );
        return(TRUE);
    }
    ScreenParams.XStart = Var;

    RetErr = STTST_GetInteger( pars_p, 23, &Var );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Ystart" );
        return(TRUE);
    }
    ScreenParams.YStart = Var;

    sprintf( VMIX_Msg, "STVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): ", \
             MixHndl,
             (ScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
             (ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
             ScreenParams.FrameRate,
             ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);

    ErrCode = STVMIX_SetScreenParams( MixHndl, &ScreenParams);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_SetScreenParams() */


/*-------------------------------------------------------------------------
 * Function : VMixSetTimeBase
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetTimeBase( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;


     UNUSED_PARAMETER(result_sym_p);


    ErrCode = ST_ERROR_BAD_PARAMETER;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, SIZEOF_STRING(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected VTG DeviceName" );
    }

    ErrCode = STVMIX_SetTimeBase( MixHndl, DeviceName);
    sprintf( VMIX_Msg, "STVMIX_SetTimeBase(%d, VTGName=\"%s\"): ", MixHndl, DeviceName);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMixSetTimeBase() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Term
 *            Terminate the mixer decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    STVMIX_TermParams_t TermParams;
    S32 LVar;
    BOOL RetErr;




#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    STAVMEM_FreeBlockParams_t   FreeBlockParams;
#endif /* defined(ST_5105) || defined(ST_5188) || defined (ST_5107) */
    ErrCode = ST_ERROR_BAD_PARAMETER;

    UNUSED_PARAMETER(result_sym_p);
#if defined(ST_5105) || defined(ST_5188) || defined (ST_5107)
    /* Free the allocated block before leaving */
    FreeBlockParams.PartitionHandle = AvmemPartitionHandle[0];
    ErrCode = STAVMEM_FreeBlock(&FreeBlockParams,&CacheBlockHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("Error : Can't deallocate the cache block : %s\n",ErrCode));
    }
#endif
    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, SIZEOF_STRING(DeviceName));
    if (RetErr)
    {
       STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
       return ( API_EnableError ? TRUE : FALSE );
    }

    /* get term param */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected ForceTerminate (TRUE, FALSE)" );
        return ( API_EnableError ? TRUE : FALSE );
    }
    TermParams.ForceTerminate = (BOOL)LVar;

    sprintf( VMIX_Msg, "STVMIX_Term(%s,forceterminate=%d): ", DeviceName, TermParams.ForceTerminate );
    ErrCode = STVMIX_Term( DeviceName, &TermParams );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Term */

#if !defined (ST_OSLINUX)
/*-------------------------------------------------------------------------
 * Function : VMIX_CLDelayWA
 *            Terminate the mixer decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_CLDelayWA( STTST_Parse_t *pars_p, char *result_sym_p )
{

    BOOL RetErr=FALSE;
     UNUSED_PARAMETER(result_sym_p);

     #if defined (ST_7109) || defined(ST_7710) || defined(ST_7100)  || defined(ST_7200)
         UNUSED_PARAMETER(pars_p);
     #endif

#ifdef WA_GNBvd31390

    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    STVTG_TimingMode_t ModeVtg;
    S32 LVar;
    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get device name */
    DeviceName[0] = '\0';
     RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }
     RetErr = STTST_GetInteger( pars_p, ModeVtg, (S32*)&ModeVtg );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected vtg mode\n");
        return(TRUE);
    }

    ErrCode = STVMIX_CLDelayWA( MixHndl,ModeVtg );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

#endif /*WA_GNBvd31390*/

    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_CLDelayWA */
#endif   /* !defined (ST_OSLINUX) */

 #if  defined(ST_5525)
 /*---------------------------------------------------------------------
 * Function : UpdateDencTxtParams
 *            Update DENC & VTG registers.
 *
 * Input    : void *ptr
 * Output   :
 * Return   : ErrCode
 * ----------------------------------------------------------------- */

ST_ErrorCode_t UpdateDencTxtParams(void)
{

ST_ErrorCode_t err=ST_NO_ERROR;


 /* FULL PAGE &   MASK DELL  DENC_TTX1 */
 STSYS_WriteRegDev8(0x19a00088,0x47);

             /*DENC_ TTX2 */
STSYS_WriteRegDev8(0x19a0008C,0xFF);
            /* DENC_TTX3 */
STSYS_WriteRegDev8(0x19a00090,0xFF);
            /* DENC_TTX4 */
STSYS_WriteRegDev8(0x19a00094,0xF0);
            /* DENC_TTX5 */
STSYS_WriteRegDev8(0x19a00098,0xFF);

            /*  DENC_TXTCFG  */
STSYS_WriteRegDev8(0x19a00100,0x30);

            /*  DEN_DACC  */ /*Framing Code Enable */
STSYS_WriteRegDev8(0x19a00104,0x8a);

            /*  DENC_CFG8   */
 STSYS_WriteRegDev8(0x19a00020,0x24);

             /*  DENC_CFG6   */
 STSYS_WriteRegDev8(0x19a00018,0x2);

            /*  TTXT VOUT  */
 /*   TTX_OUT_DELAY 4 ;TXT_ENABLE */
STSYS_WriteRegDev8(0x19a0070C,0x0F);






return err;

 }

ST_ErrorCode_t UpdateDencTxtParams2(void)
{

ST_ErrorCode_t err=ST_NO_ERROR;


 /* FULL PAGE &   MASK DELL  DENC_TTX1 */
 STSYS_WriteRegDev8(0x19B00088,0x47);

             /*DENC_ TTX2 */
STSYS_WriteRegDev8(0x19B0008C,0xFF);
            /* DENC_TTX3 */
STSYS_WriteRegDev8(0x19B00090,0xFF);
            /* DENC_TTX4 */
STSYS_WriteRegDev8(0x19B00094,0xF0);
            /* DENC_TTX5 */
STSYS_WriteRegDev8(0x19B00098,0xFF);

            /*  DENC_TXTCFG  */
STSYS_WriteRegDev8(0x19B00100,0x30);

            /*  DEN_DACC  */ /*Framing Code Enable */
STSYS_WriteRegDev8(0x19B00104,0x8a);

            /*  DENC_CFG8   */
 STSYS_WriteRegDev8(0x19B00020,0x24);

             /*  DENC_CFG6   */
 STSYS_WriteRegDev8(0x19B00018,0x2);

            /*  TTXT VOUT  */
 /*   TTX_OUT_DELAY 4 ;TXT_ENABLE */
STSYS_WriteRegDev8(0x19B0070C,0x0F);




return err;

 }



 #endif



 #if  defined(ST_5105) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162)


 /*---------------------------------------------------------------------
 * Function : UpdateDencTxtParams
 *            Update DENC & VTG registers.
 *
 * Input    : void *ptr
 * Output   :
 * Return   : ErrCode
 * ----------------------------------------------------------------- */

ST_ErrorCode_t UpdateDencTxtParams(void)
{

ST_ErrorCode_t err=ST_NO_ERROR;


 /* FULL PAGE &   MASK DELL  DENC_TTX1 */
 STSYS_WriteRegDev8(0x20900088,0x47);

             /*DENC_ TTX2 */
STSYS_WriteRegDev8(0x2090008C,0xFF);
            /* DENC_TTX3 */
STSYS_WriteRegDev8(0x20900090,0xFF);
            /* DENC_TTX4 */
STSYS_WriteRegDev8(0x20900094,0xF0);
            /* DENC_TTX5 */
STSYS_WriteRegDev8(0x20900098,0xFF);

            /*  DENC_TXTCFG  */
STSYS_WriteRegDev8(0x20900100,0x30);

            /*  DEN_DACC  */ /*Framing Code Enable */
STSYS_WriteRegDev8(0x20900104,0x8a);

            /*  DENC_CFG8   */
 STSYS_WriteRegDev8(0x20900020,0x24);

             /*  DENC_CFG6   */
 STSYS_WriteRegDev8(0x20900018,0x2);

            /*  TTXT VOUT  */
 /*   TTX_OUT_DELAY 4 ;TXT_ENABLE */
STSYS_WriteRegDev8(0x2090070C,0x0F);


return err;

 }


 #endif


 #if  !defined (ST_OSLINUX)
/*-------------------------------------------------------------------------
 * Function : VMIX_VBI_Test
 *            Test  STVMIX VBI  function
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_VBI_Test( STTST_Parse_t *pars_p, char *result_sym_p )
{

    BOOL RetErr=FALSE;

#if defined (ST_5105) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107) || defined (ST_5162)
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];

    STVMIX_VBIViewPortHandle_t  VBIVPDisplay1,VBIVPDisplay2;
    STVMIX_VBIViewPortParams_t  VBIVPDisplayParams1,VBIVPDisplayParams2;

    char FileName[127];
    long int Size;
    char *      Buffer;
    char *      AlignBuffer;
    FILE * fstream;

    UNUSED_PARAMETER(result_sym_p);

    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
     if (RetErr)
     {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
     }
     ErrCode = ST_ERROR_BAD_PARAMETER;

     /*strcpy(FileName, "../../simple4.ttx");
     FileDescriptor = debugopen(FileName, "rb" );
     Size = debugfilesize(FileDescriptor);
     Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((Size  + 15) & (~15)));
     AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));
     debugread(FileDescriptor, AlignBuffer, (size_t) Size);
     debugclose(FileDescriptor);
      */

    strcpy(FileName, "../../simple5.ttx");

    fstream = fopen(FileName, "rb");

    Buffer = (char *)STOS_MemoryAllocate(NCachePartition_p, (U32)((FILEMAXSIZE + 15) & (~15)));

    AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));

    Size = fread(AlignBuffer,1,FILEMAXSIZE,fstream);

    /*debugread(FileDescriptor, AlignBuffer, (size_t) Size);*/
    fclose(fstream);

    ErrCode = UpdateDencTxtParams(); /* Update denc Registers */

    RetErr = TRUE;
    sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl,STVMIX_VBI_TXT_ODD,&VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay1 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }


     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

     if(RetErr)
     {
         return (TRUE);
     }

      sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl,STVMIX_VBI_TXT_EVEN,&VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay2 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }

     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     VBIVPDisplayParams1.Source_p        =(U8 *)(AlignBuffer);
     VBIVPDisplayParams1.LineMask        = 0xFF;


     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay1,&VBIVPDisplayParams1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }



     VBIVPDisplayParams2.Source_p        = (U8 *)( AlignBuffer+(8*48));
     VBIVPDisplayParams2.LineMask        = 0x00FF;

     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay2,&VBIVPDisplayParams2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


#if defined (ST_5525)
                /* wait some seconds */
     task_delay(20000000);
#else
     task_delay(200000000);
#endif

                   /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

                  /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     STOS_MemoryDeallocate(NCachePartition_p,Buffer);

#endif /*#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162)*/

#if defined (ST_7109) || defined(ST_7710) || defined(ST_7100)|| defined(ST_7200)
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
#endif
    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_VBI_Test */



/*-------------------------------------------------------------------------
 * Function : VMIX_VBI_Test2
 *            Test  STVMIX VBI  function
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_VBI_Test2( STTST_Parse_t *pars_p, char *result_sym_p )
{

    BOOL RetErr=FALSE;

     UNUSED_PARAMETER(result_sym_p);

     #if defined (ST_7109) || defined(ST_7710) || defined(ST_7100) || defined(ST_7200)
         UNUSED_PARAMETER(pars_p);
     #endif

#if defined(ST_5525)

    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];

    STVMIX_VBIViewPortHandle_t  VBIVPDisplay1,VBIVPDisplay2;
    STVMIX_VBIViewPortParams_t  VBIVPDisplayParams1,VBIVPDisplayParams2;

    char FileName[127];
    long int Size;
    char *      Buffer;
    char *      AlignBuffer;
    FILE * fstream;


       /* get device name */
     DeviceName[0] = '\0';
     RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
     if (RetErr)
     {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
     }


     ErrCode = ST_ERROR_BAD_PARAMETER;

     /*strcpy(FileName, "../../simple4.ttx");
     FileDescriptor = debugopen(FileName, "rb" );
     Size = debugfilesize(FileDescriptor);
     Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((Size  + 15) & (~15)));
     AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));
     debugread(FileDescriptor, AlignBuffer, (size_t) Size);
     debugclose(FileDescriptor);
      */

    strcpy(FileName, "../../simple4.ttx");

    fstream = fopen(FileName, "rb");

    Buffer = (char *)STOS_MemoryAllocate(NCachePartition_p, (U32)((FILEMAXSIZE + 15) & (~15)));

    AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));

    Size = fread(AlignBuffer,1,FILEMAXSIZE,fstream);


    /*        debugread(FileDescriptor, AlignBuffer, (size_t) Size);*/
    fclose(fstream);





     ErrCode = UpdateDencTxtParams2(); /* Update denc Registers */


    RetErr = TRUE;
    sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl,STVMIX_VBI_TXT_ODD,&VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay1 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }


     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

     if(RetErr)
     {
         return (TRUE);
     }

      sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl,STVMIX_VBI_TXT_EVEN,&VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay2 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }

     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     VBIVPDisplayParams1.Source_p        =(U8 *)(AlignBuffer);
     VBIVPDisplayParams1.LineMask        = 0x1FFF;


     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay1,&VBIVPDisplayParams1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }



     VBIVPDisplayParams2.Source_p        = (U8 *)( AlignBuffer+(13*43));
     VBIVPDisplayParams2.LineMask        = 0x1FFF;

     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay2,&VBIVPDisplayParams2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

#if defined (ST_5525)
                /* wait some seconds */
     task_delay(20000000);
#else
     task_delay(200000000);
#endif

                   /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

                  /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     STOS_MemoryDeallocate(NCachePartition_p,Buffer);

#endif /*#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162) */

    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_VBI_Test */



/*-------------------------------------------------------------------------
 * Function : VMIX_VBI_TestMulti
 *            Test  STVMIX VBI  function
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_VBI_TestMulti( STTST_Parse_t *pars_p, char *result_sym_p )
{



    BOOL RetErr=FALSE;


     UNUSED_PARAMETER(result_sym_p);

     #if defined (ST_7109) || defined(ST_7710) || defined(ST_7100) || defined(ST_7200)

         UNUSED_PARAMETER(pars_p);

     #endif



#if defined(ST_5525)

    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];

    STVMIX_VBIViewPortHandle_t  VBIVPDisplay1,VBIVPDisplay2,VBIVPDisplay3,VBIVPDisplay4;
    STVMIX_VBIViewPortParams_t  VBIVPDisplayParams1,VBIVPDisplayParams2,VBIVPDisplayParams3,VBIVPDisplayParams4;

    STVMIX_Handle_t  MixHndl1,MixHndl2;                          /* Handle for mixer */
    char FileName[127];
    long int Size;
    char *      Buffer;
    char *      AlignBuffer;
    FILE * fstream;


       /* get device name */

     RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl1 );
     if (RetErr)
     {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
     }

     RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl2 );
     if (RetErr)
     {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
     }



     ErrCode = ST_ERROR_BAD_PARAMETER;

     /*strcpy(FileName, "../../simple4.ttx");
     FileDescriptor = debugopen(FileName, "rb" );
     Size = debugfilesize(FileDescriptor);
     Buffer = (char *)memory_allocate(NCachePartition_p, (U32)((Size  + 15) & (~15)));
     AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));
     debugread(FileDescriptor, AlignBuffer, (size_t) Size);
     debugclose(FileDescriptor);
      */

    strcpy(FileName, "../../simple4.ttx");

    fstream = fopen(FileName, "rb");

    Buffer = (char *)STOS_MemoryAllocate(NCachePartition_p, (U32)((FILEMAXSIZE + 15) & (~15)));

    AlignBuffer = (void *)(((unsigned int)Buffer + 15) & (~15));

    Size = fread(AlignBuffer,1,FILEMAXSIZE,fstream);


    /*        debugread(FileDescriptor, AlignBuffer, (size_t) Size);*/
    fclose(fstream);




     ErrCode = UpdateDencTxtParams(); /* Update denc Registers for Main */
     ErrCode = UpdateDencTxtParams2(); /* Update denc Registers  for Aux*/




     /* for Main */
    RetErr = TRUE;
    sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl1 );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl1,STVMIX_VBI_TXT_ODD,&VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay1 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }


     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

     if(RetErr)
     {
         return (TRUE);
     }

      sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl1,STVMIX_VBI_TXT_EVEN,&VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay2 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }

     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     VBIVPDisplayParams1.Source_p        =(U8 *)(AlignBuffer);
     VBIVPDisplayParams1.LineMask        = 0x1FFF;


     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay1,&VBIVPDisplayParams1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }



     VBIVPDisplayParams2.Source_p        = (U8 *)( AlignBuffer+(13*43));
     VBIVPDisplayParams2.LineMask        = 0x1FFF;

     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay2,&VBIVPDisplayParams2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

     /* for Aux */

    sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl2 );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl2,STVMIX_VBI_TXT_ODD,&VBIVPDisplay3);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay3 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }


     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay3);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay3);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

     if(RetErr)
     {
         return (TRUE);
     }

      sprintf( VMIX_Msg, "STVMIX_OpenVBIViewPort(%d): ", MixHndl2 );

         /* Open VBIViewPort  */
     ErrCode = STVMIX_OpenVBIViewPort(MixHndl2,STVMIX_VBI_TXT_EVEN,&VBIVPDisplay4);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)VBIVPDisplay4 );
     VMIX_PRINT(( VMIX_Msg ));

     if(RetErr)
     {
         return (TRUE);
     }

     sprintf( VMIX_Msg, "STVMIX_EnableVBIViewPort(%d): ", VBIVPDisplay4);
     ErrCode = STVMIX_EnableVBIViewPort(VBIVPDisplay4);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }


     VBIVPDisplayParams1.Source_p        =(U8 *)(AlignBuffer);
     VBIVPDisplayParams1.LineMask        = 0x1FFF;


     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay3);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay3,&VBIVPDisplayParams1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }



     VBIVPDisplayParams2.Source_p        = (U8 *)( AlignBuffer+(13*43));
     VBIVPDisplayParams2.LineMask        = 0x1FFF;

     sprintf( VMIX_Msg, "STVMIX_SetVBIViewPortParams(%d): ", VBIVPDisplay4);
     ErrCode = STVMIX_SetVBIViewPortParams(VBIVPDisplay4,&VBIVPDisplayParams2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }



     task_delay(50000000);


                   /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay1);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay1);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

                  /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay2);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay2);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay3);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay3);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }

                  /* Close VBIViewPort  */
     sprintf( VMIX_Msg, "STVMIX_CloseVBIViewPort(%d): ", VBIVPDisplay4);
     ErrCode = STVMIX_CloseVBIViewPort(VBIVPDisplay4);
     RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
     if(RetErr)
     {
         return (TRUE);
     }




     STOS_MemoryDeallocate(NCachePartition_p,Buffer);

#endif /*#if defined (ST_5105) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162) */

    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_VBI_Test */


/* Global Flicker Filter management */

/*-------------------------------------------------------------------------
 * Function : VMIX_EnableGlobalFlickerFilter
 *            Enable Global Flicker Filter
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_EnableGlobalFlickerFilter( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_EnableGlobalFlickerFilter(%d): ", MixHndl);
    ErrCode = STVMIX_EnableGlobalFlickerFilter( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_EnableGlobalFlickerFilter() */

/*-------------------------------------------------------------------------
 * Function : VMIX_DisableGlobalFlickerFilter
 *            Disable Global Flicker Filter
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_DisableGlobalFlickerFilter( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_DisableGlobalFlickerFilter(%d): ", MixHndl);
    ErrCode = STVMIX_DisableGlobalFlickerFilter( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_DisableGlobalFlickerFilter() */



#endif /*#if !defined (ST_OSLINUX)*/


/*#######################################################################*/
/*##########################  MIXER COMMANDS  ###########################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VMIX_InitCommand
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_InitCommand (void)
{
    BOOL RetErr;






    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "VMIX_Close",     VMIX_Close, "<Handle>, Close specified handle");
    RetErr |= STTST_RegisterCommand( "VMIX_Connect",   VMIX_ConnectLayers, \
                                    "<Handle><LayerName1><LayerName2>..., Connect Layers");
    RetErr |= STTST_RegisterCommand( "VMIX_Dconnect",  VMIX_DisconnectLayers, "<Handle>, Disconnect Layers");
    RetErr |= STTST_RegisterCommand( "VMIX_Disable",   VMIX_Disable, "<Handle>, Disable outputs of the mixer ");
    RetErr |= STTST_RegisterCommand( "VMIX_Enable",    VMIX_Enable, "<Handle>, Enable outputs of the mixer");
    RetErr |= STTST_RegisterCommand( "VMIX_Capa",      VMIX_GetCapability, "<DeviceName>, Get driver's capability");
    RetErr |= STTST_RegisterCommand( "VMIX_GBack",     VMIX_GetBackgroundColor,
                                     "<Handle>, Returns string \"Enable R G B\" ");
    RetErr |= STTST_RegisterCommand( "VMIX_GConnect",  VMIX_GetConnectedLayer,
                                     "<Handle><N>, Returns connected layer at position N from the farthest to the closest");
    RetErr |= STTST_RegisterCommand( "VMIX_GOffset",   VMIX_GetScreenOffset, "<Handle> Get screen offset");
    RetErr |= STTST_RegisterCommand( "VMIX_GScreen",   VMIX_GetScreenParams, "<Handle>, Returns string containing screen parameters");
    RetErr |= STTST_RegisterCommand( "VMIX_GTime",     VMIX_GetTimeBase, "<Handle>, Get time base");
    RetErr |= STTST_RegisterCommand( "VMIX_Init",      VMIX_Init,\
                                    "<DevName><DevType><BaseAddr><VTGName><MaxOpen><MaxLayer><Out1>...<Out?>, Initialization");
    RetErr |= STTST_RegisterCommand( "VMIX_Open",      VMIX_Open, "<DeviceName>, Get handle from device");
    RetErr |= STTST_RegisterCommand( "VMIX_Revision",  VMIX_GetRevision, "Get revision of mixer driver");
    RetErr |= STTST_RegisterCommand( "VMIX_SBack",     VMIX_SetBackgroundColor, \
                                     "<Handle><Enable><Red><Green><Blue>, Set background color");
    RetErr |= STTST_RegisterCommand( "VMIX_SOffset",   VMIX_SetScreenOffset, \
                                     "<Handle><XOffset><YOffset> Set screen offset");
    RetErr |= STTST_RegisterCommand( "VMIX_SScreen",   VMIX_SetScreenParams, \
                                     "<Handle><Scan><Ratio><FRate><Width><Height><XStart><YStart>, Set screen parameters");
    RetErr |= STTST_RegisterCommand( "VMIX_STime",     VMIX_SetTimeBase, "<Handle><VTG Name>, Set time base with VTG name");
    RetErr |= STTST_RegisterCommand( "VMIX_Term",      VMIX_Term, "<DeviceName><ForceTerminate>, Terminate");

#ifndef ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VMIX_VBI_Test",   VMIX_VBI_Test, "test VMIX vbi function on Main ");
    RetErr |= STTST_RegisterCommand( "VMIX_VBI_Test2",  VMIX_VBI_Test2, "test VMIX vbi function on Aux ");
    RetErr |= STTST_RegisterCommand( "VMIX_VBI_TestMulti",  VMIX_VBI_TestMulti, "test VMIX vbi function on Main and Aux ");

    /*WA for Chroma Luma delay bug on STi5100 : the function do nothing if WA_GNBvd31390 is not defined*/
    RetErr |= STTST_RegisterCommand( "VMIX_CLDelayWA",      VMIX_CLDelayWA, " call STDISP_CLDelayWA");

	/* Global Flicker Filter management */
	RetErr |= STTST_RegisterCommand( "VMIX_EnableGlobalFlickerFilter",      VMIX_EnableGlobalFlickerFilter, "Enable Global Flicker Filter");
	RetErr |= STTST_RegisterCommand( "VMIX_DisableGlobalFlickerFilter",     VMIX_DisableGlobalFlickerFilter, "Disable Global Flicker Filter");
#endif
    /* constants */
    RetErr |= STTST_AssignInteger ("ERR_VMIX_LAYER_UNKNOWN"         , STVMIX_ERROR_LAYER_UNKNOWN          , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_UPDATE_PARAMETERS"     , STVMIX_ERROR_LAYER_UPDATE_PARAMETERS, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_ALREADY_CONNECTED"     , STVMIX_ERROR_LAYER_ALREADY_CONNECTED, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_LAYER_ACCESS"          , STVMIX_ERROR_LAYER_ACCESS           , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_VOUT_UNKNOWN"          , STVMIX_ERROR_VOUT_UNKNOWN           , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_VOUT_ALREADY_CONNECTED", STVMIX_ERROR_VOUT_ALREADY_CONNECTED , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_NO_VSYNC"              , STVMIX_ERROR_NO_VSYNC               , TRUE);

#ifdef GAMMA_CONSTANTS
    RetErr |= STTST_AssignInteger ("G_CURSOR_BA"                    , G_CURSOR_BASE_ADDRESS               , TRUE);
    RetErr |= STTST_AssignInteger ("G_GDP1_BA"                      , G_GDP1_BASE_ADDRESS                 , TRUE);
    RetErr |= STTST_AssignInteger ("G_GDP2_BA"                      , G_GDP2_BASE_ADDRESS                 , TRUE);
#if ! (defined (ST_7710)||defined (ST_7100))
    RetErr |= STTST_AssignInteger ("G_GDP3_BA"                      , G_GDP3_BASE_ADDRESS                 , TRUE);


#endif

#if  defined(ST_7200)
RetErr |= STTST_AssignInteger ("G_GDP4_BA"                      , G_GDP4_BASE_ADDRESS                 , TRUE);
RetErr |= STTST_AssignInteger ("G_GDP5_BA"                      , G_GDP5_BASE_ADDRESS                 , TRUE);
#endif

#if (defined (ST_5528) && !defined(ST_7020))
    RetErr |= STTST_AssignInteger ("G_GDP4_BA"                      , G_GDP4_BASE_ADDRESS                 , TRUE);
#endif



#if !(defined (ST_7200))
    RetErr |= STTST_AssignInteger ("G_ALPHA_BA"                     , G_ALPHA_BASE_ADDRESS                , TRUE);
#endif
    RetErr |= STTST_AssignInteger ("G_MIX1_BA"                      , G_MIXER1_BASE_ADDRESS               , TRUE);
    RetErr |= STTST_AssignInteger ("G_MIX2_BA"                      , G_MIXER2_BASE_ADDRESS               , TRUE);
#if defined (ST_7200)
    RetErr |= STTST_AssignInteger ("G_MIX3_BA"                      , G_MIXER3_BASE_ADDRESS               , TRUE);
#endif
    RetErr |= STTST_AssignString  ("G_VIDEO1"                       , G_VIDEO1                            , TRUE);
    RetErr |= STTST_AssignString  ("G_VIDEO2"                       , G_VIDEO2                            , TRUE);

#if defined (ST_7200)
    RetErr |= STTST_AssignString  ("G_VIDEO3"                       , G_VIDEO3                            , TRUE);
#endif

#if defined (ST_5528) && defined(ST_7020)
    RetErr |= STTST_AssignInteger ("G_5528_CURSOR_BA"               , ST5528_CURSOR_LAYER_BASE_ADDRESS    , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_GDP1_BA"                 , ST5528_GDP1_LAYER_BASE_ADDRESS      , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_GDP2_BA"                 , ST5528_GDP2_LAYER_BASE_ADDRESS      , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_GDP3_BA"                 , ST5528_GDP3_LAYER_BASE_ADDRESS      , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_GDP4_BA"                 , ST5528_GDP4_LAYER_BASE_ADDRESS      , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_ALPHA_BA"                , ST5528_ALP_LAYER_BASE_ADDRESS       , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_MIX1_BA"                 , ST5528_VMIX1_BASE_ADDRESS           , TRUE);
    RetErr |= STTST_AssignInteger ("G_5528_MIX2_BA"                 , ST5528_VMIX2_BASE_ADDRESS           , TRUE);
    RetErr |= STTST_AssignString  ("G_5528_VIDEO1"                  , "SDDISPO2_VIDEO1"                   , TRUE);
    RetErr |= STTST_AssignString  ("G_5528_VIDEO2"                  , "SDDISPO2_VIDEO2"                   , TRUE);
#endif /* defined (ST_5528) && defined(ST_7020) */
#endif /* #ifdef GAMMA_CONSTANTS */

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107)\
|| defined (ST_5162)
    RetErr |= STTST_AssignInteger ("G_MIX1_BA"                      , MIXER_GDMA1_BASE_ADDRESS               , TRUE);
#endif

#ifdef STVMIX_USE_GLOBAL_DEVICE_TYPE
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined(ST_5525) || defined (ST_5107)\
|| defined (ST_5162)
    RetErr |= STTST_AssignInteger ("G_DeviceType"                   , 0               , FALSE);
#endif /* defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5188) || defined (ST_5107) || defined (ST_5162)*/
#endif /* STVMIX_USE_GLOBAL_DEVICE_TYPE*/

#if defined (ST_5100) || defined (ST_5301)  || defined (ST_5525)
    RetErr |= STTST_AssignInteger ("G_MIX2_BA"                      , MIXER_GDMA2_BASE_ADDRESS               , TRUE);
#endif

#ifdef ST_OSLINUX
  RetErr |= STTST_AssignString ("DRV_PATH_VMIX", "vmix/", TRUE);
#else
  RetErr |= STTST_AssignString ("DRV_PATH_VMIX", "", TRUE);
#endif

#if defined (ST_5105) || defined (ST_5107) || defined (ST_5162)
#ifdef SELECT_DEVICE_STB5118
  RetErr |= STTST_AssignInteger ("DEVICE_SELECTED_5118" ,1 ,TRUE);
#else
  RetErr |= STTST_AssignInteger ("DEVICE_SELECTED_5118" ,0 ,TRUE);
#endif
#endif

#if defined (ST_5100)
  RetErr |= STTST_AssignInteger ("DEVICE_SELECTED_5118" ,0 ,TRUE);
#endif


     return(!RetErr);
} /* end VMIX_InitCommand */


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL VMIX_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VMIX_InitCommand();
    if ( RetOk )
    {
        sprintf(VMIX_Msg, "VMIX_RegisterCmd() \t: ok           %s\n", STVMIX_GetRevision());
    }
    else
    {
        sprintf(VMIX_Msg, "VMIX_RegisterCmd() \t: failed !\n");
    }
    STTST_Print((VMIX_Msg));

    return( RetOk );

} /* end VMIX_CmdStart */


/*############################### END OF FILE ###########################*/



