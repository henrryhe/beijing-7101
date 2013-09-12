/************************************************************************
File name   : hdmi_cmd.c
Description : HDMI macros
Note        : All functions return TRUE if error for CLILIB compatibility

COPYRIGHT (C) STMicroelectronics 2005.

Date          Modification                                    Initials
----          ------------                                    --------
28 Feb 2005   Created                                         AC
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"
#include "stdevice.h"

#ifdef ST_OSLINUX
	#define STTBX_Print(x) printf x
#else
    #include "sttbx.h"
#endif

/*#include "staudlt.h"*/
#ifdef ST_OSLINUX
#include "staudlx.h"
#else
#ifdef ST_7710
#include "staud.h"
#else
#include "staudlx.h"
#endif
#endif

#include "stsys.h"
#include "testtool.h"
#include "stvout.h"
#include "stvid.h"
#include "stvtg.h"
#include "stvmix.h"
#include "sthdmi.h"
#include "clevt.h"

#ifdef HDMI_DEBUG
#include "../../src/hdmi_drv.h"
#endif

#include "hdmi_cmd.h"
#include "startup.h"
#include "api_cmd.h"
#include "vtg_cmd.h"
#include "vout_cmd.h"
#include "vid_cmd.h"
#include "vmix_cmd.h"
#include "aud_cmd.h"
#include "testcfg.h"
/* Private Types ------------------------------------------------------------ */
typedef U8 halvout_DencVersion_t;

typedef struct HDMITable_s
{
    char Name[15];
    U32  Val;
} HdmiTable_t;

typedef struct HDMISampleSize_s
{
    const char Size[18];
    const STAUD_DACDataPrecision_t Value;

}HDMISampleSize_t;

typedef struct HDMIDeviceInfo_s
{
   const char DeviceInfo[18];
   const STHDMI_SPDDeviceInfo_t  Value;
}HDMIDeviceInfo_t;


/* Private Constants -------------------------------------------------------- */

/* Patch for chip with no video like ST40GX1 */

#define HDMI_MAX_SUPPORTED     3
#define STRING_DEVICE_LENGTH   80
/*#define RETURN_PARAMS_LENGTH   50*/

#if defined (ST_7710)
#define HDMI_DEVICE_TYPE          STHDMI_DEVICE_TYPE_7710
#define HDMI_DEVICE_INFORMATION   "HDMI_ON_CHIP"
#define HDMI_VENDOR_NAME          "ST"
#define HDMI_PRODUCT_DESCRIPTION  "STMicro"
#define HDMI_VS_PAYLOAD           "ST"

#elif defined (ST_7100)||defined (ST_7109)
#define HDMI_DEVICE_TYPE          STHDMI_DEVICE_TYPE_7100
#define HDMI_DEVICE_INFORMATION   "HDMI_ON_CHIP"
#define HDMI_VENDOR_NAME          "ST"
#define HDMI_PRODUCT_DESCRIPTION  "STMicro"
#define HDMI_VS_PAYLOAD           "ST"
#elif defined (ST_7200)
#define HDMI_DEVICE_TYPE          STHDMI_DEVICE_TYPE_7200
#define HDMI_DEVICE_INFORMATION   "HDMI_ON_CHIP"
#define HDMI_VENDOR_NAME          "ST"
#define HDMI_PRODUCT_DESCRIPTION  "STMicro"
#define HDMI_VS_PAYLOAD           "ST"
#endif

/* Private variables ------------------------------------------------------ */
static char                     HDMI_Msg[255];  /* text for trace */
static STHDMI_Handle_t          HDMIHndl;

#define HDMI_TYPE_NAME_SIZE HDMI_MAX_SUPPORTED+2
#define HDMI_MAX_SAMPLE_SIZE   5
#define HDMI_MAX_DEV_INFO_SIZE 10

static HdmiTable_t HdmiOutTypeName[HDMI_TYPE_NAME_SIZE]={
    { "<", STVOUT_OUTPUT_DIGITAL_HDMI_RGB888-1 },
    { "HDMI_RGB888", STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 },
    { "HDMI_YcbCr444", STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 },
    { "HDMI_YCbCr422" , STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 },
    { ">", STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422+1}
};

static HDMIDeviceInfo_t  HDMIDeviceInfo[HDMI_MAX_DEV_INFO_SIZE]={
    {"DEV_UNKNOWN",STHDMI_SPD_DEVICE_UNKNOWN },
    {"DEV_DIGSTB",STHDMI_SPD_DEVICE_DIGITAL_STB},
    {"DEV_DVD" ,STHDMI_SPD_DEVICE_DVD},
    {"DEV_D_VHS", STHDMI_SPD_DEVICE_D_VHS},
    {"DEV_HDD", STHDMI_SPD_DEVICE_HDD_VIDEO},
    {"DEV_DVC", STHDMI_SPD_DEVICE_DVC},
    {"DEV_DSC", STHDMI_SPD_DEVICE_DSC},
    {"DEV_CD", STHDMI_SPD_DEVICE_VIDEO_CD},
    {"DEV_GAME", STHDMI_SPD_DEVICE_GAME},
    {"DEV_PC", STHDMI_SPD_DEVICE_PC_GENERAL}
};
static HDMISampleSize_t   HDMISampleSize[HDMI_MAX_SAMPLE_SIZE]={
    {"16BITS", STAUD_DAC_DATA_PRECISION_16BITS},
    {"18BITS", STAUD_DAC_DATA_PRECISION_18BITS},
    {"20BITS", STAUD_DAC_DATA_PRECISION_20BITS},
    {"24BITS",STAUD_DAC_DATA_PRECISION_24BITS},
    {"NONE", 0xFF}
};
/* Global Variables ------------------------------------------------------- */

#ifdef HDMI_DEBUG
extern sthdmi_Unit_t *sthdmi_DbgPtr;
extern sthdmi_Device_t *sthdmi_DbgDev;
#endif

BOOL HdmiTest_PrintDotsIfOk;

/* Private Macros --------------------------------------------------------- */
#define HDMI_PRINT(x) { \
    /* Check lenght */ \
    if (strlen(x)>sizeof(HDMI_Msg)) \
    { \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* Private Function prototypes ---------------------------------------------- */

static BOOL HDMI_GetRevision(        STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_GetCapability(      STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_GetSourceCapability(STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_GetSinkCapability(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_Init(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_Open(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_Close(              STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_Term(               STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL HDMI_FillSinkEDID (      STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL HDMI_FreeMemory ( STTST_Parse_t *pars_p, char *result_sym_p);
static void Print_Hdmi_Capabilitity( STHDMI_Capability_t Cp);
static void Print_SrcHdmi_Capability(STHDMI_SourceCapability_t CpSrc);
static void Print_SnkHdmi_Capability(STHDMI_SinkCapability_t CpSnk);

/* Functions ---------------------------------------------------------------- */
static int shift( int n)   { int p;  for ( p=0; n!=0; p++, n>>=1); return(p); }


/*-----------------------------------------------------------------------------
 * Function : HDMI_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL HDMI_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if (API_TextError(Error, Text) == FALSE)
    {
       /* RetErr = TRUE; */
        switch ( Error )
        {
            case STHDMI_ERROR_NOT_AVAILABLE :
                 strcat( Text, "(feature not available) !\n");
                 break;
            case STHDMI_ERROR_INVALID_PACKET_FORMAT :
                 strcat( Text, "(sthdmi error invalid packet format) !\n");
                 break;
            case STHDMI_ERROR_INFOMATION_NOT_AVAILABLE :
                 strcat(Text, "(sthdmi error information not available) !\n");
                 break;
            case STHDMI_ERROR_VIDEO_UNKOWN :
                 strcat( Text, "(video unkown by hdmi)!\n");
                 break;
            case STHDMI_ERROR_VTG_UNKOWN :
                 strcat( Text, "(vtg unkown by hdmi) !\n");
                 break;
            case STHDMI_ERROR_VMIX_UNKOWN :
                 strcat( Text, "(mixer unkown by hdmi) !\n");
                 break;
            case STHDMI_ERROR_AUDIO_UNKOWN :
                 strcat( Text, "(audio unkown by hdmi) !\n");
                 break;
            case STHDMI_ERROR_VOUT_UNKOWN :
                strcat( Text, "(vout unkown by hdmi) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }
    HDMI_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
} /* end of HDMI_TextError */

/*-----------------------------------------------------------------------------
 * Function : HDMI_DoTextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL HDMI_DoTextError(ST_ErrorCode_t ErrorCode, char *Text)
{
    BOOL RetErr = FALSE;

    if ((ErrorCode == ST_NO_ERROR) && HdmiTest_PrintDotsIfOk)
    {
        STTBX_Print(("."));
    }
    else
    {
        RetErr = HDMI_TextError( ErrorCode, Text);
    }
    return( API_EnableError ? RetErr : FALSE);

} /* end of HDMI_DoTextError */

static void Print_Hdmi_Capabilitity( STHDMI_Capability_t Cp)
{
    char MsgEDC[80] = ">> Capability : EDIDDataCapable : ";
    char MsgDIC[80] = ">> Capability : DataIslandCapable : ";
    char MsgVOC[80] = ">> Capability : VideoOuputCapable : ";
    char MsgAOC[80] = ">> Capability : AudioOuputCapable : ";
    char MsgOut[80] = ">> HDMI Outputs :   ";
    /*char MsgSele[80] =">> Selected :";*/

    if (Cp.IsEDIDDataSupported)   strcat(MsgEDC, "YES\n");
    else                          strcat(MsgEDC,  "NO\n");

    if (Cp.IsDataIslandSupported) strcat(MsgDIC, "YES\n");
    else                          strcat(MsgDIC,  "NO\n");

    if (Cp.IsVideoSupported)      strcat(MsgVOC, "YES\n");
    else                          strcat(MsgVOC,  "NO\n");

    if (Cp.IsAudioSupported)      strcat(MsgAOC, "YES\n");
    else                          strcat(MsgAOC,  "NO\n");

    STTBX_Print(( ">> Capability : SupportedOutputs : \n"));
    if ( Cp.SupportedOutputs == 0x0)
    {
        STTBX_Print(( "0 \n"));
    }
    else
    {
        if ( (Cp.SupportedOutputs & 0x800) == 0x800)    strcat( MsgOut, "HDMI_RGB888\t");
        if ( (Cp.SupportedOutputs & 0x1000) == 0x1000)  strcat( MsgOut, "HDMI_YCBCR444\t");
        if ( (Cp.SupportedOutputs & 0x2000) == 0x2000)  strcat( MsgOut, "HDMI_YCBCR422");
        strcat( MsgOut, "\n");
        STTBX_Print(( MsgOut));
    }

    STTBX_Print(( MsgEDC));
    STTBX_Print(( MsgDIC));
    STTBX_Print(( MsgVOC));
    STTBX_Print(( MsgAOC));
}

static void Print_SrcHdmi_Capability( STHDMI_SourceCapability_t CpSrc)
{
  char MsgAvi[80]=">> AVI: ";
  char MsgSpd[80]=">> SPD: ";
  char MsgMs[80]= ">> MS: ";
  char MsgAud[80]=">> AUDIO: ";
  char MsgVs[80]= ">> VS: ";
  char MsgASel[80] = ">> Capability : AVI Selected : ";
  char MsgSSel[80] = ">> Capability : SPD Selected : ";
  char MsgMSel[80] = ">> Capability : MS  Selected : ";
  char MsgAuSel[80] = ">> Capability: Audio Selected : ";
  char MsgVSel[80] = ">> Capability : VS  Selected : ";


  STTBX_Print(( ">> Capability : SupportedAVIInfoFrames : \n"));
  if ( CpSrc.AVIInfoFrameSupported == 0x0)
  {
      STTBX_Print(( "0 \n"));
  }
  else
  {
    if ( (CpSrc.AVIInfoFrameSupported & 0x1) == 0x1)  strcat( MsgAvi, "AVI_NONE\t");
    if ( (CpSrc.AVIInfoFrameSupported & 0x2) == 0x2)  strcat( MsgAvi, "AVI_VER1\t");
    if ( (CpSrc.AVIInfoFrameSupported & 0x4) == 0x4)  strcat( MsgAvi, "AVI_VER2");
    strcat( MsgAvi, "\n");
    STTBX_Print(( MsgAvi));
  }

  STTBX_Print(( ">> Capability : SupportedSPDInfoFrames : \n"));
  if ( CpSrc.SPDInfoFrameSupported == 0x0)
  {
      STTBX_Print(( "0 \n"));
  }
  else
  {
    if ( (CpSrc.SPDInfoFrameSupported & 0x1) == 0x1)  strcat( MsgSpd, "SPD_NONE\t");
    if ( (CpSrc.SPDInfoFrameSupported & 0x2) == 0x2)  strcat( MsgSpd, "SPD_VER1");
    strcat( MsgSpd, "\n");
    STTBX_Print(( MsgSpd));
   }

  STTBX_Print(( ">> Capability : SupportedMSInfoFrames : \n"));
  if ( CpSrc.MSInfoFrameSupported == 0x0)
  {
      STTBX_Print(( "0 \n"));
  }
  else
  {
    if ( (CpSrc.MSInfoFrameSupported & 0x1) == 0x1)  strcat( MsgMs, "MS_NONE\t");
    if ( (CpSrc.MSInfoFrameSupported & 0x2) == 0x2)  strcat( MsgMs, "MS_VER1");
    strcat( MsgMs, "\n");
    STTBX_Print(( MsgSpd));
  }

  STTBX_Print(( ">> Capability : SupportedAudioInfoFrames : \n"));
  if ( CpSrc.AudioInfoFrameSupported == 0x0)
  {
      STTBX_Print(( "0 \n"));
  }
  else
  {
    if ( (CpSrc.AudioInfoFrameSupported & 0x1) == 0x1)  strcat( MsgAud, "AUD_NONE\t");
    if ( (CpSrc.AudioInfoFrameSupported & 0x2) == 0x2)  strcat( MsgAud, "AUD_VER1");
    strcat(MsgAud, "\n");
    STTBX_Print(( MsgAud));
  }

  STTBX_Print(( ">> Capability : SupportedVSInfoFrames : \n"));
  if ( CpSrc.VSInfoFrameSupported == 0x0)
  {
      STTBX_Print(( "0 \n"));
  }
  else
  {
    if ( (CpSrc.VSInfoFrameSupported & 0x1) == 0x1)  strcat( MsgVs, "VS_NONE\t");
    if ( (CpSrc.VSInfoFrameSupported & 0x2) == 0x2)  strcat( MsgVs, "VS_VER1");
    strcat(MsgVs, "\n");
    STTBX_Print(( MsgVs));
  }
  switch (CpSrc.AVIInfoFrameSelected)
  {
    case STHDMI_AVI_INFOFRAME_FORMAT_NONE :
         strcat(MsgASel, "AVI_INFOFRAME_FORMAT_NONE\n");
        break;

    case STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE :
        strcat(MsgASel, "AVI_INFOFRAME_FORMAT_VER_ONE\n");
        break;

    case STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO :
        strcat(MsgASel, "AVI_INFOFRAME_FORMAT_VER_TWO\n");
        break;
    default :
        strcat( MsgASel, "0 \n");
        break;
  }
  STTBX_Print(( MsgASel));

  switch (CpSrc.SPDInfoFrameSelected)
  {
    case STHDMI_SPD_INFOFRAME_FORMAT_NONE :
         strcat(MsgSSel, "SPD_INFOFRAME_FORMAT_NONE\n");
        break;

    case STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE :
        strcat(MsgSSel, "SPD_INFOFRAME_FORMAT_VER_ONE\n");
        break;
    default :
        strcat( MsgSSel, "0 \n");
        break;
  }
  STTBX_Print(( MsgSSel));

  switch (CpSrc.MSInfoFrameSelected)
  {
    case STHDMI_MS_INFOFRAME_FORMAT_NONE :
         strcat(MsgMSel, "MS_INFOFRAME_FORMAT_NONE\n");
        break;

    case STHDMI_MS_INFOFRAME_FORMAT_VER_ONE :
        strcat(MsgMSel, "MS_INFOFRAME_FORMAT_VER_ONE\n");
        break;
    default :
        strcat(MsgMSel, "0 \n");
        break;
  }
  STTBX_Print(( MsgMSel));

  switch (CpSrc.AudioInfoFrameSelected)
  {
    case STHDMI_AUDIO_INFOFRAME_FORMAT_NONE :
         strcat(MsgAuSel, "AUDIO_INFOFRAME_FORMAT_NONE\n");
        break;

    case STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE :
        strcat(MsgAuSel, "AUDIO_INFOFRAME_FORMAT_VER_ONE\n");
        break;
    default :
        strcat(MsgAuSel, "0 \n");
        break;
  }
  STTBX_Print(( MsgAuSel));

  switch (CpSrc.VSInfoFrameSelected)
  {
    case STHDMI_VS_INFOFRAME_FORMAT_NONE :
         strcat(MsgVSel, "VS_INFOFRAME_FORMAT_NONE\n");
        break;

    case STHDMI_VS_INFOFRAME_FORMAT_VER_ONE :
        strcat(MsgVSel, "VS_INFOFRAME_FORMAT_VER_ONE\n");
        break;
    default :
        strcat(MsgVSel, "0 \n");
        break;
  }
  STTBX_Print(( MsgVSel));


}
static void Print_SnkHdmi_Capability( STHDMI_SinkCapability_t CpSnk)
{
 char MsgBEC[80] = ">> Capability : BaicEdidCapable : ";
 char MsgADC[80] = ">> Capability : AdditionalDataCapable : ";
 char MsgLTC[80] = ">> Capability : LCDTimingCapable : ";
 char MsgCIC[80] = ">> Capability : ColorInfomationCapable : ";
 char MsgDDC[80] = ">> Capability : DviDataCapable : ";
 char MsgTSC[80] = ">> Capability : TouchScreenDataCapable : ";
 char MsgBMC[80] = ">> Capability : BlockMapCapable : ";


 if (CpSnk.IsBasicEDIDSupported)        strcat(MsgBEC, "YES\n");
 else                                   strcat(MsgBEC,  "NO\n");

 if (CpSnk.IsAdditionalDataSupported)   strcat(MsgADC, "YES\n");
 else                                   strcat(MsgADC,  "NO\n");

 if (CpSnk.IsLCDTimingDataSupported)    strcat(MsgLTC, "YES\n");
 else                                   strcat(MsgLTC,  "NO\n");

 if (CpSnk.IsColorInfoSupported)        strcat(MsgCIC, "YES\n");
 else                                   strcat(MsgCIC,  "NO\n");

 if (CpSnk.IsDviDataSupported)          strcat(MsgDDC, "YES\n");
 else                                   strcat(MsgDDC,  "NO\n");

 if (CpSnk.IsTouchScreenDataSupported) strcat(MsgTSC, "YES\n");
 else                                  strcat(MsgTSC,  "NO\n");

 if (CpSnk.IsBlockMapSupported)        strcat(MsgBMC, "YES\n");
 else                                  strcat(MsgBMC,  "NO\n");


 STTBX_Print(( ">>\tEDID Basic Version %i Revision %i\n", CpSnk.EDIDBasicVersion, CpSnk.EDIDBasicRevision));
 STTBX_Print(( ">>\tEDID Extension version %i\n", CpSnk.EDIDExtRevision));
 STTBX_Print(( MsgBEC));
 STTBX_Print(( MsgADC));
 STTBX_Print(( MsgLTC));
 STTBX_Print(( MsgCIC));
 STTBX_Print(( MsgDDC));
 STTBX_Print(( MsgTSC));
 STTBX_Print(( MsgBMC));
}
/*#######################################################################*/
/*############################# HDMI COMMANDS ###########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : HDMI_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL HDMI_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    /*Unused parameters*/
    UNUSED_PARAMETER (pars_p);
    UNUSED_PARAMETER (result_sym_p);

    RevisionNumber = STHDMI_GetRevision( );
    sprintf( HDMI_Msg, "STHDMI_GetRevision() : revision=%s\n", RevisionNumber );
    STTBX_Print(( HDMI_Msg ));
    return ( FALSE );
} /* end HDMI_GetRevision */

/*-------------------------------------------------------------------------
 * Function : HDMI_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    STHDMI_Capability_t Capability;
    char DeviceName[80];

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }
    sprintf( HDMI_Msg, "STHDMI_GetCapability (%s) :", DeviceName );
    ErrorCode = STHDMI_GetCapability(DeviceName, &Capability);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        Print_Hdmi_Capabilitity( Capability);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_GetCapability */

/*-------------------------------------------------------------------------
 * Function : HDMI_GetSinkCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_GetSinkCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    STHDMI_SinkCapability_t SinkCapability;
    char DeviceName[80];

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }
    sprintf( HDMI_Msg, "STHDMI_GetSinkCapability (%s) :", DeviceName );
    ErrorCode = STHDMI_GetSinkCapability(DeviceName, &SinkCapability);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        Print_SnkHdmi_Capability( SinkCapability);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_GetSinkCapability */

/*-------------------------------------------------------------------------
 * Function : HDMI_GetSourceCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_GetSourceCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    STHDMI_SourceCapability_t SourceCapability;
    char DeviceName[80];

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }
    sprintf( HDMI_Msg, "STHDMI_GetSourceCapability (%s) :", DeviceName );
    ErrorCode = STHDMI_GetSourceCapability(DeviceName, &SourceCapability);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        Print_SrcHdmi_Capability( SourceCapability);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_GetSourceCapability */

/*-------------------------------------------------------------------------
 * Function : HDMI_FillAVIInfoFrame
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillAVIInfoFrame ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                   RetErr;
    ST_ErrorCode_t         ErrorCode;
    STHDMI_AVIInfoFrame_t  AVIInfoFrame;
    U32 LVar;
    char TrvStr[80], IsStr[80], *ptr;
    U32 Var; int i;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    /* Get Device Type */
    RetErr = STTST_GetInteger( pars_p, (HDMI_DEVICE_TYPE + 1), (S32*)&LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 1=7710 2=7100 3=7200\n");
        return(TRUE);
    }
    switch (LVar)
    {
        case 1 : /* no break */
        case 2 : /* no break */
        case 3 :
            /* Get Colorimetry  */
            RetErr = STTST_GetInteger( pars_p, STVOUT_ITU_R_601, (S32*)&LVar);
            if (RetErr)
            {
            STTST_TagCurrentLine( pars_p, "Expected Color Space\n");
            return(TRUE);
            }
            AVIInfoFrame.AVI861B.Colorimetry= (STVOUT_ColorSpace_t)LVar;
            /* Get Aspect Ratio */
            RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_16TO9, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Aspect Ratio\n");
                return(TRUE);
            }
            AVIInfoFrame.AVI861B.AspectRatio =(STGXOBJ_AspectRatio_t)LVar;

            /* Get Active Aspect Ratio */
            RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_16TO9, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Active Aspect Ratio\n");
                return(TRUE);
            }
            AVIInfoFrame.AVI861B.ActiveAspectRatio =(STGXOBJ_AspectRatio_t)LVar;

            /* Get output type (default: Digital HDMI RGB) */
           STTST_GetItem(pars_p, "HDMI_RGB888", TrvStr, sizeof(TrvStr));
           RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
           if (!RetErr)
           {
               strcpy(TrvStr, IsStr);
           }
            /* values to test outside output type */
           if ( (strcmp(TrvStr, "<")==0) || (strcmp(TrvStr, ">")==0) )
           {
                if ( (strcmp(TrvStr, "<")==0) )
                {
                    Var = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888-1;
                }
                if ( (strcmp(TrvStr, ">")==0) )
                {
                    Var = STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422+1;
                }
                AVIInfoFrame.AVI861B.OutputType = (STVOUT_OutputType_t)(Var);
            }
            else
            {
                for (Var=1; Var<HDMI_MAX_SUPPORTED+1; Var++)
                {
                    if (   (strcmp(HdmiOutTypeName[Var].Name, TrvStr)==0)
                        || (strncmp(HdmiOutTypeName[Var].Name, TrvStr+1, strlen(HdmiOutTypeName[Var].Name))==0))
                    {
                        break;
                    }
                }
                if (Var==HDMI_MAX_SUPPORTED+1)
                {
                    Var = (U32)strtoul(TrvStr, &ptr, 10);
                    if (TrvStr==ptr)
                    {
                        STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)\n");
                        return(TRUE);
                    }

                    for (i=1; i<HDMI_MAX_SUPPORTED+1; i++)
                    {
                        if (HdmiOutTypeName[i].Val == Var)
                        {
                            Var=i;
                            break;
                        }
                    }
                }
                AVIInfoFrame.AVI861B.OutputType = (STVOUT_OutputType_t)(1<<(Var+10));
            }

             break;
            default :
             break;
           }

    ErrorCode = STHDMI_FillAVIInfoFrame(HDMIHndl, &AVIInfoFrame);
    sprintf( HDMI_Msg, "STHDMI_FillAVIInfoFrame(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillAVIInfoFrame */

/*-------------------------------------------------------------------------
 * Function : HDMI_FillSPDInfoFrame
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillSPDInfoFrame ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                   RetErr;
    ST_ErrorCode_t         ErrorCode;
    STHDMI_SPDInfoFrame_t  SPDInfoFrame;
    char TrvStr[80], IsStr[80], *ptr;

    U32 Var1;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    /* Get Vendor Name */
    RetErr = STTST_GetString(pars_p, HDMI_VENDOR_NAME, SPDInfoFrame.VendorName, \
            sizeof(SPDInfoFrame.VendorName));
    if (RetErr)
    {
         STTST_TagCurrentLine( pars_p, "Expected Vendor Name\n");
         return(TRUE);
    }

    /* Get Product description */
    RetErr = STTST_GetString(pars_p, HDMI_PRODUCT_DESCRIPTION, SPDInfoFrame.ProductDescription, \
             sizeof(SPDInfoFrame.ProductDescription));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected product description\n");
        return(TRUE);
    }
    /* Get Device Information */
    STTST_GetItem( pars_p, "DEV_DIGSTB", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
       strcpy(TrvStr, IsStr);
    }
    for(Var1=0; Var1< HDMI_MAX_DEV_INFO_SIZE; Var1++){
    if((strcmp(HDMIDeviceInfo[Var1].DeviceInfo, TrvStr)==0)
      ||(strncmp(HDMIDeviceInfo[Var1].DeviceInfo, TrvStr+1, strlen(HDMIDeviceInfo[Var1].DeviceInfo))==0))
      break;
     }
     if(Var1== HDMI_MAX_DEV_INFO_SIZE){
       RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var1, 10);
       if (RetErr)
      {
           Var1 = (U32)strtoul(TrvStr, &ptr, 10);
           if(TrvStr==ptr)
          {
              STTST_TagCurrentLine( pars_p, "Expected Device Information (default Digital STB)\n");
              return(TRUE);
           }
       }
     }
    SPDInfoFrame.SourceDeviceInfo= (STHDMI_SPDDeviceInfo_t)Var1;

    ErrorCode = STHDMI_FillSPDInfoFrame(HDMIHndl, &SPDInfoFrame);
    sprintf( HDMI_Msg, "STHDMI_FillSPDInfoFrame(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillSPDInfoFrame */

/*-------------------------------------------------------------------------
 * Function : HDMI_FillAudioInfoFrame
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillAudioInfoFrame ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                     RetErr;
    ST_ErrorCode_t           ErrorCode;
    STHDMI_AUDIOInfoFrame_t  AudioInfoFrame;
    char TrvStr[80], IsStr[80], *ptr;
    U32 LVar, Var2;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get Sample Size (default: 16 Bits) */
    STTST_GetItem( pars_p, "NONE", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
       strcpy(TrvStr, IsStr);
    }
    for(Var2=0; Var2<HDMI_MAX_SAMPLE_SIZE; Var2++){
    if((strcmp(HDMISampleSize[Var2].Size, TrvStr)==0)
      ||(strncmp(HDMISampleSize[Var2].Size, TrvStr+1, strlen(HDMISampleSize[Var2].Size))==0))
      break;
     }
     if(Var2==HDMI_MAX_SAMPLE_SIZE){
       RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var2, 10);
       if (RetErr)
      {
           Var2 = (U32)strtoul(TrvStr, &ptr, 10);
           if(TrvStr==ptr)
          {
              STTST_TagCurrentLine( pars_p, "Expected Sample Size (default 16BITS)");
              return(TRUE);
           }
       }
     }
    /*AudioInfoFrame.SampleSize=(U32) HDMISampleSize[Var2].Value;*/
    AudioInfoFrame.SampleSize=0xFF;
    /* Get Coding type */
    RetErr = STTST_GetInteger( pars_p, STAUD_STREAM_CONTENT_NULL, (S32*)&LVar);
    if ( RetErr )
    {
          STTST_TagCurrentLine( pars_p, "Expected the Coding Type(default content NULL) \n" );
          return(TRUE);
    }
    AudioInfoFrame.CodingType =(U32)LVar;

    /* Get Channel Allocation */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
          STTST_TagCurrentLine( pars_p, "Expected the Channel Allocation \n" );
          return(TRUE);
    }
    AudioInfoFrame.ChannelAlloc =(U32)LVar;


    /* Get Level Shift for downmixing */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr )
    {
          STTST_TagCurrentLine( pars_p, "Expected level shift for downmixing \n" );
          return(TRUE);
    }
    AudioInfoFrame.LevelShift =(U32)LVar;


    /*  Get Downmix inhibit value */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DownMix inhibit (default FALSE)\n");
        return(TRUE);
    }
    AudioInfoFrame.DownmixInhibit =(BOOL)LVar;

    ErrorCode = STHDMI_FillAudioInfoFrame(HDMIHndl, &AudioInfoFrame);
    sprintf( HDMI_Msg, "STHDMI_FillAudioInfoFrame(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillAudioInfoFrame */

 /*-------------------------------------------------------------------------
 * Function : HDMI_FillMSInfoFrame
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillMSInfoFrame ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                          RetErr;
    ST_ErrorCode_t                ErrorCode;
    STHDMI_MPEGSourceInfoFrame_t  MSInfoFrame;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    ErrorCode = STHDMI_FillMSInfoFrame(HDMIHndl, &MSInfoFrame);
    sprintf( HDMI_Msg, "STHDMI_FillMSInfoFrame(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillMSInfoFrame */

/*-------------------------------------------------------------------------
 * Function : HDMI_FillVSInfoFrame
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillVSInfoFrame ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                          RetErr;
    ST_ErrorCode_t                ErrorCode;
    STHDMI_VendorSpecInfoFrame_t  VSInfoFrame;
    U32 LVar;
    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    /* Get Frame Length */
    RetErr = STTST_GetInteger( pars_p, 8, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Frame Length");
        return(TRUE);
    }
    VSInfoFrame.FrameLength = (U32)LVar;
    /* Get Registration Identification */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Regsitration Identification");
        return(TRUE);
    }
    VSInfoFrame.RegistrationId = (U32)LVar;

    /* Get Vendor Specific Payload */
    RetErr = STTST_GetString(pars_p, HDMI_VS_PAYLOAD, VSInfoFrame.VSPayload_p, \
            sizeof(char)*(VSInfoFrame.FrameLength-6));
    if (RetErr)
    {
         STTST_TagCurrentLine( pars_p, "Expected Vendor Specific Payload\n");
         return(TRUE);
    }
    ErrorCode = STHDMI_FillVSInfoFrame(HDMIHndl, &VSInfoFrame);
    sprintf( HDMI_Msg, "STHDMI_FillVSInfoFrame(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillVSInfoFrame */

/*-------------------------------------------------------------------------
 * Function : HDMI_FillSinkEDID
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FillSinkEDID ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                          RetErr;
    ST_ErrorCode_t                ErrorCode;
    static STHDMI_EDIDSink_t      EDIDBuffer;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    ErrorCode = STHDMI_FillSinkEDID(HDMIHndl, &EDIDBuffer);
    sprintf( HDMI_Msg, "STHDMI_FillSinkEDID(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FillSinkEDID */

/*-------------------------------------------------------------------------
 * Function : HDMI_FreeMemory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
 static BOOL HDMI_FreeMemory ( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL                          RetErr;
    ST_ErrorCode_t                ErrorCode;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    ErrorCode = STHDMI_FreeEDIDMemory(HDMIHndl);
    sprintf( HDMI_Msg, "STHDMI_FreeEDIDMemory(%d): ", HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of HDMI_FreeEDIDMemory */
/*-------------------------------------------------------------------------
 * Function : HDMI_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * Testtool usage :
 * hdmi_init "devicename" devicetype=7710/710X/7200  outputtype=HDMI_RGB888/HDMI_YCbCr444/HDMI_YCbCr422   maxopen
 * ----------------------------------------------------------------------*/
static BOOL HDMI_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STHDMI_InitParams_t HDMIInitParams;
    ST_ErrorCode_t      ErrorCode1= ST_NO_ERROR;
    char DeviceName[STRING_DEVICE_LENGTH], TrvStr[80], IsStr[80],*ptr;
    U32 LVar, Var, i;
    BOOL RetErr;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName\n");
        return(TRUE);
    }

    /* Get device type  */
    RetErr = STTST_GetInteger( pars_p, (HDMI_DEVICE_TYPE + 1), (S32*)&LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 1=7710 2=7100 3=7200\n");
        return(TRUE);
    }

    if ( (LVar == 0) || (LVar > 3) )  /* values to test outside device type */
    {
        if ( LVar == 0)
        {
            HDMIInitParams.DeviceType = STHDMI_DEVICE_TYPE_7710-1;
        }
        if ( LVar > 3)
        {
            HDMIInitParams.DeviceType = STHDMI_DEVICE_TYPE_7200+1;
        }
    }
    else /* Device Type ok */
    {
        HDMIInitParams.DeviceType = (STHDMI_DeviceType_t)(LVar-1);
    }


    /* Get output type (default: Digital HDMI RGB) */
    STTST_GetItem(pars_p, "HDMI_RGB888", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));

    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    /* values to test outside output type */
    if ( (strcmp(TrvStr, "<")==0) || (strcmp(TrvStr, ">")==0) )
    {
        if ( (strcmp(TrvStr, "<")==0) )
        {
            Var = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888-1;
        }
        if ( (strcmp(TrvStr, ">")==0) )
        {
            Var = STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422+1;
        }
        HDMIInitParams.OutputType = (STVOUT_OutputType_t)(Var);
    }
    else
    {
        for (Var=1; Var<HDMI_MAX_SUPPORTED+1; Var++)
        {
            if (   (strcmp(HdmiOutTypeName[Var].Name, TrvStr)==0)
                || (strncmp(HdmiOutTypeName[Var].Name, TrvStr+1, strlen(HdmiOutTypeName[Var].Name))==0))
            {
                break;
            }
        }
        if (Var==HDMI_MAX_SUPPORTED+1)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)\n");
                return(TRUE);
            }

            for (i=1; i<HDMI_MAX_SUPPORTED+1; i++)
            {
                if (HdmiOutTypeName[i].Val == Var)
                {
                    Var=i;
                    break;
                }
            }
        }
        HDMIInitParams.OutputType = (STVOUT_OutputType_t)(1<<(Var+10));
    }
   /* Reserved for HDMI ip which support EIA/CEA861, EIA/CEA861A and EIA/CEA861B  */
   switch (HDMIInitParams.DeviceType)
   {
    case STHDMI_DEVICE_TYPE_7710 : /* no break */
    case STHDMI_DEVICE_TYPE_7100 : /* no break */
    case STHDMI_DEVICE_TYPE_7200 :

            /* Get the EDID type (default: none)*/
           /* RetErr = STTST_GetInteger( pars_p, 4, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected Edid type 1= None 2= EDID1 3=EDID2 4=EDID3 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 5) )*/  /* values to test outside Edid type */
            /*{
                if ( LVar == 0)
                {
                    HDMIInitParams.EdidType = STHDMI_EDIDTIMING_EXT_TYPE_NONE-1;
                }
                if ( LVar > 5)
                {
                    HDMIInitParams.EdidType = STHDMI_EDIDTIMING_EXT_TYPE_VER_THREE+1;
                }
            }
            else *//* EDID Type ok */
            /*{
                HDMIInitParams.EdidType = (STHDMI_EDIDTimingExtType_t)(LVar-1);
            }*/

             /* Get the AVI Type (default : none)*/
             RetErr = STTST_GetInteger( pars_p,3, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected AVI type 1= None 2= AVI1 3=AVI2 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 4) )  /* values to test outside AVI type */
            {
                if ( LVar == 0)
                {
                    HDMIInitParams.AVIType = STHDMI_AVI_INFOFRAME_FORMAT_NONE-1;
                }
                if ( LVar > 4)
                {
                    HDMIInitParams.AVIType = STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO+1;
                }
            }
            else /* AVI Type ok */
            {
                HDMIInitParams.AVIType = (STHDMI_AVIInfoFrameFormat_t)(1<<(LVar-1));
            }

             /* Get the SPD Type (dafault : none)*/

            RetErr = STTST_GetInteger( pars_p, 2, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected SPD type 1= None 2= SPD1 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 3) )  /* values to test outside SPD type */
            {
                if ( LVar == 0)
                {
                    HDMIInitParams.SPDType = STHDMI_SPD_INFOFRAME_FORMAT_NONE-1;
                }
                if ( LVar > 3)
                {
                    HDMIInitParams.SPDType = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE+1;
                }
            }
            else /* SPD Type ok */
            {
                HDMIInitParams.SPDType = (STHDMI_SPDInfoFrameFormat_t)(1<<(LVar-1));
            }

             /* Get MS Type (default : none)*/
            RetErr = STTST_GetInteger( pars_p, 2, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected MS type 1= None 2= MS1 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 3) )  /* values to test outside MS type */
            {
                if ( LVar == 0)
                {
                    HDMIInitParams.MSType = STHDMI_MS_INFOFRAME_FORMAT_NONE-1;
                }
                if ( LVar > 3)
                {
                    HDMIInitParams.MSType = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE+1;
                }
            }
            else /* MS Type ok */
            {
                HDMIInitParams.MSType = (STHDMI_MSInfoFrameFormat_t)(1<<(LVar-1));
            }

            /* Get AUDIO Type (default none)*/

            RetErr = STTST_GetInteger( pars_p, 2, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected AUDIO type 1= None 2= AUDIO1 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 3) )  /* values to test outside AUDIO type */
            {
                if ( LVar == 0)
                {
                    HDMIInitParams.AUDIOType = STHDMI_AUDIO_INFOFRAME_FORMAT_NONE-1;
                }
                if ( LVar > 3)
                {
                    HDMIInitParams.AUDIOType = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE+1;
                }
            }
            else /* AUDIO Type ok */
            {
                HDMIInitParams.AUDIOType = (STHDMI_AUDIOInfoFrameFormat_t)(1<<(LVar-1));
            }

            /* Get the VS Type (default: none)*/

            RetErr = STTST_GetInteger( pars_p, 2, (S32*)&LVar );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "Expected VS type 1= None 2= VS1 \n");
                return(TRUE);
            }

            if ( (LVar == 0) || (LVar > 3) )  /* values to test outside VS type */
            {
                if ( LVar == 0)
                {
                    HDMIInitParams.VSType = STHDMI_VS_INFOFRAME_FORMAT_NONE-1;
                }
                if ( LVar > 3)
                {
                    HDMIInitParams.VSType = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE+1;
                }
            }
            else /*  VS Type ok */
            {
                HDMIInitParams.VSType = (STHDMI_VSInfoFrameFormat_t)(1<<(LVar-1));
            }
        break;

    default :
        break;
   }

   /* Reserved for HDMI ip which support EIA/CEA861A and EIA/CEA861B */
   switch (HDMIInitParams.DeviceType)
   {
    case STHDMI_DEVICE_TYPE_7710 : /* no break */
    case STHDMI_DEVICE_TYPE_7100 : /* no break */
    case STHDMI_DEVICE_TYPE_7200 :
         /* Get vout device name */
         RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, HDMIInitParams.Target.OnChipHdmiCell.VOUTDeviceName, sizeof(ST_DeviceName_t));
         if (RetErr)
         {
            STTST_TagCurrentLine( pars_p, "Expected  VOUT DeviceName\n");
            return(TRUE);
         }
         break;
    default :
        break;
   }
   /* get max open (default: 1 = STHDMI_MAX_OPEN) */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected max open init parameter\n" );
        return(TRUE);
    }

    HDMIInitParams.MaxOpen         = (U32)LVar;

    /* Get Driver partition */
    HDMIInitParams.CPUPartition_p  = DriverPartition_p;

    /* Reserved For HDMI ip which support EIA/CEA861B only... */
    switch (HDMIInitParams.DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             /* Get vtg device name  */
            RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, HDMIInitParams.Target.OnChipHdmiCell.VTGDeviceName, sizeof(ST_DeviceName_t));
            if (RetErr)
            {
               STTST_TagCurrentLine( pars_p, "Expected  VTG DeviceName\n");
               return(TRUE);
            }
            /* Get Evt device name */
            RetErr = STTST_GetString( pars_p, STEVT_DEVICE_NAME, HDMIInitParams.Target.OnChipHdmiCell.EvtDeviceName, sizeof(ST_DeviceName_t));
            if (RetErr)
            {
               STTST_TagCurrentLine( pars_p, "Expected  EVT DeviceName\n");
               return(TRUE);
            }
             break;
        default :
             break;
    }

     /* init */
     sprintf( HDMI_Msg, "STHDMI_Init(%s) : ", DeviceName );
     ErrorCode1 = STHDMI_Init(DeviceName, &HDMIInitParams);
     RetErr = HDMI_TextError( ErrorCode1, HDMI_Msg);

     switch (HDMIInitParams.DeviceType)
    {
       case STHDMI_DEVICE_TYPE_7710 : /* no break */
       case STHDMI_DEVICE_TYPE_7100 :
       case STHDMI_DEVICE_TYPE_7200 :
            sprintf( HDMI_Msg, "\tType=%s, Out=%s, Vout=\"%s\", Vtg=\"%s\"\n",
                    (HDMIInitParams.DeviceType==STHDMI_DEVICE_TYPE_7710)? "TYPE_7710":
                    (HDMIInitParams.DeviceType==STHDMI_DEVICE_TYPE_7100)? "TYPE_7100" :"TYPE_7200",
                    (Var < HDMI_MAX_SUPPORTED+1)?HdmiOutTypeName[Var].Name : "????",
                    HDMIInitParams.Target.OnChipHdmiCell.VOUTDeviceName,
                    HDMIInitParams.Target.OnChipHdmiCell.VTGDeviceName
                    );
            break;
       default :
            break;
    }
    HDMI_PRINT (HDMI_Msg);
    return ( API_EnableError ? RetErr : FALSE );
 } /* end of HDMI_Init */
 /*-------------------------------------------------------------------------
 * Function : HDMI_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STHDMI_OpenParams_t HDMIOpenParams;
    ST_ErrorCode_t      ErrorCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    /* Open a HDMI device connection */

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* get device */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( HDMI_Msg, "STHDMI_Open(%s): ", DeviceName);
    ErrorCode = STHDMI_Open(DeviceName, &HDMIOpenParams, &HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);
    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( HDMI_Msg, "\tHandle=%d\n", HDMIHndl);
        HDMI_PRINT(HDMI_Msg);
        STTST_AssignInteger( result_sym_p, HDMIHndl, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_Open */

/*-------------------------------------------------------------------------
 * Function : HDMI_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrorCode;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

  /* Close a HDMI device connection */
    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( HDMI_Msg, "STHDMI_Close(%d): ", HDMIHndl);
    ErrorCode = STHDMI_Close(HDMIHndl);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_Close */

/*-------------------------------------------------------------------------
 * Function : HDMI_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL HDMI_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STHDMI_TermParams_t HDMITermParams;
    ST_ErrorCode_t      ErrorCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    U32  LVar;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    /* get device */
    RetErr = STTST_GetString( pars_p, STHDMI_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
    STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }
    HDMITermParams.ForceTerminate = (BOOL)LVar;

    sprintf( HDMI_Msg, "STHDMI_Term(%s): ", DeviceName);
    ErrorCode = STHDMI_Term(DeviceName, &HDMITermParams);
    RetErr = HDMI_TextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_Term */

#ifdef STVOUT_CEC_PIO_COMPARE
/*-------------------------------------------------------------------------
 * Function : HDMI_CECSendCommand
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL HDMI_CECSendCommand( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    BOOL RetErr;
    STHDMI_CEC_Command_t CEC_Command;

    S32 CmdTestNb,DestAddress,SourceAddress;

    /*Unused parameter*/
    UNUSED_PARAMETER (result_sym_p);

    RetErr = STTST_GetInteger( pars_p, HDMIHndl, (S32*)&HDMIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 15, (S32*)&SourceAddress);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Source Address");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 3, (S32*)&DestAddress);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Destination Address");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&CmdTestNb);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Command Test Number");
        return(TRUE);
    }

    CEC_Command.Retries                    = 1;
    CEC_Command.Source                     = SourceAddress;
    CEC_Command.Destination                = DestAddress;

    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */
    switch(CmdTestNb)
    {
        case 0 :
            CEC_Command.CEC_Opcode         = STHDMI_CEC_OPCODE_IMAGE_VIEW_ON;
            break;
        case 1 : /* Sending Two messages test */
            CEC_Command.CEC_Opcode         = STHDMI_CEC_OPCODE_TEXT_VIEW_ON;
            RetErr = STHDMI_CECSendCommand(HDMIHndl, &CEC_Command); /* First and the global one will be the second*/
            break;
        case 2 :
            CEC_Command.CEC_Opcode         = STHDMI_CEC_OPCODE_STANDBY;
            break;
        case 3 : /* One Touch Play */
            CEC_Command.CEC_Opcode         = STHDMI_CEC_OPCODE_IMAGE_VIEW_ON;
            RetErr = STHDMI_CECSendCommand(HDMIHndl, &CEC_Command);
            CEC_Command.CEC_Opcode         = STHDMI_CEC_OPCODE_ACTIVE_SOURCE;
            CEC_Command.CEC_Operand.PhysicalAddress.A = 1;
            CEC_Command.CEC_Operand.PhysicalAddress.B = 0;
            CEC_Command.CEC_Operand.PhysicalAddress.C = 0;
            CEC_Command.CEC_Operand.PhysicalAddress.D = 0;
            break;
        default:
            CEC_Command.CEC_Opcode                 = STHDMI_CEC_OPCODE_POLLING_MESSAGE;
            CEC_Command.Retries                    = 1;
            CEC_Command.Source                     = 15; /* Source must be Unregistred */
            break;
    }

    RetErr = STHDMI_CECSendCommand(HDMIHndl, &CEC_Command);
    STOS_TaskDelay(ST_GetClocksPerSecond()/2); /* To avoid debug from corrupting Transmission */

    sprintf( HDMI_Msg, "HDMI_CECSendCommand (%d) :", HDMIHndl );
    RetErr = VOUT_DotTextError( ErrorCode, HDMI_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end HDMI_CECSendCommand */
#endif /* STVOUT_CEC_PIO_COMPARE */

/*-------------------------------------------------------------------------
 * Function : HDMI_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL HDMI_InitCommand(void)
{
    BOOL RetErr = FALSE;

/* API functions : */

    RetErr  = STTST_RegisterCommand( "HDMI_Capa",      HDMI_GetCapability, "HDMI Get Capability ('DevName')");
    RetErr |= STTST_RegisterCommand( "HDMI_SrcCapa",   HDMI_GetSourceCapability, "HDMI Get Source Capability ('DevName')");
    RetErr |= STTST_RegisterCommand( "HDMI_SnkCapa",   HDMI_GetSinkCapability, "HDMI Get Sink Capability ('DevName')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillAvi",   HDMI_FillAVIInfoFrame, "HDMI Fill AVI ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillSpd",   HDMI_FillSPDInfoFrame, "HDMI Fill SPD ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillAudio", HDMI_FillAudioInfoFrame, "HDMI Fill Audio ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillMs",    HDMI_FillMSInfoFrame, "HDMI Fill MS ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillVs",    HDMI_FillVSInfoFrame, "HDMI Fill VS ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FillEdid",  HDMI_FillSinkEDID, "HDMI Fill Sink EDID ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_FreeMemory",HDMI_FreeMemory, "HDMI Free Sink EDID memory('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_Init",      HDMI_Init, "HDMI Init ('DevName')");
    RetErr |= STTST_RegisterCommand( "HDMI_Open",      HDMI_Open, "HDMI Open ('DevName')");
    RetErr |= STTST_RegisterCommand( "HDMI_Close",     HDMI_Close, "HDMI Close ('Handle')");
    RetErr |= STTST_RegisterCommand( "HDMI_Revision",  HDMI_GetRevision, "HDMI Get Revision");
    RetErr |= STTST_RegisterCommand( "HDMI_Term",      HDMI_Term, "HDMI Term ('DevName' 'ForceTeminate')");

#ifdef STVOUT_CEC_PIO_COMPARE
    RetErr |= STTST_RegisterCommand( "HDMI_CECSendCommand", HDMI_CECSendCommand , "Send on the HDMI CEC bus ('Handle','Dest Address')");
#endif


#if defined (ST_OSLINUX)
    RetErr |= STTST_AssignString("DRV_PATH_HDMI", "hdmi/", TRUE);
#else
    RetErr |= STTST_AssignString("DRV_PATH_HDMI", "", TRUE);
#endif
    /*Adding Path of video stream */
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_1080I", "1080i/30_00fps/", TRUE);
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_720P",  "0720p/60_00fps/", TRUE);
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_480P",  "0480p/30_00fps/", TRUE);
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_480I",  "0480i/30_00fps/", TRUE);
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_576P",  "0576p/25_00fps/", TRUE);
    RetErr |= STTST_AssignString("DRV_PATH_HDMI_576I",  "0576i/25_00fps/", TRUE);
    /* Adding path of video firmware*/
    RetErr |= STTST_AssignString("PATH_FIRMWARES_HDMI","../../", FALSE);


    RetErr |= STTST_AssignInteger ("ERR_HDMI_AVAILABLE",      STHDMI_ERROR_NOT_AVAILABLE,            TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_INVALID_PACKET", STHDMI_ERROR_INVALID_PACKET_FORMAT,    TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_INVALID_INFO",   STHDMI_ERROR_INFOMATION_NOT_AVAILABLE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_VIDEO_UNKOWN",   STHDMI_ERROR_VIDEO_UNKOWN,             TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_VTG_UNKOWN",     STHDMI_ERROR_VTG_UNKOWN,               TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_VMIX_UNKOWN",    STHDMI_ERROR_VMIX_UNKOWN,              TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_AUDIO_UNKOWN",   STHDMI_ERROR_AUDIO_UNKOWN,             TRUE);
    RetErr |= STTST_AssignInteger ("ERR_HDMI_VOUT_UNKOWN",    STHDMI_ERROR_VOUT_UNKOWN,              TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_DEVICE_TYPE_7710",  STHDMI_DEVICE_TYPE_7710 + 1,TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_DEVICE_TYPE_7100",  STHDMI_DEVICE_TYPE_7100 + 1,TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_DEVICE_TYPE_7200",  STHDMI_DEVICE_TYPE_7200 + 1,TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_INFOFRAME_TYPE_VS",    shift(STHDMI_INFOFRAME_TYPE_VENDOR_SPEC),   TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_INFOFRAME_TYPE_AVI",   shift(STHDMI_INFOFRAME_TYPE_AVI),           TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_INFOFRAME_TYPE_SPD",   shift(STHDMI_INFOFRAME_TYPE_SPD),           TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_INFOFRAME_TYPE_AUDIO", shift(STHDMI_INFOFRAME_TYPE_AUDIO),         TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_INFOFRAME_TYPE_MS",    shift(STHDMI_INFOFRAME_TYPE_MPEG_SOURCE),   TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_AVI_TYPE_NONE",  shift(STHDMI_AVI_INFOFRAME_FORMAT_NONE) ,     TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_AVI_TYPE_ONE",   shift(STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE),  TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_AVI_TYPE_TWO",   shift(STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO) ,  TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_SPD_TYPE_NONE",  shift(STHDMI_SPD_INFOFRAME_FORMAT_NONE),     TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_TYPE_ONE",   shift(STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE),  TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_MS_TYPE_NONE",  shift(STHDMI_MS_INFOFRAME_FORMAT_NONE),     TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_MS_TYPE_ONE",   shift(STHDMI_MS_INFOFRAME_FORMAT_VER_ONE),  TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_AUDIO_TYPE_NONE",  shift(STHDMI_AUDIO_INFOFRAME_FORMAT_NONE),     TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_AUDIO_TYPE_ONE",   shift(STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE),  TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_VS_TYPE_NONE",  shift(STHDMI_VS_INFOFRAME_FORMAT_NONE) ,     TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_VS_TYPE_ONE",   shift(STHDMI_VS_INFOFRAME_FORMAT_VER_ONE),  TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_BAR_INFO_NONE",STHDMI_BAR_INFO_NOT_VALID +1,            TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_BAR_INFO_V",   STHDMI_BAR_INFO_V +1,                    TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_BAR_INFO_H",   STHDMI_BAR_INFO_H +1,                    TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_BAR_INFO_VH",  STHDMI_BAR_INFO_VH +1,                   TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_SCAN_INFO_NONE",    STHDMI_SCAN_INFO_NO_DATA +1,        TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SCAN_INFO_OVERS",   STHDMI_SCAN_INFO_OVERSCANNED +1,    TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SCAN_INFO_UNDERS",  STHDMI_SCAN_INFO_UNDERSCANNED +1,   TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_PICTURE_NON_UNI",  STHDMI_PICTURE_NON_UNIFORM_SCALING +1, TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_PICTURE_SCALING_H",STHDMI_PICTURE_SCALING_H +1,           TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_PICTURE_SCALING_V",STHDMI_PICTURE_SCALING_V +1,           TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_PICTURE_SCALING_HV",STHDMI_PICTURE_SCALING_HV +1,         TRUE);

    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_UNK",  STHDMI_SPD_DEVICE_UNKNOWN +1,          TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_DIG",STHDMI_SPD_DEVICE_DIGITAL_STB +1,        TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_DVD",STHDMI_SPD_DEVICE_DVD +1,                TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_D_VHS",STHDMI_SPD_DEVICE_D_VHS +1,            TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_HDD",  STHDMI_SPD_DEVICE_HDD_VIDEO +1,        TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_DVC",STHDMI_SPD_DEVICE_DVC +1,                TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_DSC",STHDMI_SPD_DEVICE_DSC +1,                TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_VID_CD",STHDMI_SPD_DEVICE_VIDEO_CD +1,        TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_GAME",STHDMI_SPD_DEVICE_GAME +1,              TRUE);
    RetErr |= STTST_AssignInteger ("HDMI_SPD_DEVICE_PC",STHDMI_SPD_DEVICE_PC_GENERAL +1,          TRUE);
    RetErr |= STTST_AssignString  ("HDMI_NAME", STHDMI_DEVICE_NAME, FALSE);
    return(!RetErr);
} /* end HDMI_InitCommand */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL HDMI_RegisterCmd()
{
    BOOL RetOk;

    RetOk = HDMI_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "HDMI_RegisterCmd()     \t: ok           %s\n", STHDMI_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "HDMI_RegisterCmd()     \t: failed !\n" ));
    }

    return(RetOk);
}
/* end of file */
