/*******************************************************************************

File name   : vout_drv.c

Description : vout module commands

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
27 Jun 2001        Created                                          JG
13 Sep 2001        Add support of ST40GX1                           HSdLM
06 Dec 2001        Add Device Type for Digital output of STi5514    HSdLM
23 Apr 2002        New DeviceType for STi7020                       HSdLM
02 Jul 2003        Add support of STi5528                           HSdLM
21 Aug 2003        New s/w hal design                               HSdLM
02 Jun 2004        Add Support for 7710                             AC
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif


#include "stvout.h"
#include "vout_drv.h"
#include "denc_out.h"
#include "vout_awg.h"

#ifdef STVOUT_SDDIG
#include "sd_dig.h"
#endif /* STVOUT_SDDIG */

#ifdef STVOUT_HDOUT
#include "analog.h"
#include "digital.h"
#endif /* STVOUT_HDOUT */

#ifdef STVOUT_SVM
#include "svm.h"
#endif /* STVOUT_SVM */

#ifdef STVOUT_HDMI
#include "hdmi_api.h"
#ifdef STVOUT_CEC_PIO_COMPARE
#include "cec.h"
#endif
#endif /* STVOUT_HDMI*/

#ifdef STVOUT_HDDACS
#include "hd_reg.h"
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#if defined (ST_7710)|| defined (ST_7100) || defined (ST_7109)
#define DSPCFG_DAC_ENABLE_SD 0x00000080  /* Enable SD DAC out on 7710 cut 3.0 */
#else
#define DSPCFG_DAC_ENABLE_SD   0x00020000  /* Enable SD DAC out, 7020 only */
#endif
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define VOUT_MAIN_GLUE_OFFSET 0x400
#define VOUT_AUX_GLUE_OFFSET  0x500
#define SD_DAC_DISABLE        0x200
#define HD_DAC_DISABLE        0x800
#define AWG_BASE_ADDRESS      ST7200_HD_TVOUT_HDF_BASE_ADDRESS
#define AWG_RAM_OFFSET        0x300
#define AWG_CTRL              0x0
#endif

/* Private Variables (static)------------------------------------------------ */
BOOL stvout_MaxDynState = FALSE;
#if defined (ST_7100) || defined (ST_7109)
S16 DefaultCoeffLuma[10] = {0x03, 0x01,(S16)0xfff7, (S16)0xfffe, 0x1e, 0x05,0x1a4, (S16)0xfff9, 0x144, 0x206};
S16 DefaultCoeffChroma[9] =  {0x0a, 0x06,(S16) 0xfff7,(S16) 0xffe7, (S16)0xffe9, 0x0d,0x49, 0x86, 0x9a};
#endif

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define stvout_rd32(a)           STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)         STSYS_WriteRegDev32LE((void*)(a),(v))

/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t halvout_Enable( stvout_Unit_t * Unit_p);
static ST_ErrorCode_t HalSetOutputParams( stvout_Unit_t * Unit_p);
static ST_ErrorCode_t SetOutputParams( stvout_Unit_t * Unit_p, const STVOUT_OutputParams_t* const OutputParams_p);
static ST_ErrorCode_t GetOutputParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p);

/* Functions ---------------------------------------------------------------- */

/*-----------------5/23/01 12:19PM------------------
 * programm hardware to disable output
 * input : internal device structure
 * output: Error code
 * --------------------------------------------------*/
ST_ErrorCode_t stvout_HalDisable( stvout_Device_t * Device_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifdef STVOUT_HDOUT
    void* Address = (void*)( (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p);
#endif /* STVOUT_HDOUT */

    switch ( Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :   /*Denc Outputs */ /* no break */
        case STVOUT_OUTPUT_ANALOG_YUV :   /*Denc Outputs */
            if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629) || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)||
                (Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13)  || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7710)||
                (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7100))
            {
                if((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                }
                else if ((Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))==(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                {
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 6);
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                   if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                }
            }
            else
            {
                if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);

            }
            break;

        case STVOUT_OUTPUT_ANALOG_YC :    /*Denc Outputs */
            switch ( Device_p->DencVersion)
            {
                case 3 :
                    if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                    if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                    break;
                case 5 :
                    if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                    if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                    break;
                case 12:
                case 13:
                     if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                       ||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                    {
                       if((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                       {
                          if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                          if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                       }
                       else if ((Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                       {
                          if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                          if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                       }

                    }
                    else if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
                    {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                    }
                    else /* For STVOUT_DEVICE_TYPE_7710  & STVOUT_DEVICE_TYPE_7100 */
                    {
                        if ((Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                        }
                        else if ((Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                        }

                    }

                    break;
                 default :
                    if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015)
                         || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                    {
                        if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                        if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                    }
                    else if((Device_p->DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)\
                    &&(((Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))))
                    {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                    }
                    else /* STVOUT_DEVICE_TYPE_DENC_ENHANCED, STVOUT_DEVICE_TYPE_DENC, GX1,... */
                    {
                        if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                        if ( OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                    }
                    break;
            }
            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :  /*Denc Outputs */
            switch ( Device_p->DencVersion)
            {
                case 3 :
                    Error = stvout_HalDisableDac( Device_p, 1);
                    break;
                case 5 :
                    Error = stvout_HalDisableDac( Device_p, 1);
                    break;
                case 12:
                case 13:
                   if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                       ||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                    {
                        if((Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_2))==(STVOUT_DAC_2))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 2);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_4))==(STVOUT_DAC_4))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_5))==(STVOUT_DAC_5))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))
                        {
                        if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 6);
                        }
                   }
                   else if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)
                   {
                        if ((Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_4))==(STVOUT_DAC_4))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 4);
                        }
                   }
                   else if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200)
                   {
                        if ((Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                        }
                   }
                   else /* For STVOUT_DEVICE_TYPE_7710  &  STVOUT_DEVICE_TYPE_7100*/
                   {
                        if ((Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 1);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 3);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_5))==(STVOUT_DAC_5))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 5);
                        }
                        else if ((Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))
                        {
                            if (OK(Error)) Error = stvout_HalDisableDac( Device_p, 6);
                        }
                   }
                   break;
              default :
                  if (((Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015)||(Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                     ||((Device_p->DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)&&((Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))))
                  {
                        Error = stvout_HalDisableDac( Device_p, 6);
                  }
                  else /*STVOUT_DEVICE_TYPE_DENC_ENHANCED, STVOUT_DEVICE_TYPE_DENC, GX1,... */
                  {
                        Error = stvout_HalDisableDac( Device_p, 3);
                  }
                  break;
            }
            break;
#ifdef STVOUT_HDOUT
        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            if (Device_p->DeviceType != STVOUT_DEVICE_TYPE_7200)
            {
                if ( OK(Error)) Error = stvout_HalDisableRGBDac( Address, 4);
                if ( OK(Error)) Error = stvout_HalDisableRGBDac( Address, 3);
                if ( OK(Error)) Error = stvout_HalDisableRGBDac( Address, 2);
            }
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
            Error = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            if ( OK(Error)) Error = stvout_HalDisableDigital(Device_p ); /* 7015/20 : ConfigBaseAddress 0x0000 */
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */
            if ((Device_p->DeviceType!= STVOUT_DEVICE_TYPE_7710)&&(Device_p->DeviceType!= STVOUT_DEVICE_TYPE_7100)
              &&(Device_p->DeviceType!= STVOUT_DEVICE_TYPE_7200))
            {
                Error = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            }
            if ( OK(Error)) Error = stvout_HalDisableDigital(Device_p ); /* 7015/20 : ConfigBaseAddress 0x0000 */
            break;

        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
            if ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015) || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
            {
                Error = stvout_HalSetDigitalEmbeddedSyncSd( Address, FALSE);
            }
            if ( OK(Error)) Error = stvout_HalDisableDigital(Device_p ); /* 7015/20 : ConfigBaseAddress 0x0000 */
            break;

        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
            Error = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            if ( OK(Error)) Error = stvout_HalDisableDigital(Device_p ); /* 7015/20 : ConfigBaseAddress 0x0000 */
            break;
#endif /* STVOUT_HDOUT */
#ifdef STVOUT_SVM
        case STVOUT_OUTPUT_ANALOG_SVM :
            Error = stvout_HalDisableSVMDac( Address, 0);
            break;
#endif /* STVOUT_SVM */
#ifdef STVOUT_HDMI
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
             Error = HDMI_Disable(Device_p->HdmiHandle);
             break;
#endif /*STVOUT_HDMI*/
        default:
            Error = ST_NO_ERROR;
            break;
    }
    return( Error);
}
/*******************************************************************************
Name        : stvout_PowerOffDac
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t stvout_PowerOffDac( stvout_Device_t * Device_p)
{
#if defined (ST_7100)|| defined (ST_7109)
    U32 Offset = DSPCFG_DAC;
    U32 Address ;
    U32 Value;
#if defined(ST_7100)
    U32 CutId;
#endif /* ST_7100 */

    Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;

#if defined (ST_7100)

    CutId = ST_GetCutRevision();

    if( CutId >=0xB0 )
    {
#endif /*7100*/

        STOS_InterruptLock();

        Value = stvout_rd32( Address + DSPCFG_DAC);
        switch ( Device_p->OutputType)
        {
             case STVOUT_OUTPUT_ANALOG_RGB :   /* no break */
             case STVOUT_OUTPUT_ANALOG_YUV :
             case STVOUT_OUTPUT_ANALOG_YC  :
             case STVOUT_OUTPUT_ANALOG_CVBS :
                 Value &= (U32)~DSPCFG_DAC_ENABLE_SD; /*power off SD Dacs*/
                 #ifdef STVOUT_HDDACS
                     Value &= (U32)~DSPCFG_DAC_PO_HD; /*power off HD Dacs*/
                 #endif
                 break;
        #ifdef STVOUT_HDOUT
             case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
             case STVOUT_OUTPUT_HD_ANALOG_YUV :
                 Value &= (U32)~DSPCFG_DAC_PO_HD; /*power off HD Dacs*/
                 break;
        #endif
             default:
                 break;
        }
        stvout_wr32( Address + Offset, Value);
        STOS_InterruptUnlock();

#if defined (ST_7100)
    }
#endif /* ST_7100 */
#elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    U32 Offset=0;
    U32 Address ;
    U32 Value=0;

    Address = (U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p;

    STOS_InterruptLock();
    switch ( Device_p->OutputType)
    {
         case STVOUT_OUTPUT_ANALOG_RGB :   /* no break */
         case STVOUT_OUTPUT_ANALOG_YUV :
         case STVOUT_OUTPUT_ANALOG_YC  :
         case STVOUT_OUTPUT_ANALOG_CVBS :
             Offset = VOUT_AUX_GLUE_OFFSET ;
             Value = stvout_rd32( Address + Offset);
             Value |= (U32)SD_DAC_DISABLE; /*power off SD Dacs*/
             break;
        #ifdef STVOUT_HDOUT
         case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
         case STVOUT_OUTPUT_HD_ANALOG_YUV :
             Offset = VOUT_MAIN_GLUE_OFFSET ;
             Value = stvout_rd32( Address + Offset);
             Value |= (U32)HD_DAC_DISABLE; /*power off HD Dacs*/
             break;
        #endif
         default:
             break;
    }
    stvout_wr32( Address + Offset, Value);
    STOS_InterruptUnlock();

#else /*!(defined (ST_7100)|| defined (ST_7109)*/
     UNUSED_PARAMETER(Device_p);
#endif /* ST_7100-7109 */
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : halvout_PowerOnDac
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t halvout_PowerOnDac(  stvout_Unit_t *Unit_p)
{
#if defined (ST_7100)|| defined (ST_7109)
    U32 Offset = DSPCFG_DAC;
    U32 Address ;
    U32 Value;
#if defined(ST_7100)
    U32 CutId;
#endif /* ST_7100 */

    Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;

#if defined (ST_7100)

    CutId = ST_GetCutRevision();

    if( CutId >=0xB0 )
    {
#endif /*7100*/

        STOS_InterruptLock();

        Value = stvout_rd32( Address + DSPCFG_DAC);
        switch ( Unit_p->Device_p->OutputType)
        {
             case STVOUT_OUTPUT_ANALOG_RGB :   /* no break */
             case STVOUT_OUTPUT_ANALOG_YUV :
             case STVOUT_OUTPUT_ANALOG_YC  :
             case STVOUT_OUTPUT_ANALOG_CVBS :
                    Value |= DSPCFG_DAC_ENABLE_SD;
                    #ifdef STVOUT_HDDACS
                        Value |= (U32)DSPCFG_DAC_PO_HD;
                    #endif
                    break;
        #ifdef STVOUT_HDOUT
             case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
             case STVOUT_OUTPUT_HD_ANALOG_YUV :
                        Value |= (U32)DSPCFG_DAC_PO_HD;
                        break;
        #endif
             default:
                 break;
        }
        stvout_wr32( Address + Offset, Value);
        STOS_InterruptUnlock();

#if defined (ST_7100)
    }
#endif /* ST_7100 */
#elif defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    U32 Offset=0;
    U32 Address;
    U32 Value=0;

    Address = (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p;

    STOS_InterruptLock();
    switch (Unit_p->Device_p->OutputType)
    {
         case STVOUT_OUTPUT_ANALOG_RGB :   /* no break */
         case STVOUT_OUTPUT_ANALOG_YUV :   /* no break */
         case STVOUT_OUTPUT_ANALOG_YC  :  /* no break */
         case STVOUT_OUTPUT_ANALOG_CVBS :
             Offset = VOUT_AUX_GLUE_OFFSET ;
             Value = stvout_rd32( Address + Offset);
             Value &= (U32)~SD_DAC_DISABLE; /*power on SD Dacs*/
             break;
        #ifdef STVOUT_HDOUT
         case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
         case STVOUT_OUTPUT_HD_ANALOG_YUV :
             Offset = VOUT_MAIN_GLUE_OFFSET ;
             Value = stvout_rd32( Address + Offset);
             Value &= (U32)~HD_DAC_DISABLE; /*power on HD Dacs*/
             break;
        #endif
         default:
             break;
    }
    stvout_wr32( Address + Offset, Value);
    STOS_InterruptUnlock();

#else /*!(defined (ST_7100)|| defined (ST_7109))*/
     UNUSED_PARAMETER(Unit_p);

#endif /* ST_7100-7109 */
    return (ST_NO_ERROR);
}

/*-----------------5/23/01 12:19PM------------------
 * programm hardware to Enable output
 * input : internal device structure
 * output: Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t halvout_Enable( stvout_Unit_t *Unit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef  STVOUT_HDOUT
    void* Address = (void*)( (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p);
#endif /* STVOUT_HDOUT */
    switch ( Unit_p->Device_p->OutputType)
    {

        case STVOUT_OUTPUT_ANALOG_RGB :   /*Denc Outputs */ /* no break */
        case STVOUT_OUTPUT_ANALOG_YUV :   /*Denc Outputs */
             if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629) || (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                || (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
             {
                 if((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                 {
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                 }
                 else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))==(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                 {
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 6);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                 }
             }
             else if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)|| (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
             {
                if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
             }
             else
             {
                if((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))==(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                {
                #ifdef STVOUT_HDOUT
                     if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 4);
                     if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 3);
                     if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 2);
                #endif
                     if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                     if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                     if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                }
                else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))==(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                {/*for RGB output*/
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 6);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                      if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                }
              }
             break;
        case STVOUT_OUTPUT_ANALOG_YC :    /*Denc Outputs */
            switch ( Unit_p->Device_p->DencVersion)
            {
                case 3 :
                    if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                    if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                    break;
                case 5 :
                    if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                    if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                    break;
                case 12 :
                case 13 :
                    if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                        ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                        ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                    {
                       if((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                       }
                    }
                    else if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)||
                             (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
                    {
                        if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                        if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                    }

                    else  /* For STVOUT_DEVICE_TYPE_7710 & STVOUT_DEVICE_TYPE_7100*/
                    {
                        if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                        {
                            if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                            if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))
                        {
                            if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                            if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                        }

                    }

                    break;
                 default:
                    if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015)
                     ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                    {
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                    }
                    else if((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)\
                    &&(((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4|STVOUT_DAC_5))==(STVOUT_DAC_4|STVOUT_DAC_5))))
                    {
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                    }
                   else /* STVOUT_DEVICE_TYPE_DENC_ENHANCED, STVOUT_DEVICE_TYPE_DENC, GX1, ... */
                    {
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                        if ( OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                    }
                    break;
            }
            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :  /*Denc Outputs */
            switch ( Unit_p->Device_p->DencVersion)
            {
                case 3 :
                    Error = stvout_HalEnableDac( Unit_p, 1);
                    break;
                case 5 :
                    Error = stvout_HalEnableDac( Unit_p, 1);
                    break;
                case 12 :
                case 13 :
                    if((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                       ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                       ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                    {
                       if((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_2))==(STVOUT_DAC_2))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 2);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4))==(STVOUT_DAC_4))
                       {
                          if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_5))==(STVOUT_DAC_5))
                       {
                          if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))
                       {
                          if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 6);
                       }
                   }
                   else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)
                   {
                       if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_4))==(STVOUT_DAC_4))
                       {
                          if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 4);
                       }
                   }
                   else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200)
                   {
                       if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                       }
                   }
                   else /* For STVOUT_DEVICE_TYPE_7710  & STVOUT_DEVICE_TYPE_7100 */
                   {
                       if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 1);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 3);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_5))==(STVOUT_DAC_5))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 5);
                       }
                       else if ((Unit_p->Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))
                       {
                           if (OK(Error)) Error = stvout_HalEnableDac( Unit_p, 6);
                       }
                   }
                   break;
               default:
                   if (((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015)||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))\
                    ||((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)&&((Unit_p->Device_p->DacSelect & (STVOUT_DAC_6))==(STVOUT_DAC_6))))
                    {
                        Error = stvout_HalEnableDac( Unit_p, 6);
                    }
                    else /*STVOUT_DEVICE_TYPE_DENC_ENHANCED, STVOUT_DEVICE_TYPE_DENC, GX1,...  */
                    {
                        Error = stvout_HalEnableDac( Unit_p, 3);
                    }
                    break;
            }
            break;
        #ifdef STVOUT_HDOUT
          case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
          case STVOUT_OUTPUT_HD_ANALOG_YUV :
              if (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7200)
              {
                  if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 4);
                  if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 3);
                  if ( OK(Error)) Error = stvout_HalEnableRGBDac( Address, 2);
              }
              break;
          case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS : /* no break */
          case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
          case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */ /* no break */
          case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
              Error = stvout_HalEnableDigital(Unit_p->Device_p ); /* 7015/20 : ConfigBaseAddress 0x0000 */
              break;
        #endif /* STVOUT_HDOUT  */
        #ifdef STVOUT_SVM
          case STVOUT_OUTPUT_ANALOG_SVM :
              Error = stvout_HalEnableSVMDac( Address, 0);
              break;
        #endif /* STVOUT_SVM */
        #ifdef STVOUT_HDMI
          case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
               Error = HDMI_Enable(Unit_p->Device_p->HdmiHandle);
               break;
        #endif /* STVOUT_HDMI*/
        default:
            Error = ST_NO_ERROR;
            break;
    }
    return( Error);
}



/*
---- Set VOUT Configuration ---------------------------------------------------
*/
/* Output Parameters on V3i2c: only denc analog output
                Denc Version
DACLevel        Not applicable
Brightness      Not applicable
Contrast        Not applicable
Saturation      Not applicable
EmbeddedType    Not applicable
InvertedOutput  ok
*/
/* Output Parameters on ST55XX: only denc analog output
                Denc Version
DACLevel        V6/7/8/9/10/11/12
Brightness        V7/8/9/10/11/12
Contrast          V7/8/9/10/11/12
Saturation        V7/8/9/10/11/12
EmbeddedType    Not applicable
InvertedOutput  V5/6/7/8/9/10/11/12
*/
/* Output Parameters on ST7015/20:
               Denc(YC/CVBS) Analog(RGB/YCBCR)
DACLevel        Ok                  N/A
Brightness      Ok                  N/A
Contrast        Ok                  N/A
Saturation      Ok                  N/A
EmbeddedType    N/A                 Ok
InvertedOutput  Ok                  N/A
               Digital
EmbeddedType    Ok
*/

static ST_ErrorCode_t stvout_SetBCSLumaChroma( stvout_Unit_t * Unit_p, const STVOUT_OutputParams_t* const OutputParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int Inc, sum = 0;

    Unit_p->Device_p->Out.Analog.StateBCSLevel =
        OutputParams_p->Analog.StateBCSLevel;
    switch ( OutputParams_p->Analog.StateBCSLevel)
    {
        case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
            break;
        case STVOUT_PARAMS_DEFAULT :
            stvout_MaxDynState = FALSE;
            Unit_p->Device_p->Out.Analog.BCSLevel.Brightness    = 0x80;
            Unit_p->Device_p->Out.Analog.BCSLevel.Contrast      = 0x00;
            Unit_p->Device_p->Out.Analog.BCSLevel.Saturation    = 0x80;
            break;
        case STVOUT_PARAMS_AFFECTED :
            Unit_p->Device_p->Out.Analog.BCSLevel.Brightness    = OutputParams_p->Analog.BCSLevel.Brightness;
            Unit_p->Device_p->Out.Analog.BCSLevel.Contrast      = OutputParams_p->Analog.BCSLevel.Contrast;
            Unit_p->Device_p->Out.Analog.BCSLevel.Saturation    = OutputParams_p->Analog.BCSLevel.Saturation;

            if (   Unit_p->Device_p->Out.Analog.BCSLevel.Brightness >  235
                || Unit_p->Device_p->Out.Analog.BCSLevel.Brightness <   16
                || Unit_p->Device_p->Out.Analog.BCSLevel.Contrast   < -112
                || Unit_p->Device_p->Out.Analog.BCSLevel.Contrast   <  108
                || Unit_p->Device_p->Out.Analog.BCSLevel.Saturation >  235
                || Unit_p->Device_p->Out.Analog.BCSLevel.Saturation <   16  )
            {
                stvout_MaxDynState = TRUE;
            }
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    Unit_p->Device_p->Out.Analog.StateChrLumFilter =  OutputParams_p->Analog.StateChrLumFilter;
    switch ( OutputParams_p->Analog.StateChrLumFilter)
    {
        case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
            break;
        case STVOUT_PARAMS_DEFAULT : /* nothing to do */
            break;
        case STVOUT_PARAMS_AFFECTED :

            for (Inc = 0; Inc < 8; Inc++)
            {
                sum += 2 * OutputParams_p->Analog.ChrLumFilter.Chroma[Inc];
            }
            sum += OutputParams_p->Analog.ChrLumFilter.Chroma[8];
            if ( !(sum == 4096 || sum == 2048 || sum == 1024 || sum == 512 || sum == 0))
            {
                Error = ST_ERROR_BAD_PARAMETER;
            }

            for (Inc = 0; Inc < 9; Inc++)
            {
                Unit_p->Device_p->Out.Analog.ChrLumFilter.Chroma[Inc] = OutputParams_p->Analog.ChrLumFilter.Chroma[Inc];
            }
            sum = 0;
            for (Inc = 0; Inc < 9; Inc++)
            {
                sum += 2 * OutputParams_p->Analog.ChrLumFilter.Luma[Inc];
            }
            sum += OutputParams_p->Analog.ChrLumFilter.Luma[9];
            if ( !(sum == 4096 || sum == 2048 || sum == 1024 || sum == 512 || sum == 0))
            {
                Error = ST_ERROR_BAD_PARAMETER;
            }

            for (Inc = 0; Inc < 10; Inc++)
            {
                Unit_p->Device_p->Out.Analog.ChrLumFilter.Luma[Inc] = OutputParams_p->Analog.ChrLumFilter.Luma[Inc];
            }
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    Unit_p->Device_p->Out.Analog.EmbeddedType   = OutputParams_p->Analog.EmbeddedType;
    Unit_p->Device_p->Out.Analog.InvertedOutput = OutputParams_p->Analog.InvertedOutput;
    return(Error);
}

/*-----------------5/23/01 12:21PM------------------
 * affect internal structure to prepare programmation
 * of output parameters
 * --------------------------------------------------*/
static ST_ErrorCode_t SetOutputParams( stvout_Unit_t * Unit_p, const STVOUT_OutputParams_t* const OutputParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    switch ( Unit_p->Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :
            Unit_p->Device_p->Out.Analog.StateAnalogLevel =
                OutputParams_p->Analog.StateAnalogLevel;
            switch ( OutputParams_p->Analog.StateAnalogLevel)
            {
                case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
                    break;
                case STVOUT_PARAMS_DEFAULT :
                       Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R      = 0;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G      = 0;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B      = 0;
                    break;
                case STVOUT_PARAMS_AFFECTED :
                    Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R      = OutputParams_p->Analog.AnalogLevel.RGB.R;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G      = OutputParams_p->Analog.AnalogLevel.RGB.G;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B      = OutputParams_p->Analog.AnalogLevel.RGB.B;
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            if ( OK(Error)) Error = stvout_SetBCSLumaChroma( Unit_p, OutputParams_p);
            #ifdef STVOUT_HDDACS
                Unit_p->Device_p->Out.Analog.ColorSpace             = OutputParams_p->Analog.ColorSpace;
            #endif
            break;
        case STVOUT_OUTPUT_ANALOG_YUV :
            Unit_p->Device_p->Out.Analog.StateAnalogLevel =
                OutputParams_p->Analog.StateAnalogLevel;
            switch ( OutputParams_p->Analog.StateAnalogLevel)
            {
                case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
                    break;
                case STVOUT_PARAMS_DEFAULT :
                    if(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7710)  /*GNBvd42154*/
                    {
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y      = (S8)0x20;
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U      = (S8)0x1d;
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V      = (S8)0xa0;
                    }
                    else
                    {
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y      = 0;
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U      = 0;
                        Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V      = 0;
                    }
                    break;
                case STVOUT_PARAMS_AFFECTED :
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y      = OutputParams_p->Analog.AnalogLevel.YUV.Y;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U      = OutputParams_p->Analog.AnalogLevel.YUV.U;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V      = OutputParams_p->Analog.AnalogLevel.YUV.V;
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            #ifdef STVOUT_HDDACS
                 Unit_p->Device_p->Out.Analog.ColorSpace             = OutputParams_p->Analog.ColorSpace;
            #endif

            if ( OK(Error)) Error = stvout_SetBCSLumaChroma( Unit_p, OutputParams_p);
            break;
        case STVOUT_OUTPUT_ANALOG_YC :
            Unit_p->Device_p->Out.Analog.StateAnalogLevel =
                OutputParams_p->Analog.StateAnalogLevel;
            switch ( OutputParams_p->Analog.StateAnalogLevel)
            {
                case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
                    break;
                case STVOUT_PARAMS_DEFAULT :
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y       = 0;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C       = 0;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio = 0;
                    break;
                case STVOUT_PARAMS_AFFECTED :
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y       = OutputParams_p->Analog.AnalogLevel.YC.Y;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C       = OutputParams_p->Analog.AnalogLevel.YC.C;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio = OutputParams_p->Analog.AnalogLevel.YC.YCRatio;
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            #ifdef STVOUT_HDDACS
                 Unit_p->Device_p->Out.Analog.ColorSpace             = OutputParams_p->Analog.ColorSpace;
            #endif
            if ( OK(Error)) Error = stvout_SetBCSLumaChroma( Unit_p, OutputParams_p);

            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :
            Unit_p->Device_p->Out.Analog.StateAnalogLevel =
                OutputParams_p->Analog.StateAnalogLevel;
            switch ( OutputParams_p->Analog.StateAnalogLevel)
            {
                case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
                    break;
                case STVOUT_PARAMS_DEFAULT :
                    #if defined(ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#if 0 /*yxl 2007-09-06 modify below section*/
                       Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS    = 0x11;
                    #else
                        Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS    = 0xB;
#endif/*end yxl 2007-09-06 modify below section*/
                    #else
                        Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS    = 0;
                    #endif
                    Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio = 0;
                    break;
                case STVOUT_PARAMS_AFFECTED :
                    Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS    = OutputParams_p->Analog.AnalogLevel.CVBS.CVBS;
                    Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio = OutputParams_p->Analog.AnalogLevel.CVBS.YCRatio;
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            #ifdef STVOUT_HDDACS
                 Unit_p->Device_p->Out.Analog.ColorSpace             = OutputParams_p->Analog.ColorSpace;
            #endif
            if ( OK(Error)) Error = stvout_SetBCSLumaChroma( Unit_p, OutputParams_p);

            break;
#ifdef STVOUT_HDOUT
        case STVOUT_OUTPUT_HD_ANALOG_RGB :    /* no break */
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            Unit_p->Device_p->Out.Analog.EmbeddedType    = OutputParams_p->Analog.EmbeddedType;
            Unit_p->Device_p->Out.Analog.SyncroInChroma  = OutputParams_p->Analog.SyncroInChroma;
            Unit_p->Device_p->Out.Analog.ColorSpace      = OutputParams_p->Analog.ColorSpace;
            break;

        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :         /* no break */
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :  /* no break */
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :           /* no break */
#endif /* STVOUT_HDOUT */
#if defined(STVOUT_HDOUT) || defined(STVOUT_SDDIG)
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
#endif /* STVOUT_HDOUT || STVOUT_SDDIG */

            Unit_p->Device_p->Out.Digital.EmbeddedType          = OutputParams_p->Digital.EmbeddedType;  /* used for HDOUT and SDDIG */
            Unit_p->Device_p->Out.Digital.SyncroInChroma        = OutputParams_p->Digital.SyncroInChroma;
            Unit_p->Device_p->Out.Digital.ColorSpace            = OutputParams_p->Digital.ColorSpace;
            Unit_p->Device_p->Out.Digital.EmbeddedSystem        = OutputParams_p->Digital.EmbeddedSystem;
            break;
#ifdef STVOUT_SVM
        case STVOUT_OUTPUT_ANALOG_SVM :
            Unit_p->Device_p->Out.SVM.StateAnalogSVM =
                OutputParams_p->SVM.StateAnalogSVM;
            switch ( OutputParams_p->SVM.StateAnalogSVM)
            {
                case STVOUT_PARAMS_NOT_CHANGED : /* nothing to do */
                    break;
                case STVOUT_PARAMS_DEFAULT :
                    Unit_p->Device_p->Out.SVM.DelayCompensation = 7;
                    Unit_p->Device_p->Out.SVM.Shape             = 0;
                    Unit_p->Device_p->Out.SVM.Gain              = 31;
                    Unit_p->Device_p->Out.SVM.OSDFactor         = 0;
                    Unit_p->Device_p->Out.SVM.VideoFilter       = 2;
                    Unit_p->Device_p->Out.SVM.OSDFilter         = 1;
                    break;
                case STVOUT_PARAMS_AFFECTED :
                    if ( OutputParams_p->SVM.Shape < STVOUT_SVM_SHAPE_OFF ||
                         OutputParams_p->SVM.Shape > STVOUT_SVM_SHAPE_3     )
                    {
                        Error = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        if ( Unit_p->Device_p->Out.SVM.OSDFactor < STVOUT_SVM_FACTOR_0 ||
                             Unit_p->Device_p->Out.SVM.OSDFactor > STVOUT_SVM_FACTOR_1   )
                        {
                            Error = ST_ERROR_BAD_PARAMETER;
                        }
                        else
                        {
                            if ( Unit_p->Device_p->Out.SVM.VideoFilter < STVOUT_SVM_FILTER_1 ||
                                 Unit_p->Device_p->Out.SVM.VideoFilter > STVOUT_SVM_FILTER_2   )
                            {
                                Error = ST_ERROR_BAD_PARAMETER;
                            }
                            else
                            {
                                if ( Unit_p->Device_p->Out.SVM.OSDFilter < STVOUT_SVM_FILTER_1 ||
                                     Unit_p->Device_p->Out.SVM.OSDFilter > STVOUT_SVM_FILTER_2   )
                                {
                                    Error = ST_ERROR_BAD_PARAMETER;
                                }
                                else
                                {
                                    Unit_p->Device_p->Out.SVM.DelayCompensation = OutputParams_p->SVM.DelayCompensation;
                                    Unit_p->Device_p->Out.SVM.Shape             = OutputParams_p->SVM.Shape;
                                    Unit_p->Device_p->Out.SVM.Gain              = OutputParams_p->SVM.Gain;
                                    Unit_p->Device_p->Out.SVM.OSDFactor         = OutputParams_p->SVM.OSDFactor;
                                    Unit_p->Device_p->Out.SVM.VideoFilter       = OutputParams_p->SVM.VideoFilter;
                                    Unit_p->Device_p->Out.SVM.OSDFilter         = OutputParams_p->SVM.OSDFilter;
                                }
                            }
                        }
                    }
                    break;
                default:
                    Error = ST_ERROR_BAD_PARAMETER;
                    break;
            }
            break;
#endif /* STVOUT_SVM */
#ifdef STVOUT_HDMI
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
             Unit_p->Device_p->Out.HDMI.ForceDVI = OutputParams_p->HDMI.ForceDVI;
             Unit_p->Device_p->Out.HDMI.IsHDCPEnable = OutputParams_p->HDMI.IsHDCPEnable;
			 Unit_p->Device_p->Out.HDMI.AudioFrequency = OutputParams_p->HDMI.AudioFrequency;
             break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(Error);
}


static ST_ErrorCode_t halvout_SetBCSLumaChroma( stvout_Unit_t * Unit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
/* Brightness, Contrast, Saturation */
    if (Unit_p->Device_p->Out.Analog.StateBCSLevel != STVOUT_PARAMS_NOT_CHANGED)
    {
        if ( OK(Error)) Error = stvout_HalMaxDynamic    ( Unit_p, stvout_MaxDynState );
        if ( OK(Error)) Error = stvout_HalSetBrightness ( Unit_p, Unit_p->Device_p->Out.Analog.BCSLevel.Brightness );
        if ( OK(Error)) Error = stvout_HalSetContrast   ( Unit_p, Unit_p->Device_p->Out.Analog.BCSLevel.Contrast   );
        if ( OK(Error)) Error = stvout_HalSetSaturation ( Unit_p, Unit_p->Device_p->Out.Analog.BCSLevel.Saturation );
        if ( OK(Error)) Error = stvout_HalEnableBCS     ( Unit_p, TRUE );
    }
/* Chroma / Luma Filtering */
    if (Unit_p->Device_p->Out.Analog.StateChrLumFilter != STVOUT_PARAMS_NOT_CHANGED)
    {
        if ( Unit_p->Device_p->Out.Analog.StateChrLumFilter == STVOUT_PARAMS_AFFECTED)
        {
            if ( OK(Error)) stvout_HalSetChromaFilter( Unit_p, Unit_p->Device_p->Out.Analog.ChrLumFilter.Chroma );
            if ( OK(Error)) stvout_HalSetLumaFilter  ( Unit_p, Unit_p->Device_p->Out.Analog.ChrLumFilter.Luma   );
        }
        else
        {
#if defined (ST_7100) || defined (ST_7109)
            if ( OK(Error)) stvout_HalSetChromaFilter( Unit_p, DefaultCoeffChroma );
            if ( OK(Error)) stvout_HalSetLumaFilter  ( Unit_p, DefaultCoeffLuma );
#else
            if ( OK(Error)) stvout_HalSetChromaFilter( Unit_p, NULL );
            if ( OK(Error)) stvout_HalSetLumaFilter  ( Unit_p, NULL );
#endif

        }
    }
/* EmbeddedType : Not applicable */
/* InvertedOutput */
    if ( OK(Error)) stvout_HalSetInvertedOutput ( Unit_p, Unit_p->Device_p->Out.Analog.InvertedOutput );

    return (ERROR(Error));
}

/*-----------------5/23/01 12:22PM------------------
 * programm hardware with internal structure previously
 * affect by SetOutputParams() function
 *
 *    Dac Outputs  : Y   C  CVBS  R   G   B   Y   U   V
 * chip (denc cell)
 *  119      3       4   5    1   5   4   6   /   /   /
 *  55xx     5       2   3    1   4   5   5   /   /   /
 *  55xx   >= 6      1   2    3   4   5   6   5   6   4
 *  7015     10      5   4    6   4   5   6   5   6   4
 *  7020     11      5   4    6   4   5   6   5   6   4
 *  4629     12      1   2    3   1   2   3   1   3   2
 * --------------------------------------------------*/
static ST_ErrorCode_t HalSetOutputParams( stvout_Unit_t * Unit_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#if defined(STVOUT_HDOUT) || defined(STVOUT_SDDIG)
    void* Address = (void*)( (U32)Unit_p->Device_p->DeviceBaseAddress_p + (U32)Unit_p->Device_p->BaseAddress_p);
#ifdef STVOUT_SVM
    U32 Filter;
#endif /* STVOUT_HDOUT */
#endif /* STVOUT_HDOUT || STVOUT_SDDIG */

/* hardware setup */
    switch ( Unit_p->Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :   /*Denc Outputs */
    /* DACLevel */
            if (Unit_p->Device_p->Out.Analog.StateAnalogLevel != STVOUT_PARAMS_NOT_CHANGED)
            {
                if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                   ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                   ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                {
                   if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))== (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                    {
                        if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R) + 8 );
                    }
                   else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                    {
                        if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R) + 8 );
                    }
                }
                else if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                    if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R) + 8 );
                }
                else
                {
                    if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))== (STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6))
                    {
                        if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R) + 8 );
                    }
                    else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                    {
                        if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R) + 8 );
                        #ifdef STVOUT_HDDACS
                             if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);
                        #endif
                    }
                }
            }
            if ( OK(Error)) Error = halvout_SetBCSLumaChroma( Unit_p);
            Error = (ERROR(Error));
            break;
        case STVOUT_OUTPUT_ANALOG_YUV :   /*Denc Outputs */
    /* DACLevel */
            if (Unit_p->Device_p->Out.Analog.StateAnalogLevel != STVOUT_PARAMS_NOT_CHANGED)
            {
                if ( (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                   ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                   ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13 ))
                {
                     if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))== (STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3))
                         {
                            if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U) + 8 );
                            if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y) + 8 );
                            if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V) + 8 );
                         }
                     else
                         {
                           if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U) + 8 );
                           if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y) + 8 );
                           if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V) + 8 );
                         }
                }
                else if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                   if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V) + 8 );
                }
                else
                {
                #ifdef STVOUT_HDDACS
                   if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V) + 8 );

                   if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);

                #else
                   if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y) + 8 );
                   if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V) + 8 );
                #endif
                }

            }
            if ( OK(Error)) Error = halvout_SetBCSLumaChroma( Unit_p);
            Error = (ERROR(Error));
            break;
        case STVOUT_OUTPUT_ANALOG_YC :    /*Denc Outputs */
            /* DACLevel */
            if (Unit_p->Device_p->Out.Analog.StateAnalogLevel != STVOUT_PARAMS_NOT_CHANGED)
            {
                if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015) || (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                {
                    if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio) );
                }
                else if (((Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_5528)
                        &&(Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_V13))
                        ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)
                        ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                    if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio) );
                }
                else
                {
                    if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                      {
                        if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio) );
                        #ifdef STVOUT_HDDACS
                            if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);
                        #endif
                      }
                    else
                     {
                        if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C) + 8 );
                        if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio) );

                     }
                }
            }
            if ( OK(Error)) Error = halvout_SetBCSLumaChroma( Unit_p);
            Error = (ERROR(Error));
            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :  /*Denc Outputs */
    /* DACLevel */
            if (Unit_p->Device_p->Out.Analog.StateAnalogLevel != STVOUT_PARAMS_NOT_CHANGED)
            {
                if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015)
                    ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                {
                    if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                }
                else if ((Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_5528)
                        && (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_V13)
                        && (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7710)
                        && (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7100)
                        && (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_5525)
                        && (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7200))
                {
                    if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                    if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                }
                else
                {
                    if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                        ||(Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13))
                    {

                       if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1))==(STVOUT_DAC_1))
                       {
                          if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                       else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_2))==(STVOUT_DAC_2))
                       {
                         if ( OK(Error)) Error = stvout_HalWriteDAC2 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                         if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                       else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_3))==(STVOUT_DAC_3))
                       {
                         if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                         if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                       else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_4))==(STVOUT_DAC_4))
                       {
                         if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                         if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                       else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_5))==(STVOUT_DAC_5))
                       {
                         if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                         if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                       else
                       {
                         if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                         if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                       }
                    }
                    else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)
                    {
                        if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                            if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                            if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_4))==(STVOUT_DAC_4))
                        {
                            if ( OK(Error)) Error = stvout_HalWriteDAC4 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                            if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                    }
                    else if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200)
                    {
                        if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                            if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                            if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1))==(STVOUT_DAC_1))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_6))==(STVOUT_DAC_6))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                    }
                    else /* For STVOUT_DEVICE_TYPE_7710 & STVOUT_DEVICE_TYPE_7100 */
                    {
                        if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_1))==(STVOUT_DAC_1))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC1 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                          #ifdef STVOUT_HDDACS
                            if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);
                          #endif
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_3))==(STVOUT_DAC_3))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC3 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                          #ifdef STVOUT_HDDACS
                            if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);
                          #endif
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_6))==(STVOUT_DAC_6))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC6 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                        else if ((Unit_p->Device_p->DacSelect &(STVOUT_DAC_5))==(STVOUT_DAC_5))
                        {
                          if ( OK(Error)) Error = stvout_HalWriteDAC5 ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS) + 8 );
                          if ( OK(Error)) Error = stvout_HalWriteDACC ( Unit_p, (Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio) );
                        }
                    }
                }
            }
            if ( OK(Error)) Error = halvout_SetBCSLumaChroma( Unit_p);
            Error = (ERROR(Error));
            break;
        #ifdef STVOUT_HDOUT
         case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
         case STVOUT_OUTPUT_HD_ANALOG_YUV :
            /* EmbeddedType */
            if (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7200)
            {
                if ( OK(Error)) Error = stvout_HalSetAnalogEmbeddedSync( Address, Unit_p->Device_p->Out.Analog.EmbeddedType);
                if ( OK(Error)) Error = stvout_HalSetSyncroInChroma( Address, Unit_p->Device_p->Out.Analog.SyncroInChroma);
                if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Analog.ColorSpace);
            }
            break;
         case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS : /* no break */
         case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */ /* no break */
         case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS : /* no break */
        #endif /*STVOUT_HDOUT */
        #if defined(STVOUT_HDOUT) || defined(STVOUT_SDDIG)
         case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
            #ifdef STVOUT_HDOUT
                if ((Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015) || (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
                {
                    if ( Unit_p->Device_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED)
                    {
                        if ( OK(Error)) Error = stvout_HalSetDigitalEmbeddedSyncSd( Address, Unit_p->Device_p->Out.Digital.EmbeddedType);
                    }
                    else
                    {
                        if ( OK(Error)) Error = stvout_HalSetDigitalEmbeddedSyncHd( Address, Unit_p->Device_p->Out.Digital.EmbeddedType);
                    }
                }
                else if (Unit_p->Device_p->DeviceType != STVOUT_DEVICE_TYPE_7200)
                {
                    if ( OK(Error)) Error = stvout_HalSetSyncroInChroma( Address, Unit_p->Device_p->Out.Digital.SyncroInChroma);
                    if ( OK(Error)) Error = stvout_HalSetColorSpace( Address, (U8)Unit_p->Device_p->Out.Digital.ColorSpace);
                }
            #endif /* STVOUT_HDOUT */
            #ifdef STVOUT_SDDIG
                if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)
                {
                    Error =  stvout_HalSetDigitalOutputCCIRMode( Address, Unit_p->Device_p->Out.Digital.EmbeddedType);
                    #if defined(ST_5528) || defined(ST_5100)|| defined(ST_5301)|| defined(ST_5107)
                        if ( OK(Error)) Error = stvout_HalSetEmbeddedSystem( Address, Unit_p->Device_p->Out.Digital.EmbeddedSystem);
                    #endif
                }
            #endif /* STVOUT_SDDIG */
            break;
        #endif /* STVOUT_HDOUT || STVOUT_SDDIG */
        #ifdef STVOUT_SVM
         case STVOUT_OUTPUT_ANALOG_SVM :
            if (Unit_p->Device_p->Out.SVM.StateAnalogSVM != STVOUT_PARAMS_NOT_CHANGED)
            {
                if ( OK(Error)) Error = stvout_HalSVMDelayComp( Address, Unit_p->Device_p->Out.SVM.DelayCompensation);
                if ( OK(Error)) Error = stvout_HalSVMModulationShapeControl( Address, (U32)Unit_p->Device_p->Out.SVM.Shape);
                if ( OK(Error)) Error = stvout_HalSVMGainControl( Address, Unit_p->Device_p->Out.SVM.Gain);
                if ( OK(Error)) Error = stvout_HalSVMOSDFactor( Address, (U32)Unit_p->Device_p->Out.SVM.OSDFactor);
                switch ( Unit_p->Device_p->Out.SVM.VideoFilter)
                {
                    case STVOUT_SVM_FILTER_1 :
                        Filter = ( Unit_p->Device_p->Out.SVM.OSDFilter == STVOUT_SVM_FILTER_1) ? 0 : 1;
                        break;
                    case STVOUT_SVM_FILTER_2 :
                        Filter = ( Unit_p->Device_p->Out.SVM.OSDFilter == STVOUT_SVM_FILTER_1) ? 2 : 3;
                        break;
                    default:
                        Filter = 0;
                        break;
                }
                if ( OK(Error)) Error = stvout_HalSVMFilter( Address, Filter);
            }
            break;
        #endif /* STVOUT_SVM */
        #ifdef STVOUT_HDMI
         case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
         case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
         case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
             if (OK(Error)) Error =  HDMI_SetParams(Unit_p->Device_p->HdmiHandle, Unit_p->Device_p->Out);
             break;
        #endif /* STVOUT_HDMI */
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return (Error);
}

/*
---- Get VOUT Configuration ---------------------------------------------------
*/
static void stvout_GetRGBParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.AnalogLevel.RGB.R   =  Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.R  ;
    OutputParams_p->Analog.AnalogLevel.RGB.G   =  Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.G  ;
    OutputParams_p->Analog.AnalogLevel.RGB.B   =  Unit_p->Device_p->Out.Analog.AnalogLevel.RGB.B  ;
}

static void stvout_GetYUVParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.AnalogLevel.YUV.Y   =  Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.Y  ;
    OutputParams_p->Analog.AnalogLevel.YUV.U   =  Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.U  ;
    OutputParams_p->Analog.AnalogLevel.YUV.V   =  Unit_p->Device_p->Out.Analog.AnalogLevel.YUV.V  ;
}

static void stvout_GetYCParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.AnalogLevel.YC.Y   =   Unit_p->Device_p->Out.Analog.AnalogLevel.YC.Y   ;
    OutputParams_p->Analog.AnalogLevel.YC.C   =   Unit_p->Device_p->Out.Analog.AnalogLevel.YC.C   ;
    OutputParams_p->Analog.AnalogLevel.YC.YCRatio = Unit_p->Device_p->Out.Analog.AnalogLevel.YC.YCRatio   ;
}

static void stvout_GetCVBSParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.AnalogLevel.CVBS.CVBS   =  Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.CVBS;
    OutputParams_p->Analog.AnalogLevel.CVBS.YCRatio = Unit_p->Device_p->Out.Analog.AnalogLevel.CVBS.YCRatio   ;
}

static void stvout_GetBCSParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.BCSLevel.Brightness =  Unit_p->Device_p->Out.Analog.BCSLevel.Brightness;
    OutputParams_p->Analog.BCSLevel.Contrast   =  Unit_p->Device_p->Out.Analog.BCSLevel.Contrast  ;
    OutputParams_p->Analog.BCSLevel.Saturation =  Unit_p->Device_p->Out.Analog.BCSLevel.Saturation;
}

static void stvout_GetLumaChromaParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    int Inc;

    for (Inc = 0; Inc < 9; Inc++)
    {
        OutputParams_p->Analog.ChrLumFilter.Chroma[Inc] = Unit_p->Device_p->Out.Analog.ChrLumFilter.Chroma[Inc] ;
    }
    for (Inc = 0; Inc < 10; Inc++)
    {
        OutputParams_p->Analog.ChrLumFilter.Luma[Inc] = Unit_p->Device_p->Out.Analog.ChrLumFilter.Luma[Inc];
    }
}

static void stvout_GetEmbInvParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    OutputParams_p->Analog.EmbeddedType        =  Unit_p->Device_p->Out.Analog.EmbeddedType       ;
    OutputParams_p->Analog.InvertedOutput      =  Unit_p->Device_p->Out.Analog.InvertedOutput     ;
}

/*-----------------5/23/01 12:24PM------------------
 * affect internal structure to returns informations
 * on output parameter.
 * --------------------------------------------------*/
static ST_ErrorCode_t GetOutputParams( stvout_Unit_t * Unit_p, STVOUT_OutputParams_t* OutputParams_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    switch ( Unit_p->Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
        case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
        #ifdef STVOUT_HDDACS
            OutputParams_p->Analog.ColorSpace      = Unit_p->Device_p->Out.Analog.ColorSpace;
        #endif
        case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
        case STVOUT_OUTPUT_ANALOG_CVBS :
            OutputParams_p->Analog.StateAnalogLevel  = Unit_p->Device_p->Out.Analog.StateAnalogLevel ;
            OutputParams_p->Analog.StateBCSLevel     = Unit_p->Device_p->Out.Analog.StateBCSLevel     ;
            OutputParams_p->Analog.StateChrLumFilter = Unit_p->Device_p->Out.Analog.StateChrLumFilter ;
            OutputParams_p->Analog.ColorSpace        = Unit_p->Device_p->Out.Analog.ColorSpace;
            stvout_GetBCSParams         ( Unit_p, OutputParams_p);
            stvout_GetLumaChromaParams  ( Unit_p, OutputParams_p);
            stvout_GetEmbInvParams      ( Unit_p, OutputParams_p);
            if ( Unit_p->Device_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB )
                stvout_GetRGBParams ( Unit_p, OutputParams_p);
            if ( Unit_p->Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV )
                stvout_GetYUVParams ( Unit_p, OutputParams_p);
            if ( Unit_p->Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YC )
                stvout_GetYCParams  ( Unit_p, OutputParams_p);
            if ( Unit_p->Device_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS )
                stvout_GetCVBSParams( Unit_p, OutputParams_p);
            break;
#ifdef STVOUT_HDOUT
        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            OutputParams_p->Analog.EmbeddedType    = Unit_p->Device_p->Out.Analog.EmbeddedType;
            OutputParams_p->Analog.SyncroInChroma  = Unit_p->Device_p->Out.Analog.SyncroInChroma;
            OutputParams_p->Analog.ColorSpace      = Unit_p->Device_p->Out.Analog.ColorSpace;
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :          /* no break */
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :   /* no break */
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :            /* no break */
#endif /* STVOUT_HDOUT */
#if defined(STVOUT_HDOUT) || defined(STVOUT_SDDIG)
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
            OutputParams_p->Digital.EmbeddedType    = Unit_p->Device_p->Out.Digital.EmbeddedType;  /* both HDOUT and SDDIG */
            OutputParams_p->Digital.SyncroInChroma  = Unit_p->Device_p->Out.Digital.SyncroInChroma;
            OutputParams_p->Digital.ColorSpace      = Unit_p->Device_p->Out.Digital.ColorSpace;
            break;
#endif /* STVOUT_HDOUT || STVOUT_SDDIG */
#ifdef STVOUT_SVM
        case STVOUT_OUTPUT_ANALOG_SVM :
            OutputParams_p->SVM.DelayCompensation = Unit_p->Device_p->Out.SVM.DelayCompensation;
            OutputParams_p->SVM.Shape             = Unit_p->Device_p->Out.SVM.Shape;
            OutputParams_p->SVM.Gain              = Unit_p->Device_p->Out.SVM.Gain;
            OutputParams_p->SVM.OSDFactor         = Unit_p->Device_p->Out.SVM.OSDFactor;
            OutputParams_p->SVM.VideoFilter       = Unit_p->Device_p->Out.SVM.VideoFilter;
            OutputParams_p->SVM.OSDFilter         = Unit_p->Device_p->Out.SVM.OSDFilter;
            break;
#endif /* STVOUT_SVM */
#ifdef STVOUT_HDMI
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
            Error = HDMI_GetParams (Unit_p->Device_p->HdmiHandle, &Unit_p->Device_p->Out);
            if (OK(Error))
            {
            OutputParams_p->HDMI.ForceDVI        = Unit_p->Device_p->Out.HDMI.ForceDVI;
                OutputParams_p->HDMI.IsHDCPEnable    = Unit_p->Device_p->Out.HDMI.IsHDCPEnable;
                OutputParams_p->HDMI.AudioFrequency  = Unit_p->Device_p->Out.HDMI.AudioFrequency;
            }
            break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }
    /*stvout_HalGetOutputParams( Unit_p);*/
    return (Error);
}
#if defined(ST_7200)||defined(ST_7111)||defined (ST_7105)
/*******************************************************************************
Name        : stvout_HALVosSetAWGSettings
Description : Set the AWG values
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvout_HALVosSetAWGSettings(stvout_Unit_t * Unit_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    STVTG_Handle_t               VTGHandle;
    STVTG_OpenParams_t           VTGOpenParams;
    STVTG_TimingMode_t           VTGTimingMode;
    STVTG_ModeParams_t           VTGModeParams;
    STVTG_VtgId_t                VTGId;
    U32 ana_cfg;
    U32 * BaseAddress_p;
    U32 Cnt = 0, Mode = 0;

   if (Unit_p->Device_p->DeviceType == STVOUT_DEVICE_TYPE_7200)
   {
       if (Unit_p->Device_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_YUV)
       {
            ErrorCode = STVTG_Open(Unit_p->Device_p->VTGName, &VTGOpenParams, &VTGHandle);
            if ( ErrorCode!= ST_NO_ERROR )
            {
                return(ST_ERROR_OPEN_HANDLE);
            }
            else
            {
                ErrorCode = STVTG_GetVtgId(Unit_p->Device_p->VTGName, &VTGId);

                if (ErrorCode!=ST_NO_ERROR)
                {
                    STVTG_Close(VTGHandle);
                    return(ST_ERROR_BAD_PARAMETER);
                }
                else
                {
                    if (VTGId == STVTG_VTG_ID_1)
                    {
                        ErrorCode = STVTG_GetMode (VTGHandle, &VTGTimingMode, &VTGModeParams);
                        if ( ErrorCode!= ST_NO_ERROR )
                        {
                        STVTG_Close(VTGHandle);
                        return( ST_ERROR_BAD_PARAMETER);
                        }

                        while((AWG_TimingMode[Mode].TimingMode != VTGTimingMode) &&
                                (Mode < AWG_NUMBER_OF_MODE))
                        {
                                Mode++;
                        }
                        if(Mode != AWG_NUMBER_OF_MODE)
                        {
                        BaseAddress_p =(U32*)(AWG_BASE_ADDRESS + AWG_RAM_OFFSET);
                        /* AWG mode found => program & activate it */
                        /*disable AWG*/
                        ana_cfg= stvout_rd32((U32)AWG_BASE_ADDRESS + AWG_CTRL);
                        stvout_wr32((U32)AWG_BASE_ADDRESS + AWG_CTRL, ana_cfg & ~(0x1<<27)  );
                        for (Cnt=0; (Cnt < AWG_RAM_SIZE); Cnt++)
                        {
                            stvout_wr32(BaseAddress_p, AWG_TimingMode[Mode].AWGRamContent[Cnt]);
                            BaseAddress_p++;
                        }

                        /*AWG_CTRL*/
                        ana_cfg= stvout_rd32((U32)AWG_BASE_ADDRESS + AWG_CTRL);
                        stvout_wr32((U32)AWG_BASE_ADDRESS + AWG_CTRL, ana_cfg | 0x1<<27 );

                        }
                        else
                        {
                            ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED ;
                        }
                    }
                }
            }
        /* Close STVTG ...*/
        STVTG_Close(VTGHandle);
      }
   }
  return( ErrorCode);
} /* end of  stvout_HALVosSetAWGSettings*/
#endif
/*
******************************************************************************
Public Functions
******************************************************************************
*/

/*******************************************************************************
Name        :   STVOUT_Disable
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STVOUT_Disable(
                const STVOUT_Handle_t   Handle
                )
{
    ST_ErrorCode_t  ErrorCode;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_Disable :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( !Unit_p->Device_p->VoutStatus )
        {
            ErrorCode = STVOUT_ERROR_VOUT_NOT_ENABLE;
        }
        else
        {
            Unit_p->Device_p->VoutStatus = FALSE;
            switch (Unit_p->Device_p->OutputType)
            {
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
                    ErrorCode = stvout_PowerOffDac( Unit_p->Device_p);
                    if( ErrorCode == ST_NO_ERROR)
                    {
                        ErrorCode = stvout_HalDisable( Unit_p->Device_p);
                    }
                    break;
                default :
                    ErrorCode = stvout_HalDisable( Unit_p->Device_p);
                    break;
            }
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}
/*******************************************************************************
Name        :   STVOUT_Enable
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STVOUT_Enable(
                const STVOUT_Handle_t   Handle
                )
{
    ST_ErrorCode_t  ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_Enable :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( Unit_p->Device_p->VoutStatus )
        {
            ErrorCode = STVOUT_ERROR_VOUT_ENABLE;
        }
        else
        {
            Unit_p->Device_p->VoutStatus = TRUE;
            #if defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105)
            ErrorCode = stvout_HALVosSetAWGSettings(Unit_p);
            #endif
            if (ErrorCode == ST_NO_ERROR) {
                switch (Unit_p->Device_p->OutputType)
                {
                    case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                    case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                    case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                        ErrorCode = halvout_Enable( Unit_p);
                        break;
                    default :
                        ErrorCode = halvout_PowerOnDac( Unit_p);
                        if( ErrorCode == ST_NO_ERROR)
                        {
                            ErrorCode = halvout_Enable( Unit_p);
                        }
                        break;
                }
            }
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}

/*******************************************************************************
Name        :   STVOUT_SetOutputParams
Description :
Parameters  :   Handle, Output Parameters (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetOutputParams(
                const STVOUT_Handle_t           Handle,
                const STVOUT_OutputParams_t*    const OutputParams_p
                )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_SetOutputParams :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( OutputParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            if ( OK(ErrorCode)) ErrorCode = SetOutputParams ( Unit_p, OutputParams_p);
            if ( OK(ErrorCode)) ErrorCode = HalSetOutputParams( Unit_p);
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* STVOUT_SetOutputParams() */

/*******************************************************************************
Name        :   STVOUT_GetOutputParams
Description :
Parameters  :   Handle, Output Parameters (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetOutputParams(
                const STVOUT_Handle_t   Handle,
                STVOUT_OutputParams_t*  const OutputParams_p
                )
{
    ST_ErrorCode_t ErrorCode;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_GetOutputParams :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( OutputParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            ErrorCode = GetOutputParams(Unit_p, OutputParams_p);
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* STVOUT_GetOutputParams() */

/*******************************************************************************
Name        : STVOUT_SetOption
Description :
Parameters  : Handle, Option Parameters (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetOption (
                const STVOUT_Handle_t         Handle,
                const STVOUT_OptionParams_t*  const OptionParams_p
                )
{
    ST_ErrorCode_t ErrorCode;
    stvout_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( OptionParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (OptionParams_p->Option)
            {
                case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:   /* no break */
                case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:   /* no break */
                case STVOUT_OPTION_RGB_SATURATION:         /* no break */
                case STVOUT_OPTION_IF_DELAY:
                    ErrorCode = stvout_HalSetOption(Unit_p->Device_p, OptionParams_p);
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
        }
    }
    return(ErrorCode);
} /* STVOUT_SetOption() */

/*******************************************************************************
Name        : STVOUT_GetOption
Description :
Parameters  : Handle, Option Parameters (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetOption (
                const STVOUT_Handle_t         Handle,
                STVOUT_OptionParams_t*        const OptionParams_p
                )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        if ( OptionParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (OptionParams_p->Option)
            {
                case STVOUT_OPTION_NOTCH_FILTER_ON_LUMA:   /* no break */
                case STVOUT_OPTION_RGB_SATURATION:         /* no break */
                case STVOUT_OPTION_IF_DELAY:
                    ErrorCode = stvout_HalGetOption(Unit_p->Device_p, OptionParams_p);
                    break;
                case STVOUT_OPTION_GENERAL_AUX_NOT_MAIN:
                    OptionParams_p->Enable = Unit_p->Device_p->AuxNotMain;
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
        }
    }
    return(ErrorCode);
} /* STVOUT_GetOption() */
/*******************************************************************************
Name        : STVOUT_SetInputSource
Description :
Parameters  : Handle, Source (MAIN/AUX)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetInputSource(
               const STVOUT_Handle_t         Handle,
               STVOUT_Source_t               Source
               )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_SetInputSource :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        ErrorCode = stvout_HalSetInputSource(Unit_p->Device_p,Source);

    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
  } /* end STVOUT_SetInputSource  */

/*******************************************************************************
Name        : STVOUT_GetDacSelect
Description :
Parameters  : Handle, Dacs selected (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetDacSelect(
               const STVOUT_Handle_t          Handle,
               U8* const                      DacSelect_p
               )

{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_GetDacSelect :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_V13 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_5525 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 :  /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break*/
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 : /* no break */
            case STVOUT_DEVICE_TYPE_7200 :
                *DacSelect_p=Unit_p->Device_p->DacSelect;
                break;
            default :
                *DacSelect_p=0;
                break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end STVOUT_GetDacSelect */
/*******************************************************************************
Name        : STVOUT_AdjustChromaLumaDelay
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_AdjustChromaLumaDelay( const STVOUT_Handle_t     Handle,
                                             const STVOUT_ChromaLumaDelay_t  CLDelay)
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_AdjustChromaLumaDelay :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_V13 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5525 : /*no break*/
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :  /* no break */
            case STVOUT_DEVICE_TYPE_7200 :
                ErrorCode = stvout_HalAdjustChromaLumaDelay(Unit_p->Device_p , CLDelay );
                break;
            default:
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED ;
                break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);



}
#ifdef STVOUT_HDMI
/*******************************************************************************
Name        : STVOUT_GetTargetInformation()
Description : Get the EDID table for different video timing format.
Parameters  : Handle, pointer to target information
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetTargetInformation(
               const STVOUT_Handle_t                Handle,
               STVOUT_TargetInformation_t*    const TargetInformation_p
               )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_GetTargetInformation :");
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /*no break*/
            case STVOUT_DEVICE_TYPE_7015 : /*no break*/
            case STVOUT_DEVICE_TYPE_7020 : /*no break*/
            case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 : /*no break*/
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
            case STVOUT_DEVICE_TYPE_V13 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 :
                 ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 : /* no break */
            case STVOUT_DEVICE_TYPE_7200 :
                 ErrorCode = HDMI_GetSinkInformation (Unit_p->Device_p->HdmiHandle,TargetInformation_p);
                 break;
            default :
                 break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
 } /* end of STVOUT_GetTargetInformation() */

/*******************************************************************************
Name        : STVOUT_GetState()
Description : Get the current state within the internal state machine
Parameters  : Handle, Current state (pointer)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetState(
               const STVOUT_Handle_t         Handle,
               STVOUT_State_t*         const State_p
               )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_GetState :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /*no break*/
            case STVOUT_DEVICE_TYPE_7015 : /*no break*/
            case STVOUT_DEVICE_TYPE_7020 : /*no break*/
            case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 : /*no break*/
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
            case STVOUT_DEVICE_TYPE_V13 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 :
                 ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 : /* no break */
            case STVOUT_DEVICE_TYPE_7200 :
                 switch (Unit_p->Device_p->OutputType)
                 {
                    case STVOUT_OUTPUT_ANALOG_RGB : /*no break*/
                    case STVOUT_OUTPUT_ANALOG_YUV : /*no break*/
                    case STVOUT_OUTPUT_ANALOG_YC :  /*no break*/
                    case STVOUT_OUTPUT_ANALOG_CVBS : /*no break*/
                    case STVOUT_OUTPUT_HD_ANALOG_RGB : /*no break*/
                    case STVOUT_OUTPUT_HD_ANALOG_YUV :
                    case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                    case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                    case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                         if (Unit_p->Device_p->VoutStatus)
                         {
                            *State_p = STVOUT_ENABLED;
                         }
                         else
                         {
                            *State_p = STVOUT_DISABLED;
                         }
                         break;

                    case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888  : /* no break */
                    case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                    case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 : /* no break */
                         ErrorCode = HDMI_GetState(State_p, Unit_p->Device_p->HdmiHandle);
                         break;

                    default :
                         ErrorCode = ST_ERROR_BAD_PARAMETER;
                         break;
                 }
                 break;
            default:
                 break;

        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
 } /* end of STVOUT_GetState() */
/*******************************************************************************
Name        : STVOUT_SendData()
Description : Send Data to the connected device called sink.
Parameters  : Handle, Buffer_p (pointer) and size of the buffer.
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SendData(
               const STVOUT_Handle_t                Handle,
               const STVOUT_InfoFrameType_t         InfoFrameType,
               U8* const                            Buffer_p,
               U32                                  Size
               )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_SendData :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /*no break*/
            case STVOUT_DEVICE_TYPE_7015 : /*no break*/
            case STVOUT_DEVICE_TYPE_7020 : /*no break*/
            case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 : /*no break*/
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
            case STVOUT_DEVICE_TYPE_V13 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 :
                 ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :
            case STVOUT_DEVICE_TYPE_7200 :
                 ErrorCode = HDMI_SendData(Unit_p->Device_p->HdmiHandle, InfoFrameType, Buffer_p, Size);
                 break;
            default:
                 break;

        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STVOUT_SendData */
/*******************************************************************************
Name        : STVOUT_SetHDMIOutputType()
Description : Sets the HDMI output type
Parameters  : OutputType RGB888, YCBCR444 or YCBCR422
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SetHDMIOutputType(
               const STVOUT_Handle_t                Handle,
               STVOUT_OutputType_t                  OutputType
               )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_SetHDMIOutputType :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /*no break*/
            case STVOUT_DEVICE_TYPE_7015 : /*no break*/
            case STVOUT_DEVICE_TYPE_7020 : /*no break*/
            case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 : /*no break*/
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
            case STVOUT_DEVICE_TYPE_V13 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 :
                 ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :
            case STVOUT_DEVICE_TYPE_7200 :
                 ErrorCode = HDMI_SetOutputType (Unit_p->Device_p->HdmiHandle,OutputType);
                 break;
            default:
                 break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STVOUT_SetHDMIOutputType */
/*******************************************************************************
Name        : STVOUT_SetHDCPParams()
Description : Set Device keys for the authentication protocol.
Parameters  : Handle, HDCP parameters (pointer).
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
#ifdef STVOUT_HDCP_PROTECTION
ST_ErrorCode_t STVOUT_SetHDCPParams(
               const STVOUT_Handle_t                Handle,
               const STVOUT_HDCPParams_t*            const HDCPParams_p
               )
{

    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_SetHDCPParams :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;


        if (HDCPParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (Unit_p->Device_p->DeviceType)
           {
                case STVOUT_DEVICE_TYPE_DENC : /*no break*/
                case STVOUT_DEVICE_TYPE_7015 : /*no break*/
                case STVOUT_DEVICE_TYPE_7020 : /*no break*/
                case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
                case STVOUT_DEVICE_TYPE_5528 : /*no break*/
                case STVOUT_DEVICE_TYPE_4629 : /*no break*/
                case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
                case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
                case STVOUT_DEVICE_TYPE_V13 : /* no break */
                case STVOUT_DEVICE_TYPE_5525 :
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                case STVOUT_DEVICE_TYPE_7710 : /* no break */
                case STVOUT_DEVICE_TYPE_7100 :
                case STVOUT_DEVICE_TYPE_7200 :
                    ErrorCode = HDMI_SetHDCPParams(Unit_p->Device_p->HdmiHandle, HDCPParams_p);
                    break;
                default:
                    break;

           }
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /*end STVOUT_SetHDCPParams()*/
/*******************************************************************************
Name        : STVOUT_GetHDCPSinkParams
Description : Retrieve the HDCP sink capabilities
Parameters  : Handle, HDCP Sink parameters (pointer).
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetHDCPSinkParams(
               const STVOUT_Handle_t                Handle,
               STVOUT_HDCPSinkParams_t*             const HDCPSinkParams_p
               )
{

    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_GetHDCPSinkParams :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;


        if (HDCPSinkParams_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (Unit_p->Device_p->DeviceType)
           {
                case STVOUT_DEVICE_TYPE_DENC : /*no break*/
                case STVOUT_DEVICE_TYPE_7015 : /*no break*/
                case STVOUT_DEVICE_TYPE_7020 : /*no break*/
                case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
                case STVOUT_DEVICE_TYPE_5528 : /*no break*/
                case STVOUT_DEVICE_TYPE_4629 : /*no break*/
                case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
                case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
                case STVOUT_DEVICE_TYPE_V13 : /* no break */
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                case STVOUT_DEVICE_TYPE_7710 : /* no break */
                case STVOUT_DEVICE_TYPE_7100 :
                case STVOUT_DEVICE_TYPE_7200 :
                    ErrorCode = HDMI_GetHDCPSinkParams(Unit_p->Device_p->HdmiHandle, HDCPSinkParams_p);
                    break;
                default:
                    break;

           }
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /*end STVOUT_SetHDCPParams()*/
/*******************************************************************************
Name        : STVOUT_UpdateForbiddenKSVs
Description : Update the Forbidden KSVs list
Parameters  : Handle, KSVList_p(pointer) and Size(U32)
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_UpdateForbiddenKSVs(
               const STVOUT_Handle_t                Handle,
               U8* const                            KSVList_p,
               U32 const                            KSVNumber
               )
{

    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_UpdateForbiddenKSVs :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;


        if ((KSVList_p == NULL)||(KSVNumber==0))
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (Unit_p->Device_p->DeviceType)
           {
                case STVOUT_DEVICE_TYPE_DENC : /*no break*/
                case STVOUT_DEVICE_TYPE_7015 : /*no break*/
                case STVOUT_DEVICE_TYPE_7020 : /*no break*/
                case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
                case STVOUT_DEVICE_TYPE_5528 : /*no break*/
                case STVOUT_DEVICE_TYPE_4629 : /*no break*/
                case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
                case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
                case STVOUT_DEVICE_TYPE_V13 : /* no break */
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                case STVOUT_DEVICE_TYPE_7710 : /* no break */
                case STVOUT_DEVICE_TYPE_7100 :
                case STVOUT_DEVICE_TYPE_7200 :
                    ErrorCode = HDMI_UpdateForbiddenKSVs(Unit_p->Device_p->HdmiHandle, KSVList_p,KSVNumber);
                    break;
                default:
                    break;

           }
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /*end STVOUT_UpdateForbiddenKSVs()*/
/*******************************************************************************
Name        : STVOUT_EnableDefaultOutput
Description : Enable the default output.
Parameters  : Handle, DefaultOutput_p(pointer).
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_EnableDefaultOutput(
               const STVOUT_Handle_t                Handle,
               const STVOUT_DefaultOutput_t*        const DefaultOutput_p
               )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_EnableDefaultOutput :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;


        if (DefaultOutput_p == NULL)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            switch (Unit_p->Device_p->DeviceType)
           {
                case STVOUT_DEVICE_TYPE_DENC : /*no break*/
                case STVOUT_DEVICE_TYPE_7015 : /*no break*/
                case STVOUT_DEVICE_TYPE_7020 : /*no break*/
                case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
                case STVOUT_DEVICE_TYPE_5528 : /*no break*/
                case STVOUT_DEVICE_TYPE_4629 : /*no break*/
                case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
                case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
                case STVOUT_DEVICE_TYPE_V13 : /* no break */
                    ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
                    break;
                case STVOUT_DEVICE_TYPE_7710 : /* no break */
                case STVOUT_DEVICE_TYPE_7100 :
                case STVOUT_DEVICE_TYPE_7200 :
                    ErrorCode = HDMI_EnableDefaultOutput(Unit_p->Device_p->HdmiHandle, DefaultOutput_p);
                    break;
                default:
                    break;

           }
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STVOUT_EnableDefaultOutput()*/

/*******************************************************************************
Name        : STVOUT_DisableDefaultOutput
Description : Disable the default output.
Parameters  : Handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_DisableDefaultOutput(
               const STVOUT_Handle_t                Handle
               )
{
    ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_DisableDefaultOutput :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
       {
          case STVOUT_DEVICE_TYPE_DENC : /*no break*/
          case STVOUT_DEVICE_TYPE_7015 : /*no break*/
          case STVOUT_DEVICE_TYPE_7020 : /*no break*/
          case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
          case STVOUT_DEVICE_TYPE_5528 : /*no break*/
          case STVOUT_DEVICE_TYPE_4629 : /*no break*/
          case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
          case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
          case STVOUT_DEVICE_TYPE_V13 : /* no break */
              ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
              break;
          case STVOUT_DEVICE_TYPE_7710 : /* no break */
          case STVOUT_DEVICE_TYPE_7100 :
          case STVOUT_DEVICE_TYPE_7200 :
              ErrorCode = HDMI_DisableDefaultOutput(Unit_p->Device_p->HdmiHandle);
              break;
          default:
              break;
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* end of STVOUT_DisableDefaultOutput()*/
#ifdef WA_GNBvd56512
/*******************************************************************************
Name        : STVOUT_EnableHDCPWA
Description : Enable the HDCP WA on 7100 cut 3.3
Parameters  : Handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_EnableHDCPWA(const STVOUT_Handle_t  Handle)
{
   ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_EnableHDCPWA :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
       {
          case STVOUT_DEVICE_TYPE_DENC : /*no break*/
          case STVOUT_DEVICE_TYPE_7015 : /*no break*/
          case STVOUT_DEVICE_TYPE_7020 : /*no break*/
          case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
          case STVOUT_DEVICE_TYPE_5528 : /*no break*/
          case STVOUT_DEVICE_TYPE_4629 : /*no break*/
          case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
          case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
          case STVOUT_DEVICE_TYPE_V13 : /* no break */
              ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
              break;
          case STVOUT_DEVICE_TYPE_7710 : /* no break */
          case STVOUT_DEVICE_TYPE_7100 :
              ErrorCode = HDMI_WA_GNBvd56512_Install(Unit_p->Device_p->HdmiHandle);
              break;
          default:
              break;
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/* end of STVOUT_EnableHDCPWA()*/
/*******************************************************************************
Name        : STVOUT_DisableHDCPWA
Description : Enable the HDCP WA on 7100 cut 3.3
Parameters  : Handle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_DisableHDCPWA(const STVOUT_Handle_t  Handle)
{
   ST_ErrorCode_t ErrorCode=ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    sprintf( Msg, "STVOUT_DisableHDCPWA :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t*) Handle;

        switch (Unit_p->Device_p->DeviceType)
       {
          case STVOUT_DEVICE_TYPE_DENC : /*no break*/
          case STVOUT_DEVICE_TYPE_7015 : /*no break*/
          case STVOUT_DEVICE_TYPE_7020 : /*no break*/
          case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
          case STVOUT_DEVICE_TYPE_5528 : /*no break*/
          case STVOUT_DEVICE_TYPE_4629 : /*no break*/
          case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /*no break*/
          case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /*no break*/
          case STVOUT_DEVICE_TYPE_V13 : /* no break */
              ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
              break;
          case STVOUT_DEVICE_TYPE_7710 : /* no break */
          case STVOUT_DEVICE_TYPE_7100 :
              ErrorCode = HDMI_WA_GNBvd56512_UnInstall(Unit_p->Device_p->HdmiHandle);
              break;
          default:
              break;
       }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/* end of STVOUT_EnableHDCPWA()*/
 #endif
#endif
/*******************************************************************************
Name        :   STVOUT_Start
Description :
Parameters  :   Handle
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  STVOUT_Start(
                const STVOUT_Handle_t   Handle
                )
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];

    sprintf( Msg, "STVOUT_Start :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        switch (Unit_p->Device_p->OutputType)
        {
          case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
              ErrorCode = HDMI_Start(Unit_p->Device_p->HdmiHandle);
              break;
          default :
              break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/* end of STVOUT_Start */
/*******************************************************************************
Name        :   STVOUT_GetStatistics
Description :
Parameters  :   Handle , STVOUT_Statistics_t(pointer out)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_GetStatistics(
               const STVOUT_Handle_t                Handle,
               STVOUT_Statistics_t*                 const Statistics_p
               )
{
    ST_ErrorCode_t  ErrorCode= ST_NO_ERROR;
    stvout_Unit_t *Unit_p;

    char Msg[100];

    sprintf( Msg, "STVOUT_GetStatistics :");

    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        switch (Unit_p->Device_p->OutputType)
        {
          case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
          case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
              ErrorCode = HDMI_GetStatistics(Unit_p->Device_p->HdmiHandle, Statistics_p);
              break;
          default :
              break;
        }
    }
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}
#endif /* STVOUT_HDMI */

#ifdef STVOUT_CEC_PIO_COMPARE
/*******************************************************************************
Name        :   STVOUT_SendCECMessage
Description :
Parameters  :   Handle , The message to send
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_SendCECMessage(   const STVOUT_Handle_t   Handle,
                                        const STVOUT_CECMessage_t * const Message_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        ErrorCode = CEC_SendMessage(Unit_p->Device_p->HdmiHandle, Message_p );
    }

    sprintf( Msg, "STVOUT_SendCECMessage ");
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/*end of STVOUT_SendCECMessage() */

/*******************************************************************************
Name        :   STVOUT_CECPhysicalAddressAvailable
Description :
Parameters  :   Handle , Physical Address has been obtained
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_CECPhysicalAddressAvailable( const STVOUT_Handle_t   Handle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        CEC_PhysicalAddressAvailable(Unit_p->Device_p->HdmiHandle);
    }

    sprintf( Msg, "STVOUT_CECPhysicalAddressAvailable");
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/*end of STVOUT_CECPhysicalAddressAvailable() */

/*******************************************************************************
Name        : STVOUT_CECSetAdditionalAddress
Description : Used to set additionnal logical address when
               another device (for example PVR) added to actual device
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STVOUT_CECSetAdditionalAddress( const STVOUT_Handle_t   Handle, const STVOUT_CECRole_t Role )
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    stvout_Unit_t *Unit_p;
    char Msg[100];
    if (!IsValidHandle(Handle))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        Unit_p = (stvout_Unit_t *) Handle;
        CEC_SetAdditionalAddress(Unit_p->Device_p->HdmiHandle, Role);
    }

    snprintf( Msg, sizeof(Msg), "STVOUT_CECSetAdditionalAddress");
    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
}/*end of STVOUT_CECSetAdditionalAddress() */


#endif /*STVOUT_CEC_PIO_COMPARE*/

/* End of vout_drv.c */
/* ------------------------------- End of file ---------------------------- */
