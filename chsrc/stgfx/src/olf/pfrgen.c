#include "pfrgen.h"

void LoadExtraItem(U8** Src_p,ExtraItem_t* Target_p,U8 RecType)
{
  U8 NumExtraItems;
  U8 ExtraItemSize;
  U8 ExtraItemType;
  U32 i;

  NumExtraItems=GetBytes(Src_p,1,0);
  for (i=0;i<NumExtraItems;i++) {
    ExtraItemSize=GetBytes(Src_p,1,0);
    ExtraItemType=GetBytes(Src_p,1,0);
    SkipBytes(Src_p,ExtraItemSize);
  }
}

void FreeExtraItem(ExtraItem_t* Target_p) {
} 

S32 GetBytes(U8** Source_p, S32 NumBytes, BOOL SgnFlg) {
  U32 i, Result=0;
  U8* pByte=(U8*)&Result;
  for (i=0;i<NumBytes;i++) *(pByte++) = *(*Source_p+NumBytes-1-i);
  if ((SgnFlg) && (Result & (0x80 << 8*(NumBytes-1)))){
    Result |= ((0xffffffff >> 8*NumBytes) << (8*NumBytes));
  }
  *Source_p+=NumBytes;
  return Result;
}
