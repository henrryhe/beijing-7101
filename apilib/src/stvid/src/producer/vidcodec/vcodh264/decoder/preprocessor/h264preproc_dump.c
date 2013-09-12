/*******************************************************************************

File name   : h264preproc_dump.c

Description : ????

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16/12/2003         Creation                                         Didier SIRON

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <stdio.h>  /* va_start */
#include <stdarg.h> /* va_start */
#include <string.h> /* memset */
#endif

#include "stavmem.h" /* For virtual mapping */
#include "H264_VideoTransformerTypes_Preproc.h"
#include "h264preproc_dump.h"
#include "delta_pp_registers.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Public variables --------------------------------------------------------- */

/* Private Defines ---------------------------------------------------------- */

#define DUMP_FILE_NAME  "../../dump_pp.txt"

/* Private Types ------------------------------------------------------------ */

/* Private Variables (static) ----------------------------------------------- */

#if !defined ST_OSLINUX
static FILE* DumpFile_p = NULL;
static int Counter = 0;

static int BBDumpNumber = 0;
#endif

/* Private Macros ----------------------------------------------------------- */

/* Private Function Prototypes ---------------------------------------------- */

#if !defined ST_OSLINUX
static int myfprintf(FILE *stream, const char *format, ...);
#endif

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
#if !defined ST_OSLINUX
static int myfprintf(FILE *stream, const char *format, ...)
{
    int ret = 0;

    if (stream != NULL)
    {
        va_list argptr;
        va_start(argptr, format);     /* Initialize variable arguments. */

        ret = vfprintf(stream, format, argptr);

        va_end(argptr);
        fflush(stream);
    }

    return ret;
}
#endif

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t PPDMP_Init(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Counter = 0;

    DumpFile_p = fopen(DUMP_FILE_NAME, "w");
    if (DumpFile_p == NULL)
    {
        printf("*** PPDMP_Init_stub: fopen error on file '%s' ***\n", DUMP_FILE_NAME);
        Error = ST_ERROR_OPEN_HANDLE;
    }

    return Error;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t PPDMP_Term(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    if (DumpFile_p != NULL)
    {
        (void)fclose(DumpFile_p);
        DumpFile_p = NULL;
    }

    return ST_NO_ERROR;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t PPDMP_DumpCommand(H264Preproc_Context_t *Preproc_Context_p)
{
#ifdef ST_OSLINUX
    /* To avoid warnings ... */
    Preproc_Context_p = Preproc_Context_p;

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    void    *PreprocBaseAddress;
    U32     pp_read_start;
    U32     pp_read_stop;
    U32     pp_isbg;
    U32     pp_cfg;
    U32     pp_picwidth;
    U32     pp_codelength;
    
    PreprocBaseAddress = Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber);

    if(DumpFile_p != NULL)
    {

        Counter += 1;
        myfprintf(DumpFile_p, "PP_TRANSFORM #%d\n", Counter);
        myfprintf(DumpFile_p, "  Preproc number = %d\n", Preproc_Context_p->PreprocNumber);

        myfprintf(DumpFile_p, "\n  General Configuration \n");
        myfprintf(DumpFile_p, "    Max OPC Size             = %d\n", PP_Read32(PreprocBaseAddress + PP_MAX_OPC_SIZE));
        myfprintf(DumpFile_p, "    Max Chunk Size           = %d\n", PP_Read32(PreprocBaseAddress + PP_MAX_CHUNK_SIZE));
        myfprintf(DumpFile_p, "    Max Message Size         = %d\n", PP_Read32(PreprocBaseAddress + PP_MAX_MESSAGE_SIZE));
        myfprintf(DumpFile_p, "\n  Picture Configuration \n");
        myfprintf(DumpFile_p, "    BBG                      = 0x%08x\n", PP_Read32(PreprocBaseAddress + PP_BBG));
        myfprintf(DumpFile_p, "    BBS                      = 0x%08x\n", PP_Read32(PreprocBaseAddress + PP_BBS));
        pp_read_start = PP_Read32(PreprocBaseAddress + PP_READ_START);
        myfprintf(DumpFile_p, "    Read Start               = 0x%08x\n", pp_read_start);
        pp_read_stop = PP_Read32(PreprocBaseAddress + PP_READ_STOP);
        myfprintf(DumpFile_p, "    Read Stop                = 0x%08x\n", pp_read_stop);
        myfprintf(DumpFile_p, "    Input data size          = %d\n", pp_read_stop - pp_read_start);
        pp_isbg = PP_Read32(PreprocBaseAddress + PP_ISBG);
        myfprintf(DumpFile_p, "    ISBG                     = 0x%08x\n", pp_isbg);
        myfprintf(DumpFile_p, "    IPBG                     = 0x%08x\n", PP_Read32(PreprocBaseAddress + PP_IPBG));
        myfprintf(DumpFile_p, "    IBS                      = 0x%08x\n", PP_Read32(PreprocBaseAddress + PP_IBS));
        pp_cfg = PP_Read32(PreprocBaseAddress + PP_CFG);
        myfprintf(DumpFile_p, "    Cfg                      = 0x%08x\n", pp_cfg);
        myfprintf(DumpFile_p, "      MBAFF flag                     = %d\n", (pp_cfg & MASK____PP_CFG__mb_adaptive_frame_field_flag) >> SHIFT___PP_CFG__mb_adaptive_frame_field_flag);
        myfprintf(DumpFile_p, "      Entropy Coding flag            = %d\n", (pp_cfg & MASK____PP_CFG__entropy_coding_mode_flag) >> SHIFT___PP_CFG__entropy_coding_mode_flag);
        myfprintf(DumpFile_p, "      Frame mbs only flag            = %d\n", (pp_cfg & MASK____PP_CFG__frame_mbs_only_flag) >> SHIFT___PP_CFG__frame_mbs_only_flag);
        myfprintf(DumpFile_p, "      POC Type                       = %d\n", (pp_cfg & MASK____PP_CFG__pic_order_cnt_type) >> SHIFT___PP_CFG__pic_order_cnt_type);
        myfprintf(DumpFile_p, "      POC Present flag               = %d\n", (pp_cfg & MASK____PP_CFG__pic_order_present_flag) >> SHIFT___PP_CFG__pic_order_present_flag);
        myfprintf(DumpFile_p, "      Delta POC always Zero flag     = %d\n", (pp_cfg & MASK____PP_CFG__delta_pic_order_always_zero_flag) >> SHIFT___PP_CFG__delta_pic_order_always_zero_flag);
        myfprintf(DumpFile_p, "      Reserved 1                     = %d\n", (pp_cfg & MASK____PP_CFG__reserved_1) >> SHIFT___PP_CFG__reserved_1);
        myfprintf(DumpFile_p, "      Weighted pred flag             = %d\n", (pp_cfg & MASK____PP_CFG__weighted_pred_flag) >> SHIFT___PP_CFG__weighted_pred_flag);
        myfprintf(DumpFile_p, "      Weighted bipred IDC flag       = %d\n", (pp_cfg & MASK____PP_CFG__weighted_bipred_idc_flag) >> SHIFT___PP_CFG__weighted_bipred_idc_flag);
        myfprintf(DumpFile_p, "      DF ctrl present flag           = %d\n", (pp_cfg & MASK____PP_CFG__deblocking_filter_control_present_flag) >> SHIFT___PP_CFG__deblocking_filter_control_present_flag);
        myfprintf(DumpFile_p, "      Num Ref IDX 10 actvie minus 1  = %d\n", (pp_cfg & MASK____PP_CFG__num_ref_idx_l0_active_minus1) >> SHIFT___PP_CFG__num_ref_idx_l0_active_minus1);
        myfprintf(DumpFile_p, "      Num Ref IDX 11 actvie minus 1  = %d\n", (pp_cfg & MASK____PP_CFG__num_ref_idx_l1_active_minus1) >> SHIFT___PP_CFG__num_ref_idx_l1_active_minus1);
        myfprintf(DumpFile_p, "      Pic Init qp                    = %d\n", (pp_cfg & MASK____PP_CFG__pic_init_qp) >> SHIFT___PP_CFG__pic_init_qp);
        myfprintf(DumpFile_p, "      transform 8x8 mode flag        = %d\n", (pp_cfg & MASK____PP_CFG__transform_8x8_mode_flag) >> SHIFT___PP_CFG__transform_8x8_mode_flag);
        myfprintf(DumpFile_p, "      Control mode                   = %d\n", (pp_cfg & MASK____PP_CFG__control_mode) >> SHIFT___PP_CFG__control_mode);
        myfprintf(DumpFile_p, "      direct 8x8 inference flag      = %d\n", (pp_cfg & MASK____PP_CFG__direct_8x8_inference_flag) >> SHIFT___PP_CFG__direct_8x8_inference_flag);
        myfprintf(DumpFile_p, "      Monochrome                     = %d\n", (pp_cfg & MASK____PP_CFG__monochrome) >> SHIFT___PP_CFG__monochrome);
        pp_picwidth = PP_Read32(PreprocBaseAddress + PP_PICWIDTH);
        myfprintf(DumpFile_p, "    Pic Width                = 0x%08x\n", pp_picwidth);
        myfprintf(DumpFile_p, "      Picture Width                  = %d\n", (pp_picwidth & MASK____PP_PICWIDTH__picture_width) >> SHIFT___PP_PICWIDTH__picture_width);
        myfprintf(DumpFile_p, "      Reserved 1                     = %d\n", (pp_picwidth & MASK____PP_PICWIDTH__reserved_1) >> SHIFT___PP_PICWIDTH__reserved_1);
        myfprintf(DumpFile_p, "      NB MB in pict. minus 1         = %d\n", (pp_picwidth & MASK____PP_PICWIDTH__nb_MB_in_picture_minus1) >> SHIFT___PP_PICWIDTH__nb_MB_in_picture_minus1);
        myfprintf(DumpFile_p, "      Reserved 2                     = %d\n", (pp_picwidth & MASK____PP_PICWIDTH__reserved_2) >> SHIFT___PP_PICWIDTH__reserved_2);
        pp_codelength = PP_Read32(PreprocBaseAddress + PP_CODELENGTH);
        myfprintf(DumpFile_p, "    Code Length              = 0x%08x\n", pp_codelength);
        myfprintf(DumpFile_p, "      log2 max frame num             = %d\n", (pp_codelength & MASK____PP_CODELENGTH__log2_max_frame_num) >> SHIFT___PP_CODELENGTH__log2_max_frame_num);
        myfprintf(DumpFile_p, "      log2 max POC lsb               = %d\n", (pp_codelength & MASK____PP_CODELENGTH__log2_max_pic_order_cnt_lsb) >> SHIFT___PP_CODELENGTH__log2_max_pic_order_cnt_lsb);
        myfprintf(DumpFile_p, "      Reserved 1                     = %d\n", (pp_codelength & MASK____PP_CODELENGTH__reserved_1) >> SHIFT___PP_CODELENGTH__reserved_1);
        myfprintf(DumpFile_p, "    Interrupt mask (ITM)     = 0x%08x\n", PP_Read32(PreprocBaseAddress + PP_ITM));
        myfprintf(DumpFile_p, "\n  Results \n");
        myfprintf(DumpFile_p, "    Status (ITS)             = 0x%08x\n", Preproc_Context_p->pp_its);
        myfprintf(DumpFile_p, "      DMA completed                  = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__dma_cmp) >> SHIFT___PP_ITS__dma_cmp);
        myfprintf(DumpFile_p, "      Soft reset completed           = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__srs_cmp) >> SHIFT___PP_ITS__srs_cmp);
        myfprintf(DumpFile_p, "      Error Start code detected      = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__error_sc_detected) >> SHIFT___PP_ITS__error_sc_detected);
        myfprintf(DumpFile_p, "      Error bit inserted             = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__error_bit_inserted) >> SHIFT___PP_ITS__error_bit_inserted);
        myfprintf(DumpFile_p, "      Int buffer overflow            = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__int_buffer_overflow) >> SHIFT___PP_ITS__int_buffer_overflow);
        myfprintf(DumpFile_p, "      Bit buffer underflow           = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__bit_buffer_underflow) >> SHIFT___PP_ITS__bit_buffer_underflow);
        myfprintf(DumpFile_p, "      Bit buffer overflow            = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__bit_buffer_overflow) >> SHIFT___PP_ITS__bit_buffer_overflow);
        myfprintf(DumpFile_p, "      Read Error                     = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__read_error) >> SHIFT___PP_ITS__read_error);
        myfprintf(DumpFile_p, "      Write Error                    = %d\n", (Preproc_Context_p->pp_its & MASK____PP_ITS__write_error) >> SHIFT___PP_ITS__write_error);
        myfprintf(DumpFile_p, "    Last Write Address (WDL) = 0x%08x\n", Preproc_Context_p->pp_wdl);
        myfprintf(DumpFile_p, "    Out. data size (WDL-ISBG)= %d\n", Preproc_Context_p->pp_wdl-pp_isbg);
        myfprintf(DumpFile_p, "\n");
        
        fflush(DumpFile_p);
    }

    return ST_NO_ERROR;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t BBDMP_Init(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    BBDumpNumber = 0;
    
    return ST_NO_ERROR;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t BBDMP_Term(void)
{
#ifdef ST_OSLINUX
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    return ST_NO_ERROR;
#endif
}

/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
ST_ErrorCode_t BBDMP_DumpBitBuffer(H264Preproc_Context_t *Preproc_Context_p)
{
#ifdef ST_OSLINUX
    /* To avoid warnings ... */
    Preproc_Context_p = Preproc_Context_p;

    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    FILE   *fp;
    char    Filename[256];
    void   *PreprocBaseAddress;
    U32     pp_read_start;
    U32     pp_read_stop;
    void   *PictureAddress;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);

    PreprocBaseAddress = Preproc_Context_p->H264PreprocHardware_Data_p->RegistersBaseAddress_p + (PP_OFFSET * Preproc_Context_p->PreprocNumber);

    pp_read_start = PP_Read32(PreprocBaseAddress + PP_READ_START);
    pp_read_stop = PP_Read32(PreprocBaseAddress + PP_READ_STOP);
    
    PictureAddress = STAVMEM_DeviceToCPU((void *)pp_read_start, &VirtualMapping);

    BBDumpNumber++;
    sprintf(Filename, "../../coded_pictures/coded_picture%04d.bin", BBDumpNumber);
    fp = fopen(Filename,"wb");
    fwrite(PictureAddress, 4, (((pp_read_stop-pp_read_start)+4)/4), fp);
    fclose(fp);
    
    return ST_NO_ERROR;
#endif
}

/* C++ support */
#ifdef __cplusplus
}
#endif

/* End of h264preproc_dump.c */
