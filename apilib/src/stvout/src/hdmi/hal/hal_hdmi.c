/*******************************************************************************
File name   : hal_hdmi.c

Description : Init/Open/Close/Term for hdmi ip

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
3 Apr 2004         Created                                          AC

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "hal_hdmi.h"
#include "hdmi_ip.h"
#include "stpio.h"

#include "sttbx.h"                       /* Dependencies */


/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define HAL_VALID_HANDLE       0x0175175f

/* Private variables (static) --------------------------------------------- */
#ifdef WA_GNBvd56512
 static const U32 HAL_AudioRegeneration[SAMPLING_FREQ_LAST][PIXEL_CLOCK_LAST] =
{
    /*           Audio clock 32 KHz for pixel clock in MHz               */
	/*  R= T_Vsyn/T_ACR= (1/Vsync_Freq)/(N/128*Fs)= 128*F_s/N*Vsync_Freq */
	/*  R= 128*32000/6206*60 = 11.0001074    OR  R= 128*32000/4096*50 = 20*/
    /*  25.2/1.001   25.2   27    27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 4576         ,4096,  4096 ,  4096 ,   4096 ,    4096,    11648,       4096,    11648,        4096},

    /*           Audio clock 44.1 KHz for pixel clock in MHz             */
    /*  R= 128*44100/9408*60 = 10     OR  R= 128*44100/9408*50 = 12      */
	/*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 9408            ,9408 , 9408,  9408,    9408,    9408,     18816 ,      9408,   9408,         9408 },

    /*           Audio clock 48 KHz for pixel clock in MHz             */
    /*  R= 128*48000/10240*60 = 10     OR  R= 128*48000/10240*50 = 12      */
	/*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 10240,        10240, 10240,  10240,   10240,     10240,       10240,  10240,   10240,         10240}
};
#else
static const U32 HAL_AudioRegeneration[SAMPLING_FREQ_LAST][PIXEL_CLOCK_LAST] =
{
    /*           Audio clock 32 KHz for pixel clock in MHz               */
    /*  25.2/1.001   25.2   27    27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 4576         ,4096,  4096 ,  4096 ,   4096 ,    4096,    11648,       4096,    11648,        4096},

    /*           Audio clock 44.1 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 7007           ,6272 , 6272,  6272,    6272,    6272,     17836 ,      6272,   8918,         6272 },

    /*           Audio clock 48 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    { 6864,          6144, 6144,    6144,   6144,       6144,       11648,  6144,   5824,         6144}
};
#endif
#if (!defined (WA_GNBvd44290)&&!defined(WA_GNBvd54182))
static const HAL_ACR_t  HAL_ACR_Default [SAMPLING_FREQ_LAST][PIXEL_CLOCK_LAST] =
{
    /*           Audio clock 32 KHz for pixel clock in MHz               */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{0,0},{0,0},{4576 , 28125}, {4576 , 28125},{0,0},{0,0},{11648 , 210937}, {4096 , 74250},{0,0},{0,0}},
    /*           Audio clock 44.1 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{0,0},{0,0},{6272 , 30000}, {6272 , 30030},{0,0},{0,0},{17836 , 234375}, {6272 , 30000},{0,0},{0,0}},
    /*           Audio clock 48 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{0,0},{0,0},{6144 , 27000}, {6144 , 27027},{0,0},{0,0},{11648 , 140625}, {6144 , 74250},{0,0},{0,0}}
};
#elif defined (WA_GNBvd44290)
/* For WA GNBvd44290 take higher N values to save CPU time */
static const HAL_ACR_t HAL_ACR_WA_GNBvd44290[SAMPLING_FREQ_LAST][PIXEL_CLOCK_LAST] =
{
    /*           Audio clock 32 KHz for pixel clock in MHz               */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{13600,83588},{13600,83671},{13600 , 89648}, {13600 , 89738},{0,0},{0,0},{13600, 246287}, {13600 , 246533},{0,0},{0,0}},
    /*           Audio clock 44.1 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{18800,83845},{18800,83928},{18800 , 89923}, {18800, 90013}, {0,0},{0,0},{18800, 247042}, {18800 , 247289},{0,0},{0,0}},
    /*           Audio clock 48 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{20470,83875},{20470,83958},{20470 , 89956}, {20470, 90046},{0,0},{0,0}, {20470 , 247132}, {20470, 247379},{0,0},{0,0}}
};
#else
/* For WA GNBvd44290 take higher N values to save CPU time */
static const HAL_ACR_t HAL_ACR_WA_GNBvd54182[SAMPLING_FREQ_LAST][PIXEL_CLOCK_LAST] =
{
    /*           Audio clock 32 KHz for pixel clock in MHz               */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{12288,75525},{12288,75600},{12288 , 81000}, {12288 , 81081},{0,0},{0,0},{12288, 222525}, {12288 , 222750},{0,0},{0,0}},
    /*           Audio clock 44.1 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27   27*1.001    54     54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{16934,75523},{16934,75598},{16934 , 80998}, {16934, 81079}, {0,0},{0,0},{16934, 222519}, {16934 , 222744},{0,0},{0,0}},
    /*           Audio clock 48 KHz for pixel clock in MHz             */
    /*  25.2/1.001   25.2   27       27*1.001          54        54*1.001  74.25/1.001  74.25   148.5/1.001    148.5 */
    {{18432,75525},{18432,75600},{18432 , 81000}, {18432, 81081},{0,0},{0,0}, {18432 , 222525}, {18432, 222750},{0,0},{0,0}}
};
#endif

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */
/* Private Functins-------------------------------------------------------- */
/* Exported Functions ----------------------------------------------------- */

/*******************************************************************************
Name        : HAL_Init
Description : Initialize HAL HDMI/DVI Cell
Parameters  : Hal Hdmi handle, InitParams(pointer)
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*********************************************************************************/
ST_ErrorCode_t HAL_Init(HALHDMI_InitParams_t* const HDMIInitParams_p, HAL_Handle_t*  const Handle_p)
{
    HALHDMI_Properties_t *  HALHDMI_Data_p;
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;

    /* Allocate a HALHDMI structure */
    HALHDMI_Data_p = (HALHDMI_Properties_t *) memory_allocate(HDMIInitParams_p->CPUPartition_p, sizeof(HALHDMI_Properties_t));

    if (HALHDMI_Data_p == NULL)    {

        return(ST_ERROR_NO_MEMORY);
    }

    /* Allocation succeeded: make handle point on structure */
     *Handle_p = (HAL_Handle_t *) HALHDMI_Data_p;
     HALHDMI_Data_p->ValidityCheck = HAL_VALID_HANDLE;

    /* Save parameters for the hal layer*/
    HALHDMI_Data_p->CPUPartition_p        = HDMIInitParams_p->CPUPartition_p;
    HALHDMI_Data_p->DeviceType            = HDMIInitParams_p->DeviceType;
    HALHDMI_Data_p->OutputType            = HDMIInitParams_p->OutputType ;
    HALHDMI_Data_p->DeviceBaseAddress_p   = HDMIInitParams_p->DeviceBaseAddress_p;
    HALHDMI_Data_p->RegisterBaseAddress_p = HDMIInitParams_p->RegisterBaseAddress_p;
    HALHDMI_Data_p->SecondBaseAddress_p   = HDMIInitParams_p->SecondBaseAddress_p;
    HALHDMI_Data_p->HPD_Bit       = HDMIInitParams_p->HPD_Bit;
    strncpy(HALHDMI_Data_p->PIODevice, HDMIInitParams_p->PIODevice,sizeof(HALHDMI_Data_p->PIODevice)-1);
    HALHDMI_Data_p->HSyncActivePolarity = HDMIInitParams_p->HSyncActivePolarity;
    HALHDMI_Data_p->VSyncActivePolarity = HDMIInitParams_p->VSyncActivePolarity;
    HALHDMI_Data_p->IsHPDInversed = HDMIInitParams_p->IsHPDInversed;
    ErrorCode = Init(*Handle_p);
    if (ErrorCode!=ST_NO_ERROR)
    {
        HAL_Term(*Handle_p);
    }
    return(ErrorCode);
} /* end of HAL_Init() function */
/*******************************************************************************
Name        : HAL_Term
Description : Terminate hdmi HAL
Parameters  : Hal Hdmi handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t HAL_Term(HAL_Handle_t const Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t * HALHDMI_Data_p;

    /* Find structure */
    HALHDMI_Data_p = (HALHDMI_Properties_t *) Handle;

    if (HALHDMI_Data_p->ValidityCheck != HAL_VALID_HANDLE)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

     /* De-validate structure */
     HALHDMI_Data_p->ValidityCheck = 0;

     ErrorCode = Term(Handle);

     /* Disable all interrupts*/
     HAL_SetInterruptMask(Handle, HDMI_ITX_DISABLE_ALL);

     /* Clear all interrupts */
     HAL_ClearInterrupt(Handle,HDMI_INTERRUPT_CLEAR_ALL);

     /* De-allocate last: data inside cannot be used after ! */
     memory_deallocate(HALHDMI_Data_p->CPUPartition_p, (void *) HALHDMI_Data_p);

     return(ErrorCode);
} /* end of HAL_Term() function */

/*******************************************************************************
Name        : HAL_Close
Description : Close opened Handle to HDMI/DVI Cell
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HAL_Close( HAL_Handle_t const Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    UNUSED_PARAMETER(Handle);
    return(ErrorCode);
} /* end of HAL_Close() function */
/*******************************************************************************
Name        : HAL_Open
Description : Open Handle to HDMI/DVI Cell
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HAL_Open( HAL_Handle_t const Handle)
{
     ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
     UNUSED_PARAMETER(Handle);
     return(ErrorCode);
} /* end of HAL_Open() function */
/*******************************************************************************
Name        : HAL_Enable
Description : Enable DVI/HDMI interface
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HAL_Enable(HAL_Handle_t const Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t *Data_p;

    if (!(IsHalValidHandle(Handle)))
    {
       return(ST_ERROR_INVALID_HANDLE);
    }

    Data_p = (HALHDMI_Properties_t *) Handle;
    switch (Data_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13:
             ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
             break;
        case STVOUT_DEVICE_TYPE_7710 : /* no break*/
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
             /* Reset the HDMI ip when enabling the interface*/
             HAL_ResetEnable(Handle);
             /* Enbale the DVI/HDMI interface */
             HAL_ControlInterface(Handle, TRUE);
             break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
             break;
    }
    return(ErrorCode);

} /* end of HAL_Enable()*/
/*******************************************************************************
Name        : HAL_Disable
Description : Disable DVI/HDMI interface
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : in params: HAL handle
              ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t HAL_Disable (HAL_Handle_t const Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t *Data_p;

    if (!(IsHalValidHandle(Handle)))
    {
       return(ST_ERROR_INVALID_HANDLE);
    }
    Data_p = (HALHDMI_Properties_t *) Handle;

    switch (Data_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13:
             ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
             break;
        case STVOUT_DEVICE_TYPE_7710 : /* no break */
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
             /* Reset the HDMI ip when disabling the interface */
             HAL_ResetDisable(Handle);
             /* Disable the DVI/HDMI interface*/
             HAL_ControlInterface(Handle,FALSE);
             break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
             break;
    }
    return(ErrorCode);
} /* end of HAL_Disable()*/
/*******************************************************************************
Name        : HAL_IsReceiverConnected
Description : Check if the receiver was connected.
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     : TRUE : The receiver was connected.
              FALSE : The receiver was not.
*******************************************************************************/
BOOL  HAL_IsReceiverConnected (HAL_Handle_t const Handle)
{
    HALHDMI_Properties_t *Data_p= (HALHDMI_Properties_t *) Handle;
    BOOL ReceiverConnected=FALSE;

    switch (Data_p->DeviceType)
    {

        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13:
             break;
        case STVOUT_DEVICE_TYPE_7710 : /* no break */
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
             /* Get the HPD status */
             HAL_GetHPDStatus(Handle, &ReceiverConnected);

             break;
        default :
             break;
    }

    return(ReceiverConnected);

} /* end of HAL_IsReceiverConnected() */
/*******************************************************************************
Name        : Hal_SendDefaultData
Description : Send default data
Parameters  : Hal Hdmi handle, U8 channel 0,1 and 2
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t  Hal_SendDefaultData (HAL_Handle_t const Handle, U8 Chl0, U8 Chl1, U8 Chl2)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t *Data_p;

    if (!(IsHalValidHandle(Handle)))
    {
       return(ST_ERROR_INVALID_HANDLE);
    }
    Data_p = (HALHDMI_Properties_t *) Handle;

    switch (Data_p->DeviceType)
    {

        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13:
             break;
        case STVOUT_DEVICE_TYPE_7710 : /* no break */
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
             /* Set Dafault data on Channel0, 1, 2*/
             HAL_SetDefaultDataChannel(Handle, Chl0, Chl1, Chl2);
             break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
             break;
    }
    return(ErrorCode);
} /* end of Hal_SendDefaultData() */
/*******************************************************************************
Name        : HAL_SetAudioNValue
Description : Set Audio regeneration value
Parameters  : Hal Hdmi handle, Pixel clock and Sampling Frequency.
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t  HAL_SetAudioNValue (HAL_Handle_t const Handle, HAL_PixelClockFreq_t PixelFreq,
                                       HAL_AudioSamplingFreq_t SamplingFreq, HAL_VsyncFreq_t VSyncFreq)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t *Data_p;
    U32 AUDIO_N;

    if (!(IsHalValidHandle(Handle)))
    {
       return(ST_ERROR_INVALID_HANDLE);
    }
    Data_p = (HALHDMI_Properties_t *) Handle;

    switch (Data_p->DeviceType)
    {

        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13: /* no break */
        case STVOUT_DEVICE_TYPE_7710 :
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
             /* Set channel status bits*/
			 HAL_SetChannelStatusBit(Handle,SamplingFreq);

             /* Set the Audio regeneration value it depends on Pixel and Sampling frequencies*/
			 AUDIO_N = HAL_AudioRegeneration[SamplingFreq][PixelFreq];
#ifdef WA_GNBvd56512
             if ((SamplingFreq == SAMPLING_FREQ_32_KHZ)&& (VSyncFreq == VSYNC_FREQ_60))
             {
                AUDIO_N +=2110;
             }
#else
             UNUSED_PARAMETER(VSyncFreq);
#endif
             HAL_SetAudioRegeneration(Handle, AUDIO_N);

             break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
             break;
    }
    return(ErrorCode);
} /* end of HAL_SetAudioNValue() */
/*******************************************************************************
Name        : HAL_SetACRPacket
Description : Set N and CTS value for ACR packet.
Parameters  : Hal Hdmi handle
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t  HAL_SetACRPacket (HAL_Handle_t const Handle, const HAL_PixelClockFreq_t EnumPixClock,
                          const HAL_AudioSamplingFreq_t   EnumAudioFreq,HAL_ACR_t * ACR)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_Properties_t *Data_p;
    HAL_ACR_t  TempACR ={0,0};

    if (!(IsHalValidHandle(Handle)))
    {
       return(ST_ERROR_INVALID_HANDLE);
    }
    Data_p = (HALHDMI_Properties_t *) Handle;

    switch (Data_p->DeviceType)
    {

        case STVOUT_DEVICE_TYPE_DENC : /*no break*/
        case STVOUT_DEVICE_TYPE_7015 : /*no break*/
        case STVOUT_DEVICE_TYPE_GX1  : /*no break*/
        case STVOUT_DEVICE_TYPE_7020 : /*no break*/
        case STVOUT_DEVICE_TYPE_5528 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629 : /*no break*/
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT: /*no break*/
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED:  /*no break*/
        case STVOUT_DEVICE_TYPE_V13: /* no break */
              break;
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
        case STVOUT_DEVICE_TYPE_7710 :
             if ((EnumAudioFreq<3 )&&(EnumPixClock <10))
             {
               #if (!defined (WA_GNBvd44290)&&!defined(WA_GNBvd54182))
                  TempACR = HAL_ACR_Default[EnumAudioFreq][EnumPixClock];
               #else /* WA_GNBvd44290 */
                #ifdef WA_GNBvd44290
                TempACR = HAL_ACR_WA_GNBvd44290[EnumAudioFreq][EnumPixClock];
                #else
                TempACR = HAL_ACR_WA_GNBvd54182[EnumAudioFreq][EnumPixClock];
                #endif
               #endif

              }
              ACR->N =  TempACR.N;
              ACR->CTS = TempACR.CTS;
              break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
             break;
    }
    return(ErrorCode);
} /* end of HAL_SetACRPacket */

/* end of Hal_hdmi.c file */
/* ------------------------------- End of file ---------------------------- */
