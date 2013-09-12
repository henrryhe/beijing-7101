/*****************************************************************************

File name: lists.c

Description: Transponder and channel lists

COPYRIGHT (C) 2004 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "lists.h"

/* Exported variables ----------------------------------------------------- */

LISTS_Transponder_t LISTS_TransponderList[] =
{
    /* Packet injector */
    {  0, MODE_DTV,        0, PLR_VERT, FEC_2_3, QPSK, TONE_OFF,          0,       0, 20000000, "Packet injector (DIRECTV)" },
    /* DVB transponders 28.2E */
    {  1, MODE_DVB, 11527000, PLR_VERT, FEC_2_3, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "Astra 2, tp D4S" },
    {  2, MODE_DVB, 11585000, PLR_HORZ, FEC_2_3, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "Astra 2, tp D7S" },
    {  3, MODE_DVB, 12032000, PLR_HORZ, FEC_2_3, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 17"  },
    {  4, MODE_DVB, 12048000, PLR_VERT, FEC_2_3, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 18"  },
    {  5, MODE_DVB, 12129000, PLR_VERT, FEC_2_3, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 2, tp 22"  },
    /* DVB transponders 19.2E */
    {  6, MODE_DVB, 12032000, PLR_HORZ, FEC_3_4, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 4" },
    {  7, MODE_DVB, 11775000, PLR_VERT, FEC_3_4, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 1068" },
    {  8, MODE_DVB, 12282000, PLR_VERT, FEC_3_4, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 27500000, "Astra 1A, tp 1094" },
    {  9, MODE_DVB, 12550000, PLR_VERT, FEC_5_6, QPSK, TONE_ON,  (int)BAND_EU_HI,  AGC_ON, 22000000, "Astra 1A, tp 1108" },
    /* DIRECTV transponder */
/*    { 10, MODE_DTV, 12238580, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_US,     AGC_ON, 20000000, "BSG200 Modulator"},*/
    { 10, MODE_DTV, 12320000/*12238580*/, PLR_HORZ, FEC_6_7, QPSK, TONE_OFF, (int)BAND_US,     AGC_ON, 20000000, "BSG200 Modulator"}, /*Send by Phil Smith*/

    /*Satellite*/
    { 11, MODE_DVB,  4005000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27000000, "Insat" },
    { 12, MODE_DVB,  3580000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  3255000, "Insat" },
    { 13, MODE_DVB,  4070000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  5000000, "Insat DD" },
    { 14, MODE_DVB,  3706000, PLR_HORZ, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  6000000, "AsiaSat-3S Ch. News" },
    { 15, MODE_DVB,  3742000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON,  3300000, "AsiaSat-3S SAB" },
    { 16, MODE_DVB,  3760000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26000000, "AsiaSat-3S NOW" },
    { 17, MODE_DVB,  3920000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26850000, "AsiaSat-3S Star" },
    { 18, MODE_DVB,  4000000, PLR_HORZ, FEC_7_8, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 26850000, "AsiaSat-3S Xing" },
    { 19, MODE_DVB,  4020000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27250000, "AsiaSat-3S Sah" },
    { 20, MODE_DVB,  4090000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 13333000, "AsiaSat-3S PTV" },
    { 21, MODE_DVB,  4140000, PLR_VERT, FEC_3_4, QPSK, TONE_OFF, (int)BAND_EU_LO,  AGC_ON, 27500000, "AsiaSat-3S Zee" },

    /* US lab */
    { 22, MODE_DTV, 12355000, PLR_HORZ, FEC_6_7, QPSK, TONE_OFF, (int)BAND_US,     AGC_ON, 20000000, "DTV in the clear!"},

    { 23, MODE_DVB, 12371000, PLR_VERT, FEC_5_6, QPSK, TONE_ON,  (int)BAND_US,     AGC_ON, 20000000, "Echostar 6" },
    { 24, MODE_DVB, 12401000, PLR_HORZ, FEC_5_6, QPSK, TONE_ON,  (int)BAND_US,     AGC_ON, 20000000, "Echostar 6" },
    { 25, MODE_DVB, 12430000, PLR_HORZ, FEC_5_6, QPSK, TONE_ON,  (int)BAND_US,     AGC_ON, 20000000, "Echostar 6 - 110" }
};



/* index, transponder, display type, VPID, APID, PCRPID... */
LISTS_Channel_t LISTS_ChannelList[] =
{
    /* Packet injector (default) */
    {  0,  0, TESTAPP_VID, TESTAPP_VPID, TESTAPP_APID, TESTAPP_PCR, TESTAPP_TPID, TESTAPP_SUBPID, TESTAPP_AUD, "stream No.1" },
    /* Applicable for PIP */
#if defined(PIP_SUPPORT)
    {  1,  0, TESTAPP_VID, TESTAPP_AUXVPID, TESTAPP_AUXAPID, TESTAPP_AUXPCR, 0, 0, TESTAPP_AUXAUD, "stream No.2"},
#else
    {  1,  0, TESTAPP_VID, TESTAPP_VPID, TESTAPP_APID, TESTAPP_PCR, 0, 0, TESTAPP_AUD, "stream No.1" },
#endif
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
    /* DirecTV */
    { 13, 10, DISP_NTSC, 120,  121,  120,    0, 0, MP1A, "BSG200 Modulator"},
    /*Satellite*/
    { 14, 11, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "ETV TELEGU"},
    { 15, 11, DISP_PAL, 1360, 1320, 1360,    0, 0, MP1A, "ETV Marathi"},
    { 16, 11, DISP_PAL, 1260, 1220, 1260,    0, 0, MP1A, "ETV Bangla"},
    { 17, 12, DISP_PAL,  513,  256, 8190,    0, 0, MP1A, "Aaj Tak"},
    { 18, 13, DISP_PAL,  308,  256,  308,    0, 0, MP1A, "DD National"},
    { 19, 14, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "Ch. News Asia"},
    { 20, 15, DISP_PAL, 1160, 1120, 1160,    0, 0, MP1A, "SABe TV"},
    { 21, 16, DISP_PAL, 1010, 1011, 1010,    0, 0, MP1A, "NOW"},
    { 22, 17, DISP_NTSC, 512,  640,  512,    0, 0, MP1A, "Star Sports"},
    { 23, 18, DISP_PAL,  514,  648,  514,    0, 0, MP1A, "Xing Kong"},
    { 24, 19, DISP_PAL,  513,  660,  513,    0, 0, MP1A, "SAHARA Rash"},
    { 25, 20, DISP_PAL,  513,  660,  513,  577, 0, MP1A, "PTV"},
    { 26, 21, DISP_PAL,   35,   34,   35,    0, 0, MP1A, "Zee Music"},

    { 27, 22, DISP_NTSC, 100,  101,  100,    0, 0, MP1A, "DTV in the clear"},

    { 28, 23, DISP_NTSC,6946, 6947, 6946,    0, 0, MP1A, "PPV Sport"},
    { 29, 24, DISP_NTSC,4386, 4387, 4386,    0, 0, MP1A, "PPV Sport"},
    { 30, 25, DISP_NTSC,4386, 4387, 4386,    0, 0, MP1A, "PPV Guide"}

};

S32 LISTS_ChannelSelection[MAX_NUM_CHANNELS] = {
#if defined(C_BAND_TESTING)
                                                 26,28,21,22,23,24,25,27,34,
#endif
                                                 13,27,28,29,30,ENDOFLIST
                                               };

/* EOF --------------------------------------------------------------------- */

