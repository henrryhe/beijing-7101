/*******************************************************************************

File name   : hxvp_main.c

Description : XVP source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "hv_vdplay.h"
#include "hxvp_main.h"
#include "hxvp_fgt.h"

#ifdef ST_OSLINUX
    #include "stlayer_core.h"
#else
    /*#define STTBX_REPORT*/ 
	#include "sttbx.h"
#endif

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    /*************************      Video LAYER parameters      *************************/
    STLAYER_Handle_t            AssociatedLayerHandle;          /* Associated layer handle, to retrieve its characteristics  */
    BOOL                        Opened;                         /* Viewport's status (Open/Close)                            */
    U8                          ViewportId;                     /* Viewport Id                                               */
    U32                         SourceNumber;                   /* Source number linked to it                                */
} HALLAYER_ViewportProperties_t;
                                    
/* Private Function prototypes ---------------------------------------------- */

/*******************************************************
 * XVP_Init                                            *
 *******************************************************/

ST_ErrorCode_t  XVP_Init(   const STLAYER_ViewPortHandle_t      VPHandle,
                            STLAYER_ProcessId_t                 ProcessId,
                            STLAYER_XVPHandle_t                 *XVPHandle_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    HALLAYER_LayerProperties_t      *LayerProperties_p;
    HALLAYER_ViewportProperties_t   *ViewportProperties_p;
    XVP_CommonData_t                *XVPData_p;    
    
    /* get data from handle */
    ViewportProperties_p = (HALLAYER_ViewportProperties_t *)VPHandle;
    
    /* allocate memory */
    LayerProperties_p = (HALLAYER_LayerProperties_t *)(ViewportProperties_p->AssociatedLayerHandle);
    if (LayerProperties_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }    
    XVPData_p = (void*)STOS_MemoryAllocate(   LayerProperties_p->CPUPartition_p,
                                              sizeof(XVP_CommonData_t));        
    if (XVPData_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    XVPData_p->CPUPartition_p       = LayerProperties_p->CPUPartition_p;
    XVPData_p->NCachePartition_p    = LayerProperties_p->NCachePartition_p;
    XVPData_p->VPHandle             = VPHandle;
    XVPData_p->LayerHandle          = ViewportProperties_p->AssociatedLayerHandle;
    *XVPHandle_p = XVPData_p;
    
    /* init requested process */
    switch (ProcessId)
    {
#ifdef ST_XVP_ENABLE_FGT         
    case STLAYER_PROCESSID_FGT :
        XVPData_p->PluginFunc_p = &FGT_PluginFunc;
        break;
#endif
        
    case STLAYER_PROCESSID_NONE :
    default :
        ErrorCode = ST_ERROR_BAD_PARAMETER;
        break;
    }
    
    if (ErrorCode == ST_NO_ERROR)
    {
        XVPData_p->PluginFunc_p->Init(*XVPHandle_p, &(XVPData_p->ProcessHandle));
    }
        
    return ErrorCode;
}


/*******************************************************
 * XVP_Term                                            *
 *******************************************************/

ST_ErrorCode_t  XVP_Term(const STLAYER_XVPHandle_t  XVPHandle)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    XVP_CommonData_t                *XVPData_p;
        
    /* Plugin Termination */    
    XVPData_p = (XVP_CommonData_t*)XVPHandle;
    XVPData_p->PluginFunc_p->Term(XVPData_p->ProcessHandle);
    
    /* memory desallocation */   
    STOS_MemoryDeallocate(XVPData_p->CPUPartition_p, XVPData_p);
  
    return ErrorCode;
}

/*******************************************************
 * XVP_SetParam                                        *
 *******************************************************/

ST_ErrorCode_t  XVP_SetParam(   const STLAYER_XVPHandle_t           XVPHandle,
                                STLAYER_XVPParam_t                  *Param_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    XVP_CommonData_t                *XVPData_p;

    XVPData_p = (XVP_CommonData_t*)XVPHandle;
    XVPData_p->PluginFunc_p->SetParam(XVPData_p->ProcessHandle, Param_p);    
    return ErrorCode;
}

/*******************************************************
 * XVP_SetExtraParam                                   *
 *******************************************************/

ST_ErrorCode_t XVP_SetExtraParam(   const STLAYER_XVPHandle_t       XVPHandle,
                                    STLAYER_XVPExtraParam_t         *ExtraParam_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    XVP_CommonData_t                *XVPData_p;

    XVPData_p = (XVP_CommonData_t*)XVPHandle;
    XVPData_p->PluginFunc_p->SetExtraParam(XVPData_p->ProcessHandle, ExtraParam_p);
    
    return ErrorCode;
}

/*******************************************************
 * XVP_Compute                                         *
 *******************************************************/

ST_ErrorCode_t XVP_Compute(const STLAYER_XVPHandle_t  XVPHandle)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    XVP_CommonData_t                *XVPData_p;
        
    /* Plugin Termination */    
    XVPData_p = (XVP_CommonData_t*)XVPHandle;
    XVPData_p->PluginFunc_p->Compute(XVPData_p->ProcessHandle);
    
    return ErrorCode;
}
