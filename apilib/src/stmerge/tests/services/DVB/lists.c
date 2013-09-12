/*****************************************************************************

File name: lists.c

Description: Transponder and channel lists

COPYRIGHT (C) 2004 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "lists.h"

/* Exported Types ------------------------------------------------------ */

/* Exported Constants -------------------------------------------------- */

/* Exported Variables -------------------------------------------------- */

/*lint -e64 */  /* Type mismatch (initialization) (int/enum) */
/*lint -e641 */ /* Converting enum '{...}' to int */

LISTS_Transponder_t LISTS_TransponderList[] =
{
    /* Packet injector */
    {  0, MODE_DVB,        0, PLR_VERT, FEC_2_3, QPSK, TONE_OFF,          0,       0, 27500000, "Packet injector (DVB)" },

    /* DVB transponders 28.2E */
    {  1, MODE_DVB, 11527000, PLR_VERT, FEC_2_3, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "Astra 2, tp D4S" },
    {  2, MODE_DVB, 11585000, PLR_HORZ, FEC_2_3, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "Astra 2, tp D7S" },
    {  3, MODE_DVB, 12032000, PLR_HORZ, FEC_2_3, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 17"  },
    {  4, MODE_DVB, 12048000, PLR_VERT, FEC_2_3, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 18"  },
    {  5, MODE_DVB, 12129000, PLR_VERT, FEC_2_3, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 22"  },
    /* DVB transponders 19.2E */
    {  6, MODE_DVB, 12032000, PLR_HORZ, FEC_3_4, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 4" },
    {  7, MODE_DVB, 11775000, PLR_VERT, FEC_3_4, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 1068" },
    {  8, MODE_DVB, 12282000, PLR_VERT, FEC_3_4, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 1094" },
    {  9, MODE_DVB, 12550000, PLR_VERT, FEC_5_6, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 22000000, "Astra 1A, tp 1108" },
    /* DVB transponders 28.2E (radio) */
    { 10, MODE_DVB, 11798000, PLR_HORZ, FEC_2_3, QPSK,  TONE_ON, (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 5" },
    /*DVB Modulator*/
    { 11, MODE_DVB,  4005000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 24444000, "DVB Modulator" }, /*Frequency for mux_188*/
    { 12, MODE_DVB,  4005000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 18085106, "DVB Modulator" }, /*Frequency for HDWaterKenya*/
    { 13, MODE_DVB,  4005000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500006, "DVB Modulator" }, /*Frequency for Canal10*/
    /*Satellite*/
    { 14, MODE_DVB,  4005000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27000000, "Insat" },
    { 15, MODE_DVB,  3580000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  3255000, "Insat" },
    { 16, MODE_DVB,  4070000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  5000000, "Insat DD" },
    { 17, MODE_DVB,  3706000, PLR_HORZ, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  6000000, "AsiaSat-3S Ch. News" },
    { 18, MODE_DVB,  3742000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  3300000, "AsiaSat-3S SAB" },
    { 19, MODE_DVB,  3760000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26000000, "AsiaSat-3S NOW" },
    { 20, MODE_DVB,  3920000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26850000, "AsiaSat-3S Star" },
    { 21, MODE_DVB,  4000000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26850000, "AsiaSat-3S Xing" },
    { 22, MODE_DVB,  4020000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27250000, "AsiaSat-3S Sah" },
    { 23, MODE_DVB,  4090000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 13333000, "AsiaSat-3S PTV" },
    { 24, MODE_DVB,  4140000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "AsiaSat-3S Zee" },
    /* TERRESTRIAL */
    { 25, MODE_DVB, 514000, 0, FEC_2_3, QAM64, 0, BAND_TER, AGC_ON, 22000000, "Terrestrial DD" }
};

/* index, transponder, display type, VPID, APID, PCRPID... */
LISTS_Channel_t LISTS_ChannelList[] =
{
    /* Packet injector (default) */
    {  0,  0, TSIN_PID, TSIN_PID, TSIN_PID, TSIN_PID ,TSIN_PID, TSIN_PID, TSIN_PID, "stream No.1" },
    {  1,  0, TSIN_PID, TSIN_PID, TSIN_PID, TSIN_PID, 0, 0, TSIN_PID, "stream No.1" },
    /* DVB 28.2E */
    {  2,  1, DISP_PAL,    0,    0,    0,    0, 0, MP1A, ""},
    {  3,  3, DISP_PAL, 2307, 2309, 2307, 2308, 0, MP1A, "ITV News"},
    {  4,  3, DISP_PAL, 2316, 2318, 2316, 2317, 0, MP1A, "QVC"},
    {  5,  3, DISP_PAL, 2328, 2330, 2328, 2328, 0, MP1A, "TV Jobshop"},
    {  6,  2, DISP_PAL,    0,    0,    0,    0, 0, MP1A, ""},
    {  7,  4, DISP_PAL, 2309, 2311, 2309, 2310, 0, MP1A, "CNN"},
    {  8,  5, DISP_PAL, 2306, 2308, 2306, 2307, 0, MP1A, "BBC Parliament"},
    /* DVB 19.2E */
    {  9,  7, DISP_PAL,  165,  100,  165,   47, 0, MP1A, "CNN Int."},
    { 10,  8, DISP_PAL,  169,  116,  169,    0, 0, MP1A, "Fashion TV.COM"},
    { 11,  9, DISP_PAL,  305,  306,  305,    0, 0, MP1A, "Sky News"},
    { 12,  9, DISP_PAL,  171,  172,  171,  173, 0, MP1A, "VIVA Plus"},
    /*DVB Modulator*/
    { 13, 11, DISP_PAL,  513,  641, 8190,    0, 0, MP1A, "MUX_188"},
    { 14, 12, DISP_PAL,   65,   68,   49,    0, 0, MP1A, "HDWaterKenya"},
    { 15, 13, DISP_PAL,  140,  128,  140,    0, 0, MP1A, "Canal10"},
    /*Satellite*/
    { 16, 14, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "ETV TELEGU"},
    { 17, 14, DISP_PAL, 1360, 1320, 1360,    0, 0, MP1A, "ETV Marathi"},
    { 18, 14, DISP_PAL, 1260, 1220, 1260,    0, 0, MP1A, "ETV Bangla"},
    { 19, 15, DISP_PAL,  513,  256, 8190,    0, 0, MP1A, "Aaj Tak"},
    { 20, 16, DISP_PAL,  308,  256,  308,    0, 0, MP1A, "DD National"},
    { 21, 17, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "Ch. News Asia"},
    { 22, 18, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "SABe TV"},
    { 23, 19, DISP_PAL, 1010, 1011, 1010,    0, 0, MP1A, "NOW"},
    { 24, 20, DISP_NTSC, 512,  640,  512,    0, 0, MP1A, "Star Sports"},
    { 25, 21, DISP_PAL,  514,  648,  514,    0, 0, MP1A, "Xing Kong"},
    { 26, 22, DISP_PAL,  513,  660,  513,    0, 0, MP1A, "SAHARA Rash"},
    { 27, 23, DISP_PAL,  513,  660,  513,  577, 0, MP1A, "PTV"},
    { 28, 24, DISP_PAL,   35,   34,   35,    0, 0, MP1A, "Zee Music"},
    /* terrestrial */
    { 29, 25, DISP_PAL,  512,  650,  128,    0, 0, MP1A, "DD NATIONAL"},
    { 30, 25, DISP_PAL,  513,  660,  129,    0, 0, MP1A, "DD METRO"},
    { 31, 25, DISP_PAL,  514,  670,  130,    0, 0, MP1A, "DD BHARTI"},
    { 32, 25, DISP_PAL,  516,  690,  132,    0, 0, MP1A, "DD INDIA"},
    { 33, 25, DISP_PAL,  515,  680,  131,    0, 0, MP1A, "DD JALANDHAR"},
    { 34, 22, DISP_PAL,  515,  680,  515,    0, 0, MP1A, "SAHARA NCR"}
};

S32 LISTS_ChannelSelection[MAX_NUM_CHANNELS] = {
#if defined(C_BAND_TESTING)
                                                23,26,28,21,22,23,24,25,27,34,
#endif
                                                8,7,3,4,5,9,10,11,12,
                                                28,29,30,31,32,ENDOFLIST
                                               };

/* Exported Macros ----------------------------------------------------- */

/* Exported Functions -------------------------------------------------- */

/* EOF --------------------------------------------------------------------- */
