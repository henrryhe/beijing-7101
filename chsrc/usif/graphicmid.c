/*Modifiction:
Date:2006-03-20 by yxl
1.Modify CH6_DisRectTxtInfoNobck(),使其更加完善，对变宽的西文字体也有效
2.Modify CH6_DisRectTxtInfo(),使其和CH6_DisRectTxtInfoNobck()有效结合。

*/


#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"
/*#include "swconfig.h"*/
#include "string.h"

/*注意RGC_P->LineWidth的使用,每次调用必须赋值，如果没有,用的是上次的值*/

#include "stcommon.h"
#include "stavmem.h"
#include "..\report\report.h"
#include "..\errors\errors.h"

#include "..\main\initterm.h"
#include "graphicbase.h"
#include "graphicmid.h"
#include "..\key\key.h"
#include "..\key\keymap.h"
#include "..\dbase\vdbase.h"
#include "channelbase.h"





                                       

#ifdef USE_GFX

const U8 radius_2[4] = {1, 0, 0, 1, };
const U8 radius_3[6] = {2, 1, 0, 0, 1, 2, };
const U8 radius_4[8] = {3, 1, 1, 0, 0, 1, 1, 3, };
const U8 radius_5[10] = {4, 2, 1, 1, 0, 0, 1, 1, 2, 4, };
const U8 radius_6[12] = {5, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 5, };
const U8 radius_7[14] = {4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, };
const U8 radius_8[16] = {5, 4, 2, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 4, 5, };
const U8 radius_9[18] = {7, 4, 3, 2, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 3, 4, 7, };
const U8 radius_10[20] = {8, 5, 4, 3, 2, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 3, 4, 5, 8, };
const U8 radius_11[22] = {8, 6, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 6, 9, };
const U8 radius_12[24] = {10, 7, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 7, 10, };
const U8 radius_13[26] = {11, 7, 6, 4, 3, 3, 2, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 3, 3, 4, 6, 7, 11, };
const U8 radius_14[28] = {12, 8, 7, 5, 4, 3, 3, 2, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 3, 3, 4, 5, 7, 8, 12, };
const U8 radius_15[30] = {12, 9, 7, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 9, 12, };
const U8 radius_16[32] = {14, 10, 8, 7, 5, 4, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 4, 5, 7, 8, 10, 14, };
const U8 radius_17[34] = {15, 10, 9, 7, 6, 5, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 3, 3, 4, 5, 6, 7, 9, 10, 15, };
const U8 radius_18[36] = {16, 11, 10, 8, 7, 5, 5, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 3, 3, 4, 5, 5, 7, 8, 10, 11, 16, };
const U8 radius_19[38] = {16, 12, 10, 9, 7, 6, 5, 4, 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4, 4, 5, 6, 7, 9, 10, 12, 17, };
const U8 radius_20[40] = {18, 13, 11, 10, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 3, 3, 4, 5, 5, 6, 8, 9, 11, 13, 18, };
const U8 radius_21[42] = {19, 14, 12, 10, 9, 7, 6, 5, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 9, 10, 12, 14, 19, };
const U8 radius_22[44] = {20, 15, 12, 11, 9, 8, 7, 6, 5, 5, 4, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 4, 5, 5, 6, 7, 8, 9, 11, 12, 15, 20, };
const U8 radius_23[46] = {20, 15, 13, 12, 10, 9, 7, 6, 6, 5, 4, 4, 3, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 9, 10, 12, 13, 16, 21, };
/*cqj 20070720 modify for ch_stock*/
const U8 radius_24[48] = {22, 17, 14, 12, 11, 9, 8, 7, 6, 5, 5, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 7, 8, 9, 11, 12, 14, 17, 22, };
const U8 radius_25[50] = {20, 17, 15, 13, 12, 10, 9, 7, 7, 6, 5, 5, 4, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 9, 10, 12, 13, 15, 17, 20, };
const U8 radius_26[52] = {21, 18, 15, 14, 12, 11, 9, 8, 7, 6, 6, 5, 4, 4, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 9, 11, 12, 14, 15, 18, 21, };
const U8 radius_27[54] = {22, 19, 16, 15, 13, 12, 10, 9, 8, 7, 6, 6, 5, 4, 4, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 9, 10, 12, 13, 15, 16, 19, 22, };
const U8 radius_28[56] = {23, 19, 17, 15, 14, 12, 11, 9, 8, 7, 7, 6, 5, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 11, 12, 14, 15, 17, 19, 23, };
const U8 radius_29[58] = {24, 20, 18, 16, 15, 13, 12, 10, 9, 8, 7, 7, 6, 5, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 10, 12, 13, 15, 16, 18, 20, 24, };
const U8 radius_30[60] = {25, 21, 18, 16, 15, 13, 12, 11, 9, 8, 8, 7, 6, 5, 5, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 7, 8, 8, 9, 11, 12, 13, 15, 16, 18, 21, 25, };
const U8 radius_31[62] = {26, 22, 19, 17, 15, 14, 12, 11, 10, 9, 8, 7, 6, 6, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11, 12, 14, 15, 17, 19, 22, 26, };
const U8 radius_32[64] = {27, 23, 20, 18, 16, 14, 13, 12, 11, 9, 9, 8, 7, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 9, 11, 12, 13, 14, 16, 18, 20, 23, 27, };
const U8 radius_33[66] = {28, 24, 21, 19, 17, 15, 14, 12, 11, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 12, 14, 15, 17, 19, 21, 24, 28, };
const U8 radius_34[68] = {29, 25, 21, 19, 18, 16, 14, 13, 12, 11, 10, 9, 8, 7, 6, 6, 5, 4, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 21, 25, 29, };
const U8 radius_35[70] = {30, 26, 22, 20, 18, 16, 15, 14, 13, 11, 10, 9, 9, 8, 7, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 9, 10, 11, 13, 14, 15, 16, 18, 20, 22, 25, 30, };
const U8 radius_36[72] = {31, 26, 23, 21, 19, 17, 16, 14, 13, 12, 11, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 26, 31, };
const U8 radius_37[74] = {32, 27, 24, 22, 20, 18, 16, 15, 14, 13, 11, 10, 10, 9, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 10, 11, 13, 14, 15, 16, 18, 20, 22, 24, 27, 32, };
const U8 radius_38[76] = {33, 28, 25, 22, 21, 19, 17, 16, 15, 13, 12, 11, 10, 9, 9, 8, 7, 6, 6, 5, 5, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 13, 15, 16, 17, 19, 21, 22, 25, 28, 33, };
const U8 radius_39[78] = {34, 29, 26, 23, 21, 19, 18, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 19, 21, 23, 26, 29, 34, };
const U8 radius_40[80] = {35, 30, 27, 24, 22, 20, 18, 17, 16, 15, 13, 12, 11, 10, 10, 9, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 10, 11, 12, 13, 15, 16, 17, 18, 20, 22, 24, 27, 30, 35, };
const U8 radius_41[82] = {36, 31, 27, 25, 23, 21, 19, 18, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 19, 21, 23, 25, 27, 31, 36, };
const U8 radius_42[84] = {37, 32, 28, 25, 24, 22, 20, 18, 17, 16, 15, 13, 12, 11, 11, 10, 9, 8, 7, 7, 6, 6, 5, 5, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 11, 12, 13, 15, 16, 17, 18, 20, 22, 24, 25, 28, 32, 37, };
const U8 radius_43[86] = {38, 32, 29, 26, 24, 22, 21, 19, 18, 17, 15, 14, 13, 12, 11, 10, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13, 14, 15, 17, 18, 19, 21, 22, 24, 26, 29, 32, 38, };
const U8 radius_44[88] = {39, 33, 30, 27, 25, 23, 21, 20, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 23, 25, 27, 30, 33, 39, };
const U8 radius_45[90] = {40, 34, 31, 28, 26, 24, 22, 20, 19, 18, 17, 15, 14, 13, 12, 11, 11, 10, 9, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 11, 12, 13, 14, 15, 17, 18, 19, 20, 22, 24, 26, 28, 31, 34, 40, };
const U8 radius_46[92] = {41, 35, 32, 28, 27, 25, 23, 21, 20, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 9, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 23, 25, 27, 28, 32, 35, 41, };
const U8 radius_47[94] = {42, 36, 33, 29, 27, 26, 24, 22, 20, 19, 18, 17, 15, 14, 13, 12, 12, 11, 10, 9, 8, 8, 7, 7, 6, 5, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15, 17, 18, 19, 20, 22, 24, 26, 27, 29, 33, 36, 42, };
const U8 radius_48[96] = {43, 37, 33, 30, 28, 26, 24, 23, 21, 20, 19, 17, 16, 15, 14, 13, 12, 11, 11, 10, 9, 8, 8, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 19, 20, 21, 23, 24, 26, 28, 30, 33, 37, 43, };
const U8 radius_49[98] = {44, 38, 34, 31, 29, 27, 25, 23, 22, 20, 19, 18, 17, 16, 14, 14, 13, 12, 11, 10, 9, 9, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9, 10, 11, 12, 13, 14, 14, 16, 17, 18, 19, 20, 22, 23, 25, 27, 29, 31, 34, 38, 44, };
const U8 radius_50[100] = {45, 38, 35, 32, 30, 28, 26, 24, 22, 21, 20, 19, 17, 16, 15, 14, 13, 12, 12, 11, 10, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 19, 20, 21, 22, 24, 26, 28, 30, 32, 35, 38, 45, };


#endif
/************************函数说明***********************************/
/*函数名:*/
/*开发人和开发时间:zengxianggen 2007-04-25                        */
/*函数功能描述:为了方便5107平台移植到STi7100平台封装函数
               实现显示刷新功能                                   */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：无                                                  */ 
/*调用注意事项:                                                   */
/*其他说明:                                                       */      
void CH_UpdateAllOSDView(void)
{
  CH_DisplayCHVP(CH6VPOSD,CH6VPOSD.ViewPortHandle);
}


void CH6_MulLine_ChangeLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
							int x_top,int y_top,int Width,int Heigth,char *TxtStr,
							unsigned int txtColor,U8 H_Align,U8 V_Align,boolean MulLine_ALine);

/* #define YXL_AUS_DGTEK yxl 2006-01-04 add this macro for aus dgtek temp interface*/

#ifdef USE_GFX

/**********************************************
函数功能:在OSD画一个象素点
参数:前3为句柄，PosX、PosY为点坐标，Color为颜色
返回值:无
备注:cqj 20070716 add
***********************************************/
 void CH_DrawPixel(STGFX_Handle_t GFXHandle,STLAYER_ViewPortHandle_t CHVPHandle,STGFX_GC_t* pGC,
				  			int PosX,int PosY,unsigned int Color)
{
#ifdef OSD_COLOR_TYPE_RGB16
	static U16* pixel = NULL;
#else
	static U32* pixel = NULL;

#endif
	pixel=memory_allocate(CHSysPartition,4);
	if(pixel != NULL)
	{
#ifdef OSD_COLOR_TYPE_RGB16	
		*pixel=(U16)Color;
#else
             if((Color&0xFF000000) != 0)  
		  *pixel = (Color&0x00FFFFFF)|0x80000000;
		else
		  *pixel =  Color;

#endif
		CH6_PutRectangle(GFXHandle, CHVPHandle, pGC, PosX, PosY, 1, 1, (U8*)pixel);
		memory_deallocate(CHSysPartition, pixel);
		pixel = NULL;
	}
	else
	{
             do_report(0,"CH_DrawPixel  memory is samll\n");
	}
}

#endif

/*cqj 20070721 add for ch_stock*/
void CH_Stk_DrawPoint(STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,unsigned int Color)
{
	int i,j,k;
	int StartX,StartY;
	int PointNumPerDot=3;
	int TotalDots=3;
	int Space;
	int XTemp,YTemp;
	StartX=PosX+10;
	StartY=PosY+10;
	Space=24-PointNumPerDot*TotalDots;
	Space=Space/(TotalDots+1);
	if(Space<0) Space=0;
	Space+=PointNumPerDot;
	XTemp=PosX+1;
#if 0/*zxg1230 change middle to bottom*/
	YTemp=PosY+(24-PointNumPerDot)/2;
#else
   YTemp=PosY+24-PointNumPerDot-2;
#endif
	for(k=0;k<TotalDots;k++)
	{
		StartX=XTemp+Space*k;
		StartY=YTemp;
		for(i=0;i<PointNumPerDot;i++)
		{
			for(j=0;j<PointNumPerDot;j++)
			{
#ifndef USE_GFX/*cqj 20070716 modify*/
				CH6_SetPixelColor(GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,StartX+j,StartY,Color);
#else
				CH_DrawPixel(GHandle, VPHandle, &gGC,StartX+j,StartY,Color);
				
#endif
			}
			StartY++;
		}

	}
}


/*yxl 2004-12-19 add this function*/
void CH6_DrawPoint(int PosX,int PosY,unsigned int Color)
{
	int i,j,k;
	int StartX,StartY;
	int PointNumPerDot=3;
	int TotalDots=3;
	int Space;
	int XTemp,YTemp;
	StartX=PosX+10;
	StartY=PosY+10;
	Space=24-PointNumPerDot*TotalDots;
	Space=Space/(TotalDots+1);
	if(Space<0) Space=0;
	Space+=PointNumPerDot;
	XTemp=PosX+1;
#if 0/*zxg1230 change middle to bottom*/
	YTemp=PosY+(24-PointNumPerDot)/2;
#else
   YTemp=PosY+24-PointNumPerDot-2;
#endif
	for(k=0;k<TotalDots;k++)
	{
		StartX=XTemp+Space*k;
		StartY=YTemp;
		for(i=0;i<PointNumPerDot;i++)
		{
			for(j=0;j<PointNumPerDot;j++)
			{
#ifndef USE_GFX/*cqj 20070716 modify*/
				CH6_SetPixelColor(GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,StartX+j,StartY,Color);
#else
				CH_DrawPixel(GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,StartX+j,StartY,Color);
				
#endif
			}
			StartY++;
		}

	}
}
/*end yxl 2004-12-19 add this function*/

/*显示矩形,向内*/
#if 1/*zxg20040824 change*/
boolean CH6m_DrawRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
						   STGFX_GC_t* pGC,U32 x, U32 y, U32 width, U32 height,unsigned int  color/*填充色*/,unsigned int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{	
	ST_ErrorCode_t    error;
	
	STGXOBJ_Rectangle_t   Rectangle;
	
	STGXOBJ_Rectangle_t   Rectangle1;
	STGXOBJ_Rectangle_t   Rectangle2;
	STGXOBJ_Rectangle_t   Rectangle3;
	STGXOBJ_Rectangle_t   Rectangle4;
	
	STGXOBJ_Bitmap_t DstBMP;
	STGFX_GC_t  *RGC_P;
#if 0	/*yxl 2004-11-10 modify this section*/	
		RGC_P=&gGC;
#else
		RGC_P=pGC;
#endif/*end yxl 2004-11-10 modify this section*/		
	
	if(color==Lcolor)
	{
		Rectangle.PositionX=x;
		Rectangle.PositionY=y;
		Rectangle.Width=width;
		Rectangle.Height=height;
		
		if(color!=-1)
			RGC_P->EnableFilling= TRUE;
		else
			RGC_P->EnableFilling= false;
		
		RGC_P->LineWidth= 1;
		
		if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
		{
			return false;
		}	
		
		/*yxl 2004-12-18 add this section*/
	/*	if(LWidth==0) Lcolor=0;*/

		/*end yxl 2004-12-18 add this section*/

		RGC_P->FillColor.Type=DstBMP.ColorType;
		RGC_P->DrawColor.Type=DstBMP.ColorType;
		if(CH6_SetColorValue(&(RGC_P->FillColor),color)) return false;
		if(CH6_SetColorValue(&(RGC_P->DrawColor),Lcolor)) return false;
		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle, 0); 
	}	
	else
	{
		Rectangle.PositionX=x+LWidth;
		Rectangle.PositionY=y+LWidth;
		Rectangle.Width=width-2*LWidth;
		Rectangle.Height=height-LWidth*2;
		
		Rectangle1.PositionX=x;
		Rectangle1.PositionY=y;
		Rectangle1.Width=width;
		Rectangle1.Height=LWidth;
		
		Rectangle2.PositionX=x;
		Rectangle2.PositionY=y;
		Rectangle2.Width=LWidth;
		Rectangle2.Height=height;
		
		Rectangle3.PositionX=x;
		Rectangle3.PositionY=y+height-LWidth;
		Rectangle3.Width=width;
		Rectangle3.Height=LWidth;
		
		Rectangle4.PositionX=x+width-LWidth;
		Rectangle4.PositionY=y;
		Rectangle4.Width=LWidth;
		Rectangle4.Height=height;
		
		

		if(color!=-1)
			RGC_P->EnableFilling= TRUE;
		else
			RGC_P->EnableFilling= false;	   
		RGC_P->LineWidth= 1;	   
		if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
		{
			return false;
		}	
		
		RGC_P->FillColor.Type=DstBMP.ColorType;
		RGC_P->DrawColor.Type=DstBMP.ColorType;
		
		if(CH6_SetColorValue(&(RGC_P->FillColor),color)) return false;
		if(CH6_SetColorValue(&(RGC_P->DrawColor),color)) return false;
		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle, 0); 
		
		if(CH6_SetColorValue(&(RGC_P->FillColor),Lcolor)) return false;
		if(CH6_SetColorValue(&(RGC_P->DrawColor),Lcolor)) return false;
		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle1, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle2, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle3, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle4, 0);
		
	}
	
	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	if(error==ST_NO_ERROR) return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
}

#else
boolean CH6m_DrawRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y, U32 width, U32 height, int  color/*填充色*/,int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{	
	ST_ErrorCode_t    error;
	
	STGXOBJ_Rectangle_t   Rectangle;
	STGXOBJ_Bitmap_t DstBMP;
	STGFX_GC_t  *RGC_P;
	
	
	RGC_P=&gGC;
	
	if(color!=-1)
		RGC_P->EnableFilling= TRUE;
	else
		RGC_P->EnableFilling= false;
	
	
	RGC_P->LineWidth= LWidth;
	
	
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	
	RGC_P->FillColor.Type=DstBMP.ColorType;
	RGC_P->DrawColor.Type=DstBMP.ColorType;
	if(color==Lcolor)
	{
		Rectangle.PositionX=x;
		Rectangle.PositionY=y;
		Rectangle.Width=width;
		Rectangle.Height=height;
		
		if(CH6_SetColorValue(&(RGC_P->FillColor),color)) return false;
		if(CH6_SetColorValue(&(RGC_P->DrawColor),Lcolor)) return false;		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,RGC_P, Rectangle, 0); 
	}
	else
	{
		
		
	}
	

	
	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	if(error==ST_NO_ERROR) return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
}
#endif

/*圆角矩形*/
#ifndef USE_GFX
#if 1/*zxg20040824 change向内*/
boolean CH6m_DrawRoundRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
								U32 XStart,U32 YStart,U32 Width,U32 Height,U32 Radiu,unsigned int  pColor/*填充色*/,unsigned int  LColor/*边线色*/,U16 LWidth)
{
	STGXOBJ_Rectangle_t   Rectangle;
	STGXOBJ_Rectangle_t   Rectangle1;
	STGXOBJ_Rectangle_t   Rectangle2;
	STGXOBJ_Rectangle_t   Rectangle3;
	STGXOBJ_Rectangle_t   Rectangle4;
	
	STGXOBJ_Rectangle_t   Rectangle5;
	STGXOBJ_Rectangle_t   Rectangle6;
	STGXOBJ_Rectangle_t   Rectangle7;
	STGXOBJ_Rectangle_t   Rectangle8;
	
	STGXOBJ_Bitmap_t DstBMP;
	ST_ErrorCode_t    error;
	
    STGFX_GC_t  *ArcGC_P;
	
#if 0 /*yxl 2004-11-11 modify this line*/
    ArcGC_P=&gGC;
#else
   ArcGC_P=pGC;
#endif/*end yxl 2004-11-11 modify this line*/

	
	if(pColor!=-1) 
		ArcGC_P->EnableFilling= TRUE;
	else
		ArcGC_P->EnableFilling= false;
	ArcGC_P->LineWidth= 1;
	
#if 0 /*yxl 2004-08-16 cancel this section for RGB16 display*/
	ArcGC_P->FillColor.Type=COLORTYPE;
	ArcGC_P->FillColor.Value.CLUT8=pColor;
	ArcGC_P->DrawColor.Type=COLORTYPE;
    ArcGC_P->DrawColor.Value.CLUT8=LColor;
#endif /*end yxl 2004-08-16 cancel this section for RGB16 display*/
	
#if 0 /*yxl 2004-07-23 cancel this section*/
	pDstBMP=&gBMPOSD;
	
	error=STGFX_DrawRoundRectangle(GFXHandle,pDstBMP,ArcGC_P,Rectangle,Radiu,Radiu,0);
#else /*yxl 2004-07-23 add this section*/
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	/*yxl 2004-08-16 add this section*/	 
	if(pColor==LColor)
	{
		
		Rectangle.PositionX=XStart;
		Rectangle.PositionY=YStart;
		Rectangle.Width=Width;
		Rectangle.Height=Height;
		ArcGC_P->FillColor.Type=DstBMP.ColorType;
		ArcGC_P->DrawColor.Type=DstBMP.ColorType;
		if(CH6_SetColorValue(&(ArcGC_P->FillColor),pColor)) return false;
		if(CH6_SetColorValue(&(ArcGC_P->DrawColor),LColor)) return false;
		/*end yxl 2004-08-16 add this section*/	 
		
		error=STGFX_DrawRoundRectangle(GHandle,&DstBMP,ArcGC_P,Rectangle,Radiu,Radiu,0);
	}
	else
	{
		Rectangle.PositionX=XStart+Radiu*2;
		Rectangle.PositionY=YStart+Radiu*2;
		Rectangle.Width=Width-4*Radiu;
		Rectangle.Height=Height-4*Radiu;
		
		
		Rectangle1.PositionX=XStart+Radiu;
		Rectangle1.PositionY=YStart;
		Rectangle1.Width=Width-2*Radiu;
		Rectangle1.Height=2*Radiu;
		
		Rectangle2.PositionX=XStart;
		Rectangle2.PositionY=YStart+Radiu;
		Rectangle2.Width=2*Radiu;
		Rectangle2.Height=Height-2*Radiu;
		
		
		Rectangle3.PositionX=XStart+Radiu;
		Rectangle3.PositionY=YStart+Height-2*Radiu;
		Rectangle3.Width=Width-2*Radiu;
		Rectangle3.Height=2*Radiu;
		
		Rectangle4.PositionX=XStart+Width-2*Radiu;
		Rectangle4.PositionY=YStart+Radiu;
		Rectangle4.Width=2*Radiu;
		Rectangle4.Height=Height-2*Radiu;
		
		
		Rectangle5.PositionX=XStart+Radiu;
		Rectangle5.PositionY=YStart;
		Rectangle5.Width=Width-2*Radiu;
		Rectangle5.Height=LWidth;
		
		Rectangle6.PositionX=XStart;
		Rectangle6.PositionY=YStart+Radiu;
		Rectangle6.Width=LWidth;
		Rectangle6.Height=Height-2*Radiu;
		
		
		Rectangle7.PositionX=XStart+Radiu;
		Rectangle7.PositionY=YStart+Height-LWidth;
		Rectangle7.Width=Width-2*Radiu;
		Rectangle7.Height=LWidth;
		
		Rectangle8.PositionX=XStart+Width-LWidth;
		Rectangle8.PositionY=YStart+Radiu;
		Rectangle8.Width=LWidth;
		Rectangle8.Height=Height-2*Radiu;
		
		
		
		CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Radiu,YStart+Radiu,Radiu,Radiu,LColor,LColor,1);
		CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Radiu,YStart+Height-Radiu-1,Radiu,Radiu,LColor,LColor,1);
		CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Width-Radiu-1,YStart+Radiu,Radiu,Radiu,LColor,LColor,1);
		CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Width-Radiu-1,YStart+Height-Radiu-1,Radiu,Radiu,LColor,LColor,1);
		
		if(CH6_SetColorValue(&(ArcGC_P->FillColor),pColor)) return false;
		if(CH6_SetColorValue(&(ArcGC_P->DrawColor),pColor)) return false;
		
		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P ,Rectangle1, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P ,Rectangle2, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle3, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle4, 0);
		
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle, 0); 
		
		if(CH6_SetColorValue(&(ArcGC_P->FillColor),LColor))return false;
		if(CH6_SetColorValue(&(ArcGC_P->DrawColor),LColor)) return false;
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle5, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle6, 0); 
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle7, 0);
		error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle8, 0); 
		
		if(LWidth<Radiu)
		{
			CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Radiu,YStart+Radiu,Radiu-LWidth,Radiu-LWidth,pColor,pColor,1);
			CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Radiu,YStart+Height-Radiu-1,Radiu-LWidth,Radiu-LWidth,pColor,pColor,1);
			CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Width-Radiu-1,YStart+Radiu,Radiu-LWidth,Radiu-LWidth,pColor,pColor,1);
			CH6m_DrawEllipse(GHandle,VPHandle,&gGC,XStart+Width-Radiu-1,YStart+Height-Radiu-1,Radiu-LWidth,Radiu-LWidth,pColor,pColor,1);
		}
		
	}
	
#endif /*end yxl 2004-07-23 add this section*/
	
	

	
	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	
	
	
	if(error==ST_NO_ERROR)
		return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
	
}
#else
boolean CH6m_DrawRoundRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 XStart,
								U32 YStart,U32 Width,U32 Height,U32 Radiu,int  pColor/*填充色*/,int  LColor/*边线色*/,U16 LWidth)
{
	STGXOBJ_Rectangle_t   Rectangle;
	STGXOBJ_Bitmap_t DstBMP;
	ST_ErrorCode_t    error;
	
    STGFX_GC_t  *ArcGC_P;
	
    Rectangle.PositionX=XStart;
    Rectangle.PositionY=YStart;
    Rectangle.Width=Width;
    Rectangle.Height=Height;
	
    ArcGC_P=&gGC;
	if(pColor!=-1) 
		ArcGC_P->EnableFilling= TRUE;
	else
		ArcGC_P->EnableFilling= false;
    
	ArcGC_P->LineWidth= LWidth;
	
#if 0 /*yxl 2004-08-16 cancel this section for RGB16 display*/
	ArcGC_P->FillColor.Type=COLORTYPE;
	ArcGC_P->FillColor.Value.CLUT8=pColor;
	ArcGC_P->DrawColor.Type=COLORTYPE;
    ArcGC_P->DrawColor.Value.CLUT8=LColor;
#endif /*end yxl 2004-08-16 cancel this section for RGB16 display*/
	
#if 0 /*yxl 2004-07-23 cancel this section*/
	pDstBMP=&gBMPOSD;
	
	error=STGFX_DrawRoundRectangle(GFXHandle,pDstBMP,ArcGC_P,Rectangle,Radiu,Radiu,0);
#else /*yxl 2004-07-23 add this section*/
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	/*yxl 2004-08-16 add this section*/	 
	ArcGC_P->FillColor.Type=DstBMP.ColorType;
	ArcGC_P->DrawColor.Type=DstBMP.ColorType;
	if(CH6_SetColorValue(&(ArcGC_P->FillColor),pColor)) return false;
	if(CH6_SetColorValue(&(ArcGC_P->DrawColor),LColor)) return false;
	/*end yxl 2004-08-16 add this section*/	 
#if 1
	error=STGFX_DrawRoundRectangle(GHandle,&DstBMP,ArcGC_P,Rectangle,Radiu,Radiu,0);
#else
	
	error= STGFX_DrawRectangle(GHandle, &DstBMP,ArcGC_P, Rectangle, 0); 
#endif
	
#endif /*end yxl 2004-07-23 add this section*/
	
	

	
	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	
	
	
	if(error==ST_NO_ERROR)
		return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
	
}
#endif
#else

/***********************************************************************************
函数功能:在OSD画圆
参数:前2为句柄，uCenterX、uCenterY为圆心坐标，uRadius为弧度(1-20pixel)，uColor为填充色
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070710 add
*************************************************************************************/
extern semaphore_t *pst_CircleSema ;
boolean CH6_QFilledCircle( STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,
							U16 uCenterX, U16 uCenterY, U8 uRadius, unsigned int uColor )
{
	U16 	uStartX, uStartY;
	U8  	*pCircleData;
	U16		uPx, uPy;
#ifdef OSD_COLOR_TYPE_RGB16	
	static U16 *CircleData = NULL; 
#else
      static U32 *CircleData = NULL;
#endif

	semaphore_wait(pst_CircleSema);
	uStartX	= uCenterX - uRadius;
	uStartY	= uCenterY - uRadius;

    if((uCenterX-uRadius)<0||(uCenterX+uRadius)>720||(uCenterY-uRadius)<0||(uCenterY+uRadius)>576)
    {
		do_report(0,"Invalid para in CH6_QFilledCircle\n");
		  semaphore_signal(pst_CircleSema);
		return true;
	}

	if( uRadius == 1 )
	{
		CH_DrawPixel(Handle, VPHandle, &gGC,uCenterX,uCenterY,uColor);
		  semaphore_signal(pst_CircleSema);
		return false;
	}

	switch( uRadius )
	{
	case 2:
		pCircleData = (U8*)radius_2;
		break;
	case 3:
		pCircleData = (U8*)radius_3;
		break;
	case 4:
		pCircleData = (U8*)radius_4;
		break;
	case 5:
		pCircleData = (U8*)radius_5;
		break;
	case 6:
		pCircleData = (U8*)radius_6;
		break;
	case 7:
		pCircleData = (U8*)radius_7;
		break;
	case 8:
		pCircleData = (U8*)radius_8;
		break;
	case 9:
		pCircleData = (U8*)radius_9;
		break;
	case 10:
		pCircleData = (U8*)radius_10;
		break;
	case 11:
		pCircleData = (U8*)radius_11;
		break;
	case 12:
		pCircleData = (U8*)radius_12;
		break;
	case 13:
		pCircleData = (U8*)radius_13;
		break;
	case 14:
		pCircleData = (U8*)radius_14;
		break;
	case 15:
		pCircleData = (U8*)radius_15;
		break;
	case 16:
		pCircleData = (U8*)radius_16;
		break;
	case 17:
		pCircleData = (U8*)radius_17;
		break;
	case 18:
		pCircleData = (U8*)radius_18;
		break;
	case 19:
		pCircleData = (U8*)radius_19;
		break;
	case 20:
		pCircleData = (U8*)radius_20;
		break;
		/* cqj 20070720 add for ch_stock*/
	case 21:
		pCircleData = (U8*)radius_21;
		break;
	case 22:
		pCircleData = (U8*)radius_22;
		break;
	case 23:
		pCircleData = (U8*)radius_23;
		break;
	case 24:
		pCircleData = (U8*)radius_24;
		break;		
	case 25:
		pCircleData = (U8*)radius_25;
		break;
	case 26:
		pCircleData = (U8*)radius_26;
		break;
	case 27:
		pCircleData = (U8*)radius_27;
		break;
	case 28:
		pCircleData = (U8*)radius_28;
		break;
	case 29:
		pCircleData = (U8*)radius_29;
		break;
	case 30:
		pCircleData = (U8*)radius_30;
		break;
	case 31:
		pCircleData = (U8*)radius_31;
		break;
	case 32:
		pCircleData = (U8*)radius_32;
		break;
	case 33:
		pCircleData = (U8*)radius_33;
		break;
	case 34:
		pCircleData = (U8*)radius_34;
		break;
	case 35:
		pCircleData = (U8*)radius_35;
		break;
	case 36:
		pCircleData = (U8*)radius_36;
		break;
	case 37:
		pCircleData = (U8*)radius_37;
		break;
	case 38:
		pCircleData = (U8*)radius_38;
		break;
	case 39:
		pCircleData = (U8*)radius_39;
		break;
	case 40:
		pCircleData = (U8*)radius_40;
		break;
	case 41:
		pCircleData = (U8*)radius_41;
		break;
	case 42:
		pCircleData = (U8*)radius_42;
		break;
	case 43:
		pCircleData = (U8*)radius_43;
		break;
	case 44:
		pCircleData = (U8*)radius_44;
		break;
	case 45:
		pCircleData = (U8*)radius_45;
		break;
	case 46:
		pCircleData = (U8*)radius_46;
		break;
	case 47:
		pCircleData = (U8*)radius_47;
		break;
	case 48:
		pCircleData = (U8*)radius_48;
		break;
	case 49:
		pCircleData = (U8*)radius_49;
		break;
	case 50:
		pCircleData = (U8*)radius_50;
		break;
	default:
		do_report(0, "圆的弧度参数错误，请检查<CH6_QFilledCircle>");
		  semaphore_signal(pst_CircleSema);		
		return true;
	}
#ifdef OSD_COLOR_TYPE_RGB16	
 	CircleData= (U16*)CH6_GetRectangle( Handle,VPHandle,&gGC,
									 uStartX, uStartY, (int)(uRadius*2), (int)(uRadius*2 ));  

#else
CircleData= (U32*)CH6_GetRectangle( Handle,VPHandle,&gGC,
									 uStartX, uStartY, (int)(uRadius*2), (int)(uRadius*2 ));  

#endif
	if(CircleData !=NULL)
	{
		for( uPy = 0; uPy < uRadius * 2; uPy ++ )
		{
			for( uPx = pCircleData[uPy]; uPx < ( 2 * uRadius - pCircleData[uPy] ); uPx ++  )
				{
#ifdef OSD_COLOR_TYPE_RGB16				
					CircleData[uPy*uRadius*2+uPx] = uColor;
#else
             if((uColor&0xFF000000) != 0)  
		    CircleData[uPy*uRadius*2+uPx] = (uColor&0x00FFFFFF)|0x80000000 ;
		else
		   CircleData[uPy*uRadius*2+uPx]  = uColor;

#endif
				}
		}
		CH6_PutRectangle(Handle, VPHandle, &gGC,uStartX,uStartY,uRadius*2,uRadius*2,(U8*)CircleData);                        
		memory_deallocate(CHSysPartition,CircleData);
		CircleData = NULL;
		  semaphore_signal(pst_CircleSema);
		return false;
	}
	else
	{
	  do_report(0,"CH6_QFilledCircle  memory is samll\n");
	  semaphore_signal(pst_CircleSema);
	   return true;
	}
	
}

void CH6_DrawRoundRect( STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC1,
								S16  uStartX, S16 uStartY,S16  uWidth, S16 uHeight,
								U8	uRadius,U32 uColorIndex )
{
	S16	uRect1StartX, uRect1StartY, uRect1Width, uRect1Height;	
	S16	uRect2StartX, uRect2StartY, uRect2Width, uRect2Height;	

	S16	uCircle1CenterX, uCircle1CenterY;							
	S16	uCircle2CenterX, uCircle2CenterY;							
	S16	uCircle3CenterX, uCircle3CenterY;							
	S16	uCircle4CenterX, uCircle4CenterY;							

	if(uWidth <= 0)
	{	
		uWidth = 1;
		uRadius = 0;
	}

	if(uHeight <= 0)
	{	
		uHeight = 1;
		uRadius = 0;
	}

	if( uRadius == 0 )
	{
		CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight, uColorIndex,0, 0 );
		return;
	}

	uRect1StartX 	= uStartX;
	uRect1StartY	= uStartY + uRadius;
	uRect1Width	= uWidth;
	uRect1Height	= uHeight - uRadius * 2;

	uRect2StartX	= uStartX + uRadius;
	uRect2StartY	= uStartY;
	uRect2Width	= uWidth - uRadius * 2;
	uRect2Height	= uHeight;

	if(uRect1Height<=0 || uRect2Width<=0)
	{	

		CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight, uColorIndex,0, 0 );
		return;
	}

	uCircle1CenterX	= uStartX + uRadius;
	uCircle1CenterY	= uStartY + uRadius;

	uCircle2CenterX	= uStartX + uWidth - uRadius;
	uCircle2CenterY	= uStartY + uRadius;

	uCircle3CenterX	= uStartX + uWidth - uRadius;
	uCircle3CenterY	= uStartY + uHeight - uRadius;

	uCircle4CenterX	= uStartX + uRadius;
	uCircle4CenterY	= uStartY + uHeight - uRadius;

	CH6_QFilledCircle( Handle, VPHandle, uCircle1CenterX, uCircle1CenterY, uRadius, uColorIndex );
	CH6_QFilledCircle( Handle, VPHandle, uCircle2CenterX, uCircle2CenterY, uRadius, uColorIndex );
	CH6_QFilledCircle( Handle, VPHandle, uCircle3CenterX, uCircle3CenterY, uRadius, uColorIndex );
	CH6_QFilledCircle( Handle, VPHandle, uCircle4CenterX, uCircle4CenterY, uRadius, uColorIndex );

	CH6m_DrawRectangle( Handle, VPHandle,pGC1, uRect1StartX, uRect1StartY, uRect1Width, uRect1Height, uColorIndex,0, 0 );
	CH6m_DrawRectangle( Handle, VPHandle,pGC1, uRect2StartX, uRect2StartY, uRect2Width, uRect2Height, uColorIndex,0, 0 );
	return;
}

/**********************************************************************************************
函数功能:在OSD画圆角矩形
参数:前3为句柄，uStartX、uStartY为顶点坐标，uWidth为宽度，uHeight为高度，uEdgeWidth
	为边线宽，tOutStyle、tInStyle为外框和内框样式，uOutColorIndex、uInColorIndex为外框和内框颜色
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070714 add
*************************************************************************************************/

void CH6_DrawFilledRect( STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC1,
							    S16 uStartX, S16 uStartY, S16 uWidth, S16 uHeight, S16 uEdgeWidth,
							    RECSTYLE	tOutStyle, U8	uOutRadius,U32	 uOutColorIndex,
							    RECSTYLE	tInStyle,  U8	uInRadius, U32	 uInColorIndex	)
{
	S16 uTmpX, uTmpY;
	S16 uTmpWidth, uTmpHeight;

	if( uEdgeWidth > 0 )
	{
		switch( tOutStyle )
		{
		case REC_STYLE_N:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );			
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );			
				break;
			}
		case REC_STYLE_A:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uEdgeWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - 2 * uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY + uOutRadius, uEdgeWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );			
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY + uHeight - uEdgeWidth, uWidth - 2 * uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_R:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY + uOutRadius, uEdgeWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_L:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY + uHeight - uEdgeWidth, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uEdgeWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_LU:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_LD:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY + uEdgeWidth, uEdgeWidth, uHeight - uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY + uHeight - uEdgeWidth, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius,  uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_RU:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY + uOutRadius, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_RD:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth - uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_U:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - 2 * uOutRadius , uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY + uOutRadius, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uHeight - uEdgeWidth, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_B:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uWidth - uEdgeWidth, uStartY, uEdgeWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY + uHeight - uEdgeWidth, uWidth - 2 * uOutRadius, uEdgeWidth, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		}

		switch( tInStyle )
		{
		case REC_STYLE_N:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				break;
			}
		case REC_STYLE_A:
			{
				CH6_DrawRoundRect( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - 2 * uEdgeWidth, uInRadius, uInColorIndex );
				break;
			}
		case REC_STYLE_R:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - uOutRadius - uEdgeWidth, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uOutRadius + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - 2 * ( uOutRadius + uEdgeWidth ), 0, 0, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius - uEdgeWidth, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius - uEdgeWidth, uInColorIndex );
				break;
			}
		case REC_STYLE_L:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth + uOutRadius, uStartY + uEdgeWidth, uWidth - uOutRadius - 2 * uEdgeWidth, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth + uOutRadius, uWidth - 2 * uEdgeWidth, uHeight - 2 * ( uOutRadius + uEdgeWidth ), 0, 0, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius + uEdgeWidth, uStartY + uOutRadius + uEdgeWidth, uInRadius, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius + uEdgeWidth, uStartY + uHeight - uOutRadius - uEdgeWidth, uInRadius, uInColorIndex );
				break;
			}
		case REC_STYLE_LU:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius + uEdgeWidth, uStartY + uOutRadius + uEdgeWidth, uOutRadius, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius + uEdgeWidth, uStartY + uEdgeWidth, uWidth - uEdgeWidth * 2 - uOutRadius, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uOutRadius + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - uEdgeWidth * 2 - uOutRadius, 0, 0, uInColorIndex );
				break;
			}
		case REC_STYLE_LD:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius + uEdgeWidth, uStartY + uHeight - ( uEdgeWidth + uOutRadius ), uOutRadius, uOutRadius );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - uEdgeWidth * 2 - uOutRadius, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth + uOutRadius, uStartY + uEdgeWidth, uWidth - uOutRadius - uEdgeWidth * 2, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				break;
			}
		case REC_STYLE_RU:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uEdgeWidth - uOutRadius, uStartY + uEdgeWidth + uOutRadius, uOutRadius, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - uEdgeWidth * 2 - uOutRadius, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth + uOutRadius, uWidth - 2 * uEdgeWidth, uHeight - 2 * uEdgeWidth - uOutRadius, 0, 0, uInColorIndex );
				break;
			}
		case REC_STYLE_RD:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - uEdgeWidth - uOutRadius, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - uEdgeWidth - uOutRadius, uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uEdgeWidth - uOutRadius, uStartY + uHeight - uEdgeWidth - uOutRadius, uOutRadius, uInColorIndex );
				break;
			}
		case REC_STYLE_U:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uEdgeWidth + uOutRadius, uStartY + uEdgeWidth + uOutRadius, uOutRadius, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uEdgeWidth - uOutRadius, uStartY + uEdgeWidth + uOutRadius, uOutRadius, uInColorIndex );

				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth + uOutRadius, uStartY + uEdgeWidth, uWidth - 2 * ( uEdgeWidth + uOutRadius ), uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth + uOutRadius, uWidth - 2 * uEdgeWidth, uHeight - uEdgeWidth * 2 - uOutRadius, 0, 0, uInColorIndex );
				break;
			}
		case REC_STYLE_B:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uEdgeWidth + uOutRadius, uStartY + uHeight - ( uEdgeWidth + uOutRadius ), uOutRadius, uInColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - ( uEdgeWidth + uOutRadius ), uStartY + uHeight - ( uEdgeWidth + uOutRadius ), uOutRadius, uInColorIndex );

				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth, uStartY + uEdgeWidth, uWidth - 2 * uEdgeWidth, uHeight - 2 * uEdgeWidth - uOutRadius, 0, 0, uInColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uEdgeWidth + uOutRadius, uStartY + uEdgeWidth, uWidth - 2 * ( uEdgeWidth + uOutRadius ), uHeight - 2 * uEdgeWidth, 0, 0, uInColorIndex );
				break;
			}
		}

	}
	else
	{
		switch( tOutStyle )
		{
		case REC_STYLE_N:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight, uOutColorIndex,0, 0 );
				break;
			}
		case REC_STYLE_A:
			{
				CH6_DrawRoundRect( Handle, VPHandle, pGC1,uStartX, uStartY, uWidth, uHeight, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_R:
			{				
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uInRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uInRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_L:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uWidth, uHeight - 2 * uOutRadius, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX+ uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX+ uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_LU:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				break;
			}
		case REC_STYLE_LD:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				break;
			}
		case REC_STYLE_RU:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				break;
			}
		case REC_STYLE_RD:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth - uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		case REC_STYLE_U:
			{
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uOutRadius, uOutRadius, uOutColorIndex );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - 2 * uOutRadius, uHeight, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY + uOutRadius, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				break;
			}
		case REC_STYLE_B:
			{
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX, uStartY, uWidth, uHeight - uOutRadius, uOutColorIndex,0, 0 );
				CH6m_DrawRectangle( Handle, VPHandle,pGC1, uStartX + uOutRadius, uStartY, uWidth - 2 * ( uOutRadius), uHeight, uOutColorIndex,0, 0 );

				CH6_QFilledCircle( Handle, VPHandle, uStartX + uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				CH6_QFilledCircle( Handle, VPHandle, uStartX + uWidth - uOutRadius, uStartY + uHeight - uOutRadius, uOutRadius, uOutColorIndex );
				break;
			}
		}
	}
}

/*********************************************************************************
函数功能:在OSD画圆角矩形
参数:前3为句柄，XStart、Ystart为顶点坐标，Width为宽度，Height为高度，Radiu
	为弧度，pColor为填充色，LColor为边线色，LWidth为边线宽
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070714 add
***********************************************************************************/

boolean CH6m_DrawRoundRectangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
										U32 XStart,U32 YStart,U32 Width,U32 Height,U32 Radiu,unsigned int  pColor,unsigned int  LColor, U16 LWidth )
{



	CH6_DrawFilledRect( GHandle, VPHandle,pGC, XStart, YStart, Width, Height, LWidth,
						REC_STYLE_A, Radiu, LColor,
						REC_STYLE_A, Radiu, pColor );
	return false;
}

#endif

/*线*/
#ifndef USE_GFX
boolean  CH6m_DrawLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
					   STGFX_GC_t* pGC,S32 X1,S32 Y1,S32 X2,S32 Y2,unsigned int Selectcolor)
{
	STGXOBJ_Bitmap_t DstBMP;
	ST_ErrorCode_t    error;
	
	STGFX_GC_t  *RGC_P;
	
#if 0 /*yxl 2004-11-10 modify this line*/
	RGC_P=&gGC;
#else
	RGC_P=pGC;
	pGC->LineWidth=1;
	
#endif/*end yxl 2004-11-10 modify this line*/
	
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	RGC_P->DrawColor.Type=DstBMP.ColorType;
	if(CH6_SetColorValue(&(RGC_P->DrawColor),Selectcolor)) return false;
	
	error= STGFX_DrawLine(GHandle,&DstBMP,RGC_P,X1,Y1,X2,Y2);
	

	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	if(error==ST_NO_ERROR) return  true;
	/*end yxl 2004-07-28 add this section*/	     
	return false;
}
#else



/**********************************************
函数功能:在OSD画任意线段
参数:前3为句柄，uX1、uY1为始点坐标，uX2、uY2为终点坐标，Color为颜色
返回值:无
备注:cqj 20070716 add
***********************************************/
boolean CH_DrawLine( STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
						   U16 uX1, U16 uY1, U16 uX2, U16 uY2, U32 uColor	)  
{
    int x = uX1, y = uY1;
    int dx = uX2-uX1, dy = uY2-uY1;
	int ax, ay, decx, decy, max, var;

    int sx = (dx > 0 ? 1 : (dx < 0 ? -1 : 0));
    int sy = (dy > 0 ? 1 : (dy < 0 ? -1 : 0));

	if((sx*dx+((uX1<uX2)?uX1:uX2))>720||(sy*dy+((uY1<uY2)?uY1:uY2))>576)
	{
		return 1;
	}
	
	if(dx == 0&&dy == 0)
	{
		CH_DrawPixel(GHandle, VPHandle, pGC,uX1,uY1,uColor);
		return 0;
	}
	if (dx == 0)
	{
		STGXOBJ_Rectangle_t 	rect;

		rect.PositionX	= (dy>0)?uX1:uX2;
		rect.PositionY	= (dy>0)?uY1:uY2;
		rect.Width	= 1;
		rect.Height	= (dy>0)?dy:dy*(-1); 
		CH6m_DrawRectangle( GHandle, VPHandle, pGC, uX1, uY1, 1, dy, uColor,uColor/*0*/, 0 );
		return 0;
	}

	if (dy == 0)
	{
		STGXOBJ_Rectangle_t 	rect;

		rect.PositionX	= ((dx>0))?uX1:uX2;
		rect.PositionY	= (dx>0)?uY1:uY2;
		rect.Width	= (dx>0)?dx:dx*(-1);
		rect.Height	= 1; 
		CH6m_DrawRectangle( GHandle, VPHandle,pGC, uX1, uY1, dx, 1, uColor,/*0*/uColor, 0 );
		return 0;
	}
    if ( dx < 0 ) 
		{
			dx = -dx;
    	}
    if ( dy < 0 ) 
		{
			dy = -dy;
    	}
	
    ax = 2*dx, ay = 2*dy;
    max = dx,var = 0;
	
    if ( dy > max ) 
		{
			var = 1; 
		}

    switch ( var )
    {
    case 0: 
        for (decy=ay-dx; ; x += sx, decy += ay)
        {
			CH_DrawPixel(GHandle, VPHandle, pGC,x,y,uColor);
			if ( x == uX2 ) 
				{
					break;
				}
			if ( decy >= 0 ) 
				{ 
					decy -= ax; y += sy; 
				}
        }
        break;
    case 1: 
        for (decx=ax-dy; ; y += sy, decx += ax)
        {
			CH_DrawPixel(GHandle, VPHandle, pGC,x,y,uColor);
			if ( y == uY2 ) 
				{
					break;
				}
			if ( decx >= 0 ) 
				{ 
					decx -= ay; x += sx;
				}
		}
        break;
	default:
		return 1;
    }
   return 0;
}


boolean  CH6m_DrawLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
					   STGFX_GC_t* pGC,U16 X1,U16 Y1,U16 X2,U16 Y2,U32 Selectcolor)
{
	return CH_DrawLine( GHandle, VPHandle,pGC, X1, Y1, X2, Y2, Selectcolor) ;

}


#endif

#if 0 /*yxl 2004-11-11 modify this section*/
/*椭圆，圆的显示*/
/*zxg20040824 change*/
boolean CH6m_DrawEllipse1(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 XStart,U32 YStart,U32 HRadiu,U32 VRadiu,int  pColor/*填充色*/,int  LColor/*边线颜色*/,U16 LWidth)
{
	STGFX_GC_t  *EllipseGC;
	STGXOBJ_Bitmap_t DstBMP;
	ST_ErrorCode_t    error;
	
	EllipseGC=&gGC;
	
	if(pColor!=-1)
		EllipseGC->EnableFilling= TRUE;
	else
		EllipseGC->EnableFilling= false;
	
	EllipseGC->LineWidth= 1;		
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	
    EllipseGC->FillColor.Type=DstBMP.ColorType;
	EllipseGC->DrawColor.Type=DstBMP.ColorType;
	
	if(pColor!=LColor)
	{
		if(CH6_SetColorValue(&(EllipseGC->FillColor),LColor)) return false;
		if(CH6_SetColorValue(&(EllipseGC->DrawColor),LColor)) return false;	
		error= STGFX_DrawEllipse(GHandle,&DstBMP,EllipseGC,XStart,YStart,HRadiu, VRadiu,0);
		if(CH6_SetColorValue(&(EllipseGC->FillColor),pColor)) return false;
		if(CH6_SetColorValue(&(EllipseGC->DrawColor),pColor)) return false;	
		if(LWidth<HRadiu&&LWidth<VRadiu)
			error= STGFX_DrawEllipse(GHandle,&DstBMP,EllipseGC,XStart,YStart,HRadiu-LWidth, VRadiu-LWidth,0);
	}
	else
	{
		if(CH6_SetColorValue(&(EllipseGC->FillColor),pColor)) return false;
		if(CH6_SetColorValue(&(EllipseGC->DrawColor),LColor)) return false;
		
		error= STGFX_DrawEllipse(GHandle,&DstBMP,EllipseGC,XStart,YStart,HRadiu, VRadiu,0);
	}	

	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	if(error==ST_NO_ERROR) return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
	
}
#endif 


boolean CH6m_DrawEllipse(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,
						 U32 XStart,U32 YStart,U32 HRadiu,U32 VRadiu,unsigned int  pColor/*填充色*/,unsigned int  LColor/*边线颜色*/,U16 LWidth)
{
#ifndef USE_GFX /*0*//*20070722 change 因为目前不支持椭圆*/

	STGFX_GC_t  *EllipseGC;
	STGXOBJ_Bitmap_t DstBMP;
	ST_ErrorCode_t    error;
#if 0 /*yxl 2004-11-10 modify this line*/
	EllipseGC=&gGC;
#else
	EllipseGC=pGC;

#endif/*end yxl 2004-11-10 modify this line*/
	
	
	
	if(pColor!=-1)
		EllipseGC->EnableFilling= TRUE;
	else
		EllipseGC->EnableFilling= false;
	
	EllipseGC->LineWidth= LWidth;		
	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	
	
	
	EllipseGC->FillColor.Type=DstBMP.ColorType;
	EllipseGC->DrawColor.Type=DstBMP.ColorType;
	if(CH6_SetColorValue(&(EllipseGC->FillColor),pColor)) return false;
	if(CH6_SetColorValue(&(EllipseGC->DrawColor),LColor)) return false;
	
	
	error= STGFX_DrawEllipse(GHandle,&DstBMP,EllipseGC,XStart,YStart,HRadiu, VRadiu,0);

	/*yxl 2004-07-28 add this section*/
	error=STGFX_Sync(GHandle);
	if(error==ST_NO_ERROR) return  true;
	/*end yxl 2004-07-28 add this section*/	  
	
	return false;
#else
return CH6_QFilledCircle( GHandle, VPHandle,
							XStart, YStart, HRadiu, pColor);
#endif
	
}

#if 0/*20050227 cnacel*/
boolean Identical(char temp1[8],char temp2[8])
{
	int i;
	for(i=0;i<4;i++)
	{
		if(temp1[i]!=temp2[i])
			return false;
	}
	return true;
}
#endif
/* compare password */

S8 PasswordVerify(char* pPassword)
{

	return 1;

}
#if 0

/*======================= Bitmap Scaling Code =============================*/
/*                       位图放大缩小                                       */
void bmscaler(int dest_width,int dest_height,int source_width, int source_height,
			  U8 *DesBitmap, U8 *bitmap)
			  
{
	/*declare local variables*/
	int color,error_term_x,error_term_y,source_x,source_y,screen_x,screen_y;
	int destination_width,destination_height;
	
	/*initialize local variables*/
	source_x=0;
	source_y=0;
	error_term_x=0;
	error_term_y=0;
	screen_x=0;	/*starting x coordinate to draw*/
	screen_y=0;	/*starting y coordinate to draw*/
	destination_width=dest_width;
	destination_height=dest_height;
	
	/*here we go, into the scaling stuff.  Pick which case we have and do it*/
	/*case #1: fatter and taller destination bitmap*/
	if((destination_width  > source_width) &&
		(destination_height > source_height))
	{
		do	/*destination fatter (x-axis) than source bitmap*/
		{
			error_term_x+=source_width;
			if(error_term_x > destination_width)
			{
				error_term_x-=destination_width;
				source_x++;
			}
			screen_x++;
			screen_y=0;
			source_y=0;
			do	/*destination taller (y-axis) than source bitmap*/
			{
				color=bitmap[source_y*source_width+source_x];
				if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/
					*(DesBitmap+screen_y*destination_width+screen_x)=color;
				error_term_y+=source_height;
				if(error_term_y > destination_height)
				{
					error_term_y-=destination_height;
					source_y++;
				}
				screen_y++;
			} while(screen_y < (destination_height));
		} while(screen_x < (destination_width));
	}
	else
		/*case #2: fatter and shorter destination bitmap*/
		if((destination_width > source_width) &&
			(destination_height < source_height))
		{
			do	/*destination fatter (x-axis) than source bitmap*/
			{
				error_term_x+=source_width;
				if(error_term_x > destination_width)
				{
					error_term_x-=destination_width;
					source_x++;
				}
				screen_x++;
				screen_y=0;
				source_y=0;
				do	/*destination shorter (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/	
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=destination_height;
					if(error_term_y > source_height)
					{
						error_term_y-=source_height;
						screen_y++;
					}
					source_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width));
		}
		/*case #3: skinnier and taller destination bitmap*/
		if((destination_width < source_width) &&
			(destination_height > source_height))
		{
			do	/*destination skinnier (x-axis) than source bitmap*/
			{
				error_term_x+=destination_width;
				if(error_term_x > source_width)
				{
					error_term_x-=source_width;
					screen_x++;
				}
				source_x++;
				screen_y=0;
				source_y=0;
				do	/*destination taller (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/		
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=source_height;
					if(error_term_y > destination_height)
					{
						error_term_y-=destination_height;
						source_y++;
					}
					screen_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width-2));
		}
		/*case#4: skinnier and shorter destination bitmap*/
		if((destination_width <= source_width) &&
			(destination_height <= source_height))
		{
			do	/*destination skinnier (x-axis) than source bitmap*/
			{
				error_term_x+=destination_width;
				if(error_term_x > source_width)
				{
					error_term_x-=source_width;
					screen_x++;
				}
				source_x++;
				screen_y=0;
				source_y=0;
				do	/*destination shorter (y-axis) than source bitmap*/
				{
					color=bitmap[source_y*source_width+source_x];
					if((screen_y*destination_width+screen_x)<destination_width*destination_height)/*zxg 030228 fix the bug*/			
						*(DesBitmap+screen_y*destination_width+screen_x)=color;
					error_term_y+=destination_height;
					if(error_term_y > source_height)
					{
						error_term_y-=source_height;
						screen_y++;
					}
					source_y++;
				} while(screen_y < (destination_height));
			} while(screen_x < (destination_width-2));
		}
		
		/*end of the four cases*/
}

/*显示自定大小的图片*/
void CH6m_DisplayBall(char * WhichBall,int x,int y,int width,int height)
{
#if 1
	U8 *temp;
	   int kk;
	   int FileIndex;
	   
	   FileIndex=GetFileIndex(WhichBall);
	   if(FileIndex==-1)
	   {
		   return ;
	   }
	   
	   temp=(U8 *)memory_allocate( CHSysPartition,width*height*sizeof(U8));
	   if(temp==NULL)
	   { 
		   do_report(0,"\n Info(%s,%dL):can't allocate memory",__FILE__,__LINE__);
		   return;
	   }
	   
	   bmscaler(width,height,GlobalFlashFileInfo[FileIndex].iWidth,
		   GlobalFlashFileInfo[FileIndex].iHeight,temp,(U8 *)(GlobalFlashFileInfo[FileIndex].FlasdAddr));
	   
	   {
		   
		   U8 *temp11;
		   U8 *temp22;
		   U8 * test_infomation;
		   
#if 0  /*yxl 2004-08-16 modify this section*/
		   test_infomation=(U8 *)memory_allocate( CHSysPartition,width*height*sizeof(U8));
		   if(test_infomation==NULL)
		   {
			   memory_deallocate(CHSysPartition,temp);
			   do_report(0,"\n Info(%s,%dL):can't allocate memory",__FILE__,__LINE__);
			   return;
		   }
		   
		   CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x,y,
			   width,height,test_infomation);
#else
		   test_infomation=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x,y,width,height);
		   if(test_infomation==NULL)
		   {
			   memory_deallocate(CHSysPartition,temp);
			   do_report(0,"\n Info(%s,%dL):can't allocate memory",__FILE__,__LINE__);
			   return;
		   }
#endif /* end yxl 2004-08-16 modify this section*/
		   
		   temp22=test_infomation;
		   temp11=temp;
		   for(kk=0;kk<width*height;kk++)
		   {			 			  			  
			   if(*(temp11+kk)==0x0||*(temp11+kk)==0xff||(kk+1)%(width)==0)
				   *(temp11+kk)=*(temp22+kk);
		   }
		   memory_deallocate(CHSysPartition,test_infomation);
	   }
	   
	   CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x,y,width,height,temp); 
	   
	   memory_deallocate(CHSysPartition,temp);
#endif
}
#endif

/*有背景的字符串的显示*/
void CH6_DisRectTxtInfo(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  BackColor,
						unsigned int  txtColor,int LineWidth,U8 Align)
{	
	STGFX_Handle_t GFXHandleTemp=GHandle; /*yxl 2004-07-23 add this line*/
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; /*yxl 2004-07-23 add this line*/
	int Text_x;
	int Text_y;
	char temp[100];
	boolean FirstChineseByte=false;
	int TextLen=CH6_GetTextWidth(TxtStr);
	Text_y=y_top+(Heigth-CHARACTER_WIDTH*2)/2;
	CH6m_DrawRectangle(GFXHandleTemp,VPHandleTemp,&gGC,x_top,y_top,Width,Heigth,BackColor,BackColor,LineWidth);
	/*zxg20041230 add for decrease one word*/
	if(Width>100)
	{
		x_top=x_top+6;
		Width=Width-12;
	}
	/**********************************/
	switch(Align)
	{
	case 0:
		Text_x=x_top;				
		break;
	case 1:
		Text_x=x_top+(Width-TextLen)/2;				
		break;
	case 2:
		Text_x=x_top+(Width-TextLen);
		break;
	default:
		break;
		
	}
	
	if(Heigth>=CHARACTER_WIDTH*2)
	{
		Width=Width;
		if(Width>=TextLen) 			
			CH6_EnTextOut( GFXHandleTemp,VPHandleTemp,&gGC,Text_x,  Text_y, txtColor, (char *)TxtStr);
		else 
		{
			int i;
			int OffTemp=0;/*3;*/
			int WidthTemp=0;
			
			for(i=0;i<(Width/CHARACTER_WIDTH-2);i++)
			{
				temp[i]=TxtStr[i];
				if((TxtStr[i]&0x80)!=0)
				{					
					FirstChineseByte=!FirstChineseByte;
				}
				else if(FirstChineseByte==true)
				{
					FirstChineseByte=false;
				}
			}
			if(FirstChineseByte==false) 
			{
				/*if((TxtStr[i]&0x80)==0)
				{
					temp[i]=TxtStr[i];
					temp[i+1]='\0';
				}
				else    yxl 2004-12-21 cancel this section*/
					temp[i]='\0';
			}
			else
			{
				#if 0 /*yxl 2004=12=21 modify this section*/
				temp[i]=TxtStr[i];				
				temp[i+1]='\0';
				#else			
				temp[i-1]='\0';
				WidthTemp=i*CHARACTER_WIDTH;/*CH6_GetTextWidth(temp);*/

				OffTemp=Width-WidthTemp-24;
				if(OffTemp>=CHARACTER_WIDTH-1)/*if(OffTemp>=CHARACTER_WIDTH+3)*/
				{
					temp[i-1]=TxtStr[i-1];
					temp[i]=TxtStr[i];				
					temp[i+1]='\0';
				}
		#endif /*end yxl 2004=12=21 modify this section*/

			}
			WidthTemp=CH6_GetTextWidth(temp);
			if(Align==1)/*zxg20041230 add*/
			{
				OffTemp=(Width-WidthTemp-24)/2;
				if(OffTemp<3) OffTemp=3; 
			}
			else
			{
             OffTemp=0;
			}
			CH6_EnTextOut(GFXHandleTemp,VPHandleTemp,&gGC,x_top+OffTemp,  Text_y, txtColor, (char *)temp);	/*yxl 2004-12-21 add +3*/
			CH6_DrawPoint(x_top+OffTemp+WidthTemp,Text_y,txtColor);
			/*end yxl 2004-12-19 modifty this section*/
			

		}
	}
}
/*无背景的字符串的显示*/
void CH6_DisRectTxtInfoNobck(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  txtColor,U8 Align)
{
	STGFX_Handle_t GFXHandleTemp=GHandle; /*yxl 2004-07-23 add this line*/
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; /*yxl 2004-07-23 add this line*/
	int Text_x;
	int Text_y;
	char temp[100];
	boolean FirstChineseByte=false;
	int TextLen=CH6_GetTextWidth(TxtStr);
	/*zxg20041230 add for decrease one word*/
	if(Width>100)
	{
    x_top=x_top+6;
		Width=Width-12;
	}
	/**********************************/
	Text_y=y_top+(Heigth-CHARACTER_WIDTH*2)/2;
	switch(Align)
	{
	case 0:
		Text_x=x_top;				
		break;
	case 1:	
		Text_x=x_top+(Width-TextLen)/2;
		break;
	case 2:
		Text_x=x_top+(Width-TextLen);
		break;
	default:
		break;
		
	}
	
	if(Heigth>=CHARACTER_WIDTH*2)
	{
		if(Width>=TextLen) 
		CH6_TextOut( GFXHandleTemp,VPHandleTemp,Text_x,  Text_y, txtColor, (char *)TxtStr);
		else 
		{
			/*yxl 2004-12-19 modifty this section*/
			#if 0 
			#else
			int i;
			int OffTemp=0;/*3;*/
			int WidthTemp=0;
				
			for(i=0;i<(Width/CHARACTER_WIDTH-2);i++)
			{
				temp[i]=TxtStr[i];
				if((TxtStr[i]&0x80)!=0)
				{					
					FirstChineseByte=!FirstChineseByte;
				}
				else if(FirstChineseByte==true)
				{
					FirstChineseByte=false;
				}
			}
			if(FirstChineseByte==false) 
			{
				/*if((TxtStr[i]&0x80)==0)
				{
					temp[i]=TxtStr[i];
					temp[i+1]='\0';
				}
				else    yxl 2004-12-21 cancel this section*/
					temp[i]='\0';
			}
			else
			{
				#if 0 /*yxl 2004=12=21 modify this section*/
				temp[i]=TxtStr[i];				
				temp[i+1]='\0';
				#else			
				temp[i-1]='\0';
				WidthTemp=i*CHARACTER_WIDTH;/*CH6_GetTextWidth(temp);*/

				OffTemp=Width-WidthTemp-24;
				if(OffTemp>=CHARACTER_WIDTH-1)/*if(OffTemp>=CHARACTER_WIDTH+3)*/
				{
					temp[i-1]=TxtStr[i-1];
					temp[i]=TxtStr[i];				
					temp[i+1]='\0';
				}
				#endif /*end yxl 2004=12=21 modify this section*/

			}
			WidthTemp=CH6_GetTextWidth(temp);
			if(Align==1)/*zxg20041230 add*/
			{
				OffTemp=(Width-WidthTemp-24)/2;
				if(OffTemp<3) OffTemp=3; 
			}
			else
			{
				OffTemp=0;
			}
			CH6_TextOut(GFXHandleTemp,VPHandleTemp,x_top+OffTemp,  Text_y, txtColor, (char *)temp);	/*yxl 2004-12-21 add +3*/
			CH6_DrawPoint(x_top+OffTemp+WidthTemp,Text_y,txtColor);
		#endif 
			/*end yxl 2004-12-19 modifty this section*/
			
		}
	}
}

/*无背景的字符串的显示*/
void CH6_DisRectTxtInfoNobck_move(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  txtColor,U8 Align)
{
	STGFX_Handle_t GFXHandleTemp=GHandle; /*yxl 2004-07-23 add this line*/
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; /*yxl 2004-07-23 add this line*/
	int Text_x;
	int Text_y;
	char temp[100];
	boolean FirstChineseByte=false;
	int TextLen=CH6_GetTextWidth(TxtStr);

	Text_y=y_top+(Heigth-CHARACTER_WIDTH*2)/2;
	switch(Align)
	{
	case 0:
		Text_x=x_top;				
		break;
	case 1:	
		Text_x=x_top+(Width-TextLen)/2;
		break;
	case 2:
		Text_x=x_top+(Width-TextLen);
		break;
	default:
		break;
		
	}
	
	if(Heigth>=CHARACTER_WIDTH*2)
	{
		if(Width>=TextLen) 
		CH6_TextOut( GFXHandleTemp,VPHandleTemp,Text_x,  Text_y, txtColor, (char *)TxtStr);
		else 
		{
			/*yxl 2004-12-19 modifty this section*/
			#if 0 
			#else
			int i;
			int OffTemp=0;/*3;*/
			int WidthTemp=0;
				
			for(i=0;i<(Width/CHARACTER_WIDTH-2);i++)
			{
				temp[i]=TxtStr[i];
				if((TxtStr[i]&0x80)!=0)
				{					
					FirstChineseByte=!FirstChineseByte;
				}
				else if(FirstChineseByte==true)
				{
					FirstChineseByte=false;
				}
			}
			if(FirstChineseByte==false) 
			{
				
					temp[i]='\0';
			}
			else
			{
							
				temp[i-1]='\0';
				WidthTemp=i*CHARACTER_WIDTH;/*CH6_GetTextWidth(temp);*/

				OffTemp=Width-WidthTemp-24;
				if(OffTemp>=CHARACTER_WIDTH-1)/*if(OffTemp>=CHARACTER_WIDTH+3)*/
				{
					temp[i-1]=TxtStr[i-1];
					temp[i]=TxtStr[i];				
					temp[i+1]='\0';
				}
				
			}
			WidthTemp=CH6_GetTextWidth(temp);
			if(Align==1)/*zxg20041230 add*/
			{
				OffTemp=(Width-WidthTemp-24)/2;
				if(OffTemp<3) OffTemp=3; 
			}
			else
			{
				OffTemp=0;
			}
			CH6_TextOut(GFXHandleTemp,VPHandleTemp,x_top+OffTemp,  Text_y, txtColor, (char *)temp);	/*yxl 2004-12-21 add +3*/
			CH6_DrawPoint(x_top+OffTemp+WidthTemp,Text_y,txtColor);
		#endif 
			/*end yxl 2004-12-19 modifty this section*/
			
		}
	}
}


/*多行显示包括水平和垂直方向对齐方式处理*/
void CH6_MulLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int x_top,int y_top,int Width,int Heigth,char *TxtStr,unsigned int  txtColor,U8 H_Align,U8 V_Align)
{

    CH6_MulLine_ChangeLine( GHandle, VPHandle,
							 x_top, y_top, Width, Heigth, TxtStr,
							  txtColor, H_Align, V_Align,true);
}





void DispLaySelect(int x,int y,unsigned int  Selectcolor)
{
	int x1,y1,x2,y2;
	x1=x-5;
	y1=y-5;
	x2=x+15;
	y2=y-15;
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x,y,x1,y1,Selectcolor);
	
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x,y,x2,y2,Selectcolor);
	
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x-1,y,x1-1,y1,Selectcolor);
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x-1,y,x2-1,y2,Selectcolor);
	
	
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+1,y,x1+1,y1,Selectcolor);
	CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+1,y,x2+1,y2,Selectcolor);
	
	
	 	 CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x-2,y,x1-2,y1,Selectcolor);
		 CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x-2,y,x2-2,y2,Selectcolor);
		 
		 
		 CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+2,y,x1+2,y1,Selectcolor);
		 CH6m_DrawLine(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+2,y,x2+2,y2,Selectcolor);
}



/*显示小图标*/

boolean CH6_ShowIcon1(U32 iStartX ,U32 iStartY ,char * WhichBall)
{
	STGXOBJ_Bitmap_t* tempBitmap;
	int FileIndex;
	FileIndex=GetFileIndex(WhichBall);
	if(FileIndex==-1)
	{
		return true;
	}
#if 0
	tempBitmap=&(gBMPOSD.Bitmap);
#endif
	tempBitmap->Width  = GlobalFlashFileInfo[FileIndex].iWidth;
	tempBitmap->Height = GlobalFlashFileInfo[FileIndex].iHeight;
	
	tempBitmap->Data1_p = (U8*)(GlobalFlashFileInfo[FileIndex].FlasdAddr);
	CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,iStartX,iStartY,tempBitmap->Width ,tempBitmap->Height,tempBitmap->Data1_p); 
	return false;
	
}

/*显示当前右上角菜单标题*/
void DisplayUpTitle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *CH_Title,char *EN_Title)
{
	STGFX_Handle_t GFXHandleTemp=GHandle; /*yxl 2004-07-23 add this line*/
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; /*yxl 2004-07-23 add this line*/	
	int x1=550;
	int x2=510;
	int y1=90-30;
	int y2=120-30;
#if 0 /*yxl 2004-07-23 cancel this section*/
	if(pstBoxInfo->abiLanguageInUse==0)
	{
		CH6_TextOut(GFXHandle,CH6VPOSD.ViewPortHandle,x1, y1,246,CH_Title );
		CH6_TextOut(GFXHandle, CH6VPOSD.ViewPortHandle,x2, y2,246,EN_Title );
	}
	else
	{
	       CH6_TextOut(GFXHandle,CH6VPOSD.ViewPortHandle,x1, y1,246,CH_Title );
		   CH6_TextOut(GFXHandle,CH6VPOSD.ViewPortHandle, x2, y2,246,EN_Title );
	}
#else /*yxl 2004-07-23 add this section*/
	if(pstBoxInfo->abiLanguageInUse==0)
	{
		CH6_TextOut(GFXHandleTemp,VPHandleTemp,x1, y1,246,CH_Title );
		CH6_TextOut(GFXHandleTemp,VPHandleTemp,x2, y2,246,EN_Title );
	}
	else
	{
		CH6_TextOut(GFXHandleTemp,VPHandleTemp,x1, y1,246,CH_Title );
		CH6_TextOut(GFXHandleTemp,VPHandleTemp, x2, y2,246,EN_Title );
	}
	
#endif /*end yxl 2004-07-23 add this section*/
}

/****得到位图数据中的某一块区域******/
U8* GetBmpData(U32 start_x,U32 start_y,U32 width,U32 height,U32 source_width,U32 source_height,U8 *SourceData)
{
    int source_x,source_y,dest_x,dest_y;
	U8* DesDta;
    source_x=start_x;
    source_y=start_y;
	DesDta=(U8 *)memory_allocate(CHSysPartition,width*height);
	if(DesDta==NULL)
		return NULL;
	for(dest_x=0;dest_x<width;dest_x++)
	{
		for(dest_y=0;dest_y<height;dest_y++)
		{
			*(DesDta+dest_y*width+dest_x)=*(SourceData+source_y*source_width+source_x); 
			source_y++;
		}
		source_x++;
		source_y=start_y;
	}
	
	
	return DesDta;
}

/*显示背景某一部分*/
#ifdef ENGLISH_FONT_MODE_EQUEALWIDTH  /*yxl 2006-03-06 move this function to graphicbase.c,
yxl 2006-04-22 modify 0 to ifdef ENGLISH_FONT_MODE_EQUEALWIDTH */
/***************************************/

/*函数描述:获取一行文字的宽度，单位象素
*参数:
text:指向文字的指针
*返回值:
-1:错误
Width:文字的长度
*/
int CH6_GetTextWidth(char* text)
{
	
	int Width = 0 ;
	int Number = strlen(text) ;
	
	if( Number > 255) return -1 ;
	   Width=strlen(text)*CHARACTER_WIDTH;
	return Width;
}

#endif /*end yxl 2006-03-06 move this function to graphicbase.c*/


/*函数说明:计算数字的位数
*/
int CH6_ReturnNumofData(int InputData)
{
	if(InputData<=0)
		return 0;
	else if(InputData>0&&InputData<=9)
	{
		return 1;
	}
	else if(InputData>9&&InputData<=99)
	{
		return 2; 
	}
	else if(InputData>99&&InputData<=999)
	{
		return 3;
	}
	else if(InputData>999&&InputData<=9999)
	{
		return 4;
	}
	else if(InputData>9999&&InputData<=99999)
	{
		return 5;
	}
	else 
		return 6;
}



/*函数描述:显示连接两点的折线，折线由三段组成，
第一段水平直线，第二段竖直直线，第三段水平直线
*参数:
StartPosX	:折线起点位置的横坐标
StartPosY		:折线起点位置的纵坐标
EndPosX		:折线终点位置的横坐标
EndPosY		:折线终点位置的纵坐标
Ratio		:第一段和第三段的比例
Thickness	:线的粗细
LineColor		:线的颜色
*返回值:无
*/
void CH6_DrawLinkLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int StartPosX,int StartPosY,int EndPosX,int EndPosY,U8 Ratio,int LineThickness,unsigned int  LineColor)
{	
	STGFX_Handle_t GFXHandleTemp=GHandle; /*yxl 2004-07-23 add this line*/
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; /*yxl 2004-07-23 add this line*/
	int totalWidth;
	totalWidth=EndPosX-StartPosX;	
	CH6m_DrawRectangle(/*GFXHandle,CH6VPOSD.ViewPortHandle*/GFXHandleTemp,VPHandleTemp,&gGC,StartPosX,StartPosY-LineThickness/2, totalWidth*Ratio/(Ratio+1),LineThickness,LineColor,LineColor,1) ;	
	CH6m_DrawRectangle(/*GFXHandle,CH6VPOSD.ViewPortHandle*/GFXHandleTemp,VPHandleTemp,&gGC,StartPosX+totalWidth*Ratio/(Ratio+1),EndPosY-3 ,totalWidth/(Ratio+1),LineThickness, LineColor,LineColor,1);
	if(StartPosY<=EndPosY)
	{    
		CH6m_DrawRectangle(/*GFXHandle,CH6VPOSD.ViewPortHandle*/GFXHandleTemp,VPHandleTemp,&gGC,StartPosX+totalWidth*Ratio/(Ratio+1),StartPosY-3 ,LineThickness,EndPosY-StartPosY+6, LineColor,LineColor,1);
		
	}
	else
	{
		CH6m_DrawRectangle(/*GFXHandle,CH6VPOSD.ViewPortHandle*/GFXHandleTemp,VPHandleTemp,&gGC,StartPosX+totalWidth*Ratio/(Ratio+1),EndPosY-3 ,LineThickness,StartPosY-EndPosY+6, LineColor,LineColor,1);			
	}	
	
}



#if 0 /*yxl 2004-11-11 cancel this function*/
/*函数描述:画一填充颜色的圆角矩形，在其中显示一行文字
*参数:
iStartX		:圆角矩形起点横坐标
iStartY		:圆角矩形起点纵坐标
iWidth		:圆角矩形宽度
iHeight		:圆角矩形高度
Color		:填充颜色
pItemMessage:要显示的文字
*返回值:无
*/
boolean CH6_DrawRoundRecText(STGFX_Handle_t Ghandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,U32 StartX ,U32 StartY , U32 Width , U32 Height ,int  BackColor,int TextColor, int Align,char* pItemMessage)
{
	int TextWidth;	
	STGXOBJ_Bitmap_t BMPTemp;	/*STGXOBJ_Bitmap_t* BMPTemp;yxl 2004-07-23 modify this line*/
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp; 
	
#if 0 /*yxl 2004-11-11 modify this line*/
	pGCTemp=&gGC;
#else
	pGCTemp=pGC;
#endif/*end yxl 2004-11-11 modify this line*/

	pGCTemp->EnableFilling=TRUE;	

	
	RectTemp.PositionX=StartX;
	RectTemp.PositionY=StartY;
	RectTemp.Width=Width;
	RectTemp.Height=Height;	
	
	
	if(CH6_GetBitmapInfo(VPHandle,&BMPTemp))
	{
		return false;
	}	
	
	pGCTemp->FillColor.Type=BMPTemp.ColorType;
	pGCTemp->DrawColor.Type=BMPTemp.ColorType;
	if(CH6_SetColorValue(&(pGCTemp->FillColor),BackColor)) return false;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),BackColor)) return false;
	
	STGFX_DrawRoundRectangle(Ghandle,&BMPTemp,pGCTemp,RectTemp,Height/2,Height/2,0);
	STGFX_Sync(Ghandle);/*yxl 2004-07-28 add this line*/
	
	
    pGCTemp->FillColor.Type=BMPTemp.ColorType;
	pGCTemp->DrawColor.Type=BMPTemp.ColorType;
	if(CH6_SetColorValue(&(pGCTemp->FillColor),BackColor)) return false;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),BackColor)) return false;
	
	
	
	TextWidth=CH6_GetTextWidth(pItemMessage);
	if(TextWidth!=-1)
	{
		if(TextWidth<=Width)
		{
			switch(Align)
			{
			case 0:/*居左*/
				CH6_TextOut(Ghandle,VPHandle,StartX, StartY + (Height-CHARACTER_WIDTH*2)/2,TextColor,pItemMessage);
				break;
				
			case 1:/*居中*/
				/*CH6_TextOut(GFXHandle,StartX+(Width - TextWidth) /2 , StartY + (Height-CHARACTER_WIDTH*2)/2,TextColor,pItemMessage);*/
				CH6_TextOut(Ghandle,VPHandle,StartX + (Width - TextWidth) /2 , StartY + (Height-CHARACTER_WIDTH*2)/2,TextColor,pItemMessage);
				break;
				
			case 2:/*居右*/
				CH6_TextOut(Ghandle,VPHandle,StartX+Width-TextWidth,StartY + (Height-CHARACTER_WIDTH*2)/2,TextColor,pItemMessage);
				break;
				
			default:
				break;
				
			}		
		}
		else
			STTBX_Print(("Text width is longer then Roundrectangle width"));
	}
	else
	{
		STTBX_Print(("Text width is too large"));
	}
	return true;
	
}
#endif/*yxl 2004-11-11 cancel this function*/

#if 0 /*yxl 2004-11-11 cancel this function*/ 

/*函数描述:画一背景框，并在其上画水平滑动杆和滑动块，
三者都是填充了颜色的矩形			     
*参数:
StartX		:背景框起点横坐标
StartY		:背景框起点纵坐标
Height		:背景框高度
sWidth		:滑动块宽度
sHeight		:滑动块高度
sLineWidth	:滑动块边线宽度
lHeight		:滑动杆粗细
ItemMax		:最大滑动次数
Step			:步进值
Index		:当前滑动块所处位置
Color		:背景框颜色
sColor		:滑动杆颜色和滑动块内框颜色
*返回值:
tempWidth:背景框宽度
*/

int CH6_DrawHorizontalSlider(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, U32 StartX,U32 StartY,U32 Height,U32 sWidth, U32 sHeight,U32 sLineWidth,U32 lHeight,U32 ItemMax,U32 Step,U32 Index,int Color,int sColor)
{
	int  i,tempWidth;
	STGXOBJ_Bitmap_t BMPTemp;	/*STGXOBJ_Bitmap_t* BMPTemp;yxl 2004-07-23 modify this line*/
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp;
	
	i=3;	
	tempWidth=Step*(ItemMax-1)+i*2+(sWidth+sLineWidth-1);
	

#if 0 /*yxl 2004-11-11 modify this line*/
	pGCTemp=&gGC;
#else
	pGCTemp=pGC;
#endif/*end yxl 2004-11-11 modify this line*/

	pGCTemp->EnableFilling=TRUE;

	pGCTemp->LineWidth=1;
	
	RectTemp.PositionX=StartX;
	RectTemp.PositionY=StartY;
	
	RectTemp.Width=tempWidth;
	RectTemp.Height=Height;
	
	
	if(CH6_GetBitmapInfo(VPHandle,&BMPTemp))
	{
		return -1;
	}	
	
	pGCTemp->FillColor.Type=BMPTemp.ColorType;
	pGCTemp->DrawColor.Type=BMPTemp.ColorType;
	if(CH6_SetColorValue(&(pGCTemp->FillColor),Color)) return -1;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),Color)) return -1;
	
	
	STGFX_DrawRectangle(GHandle,&BMPTemp, pGCTemp, RectTemp, 0);
	STGFX_Sync(GHandle);/*yxl 2004-07-28 add this line*/
	
	
/*	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;
*/
	pGCTemp->LineWidth=1;
	
	pGCTemp->FillColor.Type=BMPTemp.ColorType;
	pGCTemp->DrawColor.Type=BMPTemp.ColorType;
	if(CH6_SetColorValue(&(pGCTemp->FillColor),sColor)) return -1;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),sColor)) return -1;
	
	RectTemp.PositionX=StartX+i;
	RectTemp.PositionY=StartY+(Height-lHeight)/2;
	
	RectTemp.Width=tempWidth-i*2;
	RectTemp.Height=lHeight;
	
	
	STGFX_DrawRectangle(GHandle,/*BMPTemp*/&BMPTemp, pGCTemp, RectTemp, 0);
	STGFX_Sync(GHandle);/*yxl 2004-07-28 add this line*/
	
/*	
	gGC.EnableFilling=TRUE;
	pGCTemp=&gGC;
*/	
	pGCTemp->LineWidth=sLineWidth;
	
	
	pGCTemp->FillColor.Type=BMPTemp.ColorType;
	pGCTemp->DrawColor.Type=BMPTemp.ColorType;
	
	if(CH6_SetColorValue(&(pGCTemp->FillColor),Color)) 
		return -1;
	if(CH6_SetColorValue(&(pGCTemp->DrawColor),sColor)) 
		return -1;
	
	/*BMPTemp=&gBMPOSD;yxl 2004-07-23 cancel this line*/
	
	RectTemp.PositionX=StartX+i+Index*Step;
	RectTemp.PositionY=StartY+(Height-sHeight)/2;
	
	RectTemp.Width=sWidth;
	RectTemp.Height=sHeight;
	
	STGFX_DrawRectangle(GHandle,/*BMPTemp*/&BMPTemp, pGCTemp, RectTemp, 0);
	STGFX_Sync(GHandle);/*yxl 2004-07-28 add this line*/
	
	pGCTemp->LineWidth=1;
	return tempWidth;
	
}
#endif 


void CH6_DrawLog(void)
{
#if 0/*20070704 change*/

	CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,0,0,LOGMPG,false);
#else
       CH6_CloseMepg();
	CH_DisplayIFrame(LOGMPG,10,7);
#endif
}

#if 1 /*yxl 2005-02-23 move this function from osaicDbaseIF.c to graphicmid.c*/
/*Close Mpeg I display*/
void CH6_CloseMepg(void)
{
#if 1

	ST_ErrorCode_t          		ErrCode;
	
	STVID_StartParams_t		        tStartParam;

	STVID_Freeze_t          		Freeze;

	STVID_Stop_t                      StopMode = STVID_STOP_NOW;
	
	Freeze.Field = STVID_FREEZE_FIELD_TOP;
    Freeze.Mode = STVID_FREEZE_MODE_NONE /*FORCE*/;
	

	tStartParam.RealTime 		= TRUE;     					
	tStartParam.UpdateDisplay 	= TRUE;
    tStartParam.StreamType 	=     STVID_STREAM_TYPE_PES;

     
	tStartParam.StreamID 		= STVID_IGNORE_ID;     		/*By default accept all packets */
	tStartParam.BrdCstProfile 	= STVID_BROADCAST_DVB;
	

	/*STVID_DisableOutputWindow(VIDVPHandle);*/

   	

	ErrCode = STVID_Stop( VIDHandle[VID_MPEG2], StopMode, &Freeze );
     
	STPTI_SlotClearPid(VideoSlot);/*20070810 clear video buffer*/


/*	STPTI_SlotUnLink(VideoSlot); */
#else
    CH6_TerminateChannel();
#endif
	
}
#endif  /*end yxl 2005-02-23 move this function from osaicDbaseIF.c to graphicmid.c*/
void CH6_DrawGame(void)
{
	CH6_CloseMepg();	
	CH_SetVideoAspectRatioConversion(CH_GetSTVideoARConversionFromCH(CONVERSION_FULLSCREEN));/*20070815 add*/
	// CH_DisplayIFrame(SEARCH_MPG,10,7);
	CH_DisplayIFrame(SEARCH_MPG,1,7);


}
void CH6_DrawRadio(void)
{

      	CH6_CloseMepg();	
      CH_DisplayIFrame(SEARCH_MPG,10,7);

}
/*20060422 add display gehua radio back I-frame picture*/
void CH_DrawGehuaRadioBackpic(void)
{
#if 0
#if 0/*20070704 change*/
   CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,0,0,/*BACKGROUND_PIC 20060409 chnage*/RADIOBACK,/*true zxg 20051116 chanfe to false for quickly display*/false);
#else
	CH6_CloseMepg();	

//CH_DisplayIFrame(RADIOBACK,10,7);
CH_DisplayIFrame(RADIOBACK,1,7);
#endif
#else
CH6_AVControl( VIDEO_SHOWPIC, false, CONTROL_VIDEO);
#endif
}

/*-------------------------------------------------------------------------
ljg 040831 add 
--------------------------------------------------------------------------*/
void CH6_DrawAdjustBar(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int StartX,int StartY,int PreValue,int CurValue,BOOL FirstDraw,U8 FillColorType)
{
	int i;	
	int FillColor;
	int progX,ProgY,tempProgX,tempProgY;
	int block_index[6];
	int Value=CurValue;
	boolean Success;
	progX=tempProgX=StartX+8;
	ProgY=tempProgY=StartY+2;
		
	if((CurValue-PreValue)>30||(PreValue-CurValue)>30)
		FirstDraw=TRUE;
	
	if(FirstDraw==TRUE)
	{
		CH6_DisplayPIC(GHandle,VPHandle,StartX,StartY,PROG_PIC,true);
		CH6_DisplayPIC(GHandle,VPHandle,StartX,StartY,(FillColorType==0)?LEFT_PROG_Y : LEFT_PROG_B,true);
		for(i=0;i<6;i++)
		{
			switch(i)
			{
			case 0:
				block_index[i]=Value/50;
				Value=Value%50;
				break;
			case 1:
				block_index[i]=Value/20;
				Value=Value%20;
				break;
			case 2:
				block_index[i]=Value/10;
				Value=Value%10;
				break;
			case 3:
				block_index[i]=Value/5;
				Value=Value%5;
				break;
			case 4:
				block_index[i]=Value/2;
				Value=Value%2;
				break;
			case 5:
				block_index[i]=Value;		
				break;				
			}		
		}	

		i=0;
		while(block_index[0]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOW_PROG_50: BLUE_PROG_50, true);
			tempProgX+=150;
			block_index[0]-=1;		
		}

		i=0;
		while(block_index[1]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOW_PROG_20: BLUE_PROG_20, true);
			tempProgX+=60;
			block_index[1]-=1;	
	}
	
		i=0;
		while(block_index[2]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOW_PROG_10: BLUE_PROG_10, true);
			tempProgX+=30;
			block_index[2]-=1;		
		}

		i=0;
		while(block_index[3]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOW_PROG_5: BLUE_PROG_5, true);
			tempProgX+=15;
			block_index[3]-=1;		
		}

		i=0;
		while(block_index[4]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOW_PROG_2: BLUE_PROG_2, true);
			tempProgX+=6;
			block_index[4]-=1;		
		}

		i=0;
		while(block_index[5]>0)
		{
			CH6_DisplayPIC(GHandle,VPHandle,tempProgX,tempProgY,(FillColorType==0)?YELLOWFILL_PIC: BLUEFILL_PIC, true);
			tempProgX+=3;
			block_index[5]-=1;	
		}
	if(CurValue==100)
	{ 
		CH6_DisplayPIC(GHandle,VPHandle,progX+300,StartY,(FillColorType==0)?RIGHT_PROG_Y : RIGHT_PROG_B,true);
	}
	}
	else
	{		
	if(CurValue>=PreValue)
	{
		for(i=PreValue;i<CurValue;i++)
	{	
		if(FillColorType==0)
			CH6_DisplayPIC(GHandle,VPHandle,progX+i*3,ProgY,YELLOWFILL_PIC,true);	
		else 
			CH6_DisplayPIC(GHandle,VPHandle,progX+i*3,ProgY,BLUEFILL_PIC,true);	
	}
			
	}
	else
	{
		if(PreValue==100&&CurValue<100)
		{
			CH6_DisplayPIC(GHandle,VPHandle,progX+300,StartY,RIGHT_PROG_W,true);
		}
		for(i=CurValue;i<PreValue;i++)
		{	
	
			CH6_DisplayPIC(GHandle,VPHandle,progX+i*3,ProgY,SPROG_PIC,true);	
			
		}
	
	}
	
		if(CurValue==100)
		{ 
			CH6_DisplayPIC(GHandle,VPHandle,progX+300,StartY,(FillColorType==0)?RIGHT_PROG_Y : RIGHT_PROG_B,true);
		}
	}
	return ;
}


#ifndef USE_GFX
/*********************************************************************************
函数功能:在OSD画上下左右等腰三角形
参数:前3为句柄，Point1为顶点坐标，Point2(左与上)、3(右与下)为底边坐标，Color为颜色
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070714 add  
       2007/11/19 修改下三角时候从底到上不包括最底度Y坐标
***********************************************************************************/

boolean CH_DrawTriangle( STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, 
								STGFX_Point_t Point1, STGFX_Point_t Point2, STGFX_Point_t Point3, unsigned int Color )
{
	STGFX_Point_t tP1, tP2, tP3;
	S32			 sPx, sPy;
#ifdef OSD_COLOR_TYPE_RGB16	
	static U16* triangle_data=NULL;
#else 
      static U32* triangle_data=NULL;

#endif
	double tan_trig;

	tP1.X = Point1.X;
	tP1.Y = Point1.Y;
	tP2.X = Point2.X;
	tP2.Y = Point2.Y;
	tP3.X = Point3.X;
	tP3.Y = Point3.Y;
	
	if (tP2.Y == tP3.Y && tP1.Y < tP2.Y)
		{	
#ifdef OSD_COLOR_TYPE_RGB16			
			triangle_data = (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP1.Y, tP3.X-tP2.X, tP2.Y-tP1.Y); 
#else
			triangle_data = (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP1.Y, tP3.X-tP2.X, tP2.Y-tP1.Y); 

#endif
			if(triangle_data == NULL)
			{
			       do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP2.Y-tP1.Y)/(double)(tP3.X-tP2.X);
			for( sPy= tP1.Y; sPy < tP2.Y; sPy ++ )
			{
				for( sPx = tP1.X - (sPy-tP1.Y)/(tan_trig); sPx <= tP1.X + (sPy-tP1.Y)/(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = Color;
	#else
	                           if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = (Color&0x00FFFFFF)|0x80000000;
				      else
					  	triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;

	#endif

				}
			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X,tP1.Y,tP3.X-tP2.X,tP2.Y-tP1.Y,(U8*)triangle_data);                        
		}
	
	if (tP2.Y == tP3.Y && tP1.Y > tP2.Y)
		{
#ifdef OSD_COLOR_TYPE_RGB16					
			triangle_data= (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y);  
#else
			triangle_data= (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y);

#endif
			if(triangle_data == NULL)
			{
			      do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP1.Y-tP2.Y)/(double)(tP3.X-tP2.X);
			for( sPy= tP1.Y-1/*2007117 add -1*/; sPy >= tP2.Y/*2007117 add =*/; sPy -- )
			{
				for( sPx = tP1.X - (tP1.Y - sPy)/(tan_trig); sPx <= tP1.X + (tP1.Y - sPy)/(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;
	#else
		                           if((Color&0xFF000000) != 0)  
						 triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = (Color&0x00FFFFFF)|0x80000000;
					     else
						  triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;


	#endif
					
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y,(U8*)triangle_data);                        
		}
	
	if (tP2.X==tP3.X && tP1.X < tP2.X)
		{	
#ifdef OSD_COLOR_TYPE_RGB16							
			triangle_data= (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y);  
#else
			triangle_data= (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y);  

#endif
			if(triangle_data == NULL)
			{
			      do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP3.X-tP1.X)/(double)(tP3.Y-tP2.Y);
			for( sPy= tP2.Y; sPy < tP3.Y; sPy ++ )
			{
				for( sPx = tP2.X - (sPy <= ((tP3.Y-tP2.Y)/2.0+tP2.Y) ? (sPy - tP2.Y) :(tP3.Y - sPy))*(tan_trig); sPx < tP2.X; sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X]  = Color;
	#else
			                  if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X] = (Color&0x00FFFFFF)|0x80000000;
					    else
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X]  = Color;


	#endif						
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y,(U8*)triangle_data);                        

		}
	
	if(tP2.X==tP3.X && tP1.X > tP2.X)
		{	
#ifdef OSD_COLOR_TYPE_RGB16									
			triangle_data=(U16*) CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y); 
#else
			triangle_data=(U32*) CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y); 

#endif
			if(triangle_data == NULL)
			{
				do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP1.X-tP2.X)/(double)(tP3.Y-tP2.Y);
			for( sPy= tP2.Y; sPy < tP3.Y; sPy ++ )
			{
				for( sPx = tP2.X; sPx <= tP2.X + (sPy <= ((tP3.Y-tP2.Y)/2.0+tP2.Y) ? (sPy - tP2.Y) : (tP3.Y - sPy))*(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = Color;
	#else
				           if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = (Color&0x00FFFFFF)|0x80000000;
					    else						
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = Color;

	#endif						
					
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y,(U8*)triangle_data);                        

		}
	memory_deallocate(CHSysPartition,triangle_data);
	triangle_data=NULL;
	return false;

}
#else

/*********************************************************************************
函数功能:在OSD画上下左右等腰三角形
参数:前3为句柄，Point1为顶点坐标，Point2(左与上)、3(右与下)为底边坐标，Color为颜色
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070714 add
***********************************************************************************/

boolean CH_DrawTriangle( STGFX_Handle_t GHandle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, 
								STGFX_Point_t Point1, STGFX_Point_t Point2, STGFX_Point_t Point3, unsigned int Color )
{
	STGFX_Point_t tP1, tP2, tP3;
	S32			 sPx, sPy;
#ifdef OSD_COLOR_TYPE_RGB16	
	static U16* triangle_data=NULL;
#else 
      static U32* triangle_data=NULL;

#endif
	double tan_trig;

	tP1.X = Point1.X;
	tP1.Y = Point1.Y;
	tP2.X = Point2.X;
	tP2.Y = Point2.Y;
	tP3.X = Point3.X;
	tP3.Y = Point3.Y;
	
	if (tP2.Y == tP3.Y && tP1.Y < tP2.Y)
		{	
#ifdef OSD_COLOR_TYPE_RGB16			
			triangle_data = (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP1.Y, tP3.X-tP2.X, tP2.Y-tP1.Y); 
#else
			triangle_data = (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP1.Y, tP3.X-tP2.X, tP2.Y-tP1.Y); 

#endif
			if(triangle_data == NULL)
			{
			       do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP2.Y-tP1.Y)/(double)(tP3.X-tP2.X);
			for( sPy= tP1.Y; sPy < tP2.Y; sPy ++ )
			{
				for( sPx = tP1.X - (sPy-tP1.Y)/(tan_trig); sPx <= tP1.X + (sPy-tP1.Y)/(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = Color;
	#else
	                           if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = (Color&0x00FFFFFF)|0x80000000;
				      else
					  	triangle_data[(sPy - tP1.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;

	#endif

				}
			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X,tP1.Y,tP3.X-tP2.X,tP2.Y-tP1.Y,(U8*)triangle_data);                        
		}
	
	if (tP2.Y == tP3.Y && tP1.Y > tP2.Y)
		{
#ifdef OSD_COLOR_TYPE_RGB16					
			triangle_data= (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y);  
#else
			triangle_data= (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y);

#endif
			if(triangle_data == NULL)
			{
			      do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP1.Y-tP2.Y)/(double)(tP3.X-tP2.X);
			for( sPy= tP1.Y; sPy > tP2.Y; sPy -- )
			{
				for( sPx = tP1.X - (tP1.Y - sPy)/(tan_trig); sPx <= tP1.X + (tP1.Y - sPy)/(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;
	#else
		                           if((Color&0xFF000000) != 0)  
						 triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X] = (Color&0x00FFFFFF)|0x80000000;
					     else
						  triangle_data[(sPy - tP2.Y)*(tP3.X - tP2.X) + sPx -tP2.X]  = Color;


	#endif
					
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X, tP2.Y, tP3.X-tP2.X, tP1.Y-tP2.Y,(U8*)triangle_data);                        
		}
	
	if (tP2.X==tP3.X && tP1.X < tP2.X)
		{	
#ifdef OSD_COLOR_TYPE_RGB16							
			triangle_data= (U16*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y);  
#else
			triangle_data= (U32*)CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y);  

#endif
			if(triangle_data == NULL)
			{
			      do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP3.X-tP1.X)/(double)(tP3.Y-tP2.Y);
			for( sPy= tP2.Y; sPy < tP3.Y; sPy ++ )
			{
				for( sPx = tP2.X - (sPy <= ((tP3.Y-tP2.Y)/2.0+tP2.Y) ? (sPy - tP2.Y) :(tP3.Y - sPy))*(tan_trig); sPx < tP2.X; sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X]  = Color;
	#else
			                  if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X] = (Color&0x00FFFFFF)|0x80000000;
					    else
						triangle_data[(sPy - tP2.Y)*(tP3.X - tP1.X) + sPx -tP1.X]  = Color;


	#endif						
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP1.X, tP2.Y, tP2.X-tP1.X, tP3.Y-tP2.Y,(U8*)triangle_data);                        

		}
	
	if(tP2.X==tP3.X && tP1.X > tP2.X)
		{	
#ifdef OSD_COLOR_TYPE_RGB16									
			triangle_data=(U16*) CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y); 
#else
			triangle_data=(U32*) CH6_GetRectangle( GHandle, VPHandle,pGC,
											 tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y); 

#endif
			if(triangle_data == NULL)
			{
				do_report(0,"CH_DrawTriangle  memory is samll\n");
				return true;
			}
			tan_trig=2.0*(double)(tP1.X-tP2.X)/(double)(tP3.Y-tP2.Y);
			for( sPy= tP2.Y; sPy < tP3.Y; sPy ++ )
			{
				for( sPx = tP2.X; sPx <= tP2.X + (sPy <= ((tP3.Y-tP2.Y)/2.0+tP2.Y) ? (sPy - tP2.Y) : (tP3.Y - sPy))*(tan_trig); sPx ++  )
					{
	#ifdef 		OSD_COLOR_TYPE_RGB16		
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = Color;
	#else
				           if((Color&0xFF000000) != 0)  
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = (Color&0x00FFFFFF)|0x80000000;
					    else						
						triangle_data[(sPy - tP2.Y)*(tP1.X - tP3.X) + sPx  - tP2.X] = Color;

	#endif						
					
					}

			}
			CH6_PutRectangle(GHandle, VPHandle,pGC,tP2.X, tP2.Y, tP1.X-tP2.X, tP3.Y-tP2.Y,(U8*)triangle_data);                        

		}
	memory_deallocate(CHSysPartition,triangle_data);
	triangle_data=NULL;
	return false;

}

/****************************************************************
函数功能:在OSD画上下左右等腰三角形
参数:前3为句柄，Point1为顶点坐标，Point2、3为底边坐标，Color为颜色
返回值:0代表操作正常，1代表操作失败
备注:cqj 20070714 add
******************************************************************/

boolean DrawTriangle(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,STGFX_Point_t Point1,STGFX_Point_t Point2,STGFX_Point_t Point3,unsigned int Color)
{
	CH_DrawTriangle(  GHandle,  VPHandle, pGC, Point1,  Point2,  Point3,  Color );
	return true;
}



#endif
/*得到在指定区域给定字符串的显示行数*/
/*
int width,int height,显示区域
char *DisplayTxt,需要显示的字符串
char *DisplayLinBuf,显示输出的行Buffer
int  OnLineNum,显示输出的行Buffer每行的最大字符数
int  MaxLine,  显示输出的行Buffer最大行数,
int *OutLineNum,当前显示的行数
输出：显示Buff的位置
*/
int CH_GetCharLine(int width,int height,char *DisplayTxt,char *DisplayLinBuf,int OnLineNum,int MaxLine,int *OutLineNum,boolean MulPage)
{
    int OneLinecharNum;
	int TotalcharNum;
	int CurLine=0;
	int i;
	int Total_height=0;
    boolean ForceChangeLine;
	boolean FirstChineseByte=false;
    int TextPos=0;

	if(DisplayTxt==NULL)
	{
		return 0;
	}

	TotalcharNum=strlen(DisplayTxt);                 /*当前字符总个数*/
	
	OneLinecharNum=width/CHARACTER_WIDTH;            /*一行能够显示的最大字符数*/
	while(*DisplayTxt!='\0')
	{
		ForceChangeLine=false;

		FirstChineseByte=false;

		for(i=0;i<OneLinecharNum;i++)
		{
			if(DisplayTxt[i]=='\r'||DisplayTxt[i]=='\n'||DisplayTxt[i]=='\0')/*硬换行*/
			{
				ForceChangeLine=true;
				
				break;
			}
			DisplayLinBuf[CurLine*OnLineNum+i]=DisplayTxt[i];
			if((DisplayTxt[i]&0x80)!=0)
			{					
				FirstChineseByte=!FirstChineseByte;
			}
			else if(FirstChineseByte==true)
			{
                FirstChineseByte=false;
			}
		}

		if(ForceChangeLine==true)/*硬回车,直接换行，不作其他处理 */
		{
			DisplayLinBuf[CurLine*OnLineNum+i]='\0';

			if(DisplayTxt[i]=='\r')
			{
              i++;
              if(DisplayTxt[i]=='\n')/*跳过换行符*/
			  {
				  i++;
			  }
			}
			else if(DisplayTxt[i]=='\n')/*跳过换行符*/
			{
				i++;
			}
		}
		else                    /*软换行*/
		{
			if(i>=2)
			{

				 if(FirstChineseByte==true)/*换行位置为汉半个字,半个汉字移到下一行显示*/
				{
					i-=1;
					DisplayLinBuf[CurLine*OnLineNum+i]='\0';
				}
				else if((DisplayTxt[i-1]&0x80)!=0&&(DisplayTxt[i-2]&0x80)!=0)/*换行位置一个为汉字*/
				{
					DisplayLinBuf[CurLine*OnLineNum+i]='\0';
					
					if(DisplayTxt[i]=='\r')                     
					{
						i++;
						if(DisplayTxt[i]=='\n')/*跳过换行符*/
						{
							i++;
						}
					}
					else if(DisplayTxt[i]=='\n'||DisplayTxt[i]==' ')/*跳过下一行换行符或者下一行的空格*/
					{
						i++;
					}
					
				}
				else                                                /*换行位置为英文,考虑英文单词完整性*/
				{
		
					if(DisplayTxt[i] ==' '||DisplayTxt[i] == ','||
						DisplayTxt[i] == '.'|| DisplayTxt[i]=='\r' ||DisplayTxt[i] == '\n'||
						DisplayTxt[i] == '\0'||(DisplayTxt[i]&0x80)!=0)/*换行位置下一个字符为单词分界*/
					{
						DisplayLinBuf[CurLine*OnLineNum+i]='\0';

						if(DisplayTxt[i]=='\r')
						{
							i++;
							if(DisplayTxt[i]=='\n')/*跳过换行符*/
							{
								i++;
							}
						}
						else if(DisplayTxt[i]=='\n'||DisplayTxt[i]==' ')/*跳过下一行换行符或者下一行的空格*/
						{
							i++;
						}
						
					}
					else if(DisplayTxt[i-1] ==' '||DisplayTxt[i-1] == ','||
						DisplayTxt[i-1] == '.'|| DisplayTxt[i-1] == '\n'||
						DisplayTxt[i-1] == '\0')/*换行位置为单词分界*/
					{
						DisplayLinBuf[CurLine*OnLineNum+i]='\0';
					}
					else/*找到前一个单词分界点*/
					{
						if(/*Total_height+20+CHARACTER_WIDTH*2<=height&&*/CurLine<MaxLine)
						{
							if(MulPage==true||((MulPage==false)&&Total_height+20+CHARACTER_WIDTH*2<=height))
							{
								int temp=i;
								while(i>0)
								{
									if( DisplayTxt[i] ==' '||DisplayTxt[i] ==','||
										DisplayTxt[i] =='.'|| DisplayTxt[i]=='\n'||DisplayTxt[i]=='\r'||
										DisplayTxt[i] =='\0'||(DisplayTxt[i]&0x80)!=0)
									{
										break;
									}
									i--;
								}
								
								if(i==0)
								{
									i=temp;
								}
								else
								{
									i+=1;/*这个换行标记在当前行处理掉*/
								}
								DisplayLinBuf[CurLine*OnLineNum+i]='\0';
							}
							else
							{
								DisplayLinBuf[CurLine*OnLineNum+i]='\0';
							}

						}
						else
						{
                           DisplayLinBuf[CurLine*OnLineNum+i]='\0';
						}
					}
				}
			}
			else
			{
				DisplayLinBuf[CurLine*OnLineNum+i]='\0';
			}
		}
		DisplayTxt+=i;

		TextPos+=i;
        CurLine++;
		Total_height+=CHARACTER_WIDTH*2;
		/*如果下一行显示位置不够，不显示下一行信息*/
		if(Total_height+20>height||CurLine>=MaxLine)
		{
			break;
		}
	}
	*OutLineNum=CurLine;

	return TextPos;
}
/*显示多行文本信息，考虑英文换行*/
void CH6_MulLine_ChangeLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
							int x_top,int y_top,int Width,int Heigth,char *TxtStr,
							unsigned int txtColor,U8 H_Align,U8 V_Align,boolean MulLine_ALine)
{
	int LineNum;
	char AllIine[20][100];
	int iTextXPos,iTextYPos;
	int TextLen;
	int TextPos;
	boolean IsDisplayAll;
	int i;
	int TextNum;
	STGFX_Handle_t GFXHandleTemp=GHandle; 
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; 
	TextLen=CH6_GetTextWidth(TxtStr);
	
	x_top=x_top+6;
	Width=Width-12;
	
	if(TxtStr==NULL)
		return;
	TextNum=strlen(TxtStr);


	TextPos=CH_GetCharLine(Width,Heigth,TxtStr,(char *)&AllIine[0][0],100,20,&LineNum,false);

    if(TextPos>=TextNum)
	{
       IsDisplayAll=true;
	}
	else
	{
       IsDisplayAll=false;
	}
	
	iTextXPos=x_top;
	iTextYPos=y_top;
#if 1
	if(LineNum<=1&&IsDisplayAll==true)/*只有一行显示*/
	{
		switch(H_Align)
		{
		case 1:/*居左*/
			break;
		case 2:/*居中*/
			iTextXPos=x_top+(Width-TextLen)/2;				
			break;
		case 3:/*居右*/
			iTextXPos=x_top+(Width-TextLen);				
			break;
		default:
			break;	
		}	
	}
#endif
	switch(V_Align)
	{
	case 1:
		
		break;
	case 2:	
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum)/2;
		break;
	case 3:
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum);
		break;
	default:
		break;
		
	}	
	
	for(i=0;i<LineNum;i++)
	{
		if(MulLine_ALine==true)
		{
			TextLen=CH6_GetTextWidth(AllIine[i]);
			switch(H_Align)
			{
			case 1:/*居左*/
				break;
			case 2:/*居中*/
				iTextXPos=x_top+(Width-TextLen)/2;				
				break;
			case 3:/*居右*/
				iTextXPos=x_top+(Width-TextLen);				
				break;
			default:
				break;	
			}
		}
		if(IsDisplayAll==false&&i==(LineNum-1))/*需要显三个点完毕表示没有显示,*/
		{
            int TempLen=strlen(AllIine[i]);
			int TempTxtLen=CH6_GetTextWidth(AllIine[i]);

			if(iTextXPos+TempTxtLen+CHARACTER_WIDTH*2>=x_top+Width)
			{
				AllIine[i][TempLen-1]=AllIine[i][TempLen-2]='\0';
				TextPos-=2;
                


				CH6_DrawPoint(iTextXPos+TempTxtLen-CHARACTER_WIDTH*2,iTextYPos,txtColor);
			}
			else
			{
				CH6_DrawPoint(iTextXPos+TempTxtLen,iTextYPos,txtColor);
			}
		}

		CH6_EnTextOut(GFXHandleTemp,VPHandleTemp,&gGC,iTextXPos,iTextYPos,txtColor,AllIine[i]);

		iTextYPos+=CHARACTER_WIDTH*2;
	}

}
/**/
int CH6_MulLine_ChangeLine1(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
							int x_top,int y_top,int Width,int Heigth,char *TxtStr,
							unsigned int  txtColor,U8 H_Align,U8 V_Align,boolean MulLine_ALine,int LastLineWidth, int *OutLineNum,int DisplayPageEofTag)
{
	int LineNum;
	char AllIine[20][100];
	char TempLastLine[100];
	int LastTexPos;
	int iTextXPos,iTextYPos;
	int TextLen;
	int TextPos;
	boolean IsDisplayAll;
	int i;
	int TextNum;
	STGFX_Handle_t GFXHandleTemp=GHandle; 
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; 
	TextLen=CH6_GetTextWidth(TxtStr);
	
	x_top=x_top+6;
	Width=Width-12;
	LastLineWidth=LastLineWidth-12;

	if(TxtStr==NULL)
		return 0;
	
	TextNum=strlen(TxtStr);

/*	do_report(0,"\n%s\n",TxtStr);*/

	TextPos=CH_GetCharLine(Width,Heigth,TxtStr,(char *)&AllIine[0][0],100,20,&LineNum,true);
	
    if(TextPos>=TextNum)
	{
		IsDisplayAll=true;
	}
	else
	{
		IsDisplayAll=false;
	}
	
	iTextXPos=x_top;
	iTextYPos=y_top;
#if 1
	if(LineNum<=1&&IsDisplayAll==true)/*只有一行显示*/
	{
		switch(H_Align)
		{
		case 1:/*居左*/
			break;
		case 2:/*居中*/
			iTextXPos=x_top+(Width-TextLen)/2;				
			break;
		case 3:/*居右*/
			iTextXPos=x_top+(Width-TextLen);				
			break;
		default:
			break;	
		}	
	}
#endif	
	switch(V_Align)
	{
	case 1:
		
		break;
	case 2:	
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum)/2;
		break;
	case 3:
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum);
		break;
	default:
		break;
		
	}	
	
	for(i=0;i<LineNum;i++)
	{
		if(MulLine_ALine==true)
		{
			TextLen=CH6_GetTextWidth(AllIine[i]);
			switch(H_Align)
			{
			case 1:/*居左*/
				break;
			case 2:/*居中*/
				iTextXPos=x_top+(Width-TextLen)/2;				
				break;
			case 3:/*居右*/
				iTextXPos=x_top+(Width-TextLen);				
				break;
			default:
				break;	
			}
		}
		if(IsDisplayAll==false&&i==(LineNum-1))
		{
			
            int TempLen=strlen(AllIine[i]);
			int TempTxtLen=CH6_GetTextWidth(AllIine[i]);

			if(DisplayPageEofTag==0)/*需要显三个点完毕表示没有显示,*/
			{
				if(TempTxtLen+CHARACTER_WIDTH*2>=LastLineWidth)/*如果最后一行无法显示完*/
				{
					
					int TempLine;

					LastTexPos=CH_GetCharLine(LastLineWidth,24,AllIine[i],TempLastLine,100,20,&TempLine,true);
					
					TextPos=TextPos-(TempLen-LastTexPos);
					
					memcpy(AllIine[i],TempLastLine,LastTexPos);

					AllIine[i][LastTexPos]='\0';
					
					TempLen=strlen(AllIine[i]);

					TempTxtLen=CH6_GetTextWidth(AllIine[i]);

                   	if(TempTxtLen+CHARACTER_WIDTH*2>=LastLineWidth)
					{
						AllIine[i][TempLen-1]=AllIine[i][TempLen-2]='\0';
						TextPos-=2;
						CH6_DrawPoint(iTextXPos+TempTxtLen-CHARACTER_WIDTH*2,iTextYPos,txtColor);
					}
					else
					{
						CH6_DrawPoint(iTextXPos+TempTxtLen,iTextYPos,txtColor);
					}
					
				}
				else
				  CH6_DrawPoint(iTextXPos+TempTxtLen,iTextYPos,txtColor);
				

			}
			else if(DisplayPageEofTag==1)/*显示下箭头，表示有下一页*/
			{
                if(TempTxtLen+CHARACTER_WIDTH*2>=LastLineWidth)
				{
					int TempLine;
					LastTexPos=CH_GetCharLine(LastLineWidth-CHARACTER_WIDTH*2,24,AllIine[i],TempLastLine,100,20,&TempLine,true);
					
					TextPos=TextPos-(TempLen-LastTexPos);
					
					memcpy(AllIine[i],TempLastLine,LastTexPos);
					AllIine[i][LastTexPos]='\0';

					TempLen=strlen(AllIine[i]);
					TempTxtLen=CH6_GetTextWidth(AllIine[i]);

                   	if(TempTxtLen+CHARACTER_WIDTH*2>=LastLineWidth)
					{
						AllIine[i][TempLen-1]=AllIine[i][TempLen-2]='\0';
						TextPos-=2;
						CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,iTextXPos+TempTxtLen-CHARACTER_WIDTH*2,iTextYPos+6,DOWNARRF_PIC,true);
						
					}
					else
					{
						CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,iTextXPos+TempTxtLen,iTextYPos+6,DOWNARRF_PIC,true);

					}
				}
				else
				
					CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,iTextXPos+TempTxtLen,iTextYPos+6,DOWNARRF_PIC,true);
					
				
			}
			else/*不处理*/
			{
                ;
			}

		}

		CH6_EnTextOut(GFXHandleTemp,VPHandleTemp,&gGC,iTextXPos,iTextYPos,txtColor,AllIine[i]);

		iTextYPos+=24;
	}
	*OutLineNum=LineNum;
	return TextPos;
}



void CH6_Displayscoll(INT Scoll_X,INT Scoll_Y,INT Scoll_W,INT Scoll_H,unsigned int  pColor)
{
	int  Radiu;
	int circle_x1,circle_x2;
	int circle_y1,circle_y2;
	
	Radiu=Scoll_W/2;
	circle_x1=Scoll_X+Radiu;
	circle_y1=Scoll_Y;
	circle_x2=circle_x1;
	circle_y2=Scoll_Y+Scoll_H;

	CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Scoll_X,
								Scoll_Y,Scoll_W,Scoll_H,Radiu,pColor,pColor,1);
   	
}

#ifndef USE_GFX

/*三角形上*/
boolean DrawTrigon1(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y,unsigned   int  color/*填充色*/,unsigned int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{
	ST_ErrorCode_t    error;
	
	STGFX_Point_t   Points_p[3];
	STGXOBJ_Bitmap_t DstBMP;
	STGFX_GC_t  *RGC_P;
	
	RGC_P=&gGC;
	if(color!=-1)
		RGC_P->EnableFilling= TRUE;
	else
		RGC_P->EnableFilling= false;
	
	RGC_P->LineWidth= LWidth;
	
	Points_p[0].X=x;
	Points_p[0].Y=y;
	
	
	Points_p[1].X=x+5; 
	   Points_p[1].Y=y+10;
	   Points_p[2].X=x-5;
	   Points_p[2].Y=y+10;
	   
	   if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	   {
		   return false;
	   }	
	   
       RGC_P->FillColor.Type=DstBMP.ColorType;
	   RGC_P->DrawColor.Type=DstBMP.ColorType;
	   if(CH6_SetColorValue(&(RGC_P->FillColor),color)) return false;
	   if(CH6_SetColorValue(&(RGC_P->DrawColor),Lcolor)) return false;
	   
	   
	   error= STGFX_DrawPolygon(GHandle,&DstBMP,RGC_P,3,&Points_p[0]); 
	   

	   
	   error=STGFX_Sync(GHandle);
	   if(error==ST_NO_ERROR) return  true;
	   
	   return false;	 
}



/*三角形下*/
boolean DrawTrigon2(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y, unsigned  int  color/*填充色*/,unsigned int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{
      ST_ErrorCode_t    error;

      STGFX_Point_t   Points_p[3];
	STGXOBJ_Bitmap_t DstBMP;
      STGFX_GC_t  *RGC_P;
	  
      RGC_P=&gGC;
	  if(color!=-1)
                    RGC_P->EnableFilling= TRUE;
	  else
	  	      RGC_P->EnableFilling= false;
         
                    RGC_P->LineWidth= LWidth;

      	Points_p[0].X=x;
       Points_p[0].Y=y;

	  Points_p[1].X=x+5;
	    Points_p[1].Y=y-10;
	    Points_p[2].X=x-5;
	    Points_p[2].Y=y-10;

 	if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	{
		return false;
	}	

       RGC_P->FillColor.Type=DstBMP.ColorType;
	RGC_P->DrawColor.Type=DstBMP.ColorType;
	if(CH6_SetColorValue(&(RGC_P->FillColor),color)) return false;
	if(CH6_SetColorValue(&(RGC_P->DrawColor),Lcolor)) return false;
	
	
     error= STGFX_DrawPolygon(GHandle,&DstBMP,RGC_P,3,&Points_p[0]); 



	 
	 error=STGFX_Sync(GHandle);
 	 if(error==ST_NO_ERROR) return  true;
	 
	 	return false;
	  

}
#else
boolean DrawTrigon1(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y,  unsigned int  color/*填充色*/,int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{
	STGFX_Point_t   Points_p[3];
	
	Points_p[0].X=x;
	Points_p[0].Y=y;
	
	Points_p[1].X=x+5; 
	Points_p[1].Y=y+10;
	
	Points_p[2].X=x-5;
	Points_p[2].Y=y+10;

	CH_DrawTriangle(  GHandle,  VPHandle, &gGC, Points_p[0],  Points_p[2],  Points_p[1],  color );
}

boolean DrawTrigon2(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y, unsigned int  color/*填充色*/,unsigned int  Lcolor/*边线色*/,U16 LWidth/*边线宽*/)
{

	STGFX_Point_t   Points_p[3];

	Points_p[0].X=x;
	Points_p[0].Y=y;

	Points_p[1].X=x+5;
	Points_p[1].Y=y-10;

	Points_p[2].X=x-5;
	Points_p[2].Y=y-10;

	CH_DrawTriangle(  GHandle,  VPHandle, &gGC, Points_p[0],  Points_p[2],  Points_p[1],  color );
}

#endif
/*垂直滚动条*/
void CH_DrawScoll(INT Scoll_X,INT Scoll_Y,INT  CurItem, int Scoll_H/*滚动条高度*/, INT TotalItem/*总的条目数*/)
{
	
	int Slider_Y;
	int Slider_X;
	int Slider_W;
	int Scoll_W=14;
	int Slider_H;

	int  Percent;		
	Slider_X=Scoll_X+3;
	Slider_W=Scoll_W-6;
	if(TotalItem<1||CurItem>=TotalItem)      /*判断错误情况*/
	{
      return;
	}
	Percent=100*(CurItem+1)/TotalItem;         /*显示比例*/

	Slider_H=(Scoll_H-40)/TotalItem+15;    /*滑动条高度*/

	if(Slider_H>=(Scoll_H-40))            
		Slider_H=Scoll_H-40;                     

	Slider_Y=Scoll_Y+20+(Scoll_H-40)*Percent/100-Slider_H;
	
	if(Slider_Y<Scoll_Y+20)
     Slider_Y=Scoll_Y+20;
	
	CH6_Displayscoll(Scoll_X,Scoll_Y,Scoll_W,Scoll_H,SCROLL_FILL_COLOR);
	
	DrawTrigon1(GFXHandle,CH6VPOSD.ViewPortHandle,Scoll_X+7, Scoll_Y+3, SCROLL_SLIDER_COLOR,SCROLL_SLIDER_COLOR,1);
	
	DrawTrigon2(GFXHandle,CH6VPOSD.ViewPortHandle,Scoll_X+7, Scoll_Y+Scoll_H-3, SCROLL_SLIDER_COLOR,SCROLL_SLIDER_COLOR,1);
	
	CH6_Displayscoll(Slider_X,Slider_Y,Slider_W,Slider_H,SCROLL_SLIDER_COLOR);
	
}

/*20051115  wz  add  for */
/*显示左三角*/
boolean DrawLeftTrigon(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y,unsigned  int  color/*填充色*/,int width)
{
	ST_ErrorCode_t    error;
	
	STGFX_Point_t   Points_p[3];
	STGXOBJ_Bitmap_t DstBMP;
	STGFX_GC_t  *RGC_P;
	
	RGC_P=&gGC;
	if(color!=-1)
		RGC_P->EnableFilling= TRUE;
	else
		RGC_P->EnableFilling= false;
	
	RGC_P->LineWidth= 1;
	
	Points_p[0].X=x;
	Points_p[0].Y=y;
	
	
	   Points_p[1].X=x+width; 
	   Points_p[1].Y=y-width;
	   Points_p[2].X=x+width;
	   Points_p[2].Y=y+width;
 #ifdef USE_GFX/*1*//* cqj 20070725 modify*/
    CH_DrawTriangle(  GHandle,  VPHandle, RGC_P, Points_p[0],  Points_p[1],  Points_p[2],  color );
	  
 #else
	   if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	   {
		   return false;
	   }	
	   
       RGC_P->FillColor.Type=DstBMP.ColorType;
	   RGC_P->DrawColor.Type=DstBMP.ColorType;
	   if(CH6_SetColorValue(&(RGC_P->FillColor),color)) 
		   return false;
	   if(CH6_SetColorValue(&(RGC_P->DrawColor),color))
		   return false;
	   
       
	   error= STGFX_DrawPolygon(GHandle,&DstBMP,RGC_P,3,&Points_p[0]); 
	   

	   
	   error=STGFX_Sync(GHandle);
	   if(error==ST_NO_ERROR) return  true;
#endif       
	   return false;	 
}


/*显示右三角*/
boolean DrawRightTrigon(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 x, U32 y, unsigned  int  color/*填充色*/,int width)
{
	ST_ErrorCode_t    error;
	
	STGFX_Point_t   Points_p[3];
	STGXOBJ_Bitmap_t DstBMP;
	STGFX_GC_t  *RGC_P;
	
	RGC_P=&gGC;
	if(color!=-1)
		RGC_P->EnableFilling= TRUE;
	else
		RGC_P->EnableFilling= false;
	
	RGC_P->LineWidth= 1;
	
	Points_p[0].X=x;
	Points_p[0].Y=y;
	
	
	   Points_p[1].X=x-width; 
	   Points_p[1].Y=y-width;
	   Points_p[2].X=x-width;
	   Points_p[2].Y=y+width;
#ifdef USE_GFX  /* cqj 20070725 modify*/

    CH_DrawTriangle(  GHandle,  VPHandle, RGC_P, Points_p[0],  Points_p[1],  Points_p[2],  color );
	   
#else
	   if(CH6_GetBitmapInfo(VPHandle,&DstBMP))
	   {
		   return false;
	   }	
	   
       RGC_P->FillColor.Type=DstBMP.ColorType;
	   RGC_P->DrawColor.Type=DstBMP.ColorType;
	   if(CH6_SetColorValue(&(RGC_P->FillColor),color)) 
		   return false;
	   if(CH6_SetColorValue(&(RGC_P->DrawColor),color))
		   return false;
	   
	   
	   error= STGFX_DrawPolygon(GHandle,&DstBMP,RGC_P,3,&Points_p[0]); 
	   

	   
	   error=STGFX_Sync(GHandle);
	   if(error==ST_NO_ERROR) return  true;
	   #endif	   
	   return false;	 
}






void CH_DisplaySelectItem_bj(U32 XStart,U32 YStart,U16 ItemWidth,char *SelectName)
{

       CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
								 XStart,YStart,ItemWidth,AVSET_ITEM_HEIGH,
								 5,SYSTEM_OUTER_FRAM_COLOR/*填充色*/,
								SYSTEM_OUTER_FRAM_COLOR/*边线色*/,
								1);

       CH6_DisRectTxtInfo(GFXHandle,
					CH6VPOSD.ViewPortHandle,
					XStart+5,YStart,ItemWidth-10,AVSET_ITEM_HEIGH,
					SelectName,SYSTEM_OUTER_FRAM_COLOR,/*背景色*/
					whitecolor,/*字体色*/
					1,1);

}
void CH_DisplayLeftRightItem_Select_bj(U32 XStart,U32 YStart,U16 ItemWidth,char *SelectName)
{

        CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
								 XStart,YStart,ItemWidth,AVSET_ITEM_HEIGH,
								 5,SYSTEM_OUTER_FRAM_COLOR/*填充色*/,
								SYSTEM_OUTER_FRAM_COLOR/*边线色*/,
								1);

        CH6_DisRectTxtInfoNobck(GFXHandle,CH6VPOSD.ViewPortHandle, 
		 	XStart+15,YStart,ItemWidth-30,AVSET_ITEM_HEIGH,
		 	SelectName,whitecolor,1);

	/*显示左右两个三角形*/
     
        DrawLeftTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+5,YStart+15, orange_color,10);
	 DrawRightTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+ItemWidth-10/*5*/,YStart+15, orange_color,10);
     

}


/*20061222 add 显示输出为灰色不可选中状态*/
void CH_DisplayLeftRightItem_SelectUnused_bj(U32 XStart,U32 YStart,U16 ItemWidth,char *SelectName)
{

        CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
								 XStart,YStart,ItemWidth,AVSET_ITEM_HEIGH,
								 5,SYSTEM_OUTER_FRAM_COLOR/*填充色*/,
								SYSTEM_OUTER_FRAM_COLOR/*边线色*/,
								1);

        CH6_DisRectTxtInfoNobck(GFXHandle,CH6VPOSD.ViewPortHandle, 
		 	XStart+15,YStart,ItemWidth-30,AVSET_ITEM_HEIGH,
		 	SelectName,0xF888,1);

	/*显示左右两个三角形*/
     /*
        DrawLeftTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+5,YStart+15, orange_color,10);
	 DrawRightTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+ItemWidth-5,YStart+15, orange_color,10);
     */

}


void CH_DisplayUnSelectItem_bj(U32 XStart, U32 YStart, U16 ItemWidth, char * SelectName)
{
         CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
								 XStart,YStart,ItemWidth,AVSET_ITEM_HEIGH,
								 5,RECT_FILL_COLOR/*填充色*/,
								RECT_FILL_COLOR/*边线色*/,
								1);
         CH6_DisRectTxtInfoNobck(GFXHandle,CH6VPOSD.ViewPortHandle,
		 	XStart+5,YStart,ItemWidth-10,AVSET_ITEM_HEIGH,
		 	SelectName,TEXT_COLOR,1);
        
}
void CH_DisplayLeftRightItem_Normal_bj(U32 XStart, U32 YStart, U16 ItemWidth, char * SelectName)
{
                   CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
								 XStart,YStart,ItemWidth,AVSET_ITEM_HEIGH,
								 5,RECT_FILL_COLOR/*填充色*/,
								RECT_FILL_COLOR/*边线色*/,
								1);

			CH6_DisRectTxtInfoNobck(GFXHandle,CH6VPOSD.ViewPortHandle, 
		 	XStart+15,YStart,ItemWidth-30,AVSET_ITEM_HEIGH,
		 	SelectName,TEXT_COLOR,1);

		
        DrawLeftTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+5,YStart+15, RECT_FILL_COLOR,10);
	 DrawRightTrigon(GFXHandle,CH6VPOSD.ViewPortHandle,XStart+ItemWidth-10/*5*/,YStart+15, RECT_FILL_COLOR,10);
     


			
}

/* Jqz050323 add */
/*
  * Func: CH6_FindPageCutSize
  * Desc: 寻找换页时的切点
  * In:	uBuff			-> 字buff的指针
  		uPageLength		-> 一页中能显示多少个字符?(英文)
  		uCutPoint			-> 切点的指针
  		uMaxPage		-> 最大页数
  * Out:	分几页
  */
U8	CH6_FindPageCutSize( U8* uBuff, U16 uPageLength, U16* pCutPoint, U8 uMaxPage )
{
	U8	uPageCount, uLoop;
	U32	uBuffLength;
	U32	uTmpLength, uPageLen;
	U8	uCharLen;

	/* 错误判断 */
	if( uPageLength == 0 || uMaxPage < 2 )
		return 0;

	/* 得到字缓冲大小 */
	uBuffLength = strlen( (char*)uBuff );

	/* 得到临时页数 */
	uLoop = uBuffLength / uPageLength;
	
	if( uBuffLength % uPageLength > 0 )
		uLoop += 1;

	for( uPageCount = 0, uTmpLength = 0, uPageLen = 0; uTmpLength < uBuffLength; )
	{
		/* 如果是汉字 */
		if( uBuff[uTmpLength] >= 0x80 )
		{
			uTmpLength 	+= 2;
			uPageLen 	+= 2;

			/* 大于一页的字符数了 */
			if( uPageLen > uPageLength )
			{
				pCutPoint[uPageCount] 	= ( uTmpLength - 2 );
				uPageLen					= 0;
				uTmpLength				-= 2;

				uPageCount ++;
			}
		}
		else	/* 如果是英语 */
		{
			uTmpLength ++;
			uPageLen ++;
			
			/* 大于一页的字符数了 */
			if( uPageLen > uPageLength )
			{
				pCutPoint[uPageCount] 	= ( uTmpLength - 1 );
				uPageLen					= 0;
				uTmpLength				-= 1;

				uPageCount ++;
			}
		}

		if( uPageCount >= uLoop || uPageCount >= uMaxPage )
		{
			break;
		}
	}

	/* 返回实际页数 */
	return uPageCount + 1;
}
/* End Jqz050323 add */

/*--eof-------------------------------------------------------------------------*/


