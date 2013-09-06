#ifndef PFRGEN_H
#define PFRGEN_H

#include "stddefs.h"

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)
#define ABS(a) (((a)<0) ? -(a) : (a))
#define SGN(a) (((a)<0) ? -1 : 1)

enum XItemTypes {DUMMY};
typedef void ExtraItem_t;

void LoadExtraItem(U8**,ExtraItem_t*,U8);
void FreeExtraItem(ExtraItem_t*);

int GetBytes(U8**,S32,BOOL);
/* static __inline void SkipBytes(U8** p,S32 s){*p+=s;}
  - generates compile warnings in files that include but don't use */
#define SkipBytes(p, s){*p+=s;}

#endif /* PFRGEN_H */
