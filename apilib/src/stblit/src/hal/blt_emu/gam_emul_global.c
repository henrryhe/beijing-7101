#include "gam_emul_global.h"

/* Various parameters */
/*char *gam_emul_BLTMemory = (char*)0xC0000000; *//* Shared Memory Base Address */
char *gam_emul_BLTMemory ;
int gam_emul_BLTMemSize = 0xFFFFFFFF;


int gam_emul_RescalerCurrentState = 0;

int *gam_emul_HFilter,*gam_emul_VFilter,*gam_emul_Delay;
int *gam_emul_VF_Fifo1,*gam_emul_VF_Fifo2,*gam_emul_VF_Fifo3,*gam_emul_VF_Fifo4,*gam_emul_VF_Fifo5,*gam_emul_VF_Output;
int *gam_emul_FF_Fifo1,*gam_emul_FF_Fifo2,*gam_emul_FF_Fifo3,*gam_emul_FF_Output;
int gam_emul_VF_LineNumber,gam_emul_VF_Counter,gam_emul_VF_Subposition,gam_emul_VF_LineAvailable,gam_emul_NbNewLines;
int gam_emul_HF_Counter,gam_emul_HF_Subposition,gam_emul_NbPix;
int gam_emul_FF_Flag,gam_emul_NumberOfOutputLines;
int gam_emul_FF0[3],gam_emul_FF1[3],gam_emul_FF2[3],gam_emul_FF3[3];
int gam_emul_FFThreshold01,gam_emul_FFThreshold12,gam_emul_FFThreshold23;

/* current node */
gam_emul_BLTNode CurrentBLTNode;
