/*******************************************************************************

File name   : layerlay.c

Description : STLAYER API: Layer wrapper

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                               Name
----               ------------                               ----
2000-07-22         Created                                Michel Bruant
                                                          Philippe Legeard
2000-dec           Add 55xx gfx layers                    Michel Bruant
18 jan 2002        Added STLAYER_GetVTGParams()           Herve Goujon
Oct 2003           Added compositor                       Thomas Meyer
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#define VARIABLE_API_RESERVATION
#include "stlayer.h"
#include "layerapi.h"
#include "layer_rev.h"
#include "sttbx.h"

#ifndef ST_OSLINUX
    #include <string.h>
#endif

#ifdef ST_OSLINUX
    #include "stlayer_core.h"
#endif

#ifdef LAYER_COMPO
#include "laycompo.h"
#endif

#ifdef VIDEO
#include "halv_lay.h"
#endif

#ifdef GFX_GAMMA
#include "layergfx.h"
#endif

#ifdef GFX_55XX
#include "layerosd.h"
#include "lay_sub.h"
#ifdef GFX_STILL
#include "lay_stil.h"
#endif
#endif


/* Constants ---------------------------------------------------------------- */

#define BAD_INDEX           0x00ffffff
#define KEY_HANDLE_USED     0xa96f9d94

/* Variables ---------------------------------------------------------------- */

static BOOL FirstInitDone = FALSE;

/* Private Macros ----------------------------------------------------------- */

#define TestHandle(ApiHandle)  \
    if ((stlayer_OpenElement *)ApiHandle == NULL)      \
    {                           \
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));\
        return(ST_ERROR_INVALID_HANDLE);\
    }                           \
    if(((stlayer_OpenElement*)(ApiHandle))->Used != KEY_HANDLE_USED)\
    {\
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STLAYER - API : Bad Handle"));\
        return(ST_ERROR_INVALID_HANDLE);\
    }

#define GetDeviceName(ApiHandle) ((stlayer_OpenElement*)(ApiHandle))\
                                    ->LayerDevice->LayerName
#define GetType(ApiHandle)       ((stlayer_OpenElement*)(ApiHandle))\
                                    ->LayerDevice->LayerType
#define GetHandle(ApiHandle)     ((stlayer_OpenElement*)(ApiHandle))\
                                    ->LowLevelHandle

/* Private Functions -------------------------------------------------------- */

static U32 stlayer_IndexOfNamedLayer(const ST_DeviceName_t DeviceName)
{
    BOOL Found = FALSE;
    U32  i     = 0;
    do
    {
        if (strcmp(DeviceName,stlayer_LayerArray[i].LayerName) == 0)
        {
            Found = TRUE;
        }
        else
        {
            i++;
        }
    } while ((i < TOTAL_MAX_LAYER) && (!(Found)));
    if (Found)
    {
        return(i);
    }
    else
    {
        return(BAD_INDEX);
    }
}

/* -------------------------------------------------------------------------- */

static U32 stlayer_IndexOfFreeOpen(void)
{
    BOOL Found = FALSE;
    U32  i     = 0;
    do
    {
        if ( stlayer_OpenArray[i].Used != KEY_HANDLE_USED)
        {
            Found = TRUE;
        }
        else
        {
            i++;
        }
    } while ((i < (TOTAL_MAX_LAYER * MAX_OPEN)) && (!(Found)));
    if (Found)
    {
        return(i);
    }
    else
    {
        return(BAD_INDEX);
    }
}
/* Public Functions --------------------------------------------------------- */

/*******************************************************************************
Name        : STLAYER_GetRevision
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_Revision_t STLAYER_GetRevision(void)
{
  /*  1.0.0A6 = (02/2001) 1st code drop                 */
  /*  1.0.0A7 = (03/2001) 2nd code drop (sync with vid) */
  /*  1.0.0B  = (W18) 3rd code drop (sync with vid)     */
  /*  1.0.0   = (W21) virtual mem, thread-safe ...      */
  /*  1.0.1   = (W24) 4th code drop (sync with vid)     */
  /*  1.0.2   = (W25) 5th code drop (patch of 1.0.1 for vid) */
  /*  1.0.3   = (W27) correspond to vid 3.0.0 */
  /*  1.1.0   = (W35) correspond to vid 3.1.0 */
  /*  1.1.1   = (W36) correspond to vid 3.1.0 */
  /*  1.1.2   = (W43) add st40gx1 gdp layers */
  /*  1.1.3   = (W48) correspond to vid 3.2.0 */
  /*  1.1.4   = (W51) bad release */
  /*  1.1.5   = (W51) correspond to vid 3.2.1 */
  /*  1.2.0   = (W05) correspond to vid x.x.x */
  /*  2.0.0   = 7020 full features */
  /*  2.0.1   = 7020 full features, 2nd shot */
  /*  2.0.2   = (W37) coreespond to vid 3.4.3 */
  /*  2.0.3   = Tested on ATV2. No change as ususal */
  /*  2.0.4   = Allow 5517 compilation. Add OSD-RGB16 driver */
  /*  3.0.0   = Video PSI */
  /*  3.0.1   = 7020 2.1 Stem on mb382 (5517) */
  /*  3.0.3   = released for 5528 */
  /*  3.1.0A1 = Alpha release for sti5100/Aptix EMU1 */
  /*  3.0.4   = Fixes 5517+020, progress 5528, EMU2 5100 */
  /*  3.0.5   = Fixes 5528 */
  /*  3.1.0A2 = Alpha release for sti5100 Cut 1 silicon */
  /*  3.1.0   = Release M2 for sti5100 Cut 1 silicon  */
  /*  3.1.1   = Patch release M2 for sti5100 Cut 1 silicon  */
  /*  3.1.2   = Patch release, first one on STi7710. Adding OS21 support  */
  /*  3.1.3   = Patch release, second one on STi7710. Adding OS21 support  */
  /*  3.1.4   = Patch release, first one on STi5105.  */
  /*  3.1.5   = Patch release . fixes 5528  */
  /*  3.1.6   = Patch release, first one on STi7100.  */
  /*  3.1.7   = Patch release, video layer fixes on STi7100 & STi7710.  */
 /*  3.1.7   = Patch release, first one on STi5301 & video layer fixes on STi7100 & STi7710.*/
 /*  3.1.8   = Patch release, first one for linux on STi7100.*/
 /*  3.2.1   = Patch release, first one on STi7109.*/



    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return(LAYER_Revision);
} /* End of STLAYER_GetRevision() function */

/*******************************************************************************
Name        : STLAYER_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_Init(const ST_DeviceName_t        DeviceName,
                            const STLAYER_InitParams_t * const InitParams_p)
{

    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    U32             LowLevelHandle;

    if( stlayer_IndexOfNamedLayer(DeviceName) != BAD_INDEX)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_ALREADY_INITIALIZED"));
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    if(InitParams_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API :ST_ERROR_BAD_PARAMETER"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(FirstInitDone))
    {
        U32 Index;
        FirstInitDone = TRUE;
        for(Index = 0; Index < TOTAL_MAX_LAYER; Index ++)
        {
            stlayer_LayerArray[Index].LayerType = 0;
            strncpy(stlayer_LayerArray[Index].LayerName,"", sizeof(ST_DeviceName_t));
            stlayer_LayerArray[Index].VideoHandle = 0;
        }
        for(Index = 0; Index < TOTAL_MAX_LAYER * MAX_OPEN; Index ++)
        {
            stlayer_OpenArray[Index].LayerDevice = NULL;
            stlayer_OpenArray[Index].LowLevelHandle = 0;
            stlayer_OpenArray[Index].Used = 0;
        }
    }

    LowLevelHandle = 0;
    switch(InitParams_p->LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_Init(DeviceName,InitParams_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error          = LAYERGFX_Init(DeviceName,InitParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error          = LAYEROSD_Init(DeviceName,InitParams_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error          = LAYERSUB_Init(DeviceName,InitParams_p);
            break;
#if defined  GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error          = LAYERSTILL_Init(DeviceName,InitParams_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

			Error    = LAYERVID_Init(DeviceName, InitParams_p, &LowLevelHandle);
            break;
#endif
        default:
            break;
    }

    /* API Handles management */
    if (!Error) /* Record Name and type in stlayer_LayerArray */
    {
        U32 i = 0;
        BOOL Found = FALSE;
        do
        {
            if (strcmp(stlayer_LayerArray[i].LayerName,"") == 0) /* eq free */
            {
                strncpy(stlayer_LayerArray[i].LayerName,DeviceName,sizeof(ST_DeviceName_t)-1);
                stlayer_LayerArray[i].LayerType = InitParams_p->LayerType;
                stlayer_LayerArray[i].VideoHandle = LowLevelHandle;
                Found = TRUE;
            }
            else
            {
                i ++;
            }
        } while((i < TOTAL_MAX_LAYER) && (!(Found)));
        if (!(Found))

        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_NO_MEMORY"));
            Error = ST_ERROR_NO_MEMORY;
        }
    }

    return(Error);
}


/*******************************************************************************
Name        : STLAYER_GetCapability()
Description : Gets the capabilities of the named Graphic HAL.
Parameters  : DeviceName IN: Device name, Capability IN: Pointer to
            an allocated capabilities structure to be fillled,
            OUT: The capability structure fields
Assumptions :
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_UNKNOWN_DEVICE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    U32             Index;

    Index = stlayer_IndexOfNamedLayer(DeviceName);
    if (Index == BAD_INDEX)
    {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_UNKNOWN_DEVICE"));
        return(ST_ERROR_UNKNOWN_DEVICE);
    }

    switch(stlayer_LayerArray[Index].LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetCapability(DeviceName,Capability);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_FILTER:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetCapability(DeviceName,Capability);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetCapability(DeviceName,Capability);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetCapability(DeviceName,Capability);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetCapability(DeviceName,Capability);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            LAYERVID_GetCapability(stlayer_LayerArray[Index].VideoHandle,
                            Capability);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_BAD_PARAMETER"));
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_GetInitAllocParams()
Description : Gets the allocation parameters needed for driver initialization.
Parameters  : LayerType IN: Type of the layer about which the allocation
        information is wanted, ViewPortsNumber IN: Maximum number of wanted
        viewports, AllocParams IN: Pointer to the allloc info structure to
        be filled, OUT: The structure fields containing the allocation params
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, STLAYER_ERROR_INVALID_LAYER_TYPE ,
                STLAYER_ERROR_USER_ALLOCATION_NOT_ALLOWED
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    switch(LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetInitAllocParams(LayerType,ViewPortsNumber,
                                                    Params);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetInitAllocParams(LayerType,ViewPortsNumber,
                                                    Params);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetInitAllocParams(LayerType,ViewPortsNumber,
                                                    Params);
            break;
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetInitAllocParams(LayerType,ViewPortsNumber,
                                                    Params);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetInitAllocParams(LayerType,ViewPortsNumber,
                                                    Params);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            /* not supported */
            break;
#endif
        default:
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    U32             Index,UsedVP;
    U32             j;
    BOOL            Found;

    Index = stlayer_IndexOfNamedLayer(DeviceName);
    if( Index == BAD_INDEX)
    {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_UNKNOWN_DEVICE"));
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    /* test opened connection */
    j = 0;
    Found = FALSE;
    do
    {
        if(stlayer_OpenArray[j].LayerDevice == &stlayer_LayerArray[Index])
        {
            if (!(TermParams_p->ForceTerminate))
            {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_OPEN_HANDLE"));
                return(ST_ERROR_OPEN_HANDLE);
            }
        }
        j ++;
    } while(j < (TOTAL_MAX_LAYER * MAX_OPEN));

    switch(stlayer_LayerArray[Index].LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_Term(DeviceName,TermParams_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error = LAYERGFX_Term(DeviceName,TermParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_Term(DeviceName,TermParams_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_Term(DeviceName,TermParams_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_Term(DeviceName,TermParams_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

#ifdef ST_OSLINUX
            Error = LAYERVID_Term(DeviceName, TermParams_p);
#else
            Error = LAYERVID_Term(stlayer_LayerArray[Index].VideoHandle,
                            TermParams_p);
#endif
        break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* API Handles management */
    if(!Error)
    {
        /* unrecord layer */
        strncpy(stlayer_LayerArray[Index].LayerName,"",sizeof(ST_DeviceName_t));/* eq free index */
        stlayer_LayerArray[Index].VideoHandle = 0;
        /* unrecord open */
        j = 0;
        do
        {
            if(stlayer_OpenArray[j].LayerDevice == &stlayer_LayerArray[Index])
            {
                stlayer_OpenArray[j].LayerDevice    = 0;
                stlayer_OpenArray[j].LowLevelHandle = 0;
                stlayer_OpenArray[j].Used           = 0;
            }
            j ++;
        } while(j < TOTAL_MAX_LAYER * MAX_OPEN);
        /* unrecord viewports */
        UsedVP = stlayer_CleanLayerViewports(&stlayer_LayerArray[Index]);
        if (UsedVP)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
             "STLAYER - API : Some Viewports were still open when terminate"));
        }

    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_Open
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_Open(const ST_DeviceName_t       DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *    Handle)
{
    ST_ErrorCode_t  Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    U32             Index,j;
    STLAYER_Handle_t LowLevelHandle;

    Index   = stlayer_IndexOfNamedLayer(DeviceName);
    if (Index == BAD_INDEX)
    {
       STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "STLAYER - API : ST_ERROR_UNKNOWN_DEVICE"));
        return(ST_ERROR_UNKNOWN_DEVICE);
    }
    switch(stlayer_LayerArray[Index].LayerType)
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_Open(DeviceName,Params,&LowLevelHandle);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
        case STLAYER_GAMMA_FILTER:
            Error = LAYERGFX_Open(DeviceName,Params,&LowLevelHandle);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_Open(DeviceName,Params,&LowLevelHandle);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_Open(DeviceName,Params,&LowLevelHandle);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_Open(DeviceName,Params,&LowLevelHandle);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            Error = ST_NO_ERROR;
            LowLevelHandle = stlayer_LayerArray[Index].VideoHandle; /* already */
            break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /* API Handles management */
    if(!Error)/* record layer handle */
    {
        j = stlayer_IndexOfFreeOpen();
        if(j != BAD_INDEX)
        {
            stlayer_OpenArray[j].LayerDevice    = &stlayer_LayerArray[Index];
            stlayer_OpenArray[j].LowLevelHandle = LowLevelHandle;
            stlayer_OpenArray[j].Used           = KEY_HANDLE_USED;
            *Handle = (STLAYER_Handle_t)(&stlayer_OpenArray[j]);
        }
        else
        {
            *Handle = 0;
            Error = ST_ERROR_NO_FREE_HANDLES;
        }
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_Close
Description :
Parameters  : Handle IN: Handle of the viewport.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_Close(STLAYER_Handle_t  Handle)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(Handle);
    switch(GetType(Handle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_Close(GetHandle(Handle));
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_FILTER:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_Close(GetHandle(Handle));
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_Close(GetHandle(Handle));
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_Close(GetHandle(Handle));
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_Close(GetHandle(Handle));
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    /* unrecord open */
    if(!Error)
    {
        ((stlayer_OpenElement*)(Handle))->LayerDevice    = 0;
        ((stlayer_OpenElement*)(Handle))->LowLevelHandle = 0;
        ((stlayer_OpenElement*)(Handle))->Used           = 0;
    }

    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetLayerParams()
Description : Gets the paramters of the named diplay layer.
Parameters  : Handle IN: Device name, Parameters IN: Pointer to an allocated
            layer parameters structure to be filled, OUT: The layer parameters
            structure fields
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetLayerParams(STLAYER_Handle_t  Handle,
                         STLAYER_LayerParams_t *  LayerParams_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(Handle);
    switch(GetType(Handle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            LAYERVID_GetLayerParams(GetHandle(Handle),LayerParams_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_SetLayerParams
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_SetLayerParams(STLAYER_Handle_t  Handle,
                         STLAYER_LayerParams_t *  LayerParams_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(Handle);
    switch(GetType(Handle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_SetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_SetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_SetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_SetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_SetLayerParams(GetHandle(Handle),LayerParams_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            LAYERVID_SetLayerParams(GetHandle(Handle),LayerParams_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_AdjustIORectangle()
Description : Adjusts the rectangles (if needed) to applicable values for the
                opening or modification of a viewport.
Parameters  :Handle IN : Handle of the layer,
            InputRectangle IN : pointer on an allocated STGXOBJ_Rectangle_t,
            OUT : Modified input rectangle if constraints need to do so.
            Val-uesremain unchanged otherwise.
        OutputRectangle IN : pointer on an allocated STGXOBJ_Rectangle_t
        OUT : Modified output rectangle if constraints need to do so.
        Values remain unchanged otherwise.
Assumptions :
Limitations :
Returns     : ST_ERROR_INVALID_HANDLE, STLAYER_SUCCESS_IORECTANGLES_ADJUSTED,
                STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t STLAYER_AdjustIORectangle(STLAYER_Handle_t      Handle,
                                         STGXOBJ_Rectangle_t * InputRectangle,
                                         STGXOBJ_Rectangle_t * OutputRectangle)
{
    ST_ErrorCode_t Error;

    TestHandle(Handle);
    switch(GetType(Handle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_AdjustIORectangle(GetHandle(Handle),
                                             InputRectangle, OutputRectangle);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_AdjustIORectangle(GetHandle(Handle),
                                             InputRectangle, OutputRectangle);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_AdjustIORectangle(GetHandle(Handle),
                                             InputRectangle, OutputRectangle);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_AdjustIORectangle(GetHandle(Handle),
                                             InputRectangle, OutputRectangle);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_AdjustIORectangle(GetHandle(Handle),
                                             InputRectangle, OutputRectangle);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetBitmapAllocParams
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetBitmapAllocParams(STLAYER_Handle_t      LayerHandle,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params1_p,
                                    STGXOBJ_BitmapAllocParams_t*  Params2_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetBitmapAllocParams(GetHandle(LayerHandle),
                                                  Bitmap_p,Params1_p,Params2_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetBitmapAllocParams(GetHandle(LayerHandle),
                                                  Bitmap_p,Params1_p,Params2_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetBitmapAllocParams(GetHandle(LayerHandle),
                                                  Bitmap_p,Params1_p,Params2_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetBitmapAllocParams(GetHandle(LayerHandle),
                                                  Bitmap_p,Params1_p,Params2_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetBitmapAllocParams(GetHandle(LayerHandle),
                                                  Bitmap_p,Params1_p,Params2_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_GetBitmapHeaderSize
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetBitmapHeaderSize(STLAYER_Handle_t  LayerHandle,
                                           STGXOBJ_Bitmap_t* Bitmap_p,
                                           U32 *             HeaderSize_p)

{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetBitmapHeaderSize(GetHandle(LayerHandle),
                                                Bitmap_p,HeaderSize_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetBitmapHeaderSize(GetHandle(LayerHandle),
                                                Bitmap_p,HeaderSize_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetBitmapHeaderSize(GetHandle(LayerHandle),
                                                Bitmap_p,HeaderSize_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetBitmapHeaderSize(GetHandle(LayerHandle),
                                                Bitmap_p,HeaderSize_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_GetBitmapHeaderSize(GetHandle(LayerHandle),
                                                Bitmap_p,HeaderSize_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        :STLAYER_GetPaletteAllocParams
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetPaletteAllocParams(STLAYER_Handle_t  LayerHandle,
                                        STGXOBJ_Palette_t* Palette_p,
                                        STGXOBJ_PaletteAllocParams_t* Params_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetPaletteAllocParams(GetHandle(LayerHandle),
                                                    Palette_p,Params_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
            Error = LAYERGFX_GetPaletteAllocParams(GetHandle(LayerHandle),
                                                    Palette_p,Params_p);
            break;
        case STLAYER_GAMMA_ALPHA:
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_GetPaletteAllocParams(GetHandle(LayerHandle),
                                                    Palette_p,Params_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_GetPaletteAllocParams(GetHandle(LayerHandle),
                                                    Palette_p,Params_p);
        case STLAYER_OMEGA1_STILL:
            break;
            /* no palette in still */
            break;
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_GetVTGName
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetVTGName(STLAYER_Handle_t         LayerHandle,
                                  ST_DeviceName_t * const  VTGName_p)

{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetVTGName(GetHandle(LayerHandle), VTGName_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetVTGName(GetHandle(LayerHandle), VTGName_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            break;
        case STLAYER_OMEGA1_CURSOR:
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1:
        case STLAYER_DISPLAYPIPE_VIDEO2:
        case STLAYER_DISPLAYPIPE_VIDEO3:
        case STLAYER_DISPLAYPIPE_VIDEO4 :


            LAYERVID_GetVTGName(GetHandle(LayerHandle), VTGName_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}


/*******************************************************************************
Name        : STLAYER_GetVTGParams
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_GetVTGParams(STLAYER_Handle_t            LayerHandle,
                                    STLAYER_VTGParams_t * const VTGParams_p)

{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_GetVTGParams(GetHandle(LayerHandle), VTGParams_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_GetVTGParams(GetHandle(LayerHandle), VTGParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            break;
        case STLAYER_OMEGA1_CURSOR:
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            LAYERVID_GetVTGParams(GetHandle(LayerHandle), VTGParams_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
} /* end of STLAYER_GetVTGParams() function */


/*******************************************************************************
Name        : STLAYER_UpdateFromMixer
Description :
Parameters  : Handle IN: Handle of the layer.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t STLAYER_UpdateFromMixer(STLAYER_Handle_t         LayerHandle,
                                       STLAYER_OutputParams_t * OutputParams_p)

{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    TestHandle(LayerHandle);
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            {
                STLAYER_OutputParams_t OutputParams;

                /* Need to pass the low level handles through Output params */
                OutputParams.UpdateReason       = OutputParams_p->UpdateReason;
                OutputParams.AspectRatio        = OutputParams_p->AspectRatio;
                OutputParams.ScanType           = OutputParams_p->ScanType;
                OutputParams.FrameRate          = OutputParams_p->FrameRate;
                OutputParams.Width              = OutputParams_p->Width;
                OutputParams.Height             = OutputParams_p->Height;
                OutputParams.XStart             = OutputParams_p->XStart;
                OutputParams.YStart             = OutputParams_p->YStart;
                OutputParams.XOffset            = OutputParams_p->XOffset;
                OutputParams.YOffset            = OutputParams_p->YOffset;
                strcpy(OutputParams.VTGName,OutputParams_p->VTGName);
                OutputParams.DeviceId           = OutputParams_p->DeviceId;
                OutputParams.DisplayEnable      = OutputParams_p->DisplayEnable;
                OutputParams.BackLayerHandle    = OutputParams_p->BackLayerHandle == 0 ?
                                                  0 : GetHandle(OutputParams_p->BackLayerHandle);
                OutputParams.FrontLayerHandle   = OutputParams_p->FrontLayerHandle == 0 ?
                                                  0 : GetHandle(OutputParams_p->FrontLayerHandle);
                OutputParams.DisplayHandle      = OutputParams_p->DisplayHandle;
                OutputParams.FrameBufferDisplayLatency     = OutputParams_p->FrameBufferDisplayLatency;
                OutputParams.FrameBufferHoldTime           = OutputParams_p->FrameBufferHoldTime;

                Error = LAYCOMPO_UpdateFromMixer(GetHandle(LayerHandle),&OutputParams);
                break;
            }

#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_FILTER:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_UpdateFromMixer(GetHandle(LayerHandle),
                        OutputParams_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_UpdateFromMixer(GetHandle(LayerHandle),
                        OutputParams_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_UpdateFromMixer(GetHandle(LayerHandle),
                        OutputParams_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error = LAYERSTILL_UpdateFromMixer(GetHandle(LayerHandle),
                        OutputParams_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            LAYERVID_UpdateFromMixer(GetHandle(LayerHandle),OutputParams_p);
            Error = ST_NO_ERROR;
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}

/*******************************************************************************
Name        : STLAYER_AdjustViewportParams()
Description : Give the adjusted parameters for the viewport according to the
              hardware limitations.
Parameters  : LayerHandle IN: Handle of the Layer, VPParam IN: Pointer to an
              allocated structure which contains the parameters.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_INVALID_HANDLE, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t STLAYER_AdjustViewPortParams(
                            STLAYER_Handle_t            LayerHandle,
                            STLAYER_ViewPortParams_t*   Params_p)
{
    STLAYER_Handle_t            LowLevelLayerHandle;
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

    LowLevelLayerHandle = ((stlayer_OpenElement*)(LayerHandle))->LowLevelHandle;
    switch(GetType(LayerHandle))
    {
#if defined LAYER_COMPO
        case STLAYER_COMPOSITOR:
            Error = LAYCOMPO_AdjustViewPortParams(LowLevelLayerHandle,Params_p);
            break;
#endif
#if defined GFX_GAMMA
        case STLAYER_GAMMA_CURSOR:
        case STLAYER_GAMMA_GDP:
        case STLAYER_GAMMA_GDPVBI:
        case STLAYER_GAMMA_BKL:
        case STLAYER_GAMMA_ALPHA:
            Error = LAYERGFX_AdjustViewPortParams(LowLevelLayerHandle,Params_p);
            break;
#endif
#if defined GFX_55XX
        case STLAYER_OMEGA1_OSD:
            Error = LAYEROSD_AdjustViewPortParams(LowLevelLayerHandle,Params_p);
            break;
        case STLAYER_OMEGA1_CURSOR:
            Error = LAYERSUB_AdjustViewPortParams(LowLevelLayerHandle,Params_p);
            break;
#if defined GFX_STILL
        case STLAYER_OMEGA1_STILL:
            Error=LAYERSTILL_AdjustViewPortParams(LowLevelLayerHandle,Params_p);
            break;
#endif
#endif
#ifdef VIDEO
        case STLAYER_OMEGA2_VIDEO1:
        case STLAYER_OMEGA2_VIDEO2:
        case STLAYER_7020_VIDEO1  :
        case STLAYER_7020_VIDEO2  :
        case STLAYER_OMEGA1_VIDEO :
        case STLAYER_SDDISPO2_VIDEO1 :
        case STLAYER_SDDISPO2_VIDEO2 :
        case STLAYER_HDDISPO2_VIDEO1 :
        case STLAYER_HDDISPO2_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO1 :
        case STLAYER_DISPLAYPIPE_VIDEO2 :
        case STLAYER_DISPLAYPIPE_VIDEO3 :
        case STLAYER_DISPLAYPIPE_VIDEO4 :

            Error = LAYERVID_AdjustViewportParams(LowLevelLayerHandle, Params_p);
            break;
#endif
        default:
            Error = ST_ERROR_INVALID_HANDLE;
            break;
    }
    return(Error);
}
