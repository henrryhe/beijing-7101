/*
**  @file     : delta_pp_registers.h
**
**  @brief    : Defines the DeltaPhi/DeltaMu Preprocessors registers
**
**  @par OWNER: Vincent Capony
**
**  @author   : Vincent Capony
**
**  @par SCOPE:
**
**  @date     : 2003-12-10
**
**  &copy; 2003 ST Microelectronics. All Rights Reserved.
*/

#ifndef __DELTA_PP_REGISTERS_H
#define __DELTA_PP_REGISTERS_H

#include "stos.h"
#include "stsys.h"

#define PP_OFFSET       0x80

/*
** Preprocessor registers offsets
*/
#define PP_READ_START       (0x000)  /* DMA read start address                              */
#define PP_READ_STOP        (0x004)  /* DMA read stop address                               */
#define PP_BBG              (0x008)  /* Bit Buffer Begin                                    */
#define PP_BBS              (0x00C)  /* Bit Buffer Stop                                     */
#define PP_ISBG             (0x010)  /* Intermediate Slice Error Status Buffer Begin        */
#define PP_IPBG             (0x014)  /* Intermediate Picture Buffer Begin                   */
#define PP_IBS              (0x018)  /* Intermediate Buffer Stop                            */
#define PP_WDL              (0x01C)  /* Write DMA Last address (in the Intermediate Buffer) */
#define PP_CFG              (0x020)  /* Preprocessor configuration flags                    */
#define PP_PICWIDTH         (0x024)  /* Picture width in MBs unit                           */
#define PP_CODELENGTH       (0x028)  /* Number of bits used for u(v) coding                 */
#define PP_START            (0x02C)  /* Start preprocessor                                  */
#define PP_MAX_OPC_SIZE     (0x030)  /* Max opcode size of STBus plugs                      */
#define PP_MAX_CHUNK_SIZE   (0x034)  /* Max chunk size of STBus plugs                       */
#define PP_MAX_MESSAGE_SIZE (0x038)  /* Max message size of STBus plugs                     */
#define PP_ITS              (0x03C)  /* Interrupt register                                  */
#define PP_ITM              (0x040)  /* Interrupt Mask                                      */
#define PP_SRS              (0x044)  /* Soft reset                                          */

/*
** Preprocessors registers masks
*/
#define MASK____PP_READ_START                             0xFFFFFFFF
#define MASK____PP_READ_STOP                              0xFFFFFFFF
#define MASK____PP_BBG                                    0xFFFFFFF8
#define MASK____PP_BBS                                    0xFFFFFFF8
#define MASK____PP_ISBG                                   0xFFFFFFF8
#define MASK____PP_IPBG                                   0xFFFFFFF8
#define MASK____PP_IBS                                    0xFFFFFFF8
#define MASK____PP_WDL                                    0xFFFFFFFF

/*
**  PP_CFG register masks
*/
#define MASK____PP_CFG__mb_adaptive_frame_field_flag             0x00000001  /* mb_adaptive_frame_field_flag           */
#define MASK____PP_CFG__entropy_coding_mode_flag                 0x00000002  /* entropy_coding_mode_flag               */
#define MASK____PP_CFG__frame_mbs_only_flag                      0x00000004  /* frame_mbs_only_flag                    */
#define MASK____PP_CFG__pic_order_cnt_type                       0x00000018  /* pic_order_cnt_type                     */
#define MASK____PP_CFG__pic_order_present_flag                   0x00000020  /* pic_order_present_flag                 */
#define MASK____PP_CFG__delta_pic_order_always_zero_flag         0x00000040  /* delta_pic_order_always_zero_flag       */
#define MASK____PP_CFG__reserved_1                               0x00000080  /* reserved                               */
#define MASK____PP_CFG__weighted_pred_flag                       0x00000100  /* weighted_pred_flag                     */
#define MASK____PP_CFG__weighted_bipred_idc_flag                 0x00000200  /* weighted_bipred_idc_flag               */
#define MASK____PP_CFG__deblocking_filter_control_present_flag   0x00000400  /* deblocking_filter_control_present_flag */
#define MASK____PP_CFG__num_ref_idx_l0_active_minus1             0x0000F800  /* num_ref_idx_l0_active_minus1           */
#define MASK____PP_CFG__num_ref_idx_l1_active_minus1             0x001F0000  /* num_ref_idx_l1_active_minus1           */
#define MASK____PP_CFG__pic_init_qp                              0x07E00000  /* pic_init_qp                            */
#define MASK____PP_CFG__control_mode                             0x30000000  /* control_mode                           */
#define MASK____PP_CFG__direct_8x8_inference_flag                0x40000000  /* direct_8x8_inference_flag                               */
#define MASK____PP_CFG__transform_8x8_mode_flag                  0x08000000  /* transform_8x8_mode_flag                               */
#define MASK____PP_CFG__monochrome                               0x80000000  /* monochrome                               */
/*
**  PP_PICWIDTH register masks
*/
#define MASK____PP_PICWIDTH__picture_width                       0x0000007F   /* picture_width                  */
#define MASK____PP_PICWIDTH__reserved_1                          0x0000FF80   /* reserved                       */
#define MASK____PP_PICWIDTH__nb_MB_in_picture_minus1             0x1FFF0000   /* nb_MB_in_picture_minus1 (12:0) */
#define MASK____PP_PICWIDTH__reserved_2                          0xE0000000   /* reserved                       */

/*
**  PP_CODELENGTH  register masks
*/
#define MASK____PP_CODELENGTH__log2_max_frame_num                0x0000001F  /*  log2_max_frame_num         */
#define MASK____PP_CODELENGTH__log2_max_pic_order_cnt_lsb        0x000003E0  /*  log2_max_pic_order_cnt_lsb */
#define MASK____PP_CODELENGTH__reserved_1                        0xFFFFFC00  /*  reserved                   */

/*
**   PP_START register masks
*/
#define MASK____PP_START__pp_start                               0x00000001  /* pp_start */

/*
**   PP_CHUNK register masks
*/
#define MASK____PP_MAX_OPC_SIZE__max_opc_size                     0x00000007  /* max_opc_size     */
#define MASK____PP_MAX_CHUNK_SIZE__max_chunk_size                 0x00000007  /* max_chunk_size   */
#define MASK____PP_MAX_MESSAGE_SIZE__max_message_size             0x00000007  /* max_message_size */


/*
**   PP_ITS register masks
*/
#define MASK____PP_ITS__dma_cmp                                  0x00000001  /* Write DMA is completed, i.e preprocessor has complely finished its operation (read, parse, decode, write */
#define MASK____PP_ITS__srs_cmp                                  0x00000002  /* Soft reset is completed            */
#define MASK____PP_ITS__error_sc_detected                        0x00000004  /* Error Start Code has been detected */
#define MASK____PP_ITS__error_bit_inserted                       0x00000008  /* Error bit has been inserted in Slice Error Status Buffer */
#define MASK____PP_ITS__int_buffer_overflow                      0x00000010  /* Write address for intermediate buffer reached PP_IBS */
#define MASK____PP_ITS__bit_buffer_underflow                     0x00000020  /* Read stop address reached (PP_BBS), picture decoding not finished */
#define MASK____PP_ITS__bit_buffer_overflow                      0x00000040
#define MASK____PP_ITS__read_error                               0x00000080  /* error has been detected when reading the compressed data from external memory. */
#define MASK____PP_ITS__write_error                              0x00000100  /* error has been detected when writing the pre-processing result in external memory.*/

/*
**   PP_ITM register masks
*/
#define MASK____PP_ITM__dma_cmp                                  0x00000001  /* Write DMA is completed, i.e preprocessor has complely finished its operation (read, parse, decode, write */
#define MASK____PP_ITM__srs_cmp                                  0x00000002  /* Soft reset is completed            */
#define MASK____PP_ITM__error_sc_detected                        0x00000004  /* Error Start Code has been detected */
#define MASK____PP_ITM__error_bit_inserted                       0x00000008  /* Error bit has been inserted in Slice Error Status Buffer */
#define MASK____PP_ITM__int_buffer_overflow                      0x00000010  /* Write address for intermediate buffer reached PP_IBS */
#define MASK____PP_ITM__bit_buffer_underflow                     0x00000020  /* Read stop address reached (PP_BBS), picture decoding not finished */
#define MASK____PP_ITM__bit_buffer_overflow                      0x00000040
#define MASK____PP_ITM__read_error                               0x00000080  /* error has been detected when reading the compressed data from external memory. */
#define MASK____PP_ITM__write_error                              0x00000100  /* error has been detected when writing the pre-processing result in external memory.*/

/*
**   PP_SRS register masks
*/
#define MASK____PP_SRS__soft_reset                               0x00000001  /* soft reset */

/* BITS SHIFT VALUES FOR THE PREPROCESSOR REGISTERS */

/*
** Preprocessors registers masks
*/
#define SHIFT___PP_READ_START                             0
#define SHIFT___PP_READ_STOP                              0
#define SHIFT___PP_BBG                                    0
#define SHIFT___PP_BBS                                    0
#define SHIFT___PP_ISBG                                   0
#define SHIFT___PP_IPBG                                   0
#define SHIFT___PP_IBS                                    0
#define SHIFT___PP_WDL                                    0

/*
**  PP_CFG register SHIFTs
*/
#define SHIFT___PP_CFG__mb_adaptive_frame_field_flag                 0  /* mb_adaptive_frame_field_flag           */
#define SHIFT___PP_CFG__entropy_coding_mode_flag                     1  /* entropy_coding_mode_flag               */
#define SHIFT___PP_CFG__frame_mbs_only_flag                          2  /* frame_mbs_only_flag                    */
#define SHIFT___PP_CFG__pic_order_cnt_type                           3  /* pic_order_cnt_type                     */
#define SHIFT___PP_CFG__pic_order_present_flag                       5  /* pic_order_present_flag                 */
#define SHIFT___PP_CFG__delta_pic_order_always_zero_flag            6   /* delta_pic_order_always_zero_flag      */
#define SHIFT___PP_CFG__reserved_1                                   7  /* reserved                               */
#define SHIFT___PP_CFG__weighted_pred_flag                           8  /* weighted_pred_flag                     */
#define SHIFT___PP_CFG__weighted_bipred_idc_flag                     9  /* weighted_bipred_idc_flag               */
#define SHIFT___PP_CFG__deblocking_filter_control_present_flag      10  /* deblocking_filter_control_present_flag */
#define SHIFT___PP_CFG__num_ref_idx_l0_active_minus1                11  /* num_ref_idx_l0_active_minus1           */
#define SHIFT___PP_CFG__num_ref_idx_l1_active_minus1                16  /* num_ref_idx_l1_active_minus1           */
#define SHIFT___PP_CFG__pic_init_qp                                 21  /* pic_init_qp                            */
#define SHIFT___PP_CFG__control_mode                                28  /* control_mode                           */
#define SHIFT___PP_CFG__transform_8x8_mode_flag                     27  /* transform_8x8_mode_flag                               */
#define SHIFT___PP_CFG__direct_8x8_inference_flag                   30  /* direct_8x8_inference_flag                               */
#define SHIFT___PP_CFG__monochrome                                  31  /* reserved                               */
/*
**  PP_PICWIDTH register SHIFTs
*/
#define SHIFT___PP_PICWIDTH__picture_width                     0   /* picture_width                  */
#define SHIFT___PP_PICWIDTH__reserved_1                        7   /* reserved                       */
#define SHIFT___PP_PICWIDTH__nb_MB_in_picture_minus1          16   /* nb_MB_in_picture_minus1 (12:0) */
#define SHIFT___PP_PICWIDTH__reserved_2                       29   /* reserved                       */

/*
**  PP_CODELENGTH  register SHIFTs
*/
#define SHIFT___PP_CODELENGTH__log2_max_frame_num              0  /*  log2_max_frame_num         */
#define SHIFT___PP_CODELENGTH__log2_max_pic_order_cnt_lsb      5  /*  log2_max_pic_order_cnt_lsb */
#define SHIFT___PP_CODELENGTH__reserved_1                     10  /*  reserved                   */

/*
**   PP_START register SHIFTs
*/
#define SHIFT___PP_START__pp_start                             0  /* pp_start */

/*
**   PP_CHUNK register SHIFTs
*/
#define SHIFT____PP_MAX_OPC_SIZE__max_opc_size                 0  /* max_opc_size     */
#define SHIFT____PP_MAX_CHUNK_SIZE__max_chunk_size             0  /* max_chunk_size   */
#define SHIFT____PP_MAX_MESSAGE_SIZE__max_message_size         0  /* max_message_size */


/*
**   PP_ITS register SHIFTs
*/
#define SHIFT___PP_ITS__dma_cmp                       0  /* Write DMA is completed, i.e preprocessor has complely finished its operation (read, parse, decode, write */
#define SHIFT___PP_ITS__srs_cmp                       1  /* Soft reset is completed            */
#define SHIFT___PP_ITS__error_sc_detected             2  /* Error Start Code has been detected */
#define SHIFT___PP_ITS__error_bit_inserted            3  /* Error bit has been inserted in Slice Error Status Buffer */
#define SHIFT___PP_ITS__int_buffer_overflow           4  /* Write address for intermediate buffer reached PP_IBS */
#define SHIFT___PP_ITS__bit_buffer_underflow          5  /* Read stop address reached (PP_BBS), picture decoding not finished */
#define SHIFT___PP_ITS__bit_buffer_overflow           6
#define SHIFT___PP_ITS__read_error                    7
#define SHIFT___PP_ITS__write_error                   8

/*
**   PP_ITM register SHIFTs
*/
#define SHIFT___PP_ITM__dma_cmp                      0  /* Write DMA is completed, i.e preprocessor has complely finished its operation (read, parse, decode, write */
#define SHIFT___PP_ITM__srs_cmp                      1  /* Soft reset is completed            */
#define SHIFT___PP_ITM__error_sc_detected            2  /* Error Start Code has been detected */
#define SHIFT___PP_ITM__error_bit_inserted           3  /* Error bit has been inserted in Slice Error Status Buffer */
#define SHIFT___PP_ITM__int_buffer_overflow          4  /* Write address for intermediate buffer reached PP_IBS */
#define SHIFT___PP_ITM__bit_buffer_underflow         5  /* Read stop address reached (PP_BBS), picture decoding not finished */
#define SHIFT___PP_ITM__bit_buffer_overflow          6
#define SHIFT___PP_ITM__read_error                   7
#define SHIFT___PP_ITM__write_error                  8

/*
**   PP_SRS register SHIFTs
*/
#define SHIFT___PP_SRS__soft_reset                   0  /* soft reset */

#define PP_Read32(Address)          STSYS_ReadRegDev32LE((void *) (Address))
#define PP_Write32(Address, Value)  STSYS_WriteRegDev32LE((void *) (Address), (Value))

#endif /* #ifndef __DELTA_PP_REGISTERS_H */
