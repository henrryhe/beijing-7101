/************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : vout_test.c
Description : VOUT macros

Note        :

Date          Modification                                    Initials
----          ------------                                    --------

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stlite.h"
#include "stvout.h"
#include "stvtg.h"
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants --- */
#define VOUT_MAX_SUPPORTED 11

#if defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || defined (ST_5518) || defined (ST_5514)
#define VOUT_BASE_ADDRESS         0 /* dummy, for compilation */
#define VOUT_DEVICE_BASE_ADDRESS  0 /* dummy, for compilation */
#define VOUT_BASE_ADDRESS_DENCIF2 0 /* dummy, for compilation */
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_DENC
#elif defined(ST_7015)
#define VOUT_BASE_ADDRESS         ST7015_DSPCFG_OFFSET
#define VOUT_BASE_ADDRESS_DENCIF2 0 /* dummy, for compilation */
#define VOUT_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7015
#elif defined(ST_7020)
#define VOUT_BASE_ADDRESS         ST7020_DSPCFG_OFFSET
#define VOUT_BASE_ADDRESS_DENCIF2 0 /* dummy, for compilation */
#define VOUT_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7020
#elif defined(ST_GX1)
#define VOUT_BASE_ADDRESS         0 /* dummy, for compilation */
#define VOUT_BASE_ADDRESS_DENCIF1 0xBB440010
#define VOUT_BASE_ADDRESS_DENCIF2 0xBB440020
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_GX1
#else
#error Not defined yet
#endif

/* --- Macros ---*/
#define VOUT_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VOUT_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* --- Extern functions --- */

/* --- Global variables --- */

/* --- Extern variables --- */
extern ST_Partition_t *SystemPartition;

/* --- Private variables --- */
static char VOUT_Msg[200];            /* text for trace */
static STVOUT_Handle_t VOUTHndl;

typedef struct{
    char Name[15];
    STVOUT_OutputType_t Type;
} VoutTable_t;

static const VoutTable_t VoutTypeName[VOUT_MAX_SUPPORTED]={
    { "RGB", STVOUT_OUTPUT_ANALOG_RGB },
    { "YUV", STVOUT_OUTPUT_ANALOG_YUV },
    { "YC", STVOUT_OUTPUT_ANALOG_YC },
    { "CVBS", STVOUT_OUTPUT_ANALOG_CVBS },
    { "SVM", STVOUT_OUTPUT_ANALOG_SVM },
    { "YCbCr444", STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS },
    { "YCbCr422_16", STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED },
    { "YCbCr422_8", STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED },
    { "RGB888", STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS },
    { "HD_RGB", STVOUT_OUTPUT_HD_ANALOG_RGB },
    { "HD_YUV", STVOUT_OUTPUT_HD_ANALOG_YUV }
};


/* Local prototypes ------------------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : VOUT_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VOUT_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVOUT_ERROR_DENC_ACCESS:
                strcat( Text, "(stvout error denc access failed) !\n");
                break;
            case STVOUT_ERROR_VOUT_ENABLE:
                strcat( Text, "(stvout error output is already enable) !\n");
                break;
            case STVOUT_ERROR_VOUT_NOT_ENABLE:
                strcat( Text, "(stvout error output is already disable) !\n");
                break;
            case STVOUT_ERROR_VOUT_NOT_AVAILABLE:
                strcat( Text, "(stvout error Output not available) !\n");
                break;
            case STVOUT_ERROR_VOUT_INCOMPATIBLE:
                strcat( Text, "(stvout error Incompatible Output) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VOUT_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*#######################################################################*/
/*############################# VOUT COMMANDS ###########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VOUT_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * Testtool usage :
 * vout_init "devicename" devicetype=DENC            outputtype=RGB/YC/YUV/CVBS  "dencname"   maxopen
 * vout_init "devicename" devicetype=7015 or 7020    outputtype=RGB/YC/YUV/CVBS  "dencname"   maxopen
 * vout_init "devicename" devicetype=7015 or 7020    outputtype=HDxxx/SVM        Baseaddress  maxopen
 * vout_init "devicename" devicetype=GX1             outputtype=RGB/YC/YUV/CVBS  "dencname"   Baseaddress maxopen
 * vout_init "devicename" devicetype=DIGITAL_OUTPUT  outputtype=YCbCr422_8       Baseaddress  maxopen
 * vout_init "VOUT" typeDENC typeCVBS "DENC" 3 (default values)
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_InitParams_t VOUTInitParams;
    ST_ErrorCode_t      ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH], TrvStr[80], IsStr[80], *ptr;
    U32 LVar, Var,i;
    BOOL RetErr, IsDencOutput;

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get device type  */
    RetErr = STTST_GetInteger( pars_p, VOUT_DEVICE_TYPE, (S32*)&LVar );
    if ((RetErr==TRUE) || (LVar>4))
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 0=DENC 1=7015 2=7020 3=GX1 4=Digital Output");
        return(TRUE);
    }
    VOUTInitParams.DeviceType = (STVOUT_DeviceType_t)LVar;

    /* Get output type (default: ANALOG CVBS) */
    if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)
    {
        /* Get output type */
        STTST_GetItem(pars_p, "YCbCr422_8", TrvStr, sizeof(TrvStr));
    }
    else
    {
        /* Get output type (default: ANALOG CVBS) */
        STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    }
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VOUT_MAX_SUPPORTED; Var++){
        if((strcmp(VoutTypeName[Var].Name, TrvStr)==0)|| \
           strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0)
            break;
    }
    if(Var==VOUT_MAX_SUPPORTED){
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities)");
            return(TRUE);
        }

        for(i=0; i<VOUT_MAX_SUPPORTED; i++){
            if(VoutTypeName[i].Type == Var){
                Var=i;
                break;
            }
        }
    }

    VOUTInitParams.OutputType = (STVOUT_OutputType_t)(1<<Var);
    VOUTInitParams.CPUPartition_p = SystemPartition;

    IsDencOutput = (   (VOUTInitParams.OutputType == STVOUT_OUTPUT_ANALOG_RGB)
                    || (VOUTInitParams.OutputType == STVOUT_OUTPUT_ANALOG_YUV)
                    || (VOUTInitParams.OutputType == STVOUT_OUTPUT_ANALOG_YC)
                    || (VOUTInitParams.OutputType == STVOUT_OUTPUT_ANALOG_CVBS));

/*    if (   (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC)*/
/*        || ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015) && IsDencOutput))*/
/*    {*/
/*        RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.DencName, \*/
/*                                  sizeof(ST_DeviceName_t));*/
/*    }*/
/*    if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015) && !IsDencOutput)*/
/*    {*/
/*        RetErr = STTST_GetInteger( pars_p, VOUT_BASE_ADDRESS, (S32*)&LVar);*/
/*        if (RetErr==TRUE)*/
/*        {*/
/*            STTST_TagCurrentLine( pars_p, "Expected Base Address");*/
/*            return(TRUE);*/
/*        }*/
/*        VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void*)LVar;*/
/*        VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;*/
/*    }*/
/*    if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)*/
/*    {*/
/*        RetErr = STTST_GetInteger( pars_p, (S32)VIDEO_BASE_ADDRESS, (S32*)&LVar);*/
/*        if (RetErr==TRUE)*/
/*        {*/
/*            STTST_TagCurrentLine( pars_p, "Expected Base Address");*/
/*            return(TRUE);*/
/*        }*/
/*        VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void*)LVar;*/
/*        VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;*/
/*    }*/
    /* Get denc name and/or base Address Add */
    switch (VOUTInitParams.DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME1, VOUTInitParams.Target.DencName, sizeof(ST_DeviceName_t));
            break;
        case STVOUT_DEVICE_TYPE_7015 :
            if(VOUTInitParams.OutputType > STVOUT_OUTPUT_ANALOG_CVBS)
            {
                RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
                if (RetErr==TRUE)
                {
                    STTST_TagCurrentLine( pars_p, "Expected Base Address");
                    return(TRUE);
                }
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void*)LVar;
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            }
            else
            {
                RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME1, VOUTInitParams.Target.DencName, sizeof(ST_DeviceName_t));
            }
            break;
        case STVOUT_DEVICE_TYPE_7020 :
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME1, VOUTInitParams.Target.GenericCell.DencName, sizeof(ST_DeviceName_t));
            RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS, (S32*)&LVar);
            if (RetErr==TRUE)
            {
                STTST_TagCurrentLine( pars_p, "Expected Base Address");
                return(TRUE);
            }
            VOUTInitParams.Target.GenericCell.BaseAddress_p = (void*)LVar;
            VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
            break;
#if defined (ST_GX1)
        case STVOUT_DEVICE_TYPE_GX1:
            RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VOUTInitParams.Target.GX1Cell.DencName, \
                                  sizeof(ST_DeviceName_t));
            RetErr = STTST_GetInteger( pars_p, (S32)VOUT_BASE_ADDRESS_DENCIF2, (S32*)&LVar);
            if (RetErr==TRUE)
            {
                STTST_TagCurrentLine( pars_p, "Expected DENC Interface Register Address");
                return(TRUE);
            }
            VOUTInitParams.Target.GX1Cell.BaseAddress_p = (void*)LVar;
            VOUTInitParams.Target.GX1Cell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
#endif /* ST_GX1 */
        default :
            break;
    }

    VOUTInitParams.MaxOpen = 3; /* MaxOpen */

    if (   (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC)
        || (((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015) || (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7020))
            && IsDencOutput))
    {
        sprintf( VOUT_Msg, "VOUT_Init(%s,Type=%s,Out=%s,Denc=\"%s\"): ", DeviceName, \
                 "DENC",
                 (Var < VOUT_MAX_SUPPORTED) ? VoutTypeName[Var].Name : "????",
                 VOUTInitParams.Target.DencName
            );
    }
    if (((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015) ||
         (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7020)) && !IsDencOutput)
    {
        sprintf( VOUT_Msg, "VOUT_Init(%s,Type=%s,Out=%s,Add=0x%x): ", DeviceName, \
                    (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015) ? "7015" :
                    (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7020) ? "7020" : "????",
                    (Var < VOUT_MAX_SUPPORTED) ? VoutTypeName[Var].Name : "????",
                    (U32) VOUTInitParams.Target.HdsvmCell.BaseAddress_p
            );
    }
    if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)
    {
        sprintf( VOUT_Msg, "VOUT_Init(%s,Type=%s,Out=%s,Add=0x%x): ", DeviceName, \
                    "Digital Output",
                    (Var < VOUT_MAX_SUPPORTED) ? VoutTypeName[Var].Name : "????",
                    (U32) VOUTInitParams.Target.HdsvmCell.BaseAddress_p
            );
    }
#if defined (ST_GX1)
    if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_GX1)
    {
        sprintf( VOUT_Msg, "VOUT_Init(%s,Type=%s,Out=%s,Denc=\"%s\",Add=0x%x): ", DeviceName, \
                 "GX1",
                 (Var < VOUT_MAX_SUPPORTED) ? VoutTypeName[Var].Name : "????",
                 VOUTInitParams.Target.DencName,
                 (U32) VOUTInitParams.Target.GX1Cell.BaseAddress_p
            );
    }
#endif /* ST_GX1 */
    ErrCode = STVOUT_Init(DeviceName, &VOUTInitParams);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VOUT_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OpenParams_t VOUTOpenParams;
    ST_ErrorCode_t      ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;

    /* Init structure */
    memset((void *)&VOUTOpenParams, 0, sizeof(STVOUT_OpenParams_t));

    /* get device */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Open(%s): ", DeviceName);
    ErrCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf(VOUT_Msg, "\t Hndl = %d\n", VOUTHndl);
        VOUT_PRINT(VOUT_Msg);
        STTST_AssignInteger( result_sym_p, VOUTHndl, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VOUT_Capability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Capability( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char DeviceName[STRING_DEVICE_LENGTH];
    char StrParams[RETURN_PARAMS_LENGTH];
    STVOUT_Capability_t Capability;
    ST_ErrorCode_t      ErrCode;
    BOOL RetErr;
    U32  Var;

    /* Init structure */
    memset((void *)&Capability, 0, sizeof(STVOUT_Capability_t));

    /* get device name */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_GetCapability(%s): ",  DeviceName);
    ErrCode = STVOUT_GetCapability(DeviceName, &Capability);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);

    if(ErrCode==ST_NO_ERROR){
        sprintf( VOUT_Msg, "\tSupported Type:\t");

        for(Var=0; Var<VOUT_MAX_SUPPORTED; Var++){
            if(Capability.SupportedOutputs & (1<<Var)){
                sprintf(VOUT_Msg, "%s %s=%d ", VOUT_Msg, VoutTypeName[Var].Name, (1<<Var));
                if((Var+1)%4 == 0)
                    strcat(VOUT_Msg, "\n\t\t");
            }
        }
        Var=0;
        while((1<<Var != Capability.SelectedOutput) && (Var<VOUT_MAX_SUPPORTED))
              Var++;
        sprintf( VOUT_Msg, "%s\n\tSelected Type: %s\n", VOUT_Msg, \
                 (Var!=VOUT_MAX_SUPPORTED) ? VoutTypeName[Var].Name : "???");

        sprintf( StrParams, "%d %d", \
                 Capability.SupportedOutputs, Capability.SelectedOutput);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VOUT_PRINT(VOUT_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : VOUT_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Close(%d): ", VOUTHndl);
    ErrCode = STVOUT_Close(VOUTHndl);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VOUT_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_TermParams_t VOUTTermParams;
    ST_ErrorCode_t      ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    U32  LVar;

    /* get device */
    RetErr = STTST_GetString( pars_p, STVOUT_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }
    VOUTTermParams.ForceTerminate = (BOOL)LVar;

    sprintf( VOUT_Msg, "STVOUT_Term(%s, forceterm=%d): ", DeviceName, VOUTTermParams.ForceTerminate);
    ErrCode = STVOUT_Term(DeviceName, &VOUTTermParams);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VOUT_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
ST_Revision_t RevisionNumber;

    RevisionNumber = STVOUT_GetRevision( );
    sprintf( VOUT_Msg, "STVOUT_GetRevision(): Ok\nrevision=%s\n", RevisionNumber );
    VOUT_PRINT(VOUT_Msg);
    return ( FALSE );
} /* end VOUT_GetRevision */


/*-------------------------------------------------------------------------
 * Function : VOUT_SetParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_SetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t OutParam;
    ST_ErrorCode_t ErrCode;
    char TrvStr[80], IsStr[80], *ptr;
    BOOL RetErr;
    U32 LVar, Var,i;

    /* Init structure */
    memset((void *)&OutParam, 0, sizeof(STVOUT_OutputParams_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG CVBS) */
    STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VOUT_MAX_SUPPORTED; Var++){
        if((strcmp(VoutTypeName[Var].Name, TrvStr)==0)|| \
           strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0)
            break;
    }
    if(Var==VOUT_MAX_SUPPORTED){
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities) default CVBS");
            return(TRUE);
        }

        for(i=0; i<VOUT_MAX_SUPPORTED; i++){
            if(VoutTypeName[i].Type == Var){
                Var=i;
                break;
            }
        }
    }

    switch((STVOUT_OutputType_t) 1<<Var)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :
        case STVOUT_OUTPUT_ANALOG_YUV :
        case STVOUT_OUTPUT_ANALOG_YC :
        case STVOUT_OUTPUT_ANALOG_CVBS :
        case STVOUT_OUTPUT_HD_ANALOG_RGB :
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            /* get embedded type  */
            RetErr = STTST_GetInteger( pars_p, 0 , (S32*)&LVar );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "Expected embedded type (0-1)");
                return(TRUE);
            }
            OutParam.Analog.EmbeddedType = LVar;

            /* Other default values */
            OutParam.Analog.StateAnalogLevel = STVOUT_PARAMS_DEFAULT;
            OutParam.Analog.StateBCSLevel = STVOUT_PARAMS_DEFAULT;
            OutParam.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;

            /* get inverted  */
#if defined (mb231)
            RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
#else
            RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
#endif
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "Expected inverted (0-1)");
                return(TRUE);
            }
            OutParam.Analog.InvertedOutput = LVar;
            /* Synchro in chroma  */
            RetErr = STTST_GetInteger( pars_p, FALSE , (S32*)&LVar );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "Expected Synchro in chroma (default FALSE)");
                return(TRUE);
            }
            OutParam.Analog.SyncroInChroma = (BOOL)LVar;

            /* Color Space */
            RetErr = STTST_GetInteger( pars_p, STVOUT_SMPTE240M, (S32*)&LVar );
            if ((RetErr == TRUE) || (LVar>=4))
            {
                STTST_TagCurrentLine( pars_p, "Expected color space (SMPTE240M=1, ITU_R_601=2, ITU_R_709=3)");
                return(TRUE);
            }
            OutParam.Analog.ColorSpace = (STVOUT_ColorSpace_t)LVar;
            break;

        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
            /* Get embedded type  */
            RetErr = STTST_GetInteger( pars_p, TRUE , (S32*)&LVar );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "Expected embedded type (0-default 1)");
                return(TRUE);
            }
            OutParam.Digital.EmbeddedType = LVar;

            /* Synchro in chroma  */
            RetErr = STTST_GetInteger( pars_p, FALSE , (S32*)&LVar );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "Expected Synchro in chroma (default FALSE)");
                return(TRUE);
            }
            OutParam.Digital.SyncroInChroma = (BOOL)LVar;

            /* Color Space */
            RetErr = STTST_GetInteger( pars_p, STVOUT_SMPTE240M, (S32*)&LVar );
            if (RetErr == TRUE)
            {
                STTST_TagCurrentLine( pars_p, "Expected color space (SMPTE240M=1, ITU_R_601=2, ITU_R_709=3)");
                return(TRUE);
            }
            OutParam.Digital.ColorSpace = (STVOUT_ColorSpace_t)LVar;
            break;

        case STVOUT_OUTPUT_ANALOG_SVM :
            break;

        default:
            /* Not treated. All values to zero */
            break;
    }

    sprintf( VOUT_Msg, "STVOUT_SetOutputParams(%d): " , VOUTHndl );
    ErrCode = STVOUT_SetOutputParams(VOUTHndl,&OutParam);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);

    if(ErrCode == ST_NO_ERROR)
    {

        switch((STVOUT_OutputType_t) 1<<Var)
        {
            case STVOUT_OUTPUT_ANALOG_RGB :
            case STVOUT_OUTPUT_ANALOG_YUV :
            case STVOUT_OUTPUT_ANALOG_YC :
            case STVOUT_OUTPUT_ANALOG_CVBS :
            case STVOUT_OUTPUT_HD_ANALOG_RGB :
            case STVOUT_OUTPUT_HD_ANALOG_YUV :
                sprintf(VOUT_Msg, "\tEmb=%d Inv=%d Sync=%s Color=%d\n" ,\
                        OutParam.Analog.EmbeddedType, OutParam.Analog.InvertedOutput,
                        (OutParam.Analog.SyncroInChroma==TRUE) ? "TRUE":
                        (OutParam.Analog.SyncroInChroma==FALSE) ? "FALSE" : "????",
                        OutParam.Analog.ColorSpace);
                break;

            case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
            case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
            case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
            case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                sprintf(VOUT_Msg, "\tEmb=%s Sync=%s Color=%d\n" ,\
                        (OutParam.Digital.EmbeddedType==TRUE) ? "TRUE":
                        (OutParam.Digital.EmbeddedType==FALSE) ? "FALSE" : "????",
                        (OutParam.Digital.SyncroInChroma==TRUE) ? "TRUE":
                        (OutParam.Digital.SyncroInChroma==FALSE) ? "FALSE" : "????",
                        OutParam.Digital.ColorSpace);
                break;

            case STVOUT_OUTPUT_ANALOG_SVM :
                sprintf (VOUT_Msg , "\tTo be treated\n");
                break;

            default:
                sprintf (VOUT_Msg , "\tTo be treated\n");
                break;
        }
        VOUT_PRINT(VOUT_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VOUT_SetOutputParams */


/*-------------------------------------------------------------------------
 * Function : VOUT_GetParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_GetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_OutputParams_t OutParam;
    ST_ErrorCode_t ErrCode;
    char TrvStr[80], IsStr[80], *ptr;
    BOOL RetErr;
    U32  Var,i;

    /* Init structure */
    memset((void *)&OutParam, 0, sizeof(STVOUT_OutputParams_t));

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output type (default: ANALOG CVBS) */
    STTST_GetItem(pars_p, "CVBS", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VOUT_MAX_SUPPORTED; Var++){
        if((strcmp(VoutTypeName[Var].Name, TrvStr)==0)|| \
           strncmp(VoutTypeName[Var].Name, TrvStr+1, strlen(VoutTypeName[Var].Name))==0)
            break;
    }
    if(Var==VOUT_MAX_SUPPORTED){
        Var = (U32)strtoul(TrvStr, &ptr, 10);
        if (TrvStr==ptr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ouput type (See capabilities) default CVBS");
            return(TRUE);
        }

        for(i=0; i<VOUT_MAX_SUPPORTED; i++){
            if(VoutTypeName[i].Type == Var){
                Var=i;
                break;
            }
        }
    }
    sprintf(VOUT_Msg, "STVOUT_GetOutputParams(%d): ", VOUTHndl);
    ErrCode = STVOUT_GetOutputParams(VOUTHndl, (STVOUT_OutputParams_t* const) &OutParam);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);

    if(ErrCode == ST_NO_ERROR)
    {
        switch((STVOUT_OutputType_t) 1<<Var)
        {
            case STVOUT_OUTPUT_ANALOG_RGB :
            case STVOUT_OUTPUT_ANALOG_YUV :
            case STVOUT_OUTPUT_ANALOG_YC :
            case STVOUT_OUTPUT_ANALOG_CVBS :
            case STVOUT_OUTPUT_HD_ANALOG_RGB :
            case STVOUT_OUTPUT_HD_ANALOG_YUV :
                sprintf(VOUT_Msg, "\tEmb=%d Inv=%d Sync=%s Color=%d\n" ,\
                        OutParam.Analog.EmbeddedType, OutParam.Analog.InvertedOutput,
                        (OutParam.Analog.SyncroInChroma==TRUE) ? "TRUE":
                        (OutParam.Analog.SyncroInChroma==FALSE) ? "FALSE" : "????",
                        OutParam.Analog.ColorSpace);
                break;

            case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
            case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
            case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
            case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                sprintf(VOUT_Msg, "\tEmb=%s Sync=%s Color=%d\n" ,\
                        (OutParam.Digital.EmbeddedType==TRUE) ? "TRUE":
                        (OutParam.Digital.EmbeddedType==FALSE) ? "FALSE" : "????",
                        (OutParam.Digital.SyncroInChroma==TRUE) ? "TRUE":
                        (OutParam.Digital.SyncroInChroma==FALSE) ? "FALSE" : "????",
                        OutParam.Digital.ColorSpace);
                break;

            case STVOUT_OUTPUT_ANALOG_SVM :
                sprintf (VOUT_Msg , "\tTo be treated\n");
                break;

            default:
                sprintf (VOUT_Msg , "\tTo be treated\n");
                break;
        }
        VOUT_PRINT(VOUT_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_GetParams */


/*-------------------------------------------------------------------------
 * Function : VOUT_Enable
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Enable( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Enable(%d): ", VOUTHndl);
    ErrCode = STVOUT_Enable( VOUTHndl);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}  /* end VOUT_Enable */

/*-------------------------------------------------------------------------
 * Function : VOUT_Disable
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_Disable( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, VOUTHndl, (S32*)&VOUTHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VOUT_Msg, "STVOUT_Disable(%d): ", VOUTHndl);
    ErrCode = STVOUT_Disable( VOUTHndl);
    RetErr = VOUT_TextError( ErrCode, VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}  /* end VOUT_Disable */


/*-------------------------------------------------------------------------
 * Function : VOUT_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VOUT_InitCommand(void)
{
    BOOL RetErr = FALSE;
    /* API functions : */
    RetErr  = STTST_RegisterCommand( "VOUTInit", VOUT_Init, "<DeviceName><DeviceType><OutType> VOUT Init");
    RetErr |= STTST_RegisterCommand( "VOUTOpen", VOUT_Open, "<DeviceName> VOUT Open");
    RetErr |= STTST_RegisterCommand( "VOUTClose", VOUT_Close, "<Handle> VOUT Close");
    RetErr |= STTST_RegisterCommand( "VOUTCapa", VOUT_Capability, "<DeviceName> VOUT capabilities");
    RetErr |= STTST_RegisterCommand( "VOUTTerm", VOUT_Term, "<DeviceName> VOUT Term");
    RetErr |= STTST_RegisterCommand( "VOUTRev", VOUT_GetRevision, "VOUT Get Revision");
    RetErr |= STTST_RegisterCommand( "VOUTSetParams", VOUT_SetParams, \
                                     "<Handle><Type><Inverse><Embeded> VOUT Set Parameters");
    RetErr |= STTST_RegisterCommand( "VOUTGetParams", VOUT_GetParams, "<Handle><Type> VOUT Get Parameters");
    RetErr |= STTST_RegisterCommand( "VOUTEnable", VOUT_Enable, "<Handle> Enable VOUT");
    RetErr |= STTST_RegisterCommand( "VOUTDisable", VOUT_Disable, "<Handle> Disable VOUT");

    /* constants */
    RetErr |= STTST_AssignString  ("VOUT_NAME", STVOUT_DEVICE_NAME, TRUE);

    return( RetErr ? FALSE : TRUE);

} /* end VOUT_MacroInit */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VOUT_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VOUT_InitCommand();
    if ( RetOk )
    {
        sprintf( VOUT_Msg, "VOUT_Command()     \t: ok           %s\n", STVOUT_GetRevision() );
    }
    else
    {
        sprintf( VOUT_Msg, "VOUT_Command()     \t: failed !\n");
    }
    VOUT_PRINT(VOUT_Msg);

    return(RetOk);
}
/* end of file */
