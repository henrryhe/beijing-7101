/*!
 ************************************************************************
 * \file h264decode_cmd.c
 *
 * \brief Testool commands for the H264 decoder
 *
 * \author
 * Laurent Delaporte \n
 * CMG/STB \n
 * Copyright (C) STMicroelectronics 2007
 *
 ******************************************************************************
 */

#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"

#include "stsys.h"
#include "stvid.h"
#include "vid_com.h"
#include "vid_ctcm.h"

#include "ordqueue.h"
#include "buffers.h"
#include "api.h"

#include "producer.h"

#include "vid_prod.h"

#include "vidcodec.h"

#include "sttbx.h"
#include "stevt.h"
#include "testtool.h"
#include "h264dumpmme.h"
#include "h264decode.h"
#include "deltaphi_reg.h"
#include "delta_pp_registers.h"

#define ReadReg8(a)  STSYS_ReadRegDev8(a)
#define ReadReg16(a) STSYS_ReadRegDev16LE(a)
#define ReadReg24(a) STSYS_ReadRegDev24LE(a)
#define ReadReg32(a) STSYS_ReadRegDev32LE(a)

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#if defined(ST_DELTAMU_GREEN_HE)
#define CPUPLUG_BASE_ADDRESS 0xA3A40000
#else
#define CPUPLUG_BASE_ADDRESS 0xA3A12000
#endif
  static U32 * rst_delta_n          = (U32 *)(CPUPLUG_BASE_ADDRESS + 0x20);
#endif

/* Private Variables (static)------------------------------------------------ */
static char VID_Msg[1024];                    /* text for trace */
#define PP_TEST_CFG          0x4DB9C923
#define PP_TEST_PICWIDTH     0x00010001
#define PP_TEST_CODELENGTH   0x000001D0
static U8 PP_TEST_Data[] = {
0x00, 0x00, 0x00, 0x01, 0x27, 0x64, 0x00, 0x15, 0x08, 0xAC, 0x1B, 0x16, 0x39, 0xB2, 0x00, 0x00,
0x00, 0x01, 0x28, 0x03, 0x48, 0x47, 0x86, 0x83, 0x50, 0x13, 0x02, 0xC1, 0x4A, 0x15, 0x00, 0x00,
0x00, 0x01, 0x65, 0xB0, 0x34, 0x80, 0x00, 0x00, 0x03, 0x01, 0x6F, 0x70, 0x00, 0x14, 0x0A, 0xFF,
0xF6, 0xF7, 0xD0, 0x01, 0xAE, 0x5E, 0x3D, 0x7C, 0xCA, 0xA9, 0xBE, 0xCC, 0xB3, 0x3B, 0x50, 0x92,
0x27, 0x47, 0x24, 0x34, 0xE5, 0x24, 0x84, 0x53, 0x7C, 0xF5, 0x2C, 0x6E, 0x7B, 0x48, 0x1F, 0xC9,
0x8D, 0x73, 0xA8, 0x3F, 0x00, 0x00, 0x01, 0x0A, 0x03};

static U8 PP_TEST_CPUIntSliceErrStatusBufferBegin_p[] = {0x00, 0x00, 0x00, 0x00};
static U8 PP_TEST_EXPECTED_CPUIntSliceErrStatusBufferBegin_p[] = {0x40, 0x00, 0x00, 0x01};

static U8 PP_TEST_CPUIntPictureBufferBegin_p[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static U8 PP_TEST_EXPECTED_CPUIntPictureBufferBegin_p[] = {
0xB0, 0x34, 0x80, 0x00, 0x00, 0x01, 0x6F, 0x70, 0x00, 0x14, 0x0A, 0xE0, 0x80, 0x00, 0x00, 0x60,
0x1F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x7F, 0xFF, 0xF9,
0xE0, 0x80, 0x00, 0x00, 0x60, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x1A, 0xAA, 0xA8, 0x54, 0xAF, 0xFF, 0xFF, 0x9F, 0x7F, 0xFF, 0xF1, 0xBF, 0xFF, 0xFE, 0x15, 0xFF,
0xFF, 0xCB, 0xFF, 0xFC, 0xFF, 0xFF, 0xF1, 0x6F, 0xFF, 0xFF, 0x2E, 0xFF, 0xFF, 0xF1, 0x8F, 0xFF,
0xFC, 0x8F, 0xFF, 0xFF, 0x4F, 0x02, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00};

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
#define EMUL_PP_TEST_Data 0xA1000000 /* quick and dirty to test preprocessor without allocating memory */
#define EMUL_PP_TEST_CPUIntSliceErrStatusBufferBegin_p 0xA1001000
#define EMUL_PP_TEST_CPUIntPictureBufferBegin_p 0xA1002000
#define PP_TEST_PicStart EMUL_PP_TEST_Data
#define PP_TEST_PicStop (PP_TEST_PicStart + sizeof(PP_TEST_Data))

#define PP_TEST_IntSliceErrStatusBufferBegin_p EMUL_PP_TEST_CPUIntSliceErrStatusBufferBegin_p
#define PP_TEST_IntPictureBufferBegin_p EMUL_PP_TEST_CPUIntPictureBufferBegin_p
#define PP_TEST_IntBufferStop_p  (PP_TEST_IntPictureBufferBegin_p + sizeof(PP_TEST_CPUIntPictureBufferBegin_p))
#else
#define SI_PP_TEST_Data 0xA7F00000 /* quick and dirty to test preprocessor without allocating memory */
#define SI_PP_TEST_CPUIntSliceErrStatusBufferBegin_p 0xA7F01000
#define SI_PP_TEST_CPUIntPictureBufferBegin_p 0xA7F02000
#define PP_TEST_PicStart ((U32)SI_PP_TEST_Data & 0x1FFFFFFF)
#define PP_TEST_PicStop (PP_TEST_PicStart + sizeof(PP_TEST_Data))

#define PP_TEST_IntSliceErrStatusBufferBegin_p ((U32)SI_PP_TEST_CPUIntSliceErrStatusBufferBegin_p & 0x1FFFFFFF)
#define PP_TEST_IntPictureBufferBegin_p ((U32)SI_PP_TEST_CPUIntPictureBufferBegin_p & 0x1FFFFFFF)
#define PP_TEST_IntBufferStop_p  (PP_TEST_IntPictureBufferBegin_p + sizeof(PP_TEST_CPUIntPictureBufferBegin_p))
#endif

static U32 pp_itm_ori[2];
static U32 delta_rst_ori;

#if defined(ST_7100)
#define system_config29         0x174
static unsigned int* system_config29_reg    = (unsigned int *)(SYS_CONF_BASE + system_config29);
#endif

/* Global Variables --------------------------------------------------------- */
#define TEST_MODE_MONO_HANDLE      0
#define TEST_MODE_MULTI_HANDLES    1

extern S16   API_TestMode;     /* mono/multi handle    */
extern U32   API_ErrorCount;   /* number of drivers errors detected */
extern BOOL  API_EnableError;  /* if false, commands always return false */
                               /* to avoid macro interruption */

extern H264_DecodingMode_t H264dec_DecodingMode;

extern STVID_Handle_t VID_GetSTVIDHandleFromNumber(S32 InjectNumber);

/*-------------------------------------------------------------------------
 * Function : Viddec_dump_mme
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Viddec_dump_mme(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    BOOL HandleFound;
    VIDPROD_Data_t              *VIDPROD_Data_p;
    DECODER_Handle_t            DecoderHandle;
    H264DecoderPrivateData_t    *PrivateData_p;
    DECODER_Properties_t *       DECODER_Data_p;
    U32 iDump;

    ErrCode = ST_NO_ERROR;
    RetErr = FALSE;

    /* get Handle */
    CurrentHandle = VID_GetSTVIDHandleFromNumber(0);
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)CurrentHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }
    VIDPROD_Data_p = (VIDPROD_Data_t *)(((VIDPROD_Properties_t *)(((VideoUnit_t *)(CurrentHandle))->Device_p->DeviceData_p->ProducerHandle))->InternalProducerHandle);
    DECODER_Data_p  = (DECODER_Properties_t *)VIDPROD_Data_p->DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = ST_NO_ERROR;

        HDM_Init ();
        for (iDump = 0; (ErrCode == ST_NO_ERROR) && (iDump < PrivateData_p->NbMMECmdDumped); iDump++)
        {
            ErrCode = HDM_DumpCommand(MME_SET_GLOBAL_TRANSFORM_PARAMS, &PrivateData_p->H264CompanionDataDump[iDump].CmdInfoParam);
            if (ErrCode == ST_NO_ERROR)
            {
                ErrCode = HDM_DumpCommand(MME_TRANSFORM, &PrivateData_p->H264CompanionDataDump[iDump].CmdInfoFmw);
            }
        }
        HDM_Term();

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );

                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end Viddec_dump_mme() */

static void Viddec_dump_pp_verif_status(U32 pp_status)
{
  U32 bhc_stbrp_req    =0; /* 1 bit */
  U32 stbrp_bhc_ack    =0; /* 1 bit */
  U32 bhc_bsvalid      =0; /* 1 bit */
  U32 ppo_stbwp_req    =0; /* 1 bit */
  U32 stbwp_ppo_ack    =0; /* 1 bit */
  U32 ppo_cps_req      =0; /* 1 bit */
  U32 prs_ppo_valid    =0; /* 1 bit */
  U32 cavlc_ppo_valid  =0; /* 1 bit */
  U32 cbc_ppo_valid    =0; /* 1 bit */
  U32 prs_status       =0; /* 7 bits */
  U32 wplugctrl_status =0; /* 3 bits */
  U32 cavlc_status     =0; /* 8 bits */
  U32 cbc_status       =0; /* 5 bits */

  bhc_stbrp_req    = (0x80000000 & pp_status) >> 31;
  stbrp_bhc_ack    = (0x40000000 & pp_status) >> 30;
  bhc_bsvalid      = (0x20000000 & pp_status) >> 29;
  ppo_stbwp_req    = (0x10000000 & pp_status) >> 28;
  stbwp_ppo_ack    = (0x08000000 & pp_status) >> 27;
  ppo_cps_req      = (0x04000000 & pp_status) >> 26;
  prs_ppo_valid    = (0x02000000 & pp_status) >> 25;
  cavlc_ppo_valid  = (0x01000000 & pp_status) >> 24;
  cbc_ppo_valid    = (0x00800000 & pp_status) >> 23;
  prs_status       = (0x007F0000 & pp_status) >> 16;
  wplugctrl_status = (0x0000E000 & pp_status) >> 13;
  cavlc_status     = (0x00001FE0 & pp_status) >> 5;
  cbc_status       = (0x0000001F & pp_status) >> 0;
  STTBX_Print(("bhc_stbrp_req    : %x\n",bhc_stbrp_req));
  STTBX_Print(("stbrp_bhc_ack    : %x\n",stbrp_bhc_ack));
  STTBX_Print(("bhc_bsvalid      : %x\n",bhc_bsvalid));
  STTBX_Print(("ppo_stbwp_req    : %x\n",ppo_stbwp_req));
  STTBX_Print(("stbwp_ppo_ack    : %x\n",stbwp_ppo_ack));
  STTBX_Print(("ppo_cps_req      : %x\n",ppo_cps_req));
  STTBX_Print(("prs_ppo_valid    : %x\n",prs_ppo_valid));
  STTBX_Print(("cavlc_ppo_valid  : %x\n",cavlc_ppo_valid));
  STTBX_Print(("cbc_ppo_valid    : %x\n",cbc_ppo_valid));
  STTBX_Print(("prs_status       : %x\n",prs_status));
  STTBX_Print(("wplugctrl_status : %x\n",wplugctrl_status));
  STTBX_Print(("cavlc_status     : %x\n",cavlc_status));
  STTBX_Print(("cbc_status       : %x\n",cbc_status));

  switch(prs_status) {
     case 0x0 : { STTBX_Print(("Preprocessor parser is in state : IDLE                                  \n"));} break;
     case 0x1 : { STTBX_Print(("Preprocessor parser is in state : SEARCH_SC                             \n"));} break;
     case 0x2 : { STTBX_Print(("Preprocessor parser is in state : CHECK_NUT                             \n"));} break;
     case 0x3 : { STTBX_Print(("Preprocessor parser is in state : FIRST_MB_IN_SLICE                     \n"));} break;
     case 0x4 : { STTBX_Print(("Preprocessor parser is in state : SLICE_TYPE                            \n"));} break;
     case 0x5 : { STTBX_Print(("Preprocessor parser is in state : PIC_PARAMETER_SET_ID                  \n"));} break;
     case 0x6 : { STTBX_Print(("Preprocessor parser is in state : FRAME_NUM                             \n"));} break;
     case 0x7 : { STTBX_Print(("Preprocessor parser is in state : FIELD_PIC_FLAG                        \n"));} break;
     case 0x8 : { STTBX_Print(("Preprocessor parser is in state : BOTTOM_FIELD_FLAG                     \n"));} break;
     case 0x9 : { STTBX_Print(("Preprocessor parser is in state : IDR_PIC_ID                            \n"));} break;
     case 0xa : { STTBX_Print(("Preprocessor parser is in state : PIC_ORDER_CNT_TYPE_CHECK              \n"));} break;
     case 0xb : { STTBX_Print(("Preprocessor parser is in state : PIC_ORDER_CNT_LSB                     \n"));} break;
     case 0xc : { STTBX_Print(("Preprocessor parser is in state : DELTA_PIC_ORDER_CNT_BOTTOM            \n"));} break;
     case 0xd : { STTBX_Print(("Preprocessor parser is in state : DELTA_PIC_ORDER_CNT_0                 \n"));} break;
     case 0xe : { STTBX_Print(("Preprocessor parser is in state : DELTA_PIC_ORDER_CNT_1                 \n"));} break;
     case 0xf : { STTBX_Print(("Preprocessor parser is in state : REDUNDANT_PIC_CNT                     \n"));} break;
     case 0x10 : { STTBX_Print(("Preprocessor parser is in state : SLICE_TYPE_CHECK                      \n"));} break;
     case 0x11 : { STTBX_Print(("Preprocessor parser is in state : DIRECT_SPATIAL_MV_PRED_FLAG           \n"));} break;
     case 0x12 : { STTBX_Print(("Preprocessor parser is in state : NUM_REF_IDX_ACTIVE_OVERRIDE_FLAG      \n"));} break;
     case 0x13 : { STTBX_Print(("Preprocessor parser is in state : NUM_REF_IDX_L0_ACTIVE_MINUS1          \n"));} break;
     case 0x14 : { STTBX_Print(("Preprocessor parser is in state : NUM_REF_IDX_L1_ACTIVE_MINUS1          \n"));} break;
     case 0x15 : { STTBX_Print(("Preprocessor parser is in state : REF_PIC_LIST_REORDERING               \n"));} break;
     case 0x16 : { STTBX_Print(("Preprocessor parser is in state : REF_PIC_LIST_REORDERING_FLAG_L0       \n"));} break;
     case 0x17 : { STTBX_Print(("Preprocessor parser is in state : REORDERING_OF_PIC_NUMS_IDC            \n"));} break;
     case 0x18 : { STTBX_Print(("Preprocessor parser is in state : ABS_DIFF_PIC_NUM_MINUS1               \n"));} break;
     case 0x19 : { STTBX_Print(("Preprocessor parser is in state : REF_PIC_LONG_TERM_PIC_NUM             \n"));} break;
     case 0x1a : { STTBX_Print(("Preprocessor parser is in state : REF_PIC_LIST_REORDERING_FLAG_L1       \n"));} break;
     case 0x1b : { STTBX_Print(("Preprocessor parser is in state : PRED_WEIGHT_TABLE                     \n"));} break;
     case 0x1c : { STTBX_Print(("Preprocessor parser is in state : LUMA_LOG2_WEIGHT_DENOM                \n"));} break;
     case 0x1d : { STTBX_Print(("Preprocessor parser is in state : CHROMA_LOG2_WEIGHT_DENOM              \n"));} break;
     case 0x1e : { STTBX_Print(("Preprocessor parser is in state : LUMA_WEIGHT_L0_FLAG                   \n"));} break;
     case 0x1f : { STTBX_Print(("Preprocessor parser is in state : LUMA_WEIGHT_L0                        \n"));} break;
     case 0x20 : { STTBX_Print(("Preprocessor parser is in state : LUMA_OFFSET_L0                        \n"));} break;
     case 0x21 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_WEIGHT_L0_FLAG                 \n"));} break;
     case 0x22 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_WEIGHT_L0                      \n"));} break;
     case 0x23 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_OFFSET_L0                      \n"));} break;
     case 0x24 : { STTBX_Print(("Preprocessor parser is in state : LUMA_WEIGHT_L1_FLAG                   \n"));} break;
     case 0x25 : { STTBX_Print(("Preprocessor parser is in state : LUMA_WEIGHT_L1                        \n"));} break;
     case 0x26 : { STTBX_Print(("Preprocessor parser is in state : LUMA_OFFSET_L1                        \n"));} break;
     case 0x27 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_WEIGHT_L1_FLAG                 \n"));} break;
     case 0x28 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_WEIGHT_L1                      \n"));} break;
     case 0x29 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_OFFSET_L1                      \n"));} break;
     case 0x2a : { STTBX_Print(("Preprocessor parser is in state : DEC_REF_PIC_MARKING                   \n"));} break;
     case 0x2b : { STTBX_Print(("Preprocessor parser is in state : NO_OUTPUT_OF_PRIOR_PICS_FLAG          \n"));} break;
     case 0x2c : { STTBX_Print(("Preprocessor parser is in state : LONG_TERM_REFERENCE_FLAG              \n"));} break;
     case 0x2d : { STTBX_Print(("Preprocessor parser is in state : ADAPTIVE_REF_PIC_MARKING_MODE_FLAG    \n"));} break;
     case 0x2e : { STTBX_Print(("Preprocessor parser is in state : MEMORY_MANAGEMENT_CONTROL_OPERATION   \n"));} break;
     case 0x2f : { STTBX_Print(("Preprocessor parser is in state : DIFFERENCE_OF_PIC_NUMS_MINUS1         \n"));} break;
     case 0x30 : { STTBX_Print(("Preprocessor parser is in state : LONG_TERM_PIC_NUM                     \n"));} break;
     case 0x31 : { STTBX_Print(("Preprocessor parser is in state : LONG_TERM_FRAME_IDX                   \n"));} break;
     case 0x32 : { STTBX_Print(("Preprocessor parser is in state : MAX_LONG_TERM_FRAME_IDX_PLUS1         \n"));} break;
     case 0x33 : { STTBX_Print(("Preprocessor parser is in state : CABAC_INIT_IDC                        \n"));} break;
     case 0x34 : { STTBX_Print(("Preprocessor parser is in state : SLICE_QP_DELTA                        \n"));} break;
     case 0x35 : { STTBX_Print(("Preprocessor parser is in state : WAIT_DIV_DONE                         \n"));} break;
     case 0x36 : { STTBX_Print(("Preprocessor parser is in state : SP_FOR_SWITCH_FLAG                    \n"));} break;
     case 0x37 : { STTBX_Print(("Preprocessor parser is in state : SLICE_QS_DELTA                        \n"));} break;
     case 0x38 : { STTBX_Print(("Preprocessor parser is in state : DISABLE_DEBLOCKING_FILTER_IDC         \n"));} break;
     case 0x39 : { STTBX_Print(("Preprocessor parser is in state : SLICE_ALPHA_C0_OFFSET_DIV2            \n"));} break;
     case 0x3a : { STTBX_Print(("Preprocessor parser is in state : SLICE_BETA_OFFSET_DIV2                \n"));} break;
     case 0x3b : { STTBX_Print(("Preprocessor parser is in state : SLICE_GROUP_CHECK                     \n"));} break;
     case 0x3c : { STTBX_Print(("Preprocessor parser is in state : SLICE_GROUP_CHANGE_CYCLE              \n"));} break;
     case 0x3d : { STTBX_Print(("Preprocessor parser is in state : CABAC_ALIGNMENT_ONE_BIT               \n"));} break;
     case 0x3e : { STTBX_Print(("Preprocessor parser is in state : SLICE_DATA                            \n"));} break;
     case 0x3f : { STTBX_Print(("Preprocessor parser is in state : MB_SKIP_RUN                           \n"));} break;
     case 0x40 : { STTBX_Print(("Preprocessor parser is in state : MB_SKIP_FLAG                          \n"));} break;
     case 0x41 : { STTBX_Print(("Preprocessor parser is in state : MB_FIELD_DECODING_FLAG                \n"));} break;
     case 0x42 : { STTBX_Print(("Preprocessor parser is in state : END_OF_SLICE_FLAG                     \n"));} break;
     case 0x43 : { STTBX_Print(("Preprocessor parser is in state : MB_TYPE                               \n"));} break;
     case 0x44 : { STTBX_Print(("Preprocessor parser is in state : CHECK_TRANSFORM_FLAG_PRESENCE1        \n"));} break;
     case 0x45 : { STTBX_Print(("Preprocessor parser is in state : TRANSFORM_SIZE_8X8_FLAG               \n"));} break;
     case 0x46 : { STTBX_Print(("Preprocessor parser is in state : CHECK_MB_TYPE                         \n"));} break;
     case 0x47 : { STTBX_Print(("Preprocessor parser is in state : CABACPCM_FLUSHBS                      \n"));} break;
     case 0x48 : { STTBX_Print(("Preprocessor parser is in state : PCM_ALIGNMENT_ZERO_BIT                \n"));} break;
     case 0x49 : { STTBX_Print(("Preprocessor parser is in state : SUB_MB_TYPE0                          \n"));} break;
     case 0x4a : { STTBX_Print(("Preprocessor parser is in state : SUB_MB_TYPE1                          \n"));} break;
     case 0x4b : { STTBX_Print(("Preprocessor parser is in state : SUB_MB_TYPE2                          \n"));} break;
     case 0x4c : { STTBX_Print(("Preprocessor parser is in state : SUB_MB_TYPE3                          \n"));} break;
     case 0x4d : { STTBX_Print(("Preprocessor parser is in state : CHECK_SUB_MB_TYPE                     \n"));} break;
     case 0x4e : { STTBX_Print(("Preprocessor parser is in state : PREV_INTRA_PRED_MODE_FLAG             \n"));} break;
     case 0x4f : { STTBX_Print(("Preprocessor parser is in state : REM_INTRA_PRED_MODE                   \n"));} break;
     case 0x50 : { STTBX_Print(("Preprocessor parser is in state : INTRA_CHROMA_PRED_MODE                \n"));} break;
     case 0x51 : { STTBX_Print(("Preprocessor parser is in state : REF_IDX_L0                            \n"));} break;
     case 0x52 : { STTBX_Print(("Preprocessor parser is in state : REF_IDX_L1                            \n"));} break;
     case 0x53 : { STTBX_Print(("Preprocessor parser is in state : MVD_L0                                \n"));} break;
     case 0x54 : { STTBX_Print(("Preprocessor parser is in state : MVD_L1                                \n"));} break;
     case 0x55 : { STTBX_Print(("Preprocessor parser is in state : CODED_BLOCK_PATTERN                   \n"));} break;
     case 0x56 : { STTBX_Print(("Preprocessor parser is in state : CHECK_TRANSFORM_FLAG_PRESENCE2        \n"));} break;
     case 0x57 : { STTBX_Print(("Preprocessor parser is in state : CHECK_CBP                             \n"));} break;
     case 0x58 : { STTBX_Print(("Preprocessor parser is in state : MB_QP_DELTA                           \n"));} break;
     case 0x59 : { STTBX_Print(("Preprocessor parser is in state : WRITE_MBSTRUCT_WORD1                  \n"));} break;
     case 0x5a : { STTBX_Print(("Preprocessor parser is in state : WRITE_MBSTRUCT_WORD2                  \n"));} break;
     case 0x5b : { STTBX_Print(("Preprocessor parser is in state : WRITE_MBSTRUCT_WORD3                  \n"));} break;
     case 0x5c : { STTBX_Print(("Preprocessor parser is in state : WRITE_MBSTRUCT_WORD4                  \n"));} break;
     case 0x5d : { STTBX_Print(("Preprocessor parser is in state : WRITE_MBSTRUCT_WORD0                  \n"));} break;
     case 0x5e : { STTBX_Print(("Preprocessor parser is in state : WAIT_PPO_DONE                         \n"));} break;
     case 0x5f : { STTBX_Print(("Preprocessor parser is in state : START_TOP_SKIPPED_MB                  \n"));} break;
     case 0x60 : { STTBX_Print(("Preprocessor parser is in state : WAIT_TOP_SKIPPED_MB_DONE              \n"));} break;
     case 0x61 : { STTBX_Print(("Preprocessor parser is in state : RESIDUAL                              \n"));} break;
     case 0x62 : { STTBX_Print(("Preprocessor parser is in state : I16X16_LUMA_DC                        \n"));} break;
     case 0x63 : { STTBX_Print(("Preprocessor parser is in state : LUMA                                  \n"));} break;
     case 0x64 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_DC                             \n"));} break;
     case 0x65 : { STTBX_Print(("Preprocessor parser is in state : CHROMA_AC                             \n"));} break;
     case 0x66 : { STTBX_Print(("Preprocessor parser is in state : WAIT_BIN_SENT                         \n"));} break;
     case 0x67 : { STTBX_Print(("Preprocessor parser is in state : WAIT_CAVLC_DONE                       \n"));} break;
     case 0x68 : { STTBX_Print(("Preprocessor parser is in state : FLUSH_BS                              \n"));} break;
     case 0x69 : { STTBX_Print(("Preprocessor parser is in state : BYTE_ALIGN_IN                         \n"));} break;
     case 0x6a : { STTBX_Print(("Preprocessor parser is in state : BYTE_ALIGN_OUT                        \n"));} break;
     case 0x6b : { STTBX_Print(("Preprocessor parser is in state : EOS_SYNCHRO                           \n"));} break;
     case 0x6c : { STTBX_Print(("Preprocessor parser is in state : ERROR_BIT_INSERTION                   \n"));} break;
     case 0x6d : { STTBX_Print(("Preprocessor parser is in state : ERROR_SC_DETECTED                     \n"));} break;
     case 0x6e : { STTBX_Print(("Preprocessor parser is in state : BIT_BUFFER_OVERFLOW                   \n"));} break;
     case 0x6f : { STTBX_Print(("Preprocessor parser is in state : OTHERS                                \n"));} break;
  }

  switch(wplugctrl_status) {
     case 0x0 : { STTBX_Print(("Preprocessor outputstage is in state : IDLE                \n"));} break;
     case 0x1 : { STTBX_Print(("Preprocessor outputstage is in state : SLICE_DECODE        \n"));} break;
     case 0x2 : { STTBX_Print(("Preprocessor outputstage is in state : SLICE_STATUS_WRITE  \n"));} break;
     case 0x3 : { STTBX_Print(("Preprocessor outputstage is in state : FREEZE_PARSER       \n"));} break;
     case 0x4 : { STTBX_Print(("Preprocessor outputstage is in state : CHECK_READ_DMA      \n"));} break;
     case 0x5 : { STTBX_Print(("Preprocessor outputstage is in state : CHECK_WRITE_DMA     \n"));} break;
     case 0x6 : { STTBX_Print(("Preprocessor outputstage is in state : OTHERS              \n"));} break;
  }
}

/*-------------------------------------------------------------------------
 * Function : Viddec_dump_pp
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Viddec_dump_pp(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    BOOL HandleFound;
    VIDPROD_Data_t              *VIDPROD_Data_p;
    DECODER_Handle_t            DecoderHandle;
    H264DecoderPrivateData_t    *PrivateData_p;
    DECODER_Properties_t *       DECODER_Data_p;
    U32 iDump, value;

    ErrCode = ST_NO_ERROR;
    RetErr = FALSE;

    /* get Handle */
    CurrentHandle = VID_GetSTVIDHandleFromNumber(0);
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)CurrentHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }

    if (!(RetErr))
    {
        RetErr = TRUE;

        STTBX_Print(("\nPREPROCESSOR 1\n"));
        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_READ_START);
        STTBX_Print(("DTP_PP_READ_START                     Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_READ_START, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_READ_STOP);
        STTBX_Print(("DTP_PP_READ_STOP                      Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_READ_STOP, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_BBG);
        STTBX_Print(("DTP_PP_BBG                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_BBG, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_BBS);
        STTBX_Print(("DTP_PP_BBS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_BBS, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_ISBG);
        STTBX_Print(("DTP_PP_ISBG                           Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_ISBG, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_IPBG);
        STTBX_Print(("DTP_PP_IPBG                           Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_IPBG, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_IBS);
        STTBX_Print(("DTP_PP_IBS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_IBS, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_WDL);
        STTBX_Print(("DTP_PP_WDL                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_WDL, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_CFG);
        STTBX_Print(("DTP_PP_CFG                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_CFG, value));
        STTBX_Print(("    MB_ADAPTIVE_FRAME_FIELD_FLAG              : 0x%04x\n", (value & DTP_PP_CFG_MASK_MB_ADAPTIVE_FRAME_FIELD_FLAG) >> DTP_PP_CFG_SHFT_MB_ADAPTIVE_FRAME_FIELD_FLAG));
        STTBX_Print(("    ENTROPY_CODING_MODE_FLAG                  : 0x%04x\n", (value & DTP_PP_CFG_MASK_ENTROPY_CODING_MODE_FLAG) >> DTP_PP_CFG_SHFT_ENTROPY_CODING_MODE_FLAG));
        STTBX_Print(("    FRAME_MBS_ONLY_FLAG                       : 0x%04x\n", (value & DTP_PP_CFG_MASK_FRAME_MBS_ONLY_FLAG) >> DTP_PP_CFG_SHFT_FRAME_MBS_ONLY_FLAG));
        STTBX_Print(("    PIC_ORDER_CNT_TYPE                        : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_ORDER_CNT_TYPE) >> DTP_PP_CFG_SHFT_PIC_ORDER_CNT_TYPE));
        STTBX_Print(("    PIC_ORDER_PRESENT_FLAG                    : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_ORDER_PRESENT_FLAG) >> DTP_PP_CFG_SHFT_PIC_ORDER_PRESENT_FLAG));
        STTBX_Print(("    DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG        : 0x%04x\n", (value & DTP_PP_CFG_MASK_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG) >> DTP_PP_CFG_SHFT_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG));
        STTBX_Print(("    WEIGHTED_PRED_FLAG                        : 0x%04x\n", (value & DTP_PP_CFG_MASK_WEIGHTED_PRED_FLAG) >> DTP_PP_CFG_SHFT_WEIGHTED_PRED_FLAG));
        STTBX_Print(("    WEIGHTED_BIPRED_IDC_FLAG                  : 0x%04x\n", (value & DTP_PP_CFG_MASK_WEIGHTED_BIPRED_IDC_FLAG) >> DTP_PP_CFG_SHFT_WEIGHTED_BIPRED_IDC_FLAG));
        STTBX_Print(("    DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG    : 0x%04x\n", (value & DTP_PP_CFG_MASK_DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG) >> DTP_PP_CFG_SHFT_DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG));
        STTBX_Print(("    NUM_REF_IDX_I0_ACTIVE_MINUS1              : 0x%04x\n", (value & DTP_PP_CFG_MASK_NUM_REF_IDX_I0_ACTIVE_MINUS1) >> DTP_PP_CFG_SHFT_NUM_REF_IDX_I0_ACTIVE_MINUS1));
        STTBX_Print(("    NUM_REF_IDX_I1_ACTIVE_MINUS1              : 0x%04x\n", (value & DTP_PP_CFG_MASK_NUM_REF_IDX_I1_ACTIVE_MINUS1) >> DTP_PP_CFG_SHFT_NUM_REF_IDX_I1_ACTIVE_MINUS1));
        STTBX_Print(("    PIC_INIT_QP                               : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_INIT_QP) >> DTP_PP_CFG_SHFT_PIC_INIT_QP));
        STTBX_Print(("    TRANSFORM_8X8_MODE_FLAG                   : 0x%04x\n", (value & DTP_PP_CFG_MASK_TRANSFORM_8X8_MODE_FLAG) >> DTP_PP_CFG_SHFT_TRANSFORM_8X8_MODE_FLAG));
        STTBX_Print(("    DIRECT_8X8_INFERENCE_FLAG                 : 0x%04x\n", (value & DTP_PP_CFG_MASK_DIRECT_8X8_INFERENCE_FLAG) >> DTP_PP_CFG_SHFT_DIRECT_8X8_INFERENCE_FLAG));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_PICWIDTH);
        STTBX_Print(("DTP_PP_PICWIDTH                       Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_PICWIDTH, value));
        STTBX_Print(("    PICTURE_WIDTH                             : 0x%04x\n", (value & DTP_PP_PICWIDTH_MASK_PICTURE_WIDTH) >> DTP_PP_PICWIDTH_SHFT_PICTURE_WIDTH));
        STTBX_Print(("    NB_MB_IN_PICTURE_MINUS1                   : 0x%04x\n", (value & DTP_PP_PICWIDTH_MASK_NB_MB_IN_PICTURE_MINUS1) >> DTP_PP_PICWIDTH_SHFT_NB_MB_IN_PICTURE_MINUS1));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_CODELENGTH);
        STTBX_Print(("DTP_PP_CODELENGTH                     Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_CODELENGTH, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_ITS);
        STTBX_Print(("DTP_PP_ITS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_ITS, value));

        value = ReadReg32 (DTP_PP1_BASE_ADDRESS + DTP_PP_ITM);
        STTBX_Print(("DTP_PP_ITM                            Addr 0x%08x, Value 0x%08x\n", DTP_PP1_BASE_ADDRESS + DTP_PP_ITM, value));

#if defined(ST_DELTAMU_HE) || (defined(ST_7100)  && !defined(ST_CUT_1_X)) || defined(ST_7109) || defined(7200) || defined(ST_ZEUS)
        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_PP1_VERIF_STA);
        STTBX_Print(("DTP_PP_VERIF_STA                      Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_PP1_VERIF_STA, value));
        Viddec_dump_pp_verif_status(value);
#endif /* defined(ST_DELTAMU_HE) || (defined(ST_7100) && !defined(ST_CUT_1_X)) */

#if (defined(ST_DELTAMU_HE) && !defined(ST_DELTAMU_GREEN_HE)) || defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X)) || (defined(ST_ZEUS)  && defined(ST_CUT_1_X))
        STTBX_Print(("\nPREPROCESSOR 2\n"));
        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_READ_START);
        STTBX_Print(("DTP_PP_READ_START                     Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_READ_START, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_READ_STOP);
        STTBX_Print(("DTP_PP_READ_STOP                      Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_READ_STOP, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_BBG);
        STTBX_Print(("DTP_PP_BBG                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_BBG, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_BBS);
        STTBX_Print(("DTP_PP_BBS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_BBS, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_ISBG);
        STTBX_Print(("DTP_PP_ISBG                           Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_ISBG, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_IPBG);
        STTBX_Print(("DTP_PP_IPBG                           Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_IPBG, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_IBS);
        STTBX_Print(("DTP_PP_IBS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_IBS, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_WDL);
        STTBX_Print(("DTP_PP_WDL                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_WDL, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_CFG);
        STTBX_Print(("DTP_PP_CFG                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_CFG, value));
        STTBX_Print(("    MB_ADAPTIVE_FRAME_FIELD_FLAG              : 0x%04x\n", (value & DTP_PP_CFG_MASK_MB_ADAPTIVE_FRAME_FIELD_FLAG) >> DTP_PP_CFG_SHFT_MB_ADAPTIVE_FRAME_FIELD_FLAG));
        STTBX_Print(("    ENTROPY_CODING_MODE_FLAG                  : 0x%04x\n", (value & DTP_PP_CFG_MASK_ENTROPY_CODING_MODE_FLAG) >> DTP_PP_CFG_SHFT_ENTROPY_CODING_MODE_FLAG));
        STTBX_Print(("    FRAME_MBS_ONLY_FLAG                       : 0x%04x\n", (value & DTP_PP_CFG_MASK_FRAME_MBS_ONLY_FLAG) >> DTP_PP_CFG_SHFT_FRAME_MBS_ONLY_FLAG));
        STTBX_Print(("    PIC_ORDER_CNT_TYPE                        : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_ORDER_CNT_TYPE) >> DTP_PP_CFG_SHFT_PIC_ORDER_CNT_TYPE));
        STTBX_Print(("    PIC_ORDER_PRESENT_FLAG                    : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_ORDER_PRESENT_FLAG) >> DTP_PP_CFG_SHFT_PIC_ORDER_PRESENT_FLAG));
        STTBX_Print(("    DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG          : 0x%04x\n", (value & DTP_PP_CFG_MASK_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG) >> DTP_PP_CFG_SHFT_DELTA_PIC_ORDER_ALWAYS_ZERO_FLAG));
        STTBX_Print(("    WEIGHTED_PRED_FLAG                        : 0x%04x\n", (value & DTP_PP_CFG_MASK_WEIGHTED_PRED_FLAG) >> DTP_PP_CFG_SHFT_WEIGHTED_PRED_FLAG));
        STTBX_Print(("    WEIGHTED_BIPRED_IDC_FLAG                  : 0x%04x\n", (value & DTP_PP_CFG_MASK_WEIGHTED_BIPRED_IDC_FLAG) >> DTP_PP_CFG_SHFT_WEIGHTED_BIPRED_IDC_FLAG));
        STTBX_Print(("    DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG    : 0x%04x\n", (value & DTP_PP_CFG_MASK_DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG) >> DTP_PP_CFG_SHFT_DEBLOCKING_FILTER_CONTROL_PRESENT_FLAG));
        STTBX_Print(("    NUM_REF_IDX_I0_ACTIVE_MINUS1              : 0x%04x\n", (value & DTP_PP_CFG_MASK_NUM_REF_IDX_I0_ACTIVE_MINUS1) >> DTP_PP_CFG_SHFT_NUM_REF_IDX_I0_ACTIVE_MINUS1));
        STTBX_Print(("    NUM_REF_IDX_I1_ACTIVE_MINUS1              : 0x%04x\n", (value & DTP_PP_CFG_MASK_NUM_REF_IDX_I1_ACTIVE_MINUS1) >> DTP_PP_CFG_SHFT_NUM_REF_IDX_I1_ACTIVE_MINUS1));
        STTBX_Print(("    PIC_INIT_QP                               : 0x%04x\n", (value & DTP_PP_CFG_MASK_PIC_INIT_QP) >> DTP_PP_CFG_SHFT_PIC_INIT_QP));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_PICWIDTH);
        STTBX_Print(("DTP_PP_PICWIDTH                       Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_PICWIDTH, value));
        STTBX_Print(("    PICTURE_WIDTH                             : 0x%04x\n", (value & DTP_PP_PICWIDTH_MASK_PICTURE_WIDTH) >> DTP_PP_PICWIDTH_SHFT_PICTURE_WIDTH));
        STTBX_Print(("    NB_MB_IN_PICTURE_MINUS1                   : 0x%04x\n", (value & DTP_PP_PICWIDTH_MASK_NB_MB_IN_PICTURE_MINUS1) >> DTP_PP_PICWIDTH_SHFT_NB_MB_IN_PICTURE_MINUS1));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_CODELENGTH);
        STTBX_Print(("DTP_PP_CODELENGTH                     Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_CODELENGTH, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_ITS);
        STTBX_Print(("DTP_PP_ITS                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_ITS, value));

        value = ReadReg32 (DTP_PP2_BASE_ADDRESS + DTP_PP_ITM);
        STTBX_Print(("DTP_PP_ITM                            Addr 0x%08x, Value 0x%08x\n", DTP_PP2_BASE_ADDRESS + DTP_PP_ITM, value));

#if defined(ST_DELTAMU_HE) || (defined(ST_7100)  && !defined(ST_CUT_1_X)) || (defined(ST_7109) && defined(ST_CUT_1_X)) || (defined(ST_ZEUS) && defined(ST_CUT_1_X))
        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_PP2_VERIF_STA);
        STTBX_Print(("DTP_PP_VERIF_STA                      Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_PP2_VERIF_STA, value));
        Viddec_dump_pp_verif_status(value);
#endif /* defined(ST_DELTAMU_HE) || (defined(ST_7100) && !defined(ST_CUT_1_X)) */

#endif /* defined(ST_DELTAMU_HE) && !defined(ST_DELTAMU_GREEN_HE)) || (defined(ST_7100) || (defined(ST_7109) && defined(ST_CUT_1_X)) */

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );

                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end Viddec_dump_pp() */
/*-------------------------------------------------------------------------
 * Function : Viddec_dump_mbe
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Viddec_dump_mbe(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    BOOL HandleFound;
    VIDPROD_Data_t              *VIDPROD_Data_p;
    DECODER_Handle_t            DecoderHandle;
    H264DecoderPrivateData_t    *PrivateData_p;
    DECODER_Properties_t *       DECODER_Data_p;
    U32 iDump, value;
    U32 RefNumber;

    ErrCode = ST_NO_ERROR;
    RetErr = FALSE;

    /* get Handle */
    CurrentHandle = VID_GetSTVIDHandleFromNumber(0);
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)CurrentHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }
    if (!(RetErr))
    {
        RetErr = TRUE;

        STTBX_Print(("\nMBE \n"));

        value = ReadReg32 (DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_STA);
        STTBX_Print(("DTP_MBE_STA                           Addr 0x%08x, Value 0x%08x\n", DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_STA, value));
        STTBX_Print(("    RCP                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_RCP) >> DTP_MBE_STA_SHFT_RCP));
        STTBX_Print(("    SED                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_SED) >> DTP_MBE_STA_SHFT_SED));
        STTBX_Print(("    SEL                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_SEL) >> DTP_MBE_STA_SHFT_SEL));
        STTBX_Print(("    RSY                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_RSY) >> DTP_MBE_STA_SHFT_RSY));
        STTBX_Print(("    RSE                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_RSE) >> DTP_MBE_STA_SHFT_RSE));
        STTBX_Print(("    EBR                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_EBR) >> DTP_MBE_STA_SHFT_EBR));
        STTBX_Print(("    EBL                                       : 0x%04x\n", (value & DTP_MBE_STA_MASK_EBL) >> DTP_MBE_STA_SHFT_EBL));
        STTBX_Print(("    IMVPSTATUS                                : 0x%04x\n", (value & DTP_MBE_STA_MASK_IMVPSTATUS) >> DTP_MBE_STA_SHFT_IMVPSTATUS));
        STTBX_Print(("    DFRCN_UNEXPECTED_DATA                     : 0x%04x\n", (value & DTP_MBE_STA_MASK_DFRCN_UNEXPECTED_DATA) >> DTP_MBE_STA_SHFT_DFRCN_UNEXPECTED_DATA));
        STTBX_Print(("    DFRCN_END                                 : 0x%04x\n", (value & DTP_MBE_STA_MASK_DFRCN_END) >> DTP_MBE_STA_SHFT_DFRCN_END));
        STTBX_Print(("    MBE_PLUG_ROPC                             : 0x%04x\n", (value & DTP_MBE_STA_MASK_MBE_PLUG_ROPC) >> DTP_MBE_STA_SHFT_MBE_PLUG_ROPC));

        value = ReadReg32 (DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_ITS);
        STTBX_Print(("DTP_MBE_ITS                           Addr 0x%08x, Value 0x%08x\n", DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_ITS, value));

        value = ReadReg32 (DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_ITM);
        STTBX_Print(("DTP_MBE_ITM                           Addr 0x%08x, Value 0x%08x\n", DTP_MBE_BLK_BASE_ADDRESS + DTP_MBE_ITM, value));

        value = ReadReg32 (DTP_MBE_BLK_BASE_ADDRESS + DTP_BH_RDP);
        STTBX_Print(("DTP_BH_RDP                            Addr 0x%08x, Value 0x%08x\n", DTP_MBE_BLK_BASE_ADDRESS + DTP_BH_RDP, value));

        value = ReadReg32 (DTP_MBE_BLK_BASE_ADDRESS + DTP_DF_RCN_MB);
        STTBX_Print(("DTP_DF_RCN_MB                         Addr 0x%08x, Value 0x%08x\n", DTP_MBE_BLK_BASE_ADDRESS + DTP_DF_RCN_MB, value));

        STTBX_Print(("\nPicture commands \n"));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_NAL_REF_IDC);
        STTBX_Print(("DTP_HWCFG_PIC_NAL_REF_IDC             Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_NAL_REF_IDC, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_PICTUREWIDTH);
        STTBX_Print(("DTP_HWCFG_PIC_PICTUREWIDTH            Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_PICTUREWIDTH, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_PICTUREHEIGHT);
        STTBX_Print(("DTP_HWCFG_PIC_PICTUREHEIGHT           Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_PICTUREHEIGHT, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_FIELDPICTURE);
        STTBX_Print(("DTP_HWCFG_PIC_FIELDPICTURE            Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_FIELDPICTURE, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBAFF_FLAG);
        STTBX_Print(("DTP_HWCFG_PIC_MBAFF_FLAG              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBAFF_FLAG, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_CONSTR_INTRAPRED_FLAG);
        STTBX_Print(("DTP_HWCFG_PIC_CONSTR_INTRAPRED_FLAG   Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_CONSTR_INTRAPRED_FLAG, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_DECIMATION);
        STTBX_Print(("DTP_HWCFG_PIC_DECIMATION              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_DECIMATION, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_BBG);
        STTBX_Print(("DTP_HWCFG_PIC_READ_BBG                Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_BBG, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_BBS);
        STTBX_Print(("DTP_HWCFG_PIC_READ_BBS                Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_BBS, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_START);
        STTBX_Print(("DTP_HWCFG_PIC_READ_START              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_START, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_STOP);
        STTBX_Print(("DTP_HWCFG_PIC_READ_STOP               Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_READ_STOP, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_Y_FRAMEPTR);
        STTBX_Print(("DTP_HWCFG_PIC_Y_FRAMEPTR              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_Y_FRAMEPTR, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_C_FRAMEPTR);
        STTBX_Print(("DTP_HWCFG_PIC_C_FRAMEPTR              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_C_FRAMEPTR, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBDESCRPTR);
        STTBX_Print(("DTP_HWCFG_PIC_MBDESCRPTR              Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBDESCRPTR, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_SEC_Y_FRAMEPTR);
        STTBX_Print(("DTP_HWCFG_PIC_SEC_Y_FRAMEPTR          Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_SEC_Y_FRAMEPTR, value));

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_SEC_C_FRAMEPTR);
        STTBX_Print(("DTP_HWCFG_PIC_SEC_C_FRAMEPTR          Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_SEC_C_FRAMEPTR, value));

        for(RefNumber = 0; RefNumber < 16 ; RefNumber++)
        {
            value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_Y_REFPTR + (RefNumber * 4));
            STTBX_Print(("DTP_HWCFG_PIC_Y_REFPTR      [%2d]      Addr 0x%08x, Value 0x%08x\n", RefNumber, DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_Y_REFPTR + (RefNumber * 4), value));

            value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_C_REFPTR + (RefNumber * 4));
            STTBX_Print(("DTP_HWCFG_PIC_C_REFPTR      [%2d]      Addr 0x%08x, Value 0x%08x\n", RefNumber, DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_C_REFPTR + (RefNumber * 4), value));

            value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBDESCR_REFPTR + (RefNumber * 4));
            STTBX_Print(("DTP_HWCFG_PIC_MBDESCR_REFPTR[%2d]      Addr 0x%08x, Value 0x%08x\n", RefNumber, DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_MBDESCR_REFPTR + (RefNumber * 4), value));
        }

        value = ReadReg32 (DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_REFFIELDFRAMEFLAG);
        STTBX_Print(("DTP_HWCFG_PIC_REFFIELDFRAMEFLAG       Addr 0x%08x, Value 0x%08x\n", DTP_MBE_PIC_BASE_ADDRESS + DTP_HWCFG_PIC_REFFIELDFRAMEFLAG, value));

        STTBX_Print(("\nShort MB commands \n"));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_RESTYPE);
        STTBX_Print(("DTP_HWCFG_SMB_RESTYPE                 Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_RESTYPE, value));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_MB_FIELD_DECODING_FLAG);
        STTBX_Print(("DTP_HWCFG_SMB_MB_FIELD_DECODING_FLAG  Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_MB_FIELD_DECODING_FLAG, value));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_QPY);
        STTBX_Print(("DTP_HWCFG_SMB_QPY                     Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_QPY, value));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_QPC);
        STTBX_Print(("DTP_HWCFG_SMB_QPC                     Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_QPC, value));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_CBP);
        STTBX_Print(("DTP_HWCFG_SMB_CBP                     Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_CBP, value));

        value = ReadReg32 (DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_SKIPRESIDUAL);
        STTBX_Print(("DTP_HWCFG_SMB_SKIPRESIDUAL            Addr 0x%08x, Value 0x%08x\n", DTP_MBE_SMB_BASE_ADDRESS + DTP_HWCFG_SMB_SKIPRESIDUAL, value));

#if defined(ST_DELTAMU_HE) || (defined(ST_7100) && !defined(ST_CUT_1_X)) || defined(ST_7109) || defined(7200) || defined(ST_ZEUS)
        STTBX_Print(("\nDebug registers\n"));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_PRED_VERIF_STA0);
        STTBX_Print(("DTP_PRED_VERIF_STA0                   Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_PRED_VERIF_STA0, value));
        STTBX_Print(("    FLT_FSM_OUT                               : 0x%04x (", (value & DTP_PRED_VERIF_STA0_MASK_FLT_FSM_OUT) >> DTP_PRED_VERIF_STA0_SHFT_FLT_FSM_OUT));
        switch(value & DTP_PRED_VERIF_STA0_MASK_FLT_FSM_OUT)
        {
            case DTP_PRED_VERIF_STA0_FLT_FSM_OUT_WT_LUMA :
                STTBX_Print(("WT_LUMA)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_OUT_SEND_LUMA :
                STTBX_Print(("SEND_LUMA)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_OUT_WT_CHROMA :
                STTBX_Print(("WT_CHROMA)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_OUT_SEND_CHROMA :
                STTBX_Print(("SEND_CHROMA)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    FLT_FSM                                   : 0x%04x (", (value & DTP_PRED_VERIF_STA0_MASK_FLT_FSM) >> DTP_PRED_VERIF_STA0_SHFT_FLT_FSM));
        switch(value & DTP_PRED_VERIF_STA0_MASK_FLT_FSM)
        {
            case DTP_PRED_VERIF_STA0_FLT_FSM_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_UPD_PART_PARAM :
                STTBX_Print(("UPD_PART_PARAM)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_INIT_FIFOS :
                STTBX_Print(("INIT_FIFOS)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_PIPE_DEPTH_COMP :
                STTBX_Print(("PIPE_DEPTH_COMP)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_NOTHING :
                STTBX_Print(("NOTHING)\n"));
                break;
            case DTP_PRED_VERIF_STA0_FLT_FSM_MONOPRED_MODE :
                STTBX_Print(("MONOPRED_MODE)\n"));
                break;
/* Same value as RESET !!!!
            case DTP_PRED_VERIF_STA0_FLT_FSM_BIPRED_MODE :
                STTBX_Print(("BIPRED_MODE)\n"));
                break;
*/
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    UNEXP_CMD_CODE                            : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_UNEXP_CMD_CODE) >> DTP_PRED_VERIF_STA0_SHFT_UNEXP_CMD_CODE));
        STTBX_Print(("    EXTRA_LMB_ERR                             : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_EXTRA_LMB_ERR) >> DTP_PRED_VERIF_STA0_SHFT_EXTRA_LMB_ERR));
        STTBX_Print(("    EXTRA_SLICE_ERR                           : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_EXTRA_SLICE_ERR) >> DTP_PRED_VERIF_STA0_SHFT_EXTRA_SLICE_ERR));
        STTBX_Print(("    NBMB_INSIDE                               : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_NBMB_INSIDE) >> DTP_PRED_VERIF_STA0_SHFT_NBMB_INSIDE));
        STTBX_Print(("    PART_IN_XPREDMAC                          : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_PART_IN_XPREDMAC) >> DTP_PRED_VERIF_STA0_SHFT_PART_IN_XPREDMAC));
        STTBX_Print(("    MB_POS_X                                  : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_MB_POS_X) >> DTP_PRED_VERIF_STA0_SHFT_MB_POS_X));
        STTBX_Print(("    MB_POS_Y                                  : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_MB_POS_Y) >> DTP_PRED_VERIF_STA0_SHFT_MB_POS_Y));
        STTBX_Print(("    XPRED_NXTVLD                              : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_XPRED_NXTVLD) >> DTP_PRED_VERIF_STA0_SHFT_XPRED_NXTVLD));
        STTBX_Print(("    IPRED_REQ                                 : 0x%04x\n", (value & DTP_PRED_VERIF_STA0_MASK_IPRED_REQ) >> DTP_PRED_VERIF_STA0_SHFT_IPRED_REQ));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_PRED_VERIF_STA1);
        STTBX_Print(("DTP_PRED_VERIF_STA1                   Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_PRED_VERIF_STA1, value));
        STTBX_Print(("    IPRED_OFIFO_LEV                           : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_IPRED_OFIFO_LEV) >> DTP_PRED_VERIF_STA1_SHFT_IPRED_OFIFO_LEV));
        STTBX_Print(("    S_CMD_MGT                                 : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_S_CMD_MGT) >> DTP_PRED_VERIF_STA1_SHFT_S_CMD_MGT));
        STTBX_Print(("    SLICE_CMD_PEND                            : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_SLICE_CMD_PEND) >> DTP_PRED_VERIF_STA1_SHFT_SLICE_CMD_PEND));
        STTBX_Print(("    LMB_CMD_PEND                              : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_LMB_CMD_PEND) >> DTP_PRED_VERIF_STA1_SHFT_LMB_CMD_PEND));
        STTBX_Print(("    IPRED_MB_XPOS                             : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_IPRED_MB_XPOS) >> DTP_PRED_VERIF_STA1_SHFT_IPRED_MB_XPOS));
        STTBX_Print(("    XPREDM_PARMEM_FULL                        : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_XPREDM_PARMEM_FULL) >> DTP_PRED_VERIF_STA1_SHFT_XPREDM_PARMEM_FULL));
        STTBX_Print(("    XPREDM_MEM_FULL                           : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_XPREDM_MEM_FULL) >> DTP_PRED_VERIF_STA1_SHFT_XPREDM_MEM_FULL));
        STTBX_Print(("    XPREDM_LINE_TAKEN                         : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_XPREDM_LINE_TAKEN) >> DTP_PRED_VERIF_STA1_SHFT_XPREDM_LINE_TAKEN));
        STTBX_Print(("    XPREDM_LINE_VAL                           : 0x%04x\n", (value & DTP_PRED_VERIF_STA1_MASK_XPREDM_LINE_VAL) >> DTP_PRED_VERIF_STA1_SHFT_XPREDM_LINE_VAL));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_RCN_MAIN_VERIF_STA);
        STTBX_Print(("DTP_RCN_MAIN_VERIF_STA                Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_RCN_MAIN_VERIF_STA, value));
        STTBX_Print(("    Y_MBNB                                    : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_Y_MBNB) >> DTP_RCN_VERIF_STA_SHFT_Y_MBNB));
        STTBX_Print(("    C_MBNB                                    : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_C_MBNB) >> DTP_RCN_VERIF_STA_SHFT_C_MBNB));
        STTBX_Print(("    TR_PEND                                   : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_TR_PEND) >> DTP_RCN_VERIF_STA_SHFT_TR_PEND));
        STTBX_Print(("    END                                       : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_END) >> DTP_RCN_VERIF_STA_SHFT_END));
        STTBX_Print(("    PLUG_R_ERR                                : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_PLUG_R_ERR) >> DTP_RCN_VERIF_STA_SHFT_PLUG_R_ERR));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_RCN_AUX_VERIF_STA);
        STTBX_Print(("DTP_RCN_AUX_VERIF_STA                 Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_RCN_AUX_VERIF_STA, value));
        STTBX_Print(("    Y_MBNB                                    : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_Y_MBNB) >> DTP_RCN_VERIF_STA_SHFT_Y_MBNB));
        STTBX_Print(("    C_MBNB                                    : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_C_MBNB) >> DTP_RCN_VERIF_STA_SHFT_C_MBNB));
        STTBX_Print(("    TR_PEND                                   : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_TR_PEND) >> DTP_RCN_VERIF_STA_SHFT_TR_PEND));
        STTBX_Print(("    END                                       : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_END) >> DTP_RCN_VERIF_STA_SHFT_END));
        STTBX_Print(("    PLUG_R_ERR                                : 0x%04x\n", (value & DTP_RCN_VERIF_STA_MASK_PLUG_R_ERR) >> DTP_RCN_VERIF_STA_SHFT_PLUG_R_ERR));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_PIPE_VERIF_STA);
        STTBX_Print(("DTP_PIPE_VERIF_STA                    Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_PIPE_VERIF_STA, value));
        STTBX_Print(("    SPSOUT_INVB_REQ                           : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_SPSOUT_INVB_REQ) >> DTP_PIPE_VERIF_STA_SHFT_SPSOUT_INVB_REQ));
        STTBX_Print(("    REQ_FROM_CTRL                             : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_REQ_FROM_CTRL) >> DTP_PIPE_VERIF_STA_SHFT_REQ_FROM_CTRL));
        STTBX_Print(("    SPSIN_IZZ_VAL                             : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_SPSIN_IZZ_VAL) >> DTP_PIPE_VERIF_STA_SHFT_SPSIN_IZZ_VAL));
        STTBX_Print(("    RES_READY                                 : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_RES_READY) >> DTP_PIPE_VERIF_STA_SHFT_RES_READY));
        STTBX_Print(("    TTCBF_READY                               : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_TTCBF_READY) >> DTP_PIPE_VERIF_STA_SHFT_TTCBF_READY));
        STTBX_Print(("    SPSIN                                     : 0x%04x (", (value & DTP_PIPE_VERIF_STA_MASK_SPSIN) >> DTP_PIPE_VERIF_STA_SHFT_SPSIN));
        switch(value & DTP_PIPE_VERIF_STA_MASK_SPSIN)
        {
            case DTP_PIPE_VERIF_STA_SPSIN_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_PCM :
                STTBX_Print(("H264_PCM)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_LUMA_DC :
                STTBX_Print(("H264_LUMA_DC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_LUMA_AC_4x4 :
                STTBX_Print(("H264_LUMA_AC_4x4)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_LUMA_AC_8x8 :
                STTBX_Print(("H264_LUMA_AC_8x8)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_CHROMA_DC :
                STTBX_Print(("H264_CHROMA_DC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_H264_CHROMA_AC :
                STTBX_Print(("H264_CHROMA_AC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_SPSIN_VC1_GET_TTCBF :
                STTBX_Print(("VC1_GET_TTCBF)\n"));
                break;
            case DTP_PIPE_VERIF_STA_VC1_DC_COEF :
                STTBX_Print(("VC1_DC_COEF)\n"));
                break;
            case DTP_PIPE_VERIF_STA_VC1_WAIT_ACDC_PRED :
                STTBX_Print(("VC1_WAIT_ACDC_PRED)\n"));
                break;
            case DTP_PIPE_VERIF_STA_VC1_AC_COEF :
                STTBX_Print(("VC1_AC_COEF)\n"));
                break;
            case DTP_PIPE_VERIF_STA_MPEG :
                STTBX_Print(("MPEG)\n"));
                break;
            case DTP_PIPE_VERIF_STA_ELSE :
                STTBX_Print(("ELSE)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    BLOCK_READY                               : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_BLOCK_READY) >> DTP_PIPE_VERIF_STA_SHFT_BLOCK_READY));
        STTBX_Print(("    REQ_FROM_H264                             : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_REQ_FROM_H264) >> DTP_PIPE_VERIF_STA_SHFT_REQ_FROM_H264));
        STTBX_Print(("    H264_VAL_TO_INV                           : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_H264_VAL_TO_INV) >> DTP_PIPE_VERIF_STA_SHFT_H264_VAL_TO_INV));
        STTBX_Print(("    H264_PROCESS                              : 0x%04x (", (value & DTP_PIPE_VERIF_STA_MASK_H264_PROCESS) >> DTP_PIPE_VERIF_STA_SHFT_H264_PROCESS));
        switch(value & DTP_PIPE_VERIF_STA_MASK_H264_PROCESS)
        {
            case DTP_PIPE_VERIF_STA_H264_PROCESS_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_LUMA_DC :
                STTBX_Print(("LUMA_DC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_INIT :
                STTBX_Print(("INIT)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_LUMA_AC_4x4 :
                STTBX_Print(("LUMA_AC_4x4)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_LUMA_AC_8x8 :
                STTBX_Print(("LUMA_AC_8x8)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_CHROMA_DC :
                STTBX_Print(("CHROMA_DC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_CHROMA_AC :
                STTBX_Print(("CHROMA_AC)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_LAST_IT_4x4 :
                STTBX_Print(("LAST_IT_4x4)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_PROCESS_LAST_IT_8x8 :
                STTBX_Print(("LAST_IT_8x8)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    H264_BLK_STATE                            : 0x%04x (", (value & DTP_PIPE_VERIF_STA_MASK_H264_BLK_STATE) >> DTP_PIPE_VERIF_STA_SHFT_H264_BLK_STATE));
        switch(value & DTP_PIPE_VERIF_STA_MASK_H264_BLK_STATE)
        {
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_OTHERS :
                STTBX_Print(("OTHERS)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_1 :
                STTBX_Print(("STATE_1)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_2 :
                STTBX_Print(("STATE_2)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_3 :
                STTBX_Print(("STATE_3)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_4 :
                STTBX_Print(("STATE_4)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_5 :
                STTBX_Print(("STATE_5)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_6 :
                STTBX_Print(("STATE_6)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_7 :
                STTBX_Print(("STATE_7)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_8 :
                STTBX_Print(("STATE_8)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_9 :
                STTBX_Print(("STATE_9)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_10 :
                STTBX_Print(("STATE_10)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_11 :
                STTBX_Print(("STATE_11)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_12 :
                STTBX_Print(("STATE_12)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_13 :
                STTBX_Print(("STATE_13)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_14 :
                STTBX_Print(("STATE_14)\n"));
                break;
            case DTP_PIPE_VERIF_STA_H264_BLK_STATE_STATE_15 :
                STTBX_Print(("STATE_15)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    H264_BLK_NUM                              : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_H264_BLK_NUM) >> DTP_PIPE_VERIF_STA_SHFT_H264_BLK_NUM));
        STTBX_Print(("    ZERO                                      : 0x%04x\n", (value & DTP_PIPE_VERIF_STA_MASK_ZERO) >> DTP_PIPE_VERIF_STA_SHFT_ZERO));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_DMCDF_VERIF_STA);
        STTBX_Print(("DTP_DMCDF_VERIF_STA                   Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_DMCDF_VERIF_STA, value));
        STTBX_Print(("    DF_PIC_MB_HPOS                            : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DF_PIC_MB_HPOS) >> DTP_DMCDF_VERIF_STA_SHFT_DF_PIC_MB_HPOS));
        STTBX_Print(("    DF_PIC_MB_VPOS                            : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DF_PIC_MB_VPOS) >> DTP_DMCDF_VERIF_STA_SHFT_DF_PIC_MB_VPOS));
        STTBX_Print(("    DF_PIC_FLT_DONE                           : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DF_PIC_FLT_DONE) >> DTP_DMCDF_VERIF_STA_SHFT_DF_PIC_FLT_DONE));
        STTBX_Print(("    DMC_ADDRCALC_CMD                          : 0x%04x (", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_ADDRCALC_CMD) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_ADDRCALC_CMD));
        switch(value & DTP_DMCDF_VERIF_STA_MASK_DMC_ADDRCALC_CMD)
        {
            case DTP_DMCDF_VERIF_STA_DMC_ADDRCALC_CMD_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_ADDRCALC_CMD_ABORT :
                STTBX_Print(("ABORT)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_ADDRCALC_CMD_INIT_DMA :
                STTBX_Print(("INIT_DMA)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_ADDRCALC_CMD_STATUS :
                STTBX_Print(("STATUS)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_ADDRCALC_CMD_INC_DMA :
                STTBX_Print(("INC_DMA)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    DMC_DMA_VAL                               : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_DMA_VAL) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_DMA_VAL));
        STTBX_Print(("    DMC_DMA_ACK                               : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_DMA_ACK) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_DMA_ACK));
        STTBX_Print(("    DMC_SDI1_CTRL                             : 0x%04x (", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_SDI1_CTRL) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_SDI1_CTRL));
        switch(value & DTP_DMCDF_VERIF_STA_MASK_DMC_SDI1_CTRL)
        {
            case DTP_DMCDF_VERIF_STA_DMC_SDI1_CTRL_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SDI1_CTRL_WAIT :
                STTBX_Print(("WAIT)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SDI1_CTRL_CKH_READ_OVER :
                STTBX_Print(("CKH_READ_OVER)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    DMC_SEND_SDI1                             : 0x%04x (", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_SEND_SDI1) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_SEND_SDI1));
        switch(value & DTP_DMCDF_VERIF_STA_MASK_DMC_SEND_SDI1)
        {
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_SEND_DUMMY_MSB :
                STTBX_Print(("SEND_DUMMY_MSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_SEND_DUMMY_LSB :
                STTBX_Print(("SEND_DUMMY_LSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_WT_ACK_DUMMY_MSB :
                STTBX_Print(("WT_ACK_DUMMY_MSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_CONT_SEND_DUMMY :
                STTBX_Print(("CONT_SEND_DUMMY)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_DISCARD_DATA :
                STTBX_Print(("DISCARD_DATA)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_SEND_MSB :
                STTBX_Print(("SEND_MSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_SEND_LSB :
                STTBX_Print(("SEND_LSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_WT_ACK_SEND_MSB :
                STTBX_Print(("WT_ACK_SEND_MSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_WT_ACK_SEND_LSB :
                STTBX_Print(("WT_ACK_SEND_LSB)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_CONT_PROC_VAL :
                STTBX_Print(("CONT_PROC_VAL)\n"));
                break;
            case DTP_DMCDF_VERIF_STA_DMC_SEND_SDI1_WT_AGAIN :
                STTBX_Print(("WT_AGAIN)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    DMC_PROC_VAL                              : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_PROC_VAL) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_PROC_VAL));
        STTBX_Print(("    DMC_PROC_ACK                              : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_PROC_ACK) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_PROC_ACK));
        STTBX_Print(("    DMC_SDI1_REQ                              : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_SDI1_REQ) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_SDI1_REQ));
        STTBX_Print(("    DMC_LX_SDI1_GNT                           : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_LX_SDI1_GNT) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_LX_SDI1_GNT));
        STTBX_Print(("    DMC_T3_REQ                                : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_T3_REQ) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_T3_REQ));
        STTBX_Print(("    DMC_T3_GNT                                : 0x%04x\n", (value & DTP_DMCDF_VERIF_STA_MASK_DMC_T3_GNT) >> DTP_DMCDF_VERIF_STA_SHFT_DMC_T3_GNT));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_DF_VERIF_STA);
        STTBX_Print(("DTP_DF_VERIF_STA                      Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_DF_VERIF_STA, value));
        STTBX_Print(("    EDGEFLT                                   : 0x%04x\n", (value & DTP_DF_VERIF_STA_MASK_EDGEFLT) >> DTP_DF_VERIF_STA_SHFT_EDGEFLT));
        STTBX_Print(("    INFOPARSER                                : 0x%04x\n", (value & DTP_DF_VERIF_STA_MASK_INFOPARSER) >> DTP_DF_VERIF_STA_SHFT_INFOPARSER));
        STTBX_Print(("    DISABLE_IDC                               : 0x%04x\n", (value & DTP_DF_VERIF_STA_MASK_DISABLE_IDC) >> DTP_DF_VERIF_STA_SHFT_DISABLE_IDC));
        STTBX_Print(("    MB_YUV_LOAD                               : 0x%04x\n", (value & DTP_DF_VERIF_STA_MASK_MB_YUV_LOAD) >> DTP_DF_VERIF_STA_SHFT_MB_YUV_LOAD));
        STTBX_Print(("    MB_INFO_LOAD                              : 0x%04x\n", (value & DTP_DF_VERIF_STA_MASK_MB_INFO_LOAD) >> DTP_DF_VERIF_STA_SHFT_MB_INFO_LOAD));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_BHIMVP_VERIF_STA);
        STTBX_Print(("DTP_BHIMVP_VERIF_STA                  Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_BHIMVP_VERIF_STA, value));
        STTBX_Print(("    IMVP_CTRL                                 : 0x%04x (", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_CTRL) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_CTRL));
        switch(value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_CTRL)
        {
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_START :
                STTBX_Print(("START)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_READ_1234 :
                STTBX_Print(("READ_1234)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_READ_5 :
                STTBX_Print(("READ_5)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_COMPUTE :
                STTBX_Print(("COMPUTE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_WAIT_SHIFT :
                STTBX_Print(("WAIT_SHIFT)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_CTRL_WAIT :
                STTBX_Print(("WAIT)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    IMVP_COMPUTE                              : 0x%04x (", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_COMPUTE) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_COMPUTE));
        switch(value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_COMPUTE)
        {
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_STATE_1 :
                STTBX_Print(("STATE_1)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_STATE_2 :
                STTBX_Print(("STATE_2)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_STATE_3 :
                STTBX_Print(("STATE_3)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_STATE_4 :
                STTBX_Print(("STATE_4)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_DIRECT_0 :
                STTBX_Print(("DIRECT_0)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_DIRECT_1 :
                STTBX_Print(("DIRECT_1)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_IMVP_COMPUTE_DIRECT_1BIS :
                STTBX_Print(("DIRECT_1BIS)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    IMVP_LX_SDI0_GNT                          : 0x%04x\n", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_LX_SDI0_GNT) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_LX_SDI0_GNT));
        STTBX_Print(("    IMVP_LX_SDI2_REQ                          : 0x%04x\n", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_LX_SDI2_REQ) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_LX_SDI2_REQ));
        STTBX_Print(("    IMVP_IMVP_SDI_LOCK                        : 0x%04x\n", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_IMVP_SDI_LOCK) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_IMVP_SDI_LOCK));
        STTBX_Print(("    IMVP_BH_SDI_LOCK                          : 0x%04x\n", (value & DTP_BHIMVP_VERIF_STA_MASK_IMVP_BH_SDI_LOCK) >> DTP_BHIMVP_VERIF_STA_SHFT_IMVP_BH_SDI_LOCK));
        STTBX_Print(("    BH_PROCESS                                : 0x%04x (", (value & DTP_BHIMVP_VERIF_STA_MASK_BH_PROCESS) >> DTP_BHIMVP_VERIF_STA_SHFT_BH_PROCESS));
        switch(value & DTP_BHIMVP_VERIF_STA_MASK_BH_PROCESS)
        {
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_WT_OPCODE_TOGGLE :
                STTBX_Print(("WT_OPCODE_TOGGLE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_LAUNCH_OPCODE :
                STTBX_Print(("LAUNCH_OPCODE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_WT_SHOW_DATA :
                STTBX_Print(("WT_SHOW_DATA)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_MOVE :
                STTBX_Print(("MOVE)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETBIT1 :
                STTBX_Print(("GETBIT1)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETBIT2 :
                STTBX_Print(("GETBIT2)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETGOLOMB :
                STTBX_Print(("GETGOLOMB)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_1CYCLE_UNSIGNED :
                STTBX_Print(("1CYCLE_UNSIGNED)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETGOLOMB_2CYCLES_2 :
                STTBX_Print(("GETGOLOMB_2CYCLES_2)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETGOLOMB_2CYCLES_3 :
                STTBX_Print(("GETGOLOMB_2CYCLES_3)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETUEG3_1 :
                STTBX_Print(("GETUEG3_1)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETUEG3_2 :
                STTBX_Print(("GETUEG3_2)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETUEG3_2_2CYCLES :
                STTBX_Print(("GETUEG3_2_2CYCLES)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_GETVC1 :
                STTBX_Print(("GETVC1)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_RED_PROCESS :
                STTBX_Print(("RED_PROCESS)\n"));
                break;
            case DTP_BHIMVP_VERIF_STA_BH_PROCESS_RUN_GET_STATUS :
                STTBX_Print(("RUN_GET_STATUS)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    BH_DATA_VAL                                 : 0x%04x\n", (value & DTP_BHIMVP_VERIF_STA_MASK_BH_DATA_VAL) >> DTP_BHIMVP_VERIF_STA_SHFT_BH_DATA_VAL));

        value = ReadReg32 (DTP_REGVRF_BASE_ADDRESS + DTP_RED_VERIF_STA);
        STTBX_Print(("DTP_RED_VERIF_STA                     Addr 0x%08x, Value 0x%08x\n", DTP_REGVRF_BASE_ADDRESS + DTP_RED_VERIF_STA, value));
        STTBX_Print(("    PCM_PROCESS                               : 0x%04x (", (value & DTP_RED_VERIF_STA_MASK_PCM_PROCESS) >> DTP_RED_VERIF_STA_SHFT_PCM_PROCESS));
        switch(value & DTP_RED_VERIF_STA_MASK_PCM_PROCESS)
        {
            case DTP_RED_VERIF_STA_PCM_PROCESS_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_RED_VERIF_STA_PCM_PROCESS_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_RED_VERIF_STA_PCM_PROCESS_TRANSMIT :
                STTBX_Print(("TRANSMIT)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    CAVLC_PROCESS                             : 0x%04x (", (value & DTP_RED_VERIF_STA_MASK_CAVLC_PROCESS) >> DTP_RED_VERIF_STA_SHFT_CAVLC_PROCESS));
        switch(value & DTP_RED_VERIF_STA_MASK_CAVLC_PROCESS)
        {
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_WT_DEC_LEVEL_POS :
                STTBX_Print(("WT_DEC_LEVEL_POS)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEVEL_POS_AND_T1 :
                STTBX_Print(("LEVEL_POS_AND_T1)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_BOTH_0_LEVEL :
                STTBX_Print(("BOTH_0_LEVEL)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_DEC_TRAIL_ONE :
                STTBX_Print(("DEC_TRAIL_ONE)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_DEC_T1_EQ_2 :
                STTBX_Print(("DEC_T1_EQ_2)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_DEC_T1_EQ_3 :
                STTBX_Print(("DEC_T1_EQ_3)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE0_0 :
                STTBX_Print(("LEV_DEC_STATE0_0)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE0_1 :
                STTBX_Print(("LEV_DEC_STATE0_1)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE1_0 :
                STTBX_Print(("LEV_DEC_STATE1_0)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE1 :
                STTBX_Print(("LEV_DEC_STATE1)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE2 :
                STTBX_Print(("LEV_DEC_STATE2)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE3 :
                STTBX_Print(("LEV_DEC_STATE3)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE4 :
                STTBX_Print(("LEV_DEC_STATE4)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE5 :
                STTBX_Print(("LEV_DEC_STATE5)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_LEV_DEC_STATE6 :
                STTBX_Print(("LEV_DEC_STATE6)\n"));
                break;
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_2CYCLE_LEVEL :
                STTBX_Print(("2CYCLE_LEVEL)\n"));
                break;
/* Same value as DTP_RED_VERIF_STA_CAVLC_PROCESS_2CYCLE_LEVEL
            case DTP_RED_VERIF_STA_CAVLC_PROCESS_ERROR_DETECTED :
                STTBX_Print(("ERROR_DETECTED)\n"));
                break;
*/
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    CABAC_PROCESS2                            : 0x%04x (", (value & DTP_RED_VERIF_STA_MASK_CABAC_PROCESS2) >> DTP_RED_VERIF_STA_SHFT_CABAC_PROCESS2));
        switch(value & DTP_RED_VERIF_STA_MASK_CABAC_PROCESS2)
        {
            case DTP_RED_VERIF_STA_CABAC_PROCESS2_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS2_LEVEL_DEC_START :
                STTBX_Print(("LEVEL_DEC_START)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS2_LEVEL_DEC1 :
                STTBX_Print(("LEVEL_DEC1)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS2_LEVEL_DEC2 :
                STTBX_Print(("LEVEL_DEC2)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS2_ERROR_DETECTED :
                STTBX_Print(("ERROR_DETECTED)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    CABAC_PROCESS1                            : 0x%04x (", (value & DTP_RED_VERIF_STA_MASK_CABAC_PROCESS1) >> DTP_RED_VERIF_STA_SHFT_CABAC_PROCESS1));
        switch(value & DTP_RED_VERIF_STA_MASK_CABAC_PROCESS1)
        {
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_LEVEL_POS1 :
                STTBX_Print(("LEVEL_POS1)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_LEVEL_POS2 :
                STTBX_Print(("LEVEL_POS2)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_1CYCLE_BEF_TRANS :
                STTBX_Print(("1CYCLE_BEF_TRANS)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_NOTHING_TO_TRANS :
                STTBX_Print(("NOTHING_TO_TRANS)\n"));
                break;
            case DTP_RED_VERIF_STA_CABAC_PROCESS1_WT_END_OF_BLK :
                STTBX_Print(("WT_END_OF_BLK)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
        STTBX_Print(("    VC1_PROCESS                               : 0x%04x (", (value & DTP_RED_VERIF_STA_MASK_VC1_PROCESS) >> DTP_RED_VERIF_STA_SHFT_VC1_PROCESS));
        switch(value & DTP_RED_VERIF_STA_MASK_VC1_PROCESS)
        {
            case DTP_RED_VERIF_STA_VC1_PROCESS_RESET :
                STTBX_Print(("RESET)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_IDLE :
                STTBX_Print(("IDLE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_TTMB_DECODE :
                STTBX_Print(("TTMB_DECODE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_TTBLK_DECODE :
                STTBX_Print(("TTBLK_DECODE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_SUBBLKPAT_4X4 :
                STTBX_Print(("SUBBLKPAT_4X4)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_SUBBLKPAT_8X4_OR_4X8 :
                STTBX_Print(("SUBBLKPAT_8X4_OR_4X8)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_DC_DIFFERENTIAL :
                STTBX_Print(("DC_DIFFERENTIAL)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_DC_ESCAPEMODE :
                STTBX_Print(("DC_ESCAPEMODE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_COEFF1 :
                STTBX_Print(("AC_COEFF1)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_COEFF1_TWO_CYCLE :
                STTBX_Print(("AC_COEFF1_TWO_CYCLE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_COEFF2_MODE1_OR_2 :
                STTBX_Print(("AC_COEFF2_MODE1_OR_2)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_COEFF2_MODE1_OR_2_TWO_CYCLE :
                STTBX_Print(("AC_COEFF2_MODE1_OR_2_TWO_CYCLE)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_ADD_DELTA_MODE1_OR_2 :
                STTBX_Print(("ADD_DELTA_MODE1_OR_2)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_FIRST_MODE3 :
                STTBX_Print(("AC_FIRST_MODE3)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_FIRST_MODE3_BIS :
                STTBX_Print(("AC_FIRST_MODE3_BIS)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_AC_MODE3 :
                STTBX_Print(("AC_MODE3)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_SAVE_BH_DATA :
                STTBX_Print(("SAVE_BH_DATA)\n"));
                break;
            case DTP_RED_VERIF_STA_VC1_PROCESS_ERROR_TABLE :
                STTBX_Print(("ERROR_TABLE)\n"));
                break;
            default :
                STTBX_Print(("!! UNKNOWN !!)\n"));
                break;
        }
#endif /* defined(ST_DELTAMU_HE) || (defined(ST_7100) && !defined(ST_CUT_1_X)) */

        switch (ErrCode)
        {
            case ST_NO_ERROR :
                RetErr = FALSE;
                strcat(VID_Msg, "ok\n" );

                break;
            case ST_ERROR_INVALID_HANDLE:
                API_ErrorCount++;
                strcat(VID_Msg, "handle is invalid !\n" );
                break;
            default:
                API_ErrorCount++;
                sprintf(VID_Msg, "%sunexpected error [%X] !\n", VID_Msg, ErrCode );
                break;
        } /* end switch */
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Print((VID_Msg ));
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end Viddec_dump_mbe() */

static void PP_TEST_preprocessor_Start(DECODER_Properties_t * DECODER_Data_p, int PreprocessorId)
{
    void    *PreprocBaseAddress;
    U32     Reg_Value;

    PreprocBaseAddress = (void*)((U32)DECODER_Data_p->RegistersBaseAddress_p +
    (U32)(PP_OFFSET * PreprocessorId));

    /* Clear the whole status register */
    Reg_Value = MASK____PP_ITS__dma_cmp |
                MASK____PP_ITS__srs_cmp |
                MASK____PP_ITS__error_sc_detected |
                MASK____PP_ITS__error_bit_inserted |
                MASK____PP_ITS__int_buffer_overflow |
                MASK____PP_ITS__bit_buffer_underflow |
                MASK____PP_ITS__bit_buffer_overflow |
                MASK____PP_ITM__read_error |
                MASK____PP_ITM__write_error;
    PP_Write32((U32)PreprocBaseAddress + (U32)PP_ITS, Reg_Value);

    /* start the preprocessing */
    PP_Write32((U32)PreprocBaseAddress + (U32)PP_START, MASK____PP_START__pp_start);
}

static ST_ErrorCode_t PP_TEST_preprocessor_Configuration(DECODER_Properties_t * DECODER_Data_p, int PreprocessorId)
{
    void    *PreprocBaseAddress;

    PreprocBaseAddress = (void*)((U32)DECODER_Data_p->RegistersBaseAddress_p +
    ((U32)PP_OFFSET * PreprocessorId));

#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
    memcpy((U32 *)EMUL_PP_TEST_Data, PP_TEST_Data, sizeof(PP_TEST_Data));
    memcpy((U32 *)EMUL_PP_TEST_CPUIntSliceErrStatusBufferBegin_p, PP_TEST_CPUIntSliceErrStatusBufferBegin_p, sizeof(PP_TEST_CPUIntSliceErrStatusBufferBegin_p));
    memcpy((U32 *)EMUL_PP_TEST_CPUIntPictureBufferBegin_p, PP_TEST_CPUIntPictureBufferBegin_p, sizeof(PP_TEST_CPUIntPictureBufferBegin_p));
#else
    memcpy((U32 *)SI_PP_TEST_Data, PP_TEST_Data, sizeof(PP_TEST_Data));
    memcpy((U32 *)SI_PP_TEST_CPUIntSliceErrStatusBufferBegin_p, PP_TEST_CPUIntSliceErrStatusBufferBegin_p, sizeof(PP_TEST_CPUIntSliceErrStatusBufferBegin_p));
    memcpy((U32 *)SI_PP_TEST_CPUIntPictureBufferBegin_p, PP_TEST_CPUIntPictureBufferBegin_p, sizeof(PP_TEST_CPUIntPictureBufferBegin_p));
#endif

    /* PREPROCESSOR CONFIGURATION */
    PP_Write32((U32)PreprocBaseAddress + PP_BBG,         (U32)PP_TEST_PicStart);
    PP_Write32((U32)PreprocBaseAddress + PP_BBS,         (U32)PP_TEST_PicStop);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_START,  (U32)PP_TEST_PicStart);
    PP_Write32((U32)PreprocBaseAddress + PP_READ_STOP,   (U32)PP_TEST_PicStop);

    PP_Write32((U32)PreprocBaseAddress + PP_ISBG,        (U32)PP_TEST_IntSliceErrStatusBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IPBG,        (U32)PP_TEST_IntPictureBufferBegin_p);
    PP_Write32((U32)PreprocBaseAddress + PP_IBS,         (U32)PP_TEST_IntBufferStop_p);

    PP_Write32((U32)PreprocBaseAddress + PP_CFG,         PP_TEST_CFG);
    PP_Write32((U32)PreprocBaseAddress + PP_PICWIDTH,    PP_TEST_PICWIDTH);
    PP_Write32((U32)PreprocBaseAddress + PP_CODELENGTH,  PP_TEST_CODELENGTH);

    pp_itm_ori[PreprocessorId] = PP_Read32((U32)PreprocBaseAddress + PP_ITM); /* Save original value */
    PP_Write32((U32)PreprocBaseAddress + PP_ITM,  0);

    return ST_NO_ERROR;
}

static ST_ErrorCode_t  PP_TEST_preprocessor_SoftReset(DECODER_Properties_t * DECODER_Data_p, int PreprocessorId)
{
    void    *PreprocBaseAddress;

    PreprocBaseAddress = (void*) ((U32)DECODER_Data_p->RegistersBaseAddress_p +
    ((U32)PP_OFFSET * PreprocessorId));

    /* Launch the preprocessor soft reset */
    PP_Write32((U32)PreprocBaseAddress + (U32)PP_SRS, MASK____PP_SRS__soft_reset);

    STOS_TaskDelayUs(6000000);  /* 6s */
    if ((PP_Read32((U32)PreprocBaseAddress + (U32)PP_ITS) & MASK____PP_ITS__srs_cmp) == 0)
    {
      STTBX_Print(("PP%d Soft Reset time out\n", PreprocessorId));
      return ST_ERROR_INVALID_HANDLE;
    }
    else
    {
      STTBX_Print(("PP%d Soft Reset done\n", PreprocessorId));
    }

    return ST_NO_ERROR;
}

static ST_ErrorCode_t  PP_TEST_preprocessor_end(DECODER_Properties_t * DECODER_Data_p, int PreprocessorId)
{
    void    *PreprocBaseAddress;

    PreprocBaseAddress = (void*)((U32)DECODER_Data_p->RegistersBaseAddress_p +
    ((U32)PP_OFFSET * PreprocessorId));

    /*  PREPROCESSOR END */
    STOS_TaskDelayUs(6000000);  /* 6s */
    if (PP_Read32((U32)PreprocBaseAddress + (U32)PP_ITS) != MASK____PP_ITS__dma_cmp)
    {
        STTBX_Print(("PP%d DMA returned errors 0x%x\n", PreprocessorId, PP_Read32((U32)PreprocBaseAddress + PP_ITS)));
        return ST_ERROR_INVALID_HANDLE;
    }
    else
    {
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
        if (memcmp((U32 *)PP_TEST_EXPECTED_CPUIntSliceErrStatusBufferBegin_p, (U32 *)EMUL_PP_TEST_CPUIntSliceErrStatusBufferBegin_p, sizeof(PP_TEST_EXPECTED_CPUIntSliceErrStatusBufferBegin_p)))
#else
        if (memcmp(PP_TEST_EXPECTED_CPUIntSliceErrStatusBufferBegin_p, (U32 *)SI_PP_TEST_CPUIntSliceErrStatusBufferBegin_p, sizeof(PP_TEST_EXPECTED_CPUIntSliceErrStatusBufferBegin_p)))
#endif
        {
            STTBX_Print(("PP%d SESB mismatch\n", PreprocessorId));
            return ST_ERROR_INVALID_HANDLE;
        }
#if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
        if (memcmp((U32 *)PP_TEST_EXPECTED_CPUIntPictureBufferBegin_p, (U32 *)EMUL_PP_TEST_CPUIntPictureBufferBegin_p, sizeof(PP_TEST_EXPECTED_CPUIntPictureBufferBegin_p)))
#else
        if (memcmp(PP_TEST_EXPECTED_CPUIntPictureBufferBegin_p, (U32 *)SI_PP_TEST_CPUIntPictureBufferBegin_p, sizeof(PP_TEST_EXPECTED_CPUIntPictureBufferBegin_p)))
#endif
        {
            STTBX_Print(("PP%d Intermediate Buffer mismatch\n", PreprocessorId));
            return ST_ERROR_INVALID_HANDLE;
        }
        STTBX_Print(("PP%d DMA done\n", PreprocessorId));
    }

    return ST_NO_ERROR;
}

static ST_ErrorCode_t  PP_TEST_RestoreConfiguration(DECODER_Properties_t * DECODER_Data_p, int PreprocessorId)
{
    void    *PreprocBaseAddress;

    PreprocBaseAddress = (void*)((U32)DECODER_Data_p->RegistersBaseAddress_p +
    ((U32)PP_OFFSET * PreprocessorId));

    PP_Write32((U32)PreprocBaseAddress + (U32)PP_ITM, pp_itm_ori[PreprocessorId]);
#if defined(ST_7100)
    *system_config29_reg = delta_rst_ori;
#endif

    return ST_NO_ERROR;
}

static BOOL Viddec_pp_test(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    S32 LVar;
    BOOL HandleFound;
    VIDPROD_Data_t              *VIDPROD_Data_p;
    DECODER_Handle_t            DecoderHandle;
    H264DecoderPrivateData_t    *PrivateData_p;
    DECODER_Properties_t *       DECODER_Data_p;
    int PreprocessorId;

    ErrCode = ST_NO_ERROR;
    RetErr = FALSE;

    /* get Handle */
    CurrentHandle = VID_GetSTVIDHandleFromNumber(0);
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)CurrentHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }
    VIDPROD_Data_p = (VIDPROD_Data_t *)(((VIDPROD_Properties_t *)(((VideoUnit_t *)(CurrentHandle))->Device_p->DeviceData_p->ProducerHandle))->InternalProducerHandle);
    DECODER_Data_p  = (DECODER_Properties_t *)VIDPROD_Data_p->DecoderHandle;
    PrivateData_p   = (H264DecoderPrivateData_t *) DECODER_Data_p->PrivateData_p;

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        if (LVar == 1)
        {
          PreprocessorId = 0;
        }
        else
        {
          PreprocessorId = 1;
        }

        /* Disable reset of delta, if not already done */
        #if defined(ST_7100)
        delta_rst_ori = * system_config29_reg;
        * system_config29_reg = delta_rst_ori & 0xfffffffe;
        #endif
        #if defined(ST_DELTAPHI_HE) || defined(ST_DELTAMU_HE)
        * rst_delta_n =1;
        #endif

        ErrCode = PP_TEST_preprocessor_Configuration(DECODER_Data_p, PreprocessorId);
        if (ErrCode != ST_NO_ERROR)
        {
          RetErr = TRUE;
        }
        else
        {
          ErrCode = PP_TEST_preprocessor_SoftReset(DECODER_Data_p, PreprocessorId);
          if (ErrCode != ST_NO_ERROR)
          {
            RetErr = TRUE;
          }
          else
          {
            PP_TEST_preprocessor_Start(DECODER_Data_p, PreprocessorId);
            ErrCode = PP_TEST_preprocessor_end(DECODER_Data_p, PreprocessorId);
            if (ErrCode != ST_NO_ERROR)
            {
              RetErr = TRUE;
            }
          }
        }
        PP_TEST_RestoreConfiguration(DECODER_Data_p, PreprocessorId);
    }
    return (!RetErr);
} /* end Viddec_pp_test() */

/*-------------------------------------------------------------------------
 * Function : Viddec_LX_erc
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Viddec_LX_erc(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t              ErrCode;
    BOOL                        RetErr;
    STVID_Handle_t              CurrentHandle;
    S32                         LVar;
    BOOL                        HandleFound;
    VIDPROD_Data_t             *VIDPROD_Data_p;
    BOOL                        Enable;

    ErrCode = ST_NO_ERROR;
    RetErr = FALSE;

    /* get Handle */
    CurrentHandle = VID_GetSTVIDHandleFromNumber(0);
    if (API_TestMode == TEST_MODE_MULTI_HANDLES)
    {
        RetErr = STTST_GetInteger(pars_p, (int)CurrentHandle, &LVar );
        CurrentHandle = (STVID_Handle_t) LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Handle" );
        }
    }

    RetErr = STTST_GetInteger(pars_p, (int)TRUE, &LVar );
    Enable = (BOOL) LVar;
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "expected TRUE|FALSE" );
    }

    VIDPROD_Data_p = (VIDPROD_Data_t *)(((VIDPROD_Properties_t *)(((VideoUnit_t *)(CurrentHandle))->Device_p->DeviceData_p->ProducerHandle))->InternalProducerHandle);

    if(Enable)
    {
        H264dec_DecodingMode = H264_NORMAL_DECODE;
    }
    else
    {
        H264dec_DecodingMode = H264_NORMAL_DECODE_WITHOUT_ERROR_RECOVERY;
    }

    STTBX_Print(("VVIDPROD_LX_Erc(handle=%d) : LX ERC set to %s\n", CurrentHandle, Enable ? "TRUE" : "FALSE"));

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return (API_EnableError ? RetErr : FALSE );

} /* end Viddec_LX_erc() */
/*-------------------------------------------------------------------------
 * Function : VIDH264Decode_RegisterCmd
 *            Register video command (1 for each API function)
 * Input    :
 * Output   :
 * Return   : TRUE if success
 * ----------------------------------------------------------------------*/
BOOL VIDH264Decode_RegisterCmd (void)
{
    BOOL RetErr;
    U32  i;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("DEC_DUMP_MME",     Viddec_dump_mme,                 "[<Handle>] Prints last MME commands");
    RetErr |= STTST_RegisterCommand("DEC_DUMP_PP",      Viddec_dump_pp,                  "[<Handle>] Prints PreProcessors registers");
    RetErr |= STTST_RegisterCommand("DEC_DUMP_MBE",     Viddec_dump_mbe,                 "[<Handle>] Prints Macroblock Engine registers");
    RetErr |= STTST_RegisterCommand("PP_TEST",          Viddec_pp_test,                  "[<Handle>] <pp_num> test preprocessor <pp_num>");
    RetErr |= STTST_RegisterCommand("DEC_LX_ERC",       Viddec_LX_erc,                   "[<Handle>] [TRUE|FALSE] Enable|Disable Error Concealment in H264 firmware (enabled at startup)");

    if (RetErr)
    {
        STTST_Print(("VIDH264Decode_RegisterCmd()     : failed !\n" ));
    }
    else
    {
        STTST_Print(("VIDH264Decode_RegisterCmd()     : ok\n" ));
    }
    return(RetErr ? FALSE : TRUE);

} /* end VIDH264Decode_RegisterCmd */
