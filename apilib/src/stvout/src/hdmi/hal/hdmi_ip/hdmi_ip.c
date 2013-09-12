/*******************************************************************************
File name   : hdmi_ip.c

Description : HAL HDMI source file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
25 Mar 2004        Created                                          DB
08 Apr 2004        Updated                                          AC
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#if defined(ST_OSLINUX)
#include "stvout_core.h"
#endif

#include "sttbx.h"
#include "stsys.h"


#include "stdevice.h"
#include "hdmi_api.h"
#include "hdmi_ip.h"
#include "hal_hdmi.h"
#include "stddefs.h"
#include "stcommon.h"
/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define HAL_Context_p ((HALHDMI_Properties_t *) Handle)

#define DEFAULT_EXTS_MAX_DELAY  0x00000002      /* Default maximum time between extented control periods*/
#define DEFAULT_EXTS_MIN        0x00000032      /* Default minimum duration of extended control periods */
#define DLL_CONFIG_MASK         0x00000C00      /* Dll config ratio bit mask */
#define DLL_CONFIG_SD           0x00000C00      /* Dll config ratio for Analog SD outputs*/
#define DLL_CONFIG_HD           0x00000800      /* Dll config ratio for Analog HD outputs*/
#define HDMI_CONFIG_RESET       0x00000000      /* all the configuratiion bits for the HDMI cell are in reset state */
#define HDCP_CNTRL_RESET        0x00000000      /* all bits of the HDCP control register are in reset state */

#if defined(ST_7100)||defined(ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define SYS_CONFIG_STATUS_6     0x00000020     /* system_status6 offset register*/
#define CLKB_BASE_ADDRESS       0xB9000000     /* ClkB base address */
#define CKGB_CLK_EN             0x000000B0     /* Clocks enable */
#define CLK_TMDS_HDMI_EN        0x00000020     /* Clock TMDS HDMI enabled */
#define CLK_PIX_HDMI_EN         0x00000400     /* Clock pixel HDMI  */
#define CKGB_CLK_EN_MASK        0x00003FFF     /* Clocks enable mask*/
#define HDCP_FUSE_BASE_ADDRESS  0xB9208000     /* HDCP Antifuse Configuration Base Address */
#define HDCP_FUSE_OFFSET        0x00000010     /* HDCP Antifuse base Address */
#define HDCP_FUSE_ENABLE        0x01000000     /* HDCP Fuse enable */

#ifndef AUD_CONF_BASE_ADDRESS
#define AUD_CONF_BASE_ADDRESS   0xB9210000   /* Audio config Base address */
#endif
#define AUD_SPDIF_BASE_ADDRESS  0xB8103000   /* Audio Spdif Base Address */
#define AUD_FSYN2_PROG_EN       0x0000003C   /* Frequency synthesizer channel 2 program enable*/
#define AUD_IO_CTRL             0x00000200   /* Audio outputs control */
#define AUD_SPDIF_CTRL          0x0000001C   /* SPDIF player control */
#define AUD_SPDIF_IT_EN_SET     0x00000014   /* SPDIF player interrupt enable set */
#define AUD_SPDIF_RST           0x00000000   /* SPDIF player soft reset */

#define AUD_PROG_ENABLED        0x00000001   /* The new configuration was taken into account for the Frequency synth 2 */
#define AUD_PROG_DISABLED       0x00000000   /* The new configuration was not taken into account */
#define AUD_IO_CTRL_VALUE       0x00000008   /* Controls the direction of the audio output pins. */
#define AUD_SPDIF_CTRL_VALUE    0x0400004b   /* Controls the PCM play */
#define AUD_SPDIF_SRSTP         0x00000001   /* Soft reset the S/PDIF player */

#define AUD_SPDIF_END_OF_PD     0x00000010   /* AUDIO Spdif end of PD */
#define AUD_SPDIF_END_OF_LAT    0x00000008   /* AUDIO Spdif end of Latency */
#define AUD_SPDIF_END_OF_BLOCK  0x00000004   /* AUDIO Spdif end of block */
#define AUD_SPDIF_END_OF_DATA   0x00000002   /* AUDIO Spdif end of data burst */
#define AUD_SPDIF_FIFO_UF       0x00000001   /* AUDIO Spdif FIFO under Flow */
#endif
#ifdef ST_7710
#define AUD_DEC_BASE_ADDRESS    0x38211000   /* Audio Decode base address */
#define ADSC_BASE_ADDRESS       0x38210000   /* Audio data stream controller base address */
/* ------------------------------
 * MMDSP registers---------------
 * ------------------------------*/
#define AUD_SOFTRESET           0x00000040   /* Audio soft reset  */
#define AUD_BREAKPOINT          0x0000002B   /* Audio break point */
#define AUD_CLOCKCMD            0x0000003A   /* Audio clock cmd */
#define SPDIF_SIN_SETUP         0x00000030   /* Audio sin setup */
#define SPDIF_CAN_SETUP         0x00000034   /* Audio converter setup */
#define SPDIF_MUTE              0x00000050   /* Audio mute */
#define SPDIF_PCMDIVIDER        0x00000150   /* Audio PCM divider */
#define SPDIF_PCM_CONF          0x00000154   /* Audio PCM Configuration */
#define SPDIF_PCM_CROSS         0x00000158   /* Audio PCM cross */
#define SPDIF_CMD               0x00000178   /* Audio S/PDIF commands */
#define SPDIF_CONF              0x00000180   /* Audio S/PDIF configuration */
#define SPDIF_STATUS            0x00000184   /* Audio S/PDIF Status */
#define SPDIF_REP_TIME          0x000001D4   /* Audio S/PDIF repetition time of a pause frame */
#define SPDIF_LATENCY           0x000001F8   /* Audio S/PDIF Latency */
#define SPDIF_SRC_HANDSHAKE     0x000002DC   /* Audio S/PDIF Latency, Allows the input of a new SRC coefficient */
#define AUD_RUN                 0x000001C8   /* Audio run */
#define AUD_PLAY                0x0000004C   /* Audio Play */
#define AUD_Mute                0x00000050   /* Audio mute */
/* ---------------------------------
 * ADSC cell registers--------------
 * --------------------------------*/
#define AUD0_PKT                0x0000002C   /* Audio Chunk size configuration */
#define AUD0_STA                0x00000040   /* Audio Status */
#define AUD0_SER                0x00000050   /* Audio Serializer configuration */
#define PCMI_CFG                0x00000420   /* Audio PCMI - serial input configuration */
#define I2S_MDEC_CFG            0x00000500   /* Audio I2S main decoder configuration */
#define CKG0_CFG                0x00000600   /* Audio Clock - generation configuration  */
#define CKG0_MD                 0x00000604   /* Audio Clock - frequency synthesizer MD parameter */
#define CKG0_PE                 0x00000608   /* Audio Clock - frequency synthesizer PE parameter */
#define CKG0_SDIV               0x0000060C   /* Audio Clock - frequency synthesizer SDIV parameter */
#define CKG0_PSEL               0x00000610   /* Audio Clock - generation parameter selection */
#define CKG0_PROG               0x00000614   /* Audio Clock - generation parameter validation */
#define IO_CFG                  0x00000700   /* Audio CFG -audio I/O configuration */
#define ADAC_CFG                0x00000800   /* Audio ADAC - audio DAC configuration */
#endif

#define HDMI_PHY_LCK_STA        0x00000124   /* HDMI Physical lock status */

#if defined(ST_7100)||defined(ST_7109)
#define SYS_CFG3                0x0000010C   /* System Configuration 3 */
#define DSP_CFG_CLK             0x00000070   /* DSP cfg clock */
#define DVP_REF_CONF            0x00000040
#define HDMI_IDDQ               0x00000080   /* HDMI serializer power down (bus = high-impedance) */

#define HDMI_CFG                0x00000002   /* RGB YCbCr */

#define HDMI_RERIALIZER_RST     0xFFFFF7FF    /* HDMI serializer reset */
#define PLL_POWERED_DOWN        0xFFFFEFFF    /* Pll is powered down */
#define PLL_S_HDMI_EN           0x00001000   /* Pll HDMI enabled */
#define S_HDMI_RST_N            0x00000800   /* serializer on (DLL) */
#define HDMI_PLL_LOCKED         0x00000002   /* Pll locked */
#define HDMI_DLL_LOCKED         0x00000001   /* DLL locked */
#endif

#if defined (ST_7710)
#define DSP_CFG_CLK             0x00000070   /* DSP cfg clock */
#define HDMI_CFG                0x00000002   /* RGB YCbCr */

#define PLL2_CONFIG_2           0x00000098   /* Pll2 Config2 */
#define MICROSYS_GLUE_BASE_ADD  0x38201000   /* Micro system Glue Base address */
#define MICROSYS_GLUE_OFFSET    0x0000000C   /* Micro system offset */
#define HDMI_CELL_OFF           0x00000080   /* HDMI Cell Off */
#define UNLOCK_KEY              0x5aa50000   /* Unlock register to have write access */
#define RESET_CTRL_1            0x000000D4   /* Reset Control 1 */
#define PLL2_CONFIG_2_NRST      0x00000004   /* reset */
#define PLL2_CONFIG_2_POFF      0x00000002   /* Pll is powered off */
#define UC_RESET_TMDS           0x00000001   /* Send a reset to the TMDS macrocell. */
#define UC_RESET_LMIPL          0x00000002   /* Send a reset to the LMI padlogic. */
#endif
#define VTG1_DRST               0x00000024   /* VTG1 raster reset offset */
#define VTG1_RST_ENABLED        0x00000001

#define HDMI_IT_CLEAR_ALL                 0x0000007E
#define HDMI_IT_DISABLE_ALL               0x00000000
#if defined(ST_7200)|| defined (ST_7111)|| defined (ST_7105)
#define I2S_2_SPDIF_BASE_ADDRESS 0xFD106000
#else
#define I2S_2_SPDIF_BASE_ADDRESS 0xB8103800
#endif

#define CHANNEL_STATUS_REG_0     0x108
#define CHANNEL_STATUS_REG_1     0x10C
#define CHANNEL_STATUS_REG_2     0x110
#define CHANNEL_STATUS_REG_3     0x114
#define CHANNEL_STATUS_REG_4     0x118
#define CHANNEL_STATUS_REG_5     0x11C

#ifdef ST_7710
/* Activate I2S to SPDIF converter */
#define I2S2SPDIF_1 0x20103200
#define I2S2SPDIF_2 0x20103A00
/* Temporary here should be integrated inside the audio driver */
/* Common status bit */
#define CHAN_REG_0 0x20103800+0x108+4*0
#define CHAN_REG_1 0x20103800+0x108+4*1
#define CHAN_REG_2 0x20103800+0x108+4*2
#define CHAN_REG_3 0x20103800+0x108+4*3
#define CHAN_REG_4 0x20103800+0x108+4*4
#define CHAN_REG_5 0x20103800+0x108+4*5
/* Perform reset */
#define I2S2SPDIF_Reset 0x20103800
#endif
#if defined(ST_7200)||defined(ST_7111)||defined (ST_7105)
#define DIG1_CFG             0x100
#define TVO_SYNC_SEL         0x08
#define CKGB_FS0_CLK2_CFG    0x10
#define HDMI_PLL_LOCKED      0x00000004   /* Pll locked */
#define HDMI_DLL_LOCKED      0x00000020   /* DLL locked */
#define TVO_SYNC_SEL_HDMI    0x30
#define DIG1_SEL_INPUT_RGB   0x01
#define PLL_POWERED_DOWN        0xFFFFFFFD   /* Pll is powered down */
#define HDMI_RERIALIZER_RST     0xFFFFFFFE   /* HDMI serializer reset */
#define SYS_CFG3                0x0000010C   /* System Configuration 3 */
#define PLL_S_HDMI_EN           0x00000002   /* Pll HDMI enabled */
#define S_HDMI_RST_N            0x00000001   /* serializer on (DLL) */
#endif
/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */
#if defined(STVOUT_STATE_MACHINE_TRACE)
extern U32 PIO_Int_Time;
#endif
extern HDMI_Data_t *HDMI_Driver_p;
/* Private Macros ----------------------------------------------------------- */
#define HDMI_Read32(a)      STSYS_ReadRegDev32LE((void*)(a))
#define HDMI_Write32(a,v)   STSYS_WriteRegDev32LE((void*)(a),(v))
#define HDMI_Read8(a)       STSYS_ReadRegDev8((void*)(a))
#define HDMI_Write8(a,v)    STSYS_WriteRegDev8((void*)(a),(v))


#define GetPos(BitPos)  ((BitPos)==PIO_BIT_0 ? 0 :    \
                         (BitPos)==PIO_BIT_1 ? 1 :    \
                         (BitPos)==PIO_BIT_2 ? 2 :    \
                         (BitPos)==PIO_BIT_3 ? 3 :    \
                         (BitPos)==PIO_BIT_4 ? 4 :    \
                         (BitPos)==PIO_BIT_5 ? 5 :    \
                         (BitPos)==PIO_BIT_6 ? 6 : 7 )

/* Private Function prototypes ---------------------------------------------- */



/* Functions ---------------------------------------------------------------- */

/* ----------------------------- Init/Term Management------------------------ */

/*******************************************************************************
Name        : HAL_PioIntHandler
Description : Pio interrupt Handler
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*********************************************************************************/
void HAL_PioIntHandler(const STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)
{
  STPIO_Compare_t             PioCompare;
  U8                          PioBits;
  HALHDMI_IP_Properties_t     *HALHDMI_IP_Data_p = ((HALHDMI_Properties_t *)(HDMI_Driver_p->HalHdmiHandle))->PrivateData_p;

#if defined(STVOUT_STATE_MACHINE_TRACE_PIO_TIME)
PIO_Int_Time = time_now();
#endif
    UNUSED_PARAMETER(Handle);
    UNUSED_PARAMETER(ActiveBits);

    STPIO_Read(HALHDMI_IP_Data_p->PioHandle, &PioBits);
    /* Changing the toggling state for the HDP Interrutp */
    PioCompare.CompareEnable = HALHDMI_IP_Data_p->HPD_Bit;
    PioCompare.ComparePattern = PioBits;
    STPIO_SetCompare(HALHDMI_IP_Data_p->PioHandle, &PioCompare);

    if (HALHDMI_IP_Data_p->IsHPDInversed)
    {
        HDMI_Driver_p->IsReceiverConnected = (!(PioBits & HALHDMI_IP_Data_p->HPD_Bit)) ? TRUE : FALSE;
    }
    else
    {
        HDMI_Driver_p->IsReceiverConnected = ((PioBits & HALHDMI_IP_Data_p->HPD_Bit)) ? TRUE : FALSE;

    }

    if (HDMI_Driver_p->InternalTaskStarted)
    {
        PushControllerCommand(HDMI_Driver_p, CONTROLLER_COMMAND_HPD);
        STOS_SemaphoreSignal(HDMI_Driver_p->ChangeState_p);
    }
} /* end of HAL_PioIntHandler ()*/
/*******************************************************************************
Name        : HAL_Init
Description : DVI/HDMI internal cell HAL initialization.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if OK.
              ST_ERROR_NO_MEMORY if memory allocation problem.
              ST_ERROR_BAD_PARAMETER for any other problem.
*******************************************************************************/
ST_ErrorCode_t Init(const HAL_Handle_t  Handle)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STPIO_OpenParams_t          PioOpenParams;
    U8                          PioBits;
    STPIO_Compare_t             PioCompare;
    HALHDMI_IP_Properties_t *   HALHDMI_IP_Data_p;

    /* Allocate a HALHDMI structure */
    ((HALHDMI_Properties_t*) Handle)->PrivateData_p = memory_allocate(((HALHDMI_Properties_t*) Handle)->CPUPartition_p,
                                                      sizeof(HALHDMI_IP_Properties_t));

    if (((HALHDMI_Properties_t*) Handle)->PrivateData_p == NULL)
    {
         return(ST_ERROR_NO_MEMORY);
    }
    HALHDMI_IP_Data_p = (HALHDMI_IP_Properties_t *)(((HALHDMI_Properties_t*) Handle)->PrivateData_p);

    /* Save relevant data. */
    HALHDMI_IP_Data_p->CPUPartition_p   = ((HALHDMI_Properties_t*) Handle)->CPUPartition_p;
#if !defined(ST_OSWINCE)
    PioOpenParams.IntHandler            = HAL_PioIntHandler;
#else
    PioOpenParams.IntHandler            = NULL;
#endif /* ST_OSWINCE*/
    /* Now initialize hot plug detection mechanism. */
    /* Open the STPIO instance to manage Hot plug in signal*/
    HALHDMI_IP_Data_p->HPD_Bit          = ((HALHDMI_Properties_t*) Handle)->HPD_Bit;
    HALHDMI_IP_Data_p->IsHPDInversed    = ((HALHDMI_Properties_t*) Handle)->IsHPDInversed;
    strncpy(HALHDMI_IP_Data_p->PIODevice,  ((HALHDMI_Properties_t*) Handle)->PIODevice, sizeof(HALHDMI_IP_Data_p->PIODevice));

    PioOpenParams.ReservedBits                                     = HALHDMI_IP_Data_p->HPD_Bit;
    PioOpenParams.BitConfigure[GetPos(HALHDMI_IP_Data_p->HPD_Bit)] = STPIO_BIT_INPUT;
    ErrorCode = STPIO_Open(HALHDMI_IP_Data_p->PIODevice, &PioOpenParams, &HALHDMI_IP_Data_p->PioHandle);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ST_ERROR_OPEN_HANDLE);
    }
    ErrorCode = STPIO_Read(HALHDMI_IP_Data_p->PioHandle, &PioBits);

#if !defined(ST_OSWINCE)
    /*Changing the toggling state for the HPD Interrutp */
    PioCompare.CompareEnable = HALHDMI_IP_Data_p->HPD_Bit;
    PioCompare.ComparePattern = PioBits ^ HALHDMI_IP_Data_p->HPD_Bit;
    ErrorCode |= STPIO_SetCompare(HALHDMI_IP_Data_p->PioHandle, &PioCompare);
#endif
    return(ErrorCode);

}/* End of Init() function. */
/*******************************************************************************
Name        : Term
Description : DVI/HDMI internal cell HAL term.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR.
*******************************************************************************/
ST_ErrorCode_t Term(const HAL_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HALHDMI_IP_Properties_t *   HALHDMI_IP_Data_p;
    HALHDMI_IP_Data_p = (HALHDMI_IP_Properties_t *)(((HALHDMI_Properties_t*) Handle)->PrivateData_p);

    /* Close currently opened pio instance. */
    ErrorCode = STPIO_Close (HALHDMI_IP_Data_p->PioHandle);

    if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_BAD_PARAMETER);

    /* De-allocate last: data inside cannot be used after ! */
    memory_deallocate (HALHDMI_IP_Data_p->CPUPartition_p, (void*)HALHDMI_IP_Data_p);

    return(ST_NO_ERROR);

} /* End of Term() function. */

/*---------------------------- Interface Management -----------------------------*/
/*******************************************************************************
Name        : HAL_ControlInterface
Description : Enbale
Parameters  : Handle, Enable/Disable the HDMI cell
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_ControlInterface(const HAL_Handle_t  Handle, BOOL Enable)
{
    U32 UValue;
    #if defined(ST_7100)||defined(ST_7109)|| defined (ST_7710)
    U32 UValue1;
    #endif

    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG);
    if (Enable)
    {
        UValue |= HDMI_CONFIG_DEVICE_ENABLE;
        /* for leakage currents */
        #if defined(ST_7100)||defined(ST_7109)
          UValue1 = HDMI_Read32(VOS_BASE_ADDRESS+DSP_CFG_CLK);
		  UValue1 &= ~HDMI_IDDQ;
          HDMI_Write32(VOS_BASE_ADDRESS+DSP_CFG_CLK, UValue1);
        #endif
        #ifdef ST_7710
          UValue1 = HDMI_Read32(MICROSYS_GLUE_BASE_ADD+MICROSYS_GLUE_OFFSET);
          UValue1 &= ~HDMI_CELL_OFF;
          HDMI_Write32(MICROSYS_GLUE_BASE_ADD+MICROSYS_GLUE_OFFSET, UValue1);
        #endif

    }
    else
    {
        UValue &= ~HDMI_CONFIG_DEVICE_ENABLE;
        /* for leakage currents */
        #if defined(ST_7100)||defined(ST_7109)
          UValue1 = HDMI_Read32(VOS_BASE_ADDRESS+DSP_CFG_CLK);
          UValue1 |= HDMI_IDDQ;
          HDMI_Write32(VOS_BASE_ADDRESS+DSP_CFG_CLK, UValue1);
        #endif
        #ifdef ST_7710
          UValue1 = HDMI_Read32(MICROSYS_GLUE_BASE_ADD+MICROSYS_GLUE_OFFSET);
          UValue1 |= HDMI_CELL_OFF;
          HDMI_Write32(MICROSYS_GLUE_BASE_ADD+MICROSYS_GLUE_OFFSET, UValue1);
        #endif
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG, UValue& HDMI_CONFIG_MASK);

} /* end of HAL_ControlInterface() function */

/*******************************************************************************
Name        : HAL_Reset
Description : Reset the HDMI ip to enable the HDMI interface
Parameters  : Handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_ResetEnable(const HAL_Handle_t Handle)
{

#if defined(ST_7100)||defined(ST_7109)
    U32 UValue;
	U32 Enabled;
    U32 Mili_Second;

    Mili_Second = ST_GetClocksPerSecond()/1000;
    UNUSED_PARAMETER(Handle);
    /* Disable HDMI cell */
    STOS_InterruptLock();
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG);
	Enabled = UValue & HDMI_CONFIG_DEVICE_ENABLE;
    UValue &= ~HDMI_CONFIG_DEVICE_ENABLE;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG, UValue& HDMI_CONFIG_MASK);
    STOS_InterruptUnlock();

	/* Make sure clocks TMDS_HDMI and PIX_HDMI are running */
    STOS_InterruptLock();
    UValue = HDMI_Read32(CLKB_BASE_ADDRESS+ CKGB_CLK_EN);
    UValue |= (CLK_TMDS_HDMI_EN | CLK_PIX_HDMI_EN);
    HDMI_Write32(CLKB_BASE_ADDRESS+ CKGB_CLK_EN, UValue);
    STOS_InterruptUnlock();

	/* PLL and Serializer reset */
    /* HDMI serializer reset and pll power down */
    STOS_InterruptLock();
    UValue  = HDMI_Read32(CFG_BASE_ADDRESS + SYS_CFG3);
    UValue &= (PLL_POWERED_DOWN & HDMI_RERIALIZER_RST);
    HDMI_Write32(CFG_BASE_ADDRESS + SYS_CFG3, UValue);
    STOS_InterruptUnlock();

    /* Wait for PLL and DLL flags falling down. */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(VOS_BASE_ADDRESS+HDMI_PHY_LCK_STA);
    } while (UValue != 0);

    STOS_TaskDelay(Mili_Second);

    /* Power up PLL*/
    STOS_InterruptLock();
    UValue = HDMI_Read32(CFG_BASE_ADDRESS + SYS_CFG3);
    HDMI_Write32( CFG_BASE_ADDRESS + SYS_CFG3, UValue | PLL_S_HDMI_EN); /* PLL_S_HDMI_EN bit */
    STOS_InterruptUnlock();

    /* Wait for PLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(VOS_BASE_ADDRESS+HDMI_PHY_LCK_STA) & HDMI_PLL_LOCKED;
    } while (UValue != HDMI_PLL_LOCKED) ;

    STOS_TaskDelay(Mili_Second);

    /* Release serializer reset (DLL) */
    STOS_InterruptLock();
    UValue = HDMI_Read32(CFG_BASE_ADDRESS+ SYS_CFG3);
    HDMI_Write32( CFG_BASE_ADDRESS+ SYS_CFG3, UValue | S_HDMI_RST_N); /* S_HDMI_RST_N bit */
    STOS_InterruptUnlock();

    /* Wait for DLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(VOS_BASE_ADDRESS+HDMI_PHY_LCK_STA) & (HDMI_PLL_LOCKED | HDMI_DLL_LOCKED);
    } while (UValue != (HDMI_PLL_LOCKED | HDMI_DLL_LOCKED)) ;

    STOS_TaskDelay(Mili_Second);

    /*HDMI_Write32(VOS_BASE_ADDRESS+ VTG1_DRST, VTG1_RST_ENABLED );*/

    /* Enable HDMI cell */
    STOS_InterruptLock();
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG);
    UValue |= Enabled;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG, UValue & HDMI_CONFIG_MASK);
    STOS_InterruptUnlock();
#endif  /*defined(ST_7100)||defined(ST_7109)*/


#ifdef ST_7710
    STOS_InterruptLock();
    HDMI_Write32(CKG_BASE_ADDRESS+RESET_CTRL_1,UNLOCK_KEY|UC_RESET_TMDS);
    STOS_TaskDelay(ST_GetClocksPerSecond()/1000);
    HDMI_Write32(CKG_BASE_ADDRESS+RESET_CTRL_1,UNLOCK_KEY);
    STOS_InterruptUnlock();
#endif/*ST_7710*/

#if defined(ST_7200)|| defined(ST_7111)|| defined (ST_7105)
    U32 UValue;
    U32 Mili_Second;
    U32 Enabled;
    Mili_Second = ST_GetClocksPerSecond()/1000;

    /* Disable HDMI cell */
     STOS_InterruptLock();
     UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG);
     Enabled = UValue & HDMI_CONFIG_DEVICE_ENABLE;
     UValue &= ~HDMI_CONFIG_DEVICE_ENABLE;
     HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG, UValue& HDMI_CONFIG_MASK);
     STOS_InterruptUnlock();

     #if defined(ST_7200)
     /* PLL and Serializer reset */
    /* HDMI serializer reset and pll power down */
     STOS_InterruptLock();
     UValue  = HDMI_Read32(ST7200_CFG_BASE_ADDRESS + SYS_CFG3);
     UValue &= (PLL_POWERED_DOWN & HDMI_RERIALIZER_RST);
     HDMI_Write32(ST7200_CFG_BASE_ADDRESS + SYS_CFG3, UValue);
     STOS_InterruptUnlock();


     STOS_TaskDelay(Mili_Second);

     /* Power up PLL*/
     STOS_InterruptLock();
     UValue = HDMI_Read32(ST7200_CFG_BASE_ADDRESS + SYS_CFG3);
     HDMI_Write32( ST7200_CFG_BASE_ADDRESS + SYS_CFG3, UValue | PLL_S_HDMI_EN); /* PLL_S_HDMI_EN bit */
     STOS_InterruptUnlock();

    /* Wait for PLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(ST7200_CFG_BASE_ADDRESS+CKGB_FS0_CLK2_CFG) & HDMI_PLL_LOCKED;
    } while (UValue != HDMI_PLL_LOCKED) ;

    #elif defined(ST_7111)
     /* PLL and Serializer reset */
    /* HDMI serializer reset and pll power down */
     STOS_InterruptLock();
     UValue  = HDMI_Read32(ST7111_CFG_BASE_ADDRESS + SYS_CFG3);
     UValue &= (PLL_POWERED_DOWN & HDMI_RERIALIZER_RST);
     HDMI_Write32(ST7111_CFG_BASE_ADDRESS + SYS_CFG3, UValue);
     STOS_InterruptUnlock();


     STOS_TaskDelay(Mili_Second);

     /* Power up PLL*/
     STOS_InterruptLock();
     UValue = HDMI_Read32(ST7111_CFG_BASE_ADDRESS + SYS_CFG3);
     HDMI_Write32( ST7111_CFG_BASE_ADDRESS + SYS_CFG3, UValue | PLL_S_HDMI_EN); /* PLL_S_HDMI_EN bit */
     STOS_InterruptUnlock();

    /* Wait for PLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(ST7111_CFG_BASE_ADDRESS+CKGB_FS0_CLK2_CFG) & HDMI_PLL_LOCKED;
    } while (UValue != HDMI_PLL_LOCKED) ;

    #elif defined(ST_7105)
    /* PLL and Serializer reset */
    /* HDMI serializer reset and pll power down */
     STOS_InterruptLock();
     UValue  = HDMI_Read32(ST7105_CFG_BASE_ADDRESS + SYS_CFG3);
     UValue &= (PLL_POWERED_DOWN & HDMI_RERIALIZER_RST);
     HDMI_Write32(ST7105_CFG_BASE_ADDRESS + SYS_CFG3, UValue);
     STOS_InterruptUnlock();


     STOS_TaskDelay(Mili_Second);

     /* Power up PLL*/
     STOS_InterruptLock();
     UValue = HDMI_Read32(ST7105_CFG_BASE_ADDRESS + SYS_CFG3);
     HDMI_Write32( ST7105_CFG_BASE_ADDRESS + SYS_CFG3, UValue | PLL_S_HDMI_EN); /* PLL_S_HDMI_EN bit */
     STOS_InterruptUnlock();

    /* Wait for PLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32(ST7105_CFG_BASE_ADDRESS+CKGB_FS0_CLK2_CFG) & HDMI_PLL_LOCKED;
    } while (UValue != HDMI_PLL_LOCKED) ;
    #endif

    STOS_TaskDelay(Mili_Second);

    /* Release serializer reset (DLL) */
    STOS_InterruptLock();
    UValue = HDMI_Read32(CFG_BASE_ADDRESS+ SYS_CFG3);
    HDMI_Write32( CFG_BASE_ADDRESS+ SYS_CFG3, UValue | S_HDMI_RST_N); /* S_HDMI_RST_N bit */
    STOS_InterruptUnlock();

    /* Wait for DLL locked status */
    do {
        STOS_TaskDelay(Mili_Second);
        UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_STATUS)& HDMI_DLL_LOCKED;
    } while (UValue != HDMI_DLL_LOCKED);
    STOS_TaskDelay(Mili_Second);

    /* Enable HDMI cell */
    STOS_InterruptLock();
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG);
    UValue |= Enabled;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p)+ HDMI_CONFIG, UValue & HDMI_CONFIG_MASK);
    STOS_InterruptUnlock();

#endif /* ST_7200||ST_7111||ST_7105*/
} /* end of HAL_ResetEnable() function  */
/*******************************************************************************
Name        : HAL_ResetDisable
Description : Reset the HDMI ip to disable the interface
Parameters  : Handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_ResetDisable(const HAL_Handle_t Handle)
{

#if defined(ST_7100)||defined(ST_7109)
    U32 UValue;
    UNUSED_PARAMETER(Handle);
    /* Make sure clocks TMDS_HDMI and PIX_HDMI are running */
    STOS_InterruptLock();
    UValue = HDMI_Read32(CLKB_BASE_ADDRESS+ CKGB_CLK_EN);
    UValue |= (CLK_TMDS_HDMI_EN | CLK_PIX_HDMI_EN);
    HDMI_Write32(CLKB_BASE_ADDRESS+ CKGB_CLK_EN, UValue);
    STOS_InterruptUnlock();

	/* PLL and Serializer reset */
    /* HDMI serializer reset and pll power down */
    STOS_InterruptLock();
    UValue  = HDMI_Read32(CFG_BASE_ADDRESS + SYS_CFG3);
    UValue &= (PLL_POWERED_DOWN & HDMI_RERIALIZER_RST);
    HDMI_Write32(CFG_BASE_ADDRESS + SYS_CFG3, UValue);
    STOS_InterruptUnlock();

    /* Wait for PLL and DLL flags falling down. */
    do {
        STOS_TaskDelay(ST_GetClocksPerSecond()/1000);
        UValue = HDMI_Read32(VOS_BASE_ADDRESS+HDMI_PHY_LCK_STA);
    } while (UValue != 0);
#else
    UNUSED_PARAMETER(Handle);
#endif
  /* to be completed for 7710.... */

} /* end of HAL_ResetDisable() function  */
/*******************************************************************************
Name        : HAL_SetInterfaceTiming
Description : Set pixel repetition, BCH clcok and Dll Config
Parameters  : Handle and interface timing parameter (pointer)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetInterfaceTiming(const HAL_Handle_t Handle, HAL_TimingInterfaceParams_t const Params)
{
    U32  UValue;

    UValue= HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG);

    /* Enable or disable the pixel repetition by 2*/
    if (Params.EnaPixrepetition)
    {
        UValue |= HDMI_CONFIG_PIXEL_REPEAT;
    }
    else
    {
        UValue &= ~HDMI_CONFIG_PIXEL_REPEAT;
    }
    /* Set BCH Clock ratio*/
    switch (Params.BCHClockRatio)
    {
        case TMDS_CLOCK_2X : /* BCH clock = 2xTMDS Clock*/
             UValue &= ~HDMI_CONFIG_BCH_CLOCK_RATIO;
             break;

        case TMDS_CLOCK_4X : /* BCH Clock = 4xTMDS Clock*/
             UValue |= HDMI_CONFIG_BCH_CLOCK_RATIO;
            break;

        default :
            break;
    }
    /*Set the config relation between DLL clock and TMDS Clock*/
    switch (Params.DllConfig)
    {
        case HD_DllCONFIG_RATIO :
              UValue = (UValue & ~DLL_CONFIG_MASK) | DLL_CONFIG_HD;
              break;
        case SD_DllCONFIG_RATIO :
              UValue = (UValue & ~DLL_CONFIG_MASK) | DLL_CONFIG_SD;
              break;
        default :
              break;
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG, UValue & HDMI_CONFIG_MASK);

} /* end of HAL_SetInterfaceTiming() function */


/*---------------------------- Interrupt Management -----------------------------*/
/*******************************************************************************
Name        : HAL_EnableInterrupt
Description : The interrupt status bit can be enabled by performing an enable bit
              operation at the corresponding location in interrupt enable register.
Parameters  : HDMI handle,
              EnableInt: binary 1 to enable the respective interrupt.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_EnableInterrupt(const HAL_Handle_t Handle, U32 EnableInt)
{
    U32 UValue;
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_ENABLE);
    UValue |= EnableInt;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_ENABLE, UValue& HDMI_INTERRUPT_ENABLE_MASK);
} /* end of HAL_EnableInterrupt */
/*******************************************************************************
Name        : HAL_ClearInterrupt
Description : The interrupt status bit can be cleared by performing a clear bit
              operation at the corresponding location in interrupt clear register.
Parameters  : HDMI handle,
              ClearInt: binary 1 to clear the respective interrupt.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_ClearInterrupt(const HAL_Handle_t Handle, U32 ClearInt)
{
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_CLEAR, ClearInt& HDMI_INTERRUPT_CLEAR_MASK);
} /* end of HAL_ClearInterrupt */
/*******************************************************************************
Name        : HAL_SetInterruptMask
Description : Set up mask on interrupts.
Parameters  : HDMI handle,
              Mask: binary 1 to mask the respective interrupt.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetInterruptMask(const HAL_Handle_t Handle, U32 Mask)
{
     HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_ENABLE, Mask & HDMI_INTERRUPT_ENABLE_MASK );

} /* end of HAL_SetInterruptMask() function */

/*******************************************************************************
Name        : HAL_GetInterruptStatus
Description : Get HDMI Interrupt status
Parameters  : HAL HDMI manager handle
Assumptions : This is called inside interrupt context
Limitations : Beware that this function is called under interrupt context.
              Should stay in the shortest time as possible !

Returns     : HDMI interrupt status.
*******************************************************************************/
 U32 HAL_GetInterruptStatus(const HAL_Handle_t Handle)
{
    U32 UValue, HWStatus;

    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_STATUS);
    HWStatus = UValue & HDMI_INTERRUPT_STATUS_MASK;

    if(HWStatus != 0)
    {
        HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INTERRUPT_CLEAR, HWStatus);
    }

    return(HWStatus);
} /* end of HAL_GetInterruptStatus() function */
/*---------------------------- Format Management -----------------------------*/
/*******************************************************************************
Name        : HAL_SetOutputSize
Description : Set output size for video data active
Parameters  : HDMI handle,
              active windows corners U32 XStart, U32 XStop, U32 YStart, U32 YStop
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetOutputSize(const HAL_Handle_t Handle,
            U32 XStart, U32 XStop, U32 YStart, U32 YStop)
{
    /* Set the Xmin output size for the video active data*/
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_ACTIVE_VIDEO_XMIN, (XStart & HDMI_ACTIVE_VIDEO_XMIN_MASK));
    /* Set the Xmax output size for the video active data*/
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_ACTIVE_VIDEO_XMAX, (XStop & HDMI_ACTIVE_VIDEO_XMAX_MASK));
    /* Set the Ymin output size for the video active data*/
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_ACTIVE_VIDEO_YMIN, (YStart & HDMI_ACTIVE_VIDEO_YMIN_MASK));
    /* Set the Ymax output size for the video active data*/
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_ACTIVE_VIDEO_YMAX, (YStop & HDMI_ACTIVE_VIDEO_YMAX_MASK));

} /* end of HAL_SetOutputSize() function */
/*******************************************************************************
Name        : HAL_SetSyncPolarity
Description : Set up the HSYNC and VSYNC poalarity in Low or High
Parameters  : HDMI handle,
              PositivePolaritfy sychronization polarity.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetSyncPolarity(const HAL_Handle_t Handle, BOOL Polarity)
{
    U32 UValue;
    UValue =  HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG);

    /* HSYNC and VSYNC active Low */
    if (Polarity)
    {
        UValue |= HDMI_CONFIG_SYNC_POLARITY;
    }
    /* HSYNC and VSYNC active High*/
    else
    {
        UValue &= ~HDMI_CONFIG_SYNC_POLARITY;
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG, UValue & HDMI_CONFIG_MASK);
} /* end of HAL_SetSyncPolarity() function */
/*******************************************************************************
Name        : HAL_SetInputFormat
Description : Set the input Format : RGB, YCbCr444 and YCbCr422
Parameters  : HDMI handle,
              HDMI_Format_t Format
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_SetInputFormat (const HAL_Handle_t Handle, HAL_OutputFormat_t const Format)
{
    U32 UValue;
    /* The format of the Input is RGB or YCbCr*/
    #if !(defined(ST_7200)|| defined(ST_7111)|| defined(ST_7105))
    UValue = HDMI_Read32(VOS_BASE_ADDRESS+DSP_CFG_CLK);
    if (Format == HDMI_RGB888)
    {
        UValue &= ~HDMI_CFG;  /* RGB: bit1 of DSPCFG_CLK=0 */
    }
    else
    {
        UValue |= HDMI_CFG;   /* YCbCr: bit1 of DSPCFG_CLK=1 */
    }

    HDMI_Write32(VOS_BASE_ADDRESS+DSP_CFG_CLK, UValue);

    #elif defined (ST_7200)
    UValue = HDMI_Read32(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
    if (Format == HDMI_RGB888)
    {
        UValue |= DIG1_SEL_INPUT_RGB;   /* Main RGB input */
    }
    else
    {
        UValue &= ~DIG1_SEL_INPUT_RGB;  /* Main YCbCr input */
    }
    HDMI_Write32(ST7200_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, UValue);

    #elif defined (ST_7111)
    UValue = HDMI_Read32(ST7111_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
    if (Format == HDMI_RGB888)
    {
        UValue |= DIG1_SEL_INPUT_RGB;   /* Main RGB input */
    }
    else
    {
        UValue &= ~DIG1_SEL_INPUT_RGB;  /* Main YCbCr input */
    }
    HDMI_Write32(ST7111_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, UValue);

    #elif defined (ST_7105)
    UValue = HDMI_Read32(ST7105_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG);
    if (Format == HDMI_RGB888)
    {
        UValue |= DIG1_SEL_INPUT_RGB;   /* Main RGB input */
    }
    else
    {
        UValue &= ~DIG1_SEL_INPUT_RGB;  /* Main YCbCr input */
    }
    HDMI_Write32(ST7105_HD_TVOUT_HDF_BASE_ADDRESS+ DIG1_CFG, UValue);

    #endif
    /* The format of the Input is YCbCr422 or YCbCr444*/
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG);
    if ((Format & HDMI_YCBCR422)!=0)
    {
        UValue |= HDMI_CONFIG_ENABLE_422;
    }
    else
    {
        UValue &= ~HDMI_CONFIG_ENABLE_422;
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG, UValue & HDMI_CONFIG_MASK );

}/* end of HAL_SetInputFormat() function */
/*******************************************************************************
Name        : HAL_SetOutputParams
Description : Set parameters to take effect from the new frame.
Parameters  : hal HDMI handle,
              parameters of new frame.
Assumptions :
Limitations :
Returns     :Nothing
*******************************************************************************/
void  HAL_SetOutputParams (const HAL_Handle_t Handle, HAL_OutputParams_t const Params)
{
    U32 UValue;

    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG);
    /* Enable/Disable HDMI compatible frame generation*/
    if (Params.DVINotHDMI)
    {
        UValue &= ~HDMI_CONFIG_HDMI_NOT_DVI;
    }
    else
    {
        UValue |= HDMI_CONFIG_HDMI_NOT_DVI;
    }
	/* ESSnotOESS bit is valid only if HDCP enable bit is activated */
        /* These protocols are only used when the HDCP DEVICE is in authentificated state*/
#ifdef STVOUT_HDCP_PROTECTION
    if (Params.EssNotOess)
    {
       UValue |= HDMI_CONFIG_ESS_NOT_OESS;
    }
    else
    {
       UValue &= ~HDMI_CONFIG_ESS_NOT_OESS;
    }
    if (!Params.HDCPEnable)
    {
        UValue &= ~HDMI_CONFIG_HDCP_ENABLE;
    }
    else
    {
        /* If HDCP is enabled, default screen is managed by (enable,disable) API */
    }

#endif
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG, UValue& HDMI_CONFIG_MASK);

} /* end of HAL_SetOutputParams() function */

#ifdef STVOUT_HDCP_PROTECTION
/*******************************************************************************
Name        : HAL_EnableDefaultOuput
Description : Enable Default output
Parameters  : hal HDMI handle, IsHDCPEnbale(BOOL)

Assumptions :
Limitations :
Returns     :Nothing
*******************************************************************************/
void  HAL_EnableDefaultOuput (const HAL_Handle_t Handle, BOOL IsHDCPEnable)
{
  U32 UValue;

  UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG);
  if ( IsHDCPEnable)
  {
    UValue |= HDMI_CONFIG_HDCP_ENABLE;
  }
  else
  {
    UValue &= ~HDMI_CONFIG_HDCP_ENABLE;
  }
  HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CONFIG, UValue& HDMI_CONFIG_MASK);


}/* end of HAL_EnableDefaultOuput()*/
#endif
/*---------------------------- Audio Management -----------------------------*/
/*******************************************************************************
Name        : HAL_SetAudioParams
Description : Set Audio parameters.
Parameters  : hal HDMI handle,
              Audio parameters.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetAudioParams(const HAL_Handle_t Handle, HAL_AudioParams_t Params)
{
    U32 UValue;

    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_AUDIO_CONFIG);
     /*8 channel audio*/
    if(Params.Numberofchannel==8)
    {
        UValue|= HDMI_AUDIO_CONFIG_8CHL;
    }
     /*2 Channel audio*/
    else
    {
       UValue&= ~HDMI_AUDIO_CONFIG_8CHL;
    }

    switch (Params.SpdifDiv)
    {
        case 1 : /* Divide input spdif clock by 1 before using */
            UValue|= HDMI_SPDIF_IN_DIV_BY_1;
            break;

        case 2 : /* Divide input spdif clock by 2 before using */
            UValue|= HDMI_SPDIF_IN_DIV_BY_2;
            break;

        case 3 : /* Divide input spdif clock by 3 before using */
            UValue|= HDMI_SPDIF_IN_DIV_BY_3;
            break;

        case 4 : /* Divide input spdif clock by 4 before using */
           UValue|= HDMI_SPDIF_IN_DIV_BY_4;
            break;

        default :
            break;
    }
    /* Clears FIFO 0 status */
    if ((Params.ClearFiFos&FIFO_0_OVERRUN)==FIFO_0_OVERRUN)
    {
        UValue|= HDMI_FIFO_0_OVERRUN_CLEAR;
    }
    else
    {
        UValue &= ~HDMI_FIFO_0_OVERRUN_CLEAR;
    }
    /* Clears FIFO 1 status */
    if ((Params.ClearFiFos&FIFO_1_OVERRUN)==FIFO_1_OVERRUN)
    {
        UValue|= HDMI_FIFO_1_OVERRUN_CLEAR;
    }
    else
    {
        UValue &= ~HDMI_FIFO_1_OVERRUN_CLEAR;
    }
    /* Clears FIFI 2 status */
    if ((Params.ClearFiFos&FIFO_2_OVERRUN)==FIFO_2_OVERRUN)
    {
        UValue|= HDMI_FIFO_2_OVERRUN_CLEAR;
    }
    else
    {
        UValue &= ~HDMI_FIFO_2_OVERRUN_CLEAR;
    }

    /* Clears FIFI 3 status */
    if ((Params.ClearFiFos&FIFO_3_OVERRUN)==FIFO_3_OVERRUN)
    {
        UValue|= HDMI_FIFO_3_OVERRUN_CLEAR;
    }
    else
    {
        UValue &= ~HDMI_FIFO_3_OVERRUN_CLEAR;
    }

    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_AUDIO_CONFIG, UValue& HDMI_AUDIO_CONFIG_MASK);

} /* end of HAL_SetAudioParams() function */
/*******************************************************************************
Name        : HAL_SetAudioRegeneration
Description : Set Audio Regeneration Value
Parameters  : hal HDMI handle, U32 N (audio regeneration value)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetAudioRegeneration (const HAL_Handle_t Handle, U32 AudN)
{

   HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_AUDN, AudN&HDMI_AUD_REG_CLOCK_MASK);
} /* HAL_SetAudioRegeneration()*/
/*******************************************************************************
Name        : HAL_SetChannelStatusBit
Description : Set the channel status bits corresponding to one frame (192 bits)
Parameters  : hal HDMI handle, audio frequency
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_SetChannelStatusBit (const HAL_Handle_t Handle, HAL_AudioSamplingFreq_t SamplingFrequency)
{

  #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
  U32 UValue;
  #endif
  #if defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
  UNUSED_PARAMETER(Handle);
  #if defined (ST_7200)
  /*For 7200 cut 1.0 it is essential to set bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS in order to access i2stospdif register*/
  UValue = HDMI_Read32(ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue |= (0x1 << 31);
  HDMI_Write32(ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #elif defined (ST_7111)
  UValue = HDMI_Read32(ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue |= (0x1 << 31);
  HDMI_Write32(ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #elif defined (ST_7105)
  UValue = HDMI_Read32(ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue |= (0x1 << 31);
  HDMI_Write32(ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #endif

  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_0, 0x03C30000);
  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_2, 0x000000C3);
  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_3, 0x0);
  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_4, 0x0);
  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_5, 0x0);


  switch (SamplingFrequency)
  {
  	case SAMPLING_FREQ_32_KHZ :
		  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_1,0xF0000);
  		  break;
	case SAMPLING_FREQ_44_1_KHZ :
		  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_1, 0x0);
  		  break;
   case SAMPLING_FREQ_48_KHZ :
		  HDMI_Write32(I2S_2_SPDIF_BASE_ADDRESS + CHANNEL_STATUS_REG_1, 0xC0000);
		  break;
   default:
		  break;
  }
  #if defined (ST_7200)
  /*Now reset back bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS register.*/
  UValue = HDMI_Read32(ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue &=(0xFFFFFFFF>>1);
  HDMI_Write32(ST7200_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #endif

  #if defined (ST_7111)
  /*Now reset back bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS register.*/
  UValue = HDMI_Read32(ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue &=(0xFFFFFFFF>>1);
  HDMI_Write32(ST7111_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #endif

  #if defined (ST_7105)
  /*Now reset back bit31 of HD_TVOUT_AUX_GLUE_BASE_ADDRESS register.*/
  UValue = HDMI_Read32(ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS);
  UValue &=(0xFFFFFFFF>>1);
  HDMI_Write32(ST7105_HD_TVOUT_AUX_GLUE_BASE_ADDRESS, UValue);
  #endif
  #endif
  #ifdef ST_7710
    /* Activate I2S to SPDIF converter */
    *((volatile U32*)0x20103200) = 0x2;
    *((volatile U32*)0x20103A00) = 0x43;

    /* Temporary here should be integrated inside the audio driver */

    /* Common status bit */
    *((volatile U32*)(0x20103800+0x108+4*0)) = 0x03C30000;
    /* 0x108+4*1 is configured below */
    *((volatile U32*)(0x20103800+0x108+4*2)) = 0x0000003C;
    *((volatile U32*)(0x20103800+0x108+4*3)) = 0x0;
    *((volatile U32*)(0x20103800+0x108+4*4)) = 0x0;
    *((volatile U32*)(0x20103800+0x108+4*5)) = 0x0;

    switch(SamplingFrequency)
    {
        case SAMPLING_FREQ_32_KHZ :
            /* Status bit for 32KHz +- 1000 ppm */
            *((volatile U32*)(0x20103800+0x108+4*1)) = 0x000F0000;
            break;

        case SAMPLING_FREQ_44_1_KHZ :
            /* Status bit for 44.1KHz +- 1000 ppm */
            *((volatile U32*)(0x20103800+0x108+4*1)) = 0x00000000;
            break;

        case SAMPLING_FREQ_48_KHZ :
        default:
            /* default is 48KHz */
            /* Status bit for 48KHz +- 1000 ppm */
            *((volatile U32*)(0x20103800+0x108+4*1)) = 0x000C0000;
            break;
    }

    *((volatile U32*)0x20103A00) = 0x43;

    /* Perform reset */
    *((volatile U32*)0x20103800) = 0x37;
    STOS_TaskDelay(ST_GetClocksPerSecond()/100);
    *((volatile U32*)0x20103800) = 0x35;
   #endif /* ST_7710 */

} /* End of HAL_SetChannelStatusBit*/
/*******************************************************************************
Name        : HAL_SetAudioSamplePacket
Description : Set the configuration of the audio sample packet.
Parameters  : hal HDMI handle, sampleflat bit
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_SetAudioSamplePacket(const HAL_Handle_t Handle, HAL_AudioSamplePacket_t SampleFlat)
{
    U32 UValue;
    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_SAMPLE_FLAT_MASK_REG);

    /* Set the sample flat bit for sub packet 0 */
    if ((SampleFlat&SAMPLE_FLAT_BIT0)== SAMPLE_FLAT_BIT0)
    {
       UValue |= HDMI_SAMPLE_FLAT_FOR_SB_0;
    }
    else
    {
       UValue &= ~HDMI_SAMPLE_FLAT_FOR_SB_0;
    }

    /* Set the sample flat bit for sub packet 1 */
    if ((SampleFlat&SAMPLE_FLAT_BIT1)== SAMPLE_FLAT_BIT1)
    {
       UValue |= HDMI_SAMPLE_FLAT_FOR_SB_1;
    }
    else
    {
       UValue &= ~HDMI_SAMPLE_FLAT_FOR_SB_1;
    }

    /* Set the sample flat bit for sub packet 2 */
    if ((SampleFlat&SAMPLE_FLAT_BIT2)== SAMPLE_FLAT_BIT2)
    {
       UValue |= HDMI_SAMPLE_FLAT_FOR_SB_2;
    }
    else
    {
       UValue &= ~HDMI_SAMPLE_FLAT_FOR_SB_2;
    }

    /* Set the sample flat bit for sub packet 3 */
    if ((SampleFlat&SAMPLE_FLAT_BIT3)== SAMPLE_FLAT_BIT3)
    {
       UValue |= HDMI_SAMPLE_FLAT_FOR_SB_3;
    }
    else
    {
       UValue &= ~HDMI_SAMPLE_FLAT_FOR_SB_3;
    }

    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_SAMPLE_FLAT_MASK_REG, UValue& HDMI_SAMPLE_FLAT_MASK_REG_MASK);

} /* HAL_SetAudioSamplePacket() */

/*******************************************************************************
Name        : HAL_SetGPConfig
Description : Set configuration for 16 general purpose output pins..
Parameters  : hal HDMI handle, general purpose config
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_SetGPConfig(const HAL_Handle_t Handle, U32 GPConfig)
{
    U32 UValue;
    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_GPOUT);
    UValue |= GPConfig;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_GPOUT, UValue& HDMI_GPOUT_MASK);
} /* HAL_SetGPConfig()*/

/*---------------------------- InfoFrame Management -----------------------------*/

/*******************************************************************************
Name        : HAL_ControlInfoFrame
Description : General Control of the packet info frame
Parameters  : hal HDMI handle,
              General control packet parameters
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_ControlInfoFrame (const HAL_Handle_t Handle, HAL_ControlParams_t CtlParams)
{
    U32 UValue;

    UNUSED_PARAMETER(Handle);
    /* Set the maximum time between the extended control periods*/
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_EXTS_MAX_DELAY);

    switch (CtlParams.MaxDelay)
    {
        case MAX_DELAY_DEFAULT_VALUE :
             UValue |= HDMI_EXTS_DEFAULT_MAX_DELAY;
            break;

        case MAX_DELAY_16_VSYNCS :
             UValue |= HDMI_EXTS_MAX_DELAY1;
            break;

        case MAX_DELAY_1_VSYNCS :
             UValue |= HDMI_EXTS_MAX_DELAY2;
            break;

        default :
            break;
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_EXTS_MAX_DELAY, UValue&HDMI_EXTS_MAX_DELAY_MASK);

    /* Specifies duration of extended control */
    UValue = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_EXTS_MIN);

    switch (CtlParams.MinExts)
    {
        case MIN_EXTS_DEFAULT_VALUE:
            UValue |= HDMI_EXTDS_DEFAULT_MIN;
            break;

        case MIN_EXTS_CTL_PERIOD:
            UValue |= HDMI_EXTDS_MIN;
            break;
        default :
            break;
    }
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_EXTS_MIN, UValue&HDMI_EXTS_MIN_MASK);

} /* end of HAL_ControlInfoFrame() function */
/*******************************************************************************
Name        : HAL_ComputeInfoFrame
Description : Header bytes have to be assembled as words (with LSB containing HB0)
              Packet bytes hve to be assembled as words (LSB containing the LSB of the word)
Parameters  : hal HDMI handle, Frame buffer (pointer)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_ComputeInfoFrame(const HAL_Handle_t Handle, U8 *FrameBuffer_p, U32* PacketWord_p, U32 BufferSize)
{
    U8 HeaderByte=0;
    U8 PacketByte=0;
    U32 UValue,Header,Packet;
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
    U32 CTSBuffer, NBuffer, CTSValue, NValue;
#endif
    int i;

    UValue =0;
    UNUSED_PARAMETER(Handle);
    while ((HeaderByte<3)&&(FrameBuffer_p!= NULL))
    {
        Header = (U8)(*FrameBuffer_p);
        Header <<=(8* HeaderByte);
        UValue |=(U32)Header;
        HeaderByte++;
        FrameBuffer_p++;
     }

     UValue &= HDMI_INFOFRAME_HEADER_WORD_MASK;
     BufferSize -=HeaderByte;

     /* Initialize Packet word i= 0,7*/
     for (i=0;i<8;i++) PacketWord_p[i]=0;

     /* Packet header is PacketWord_p[0] */
     PacketWord_p[0] = UValue;
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
     /* Check if ACR Frame */
     if(*PacketWord_p == 0x1)
     {
         FrameBuffer_p++;
         NBuffer = *((U32*)FrameBuffer_p);
         FrameBuffer_p += 4;
         CTSBuffer = *((U32*)FrameBuffer_p);
         NValue = ((NBuffer&0xFF0000)>>16)|(NBuffer&0xFF00)|((NBuffer&0xFF)<<16);
         CTSValue = ((CTSBuffer&0xFF0000)>>16)|(CTSBuffer&0xFF00)|((CTSBuffer&0xFF)<<16);

         /* Build ACR packet */
        PacketWord_p[1] = CTSValue<<8;
        PacketWord_p[2] = NValue;
        PacketWord_p[3] = CTSValue|((NValue&0xFF)<<24);
        PacketWord_p[4] = ((CTSValue&0xFF)<<24)|((NValue&0xFFFF00)>>8);
        PacketWord_p[5] = ((NValue&0xFFFF)<<16)|((CTSValue&0xFFFF00)>>8);
        PacketWord_p[6] = ((CTSValue&0xFFFF)<<16)|((NValue&0xFF0000)>>16);
        PacketWord_p[7] = ((NValue&0xFFFFFF)<<8)|((CTSValue&0xFF0000)>>16);
        return;
     }
#endif

    /* Load the infoframe packet word */
    while ((PacketByte <BufferSize)&&(FrameBuffer_p!= NULL))
    {
         Packet = (U8)(*FrameBuffer_p);

         /* Packet bytes has to assembled as a word*/
         if (PacketByte <4)
         {
            switch (PacketByte)
            {
                   case 1 : /*Assemble the Second byte in the packet word so shift one byte*/
                       Packet <<=8;
                       break;
                   case 2 : /*Assemble the third byte in the packet word so shift two bytes*/
                       Packet <<=16;
                       break;
                   case 3 : /* Assemble the forth byte in the packet word so shift three bytes*/
                       Packet <<=24;
                       break;
                   default : /* Nothing to do*/
                       break;
            }
            PacketWord_p[1] |= Packet;
         }
         else if (PacketByte < 8)
         {
             switch (PacketByte)
             {
                case 5 :  /* Assemble the 6th byte in the packet word so shift one byte*/
                        Packet <<=8;
                         break;
                case 6 :  /* Assemble the 7th byte in the packet word so shift two bytes*/
                        Packet <<=16;
                        break;
                case 7 : /* Assemble the 8th byte in the packet word so shift three bytes*/
                        Packet <<=24;
                        break;
                default :
                        break;
            }
              PacketWord_p[2] |= Packet;

         }
         else if (PacketByte < 12)
         {
              switch (PacketByte)
              {
                  case 9 : /* Assemble the 10th byte in the packet word so shift one byte*/
                       Packet <<=8;
                       break;
                  case 10 : /* Assemble the 11th byte in the packet word so shift two bytes*/
                       Packet <<=16;
                       break;
                  case 11 : /* Assemble the 12th byte in the packet word so shift three bytes*/
                       Packet <<=24;
                      break;
                  default :
                     break;
            }
             PacketWord_p[3] |= Packet;
         }
         else if (PacketByte <16)
         {
             switch (PacketByte)
             {
                 case 13 : /* Assemble the 14th byte in the packet word so one byte*/
                      Packet <<=8;
                      break;
                 case 14 : /* Assemble the 15th byte in the packet word so shift two bytes*/
                      Packet <<=16;
                      break;
                case 15 : /* Assemble the 16th byte in the packet word so shift three bytes*/
                      Packet <<=24;
                      break;
                default :
                      break;
            }
            PacketWord_p[4] |= Packet;
         }
         else if (PacketByte <20)
         {
            switch (PacketByte)
            {
                case 17 : /*Assemble the 18th byte in the packet word so shift one byte*/
                     Packet <<=8;
                     break;
                case 18 : /* Assemble the 19th byte in the packet word so shift two bytes*/
                     Packet <<=16;
                     break;
                case 19 : /* Assemble the 20th byte in the packet word so shift three bytes*/
                     Packet <<=24;
                     break;
                default :
                    break;
            }
            PacketWord_p[5] |= Packet;

        }
        else if (PacketByte <24)
        {
            switch (PacketByte)
            {
                case 21 : /* Assemble the 22th byte in the packet word so shift one byte*/
                    Packet <<=8;
                    break;
                case 22 : /* Assemble the 23th byte in the packet word*/
                    Packet <<=16;
                    break;
                case 23 :  /* Assemble the 24th byte in the packet word*/
                    Packet <<=24;
                    break;
                default :
                    break;
            }
            PacketWord_p[6] |= Packet;
        }
        else /*PacketByte>=24 && PacketByte<28*/
        {
            switch (PacketByte)
            {
                case 25 : /* Assemble the 26th byte in the packet word so shift one byte*/
                    Packet <<=8;
                    break;
                case 26 : /* Assemble the 27th byte in the packet word so shift two bytes*/
                    Packet <<=16;
                    break;
                case 27 :  /* Assemble the 28th byte in the packet word so shift three bytes*/
                    Packet <<=24;
                    break;
                default :
                    break;
            }
            PacketWord_p[7] |= Packet;
        }
        PacketByte++;
        FrameBuffer_p++;
    }

    return;
} /* end of HAL_ComputeInfoFrame() function */
/******************************************************************************
Name        : HAL_SendInfoFrame
Description : Send Info Frames within Header and packet words.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#define HDMI_TIMESTAMP_FILE_SIZE 30
extern U32 hdmi_TimeStamp[HDMI_TIMESTAMP_FILE_SIZE][3];
extern U32 hdmi_TimeStampModulo;
U32 hdmi_TimeStampSend = 0;
void HAL_SendInfoFrame (const HAL_Handle_t Handle, U32* PacketWord_p)
{
    /* Write infoframe header & packet words in infoframe registers */
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_HEADER_WORD,
                 PacketWord_p[0] & HDMI_INFOFRAME_HEADER_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD0,
                 PacketWord_p[1] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD1,
                 PacketWord_p[2] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD2,
                 PacketWord_p[3] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD3,
                 PacketWord_p[4] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD4,
                 PacketWord_p[5] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD5,
                 PacketWord_p[6] & HDMI_INFOFRAME_PACKET_WORD_MASK);
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_PACKET_WORD6,
                 PacketWord_p[7] & HDMI_INFOFRAME_PACKET_WORD_MASK);

    /* Enable the info frame transmission */
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_INFOFRAME_CONFIG, HDMI_INFOFRAME_CONFIG_ENABLE);

    if (hdmi_TimeStampSend < HDMI_TIMESTAMP_FILE_SIZE)
      hdmi_TimeStamp[hdmi_TimeStampSend++][2] = time_now();
    hdmi_TimeStampSend %= hdmi_TimeStampModulo;

 } /* end of HAL_SendInfoFrame()*/
/*******************************************************************************
Name        : HAL_AVMute
Description : Control the transmission of data (AVI info ) in the buffer.
Parameters  : hal HDMI handle,
              Boolean
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void  HAL_AVMute (const HAL_Handle_t Handle, BOOL Enable)
{
    U32 UValue;

    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_GENCTRL_PACKET_CONFIG);
    /* AVMute inferred from next bit and AVmute enable*/
    if(Enable)
    {
      UValue |=  HDMI_GENCTRL_PACKET_AVMUTE;
    }
    /* Data in the buffer transmitted and AVmute disable*/
    else
    {
      UValue &= ~HDMI_GENCTRL_PACKET_AVMUTE;
    }
    UValue |= HDMI_GENCTRL_PACKET_ENABLE;
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_GENCTRL_PACKET_CONFIG,
                 UValue & HDMI_GENCTRL_PACKET_CONFIG_MASK );
} /* end of HAL_AVMute() function */

/* -----------------------------Hot Plug Signal management------------------------------ */
/*******************************************************************************
Name        : HAL_GetHPDStatus
Description : Get Hot Plug Detection status.
Parameters  :  HDMI handle, Connection status pointer.
Assumptions :
Limitations :
Returns     : N.A.
*******************************************************************************/
void  HAL_GetHPDStatus (HAL_Handle_t const Handle, BOOL * const ReceiverConnected)
{
#if defined(ST_OSWINCE)
    HALHDMI_IP_Properties_t *   HALHDMI_IP_Data_p;
    U8                          PioBits;

    HALHDMI_IP_Data_p = (HALHDMI_IP_Properties_t*) (((HALHDMI_Properties_t*)Handle)->PrivateData_p);
    STPIO_Read(HALHDMI_IP_Data_p->PioHandle, &PioBits);
    if (HALHDMI_IP_Data_p->IsHPDInversed)
    {
        HDMI_Driver_p->IsReceiverConnected = (!(PioBits & HALHDMI_IP_Data_p->HPD_Bit)) ? TRUE : FALSE;
    }
    else
    {
        HDMI_Driver_p->IsReceiverConnected = ((PioBits & HALHDMI_IP_Data_p->HPD_Bit)) ? TRUE : FALSE;
    }
#else /* ST_OSWINCE */
     UNUSED_PARAMETER(Handle);
#endif

    *ReceiverConnected = HDMI_Driver_p->IsReceiverConnected;

} /* end of HAL_GetHPDStatus()*/

/*******************************************************************************
Name        : HAL_GetCapturedPixel
Description : Get captured data stored in CHL0
Parameters  : in: HDMI handle
              out: the content of CHL0 register
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_GetCapturedPixel (const HAL_Handle_t Handle,U8 *chl0)
{
    U32 UValue;
    /* the captured data corresponding to chl0 is stored here*/
    UValue  = HDMI_Read32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_CHL0_CAPTURE_DATA);
    *chl0   = UValue& HDMI_CHL0_CAPTURE_DATA_MASK;

} /* end of HAL_GetCaprturedPixel */

/*******************************************************************************
Name        : HAL_SetDefaultDataChannel
Description : Send default output when the encryption was enabled
              and the authentication failed
Parameters  : in: HDMI handle
              Default values of channel 0,1,2
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void HAL_SetDefaultDataChannel (const HAL_Handle_t Handle,U8 chl0, U8 chl1, U8 chl2)
{
    /* Set Default value for channel 0 */
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_DEFAULT_CHL0_DATA, chl0&HDMI_DEFAULT_CHL0_DATA_MASK);
    /* Set Default value for channel 1 */
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_DEFAULT_CHL1_DATA, chl1&HDMI_DEFAULT_CHL1_DATA_MASK);
     /* Set Default value for channel 2 */
    HDMI_Write32((U8*)(HAL_Context_p->RegisterBaseAddress_p) + HDMI_DEFAULT_CHL2_DATA, chl2&HDMI_DEFAULT_CHL2_DATA_MASK);
} /* end of HAL_SetDefaultDataChannel */

/* End of hdmi_ip.c */
/* ------------------------------- End of file ---------------------------- */
