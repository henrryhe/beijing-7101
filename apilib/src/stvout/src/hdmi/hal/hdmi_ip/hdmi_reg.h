/*****************************************************************************
*
*   File name   : hdmi_reg.h
*
*   Description : HDMI register addresses
*
*   COPYRIGHT (C) ST-Microelectronics 2004.
*
*   Date            Modification                                     Name
*   ----            ------------                                     ----
*   24-Feb-2004     Created                                          DB
*   14-Apr-2004     Updated                                          AC
*
*   TODO:
*   =====
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Register offsets */

      /* Register base address */
     #define HDMI_RBA            HDMI_REG_BASE_ADDRESS




     /* ---------------- HDMI registers-----------------------*/

     /*------------------------ HDMI Cell registers
     */
     #define HDMI_CONFIG                     0x000          /* Configuration register */

     /*---------------------- HDMI interrupt registers
     */
     #define HDMI_INTERRUPT_ENABLE           0x004          /* Interrupt Enable register */
     #define HDMI_INTERRUPT_STATUS           0x008          /* Interrupt Status register */
     #define HDMI_INTERRUPT_CLEAR            0x00C          /* Interrupt Clear register */

     /* ------------------------HDMI status register
     */
     #define HDMI_STATUS                     0x010          /* HDMI Status register */

     /* ------------------------Extented control period
      configuration regsiters
     */
     #define HDMI_EXTS_MAX_DELAY             0x014          /* Max. time between extented control periods */
     #define HDMI_EXTS_MIN                   0x018          /* Duration of extended control period */

     /* ------------------------DVI Control register
     */
     #define HDMI_DVI_CONTROL_BITS           0x01C          /* DVI control bits */

     /* ----------------------16 general purpose output pin
     */
     #define HDMI_GPOUT                      0x020          /* 16 general purpose output pins */

     /*-------------------Active Video window co-ordinates
     */
     #define HDMI_ACTIVE_VIDEO_XMIN          0x100          /* Xmin Active Video window co-ordinates */
     #define HDMI_ACTIVE_VIDEO_XMAX          0x104          /* Xmax Active Video window co-ordinate */
     #define HDMI_ACTIVE_VIDEO_YMIN          0x108          /* Ymin Active Video window co-ordinates */
     #define HDMI_ACTIVE_VIDEO_YMAX          0x10C          /* Ymax Active Video window co-ordinate */

     /* ---------------- Channel data that is to be sent
      when the HDCP is not authenticated----------------
     */
     #define HDMI_DEFAULT_CHL0_DATA          0x110
     #define HDMI_DEFAULT_CHL1_DATA          0x114
     #define HDMI_DEFAULT_CHL2_DATA          0x118

     /* ---------------- Captured data registers.
     */
     #define HDMI_CHL0_CAPTURE_DATA          0x11C

     /* ---------------- Audio Channel Configuration
     */
     #define HDMI_AUDIO_CONFIG               0x200          /* Audio layout 2 chl or 8 chl audio */

     /*------------------HDMI Spdif fifo status
     */
     #define HDMI_SPDIF_FIFO_STATUS          0x204

     /*--------------------Header Word register.
     */
     #define HDMI_INFOFRAME_HEADER_WORD      0x210         /* Header Word of AVI info frame */

     /* ---------------- info frame packet word (0->6)
    */
     #define HDMI_INFOFRAME_PACKET_WORD0     0x214
     #define HDMI_INFOFRAME_PACKET_WORD1     0x218
     #define HDMI_INFOFRAME_PACKET_WORD2     0x21C
     #define HDMI_INFOFRAME_PACKET_WORD3     0x220
     #define HDMI_INFOFRAME_PACKET_WORD4     0x224
     #define HDMI_INFOFRAME_PACKET_WORD5     0x228
     #define HDMI_INFOFRAME_PACKET_WORD6     0x22C

     /*---------------- Info frame Config register.
     */
     #define HDMI_INFOFRAME_CONFIG           0x230

     /*----------------Sample Flat mask register.
     */
     #define HDMI_SAMPLE_FLAT_MASK_REG       0x244           /* Sample flat mask register */

     /*-------------------Packet config register.
     */
     #define HDMI_GENCTRL_PACKET_CONFIG      0x248            /* Configuration for general control Packet */

     /*---------------output ref signature (1->3).
     */
     #define HDMI_MISR_REF_SIGNATURE1        0x300           /* MISR1/ channel0 output reference signature */
     #define HDMI_MISR_REF_SIGNATURE2        0x304           /* MISR2/ channel1 output reference signature */
     #define HDMI_MISR_REF_SIGNATURE3        0x308           /* MISR3/ channel2 output reference signature */

     /*-------------------Test Control register.
     */
     #define HDMI_MISR_TEST_CONTROL          0x30C          /* MISR test Control */

     /*-----------------------------Test Status.
     */
     #define HDMI_MISR_TEST_STATUS           0x310          /* MISR test status */

     /* ------------------HDMI audio regeneration
     */
     #define HDMI_AUDN                       0x400          /* HDMI audio regeneration value */

     /* ------------------HDMI Serializer test
      */
     #define HDMI_SERIALIZER_TEST            0x500          /* HDMI serializer test just for debug  */

     /*-------------------HDMI Cell Configuration.
     */
     #define HDMI_CONFIG_MASK                0x80001F1F          /* Configuration register */

     #define HDMI_CONFIG_DEVICE_ENABLE       0x00000001          /* Enable HDMI cell */
     #define HDMI_CONFIG_HDMI_NOT_DVI        0x00000002          /* Select HDMI vs DVI */
     #define HDMI_CONFIG_HDCP_ENABLE         0x00000004          /* Enable HDCP */
     #define HDMI_CONFIG_ESS_NOT_OESS        0x00000008          /* Select ESS vs OESS */
     #define HDMI_CONFIG_SYNC_POLARITY       0x00000010          /* Select HSYNC/VSYNC avtive mode */
     #define HDMI_CONFIG_ENABLE_422          0x00000100          /* Enable YCbCr 4:2:2: format */
     #define HDMI_CONFIG_PIXEL_REPEAT        0x00000200          /* Enable Pixel Repetition by 2 */
     #define HDMI_TMDSCLOCK_MASK             0x00000C00          /* TMDS clock mask*/
     /* dll config ?? */
     #define HDMI_CONFIG_BCH_CLOCK_RATIO     0x00001000          /* Set BCH clock ratio relating to TMDS clock */
     #define HDMI_CONFIG_SW_RESET            0x80000000          /* Software reset enable */

     /*--------------------------------
     *  interrupts registers bits-----
     *--------------------------------*/
     #define HDMI_INTERRUPT_ENABLE_MASK      0x000001FF          /* Interrupt Enable register */
     #define HDMI_INTERRUPT_STATUS_MASK      0x000001FF          /* Interrupt Status register */
     #define HDMI_INTERRUPT_CLEAR_MASK       0x000001FE          /* Interrupt Clear register*/
     #define HDMI_INTERRUPT_CLEAR_ALL        0x000001FE          /* Clears all Interrupts */

     #define HDMI_ITX_DISABLE_ALL            0x00000000          /* Disable all Its */
     #define HDMI_ITX_ENABLE                 0x00000001          /* IT global bit */
     #define HDMI_ITX_SW_RESET_ENABLE        0x00000002          /* SW reset completion IT */
     #define HDMI_ITX_INFOFRAME_ENABLE       0x00000004          /* Info frame transmission IT */
     #define HDMI_ITX_PIX_CAPTURE_ENABLE     0x00000008          /* Pixel capture IT */
     #define HDMI_ITX_HOTPLUG_ENABLE         0x00000010          /* Hot plug IT */
     #define HDMI_ITX_DLL_LOCK_ENABLE        0x00000020          /* Dll lock IT */
     #define HDMI_ITX_NEWFRAME_ENABLE        0x00000040          /* New frame IT */
     #define HDMI_ITX_GENCTRL_PACKET_ENABLE  0x00000080          /* Ceneral ctrl packet Xmission IT */
     #define HDMI_ITX_SPDIF_OVERRUN_ENABLE   0x00000100          /* SPDIF fifo overrun*/

     #define HDMI_ITX_PENDING                0x00000001           /* Global IT pending*/
     #define HDMI_ITX_SW_RESET_PENDING       0x00000002           /* SW reset IT pending*/
     #define HDMI_ITX_INFOFRAME_PENDING      0x00000004           /* Info frame transmission IT pending*/
     #define HDMI_ITX_PIX_CAPTURE_PENDING    0x00000008           /* Pixel capture IT pending*/
     #define HDMI_ITX_HOTPLUG_PENDING        0x00000010           /* Hot plug IT pending*/
     #define HDMI_ITX_DLL_LOCK_PENDING       0x00000020           /* DLL Lock IT pending */
     #define HDMI_ITX_NEWFRAME_PENDING       0x00000040           /* New frame IT pending*/
     #define HDMI_ITX_GENCTRL_PACKET_PENDING 0x00000080           /* Ceneral ctrl packet Xmission IT pending*/
     #define HDMI_ITX_SPDIF_OVERRUN_PENDING  0x00000100           /* SPDIF FIFO overrun IT pending */

     #define HDMI_ITX_SW_RESET               0x00000002           /* SW reset IT */
     #define HDMI_ITX_INFOFRAME              0x00000004           /* Info frame transmission IT*/
     #define HDMI_ITX_PIX_CAPTURE            0x00000008           /* Pixel capture IT*/
     #define HDMI_ITX_HOTPLUG                0x00000010           /* Hot plug IT*/
     #define HDMI_ITX_DLL_LOCK               0x00000020           /* DLL lock IT */
     #define HDMI_ITX_NEWFRAME               0x00000040           /* New frame IT*/
     #define HDMI_ITX_GENCTRL_PACKET         0x00000080           /* Ceneral ctrl packet Xmission IT*/
     #define HDMI_ITX_SPDIF_OVERRUN          0x00000100           /* SPDIF FIFO overrun IT*/


     /*------------------HDMI Status Configuration
     */
     #define HDMI_STATUS_MASK                0x0000001E          /* HDMI Status register */
     #define HDMI_STATUS_SW_RST              0x00000002          /* HDMI Status SW reset*/
     #define HDMI_STATUS_INFO_BUFFER         0x00000004          /* HDMI Status info buffer */
     #define HDMI_STATUS_PIX_CAPTURE         0x00000008          /* HDMI Status pixel Capture */
     #define HDMI_STATUS_HOT_PLUG            0x00000010          /* HDMI Status hot plug*/

     /*------------------HDMI Control periods config
     */
     #define HDMI_EXTS_MAX_DELAY_MASK        0x0000000F          /* Max. time between extented control periods */
     #define HDMI_EXTS_MIN_MASK              0x000000FF          /* Duration of extended control period */
     #define HDMI_EXTS_MAX_DELAY1            0x00000000          /* max delay between extended control periods: 16vsyncs*/
     #define HDMI_EXTS_MAX_DELAY2            0x00000001          /* max delay between extended control periods: 1vsync*/
     #define HDMI_EXTS_DEFAULT_MAX_DELAY     0x00000002          /* max delay between extended control periods (default value) */
     #define HDMI_EXTDS_DEFAULT_MIN          0x00000032          /* specifies duration of extended control (default value)*/
     #define HDMI_EXTDS_MIN                  0x00000000          /* specifies duration of extended control*/

     /*---------------------------DVI Control Bits.
      */
     #define HDMI_DVI_CONTROL_BITS_MASK      0x00000007           /* DVI control bits */
     #define HDMI_DVI_CTRL0                  0x00000001           /* value of CTL0 */
     #define HDMI_DVI_CTRL1                  0x00000002           /* value of CTL1 */
     #define HDMI_DVI_CTRL2                  0x00000004           /* value of CTL2 */

    /* --------------------------General purpose output pins
    */
     #define HDMI_GPOUT_MASK                 0x0000FFFF           /* gpout register mask */

    /* ---------------- Active Video window co-ordinates masks
    */
     #define HDMI_ACTIVE_VIDEO_XMIN_MASK     0x00001FFF           /* Xmin Active Video window co-ordinates */
     #define HDMI_ACTIVE_VIDEO_XMAX_MASK     0x00001FFF           /* Xmax Active Video window co-ordinate */
     #define HDMI_ACTIVE_VIDEO_YMIN_MASK     0x00001FFF           /* Ymin Active Video window co-ordinates */
     #define HDMI_ACTIVE_VIDEO_YMAX_MASK     0x00001FFF           /* Ymax Active Video window co-ordinate */

     /* ---------------- Channel data that is to be
       sent when the HDCP is not authenticated masks
     */
     #define HDMI_DEFAULT_CHL0_DATA_MASK     0x000000FF
     #define HDMI_DEFAULT_CHL1_DATA_MASK     0x000000FF
     #define HDMI_DEFAULT_CHL2_DATA_MASK     0x000000FF

    /* ---------------- Captured data registers masks.
    */
    #define HDMI_CHL0_CAPTURE_DATA_MASK      0x000000FF

    /* ---------------- Audio Channel Configuration
    */
    #define HDMI_AUDIO_CONFIG_MASK           0x000000FF          /* Hdmi Audio Config mask*/
    #define HDMI_AUDIO_CONFIG_8CHL           0x00000001          /*8 channels audio*/
    #define HDMI_SPDIF_IN_DIV_BY_1           0x00000000          /* Divide input SPDIF clock by 1 before using */
    #define HDMI_SPDIF_IN_DIV_BY_2           0x00000002          /* Divide input SPDIF clock by 2 before using */
    #define HDMI_SPDIF_IN_DIV_BY_3           0x00000004          /* Divide input SPDIF clock by 3 before using */
    #define HDMI_SPDIF_IN_DIV_BY_4           0x00000006          /* Divide input SPDIF clock by 4 before using */
    #define HDMI_FIFO_0_OVERRUN_CLEAR        0x00000010          /* Clears FIFO 0 overrun status */
    #define HDMI_FIFO_1_OVERRUN_CLEAR        0x00000020          /* Clears FIFO 1 overrun status */
    #define HDMI_FIFO_2_OVERRUN_CLEAR        0x00000040          /* Clears FIFO 2 overrun status */
    #define HDMI_FIFO_3_OVERRUN_CLEAR        0x00000080          /* Clears FIFO 3 overrun status */


    /* ------------- Info frame Header and Packet mask
    */
    #define HDMI_INFOFRAME_HEADER_WORD_MASK  0x00FFFFFF           /* Header Word of AVI info frame */
    #define HDMI_INFOFRAME_PACKET_WORD_MASK  0xFFFFFFFF

    /*------------------------Info frame configuration
    */
    #define HDMI_INFOFRAME_CONFIG_ENABLE     0x00000001           /* Enable the info frame with data in the FIFO*/

    /* -------------------- Stream dependent registers
    */
    #define HDMI_SAMPLE_FLAT_MASK_REG_MASK   0x0000000F           /* Sample flat mask register */
    #define HDMI_SAMPLE_FLAT_FOR_SB_0        0x00000001           /* Sample flat bit for sub packet 0 */
    #define HDMI_SAMPLE_FLAT_FOR_SB_1        0x00000002           /* Sample flat bit for sub packet 1 */
    #define HDMI_SAMPLE_FLAT_FOR_SB_2        0x00000004           /* Sample flat bit for sub packet 2 */
    #define HDMI_SAMPLE_FLAT_FOR_SB_3        0x00000008           /* Sample flat bit for sub packet 3 */


    /* ------------ HDMI General control packet Configuration
    */
    #define HDMI_GENCTRL_PACKET_CONFIG_MASK  0x00000007           /* Configuration for general control Packet */
    #define HDMI_GENCTRL_PACKET_ENABLE       0x00000001           /* Enable General control packet Xmission*/
    #define HDMI_GENCTRL_PACKET_BUFFNOTREG   0x00000002           /* Data in the buffer transmitted */
    #define HDMI_GENCTRL_PACKET_AVMUTE       0x00000004           /* AVMute status */

    /*-----------------------Misr test Control config.
    */
    #define HDMI_MISR_REF_SIGNATURE_MASK     0x00FFFFFF      /* MISR/ channel output reference signature */
    #define HDMI_MISR_TEST_CONTROL_MASK      0x0000001F      /* MISR test Control */
    #define HDMI_MISR_TEST_STATUS_MASK       0x0000000F      /* MISR test status */

    #define HDMI_MISR_TEST_RESULT_RST        0x00000001      /* Resets The HDMI_MISR_TEST_STATUS */
    #define HDMI_MISR_ENABLE_TEST1           0x00000002      /* signature genration and comparaison enabled for misr1*/
    #define HDMI_MISR_ENABLE_TEST2           0x00000004      /* signature genration and comparaison enabled for misr2*/
    #define HDMI_MISR_ENABLE_TEST3           0x00000008      /* signature genration and comparaison enabled for misr3*/
    #define HDMI_FULL_FRAME_FOR_MISR         0x00000010      /* Entire Frame included for misr */

    #define HDMI_MISR_TEST_RESULT_VALID      0x00000001
    #define HDMI_MISR_TEST_CHL0_FAIL         0x00000002
    #define HDMI_MISR_TEST_CHL1_FAIL         0x00000004
    #define HDMI_MISR_TEST_CHL2_FAIL         0x00000008

    /* ----------------HDMI AUDIO regeneration clock config
    */
    #define HDMI_AUD_REG_CLOCK_MASK          0x000FFFFF

    /* ------------------------------HDMI serializer
    */
    #define HDMI_SERIALIZER_TEST_MASK        0xBFFFFFFF     /* HDMI Serializer test */
    #define HDMI_SERIALIZER_FF_BY_PASSED     0x80000000     /* Frame Formatter output by passed */


#ifdef __cplusplus
}
#endif

/* ------------------------------- End of file  hdmi_reg.h ---------------------------- */



