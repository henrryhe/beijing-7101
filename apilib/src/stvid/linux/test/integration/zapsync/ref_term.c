/* ************************************************************************* */
/* term functions                                                        */
/* ************************************************************************* */

#include "ref_sync.h"
#include "ref_term.h"

/* Extern Variables --------------------------------------------------------- */

extern STEVT_Handle_t                  EVT_Handle;
extern STAUD_Handle_t                  AUD_Handle;
extern STCLKRV_Handle_t                CLKRV_Handle;
extern STDENC_Handle_t                 DENC_Handle;
extern STVOUT_Handle_t                 VOUT_Handle;
extern STVOUT_Handle_t                 VOUT_Handle_HD;
extern STVTG_Handle_t                  VTG_Handle;
extern STVMIX_Handle_t                 VMIX_Handle;
extern STLAYER_Handle_t                LAYER_Handle;
extern STVID_Handle_t                  VID_Handle;
extern int HD;

/*******************************************************************************
Name        : PTI_Term
Description : Terminate the STPTI driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void PTI_Term(void)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STPTI_TermParams_t  TermParams;
    ST_DeviceName_t        DeviceName;

    TermParams.ForceTermination = TRUE;

printf("file:%s line:%d\n",__FILE__,__LINE__);
    
    if (EVT_Handle != 0)
    {
printf("file:%s line:%d\n",__FILE__,__LINE__);
        ErrorCode = STEVT_UnsubscribeDeviceEvent(EVT_Handle, 
                                               STCLKRV_DEVICE_NAME,
                                               (STEVT_EventConstant_t) STCLKRV_PCR_VALID_EVT);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV errors Unsubscription failed. Error 0x%x ! ", ErrorCode);
        }
printf("file:%s line:%d\n",__FILE__,__LINE__);

        ErrorCode = STEVT_UnsubscribeDeviceEvent(EVT_Handle, 
                                               STCLKRV_DEVICE_NAME,
                                               (STEVT_EventConstant_t) STCLKRV_PCR_DISCONTINUITY_EVT);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf("STCLKRV errors Unsubscription failed. Error 0x%x ! ", ErrorCode);
        }

    }
printf("file:%s line:%d\n",__FILE__,__LINE__);

    strcpy((char *)DeviceName, STPTI_DEVICE_NAME);
printf("file:%s line:%d\n",__FILE__,__LINE__);
    
    ErrorCode = STPTI_Term(STPTI_DEVICE_NAME, &TermParams);
printf("file:%s line:%d\n",__FILE__,__LINE__);
    if (ErrorCode != ST_NO_ERROR)
    {
        printf("%s term. error 0x%x ! \n", STPTI_DEVICE_NAME, ErrorCode);
    }
    else
    {
        printf("%s Terminated\n", STPTI_DEVICE_NAME);
    }
    
}

/*******************************************************************************
Name        : MERGE_Term
Description : Initialise the STMERGE driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void MERGE_Term(void)
{
    ST_ErrorCode_t                  ErrorCode;

    ErrorCode = STMERGE_Term("MERGE0", NULL );
    printf("STMERGE_Term Done\n");

} /* end MERGE_Term() */

/*******************************************************************************
Name        : EVT_Term
Description : Initialise the STEVT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void EVT_Term(void)
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STEVT_TermParams_t TermParams;

    STEVT_Close(EVT_Handle);
    
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STEVT_Term(STEVT_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        printf("STEVT_Term(%s): Warning !! Still open handle\n", STEVT_DEVICE_NAME);
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STEVT_Term(STEVT_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STEVT_Term(%s): ok\n", STEVT_DEVICE_NAME);
    }
    else
    {
        printf("STEVT_Term(%s): error=0x%0x !\n", STEVT_DEVICE_NAME, ErrorCode);
    }
} /* end EVT_Term() */

/*******************************************************************************
Name        : VIDTEST_Term
Description : Terminate the VIDTEST driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void VIDTEST_Term(void)
{
    ST_ErrorCode_t         ErrorCode = ST_NO_ERROR;
    STVIDTEST_TermParams_t TermParams;
    ST_DeviceName_t        DeviceName;

    strcpy((char *)DeviceName, STVIDTEST_DEVICE_NAME);

    TermParams.ForceTerminate = FALSE;
    ErrorCode = STVIDTEST_Term (DeviceName, &TermParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STVIDTEST_Term(%s): ok\n", DeviceName);
    }
    else
    {
        printf("STVIDTEST_Term(%s): error=0x%0x !\n", DeviceName, ErrorCode);
    }
}

/*******************************************************************************
Name        : CLKRV_Term
Description : Terminate the CLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void CLKRV_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STCLKRV_TermParams_t TermParams;
    ST_DeviceName_t      DeviceName;
printf("file:%s line:%d\n",__FILE__,__LINE__);

    strcpy((char *)DeviceName, STCLKRV_DEVICE_NAME);
printf("file:%s line:%d\n",__FILE__,__LINE__);

    STCLKRV_Close(CLKRV_Handle);
printf("file:%s line:%d\n",__FILE__,__LINE__);
    TermParams.ForceTerminate = FALSE;
printf("file:%s line:%d\n",__FILE__,__LINE__);
    ErrorCode = STCLKRV_Term(DeviceName, &TermParams);
printf("file:%s line:%d\n",__FILE__,__LINE__);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
printf("file:%s line:%d\n",__FILE__,__LINE__);
        printf("STCLKRV_Term(%s): Warning !! Still open handle\n", STCLKRV_DEVICE_NAME);
        TermParams.ForceTerminate = true;
        
printf("file:%s line:%d\n",__FILE__,__LINE__);
        ErrorCode = STCLKRV_Term(STCLKRV_DEVICE_NAME, &TermParams);
printf("file:%s line:%d\n",__FILE__,__LINE__);
    }
    
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STCLKRV_Term(%s): ok\n", STCLKRV_DEVICE_NAME);
    }
    else
    {
        printf("STCLKRV_Term(%s): error 0x%0x\n", STCLKRV_DEVICE_NAME, ErrorCode);
    }
} /* end CLKRV_Term() */

/*******************************************************************************
Name        : AUD_Term
Description : Terminate the STAUDLT/LX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void AUD_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STAUD_TermParams_t   TermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STAUD_DEVICE_NAME);

    STAUD_Close(AUD_Handle);
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STAUD_Term(DeviceName, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        printf("STAUD_Term(%s): Warning !! Still open handle\n", STAUD_DEVICE_NAME);
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STAUD_Term(STAUD_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STAUD_Term(%s): ok\n", STAUD_DEVICE_NAME);
    }
    else
    {
        printf("STAUD_Term(%s): error 0x%0x\n", STAUD_DEVICE_NAME, ErrorCode);
    }
} /* end AUD_Term() */

/*******************************************************************************
Name        : DENC_Term
Description : Terminate the STDENC driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void DENC_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STDENC_TermParams_t  DENCTermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STDENC_DEVICE_NAME);

    ErrorCode = STDENC_Close(DENC_Handle);

    DENCTermParams.ForceTerminate = FALSE;
    if (STDENC_Term(DeviceName, &DENCTermParams) != ST_NO_ERROR)
    {
        printf( "STDENC_Term  %s Failed !\n", STDENC_DEVICE_NAME);
    }
    else
    {
        printf( "STDENC_Term  %s Done\n", STDENC_DEVICE_NAME);
    }
}

/*******************************************************************************
Name        : VTG_Term
Description : Terminate the STVTG driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void VTG_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STVTG_TermParams_t   VTGTermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STVTG_DEVICE_NAME);

    ErrorCode = STVTG_Close(VTG_Handle);

    VTGTermParams.ForceTerminate = FALSE;
    if (STVTG_Term(DeviceName, &VTGTermParams) != ST_NO_ERROR)
    {
        printf( "STVTG_Term  %s Failed !\n", STVTG_DEVICE_NAME);
    }
    else
    {
        printf( "STVTG_Term  %s Done\n", STVTG_DEVICE_NAME);
    }
}

/*******************************************************************************
Name        : VOUT_Term
Description : Terminate the STOUT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void VOUT_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STVOUT_TermParams_t  VOUTTermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STVOUT_DEVICE_NAME);

    if (HD != 0)
    {
        if ((ErrorCode = STVOUT_Disable( VOUT_Handle_HD )) != ST_NO_ERROR)
            printf("STVOUT_Disable(HD) fail, ErrCode 0x%x", ErrorCode);
        if ((ErrorCode = STVOUT_Close(VOUT_Handle_HD)) != ST_NO_ERROR)
            printf("STVOUT_close() fail, ErrCode 0x%x", ErrorCode);

        VOUTTermParams.ForceTerminate = TRUE;
        ErrorCode = STVOUT_Term(STVOUT_DEVICE_NAME_HD, &VOUTTermParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            printf( "STVOUT_Term(HD)  %s Failed !\n", STVOUT_DEVICE_NAME_HD);
        }
        else
        {
            printf( "STVOUT_Term(HD)  %s Done\n", STVOUT_DEVICE_NAME_HD);
        }
    }

    if ((ErrorCode = STVOUT_Disable( VOUT_Handle )) != ST_NO_ERROR)
        printf("STVOUT_Disable() fail, ErrCode 0x%x", ErrorCode);
    if ((ErrorCode = STVOUT_Close(VOUT_Handle)) != ST_NO_ERROR)
        printf("STVOUT_close() fail, ErrCode 0x%x", ErrorCode);

    VOUTTermParams.ForceTerminate = FALSE;
    if (STVOUT_Term(DeviceName, &VOUTTermParams) != ST_NO_ERROR)
    {
        printf( "STVOUT_Term  %s Failed !\n", STVOUT_DEVICE_NAME);
    }
    else
    {
        printf( "STVOUT_Term  %s Done\n", STVOUT_DEVICE_NAME);
    }
}

/*******************************************************************************
Name        : VMIX_Term
Description : Terminate the STVMIX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void VMIX_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STVMIX_TermParams_t  VMIXTermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STVMIX_DEVICE_NAME);
    
    ErrorCode = STVMIX_Close(VMIX_Handle);

    VMIXTermParams.ForceTerminate = FALSE;
    if (STVMIX_Term(DeviceName, &VMIXTermParams) != ST_NO_ERROR)
    {
        printf( "STVMIX_Term  %s Failed !\n", STVMIX_DEVICE_NAME);
    }
    else
    {
        printf( "STVMIX_Term  %s Done\n", STVMIX_DEVICE_NAME);
    }
}

/*******************************************************************************
Name        : LAYER_Term
Description : Terminate the STLAYER driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void LAYER_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STLAYER_TermParams_t LAYERTermParams;
    ST_DeviceName_t      DeviceName;

    strcpy((char *)DeviceName, STLAYER_DEVICE_NAME);
    
    ErrorCode = STLAYER_Close(LAYER_Handle);
    
    LAYERTermParams.ForceTerminate = FALSE;
    if ((ErrorCode = STLAYER_Term(DeviceName, &LAYERTermParams)) != ST_NO_ERROR)
    {
        printf( "STLAYER_Term  %s Failed ! Error:%x\n", STLAYER_DEVICE_NAME, ErrorCode);
    }
    else
    {
        printf( "STLAYER_Term  %s Done\n", STLAYER_DEVICE_NAME);
    }
    
} /* end LAYER_Term */

/*******************************************************************************
Name        : VID_Term
Description : Terminate the STVID driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void VID_Term(void)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STVID_TermParams_t      VIDTermParams;
    ST_DeviceName_t         DeviceName;

    strcpy((char *)DeviceName, STVID_DEVICE_NAME);

    ErrorCode = STVID_Close(VID_Handle);

    VIDTermParams.ForceTerminate = FALSE;
    if (STVID_Term(DeviceName, &VIDTermParams) != ST_NO_ERROR)
    {
        printf( "STVID_Term  %s Failed !\n", STVID_DEVICE_NAME);
    }
    else
    {
        printf( "STVID_Term  %s Done\n", STVID_DEVICE_NAME);
    }
}
