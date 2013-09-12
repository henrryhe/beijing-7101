/****************************************************************************

File Name   : pccrd_hal.c

Description : This file contains functions which are either hardware specific
              or local to driver.
History     : Support for the STV0700 and STV0701 is added.
              Ported to 7100 & OS21.

Copyright (C) 2005 STMicroelectronics

Reference  : ST API Definition "STPCCRD Driver API" STB-API-332
****************************************************************************/
/* Includes --------------------------------------------------------------- */
/* Standard Library Includes */
#if defined(ST_OS20) || defined(ST_OS21)
#include <stdio.h>
#include <stdlib.h>
#endif

#if !defined( ST_OSLINUX )
#include "stlite.h"
#include "sttbx.h"
#endif

/* STAPI Includes */

#include "stdevice.h"

#if !defined( ST_OSLINUX )
#include "stsys.h"
#include "sti2c.h"
#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#include "stevt.h"
#endif

/* Private Includes */
#include "stpccrd.h"
#include "pccrd_hal.h"


#ifdef STPCCRD_HOT_PLUGIN_ENABLED

/****************************************************************************
Name         : PCCRDInterruptHandler()

Description  : Interrupt Handler which signals the CardDetectSemaphore

Parameters   : ControlBlkPtr_p : pointer on the controller block data structure

Return Value : None
****************************************************************************/

static void PCCRDInterruptHandler(void* Param_p)
{

   PCCRD_ControlBlock_t        *ControlBlkPtr_p;

   /* Get the Parameter */
   ControlBlkPtr_p= (PCCRD_ControlBlock_t*)Param_p;

#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif

    STOS_SemaphoreSignal(ControlBlkPtr_p->CardDetectSemaphore_p);

#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif

}/* PCCRDInterruptHandler */

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

/****************************************************************************
Name         : PCCRD_HardInit()

Description  : Hardware Controller initialisation

Parameters   : ControlBlkPtr_p  : Pointer on the controller block data structure

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_INTERRUPT_INSTALL    Error In Interrupt Installation
               STPCCRD_ERROR_I2C_READ        Error in reading the I2C Interface

******************************************************************************/

ST_ErrorCode_t PCCRD_HardInit( PCCRD_ControlBlock_t  *ControlBlkPtr_p)
{
    ST_ErrorCode_t  ErrorCode;


#if defined(ST_5516) || defined(ST_5517)
    U32  OriginalControlRegE;
#endif /* (ST_5516) || (ST_5517) */

#if defined(ST_7100) || defined(ST_7109)
    U8 TestRegister;
    U32  EPLDAddress;

#endif /* (ST_7100) */

#if defined(ST_5105) || defined(ST_5107)
    U32  OriginalControlReg;
#endif

#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    U8   RegValue;
#endif /* (ST_5514) || (ST_5518) */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#if !defined(ST_OSLINUX)
    S32   IntReturn;
#endif
    U32   TriggerMode;
    U32   InterruptLvlCntrlOffset;
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    ErrorCode = ST_NO_ERROR;

#if defined(ST_OSLINUX)

    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].CommandBaseAddress = (U32)ControlBlkPtr_p->MappingDVBCIBaseAddress;
    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].CommandBaseAddress = DVB_CI_B_OFFSET + (U32)ControlBlkPtr_p->MappingDVBCIBaseAddress;

    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].AttributeBaseAddress = (U32)ControlBlkPtr_p->MappingDVBCIBaseAddress + DVB_CI_A_REG_MEM_BASE_ADDRESS;
    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].AttributeBaseAddress = DVB_CI_B_OFFSET + \
                                              (U32)ControlBlkPtr_p->MappingDVBCIBaseAddress + DVB_CI_B_REG_MEM_BASE_ADDRESS;
#else
    /* Setting the CI and Memory Base Address */
    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].CommandBaseAddress = DVB_CI_A_REG_IO_BASE_ADDRESS;
    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].CommandBaseAddress = DVB_CI_B_OFFSET + DVB_CI_B_REG_IO_BASE_ADDRESS;

    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_A].AttributeBaseAddress = DVB_CI_A_REG_MEM_BASE_ADDRESS;
    ControlBlkPtr_p->OpenBlkPtr_p[STPCCRD_MOD_B].AttributeBaseAddress = DVB_CI_B_OFFSET + DVB_CI_B_REG_MEM_BASE_ADDRESS;

#endif/*ST_OSLINUX*/

#if defined(ST_5516) || defined(ST_5517)

    /* Configuring the EMI Data Registers */
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA0,EMI_CONFIG_REG0_VALUE);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA1,EMI_CONFIG_REG1_VALUE);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA2,EMI_CONFIG_REG2_VALUE);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA3,EMI_CONFIG_REG3_VALUE);

    /* Enabling the Padlogic Control for DVB-CI */
    OriginalControlRegE = STSYS_ReadRegDev32LE( CFG_BASE_ADDRESS + CONFIG_CONTROL_REG_E ) ;
    STSYS_WriteRegDev32LE( CFG_BASE_ADDRESS + CONFIG_CONTROL_REG_E, OriginalControlRegE |
                           CONFIG_EMISS_PORTSIZEBANK_DVB_CI | CONFIG_EMISS_DVBCI_ENABLE );

    /* Enabling the DVB Logic via I2C Interface */
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U16)DVB_CI_ENABLE);

#endif /* defined(ST_5516) || defined(ST_5517) */

#if defined(ST_5528)

    /* Address shift enable for Bank2 */
    /*STSYS_WriteRegDev32LE((void*)(EMI_GENCFG ), 0x00000080);*/

    /* Configuring the EMI Data Registers */
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA0,0x001016F9);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA1,0x9d200000);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA2,0x9d200000);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA3,0x00000000);

#endif

#if defined(ST_7100) || defined(ST_7109)

#if defined(ST_OSLINUX)
 /* Configuring the EMI Data Registers */

    STSYS_WriteRegDev32LE(ControlBlkPtr_p->MappingEMICFGBaseAddress + EMI_CONFIGDATA0,0x002046F9);
    STSYS_WriteRegDev32LE(ControlBlkPtr_p->MappingEMICFGBaseAddress + EMI_CONFIGDATA1,0xa5a20000);
    STSYS_WriteRegDev32LE(ControlBlkPtr_p->MappingEMICFGBaseAddress + EMI_CONFIGDATA2,0xa5a20000);
    STSYS_WriteRegDev32LE(ControlBlkPtr_p->MappingEMICFGBaseAddress + EMI_CONFIGDATA3,0x0000000a);

    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U16)0xFFFF); /* For new EPLD I2C IO Expander is obsolate - so put the IO to High o.p.*/
    /* Just for test purpose - Expected value should be 0xA5 */
    EPLDAddress =(U32)ControlBlkPtr_p->MappingEPLDBaseAddress + TEST_REG;
    TestRegister = *(U8*)EPLDAddress;
    *(U8*)EPLDAddress = 0x5A;
    TestRegister = *(U8*)EPLDAddress;

    /* Required to have the DVBCI signals linked to the EMI Addresses*/
    TestRegister = *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_CMD_REG);

    /* notSTRB_BUF_sig = 0; notADDROE_sig = 0; notDATAOE_sig = 1;PDDIR_sig = 0;notPDOE_sig = 0?1 */
    TestRegister = TestRegister & (~ NOTSTRB_BUF_SIG)
                                & (~ NOTADDROE_SIG)
                                | (NOTDATAOE_SIG)
                                & (~PDDIR_SIG)
                                & (~NOTPDOE_SIG);

    *(U8*)((U32)ControlBlkPtr_p->MappingEPLDBaseAddress + CARD_CMD_REG) = TestRegister;

#else
    /* Configuring the EMI Data Registers */
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA0,0x002046F9);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA1,0xa5a20000);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA2,0xa5a20000);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA3,0x0000000a);

    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U16)0xFFFF); /* For new EPLD I2C IO Expander is obsolate - so put the IO to High o.p.*/
    /* Just for test purpose - Expected value should be 0xA5 */
    EPLDAddress = TEST_REG;
    TestRegister = *(U8*)EPLDAddress;
    *(U8*)EPLDAddress = 0x5A;
    TestRegister = *(U8*)EPLDAddress;

    /* Required to have the DVBCI signals linked to the EMI Addresses*/
    TestRegister = *(U8*)CARD_CMD_REG;

    /* notSTRB_BUF_sig = 0; notADDROE_sig = 0; notDATAOE_sig = 1;PDDIR_sig = 0;notPDOE_sig = 0?1 */
    TestRegister = TestRegister & (~ NOTSTRB_BUF_SIG)
                                & (~ NOTADDROE_SIG)
                                | (NOTDATAOE_SIG)
                                & (~PDDIR_SIG)
                                & (~NOTPDOE_SIG);

    *(U8*)CARD_CMD_REG = TestRegister;

#endif/*ST_OSLINUX*/
#endif

#if defined(ST_5100)

    /* DVB-CI enable from FMI config register */
    STSYS_WriteRegDev32LE((void*)(FMI_GENCFG ), 0xC0000000);

    /* All banks Enable */
    STSYS_WriteRegDev32LE((void*)(BANKS_ENABLED), 0x4);

    /* Configuring the FMI Data Registers for BANK1 */
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA0_BANK_1, 0x010087f9);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA1_BANK_1, 0x30228822);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA2_BANK_1, 0x30228822);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA3_BANK_1, 0x0000000a);

    /* Enable Io Mode */
    STSYS_WriteRegDev8((void*)(EPLD_DVBCI_ENABLE), 0x01);

    /* Providing 5V to DVBCI modules */
    STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR), 0x0C);

#endif

#if defined(ST_5105) || defined(ST_5107)


    /* DVBCI Mode Enable */
    OriginalControlReg = STSYS_ReadRegDev32LE( CONFIG_CONTROL_C ) ;
    STSYS_WriteRegDev32LE( CONFIG_CONTROL_C, ((OriginalControlReg & 0xFFCFFFFF) | (1<<21)));

    OriginalControlReg = STSYS_ReadRegDev32LE( CONFIG_MONITOR_G ) ;
    STSYS_WriteRegDev32LE(CONFIG_MONITOR_G, OriginalControlReg | (1<<0)); /* DVB_BANK1_ENABLE */

    /* PIO3.5 works as DVBCI_IOWR, PIO3.6 works as DVBCI_IORD */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, (1<<21) | (1<<22));

    /* DVB-CI enable from FMI config register */
    STSYS_WriteRegDev32LE((void*)(FMI_GENCFG ), 0xC0000000 );

    /* All banks Enable */
    STSYS_WriteRegDev32LE((void*)(BANKS_ENABLED), 0x4);

    /* Configuring the FMI Data Registers for BANK1 */
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA0_BANK_1, 0x010087F9);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA1_BANK_1, 0xAe025721);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA2_BANK_1, 0xAe025721);
    STSYS_WriteRegDev32LE((void*)FMI_CONFIGDATA3_BANK_1, 0x0000000a);

    /* Drives PIO3.5 out of the EPLD to the DVBCInotIOWR signal */
    STSYS_WriteRegDev8((U8*)(EPLD_MEMREQ_MDVBCI_IOWR), 0x00);

    /* DVB-CI enable from EPLD DvbciCsAAddr */
    STSYS_WriteRegDev8((U8*)(EPLD_DVBCI_CS), 0x01);

    /* DVB-CI enable from EPLD STEMCS */
    STSYS_WriteRegDev8((U8*)(EPLD_DVBCI_STEMCS), 0x00);

#if defined(STPCCRD_VCC_5V)      /*0x01- 3V ,0x02 - 5V*/
    /* Providing 5V to DVBCI modules */
    STSYS_WriteRegDev8((U8*)(EPLD_EPM3256B_ADDR),0x02);
#else
    /* Providing 5V to DVBCI modules */
    STSYS_WriteRegDev8((U8*)(EPLD_EPM3256B_ADDR),0x01);
#endif

#endif

#if defined(ST_7710)

    /* Configuring EMI Gen for PCCard */
    STSYS_WriteRegDev32LE(EMI_GENCFG,0x00000018);/* enable DVBCI on bank4 only */

    /* All banks Enable */
    STSYS_WriteRegDev32LE((void*)(BANKS_ENABLED), 0x5);

    /* Configuring the EMI Data Registers */
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA0,0x010087f9);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA1,0x30228822);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA2,0x30228822);
    STSYS_WriteRegDev32LE(EMI_BASE_ADDRESS + DVB_CI_BANK_ADDR_OFFSET + EMI_CONFIGDATA3,0x0000000a);

    /* Enable Io Mode */
    STSYS_WriteRegDev8((void*)(EPLD_DVBCI_ENABLE), 0x01);

    /* Providing 5V to DVBCI modules */
    STSYS_WriteRegDev8((void*)(EPLD_EPM3256B_ADDR),0x0C );

#endif

#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)

#if defined(STV0700)

    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,CICNTRL,&RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    RegValue  |= STV0700_CICNTRL_RESET;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,CICNTRL,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* module power on    */
    RegValue  = STV0700_CARD_POWER_ON;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* auto-select mode   */
    RegValue  = STV0700_DSR_AUTOSEL;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,DSR,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* INT output pin configuration  */
    RegValue  = (STV0700_INTCONF_ITDRV|STV0700_INTCONF_ITLVL);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,INTCONF,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* WAIT/ACK pin configuration    */
    RegValue  = (STV0700_UCSG2_WDRV|STV0700_UCSG2_WLVL);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,UCSG2,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* CS pin configuration          */
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,UCSG1,&RegValue);
    RegValue  = (~STV0700_UCSG1_CSLVL) & RegValue;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,UCSG1,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* Automatic selection between the modules A,B      */
    /* or the external device connected on the EXTCS    */
    /* external device selected by default              */

    RegValue  = STV0700_ASMHE_EXTCS;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMHE,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    /* A[25:20] & A[16:15] are masked        */
    /* take care about A[19,18,17]           */
    RegValue  = (STV0700_ASMHA_MA25 & STV0700_ASMHA_MA24 & STV0700_ASMHA_MA23);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMHA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMHB,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    RegValue  = (STV0700_ASMLA_MA19|STV0700_ASMLA_MA18|STV0700_ASMLA_MA17);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMLA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMLB,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* module A selected on @ 0x20000 to @ 0x3FFFF  */
    /* module B selected on @ 0x40000 to @ 0x9FFFF  */
    RegValue  = (STV0700_ASMPHA_PA25 & STV0700_ASMPHA_PA24 & STV0700_ASMPHA_PA23);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMPHA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMPHB,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    RegValue  = STV0700_ASMPLA_PA17;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMPLA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    RegValue  = STV0700_ASMPLB_PA18;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,ASMPLB,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* A 200 ns memory access cycle time */
    RegValue  = (STV0700_MACTA_AM1|STV0700_MACTA_CM1) ;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MACTA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MACTB,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,INTMSK,&RegValue);
    if (ErrorCode == ST_NO_ERROR)
    {
        RegValue  |= (STV0700_INTSTA_DETA|STV0700_INTSTA_DETB);
        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,INTMSK,(U16)RegValue);
        if (ErrorCode != ST_NO_ERROR)
        {
            return (ErrorCode);
        }
    }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    /* lock configuration  */
    RegValue  = STV0700_CICNTRL_LOCK;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,CICNTRL,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,CICNTRL,&RegValue);
    if (ErrorCode == ST_NO_ERROR)
    {
        if ((RegValue & STV0700_CICNTRL_LOCK )== 0x01)
        {
            RegValue  = STV0700_CARD_POWER_ON;
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)RegValue);
            if (ErrorCode != ST_NO_ERROR)
            {
                return (ErrorCode);
            }
        }
    }
#endif  /* ifdef STV0700 */

#if defined(STV0701)

    /* Reset STV0701 */
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,CICNTRL,&RegValue);
    RegValue |= STV0700_CICNTRL_RESET;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,CICNTRL,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* Configure the phase of the MPEG transport stream output clock*/
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,CICNTRL,&RegValue);
    RegValue |= STV0700_CICNTRL_OPHS;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,CICNTRL,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* Module power on */
    RegValue  = (STV0701_PCR_ARSE|STV0701_PCR_VPDRV|STV0701_PCR_VCC|STV0701_PCR_VCLVL|STV0701_PCR_VCDRV);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* manually select module SEL1 =0,SEL0 =1*/
    RegValue  = STV0701_DSR_SEL0;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,DSR,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* INT pin configuration */
    RegValue  = (STV0700_INTCONF_ITLVL|STV0700_INTCONF_ITDRV);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,INTCONF,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* WAIT/ACK pin configuration      */
    RegValue  = (STV0700_UCSG2_WDRV|STV0700_UCSG2_WLVL);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,UCSG2,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* CS pin configuration */
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,UCSG1,&RegValue);
    RegValue  = (~STV0700_UCSG1_CSLVL) & RegValue;
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,UCSG1,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* A 600 ns memory access cycle time */
    RegValue  = (STV0700_MACTA_AM2|STV0700_MACTA_CM2);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MACTA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

    /* Initialisation of module A with AUTO=1, also TS through module enabled */
    RegValue  = (STV0700_MOD_AUTO|STV0700_MOD_TS_BYPASS_DISABLE);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODA,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,INTMSK,&RegValue);
    if (ErrorCode == ST_NO_ERROR)
    {
        RegValue |= (STV0700_INTSTA_DETA);

        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,INTMSK,(U16)RegValue);
        if (ErrorCode != ST_NO_ERROR)
        {
              return (ErrorCode);
        }
     }
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

    /* Lock configuration and phase of the MPEG transport stream */
    RegValue  = (STV0700_CICNTRL_LOCK|STV0700_CICNTRL_OPHS);
    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,CICNTRL,(U16)RegValue);
    if (ErrorCode != ST_NO_ERROR)
    {
        return (ErrorCode);
    }
    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,CICNTRL,&RegValue);
    if (ErrorCode == ST_NO_ERROR)
    {
        if ((RegValue & STV0700_CICNTRL_LOCK) == 0x01)
        {
            RegValue  = STV0700_CARD_POWER_ON;
            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,PCR,(U16)RegValue);
            if (ErrorCode != ST_NO_ERROR)
            {
                return (ErrorCode);
            }
        }
    }

#endif/* End if STV0701 */

#endif /* defined(ST_5514) || defined(ST_5518) */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#if !defined(ST_OSLINUX)
    /* Install the Interrupt only in case of HotPlugIn support */
    IntReturn = interrupt_install(ControlBlkPtr_p->InterruptNumber,
                                  ControlBlkPtr_p->InterruptLevel,
                                  PCCRDInterruptHandler,
                                  ControlBlkPtr_p);
    if (IntReturn == 0)
#else
    printk("installing inetrrupts");
    if(request_irq((int)(ControlBlkPtr_p->InterruptNumber),
                                     PCCRDInterruptHandler,
                                     0,
                                     "STPCCRD",
                                    (void *)&ControlBlkPtr_p)== 0)  /*success*/
#endif/*ST_OSLINUX*/
    {

#if defined(ST_5514) || defined(ST_7100) || defined(ST_7109)|| defined(ST_7710) || defined(ST_5100) \
|| defined(ST_5105) || defined(ST_5528) || defined(ST_5107)

        TriggerMode = RISING_EDGE_TRIG_MODE;

#elif defined(ST_5516)|| defined(ST_5517)

        TriggerMode = FALLING_EDGE_TRIG_MODE;

#endif/* ST_5514 || ST_5516 || ST_5517 */

        /* Setting the Trigger Mode to the necessary Edge */
        InterruptLvlCntrlOffset = ILC_OFFSET + (ControlBlkPtr_p->InterruptNumber)* ILC_OFFSET_MULTIPLIER + ILC_REG_OFFSET;
        STSYS_WriteRegDev32LE( INTERRUPT_LEVEL_CONTROLLER_BASE +InterruptLvlCntrlOffset ,
                               TriggerMode  );

#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif

    }
    else
    {
        ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
    }
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    return (ErrorCode);

}/* End PCCRD_HardInit */

#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
/****************************************************************************
Name         : PCCRD_WriteRegister()

Description  : Write the value 'Data' in the buffer to the I2C Interface

Parameters   : ControlBlkPtr_p  : Pointer on the controller block data structure
               Address          : I2C device address
               Data             : Data to be written at I2C interface

Return Value : ST_NO_ERROR             No errors occurred
               STPCCRD_ERROR_I2C_WRITE Error in writing the I2C Interface

 ****************************************************************************/

ST_ErrorCode_t PCCRD_WriteRegister(PCCRD_ControlBlock_t *ControlBlkPtr_p, U32 Address, U16 Data)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8              BuffLengthToWrite;
    U8              Buffer[I2C_DATA_LENGTH] ;

#if !defined(ST_OSLINUX)
    U32             BuffLengthWritten;
#endif
    BuffLengthToWrite = I2C_DATA_LENGTH;
#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)

    Buffer[0] = Data & LOWER_BYTE;
    Buffer[1] = (Data & UPPER_BYTE)>>SHIFT_EIGHT_BIT;

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)

    Buffer[0] = (U8)Address;
    Buffer[1] = (U8)Data;

#endif /* (ST_5516) || (ST_5517)|| (ST_5514) || (ST_5518)*/

#if !defined(ST_OSLINUX)
    ErrorCode = STI2C_Write( ControlBlkPtr_p->I2C_Handle, Buffer, BuffLengthToWrite, I2C_BUS_WR_TIMEOUT,
                             &BuffLengthWritten );
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STPCCRD_ERROR_I2C_WRITE;
    }
#else
    ControlBlkPtr_p->I2C_Message.flags  = 0;
    ControlBlkPtr_p->I2C_Message.buf    = Buffer;
    ControlBlkPtr_p->I2C_Message.len    = BuffLengthToWrite;
    ErrorCode = i2c_transfer((struct i2c_adapter*)ControlBlkPtr_p->Adapter_299,&(ControlBlkPtr_p->I2C_Message), 1);
    if (ErrorCode <  0)
    {
        printk(KERN_INFO "read_transfer is failed\n");
        ErrorCode = STPCCRD_ERROR_I2C_WRITE;
    }
    else
    {
        ErrorCode = ST_NO_ERROR;
    }

#endif/*ST_OSLINUX*/

    return (ErrorCode);

}/* PCCRD_WriteRegister */

/****************************************************************************
Name         : PCCRD_ReadRegister()

Description  : Read the value 'data' in the buffer from I2C Interface

Parameters   : ControlBlkPtr_p  : Pointer on the controller block data structure
               Address          : I2C device address
               Buffer_p         : Pointer to buffer where data is to be read

Return Value : ST_NO_ERROR            No errors occurred
               STPCCRD_ERROR_I2C_READ Error in reading the I2C Interface

 ****************************************************************************/

ST_ErrorCode_t PCCRD_ReadRegister(PCCRD_ControlBlock_t  *ControlBlkPtr_p, U32 Address, U8* Buffer_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
#if !defined(ST_OSLINUX)
    U8              BuffLengthToRead = I2C_DATA_LENGTH;
    U32             BuffLengthRead   = 0;
#else
    U32             Error;
#endif

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)
#if !defined(ST_OSLINUX)
    ErrorCode = STI2C_Read( ControlBlkPtr_p->I2C_Handle, Buffer_p, BuffLengthToRead,I2C_BUS_RD_TIMEOUT, &BuffLengthRead );
    if (ErrorCode != ST_NO_ERROR)
    {
        ErrorCode = STPCCRD_ERROR_I2C_READ;
    }
#else
    ControlBlkPtr_p->I2C_Message.flags  = I2C_M_RD;
    ControlBlkPtr_p->I2C_Message.buf    = Buffer_p;
    ControlBlkPtr_p->I2C_Message.len    = I2C_DATA_LENGTH;
    Error = i2c_transfer((struct i2c_adapter*)ControlBlkPtr_p->Adapter_299,&(ControlBlkPtr_p->I2C_Message), 1);
    if (Error <  0)
    {
        printk(KERN_INFO "read_transfer is failed\n");
        ErrorCode = STPCCRD_ERROR_I2C_READ;
    }
#endif/*ST_OSLINUX*/

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U32  BuffLengthWritten = 0;

        /* Write 1 byte register offset */
        ErrorCode = STI2C_Write( ControlBlkPtr_p->I2C_Handle,(U8*)&Address,(BuffLengthToRead-1),
                                                               I2C_BUS_WR_TIMEOUT, &BuffLengthWritten );
        if (ErrorCode != ST_NO_ERROR)
        {
            ErrorCode = STPCCRD_ERROR_I2C_WRITE;
        }
        else
        {
            /* Read 1 byte register */
            ErrorCode = STI2C_Read( ControlBlkPtr_p->I2C_Handle, Buffer_p,(BuffLengthToRead-1),I2C_BUS_RD_TIMEOUT,
                                                                                                   &BuffLengthRead );
            if (ErrorCode != ST_NO_ERROR)
            {
                ErrorCode = STPCCRD_ERROR_I2C_READ;
            }
        }
    }

#endif /* (ST_5516) || (ST_5517) || (ST_5514) || (ST_5518)*/

    return (ErrorCode);

}/* PCCRD_ReadRegister */

#endif

/****************************************************************************
Name         : PCCRD_ReadTupple()

Description  : Read a tupple from attribute memory at specified offset

Parameters   : *OpenBlkPtr_p   : Pointer to the OpenBlock data structure,
               TuppleOffset    : Offset of the tupple to be read,
               *Tupple_p       : Pointer to tupple data structure

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               STPCCRD_ERROR_CIS_READ      Error in reading the CIS

 ****************************************************************************/

ST_ErrorCode_t PCCRD_ReadTupple( PCCRD_OpenBlock_t *OpenBlkPtr_p,
                                 U16 TuppleOffset, PCCRD_Tupple_t *Tupple_p)
{
    U32                 TuppleBaseAddress = 0;
    volatile U8         *TuppleAddress_p;
    U8                  TuppleByteCount;
    U16                 ByteOffset = 0;

    /* The tupple reading should not be preempted by other task */
    STOS_TaskLock();

#if !defined(ST_OSLINUX)
    TuppleBaseAddress =  BANK_BASE_ADDRESS; /* Mapped OpenBlkPtr_p->AttributeBaseAddress includes
    													               BANK_BASE_ADDRESS*/
#endif/*ST_OSLINUX*/
    TuppleBaseAddress += OpenBlkPtr_p->AttributeBaseAddress;
    TuppleBaseAddress += (TuppleOffset<<EVEN_ADDRESS_SHIFT);
    TuppleAddress_p   =  (volatile U8*)TuppleBaseAddress;

    Tupple_p->TuppleTag = TuppleAddress_p[0];

    switch (Tupple_p->TuppleTag)
    {
        case CISTPL_DEVICE :
        case CISTPL_DEVICE_OA :
        case CISTPL_DEVICE_OC :
        case CISTPL_VERS_1  :
        case CISTPL_MANFID :
        case CISTPL_CONFIG  :
        case CISTPL_CFTABLE_ENTRY :
        case CISTPL_NO_LINK :
        case CISTPL_17 :
        case CISTPL_FUNCID :
        case CISTPL_END :
            break;
        default :
             STOS_TaskUnlock();
            return (STPCCRD_ERROR_CIS_READ);
    } /* End switch */

    ByteOffset = (1 << EVEN_ADDRESS_SHIFT);
    Tupple_p->TuppleLink = TuppleAddress_p[ByteOffset];

    if ( (Tupple_p->TuppleLink == 0) && (Tupple_p->TuppleTag != CISTPL_NO_LINK) )
    {
        STOS_TaskUnlock();
        return (STPCCRD_ERROR_CIS_READ);
    }

    if (Tupple_p->TuppleTag == CISTPL_END)
    {
        Tupple_p->TuppleLink = 0;
    }

    /* Reading the tupple data */
    for (TuppleByteCount = 0; TuppleByteCount<Tupple_p->TuppleLink; TuppleByteCount++ )
    {
        Tupple_p->TuppleData[TuppleByteCount] = TuppleAddress_p[(TuppleByteCount+2)<<EVEN_ADDRESS_SHIFT];

    }

    Tupple_p->TuppleData[TuppleByteCount++] = '\0';
    Tupple_p->TuppleData[TuppleByteCount++] = '\0';
    Tupple_p->TuppleData[TuppleByteCount++] = '\0';
    Tupple_p->TuppleData[TuppleByteCount]   = '\0';

    STOS_TaskUnlock();

    return (ST_NO_ERROR);

}/* PCCRD_ReadTupple */


/****************************************************************************
Name         : PCCRD_ChangeAccessMode()

Description  : Change the access mode for the specified device

Parameters   : PCCRD_ControlBlock_t  *ControlBlkPtr_p is the current controller
               U32 OpenIndex is the  index of the PCCRDA device on the controller
               STPCCRD_AccessMode_t  AccessMode specifies the mode to set

Return Value : ST_NO_ERROR           No errors occurred

 ****************************************************************************/
ST_ErrorCode_t PCCRD_ChangeAccessMode (PCCRD_ControlBlock_t *ControlBlkPtr_p, U32 OpenIndex,
                                       PCCRD_AccessMode_t  AccessMode)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7710) || defined(ST_5100) || defined(ST_5301) \
|| defined(ST_5105) || defined(ST_7109) || defined(ST_5107)

    ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState = AccessMode;

#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U8    ControllerRegValue;
        U32   I2CAddressOffset;

        /* Changing the AccessMode with STV0700 or STV0701 requires
           changing the Control Register */
#ifdef STV0701
        if (OpenIndex != STPCCRD_MOD_A)
        {
            return( ST_ERROR_BAD_PARAMETER );
        }
#endif
        switch (OpenIndex)
        {
            case STPCCRD_MOD_A:
            {
                I2CAddressOffset = MODA;
                break;
            }
            case STPCCRD_MOD_B:
            {
                I2CAddressOffset = MODB;
                break;
            }
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                return (ErrorCode);

        }/* End of Switch */

        ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2CAddressOffset,&ControllerRegValue);
        if (ErrorCode == ST_NO_ERROR)
        {
            switch (AccessMode)
            {
                case PCCRD_IO_ACCESS :
                {
                    ControllerRegValue &= ~(STV0700_MOD_RESERVED);/* 1st clear bit2&3 */
                    ControllerRegValue |= STV0700_MOD_IO_ACCESS;
                    break;
                }
                case PCCRD_ATTRIB_ACCESS :
                {
                    ControllerRegValue &= ~(STV0700_MOD_RESERVED);
                    break;
                }
                case PCCRD_COMMON_ACCESS :
                {
                    ControllerRegValue &= ~(STV0700_MOD_RESERVED);
                    ControllerRegValue |= STV0700_MOD_COMMON_ACCESS;
                    break;
                }
                default:
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    return (ErrorCode);
            }

            ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2CAddressOffset,(U16)ControllerRegValue);
            if (ErrorCode == ST_NO_ERROR)
            {
                /* Changing the Access mode */
                ControlBlkPtr_p->OpenBlkPtr_p[OpenIndex].AccessState = AccessMode;
            }
        }
    }
#endif /* (ST_5516) || (ST_5517) || (ST_5514) || (ST_5518)*/

    return (ErrorCode);

}/* PCCRD_ChangeAccessMode */

/****************************************************************************
Name         : PCCRD_ControlTS

Description  : Controls the Transport stream flow logic

Parameters   : Handle   Identifies connection with PCCRD Slot(A or B)
               Option   Specifies the condition for TS flow

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                 No errors occurred
               STPCCRD_ERROR_I2C_WRITE     Error in writing the I2C Interface
               STPCCRD_ERROR_I2C_READ      Error in reading the I2C Interface

*****************************************************************************/

ST_ErrorCode_t PCCRD_ControlTS(PCCRD_ControlBlock_t *ControlBlkPtr_p,U8 Option)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_disable (ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_disable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif/* STPCCRD_HOT_PLUGIN_ENABLED */

#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)
    {
        U16    I2CPIOState = 0;

        switch (Option)
        {
            case NO_PCCARD:
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState & DVB_CI_NO_CARD;
                    I2CPIOState = I2CPIOState | DVB_CI_NO_CARD_MASK| DVB_CI_CD_A_B;

                    /* Writing to the contol pins */
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);

                }
                break;

            case PCCARD_A_ONLY:
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState & DVB_CI_SLOT_A_ONLY;
                    I2CPIOState = I2CPIOState | DVB_CI_SLOT_A_ONLY_MASK| DVB_CI_CD_A_B;

                    /* Writing to the contol pins */
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                }
                break;

            case PCCARD_B_ONLY:
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState & DVB_CI_SLOT_B_ONLY;
                    I2CPIOState = I2CPIOState | DVB_CI_SLOT_B_ONLY_MASK| DVB_CI_CD_A_B;

                    /* Writing to the contol pins */
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                }
                break;

            case PCCARD_A_AND_B:
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,(U8*)&I2CPIOState);
                if (ErrorCode == ST_NO_ERROR)
                {
                    I2CPIOState = I2CPIOState & DVB_CI_SLOT_A_AND_B;
                    I2CPIOState = I2CPIOState | DVB_CI_SLOT_A_AND_B_MASK | DVB_CI_CD_A_B;

                    /* Writing to the contol pins */
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,I2C_ADDRESS_OFFSET,I2CPIOState);
                }
                break;

            default :
                /* Error */
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;

        }/* switch */
    }
#elif defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
    {
        U8   ControllerRegValue;

        switch (Option)
        {
            case NO_PCCARD:
            {
                /*  ByPass Enable for both cards */
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODA,&ControllerRegValue);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControllerRegValue= ControllerRegValue & (~STV0700_MOD_TS_BYPASS_DISABLE);
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODA,(U16)ControllerRegValue);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return (ErrorCode);
                    }
#ifndef STV0701
                    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODB,&ControllerRegValue);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControllerRegValue = ControllerRegValue & (~STV0700_MOD_TS_BYPASS_DISABLE);
                        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODB,(U16)ControllerRegValue);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            return (ErrorCode);
                        }
                    }
#endif /* STV0701*/
                }
                break;
            }
            case PCCARD_A_ONLY:
            {
                /* ByPass Enable for B and Disable for A */
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODA,&ControllerRegValue);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControllerRegValue = ControllerRegValue | STV0700_MOD_TS_BYPASS_DISABLE;
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODA,(U16)ControllerRegValue);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return (ErrorCode);
                    }
#ifndef STV0701
                    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODB,&ControllerRegValue);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControllerRegValue = ControllerRegValue & (~STV0700_MOD_TS_BYPASS_DISABLE) ;
                        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODB,(U16)ControllerRegValue);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            return (ErrorCode);
                        }
                    }
#endif/* STV0701 */
                }
                break;
            }
            case PCCARD_B_ONLY:
            {
                /* ByPass Enable for A and Disable for B */
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODB,&ControllerRegValue);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControllerRegValue = ControllerRegValue | STV0700_MOD_TS_BYPASS_DISABLE;
                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODB,(U16)ControllerRegValue);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return (ErrorCode);
                    }
                    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODA,&ControllerRegValue);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControllerRegValue = ControllerRegValue & (~STV0700_MOD_TS_BYPASS_DISABLE);
                        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODA,(U16)ControllerRegValue);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            return (ErrorCode);
                        }
                    }
                }
                break;
            }
            case PCCARD_A_AND_B:
            {
                /* ByPass Disable for both */
                ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODB,&ControllerRegValue);
                if (ErrorCode == ST_NO_ERROR)
                {
                    ControllerRegValue = ControllerRegValue | STV0700_MOD_TS_BYPASS_DISABLE;

                    ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODB,(U16)ControllerRegValue);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        return (ErrorCode);
                    }

                    ErrorCode = PCCRD_ReadRegister(ControlBlkPtr_p,MODA,&ControllerRegValue);
                    if (ErrorCode == ST_NO_ERROR)
                    {
                        ControllerRegValue = ControllerRegValue | STV0700_MOD_TS_BYPASS_DISABLE ;
                        ErrorCode = PCCRD_WriteRegister(ControlBlkPtr_p,MODA,(U16)ControllerRegValue);
                        if (ErrorCode != ST_NO_ERROR)
                        {
                            return (ErrorCode);
                        }
                    }
                }
                break;
            }
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;

        }/* switch */
    }
#elif  defined(ST_5100) || defined(ST_5301) || defined(ST_7710)
    {
        switch (Option)
        {
            case NO_PCCARD:
                /*  ByPass Enable for both cards */
                STSYS_WriteRegDev32LE((void*)(EPLD_DVBCI_TSMUX), 0x00000000); /* TS Mode 0 */
                break;

            case PCCARD_B_ONLY:
                /* ByPass Enable for B and Disable for A */
                STSYS_WriteRegDev32LE((void*)(EPLD_DVBCI_TSMUX), 0x00020002);  /* TS Mode 2 */
                break;

            case PCCARD_A_AND_B:
                /* ByPass Disable for both */
                STSYS_WriteRegDev32LE((void*)(EPLD_DVBCI_TSMUX), 0x00010001);  /* TS Mode 1 */
                break;

            case PCCARD_A_ONLY:
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;

        }/* switch */

    }
#elif defined(ST_5105) || defined(ST_5107)

    switch (Option)
    {
        case NO_PCCARD:
            /* Bypass DVBCI */
            STSYS_WriteRegDev8((void*)(EPLD_DVBCI_TSMUX), 0x00);
            break;

         case PCCARD_A_ONLY:
            /* Through DVBCI */
            STSYS_WriteRegDev8((void*)(EPLD_DVBCI_TSMUX), 0x01);
            break;

         case PCCARD_A_AND_B:
         case PCCARD_B_ONLY:
         default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
     }


#endif /* ST_5516 || ST_5517 || ST_5514 || ST_5518 */

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifndef ST_OS21
#ifdef STAPI_INTERRUPT_BY_NUMBER
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#else
    /* Disable the Interrupt */
    interrupt_enable(ControlBlkPtr_p->InterruptLevel);
#endif
#else /* ST_OS21 */
    interrupt_enable_number(ControlBlkPtr_p->InterruptNumber);
#endif
#endif/* STPCCRD_HOT_PLUGIN_ENABLED */

    return (ErrorCode);

}/* PCCRD_ControlTS */
/*********************************************************************************************/
/* End of File */
