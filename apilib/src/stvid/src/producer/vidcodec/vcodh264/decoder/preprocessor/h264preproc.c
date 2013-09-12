/*******************************************************************************

File name   : h264preproc.c

Description :

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
21 Oct 2003       Created                                           JA
*******************************************************************************/

/* Define to add debug info and traces */
#ifdef STVID_DEBUG_PREPROC
#define STTBX_REPORT
/*#define H264PREPROC_TRACE_VERBOSE*/
#endif /* STVID_DEBUG_PREPROC */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"

#if !defined ST_OSLINUX
#include "stlite.h"
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stdevice.h"
#include "stavmem.h"

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

#include "H264_VideoTransformerTypes_Preproc.h"
#include "h264preproc.h"
#if defined(STVID_PP_DUMP) || defined(STVID_BITBUFFER_DUMP)
    #include "h264preproc_dump.h"
#endif /* defined(STVID_PP_DUMP) || defined(STVID_BITBUFFER_DUMP) */

#include "delta_pp_registers.h"

#if defined(DVD_SECURED_CHIP)
#include "stmes.h"
#endif /* defined(DVD_SECURED_CHIP) */

/* Local Constants ------------------------------------------------------- */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
/* CPU PLUG Registers */
/* Interrupts */

#if !defined(CPUPLUG_BASE_ADDRESS)
#if defined(ST_DELTAMU_GREEN_HE)
#define CPUPLUG_BASE_ADDRESS   (ST5528_EMI_BASE + 0x03A40000) /* see cpuplug vhdl file: mapping_pack.vhd */
    #else /* defined(ST_DELTAMU_GREEN_HE) */
#define CPUPLUG_BASE_ADDRESS   (ST5528_EMI_BASE + 0x03A12000) /* see cpuplug vhdl file: mapping_pack.vhd */
    #endif /* not defined(ST_DELTAMU_GREEN_HE) */
#endif /* !defined(CPUPLUG_BASE_ADDRESS) */

#define CPUPLUG_IT_MSB         (CPUPLUG_BASE_ADDRESS + 0x00)
#define CPUPLUG_IT_LSB         (CPUPLUG_BASE_ADDRESS + 0x04)
#define CPUPLUG_IT_MASK_MSB    (CPUPLUG_BASE_ADDRESS + 0x08)
#define CPUPLUG_IT_MASK_LSB    (CPUPLUG_BASE_ADDRESS + 0x0C)

/* Interrupt lines (bit position) on IT_LSB */
#define PP0_IRQ       1
#define PP1_IRQ       2
#define MBE_IRQ       4
#define MBX_IRQ       8
#define FDMA_MBX_IRQ 16
#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

#ifdef GNBvd42332
    #define GNBvd42332_CFG          0x4DB9C923
    #define GNBvd42332_PICWIDTH     0x00010001
    #define GNBvd42332_CODELENGTH   0x000001D0
    static U8 GNBvd42332_Data[] = {
0x00, 0x00, 0x00, 0x01, 0x27, 0x64, 0x00, 0x15, 0x08, 0xAC, 0x1B, 0x16, 0x39, 0xB2, 0x00, 0x00,
0x00, 0x01, 0x28, 0x03, 0x48, 0x47, 0x86, 0x83, 0x50, 0x13, 0x02, 0xC1, 0x4A, 0x15, 0x00, 0x00,
0x00, 0x01, 0x65, 0xB0, 0x34, 0x80, 0x00, 0x00, 0x03, 0x01, 0x6F, 0x70, 0x00, 0x14, 0x0A, 0xFF,
0xF6, 0xF7, 0xD0, 0x01, 0xAE, 0x5E, 0x3D, 0x7C, 0xCA, 0xA9, 0xBE, 0xCC, 0xB3, 0x3B, 0x50, 0x92,
0x27, 0x47, 0x24, 0x34, 0xE5, 0x24, 0x84, 0x53, 0x7C, 0xF5, 0x2C, 0x6E, 0x7B, 0x48, 0x1F, 0xC9,
0x8D, 0x73, 0xA8, 0x3F, 0x00, 0x00, 0x01, 0x0A, 0x03};
#endif /* GNBvd42332 */

#define INTERRUPT_NAME_LENGTH               15
#define STVID_PREPROC0_IRQ_NAME             "stvid-pp0"     /* Length must not exceed INTERRUPT_NAME_LENGTH */
#define STVID_PREPROC1_IRQ_NAME             "stvid-pp1"     /* Length must not exceed INTERRUPT_NAME_LENGTH */

/*ZQ add at 2013/07/25 for TJ project */
#define FIX_MOSIC_ISSUE


static char InterruptName[H264PP_NB_PREPROCESSORS][INTERRUPT_NAME_LENGTH];

static STOS_INTERRUPT_DECLARE(H264PreprocessorInterruptHandler, param);

/* Local Types -------------------------------------------------------------- */

#if defined(GNBvd42332) && defined(DVD_SECURED_CHIP)
/* Structure for controling the initialization of the GNBvd42332 secure data */
typedef struct GNBvd42332_Secure_H264PreprocData_Control_s
{
    BOOL                  SecureData_InitDone;
    STAVMEM_BlockHandle_t SecureData_Handle;
    void *                SecureData_StartAddress;
    void *                SecureData_StopAddress;
    U32                   SecureData_PicStart;
    U32                   SecureData_PicStop;

} GNBvd42332_SecureDataControl_t;
#endif /* #if defined(GNBvd42332) && defined(DVD_SECURED_CHIP) */

/* Local Variables ---------------------------------------------------------- */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
#if !defined(ST_DELTAMU_GREEN_HE) && !defined(STVID_USE_ONE_PREPROC)
static U32 PP_IRQ_LINES[H264PP_NB_PREPROCESSORS] = {PP0_IRQ, PP1_IRQ};
#else /* !defined(ST_DELTAMU_GREEN_HE) && !defined(STVID_USE_ONE_PREPROC) */
static U32 PP_IRQ_LINES[H264PP_NB_PREPROCESSORS] = {PP0_IRQ};
#endif /* not (!defined(ST_DELTAMU_GREEN_HE) && !defined(STVID_USE_ONE_PREPROC))*/
#endif /* defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

static H264PreprocHardware_Data_t *H264PreprocHardware_List_p = NULL;

#if defined(GNBvd42332) && defined(DVD_SECURED_CHIP)
static GNBvd42332_SecureDataControl_t GNBvd42332_SecureDataControl =
         {
            FALSE, /* SecureData_InitDone */
            (STAVMEM_BlockHandle_t)NULL, /* SecureData_Handle */
            (void*)NULL, /* SecureData_StartAddress */
            (void*)NULL, /* SecureData_StopAddress */
            (U32)NULL,   /* SecureData_PicStart */
            (U32)NULL    /* SecureData_PicStop */
         };

#endif /* defined (GNBvd42332) && defined(DVD_SECURED_CHIP) */

/* static functions --------------------------------------------------------- */

/*******************************************************************************
Name        :  H264PreprocessorInterruptHandler
Description :
Parameters  :
Assumptions :
Limitations :
Comment     : STOS_INTERRUPT_DECLARE is used in order to be free from
            : OS dependent interrupt function prototype
Returns     : Depending on OS the function returns the associated value
*******************************************************************************/
static STOS_INTERRUPT_DECLARE(H264PreprocessorInterruptHandler, param)
{
    H264Preproc_Context_t  *Preproc_Context_p = (H264Preproc_Context_t *)param;
    U32                     ITS_Value;
    void                   *PreprocBaseAddress;
#ifdef PERF1_TASK3
	H264_CommandStatus_preproc_t   *CommandStatusPreproc_p;
#endif

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
    U32                     CpuplugInterruptStatus;
#endif /* ST_DELTAPHI_HE */

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
    /* Check if external interrupt is due to preprocessor 1 interruption */
    CpuplugInterruptStatus = STSYS_ReadRegDev32LE(CPUPLUG_IT_LSB);
    if ((CpuplugInterruptStatus & PP_IRQ_LINES[Preproc_Context_p->PreprocNumber]) != PP_IRQ_LINES[Preproc_Context_p->PreprocNumber])
    {
        /* This interrupt is not for the preprocessor 1 */
        STOS_INTERRUPT_EXIT(STOS_FAILURE);
    }
#endif /*  defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) */

    PreprocBaseAddress = (void *)((U32)Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber));
    /* Read the preprocessing registers */
    Preproc_Context_p->pp_isbg = PP_Read32((U32)PreprocBaseAddress + PP_ISBG);
    Preproc_Context_p->pp_ipbg = PP_Read32((U32)PreprocBaseAddress + PP_IPBG);
    Preproc_Context_p->pp_wdl  = PP_Read32((U32)PreprocBaseAddress + PP_WDL);
    Preproc_Context_p->pp_its  = PP_Read32((U32)PreprocBaseAddress + PP_ITS);

    /* Clear the preprocessor interrupt: clear all pending status bits by writing '1' in all bits of ITS */
    /* The ITS register has been stored before for later processing */
    /* This process does not check for particular IT status bit at this stage: it is done in the upper layers */
    ITS_Value = MASK____PP_ITS__dma_cmp |
                MASK____PP_ITS__srs_cmp |
                MASK____PP_ITS__error_sc_detected |
                MASK____PP_ITS__error_bit_inserted |
                MASK____PP_ITS__int_buffer_overflow |
                MASK____PP_ITS__bit_buffer_underflow |
                MASK____PP_ITS__bit_buffer_overflow |
                MASK____PP_ITS__read_error |
                MASK____PP_ITS__write_error;
    PP_Write32((U32)PreprocBaseAddress + PP_ITS, ITS_Value);

    /* the processing of the interrupt is now done within the interrupt handler, and the preprocessor task is removed (PERf1.TASK3 perf enhancement) */

#if !defined (PERF1_TASK3)
	Preproc_Context_p->IRQ_Occured = TRUE;
    /* Signals the PreProcObject that submits the preprocessing */
    STOS_SemaphoreSignal(Preproc_Context_p->H264PreprocHardware_Data_p->PreprocJobCompletion);

#else /*PERF1_TASK3*/

	/* Code copied from preproc_task function The task doesn't exist anymore */
#if defined TRACE_UART
    TraceBuffer(("PPTask%d ", Preproc_Context_p->H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
#ifdef GNBvd42332
    /* Test if the interrupt is due to GNBvd42332 "dummy" picture processing */
    if((Preproc_Context_p->GNBvd42332_PictureInProcess) &&
       !(((Preproc_Context_p->pp_its) & (MASK____PP_ITS__srs_cmp) ) ==  MASK____PP_ITS__srs_cmp))
    {
        Preproc_Context_p->GNBvd42332_PictureInProcess = FALSE;
        STOS_SemaphoreSignal(Preproc_Context_p->GNBvd42332_Wait_semaphore);
    }
    else
    {
#endif /* GNBvd42332 */
        if ( ( (Preproc_Context_p->pp_its) & (MASK____PP_ITS__srs_cmp) ) ==  MASK____PP_ITS__srs_cmp)
        {
#ifdef GNBvd42332
            /* Test if a GNBvd42332 "dummy" picture processing was on going */
            if(Preproc_Context_p->GNBvd42332_PictureInProcess)
            {
                Preproc_Context_p->GNBvd42332_PictureInProcess = FALSE;
                STOS_SemaphoreSignal(Preproc_Context_p->GNBvd42332_Wait_semaphore);
            }
#endif /* GNBvd42332 */
            /* The IT is due to a soft reset, release the SRS semaphore for the waiting task */
            STOS_SemaphoreSignal(Preproc_Context_p->SRS_Wait_semaphore_p);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Preprocessor SRS interrupt received bug not managed by the transformer\n"));
        }
        else
        {
#ifdef STVID_PP_DUMP
            PPDMP_DumpCommand(Preproc_Context_p);
#endif /* STVID_PP_DUMP */
#ifdef STVID_BITBUFFER_DUMP
            BBDMP_DumpBitBuffer(Preproc_Context_p);
#endif /* STVID_BITBUFFER_DUMP */

            /* set the error bits */
            Preproc_Context_p->H264PreprocCommand_p->Error = H264_PREPROCESSOR_BASE + ((Preproc_Context_p->pp_its) & ~3);
#ifdef GNBvd42332
            if (Preproc_Context_p->H264PreprocCommand_p->Error != H264_PREPROCESSOR_BASE)
            {
                /* We got a preprocessing error, mark the need for call of workaround on next launch on this preproc HW  */
                Preproc_Context_p->GNBvd42332_CurrentPreprocHasError = TRUE;
            }
#endif /* GNBvd42332 */

            CommandStatusPreproc_p = &(Preproc_Context_p->H264PreprocCommand_p->CommandStatus_preproc);
            CommandStatusPreproc_p->IntSESBBegin_p       =  (U32 *) Preproc_Context_p->pp_isbg;
            CommandStatusPreproc_p->IntSESBEnd_p         =  (U32 *) Preproc_Context_p->pp_ipbg;
            CommandStatusPreproc_p->IntPictBufferBegin_p =  (U32 *) Preproc_Context_p->pp_ipbg;
            CommandStatusPreproc_p->IntPictBufferEnd_p   =  (U32 *) Preproc_Context_p->pp_wdl;

            if(!Preproc_Context_p->IsAbortPending)
            {
                Preproc_Context_p->Callback(Preproc_Context_p->H264PreprocCommand_p, Preproc_Context_p->CallbackUserData);
            }

            if(!Preproc_Context_p->IsAbortPending)
            {
                Preproc_Context_p->IsPreprocRunning = FALSE;
            }
        }
#ifdef GNBvd42332
    }
#endif /* GNBvd42332 */

#endif /* PERF1_TASK3 */
    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
} /* H264PreprocessorInterruptHandler */

/*******************************************************************************
Name        :  preproc_interrupt_init
Description :  interrupt handler installation for PP1.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void preproc_interrupt_init(H264PreprocHardware_Data_t *H264PreprocHardware_Data_p, U32 PreprocNum)
{
    STOS_InterruptLock();
    /* register and enable the interrupt handler with the global handlers */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
    if (interrupt_install_shared(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum], 0, H264PreprocessorInterruptHandler, &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNum])))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to attach handler for preproc%d interrupt\n", PreprocNum));
        STOS_InterruptUnlock();
        return;
    }

#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    if(STOS_InterruptInstall(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum],
                             0,
                             H264PreprocessorInterruptHandler,
                             InterruptName[PreprocNum],
                             &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNum])) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to attach handler for preproc%d interrupt\n", PreprocNum));
        STOS_InterruptUnlock();
        return;
    }

#endif /* 7100 | 7109 | 7200 */
    STOS_InterruptUnlock();

    if (STOS_InterruptEnable(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum], 0) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to enable preproc%d interrupt\n", PreprocNum));
        return;
    }

    return;
}

/*******************************************************************************
Name        :  preproc_interrupt_term
Description :  interrupt handler de-installation for PP1.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void preproc_interrupt_term(H264PreprocHardware_Data_t *H264PreprocHardware_Data_p, U32 PreprocNum)
{
    if (STOS_InterruptDisable(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum], 0))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to disable preproc%d interrupt\n", PreprocNum));
        return;
    }

    STOS_InterruptLock();
    /* unregister and disable the interrupt handler with the global handlers */
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE) /* emulation platform */
    if (interrupt_uninstall_shared(interrupt_handle(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum]), H264PreprocessorInterruptHandler, &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNum])))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to detach handler for preproc%d interrupt\n", PreprocNum));
        STOS_InterruptUnlock();
        return;
    }
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)

    if (STOS_InterruptUninstall(H264PreprocHardware_Data_p->InterruptNumber[PreprocNum], 0, &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNum])) != STOS_SUCCESS)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Failed to detach handler for preproc%d interrupt\n", PreprocNum));
        STOS_InterruptUnlock();
        return;
    }

#endif /* 7100 | 7109 | 7200 */
    STOS_InterruptUnlock();

    return;
}

/*******************************************************************************
Name        :  preprocessor_SoftReset
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void preprocessor_SoftReset(H264Preproc_Context_t *Preproc_Context_p)
{
    void                            *PreprocBaseAddress;
	STOS_Clock_t  MaxWaitResetTime;

    PreprocBaseAddress = (void *)((U32)Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber));

    /*  PREPROCESSOR SOFT RESET */
    PP_Write32((U32)PreprocBaseAddress + PP_SRS, MASK____PP_SRS__soft_reset);

    /* Wait for soft reset completion */
	MaxWaitResetTime = time_plus(time_now(), 20 * STVID_MAX_VSYNC_DURATION);
    STOS_SemaphoreWaitTimeOut(Preproc_Context_p->SRS_Wait_semaphore_p, &MaxWaitResetTime);

    return;
}

#if !defined PERF1_TASK3
/*******************************************************************************
Name        :  PreprocessorTask
Description :  Receiver task for end of preprocessing
Parameters  :
Assumptions :
Limitations :
Returns     :
void PreprocessorTask(void)
*******************************************************************************/
static void PreprocessorTask(H264PreprocHardware_Data_t * H264PreprocHardware_Data_p)
{

    U32                             PreprocNumber;
    H264Preproc_Context_t          *Preproc_Context_p;
    H264_CommandStatus_preproc_t   *CommandStatusPreproc_p;
#ifdef FIX_MOSIC_ISSUE    //ZQ add at 2013/07/25 for TJ project 
    //Start_Patch
    void                           *SliceStatus_NCp;
    U32                             SliceStatus;
    U32                             SESB_entry;
    BOOL                            lastSlice=FALSE;
    //End_Patch
#endif //<!-- ZQiang 2013/7/25 10:14:36 --!>

    STOS_TaskEnter(NULL);

    while (!(H264PreprocHardware_Data_p->PreprocessorTask.ToBeDeleted))
    {
        STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocJobCompletion);

        PreprocNumber = 0;
        Preproc_Context_p = NULL;
        while((PreprocNumber < H264PP_NB_PREPROCESSORS) && (Preproc_Context_p == NULL))
        {
            if(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsPreprocRunning &&
               H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IRQ_Occured)
            {
                Preproc_Context_p = &H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber];
            }
            else
            {
                PreprocNumber++;
            }
        }

        if(Preproc_Context_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"PreprocJobCompletion signaled but no corresponding context found\n"));
        }
        else
        {
            
#ifdef FIX_MOSIC_ISSUE    //ZQ add at 2013/07/25 for TJ project 
            //Start_Patch

                       if(((Preproc_Context_p->pp_its) & (MASK____PP_ITS__error_sc_detected) ) ==  MASK____PP_ITS__error_sc_detected)
        	        	        	            {

                                                         lastSlice=FALSE;
                                                         SESB_entry=Preproc_Context_p->pp_isbg;
                                                         do {
        	                                                           SliceStatus_NCp = STOS_MapPhysToUncached((void *)(SESB_entry), 8);
            	                                                       SliceStatus = PP_Read32(SliceStatus_NCp);
            	                                                       if(((SliceStatus >> 6)&0x1) == 1)
                                                                          lastSlice=TRUE;
            	                                                       else
            		                                                     {
            		                                                        STOS_UnmapPhysToUncached(SliceStatus_NCp, 8);
            		                                                        SESB_entry+=8;
            		                                                      }
                                                              }while(!lastSlice);

                                                  if((((SliceStatus >> 24)&0xFF) | ((SliceStatus >> 8)&0xFF00)) == Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam.nb_MB_in_picture_minus1)
        	        	        		                 {
        	        	        		                   	                  Preproc_Context_p->pp_its &= ~MASK____PP_ITS__error_sc_detected;
        	        	        		                   	                  Preproc_Context_p->pp_its &= ~MASK____PP_ITS__bit_buffer_overflow;
        	        	        		                   	                  SliceStatus &= ~0x80;
        	        	        		                   	                  PP_Write32((SliceStatus_NCp),SliceStatus);
        	        	        	                     }
        	        	        		           STOS_UnmapPhysToUncached(SliceStatus_NCp, 8);

        	        	        	             }

            
          //End_Patch
#endif //<!-- ZQiang 2013/7/25 10:13:44 --!>
            
            
            Preproc_Context_p->IRQ_Occured = FALSE;
#if defined TRACE_UART
            TraceBuffer(("PPTask%d ", Preproc_Context_p->H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
#ifdef GNBvd42332
            /* Test if the interrupt is due to GNBvd42332 "dummy" picture processing */
            if((Preproc_Context_p->GNBvd42332_PictureInProcess) &&
               !(((Preproc_Context_p->pp_its) & (MASK____PP_ITS__srs_cmp) ) ==  MASK____PP_ITS__srs_cmp))
            {
                Preproc_Context_p->GNBvd42332_PictureInProcess = FALSE;
                STOS_SemaphoreSignal(Preproc_Context_p->GNBvd42332_Wait_semaphore);
            }
            else
            {
#endif /* GNBvd42332 */
                if ( ( (Preproc_Context_p->pp_its) & (MASK____PP_ITS__srs_cmp) ) ==  MASK____PP_ITS__srs_cmp)
                {
#ifdef GNBvd42332
                    /* Test if a GNBvd42332 "dummy" picture processing was on going */
                    if(Preproc_Context_p->GNBvd42332_PictureInProcess)
                    {
                        Preproc_Context_p->GNBvd42332_PictureInProcess = FALSE;
                        STOS_SemaphoreSignal(Preproc_Context_p->GNBvd42332_Wait_semaphore);
                    }
#endif /* GNBvd42332 */
                    /* The IT is due to a soft reset, release the SRS semaphore for the waiting task */
                    STOS_SemaphoreSignal(Preproc_Context_p->SRS_Wait_semaphore_p);

                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Preprocessor SRS interrupt received bug not managed by the transformer\n"));
                }
                else
                {
#ifdef STVID_PP_DUMP
                    PPDMP_DumpCommand(Preproc_Context_p);
#endif /* STVID_PP_DUMP */
#ifdef STVID_BITBUFFER_DUMP
                    BBDMP_DumpBitBuffer(Preproc_Context_p);
#endif /* STVID_BITBUFFER_DUMP */

                    /* set the error bits */
                    Preproc_Context_p->H264PreprocCommand_p->Error = H264_PREPROCESSOR_BASE + ((Preproc_Context_p->pp_its) & ~3);
#ifdef GNBvd42332
                    if (Preproc_Context_p->H264PreprocCommand_p->Error != H264_PREPROCESSOR_BASE)
                    {
                        /* We got a preprocessing error, mark the need for call of workaround on next launch on this preproc HW  */
                        Preproc_Context_p->GNBvd42332_CurrentPreprocHasError = TRUE;
                    }
#endif /* GNBvd42332 */

                    CommandStatusPreproc_p = &(Preproc_Context_p->H264PreprocCommand_p->CommandStatus_preproc);
                    CommandStatusPreproc_p->IntSESBBegin_p       =  (U32 *) Preproc_Context_p->pp_isbg;
                    CommandStatusPreproc_p->IntSESBEnd_p         =  (U32 *) Preproc_Context_p->pp_ipbg;
                    CommandStatusPreproc_p->IntPictBufferBegin_p =  (U32 *) Preproc_Context_p->pp_ipbg;
                    CommandStatusPreproc_p->IntPictBufferEnd_p   =  (U32 *) Preproc_Context_p->pp_wdl;

                    if(!Preproc_Context_p->IsAbortPending)
                    {
                        Preproc_Context_p->Callback(Preproc_Context_p->H264PreprocCommand_p, Preproc_Context_p->CallbackUserData);
                    }

                    if(!Preproc_Context_p->IsAbortPending)
                    {
                        /* Lock access to preproc status */
                        STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
                        Preproc_Context_p->IsPreprocRunning = FALSE;
                        /* Unlock access to preproc status */
                        STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
                    }
                }
#ifdef GNBvd42332
            }
#endif /* GNBvd42332 */
        }
    }
    STOS_TaskExit(NULL);

} /* End of PreprocessorTask */
#endif /* PERF1_TASK3 */

/*******************************************************************************
Name        :  preprocessor_Configuration
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void preprocessor_Configuration(H264Preproc_Context_t *Preproc_Context_p)
{
    H264_TransformParam_preproc_t   *TransformParam;
    void                            *PreprocBaseAddress;
    U32                             Reg_Value;

    TransformParam = &Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam;

    PreprocBaseAddress = (void *)((U32)Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber));

    /* PREPROCESSOR CONFIGURATION */
    PP_Write32((U32)PreprocBaseAddress + PP_BBG,         (U32)TransformParam->BitBufferBeginAddress_p);
    PP_Write32((U32)PreprocBaseAddress + PP_BBS,         (U32)TransformParam->BitBufferEndAddress_p);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_START,  (U32)TransformParam->PictureStartAddrBitBuffer_p);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_STOP,   (U32)TransformParam->PictureStopAddrBitBuffer_p);

    PP_Write32((U32)PreprocBaseAddress + PP_ISBG,        (U32)TransformParam->IntSliceErrStatusBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IPBG,        (U32)TransformParam->IntPictureBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IBS,         (U32)TransformParam->IntBufferStop_p);

    Reg_Value  = TransformParam->mb_adaptive_frame_field_flag           << SHIFT___PP_CFG__mb_adaptive_frame_field_flag;
    Reg_Value |= TransformParam->entropy_coding_mode_flag               << SHIFT___PP_CFG__entropy_coding_mode_flag;
    Reg_Value |= TransformParam->frame_mbs_only_flag                    << SHIFT___PP_CFG__frame_mbs_only_flag;
    Reg_Value |= TransformParam->pic_order_cnt_type                     << SHIFT___PP_CFG__pic_order_cnt_type;
    Reg_Value |= TransformParam->pic_order_present_flag                 << SHIFT___PP_CFG__pic_order_present_flag;
    Reg_Value |= TransformParam->delta_pic_order_always_zero_flag       << SHIFT___PP_CFG__delta_pic_order_always_zero_flag;
    Reg_Value |= TransformParam->weighted_pred_flag                     << SHIFT___PP_CFG__weighted_pred_flag;
    Reg_Value |= TransformParam->weighted_bipred_idc_flag               << SHIFT___PP_CFG__weighted_bipred_idc_flag;
    Reg_Value |= TransformParam->deblocking_filter_control_present_flag << SHIFT___PP_CFG__deblocking_filter_control_present_flag;
    Reg_Value |= TransformParam->num_ref_idx_l0_active_minus1           << SHIFT___PP_CFG__num_ref_idx_l0_active_minus1;
    Reg_Value |= TransformParam->num_ref_idx_l1_active_minus1           << SHIFT___PP_CFG__num_ref_idx_l1_active_minus1;
    Reg_Value |= TransformParam->pic_init_qp                            << SHIFT___PP_CFG__pic_init_qp;
    Reg_Value |= TransformParam->direct_8x8_inference_flag              << SHIFT___PP_CFG__direct_8x8_inference_flag;
    Reg_Value |= TransformParam->transform_8x8_mode_flag                << SHIFT___PP_CFG__transform_8x8_mode_flag;
    Reg_Value |= TransformParam->monochrome                             << SHIFT___PP_CFG__monochrome;
    PP_Write32((U32)PreprocBaseAddress + PP_CFG,         Reg_Value);

    Reg_Value  = TransformParam->PictureWidthInMbs                      << SHIFT___PP_PICWIDTH__picture_width;
    Reg_Value |= TransformParam->nb_MB_in_picture_minus1                << SHIFT___PP_PICWIDTH__nb_MB_in_picture_minus1;
    PP_Write32((U32)PreprocBaseAddress + PP_PICWIDTH,    Reg_Value);

    Reg_Value  = TransformParam->log2_max_frame_num                     << SHIFT___PP_CODELENGTH__log2_max_frame_num;
    Reg_Value |= TransformParam->log2_max_pic_order_cnt_lsb             << SHIFT___PP_CODELENGTH__log2_max_pic_order_cnt_lsb;
    PP_Write32((U32)PreprocBaseAddress + PP_CODELENGTH,  Reg_Value);

    return;
}

#ifdef GNBvd42332
/*******************************************************************************
Name        :  GNBvd42332_preprocessor_Configuration
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GNBvd42332_preprocessor_Configuration(H264Preproc_Context_t *Preproc_Context_p)
{
    H264_TransformParam_preproc_t   *TransformParam;
    void                            *PreprocBaseAddress;

    TransformParam = &Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam;

    PreprocBaseAddress = (void*)((U32)Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber));

    /* PREPROCESSOR CONFIGURATION */
    PP_Write32((U32)PreprocBaseAddress + PP_BBG,         (U32)Preproc_Context_p->H264PreprocHardware_Data_p->GNBvd42332_PicStart);
    PP_Write32((U32)PreprocBaseAddress + PP_BBS,         (U32)Preproc_Context_p->H264PreprocHardware_Data_p->GNBvd42332_PicStop);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_START,  (U32)Preproc_Context_p->H264PreprocHardware_Data_p->GNBvd42332_PicStart);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_STOP,   (U32)Preproc_Context_p->H264PreprocHardware_Data_p->GNBvd42332_PicStop);

    PP_Write32((U32)PreprocBaseAddress + PP_ISBG,        (U32)TransformParam->IntSliceErrStatusBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IPBG,        (U32)TransformParam->IntPictureBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IBS,         (U32)TransformParam->IntBufferStop_p);

    PP_Write32((U32)PreprocBaseAddress + PP_CFG,         GNBvd42332_CFG);
    PP_Write32((U32)PreprocBaseAddress + PP_PICWIDTH,    GNBvd42332_PICWIDTH);
    PP_Write32((U32)PreprocBaseAddress + PP_CODELENGTH,  GNBvd42332_CODELENGTH);

    return;
}
#endif /* GNBvd42332 */

/*******************************************************************************
Name        :  preprocessor_Start
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void preprocessor_Start(H264Preproc_Context_t *Preproc_Context_p)
{
    U32     Reg_Value;
    void    *PreprocBaseAddress;

    PreprocBaseAddress = (void *)((U32)Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber));

    /* Clear the whole status register */
    Reg_Value = MASK____PP_ITS__dma_cmp |
                MASK____PP_ITS__srs_cmp |
                MASK____PP_ITS__error_sc_detected |
                MASK____PP_ITS__error_bit_inserted |
                MASK____PP_ITS__int_buffer_overflow |
                MASK____PP_ITS__bit_buffer_underflow |
                MASK____PP_ITS__bit_buffer_overflow |
                MASK____PP_ITS__read_error |
                MASK____PP_ITS__write_error;
    PP_Write32((U32)PreprocBaseAddress + PP_ITS, Reg_Value);

    /* start the preprocessing */
    PP_Write32((U32)PreprocBaseAddress + PP_START, MASK____PP_START__pp_start);
}

/*******************************************************************************
Name        :  LaunchPreprocessor
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void LaunchPreprocessor(H264Preproc_Context_t *Preproc_Context_p)
{
#ifdef H264PREPROC_TRACE_VERBOSE
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Launch preprocessing on PP#%d \n", Preproc_Context_p->PreprocNumber));
#endif /*H264PREPROC_TRACE_VERBOSE */

    preprocessor_Configuration(Preproc_Context_p);
    preprocessor_Start(Preproc_Context_p);

    return;
}

#ifdef GNBvd42332
/*******************************************************************************
Name        :  GNBvd42332_LaunchPreprocessor
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void GNBvd42332_LaunchPreprocessor(H264Preproc_Context_t *Preproc_Context_p)
{
#ifdef H264PREPROC_TRACE_VERBOSE
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Launch preprocessing for GNBvd42332 on PP#%d \n", Preproc_Context_p->PreprocNumber));
#endif /* H264PREPROC_TRACE_VERBOSE */

    GNBvd42332_preprocessor_Configuration(Preproc_Context_p);

    Preproc_Context_p->GNBvd42332_PictureInProcess = TRUE;
    preprocessor_Start(Preproc_Context_p);
    STOS_SemaphoreWait(Preproc_Context_p->GNBvd42332_Wait_semaphore);

    return;
}
#endif /* GNBvd42332 */

/*******************************************************************************
Name        :  H264PreprocInitialize
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static ST_ErrorCode_t H264PreprocInitialize(H264PreprocHardware_Data_t *H264PreprocHardware_Data_p)
{
    U32     PreprocNumber;
    void    *PreprocBaseAddress;
    /* Initialize the Preprocessor hw ressources */
    U32     max_opc_size = 5;
    U32     max_chunk_size= 0 ;
    U32     max_message_size = 3;
    U32     RegValue;

#ifdef H264PREPROC_TRACE_VERBOSE
    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"H264PreprocInitialize start.\n"));
#endif /* H264PREPROC_TRACE_VERBOSE */

    /* PREPROCESSOR(s)) INITIALIZATION */
    for(PreprocNumber = 0 ; PreprocNumber < H264PP_NB_PREPROCESSORS ; PreprocNumber++)
    {
        PreprocBaseAddress = (void *)((U32)H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * PreprocNumber));

        /* Clear preproc interrupt status to avoid dummy interrupt */
        RegValue = MASK____PP_ITS__dma_cmp |
                   MASK____PP_ITS__srs_cmp |
                   MASK____PP_ITS__error_sc_detected |
                   MASK____PP_ITS__error_bit_inserted |
                   MASK____PP_ITS__int_buffer_overflow |
                   MASK____PP_ITS__bit_buffer_underflow |
                   MASK____PP_ITS__bit_buffer_overflow |
                   MASK____PP_ITS__read_error |
                   MASK____PP_ITS__write_error;

        PP_Write32((U32)PreprocBaseAddress + PP_ITS, RegValue);

        /* install interrupt handlers and enable interrupts */
        preproc_interrupt_init(H264PreprocHardware_Data_p, PreprocNumber);

        /*  Set bits in PP_ITM register to 1's for DMA Completed and soft reset only  */
        /* All other bits are masked, as the driver must not be interrupted before the dma_cmp is raised */
        /* Set_PP_Register(RegAddress, Mask, Shift, Value) */
        RegValue = MASK____PP_ITM__dma_cmp |
                   MASK____PP_ITM__srs_cmp;
        PP_Write32((U32)PreprocBaseAddress + PP_ITM, RegValue);

        PP_Write32((U32)PreprocBaseAddress + PP_MAX_OPC_SIZE, max_opc_size);
        PP_Write32((U32)PreprocBaseAddress + PP_MAX_CHUNK_SIZE, max_chunk_size);
        PP_Write32((U32)PreprocBaseAddress + PP_MAX_MESSAGE_SIZE, max_message_size);
    }

#ifdef STVID_PP_DUMP
    PPDMP_Init();
#endif /* STVID_PP_DUMP */
#ifdef STVID_BITBUFFER_DUMP
    BBDMP_Init();
#endif /*  STVID_BITBUFFER_DUMP */

    return ST_NO_ERROR;
}

/* Public functions --------------------------------------------------------- */

/*******************************************************************************
Name        :  H264PreprocHardware_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264PreprocHardware_Init(H264Preproc_InitParams_t *H264Preproc_InitParams_p, void **handle)
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_Init(H264Preproc_InitParams_t *H264Preproc_InitParams_p, void **Handle)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p;
    H264PreprocHardware_Data_t *H264PreprocHardware_Ptr = H264PreprocHardware_List_p;
    BOOL                        FoundInitializedHardware;
    U32                         PreprocNumber;
    U32                         PreprocHardwareNumber;
    char                        TaskName[30] = "STVID.H264PP[?]PreprocTask";
#ifdef GNBvd42332
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
#endif /* GNBvd42332 */

    FoundInitializedHardware = FALSE;
    PreprocHardwareNumber = 0;
    while((H264PreprocHardware_Ptr != NULL) && (!FoundInitializedHardware))
    {
        if(H264PreprocHardware_Ptr->RegistersBaseAddress_p == H264Preproc_InitParams_p->RegistersBaseAddress_p)
        {
            FoundInitializedHardware = TRUE;
        }
        else
        {
            PreprocHardwareNumber++;
            H264PreprocHardware_Ptr = H264PreprocHardware_Ptr->NextHardware_p;
        }
    }

    if(FoundInitializedHardware)
    {
        /* No need to initialize the hardware again, just return handle to the
           already initialized one */
        *Handle = (void *)H264PreprocHardware_Ptr;
        H264PreprocHardware_Ptr->NbLinkedTransformer++;
        return(ST_NO_ERROR);
    }
    else
    {
        H264PreprocHardware_Data_p = STOS_MemoryAllocate(H264Preproc_InitParams_p->CPUPartition_p, sizeof(H264PreprocHardware_Data_t));
        if(H264PreprocHardware_Data_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"insufficient memory for allocation [H264PreprocHardware_Data_p]\n"));
            return(ST_ERROR_NO_MEMORY);
        }

        memset(H264PreprocHardware_Data_p, 0, sizeof(H264PreprocHardware_Data_t));

        *Handle = (void *)H264PreprocHardware_Data_p;

        H264PreprocHardware_Data_p->CPUPartition_p = H264Preproc_InitParams_p->CPUPartition_p;
        H264PreprocHardware_Data_p->AvmemPartitionHandle = H264Preproc_InitParams_p->AvmemPartitionHandle;
        H264PreprocHardware_Data_p->RegistersBaseAddress_p = H264Preproc_InitParams_p->RegistersBaseAddress_p;
        H264PreprocHardware_Data_p->NbLinkedTransformer = 1;
#if !defined (PERF1_TASK3)
        H264PreprocHardware_Data_p->PreprocJobCompletion = NULL;
        H264PreprocHardware_Data_p->PreprocStatusSemaphore = NULL;
#endif /* PERF1_TASK3*/
        for(PreprocNumber = 0 ; PreprocNumber < H264PP_NB_PREPROCESSORS ; PreprocNumber++)
        {
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p = NULL;
#ifdef GNBvd42332
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore = NULL;
#endif /* GNBvd42332 */
        }

#ifdef GNBvd42332
        /* Allocate AVMEM memory and copy Dummy bitstream (mandatory for Emulation platform) */
        AllocBlockParams.PartitionHandle          = H264PreprocHardware_Data_p->AvmemPartitionHandle;
        AllocBlockParams.Alignment                = 4;
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        AllocBlockParams.Size = sizeof(GNBvd42332_Data);
        AllocBlockParams.Size = ((AllocBlockParams.Size + 3) / 4) * 4;

        if(STAVMEM_AllocBlock(&AllocBlockParams, &(H264PreprocHardware_Data_p->GNBvd42332_Data_Handle)) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate STAVMEM memory for GNBvd42332 WA [H264PreprocHardware_Data_p->Preproc_Context[%d]->GNBvd42332_Wait_semaphore] semaphore.\n", PreprocNumber));
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }
        if(STAVMEM_GetBlockAddress(H264PreprocHardware_Data_p->GNBvd42332_Data_Handle, (void **)&(H264PreprocHardware_Data_p->GNBvd42332_Data_Address)) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to get STAVMEM block address for GNBvd42332 WA [H264PreprocHardware_Data_p->Preproc_Context[%d]->GNBvd42332_Wait_semaphore] semaphore.\n", PreprocNumber));
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }
        H264PreprocHardware_Data_p->GNBvd42332_PicStart = (U32)STAVMEM_VirtualToDevice(H264PreprocHardware_Data_p->GNBvd42332_Data_Address, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
        H264PreprocHardware_Data_p->GNBvd42332_PicStop = (H264PreprocHardware_Data_p->GNBvd42332_PicStart + sizeof(GNBvd42332_Data));

        STOS_memcpy(H264PreprocHardware_Data_p->GNBvd42332_Data_Address, &GNBvd42332_Data, sizeof(GNBvd42332_Data));
#endif /* GNBvd42332 */

        for(PreprocNumber = 0 ; PreprocNumber < H264PP_NB_PREPROCESSORS ; PreprocNumber++)
        {
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].PreprocNumber = PreprocNumber;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].H264PreprocHardware_Data_p = H264PreprocHardware_Data_p;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IRQ_Occured = FALSE;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsPreprocRunning = FALSE;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsAbortPending = FALSE;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p = STOS_SemaphoreCreateFifoTimeOut(H264PreprocHardware_Data_p->CPUPartition_p, 0);
            if (H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to initialize [H264PreprocHardware_Data_p->Preproc_Context[%d]->SRS_Wait_semaphore_p] semaphore.\n", PreprocNumber));
                H264PreprocHardware_Term(*Handle);
                return(ST_ERROR_NO_MEMORY);
            }
#ifdef GNBvd42332
            /* create GNBvd42332_Wait_semaphore semaphore */
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore = STOS_SemaphoreCreateFifo(H264PreprocHardware_Data_p->CPUPartition_p, 0);
            if (H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to initialize [H264PreprocHardware_Data_p->Preproc_Context[%d]->GNBvd42332_Wait_semaphore] semaphore.\n", PreprocNumber));
                H264PreprocHardware_Term(*Handle);
                return(ST_ERROR_NO_MEMORY);
            }
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_PictureInProcess = FALSE;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_CurrentPreprocHasError = FALSE;
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Previous_MBAFF_flag = 0;
            /* GNBvd42332_PreviousPreprocHasError is set to TRUE
               at init in order to ensure that workaround will be applied on the first picture
               after a term/start sequence this ensure correct reset of preprocessor when context
               has been lost due to term operation */
            H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_PreviousPreprocHasError = TRUE;
#endif /* GNBvd42332 */
        }

        H264PreprocHardware_Data_p->InterruptNumber[0] = H264Preproc_InitParams_p->InterruptNumber;

#ifdef ST_OSLINUX
        strcpy(InterruptName[0],  STVID_PREPROC0_IRQ_NAME);
#endif /* ST_OSLINUX */

#if H264PP_NB_PREPROCESSORS > 1
#if (defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE))
#if defined (ST_DELTAMU_GREEN_HE)
        H264PreprocHardware_Data_p->InterruptNumber[1] = 0;
#else
        H264PreprocHardware_Data_p->InterruptNumber[1] = OS21_INTERRUPT_IRL_ENC_5;
#endif  /* ST_DELTAMU_GREEN_HE */

#elif defined(ST_7100) || defined(ST_7109)
	#ifdef ST_7100
        H264PreprocHardware_Data_p->InterruptNumber[1] = ST7100_VID_DPHI_PRE1_INTERRUPT;
	#elif defined(ST_7109)
        H264PreprocHardware_Data_p->InterruptNumber[1] = ST7109_VID_DPHI_PRE1_INTERRUPT;
	#endif
#endif /* elif defined(ST_7100) || defined(ST_7109) */

#ifdef ST_OSLINUX
        strcpy(InterruptName[1],  STVID_PREPROC1_IRQ_NAME);
#endif /* ST_OSLINUX */
#endif /* H264PP_NB_PREPROCESSORS > 1 */

        /* Update chained list of initialized hardware */
        H264PreprocHardware_Data_p->NextHardware_p = H264PreprocHardware_List_p;
        H264PreprocHardware_List_p = H264PreprocHardware_Data_p;

#if !defined PERF1_TASK3
        /* create PreprocJobCompletion semaphore */
        H264PreprocHardware_Data_p->PreprocJobCompletion = STOS_SemaphoreCreateFifo(H264PreprocHardware_Data_p->CPUPartition_p, 0);
        if (H264PreprocHardware_Data_p->PreprocJobCompletion == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to initialize [H264PreprocHardware_Data_p.PreprocJobCompletion] semaphore.\n"));
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }

        /* create PreprocStatusSemaphore semaphore */
        H264PreprocHardware_Data_p->PreprocStatusSemaphore = STOS_SemaphoreCreateFifo(H264PreprocHardware_Data_p->CPUPartition_p, 1);
        if (H264PreprocHardware_Data_p->PreprocStatusSemaphore == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to initialize [H264PreprocHardware_Data_p.PreprocStatusSemaphore] semaphore.\n"));
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }

        /* Create PreprocessorTask task for end of preprocessing */
        H264PreprocHardware_Data_p->PreprocessorTask.IsRunning = FALSE;
        H264PreprocHardware_Data_p->PreprocessorTask.ToBeDeleted = FALSE;
        TaskName[13] = (char) ((U8) '0' + PreprocHardwareNumber);

		if (STOS_TaskCreate((void (*) (void*)) PreprocessorTask,
                              (void *) H264PreprocHardware_Data_p,
                              H264PreprocHardware_Data_p->CPUPartition_p,
                              STVID_TASK_STACK_SIZE_DECODE,
                              &(H264PreprocHardware_Data_p->PreprocessorTask.TaskStack),
                              H264PreprocHardware_Data_p->CPUPartition_p,
                              &(H264PreprocHardware_Data_p->PreprocessorTask.Task_p),
                              &(H264PreprocHardware_Data_p->PreprocessorTask.TaskDesc),
                              STVID_TASK_PRIORITY_DECODE,
                              TaskName,
                              task_flags_no_min_stack_size) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to create Proprocessor task\n"));
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }

        H264PreprocHardware_Data_p->PreprocessorTask.IsRunning = TRUE;
#endif /* PERF1_TASK3 */

        if(H264PreprocInitialize(H264PreprocHardware_Data_p) != ST_NO_ERROR)
        {
            H264PreprocHardware_Term(*Handle);
            return(ST_ERROR_NO_MEMORY);
        }
    }

    return(ST_NO_ERROR);
} /* H264PreprocHardware_Init */

#if defined(DVD_SECURED_CHIP)
/*******************************************************************************
Name        :  H264PreprocHardware_SetupReservedPartitionForH264PreprocWA_GNB42332
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_SetupReservedPartitionForH264PreprocWA_GNB42332(void *Handle, const STAVMEM_PartitionHandle_t AVMEMPartition)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p = (H264PreprocHardware_Data_t *)Handle;
    STAVMEM_AllocBlockParams_t  AllocBlockParams;
    STAVMEM_FreeBlockParams_t   AvmemFreeParams;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    /* Check parameters */
    if(H264PreprocHardware_Data_p == NULL || AVMEMPartition == (STAVMEM_PartitionHandle_t) NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

#if !defined(GNBvd42332)
        return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else /* !defined(GNBvd42332) */

    /* If the GNBvd42332 secure data have not been initialzed then do it                  */
    /* Note: only one instance of the GNBvd42332 data for all H264 preprocessor instances */
    if(!GNBvd42332_SecureDataControl.SecureData_InitDone)
    {
        /* Allocate a block in the new partition */
        /* Allocate AVMEM memory and copy Dummy bitstream (mandatory for Emulation platform) */
        AllocBlockParams.PartitionHandle          = AVMEMPartition;
        AllocBlockParams.Alignment                = 32; /* 32 bytes aligned address for STMES secure range */
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_TOP_BOTTOM;
        AllocBlockParams.NumberOfForbiddenRanges  = 0;
        AllocBlockParams.ForbiddenRangeArray_p    = NULL;
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        AllocBlockParams.Size = sizeof(GNBvd42332_Data);
        /* End of STMES secure range must be 32 bytes aligned */
        AllocBlockParams.Size = ((AllocBlockParams.Size + 31) / 32) * 32;

        ErrorCode = STAVMEM_AllocBlock(&AllocBlockParams, &(GNBvd42332_SecureDataControl.SecureData_Handle));
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to allocate STAVMEM memory for GNBvd42332/Secure WA [H264PreprocHardware_Data_p=%p]\n", H264PreprocHardware_Data_p));
            return(ST_ERROR_NO_MEMORY);
        }

        ErrorCode = STAVMEM_GetBlockAddress(GNBvd42332_SecureDataControl.SecureData_Handle, (void **)&(GNBvd42332_SecureDataControl.SecureData_StartAddress));
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to get STAVMEM block address for GNBvd42332 WA [H264PreprocHardware_Data_p= %p]\n", H264PreprocHardware_Data_p));
            AvmemFreeParams.PartitionHandle = AVMEMPartition;
            STAVMEM_FreeBlock(&AvmemFreeParams, &(H264PreprocHardware_Data_p->GNBvd42332_Data_Handle));
            return(ST_ERROR_NO_MEMORY);
        }

        /* Copy the GNBvd42332 secure data into the new partition */
        STOS_memcpy(GNBvd42332_SecureDataControl.SecureData_StartAddress, &GNBvd42332_Data, sizeof(GNBvd42332_Data));
        GNBvd42332_SecureDataControl.SecureData_StopAddress = (void*)((U32)(GNBvd42332_SecureDataControl.SecureData_StartAddress) + AllocBlockParams.Size - 1);

        /* Compute and store the dummy picture start/stop adresses */
        GNBvd42332_SecureDataControl.SecureData_PicStart = (U32)STAVMEM_VirtualToDevice(GNBvd42332_SecureDataControl.SecureData_StartAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P);
        GNBvd42332_SecureDataControl.SecureData_PicStop  = GNBvd42332_SecureDataControl.SecureData_PicStart + sizeof(GNBvd42332_Data);

        /* Turn the data block to "secure" */
        /* NOTE: once turned to "secure" the block can no more be deallocated */
        ErrorCode = STMES_SetSecureRange( STAVMEM_VirtualToDevice(GNBvd42332_SecureDataControl.SecureData_StartAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                          STAVMEM_VirtualToDevice(GNBvd42332_SecureDataControl.SecureData_StopAddress, STAVMEM_VIRTUAL_MAPPING_AUTO_P),
                                          SID_SH4_CPU);
        if(ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to get turn GNBvd42332 data to 'secure'\n"));
            return(ErrorCode);
        }

        /* Init of GNBvd42332 secure data completed */
        GNBvd42332_SecureDataControl.SecureData_InitDone = TRUE;
    }

    /* If this preprocessor instance had already non-secure data then deallocate them */
    if(H264PreprocHardware_Data_p->GNBvd42332_Data_Address != NULL)
    {
        AvmemFreeParams.PartitionHandle = H264PreprocHardware_Data_p->AvmemPartitionHandle;
        STAVMEM_FreeBlock(&AvmemFreeParams, &(H264PreprocHardware_Data_p->GNBvd42332_Data_Handle));
        H264PreprocHardware_Data_p->GNBvd42332_Data_Address = (void*)NULL;
        H264PreprocHardware_Data_p->GNBvd42332_Data_Handle = (STAVMEM_BlockHandle_t)NULL;
    }

    /* Make this preprocessor instance point to the GNBvd42332 secure data */
    H264PreprocHardware_Data_p->GNBvd42332_PicStart = GNBvd42332_SecureDataControl.SecureData_PicStart;
    H264PreprocHardware_Data_p->GNBvd42332_PicStop  = GNBvd42332_SecureDataControl.SecureData_PicStop;

    /* Store the new partition handle */
    H264PreprocHardware_Data_p->AvmemPartitionHandle = AVMEMPartition;

#endif /* defined(GNBvd42332) */

    return ErrorCode;

} /* H264PreprocHardware_SetupReservedPartitionForH264PreprocWA_GNB42332 */
#endif /* defined(DVD_SECURED_CHIP) */

/*******************************************************************************
Name        :  H264PreprocHardware_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264PreprocHardware_Term (void *Handle)
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_Term(void *Handle)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p = (H264PreprocHardware_Data_t *)Handle;
    U32                         PreprocNumber;
#ifdef GNBvd42332
    STAVMEM_FreeBlockParams_t   FreeParams;
#endif /* GNBvd42332 */

    if(H264PreprocHardware_Data_p->NbLinkedTransformer == 1)
    { /* Actually terminate only if this is the last transformer using this hardware */
        /* delete task */
#if !defined PERF1_TASK3
        if (!(H264PreprocHardware_Data_p->PreprocessorTask.IsRunning))
        {
            return(ST_ERROR_UNKNOWN_DEVICE);
        }

        /* End the function of the task here... */
        H264PreprocHardware_Data_p->PreprocessorTask.ToBeDeleted = TRUE;
        /* Signal semaphore to release task that may wait infinitely if stopped */

        STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocJobCompletion);

        STOS_TaskWait(&H264PreprocHardware_Data_p->PreprocessorTask.Task_p, TIMEOUT_INFINITY);
        STOS_TaskDelete ( H264PreprocHardware_Data_p->PreprocessorTask.Task_p,
                      H264PreprocHardware_Data_p->CPUPartition_p,
                      &(H264PreprocHardware_Data_p->PreprocessorTask.TaskStack),
                      H264PreprocHardware_Data_p->CPUPartition_p);

        H264PreprocHardware_Data_p->PreprocessorTask.IsRunning = FALSE;

        /* delete PreprocJobCompletion semaphore */
        if(H264PreprocHardware_Data_p->PreprocJobCompletion != NULL)
        {
            STOS_SemaphoreDelete(H264PreprocHardware_Data_p->CPUPartition_p, H264PreprocHardware_Data_p->PreprocJobCompletion);
            H264PreprocHardware_Data_p->PreprocJobCompletion = NULL;
        }

        /* delete PreprocStatusSemaphore semaphore */
        if(H264PreprocHardware_Data_p->PreprocStatusSemaphore != NULL)
        {
            STOS_SemaphoreDelete(H264PreprocHardware_Data_p->CPUPartition_p, H264PreprocHardware_Data_p->PreprocStatusSemaphore);
            H264PreprocHardware_Data_p->PreprocStatusSemaphore = NULL;
        }
#endif /* PERF1_TASK3 */

        for(PreprocNumber = 0 ; PreprocNumber < H264PP_NB_PREPROCESSORS ; PreprocNumber++)
        {
            /* Uninstall interrupt handler */
            preproc_interrupt_term(H264PreprocHardware_Data_p, PreprocNumber);
            /* delete SRS_Wait_semaphore_p semaphore */
            if(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p != NULL)
            {
                STOS_SemaphoreDelete(H264PreprocHardware_Data_p->CPUPartition_p, H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p);
                H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].SRS_Wait_semaphore_p = NULL;
            }
#ifdef GNBvd42332
            /* delete GNBvd42332_Wait_semaphore semaphore */
            if(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore != NULL)
            {
                STOS_SemaphoreDelete(H264PreprocHardware_Data_p->CPUPartition_p, H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore);
                H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].GNBvd42332_Wait_semaphore = NULL;
            }
#endif /* GNBvd42332 */
        }
#ifdef GNBvd42332
        if(H264PreprocHardware_Data_p->GNBvd42332_Data_Address != NULL)
        {
            FreeParams.PartitionHandle = H264PreprocHardware_Data_p->AvmemPartitionHandle;
            STAVMEM_FreeBlock(&FreeParams, &(H264PreprocHardware_Data_p->GNBvd42332_Data_Handle));
            H264PreprocHardware_Data_p->GNBvd42332_Data_Address = NULL;
        }
#endif /* GNBvd42332 */
        /* free instance local memory */
        if(H264PreprocHardware_Data_p != NULL)
        {
            STOS_MemoryDeallocate(H264PreprocHardware_Data_p->CPUPartition_p, H264PreprocHardware_Data_p);
            /* If the Current Preproc terminated was the head of the preproc list (H264PreprocHardware_List_p) clean up this list first entry */
            if (H264PreprocHardware_Data_p == H264PreprocHardware_List_p)
            {
                H264PreprocHardware_List_p = NULL;
            }
            H264PreprocHardware_Data_p = NULL;
        }
    }
    else
    {
        H264PreprocHardware_Data_p->NbLinkedTransformer--;
    }

    return(ST_NO_ERROR);
} /* H264PreprocHardware_Term */

/*******************************************************************************
Name        : H264PreprocHardware_ProcessPicture
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264PreprocHardware_ProcessPicture(void *Handle, H264PreprocCommand_t *H264PreprocCommand_p)
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_ProcessPicture(void *Handle,
                                                  H264PreprocCommand_t *H264PreprocCommand_p,
                                                  H264PreprocCallback_t Callback,
                                                  void* CallbackUserData)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p = (H264PreprocHardware_Data_t *)Handle;
    U32                     PreprocNumber;
    H264Preproc_Context_t  *Preproc_Context_p;

    /* Look for an availiable preprocessor */
    /* Lock access to preproc status */
#ifdef PERF1_TASK3
	STOS_InterruptLock();
#else
    STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif
    PreprocNumber = 0;
    Preproc_Context_p = NULL;
    while((PreprocNumber < H264PP_NB_PREPROCESSORS) && (Preproc_Context_p == NULL))
    {
        if(!H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsPreprocRunning)
        {
            Preproc_Context_p = &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber]);
        }
        else
        {
            PreprocNumber++;
        }
    }

    if(Preproc_Context_p != NULL)
    {
        /* Get the found picture */
        Preproc_Context_p->IsPreprocRunning = TRUE;
    }

#ifdef PERF1_TASK3
	STOS_InterruptUnlock();
#else
    /* Unlock access to preproc status */
    STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif

    if(Preproc_Context_p == NULL)
    {
#if defined TRACE_UART
        TraceBuffer(("PP Process CmdId not found %d ", H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"PP Process picture not found %d ", H264PreprocCommand_p->CmdId));
        return(ST_ERROR_NO_MEMORY);
    }

    Preproc_Context_p->H264PreprocCommand_p = H264PreprocCommand_p;
    Preproc_Context_p->Callback = Callback;
    Preproc_Context_p->CallbackUserData = CallbackUserData;

#ifdef GNBvd42332
    if( Preproc_Context_p->GNBvd42332_PreviousPreprocHasError ||
       ((Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam.mb_adaptive_frame_field_flag == 0) &&
        (Preproc_Context_p->GNBvd42332_Previous_MBAFF_flag == 1) &&
        (Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam.entropy_coding_mode_flag == 1))
      )
    {
        GNBvd42332_LaunchPreprocessor(Preproc_Context_p);
    }
    Preproc_Context_p->GNBvd42332_Previous_MBAFF_flag = Preproc_Context_p->H264PreprocCommand_p->TransformerPreprocParam.mb_adaptive_frame_field_flag;
    Preproc_Context_p->GNBvd42332_PreviousPreprocHasError = Preproc_Context_p->GNBvd42332_CurrentPreprocHasError;
    Preproc_Context_p->GNBvd42332_CurrentPreprocHasError = FALSE;
#endif /* GNBvd42332 */

    if(!Preproc_Context_p->IsAbortPending)
    {
#if defined TRACE_UART
        TraceBuffer(("PPLaunch%d ", Preproc_Context_p->H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
        LaunchPreprocessor(Preproc_Context_p);
    }

    return(ST_NO_ERROR);
} /* H264PreprocHardware_ProcessPicture */

#if !defined PERF1_TASK3
/*******************************************************************************
Name        : H264PreprocHardware_IsHardwareBusy
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264PreprocHardware_IsHardwareBusy(H264PreprocHandle_t Handle, BOOL *Busy)
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_IsHardwareBusy(void *Handle, BOOL *Busy)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p = (H264PreprocHardware_Data_t *)Handle;
    U32                         PreprocNumber;

    /* Look for an availiable preprocessor */
    /* Lock access to preproc status */
    STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocStatusSemaphore);

    PreprocNumber = 0;
    *Busy = TRUE;
    while((PreprocNumber < H264PP_NB_PREPROCESSORS) && (*Busy == TRUE))
    {
        if(!H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsPreprocRunning)
        {
            *Busy = FALSE;
        }
        else
        {
            PreprocNumber++;
        }
    }
    /* Unlock access to preproc status */
    STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);

    return(ST_NO_ERROR);
} /* H264PreprocHardware_IsHardwareBusy */

#endif /*PERF1_TASK3*/

/*******************************************************************************
Name        : H264PreprocHardware_AbortCommand
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
ST_ErrorCode_t H264PreprocHardware_AbortCommand(void *Handle, H264PreprocCommand_t *H264PreprocCommand_p)
*******************************************************************************/
ST_ErrorCode_t H264PreprocHardware_AbortCommand(void *Handle, H264PreprocCommand_t *H264PreprocCommand_p)
{
    H264PreprocHardware_Data_t *H264PreprocHardware_Data_p = (H264PreprocHardware_Data_t *)Handle;
    U32                     PreprocNumber;
    H264Preproc_Context_t  *Preproc_Context_p;

    /* Look requested command */
    /* Lock access to preproc status */
#ifdef PERF1_TASK3
	STOS_InterruptLock();
#else
    STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif

    PreprocNumber = 0;
    Preproc_Context_p = NULL;
    while((PreprocNumber < H264PP_NB_PREPROCESSORS) && (Preproc_Context_p == NULL))
    {
        if(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].IsPreprocRunning &&
           (H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber].H264PreprocCommand_p->CmdId == H264PreprocCommand_p->CmdId))
        {
            Preproc_Context_p = &(H264PreprocHardware_Data_p->Preproc_Context[PreprocNumber]);
        }
        else
        {
            PreprocNumber++;
        }
    }

    if(Preproc_Context_p != NULL)
    {
        if (Preproc_Context_p->IsAbortPending)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"PPAbort CmdId already aborted %d ", H264PreprocCommand_p->CmdId));
#if defined TRACE_UART
        TraceBuffer(("PPAbort CmdId already aborted %d ", H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
#ifdef PERF1_TASK3
			STOS_InterruptUnlock();
#else
            /* ST-HG added */ STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif
            return(ST_ERROR_BAD_PARAMETER);
        }
        /* Get the found picture */
        Preproc_Context_p->IsAbortPending = TRUE;
    }

#ifdef PERF1_TASK3
	STOS_InterruptUnlock();
#else
    /* Unlock access to preproc status */
    STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif /* PERF1_TASK3 */

    if(Preproc_Context_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"PPAbort CmdId not found %d ", H264PreprocCommand_p->CmdId));
#if defined TRACE_UART
        TraceBuffer(("PPAbort CmdId not found %d ", H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */
        return(ST_ERROR_BAD_PARAMETER);
    }

#if defined TRACE_UART
    TraceBuffer(("PPSRSLaunch%d ", Preproc_Context_p->H264PreprocCommand_p->CmdId));
#endif /* TRACE_UART */

    preprocessor_SoftReset(Preproc_Context_p);

#ifdef PERF1_TASK3
	STOS_InterruptLock();
#else
    /* Lock access to preproc status */
    STOS_SemaphoreWait(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif /* PERF1_TASK3 */

    Preproc_Context_p->IsAbortPending = FALSE;
    Preproc_Context_p->IsPreprocRunning = FALSE;

#ifdef PERF1_TASK3
	STOS_InterruptUnlock();
#else
    /* Unlock access to preproc status */
    STOS_SemaphoreSignal(H264PreprocHardware_Data_p->PreprocStatusSemaphore);
#endif /* PERF1_TASK3 */

    return(ST_NO_ERROR);
} /* H264PreprocHardware_AbortCommand */

/* End of h264preproc.c */
