/*kernel 通用函数定义、宏定义、数据类型*/

#ifndef  _COMMON_H_
#define  _COMMON_H_
/*变量定义约定: 见 长虹网络公司《C语言代码开发基本规范 V2.0》
 注释不要用'//' ,因为有些编译器不认识。
***********************************************************************/


#include "os_plat.h"

/*typedef S32 CH_BOOL;

#define CH_TRUE 1
#define CH_FALSE 0*/


/***********************************************************************
** MACROS:      CH_BIT
**              CH_BITMASK
** DESCRIPTION:
** Bit masking macros.  XXX n must be <= 31 to be portable
***********************************************************************/
#define CH_BIT(n)       ((U32)1 << (n))
#define CH_BITMASK(n)   (CH_BIT(n) - 1)

#define CH_BEGIN_MACRO  do {
#define CH_END_MACRO    } while (0)

#define CH_ARRAY_LENGTH(array_)   (sizeof(array_)/sizeof(array_[0]))

/* 根据结构成员的地址返回结构的地址 */
#define STRUCT_ENTRY(pMemberPtr,STRUCT_TYPE,MemberVar)\
 	((STRUCT_TYPE *)((unsigned long)(pMemberPtr)-(unsigned long)(&((STRUCT_TYPE *)0)->MemberVar)))

/*取最大、最小值*/
#define	CH_MIN(x,y)	((x)<(y)?(x):(y))
#define	CH_MAX(x,y)	((x)>(y)?(x):(y))
#define CH_ABS(x) (((x)<0)?(-(x)):(x))
#define CH_SQU(x)	((x) * (x))
#define CH_ZERO(x)	memset(&(x), 0, sizeof(x))
/*判断直线(a1,a2) 是否与直线(b1,b2)部分重合*/
#define CH_MIDLINE(a1,a2,b1, b2) ((((a1) >= (b1)) && ((a1) <= (b2))) || (((a2) >= (b1)) && ((a2) <= (b2)))		\
						|| (((b1) >= (a1)) && ((b1) <= (a2))) || (((b2) >= (a1)) && ((b2) <= (a2))))

#define z_INVALIDED	((int*)0xff)		/*无效的值*/
#define z_FALSE	((int*)0)		/*否*/
#define z_TRUE	((int*)1)		/*是*/
#define z_UserCancle    (-5)	/*用户取消了搜索操作 */
#if 0
/* 无符号64位整数 */
typedef struct  
{
#ifdef IS_LITTLE_ENDIAN
	U32 ul_Low32;
	U32 ul_High32;
#else
	U32 ul_High32;
	U32 ul_Low32;
#endif
}CH_UNSIGNED_64,*PCH_UNSIGNED_64;

/* 符号64位整数 */
typedef CH_UNSIGNED_64 CH_SIGNED_64,*PCH_SIGNED_64;

/*64位整数相关操作*/

/*是否是零*/
#define CH64_IS_ZERO(a)        (((a).ul_High32 == 0) && ((a).ul_Low32 == 0))

/*是否相等*/
#define CH64_EQ(a, b)        (((a).ul_High32 == (b).ul_High32) && ((a).ul_Low32 == (b).ul_Low32))

/*不等*/
#define CH64_NE(a, b)        (((a).ul_High32 != (b).ul_High32) || ((a).ul_Low32 != (b).ul_Low32))

/* >= 0 */
#define CH64_GE_ZERO(a)        (((a).ul_High32 >> 31) == 0)

/*符号比较a,b的值op 是操作符: 可以是: > , >= , < , <= ,==,!=*/
#define CH64_CMP(a, op, b)    (((a).ul_High32 == (b).ul_High32) ? ((a).ul_Low32 op (b).ul_Low32) : \
                 ((S32)(a).ul_High32 op (S32)(b).ul_High32))

/*无符号比较a,b的值op 是操作符: 可以是: > , >= , < , <= ,==,!=*/
#define CH64_UCMP(a, op, b)    (((a).ul_High32 == (b).ul_High32) ? ((a).ul_Low32 op (b).ul_Low32) : \
                 ((a).ul_High32 op (b).ul_High32))

/*a,b与运算，结果在r 中*/
#define CH64_AND(r, a, b)        ((r).ul_Low32 = (a).ul_Low32 & (b).ul_Low32, \
                 (r).ul_High32 = (a).ul_High32 & (b).ul_High32)

/*a,b或运算，结果在r 中*/
#define CH64_OR(r, a, b)        ((r).ul_Low32 = (a).ul_Low32 | (b).ul_Low32, \
                 (r).ul_High32 = (a).ul_High32 | (b).ul_High32)

/*a,b异或运算，结果在r 中*/
#define CH64_XOR(r, a, b)        ((r).ul_Low32 = (a).ul_Low32 ^ (b).ul_Low32, \
                 (r).ul_High32 = (a).ul_High32 ^ (b).ul_High32)

/*r,a或运算，结果在r 中*/
#define CH64_OR2(r, a)        ((r).ul_Low32 = (r).ul_Low32 | (a).ul_Low32, \
                 (r).ul_High32 = (r).ul_High32 | (a).ul_High32)

/*a 非运算, 结果在r 中*/
#define CH64_NOT(r, a)        ((r).ul_Low32 = ~(a).ul_Low32, \
                 (r).ul_High32 = ~(a).ul_High32)

/*a求补,结果在r 中*/
#define CH64_NEG(r, a)        ((r).ul_Low32 = -(S32)(a).ul_Low32, \
                 (r).ul_High32 = -(S32)(a).ul_High32 - ((r).ul_Low32 != 0))
                 
/*a,b加法运算, 结果在 r 中*/
#define CH64_ADD(r, a, b) \
{ \
	CH_UNSIGNED_64 _a, _b; \
	_a = a;\
	_b = b; \
	(r).ul_Low32 = _a.ul_Low32 + _b.ul_Low32; \
	(r).ul_High32 = _a.ul_High32 + _b.ul_High32 + ((r).ul_Low32 < _b.ul_Low32); \
}

/* a 减b ，结果在 r 中*/
#define CH64_SUB(r, a, b) \
{ \
	CH_UNSIGNED_64 _a, _b; \
	_a.ul_High32 = (a).ul_High32;\
	_a.ul_Low32 = (a).ul_Low32;\
	_b.ul_High32 = (b).ul_High32; \
	_b.ul_Low32 = (b).ul_Low32; \
	(r).ul_Low32 = _a.ul_Low32 - _b.ul_Low32; \
	(r).ul_High32 = _a.ul_High32 - _b.ul_High32 - (_a.ul_Low32 < _b.ul_Low32); \
}

/* a 乘b ，结果在 r 中*/
#define CH64_MUL(r, a, b) \
{ \
	CH_UNSIGNED_64 _a, _b; \
	_a.ul_High32 = (a).ul_High32;\
	_a.ul_Low32 = (a).ul_Low32;\
	_b.ul_High32 = (b).ul_High32; \
	_b.ul_Low32 = (b).ul_Low32; \
	CH64_MUL32(r, _a.ul_Low32, _b.ul_Low32); \
	(r).ul_High32 += _a.ul_High32 * _b.ul_Low32 + _a.ul_Low32 * _b.ul_High32; \
}

/*取32位的低16位*/
#define CH_LO16(a)        ((a) & CH_BITMASK(16))
/*取32位的高16位*/
#define CH_HI16(a)        ((a) >> 16)

/*a,b 两个32 位整数相乘,结果在64位整数r 中*/
#define CH64_MUL32(r, a, b) \
{ \
	U32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3; \
	_a1 = CH_HI16(a), _a0 = CH_LO16(a); \
	_b1 = CH_HI16(b), _b0 = CH_LO16(b); \
	_y0 = _a0 * _b0; \
	_y1 = _a0 * _b1; \
	_y2 = _a1 * _b0; \
	_y3 = _a1 * _b1; \
	_y1 += CH_HI16(_y0);                         /* can't carry */ \
	_y1 += _y2;                                /* might carry */ \
	if (_y1 < _y2)    \
		_y3 += (U32)(CH_BIT(16));  /* propagate */ \
	(r).ul_Low32 = (CH_LO16(_y1) << 16) + CH_LO16(_y0); \
	(r).ul_High32 = _y3 + CH_HI16(_y1); \
}

/*  a 除以 b , qp 是商, rp 是余数,
  *  a,b 都是无符号整数*/
void CH_udivmod(PCH_UNSIGNED_64 rpu64_qp,
				     PCH_UNSIGNED_64 rpu64_rp,
				     PCH_UNSIGNED_64 rpu64_a,
				     PCH_UNSIGNED_64 rpu64_b);

/*  a 除以 b , qp 是商, rp 是余数  
  *  a,b 都是无符号整数*/
#define CH64_UDIVMOD(qp, rp, a, b)    CH_udivmod(((qp)?&(qp),0),((rp)?&(rp),0),&a,&b)

/*  a 除以 b , r 是商 即: r=a / b
  *  a,b 都是符号或无符号整数*/
#define CH64_DIV(r, a, b) \
{ \
	CH_UNSIGNED_64 _a, _b; \
	U32 _negative;\
	_negative = ((S32)(a).ul_High32) < 0; \
	if (_negative)\
	{ \
		CH64_NEG(_a, a); \
	}\
	else\
	{ \
		_a.ul_High32 = (a).ul_High32;\
		_a.ul_Low32 = (a).ul_Low32;\
	} \
	if (((S32)(b).ul_High32) < 0) \
	{ \
		_negative ^= 1; \
		CH64_NEG(_b, b); \
	}\
	else\
	{ \
		_b.ul_High32 = (b).ul_High32; \
		_b.ul_Low32 = (b).ul_Low32; \
	} \
	CH64_UDIVMOD(r,0, _a, _b); \
	if (_negative) \
		CH64_NEG(r, r); \
}

/*  a 除以 b , r 是余数(模) 即: r=a%b
  *  a,b 都是符号或无符号整数*/
#define CH64_MOD(r, a, b) \
{ \
	CH_UNSIGNED_64 _a, _b; \
	U32 _negative;\
	_negative = ((S32)(a).ul_High32) < 0; \
	if (_negative)\
	{ \
		CH64_NEG(_a, a); \
	}\
	else\
	{ \
		_a.ul_High32 = (a).ul_High32;\
		_a.ul_Low32 = (a).ul_Low32;\
	} \
	if (((S32)(b).ul_High32) < 0)\
	{ \
		CH64_NEG(_b, b); \
	}\
	else\
	{ \
		_b.ul_High32 = (b).ul_High32; \
		_b.ul_Low32 = (b).ul_Low32; \
	} \
	CH64_UDIVMOD(0,r, _a, _b); \
	if (_negative) \
		CH64_NEG(r, r); \
}

/*a 左移b[0...64]位,即: r = a << b	
  * a,r 都是64位整数*/
#define CH64_SHL(r, a, b)\
{ \
	if (b)\
	{ \
		CH_UNSIGNED_64 _a; \
		_a.ul_High32 = (a).ul_High32;\
		_a.ul_Low32 = (a).ul_Low32;\
		if ((b) < 32)\
		{ \
			(r).ul_Low32 = _a.ul_Low32 << ((b) & 31); \
			(r).ul_High32 = (_a.ul_High32 << ((b) & 31)) | (_a.ul_Low32 >> (32 - (b))); \
		}\
		else\
		{ \
			(r).ul_Low32 = 0; \
			(r).ul_High32 = _a.ul_Low32 << ((b) & 31); \
		} \
	} \
	else\
	{ \
		(r).ul_High32 = (a).ul_High32;\
		(r).ul_Low32 = (a).ul_Low32;\
	} \
}

/*a 左移b[0...64]位,即: r = a << b	
  * a 都是32 位整数,r 是64 位整数*/
#define CH64_ISHL(r, a, b) \
{ \
	if (b) \
	{ \
		CH_UNSIGNED_64 _a; \
		_a.ul_Low32 = (a); \
		_a.ul_High32 = 0; \
		if ((b) < 32) \
		{ \
			(r).ul_Low32 = (a) << ((b) & 31); \
			(r).ul_High32 = ((a) >> (32 - (b))); \
		}\
		else\
		{ \
			(r).ul_Low32 = 0; \
			(r).ul_High32 = (a) << ((b) & 31); \
		} \
	}\
	else\
	{ \
		(r).ul_Low32 = (a); \
		(r).ul_High32 = 0; \
	} \
}

/*a 右移b[0...64]位,即: r = a >> b	
  * a 都是64 位整数,r 是64 位整数*/
#define CH64_SHR(r, a, b)\
{ \
	if (b)\
	{ \
		CH_UNSIGNED_64 _a; \
		_a.ul_High32 = (a).ul_High32;\
		_a.ul_Low32 = (a).ul_Low32;\
		if ((b) < 32) \
		{ \
			(r).ul_Low32 = (_a.ul_High32 << (32 - (b))) | (_a.ul_Low32 >> ((b) & 31)); \
			(r).ul_High32 = ((S32)_a.ul_High32) >> ((b) & 31); \
		}\
		else\
		{ \
			(r).ul_Low32 = ((S32)_a.ul_High32) >> ((b) & 31); \
			(r).ul_High32 = ((S32)_a.ul_High32) >> 31; \
		} \
	}\
	else\
	{ \
		(r).ul_High32 = (a).ul_High32;\
		(r).ul_Low32 = (a).ul_Low32;\
	} \
}

/*a 右移b[0...64]位,即: r = a >> b	
  * a 都是32 位整数,r 是64 位整数*/
#define CH64_USHR(r, a, b) \
{ \
	if (b) \
	{ \
		CH_UNSIGNED_64 _a; \
		_a.ul_High32 = (a).ul_High32;\
		_a.ul_Low32 = (a).ul_Low32;\
		if ((b) < 32) \
		{ \
			(r).ul_Low32 = (_a.ul_High32 << (32 - (b))) | (_a.ul_Low32 >> ((b) & 31)); \
			(r).ul_High32 = _a.ul_High32 >> ((b) & 31); \
		}\
		else\
		{ \
			(r).ul_Low32 = _a.ul_High32 >> ((b) & 31); \
			(r).ul_High32 = 0; \
		} \
	}\
	else\
	{ \
		(r).ul_High32 = (a).ul_High32;\
		(r).ul_Low32 = (a).ul_Low32;\
	} \
}

/*64 位转换到32 位*/
#define CH64_L2I(i, l)        ((i) = (l).ul_Low32)
#define CH64_L2UI(ui, l)        ((ui) = (l).ul_Low32)

/*符号32 位转换到64 位*/
#define CH64_I2L(l, i)        {S32 _i = ((S32)(i)) >> 31; (l).ul_Low32 = (i); (l).ul_High32 = _i; }

/*无符号32 位转换到64 位*/
#define CH64_UI2L(l, ui)      ((l).ul_Low32 = (ui), (l).ul_High32 = 0)


#endif

typedef struct _time_
{
	U16 ui_Year;
	U8  uc_Month;
	U8  uc_Date;
	U8  uc_Hour;
	U8  uc_Minute;
	U8  uc_Second;
	U16 ui_MiSecond;
}CH_TIME,*PCH_TIME;

/* 链表结构 */
typedef struct _ListStr_
{
    struct _ListStr_ *p_Next;
    struct _ListStr_ *p_Prev;
}CH_LISTPTR,*PCH_LISTPTR;

/* 插入元素 "_e" 到链表中，并放在"_l"之前.*/
#define CH_LSIT_INSERT_BEFORE(_e,_l)	 \
{		 \
	(_e)->p_Next = (_l);	 \
	(_e)->p_Prev = (_l)->p_Prev; \
	(_l)->p_Prev->p_Next = (_e); \
	(_l)->p_Prev = (_e);	 \
}

/* 插入元素 "_e" 到链表中，并放在"_l"之后.*/
#define CH_LIST_INSERT_AFTER(_e,_l)	 \
{		 \
	(_e)->p_Next = (_l)->p_Next; \
	(_e)->p_Prev = (_l);	 \
	(_l)->p_Next->p_Prev = (_e); \
	(_l)->p_Next = (_e);	 \
}

/** Remove the element "_e" from it's circular list.*/
#define CH_LIST_REMOVE_LINK(_e)	       \
{		       \
	(_e)->p_Prev->p_Next = (_e)->p_Next; \
	(_e)->p_Next->p_Prev = (_e)->p_Prev; \
}

/* Remove the element "_e" from it's circular list. Also initializes the
 * linkage. */
#define CH_LIST_REMOVE_AND_INIT_LINK(_e)    \
{		       \
	(_e)->p_Prev->p_Next = (_e)->p_Next; \
	(_e)->p_Next->p_Prev = (_e)->p_Prev; \
	(_e)->p_Next = (_e);	       \
	(_e)->p_Prev = (_e);	       \
}

/* Return non-zero if the given circular list "_l" is empty, zero if the
 * circular list is not empty */
#define CH_LIST_CLIST_IS_EMPTY(_l) ((CH_BOOL)((_l)->p_Next == (_l)))

/** Initialize a circular list */
#define CH_LIST_INIT_CLIST(_l)  \
{	   \
	(_l)->p_Next = (_l); \
	(_l)->p_Prev = (_l); \
}


/* 动态通用数组 */
/* 第一个位置是 0 ，最后一个位置是 iMaxElementNumber-1 */
typedef struct 
{
	S32 iCurrentIndex;    /* 当前元素的索引，暂时未用 */

	S32 iElementCount;/* 数组中元素个数 */
	S32 iMaxElementNumber;/* 数组中最大元素个数 */
	void** ppData; /* 数据指针数组 */
}CH_GENERAL_ARRAY,*PCH_GENERAL_ARRAY;

/* 比较数组元素大小的函数类型定义
 * 参数：
 *		pElement1 ：要比较的第1个元素 
 *		pElement2 ：要比较的第2个元素 
 *		pOtherArg ：比较需要的其他任意参数
 * 返回值:
 * 	< 0     :     表示 pElement1 小于  pElement2
 *	0       :     表示 pElement1 等于  pElement2
 *	> 0     :     表示 pElement1 大于  pElement2
 */
typedef S32 (* ARRAY_COMPARATOR_FUNC) (void* pElement1,void* pElement2,void* pOtherArg);

S32 CH_StrArrayComp(void* p1,void *p2,void*pOtherArg);


/* 创建一个最大容量为 iMaxElementNum 的数组,
 * 函数首先申请一个 CH_GENERAL_ARRAY 结构空间,
 * 然后调用 CH_InitArray 初始化结构成员*/
PCH_GENERAL_ARRAY CH_CreateArray(S32 iMaxElementNum);

/* 初始化结构成员，申请 iMaxElementNum 个元素空间
 * 若 iMaxElementNum <= 0,则 设iMaxElementNum = 32；
 * 注意：
 *      调用时 rp_Array 必须已经分配内存，函数不负责分配CH_GENERAL_ARRAY 结构内存
 *      函数只是分配数组中元素的内存 */
CH_BOOL CH_InitArray(PCH_GENERAL_ARRAY rp_Array,S32 iMaxElementNum);

/* 取数组的第 iIndex 个元素,(第一个位置是 0 ，最后一个位置
 * 是 iMaxElementNumber-1)元素仍然在数组中。
 * 返回值是一个void指针，可以根据需要转换成其它类型的指针或直接
 * 转换成32位整数使用 */
void* CH_GetArrayElement(PCH_GENERAL_ARRAY pArray,S32 iIndex);

/* 设置数组的第 iIndex 个元素,(第一个位置是 0 ，最后一个位置
 * 是 iMaxElementNumber-1)。
 * 成功返回TRUEM，失败返回FALSE */
CH_BOOL CH_SetArrayElement(PCH_GENERAL_ARRAY pArray,S32 iIndex,void* pElement);

/* 返回数组元素个数 */
S32 CH_GetArrayCount(PCH_GENERAL_ARRAY pArray);

/* 返回数组最大元素个数 */
S32 CH_MaxArrayCount(PCH_GENERAL_ARRAY pArray);

/* 在数组中查找与给定的元素(pElement)相匹配的元素的位置(第一个位置是0,最后一个位置是 iMaxElementNumber-1)
 * 参数:
 *		pArray:要查的数组
 *		pElement：要匹配的数组元素
 *		fn_ComFunc:比较数组元素大小的函数
 * 返回：
 *		-1: 没有找到匹配的数组元素
 *	   >=0: 匹配元素在数组中的位置*/
S32 CH_FindElementInArray(PCH_GENERAL_ARRAY pArray,void* pElement,ARRAY_COMPARATOR_FUNC fn_ComFunc);

/* 返回元素 pElement 在数组中的位置(第一个位置是 0 ，最后一个位置
 * 是 iMaxElementNumber-1),不在数组中返回 -1 */
S32 CH_IndexOfArray(PCH_GENERAL_ARRAY pArray,void* pElement);

/* 在iIndex位置插入一个元素,如果iIndex 的值> pArray->iElementCount 或者 <０
 * 则在数组末尾追加.
 * 返回实际插入的位置，返回 -1 表示出错*/
S32 CH_InsertArrayElement(PCH_GENERAL_ARRAY pArray,void* pElement,S32 iIndex);

/* 追加一个元素到数组末尾 */
CH_BOOL CH_AppendArrayElement(PCH_GENERAL_ARRAY pArray,void* pElement);

/* 把pSourArray中的元素追加到pDestArray数组末尾 */
CH_BOOL CH_AppendArray(PCH_GENERAL_ARRAY pDestArray,PCH_GENERAL_ARRAY pSourArray);

/* 在数组中删除元素:pElement(元素并没有从内存中释放，只是从数组中去除) */
CH_BOOL CH_RemoveArrayElement(PCH_GENERAL_ARRAY pArray,void* pElement);

/* 删除数组中第 iIndex 个元素(元素并没有从内存中释放，只是从数组中去除) 
 * iIndex < 0 或 >元素个数,表示从删除尾部元素
 * 返回值：成功，返回从数组中删除的元素；失败，返回 NULL */
void* CH_RemoveArrayElementIndex(PCH_GENERAL_ARRAY pArray,S32 iIndex);

/* 用给定的元素替换原来在 iIndex 位置的元素 
 *  iIndex < 0 或 >元素个数,表示直接在数组尾部追加元素
 * 返回值：成功，返回从数组中被替换的元素,如果是直接追加的元素
 * 返回追加的元素，即:rp_Element；失败，返回 NULL */
void* CH_ReplaceArrayElementIndex(PCH_GENERAL_ARRAY rp_Array,void* rp_Element,S32 rl_Index);

/* 内存释放函数 */
typedef void (* ARRAY_DELE_FUNC) (void* pElement, void *pOtherArg);

/* 清除数组中所有的元素,但不释放由 CH_InitArray 分配的元素空间,数组初始化为空数组。
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从数组中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　*/
void CH_ResetArray(PCH_GENERAL_ARRAY pArray,CH_BOOL bIsFreeElement,
				   ARRAY_DELE_FUNC fnFreeFunc,void *pOtherArg);

/* 清除数组中所有的元素,释放由 CH_InitArray 分配的元素空间,数组初始化为空数组。
 * 下次使用时应该重新调用 CH_InitArray 后才能使用
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从数组中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　*/
void CH_ClearArray(PCH_GENERAL_ARRAY pArray,CH_BOOL bIsFreeElement,
				   ARRAY_DELE_FUNC fnFreeFunc,void *pOtherArg);

/* 从内存中删除数组,函数先调用 CH_ClearArray 释放CH_InitArray 分配的元素空间，
 * 然后调用 CH_FreeMem 释放数组本身的数据结构(CH_GENERAL_ARRAY)空间,释放后,
 * pArray 指针是无效的 */
void CH_DestroyArray(PCH_GENERAL_ARRAY pArray);

/* 重新设置数组中元素的最大个数 */
CH_BOOL CH_ResetArrayMaxCount(PCH_GENERAL_ARRAY pArray,S32 iNewMaxElementNumber);

/* 将数组中的元素排序,排序比较函数是fnArrayCompFunc ,
 * fnArrayCompFunc 返回：
 	< 0     :     表示 pElement1 小于  pElement2(pElement1在pElement2的前面)
	0       :     表示 pElement1 等于  pElement2
	> 0     :     表示 pElement1 大于  pElement2(pElement1在pElement2的后面)
 * 如果 fnArrayCompFunc == NULL 那么把pElement1 和 pElement2 转换成S32 类型直接比较 
 *      参数:pOtherArg　直接传给函数fnArrayCompFunc的pOtherArg参数,一般情况可设为NULL　*/
void CH_SortArray(PCH_GENERAL_ARRAY pArray,ARRAY_COMPARATOR_FUNC fnArrayCompFunc,void *pOtherArg);

/* 排序插入一个元素,排序比较函数是fnArrayCompFunc（介绍同CH_SortArray）
 * 如果数组是未排序的，则该函数执行后，插入位置是没有意义的 
 * 调用该函数时，要保证数组是用同一个比较函数排序的
 * 返回实际插入的位置,返回 -1 表示失败. */
S32 CH_SortInsertElementToArray(PCH_GENERAL_ARRAY pArray,void* pElement,
								ARRAY_COMPARATOR_FUNC fnArrayCompFunc,void *pOtherArg);

/* 枚举操作函数
  * 返回:  CH_TRUE: 可以继续枚举下一个元素
  *                CH_FALSE:停止枚举下一个元素 */
  #define CH_PRE_CALLBACK
typedef CH_BOOL (*CH_PRE_CALLBACK ARRAY_ENUM_FUNC) (void* pElement, void *pOtherArg);
/* 枚举数组中下标为 iStartIndex 到 iEndIndex(包含iStartIndex 和 iEndIndex) 的数组元素,
 * 当 iStartIndex < iEndIndex,从iStartIndex 顺序增加
 * 当 iStartIndex > iEndIndex,从iStartIndex 顺序减少
 * 元素进行的操作由 fnEnumFunc 定义　 
 *      参数:pOtherArg　直接传给函数fnEnumFunc的pOtherArg参数,一般情况可设为NULL　
 *  返回实际枚举的元素个数,失败返回-1*/
S32 CH_EnumerateArrayElement(PCH_GENERAL_ARRAY pArray,S32 iStartIndex,S32 iEndIndex,
								 ARRAY_ENUM_FUNC fnEnumFunc,void *pOtherArg);


/* 队列 */
/* 队列头位置是对应元素索引 0 ，队列尾对应元素索引位置是 iMaxElementNumber-1 
typedef struct 
{
	S32 iElementCount;  队列中元素个数 
	S32 iMaxElementNumber; 队列中最大元素个数 
	S32 iCurrentIndex;队列中当前元素的索引 
	void** ppData;  队列元素指针数组 
}CH_DEQUEUE,*PCH_DEQUEUE; */

#if 0
#define CH_DEQUEUE CH_GENERAL_ARRAY
#define PCH_DEQUEUE PCH_GENERAL_ARRAY
#else
typedef CH_GENERAL_ARRAY CH_DEQUEUE,*PCH_DEQUEUE;
#endif


/* 创建一个最大容量为 iMaxElementNum 的队列,
 * 函数首先申请一个 CH_DEQUEUE 结构空间,
 * 然后调用 CH_InitDequeue 初始化结构成员
 * PCH_DEQUEUE CH_CreateDequeue(S32 iMaxElementNum); */
#define CH_CreateDequeue CH_CreateArray

/* 初始化结构成员，申请 iMaxElementNum 个元素空间
 * 若 iMaxElementNum <= 0,则 设iMaxElementNum = 32；
 * 注意：
 *      调用时 pDequeue 必须已经分配内存，函数不负责分配CH_DEQUEUE 结构内存
 *      函数只是分配数组中元素的内存,函数原型：
 * CH_BOOL CH_InitDequeue(PCH_DEQUEUE pDequeue,S32 iMaxElementNum); */
#define CH_InitDequeue CH_InitArray

/* 内存释放函数 
 * typedef void (* DEQUEUE_DELE_FUNC) (void* pElement, void *pOtherArg); */
#define DEQUEUE_DELE_FUNC ARRAY_DELE_FUNC
/* 清除队列中所有的元素,释放由 CH_InitDequeue 分配的元素空间,队列初始化为空队列,
 * 下次使用时应该重新调用 CH_InitDequeue 后才能使用
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从队列中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　
   void CH_ClearDequeue(PCH_DEQUEUE pDequeue,CH_BOOL bIsFreeElement,
				   DEQUEUE_DELE_FUNC fnFreeFunc,void *pOtherArg); */
#define CH_ClearDequeue CH_ClearArray

/* 清除队列中所有的元素,但不释放由 CH_InitDequeue 分配的元素空间,队列初始化为空队列,
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从队列中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　
   void CH_ResetDequeue(PCH_DEQUEUE pDequeue,CH_BOOL bIsFreeElement,
				   DEQUEUE_DELE_FUNC fnFreeFunc,void *pOtherArg); */
#define CH_ResetDequeue CH_ResetArray

/* 从内存中删除队列
   void CH_DestroyDequeue(PCH_DEQUEUE pDequeue); */
#define CH_DestroyDequeue CH_DestroyArray

/* 返回栈元素个数 */
#define  CH_GetDequeueCount CH_GetArrayCount

/* 枚举操作函数 */
#define DEQUEUE_ENUM_FUNC ARRAY_ENUM_FUNC
/* 枚举队列中索引为 iStartIndex 到 iEndIndex(包含iStartIndex 和 iEndIndex) 的队列元素,
 * 元素进行的操作由 fnEnumFunc 定义　 
 *      参数:pOtherArg　直接传给函数fnEnumFunc的pOtherArg参数,一般情况可设为NULL　
   CH_BOOL CH_EnumerateDequeueElement(PCH_DEQUEUE pDequeue,S32 iStartIndex,S32 iEndIndex,
								 DEQUEUE_ENUM_FUNC fnEnumFunc,void *pOtherArg); */
#define CH_EnumerateDequeueElement CH_EnumerateArrayElement

/* 在队列中删除元素:pElement(元素并没有从内存中释放，只是从队列中去除) 
   CH_BOOL CH_RemoveDequeueElement(PCH_DEQUEUE pDequeue,void* pElement); */
#define CH_RemoveDequeueElement CH_RemoveArrayElement

/* 删除队列中第 iIndex 个元素(元素并没有从内存中释放，只是从队列中去除) 
 * iIndex < 0 或 >元素个数,表示从删除尾部元素
 * 返回值：成功,返回从队列中删除的元素；失败,返回 NULL 
   void* CH_RemoveDequeueElementIndex(PCH_DEQUEUE pDequeue,S32 iIndex); */
#define CH_RemoveDequeueElementIndex CH_RemoveArrayElementIndex

/* 取队列的第 iIndex 个元素,(第一个位置是 0 ，最后一个位置
 * 是 iMaxElementNumber-1)元素仍然在队列中。
 * 返回值是一个void指针，可以根据需要转换成其它类型的指针或直接
 * 转换成32位整数使用 
   void* CH_GetDequeueElement(PCH_DEQUEUE pDequeue,S32 iIndex); */
#define  CH_GetDequeueElement CH_GetArrayElement 

/* 在队列的iIndex位置插入一个元素,如果iIndex 的值> pArray->iElementCount 或者 <０
 * 则在队列末尾追加.
 * 返回实际插入的位置，返回 -1 表示出错 */
S32 CH_InsertDequeueElement(PCH_DEQUEUE rp_Dequeue,void* rp_Element,S32 rl_Index);

/* 追加一个元素到队列末尾 */
CH_BOOL CH_AppendDequeueElement(PCH_DEQUEUE pDequeue,void* pElement); 

/* 把pSourDequeue中的元素追加到pDestDequeue末尾 */
CH_BOOL CH_AppendDequeue(PCH_DEQUEUE pDestDequeue,PCH_DEQUEUE pSourDequeue); 

/* 重新设置队列中元素的最大个数 
	CH_BOOL CH_ResetDequeueMaxCount(PCH_DEQUEUE pDequeue,S32 iNewMaxElementNumber); */
#define CH_ResetDequeueMaxCount CH_ResetArrayMaxCount



/* 栈(Statck)操作 */
/* 栈底位置是对应元素索引 0 ，栈顶对应元素索引位置是 iMaxElementNumber-1 
typedef struct 
{
	S32 iElementCount;  队列中元素个数 
	S32 iMaxElementNumber;队列中最大元素个数 
	S32 iCurrentIndex; 队列中当前元素的索引 
	void** ppData;  队列元素指针数组 
}CH_STACK,*PCH_STACK; */

#if 0
#define CH_STACK CH_GENERAL_ARRAY
#define PCH_STACK PCH_GENERAL_ARRAY
#else
typedef CH_GENERAL_ARRAY CH_STACK,*PCH_STACK;
#endif

/* 创建一个最大容量为 iMaxElementNum 的栈,
 * 函数首先申请一个 CH_STACK 结构空间,
 * 然后调用 CH_InitStack 初始化结构成员
 * PCH_STACK CH_CreateStack(S32 iMaxElementNum); */
#define CH_CreateStack CH_CreateArray

/* 初始化结构成员，申请 iMaxElementNum 个元素空间
 * 若 iMaxElementNum <= 0,则 设iMaxElementNum = 32；
 * 注意：
 *      调用时 pStack 必须已经分配内存，函数不负责分配 CH_STACK 结构内存
 *      函数只是分配数组中元素的内存,函数原型：
 * CH_BOOL CH_InitStack(PCH_STACK pStack,S32 iMaxElementNum); */
#define CH_InitStack CH_InitArray

/* 内存释放函数 
 * typedef void (*STACK_DELE_FUNC) (void* pElement,void *pOtherArg); */
#define STACK_DELE_FUNC ARRAY_DELE_FUNC
/* 清除栈中所有的元素,释放由 CH_InitStack 分配的元素空间,栈初始化为空栈,
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从栈中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　
   void CH_ClearStack(PCH_STACK pStack,CH_BOOL bIsFreeElement,
				   STACK_DELE_FUNC fnFreeFunc,void *pOtherArg); */
#define CH_ClearStack CH_ClearArray

/* 清除栈中所有的元素,但是不释放由 CH_InitStack 分配的元素空间,栈初始化为空栈,
 * 下次使用时应该重新调用 CH_InitStack 后才能使用
 * 当　bIsFreeElement==CH_FALSE 时,元素并没有从内存中释放，只是从栈中去除
 * 当　bIsFreeElement==CH_TRUE 时,用 fnFreeFunc 将元素从内存中清除,若
 * fnFreeFunc 为 NULL ，则直接用 CH_FreeMem() 将元素释放 
 *      参数:pOtherArg　直接传给函数fnFreeFunc的pOtherArg参数,一般情况可设为NULL　
   void CH_ResetStack(PCH_STACK pStack,CH_BOOL bIsFreeElement,
				   STACK_DELE_FUNC fnFreeFunc,void *pOtherArg); */
#define CH_ResetStack CH_ResetArray

/* 从内存中删除栈 
  void CH_DestroyStack(PCH_STACK pStack); */
#define CH_DestroyStack CH_DestroyArray

/* 返回栈元素个数 */
#define  CH_GetStackCount CH_GetArrayCount

/* 把元素压入栈,并移动栈指针 */
CH_BOOL CH_PushStack(PCH_STACK pStack,void* pStackElement); 

/* 取栈顶元素，并把栈指针下移一位 */
void* CH_PopStack(PCH_STACK pStack);

/* 重新设置队列中元素的最大个数 
 CH_BOOL CH_ResetStackMaxCount(PCH_STACK pStack,S32 iNewMaxCount); */
#define CH_ResetStackMaxCount CH_ResetArrayMaxCount

/* 从栈的指定位置中取一个元素,栈顶位置是 pStack->iElementCount-1,栈底位置是 0
 * 返回值是一个void指针，可以根据需要转换成其它类型的指针或直接
 * 转换成32位整数使用 
   void* CH_GetStackElement(PCH_STACK pStack,S32 iIndex); */
#define  CH_GetStackElement CH_GetArrayElement 

/* 在栈的指定位置插入一个元素,栈顶位置是 pStack->iElementCount-1,栈底位置是 0,
 * 如果iIndex 的值> pArray->iElementCount 或者 <０则出错 */
CH_BOOL CH_InsertStackElement(PCH_STACK pStack,void* pStackElement,S32 iIndex);

/* 把pSourStack中的元素追加到pDestStack末尾 */
CH_BOOL CH_AppendStack(PCH_STACK pDestStack,PCH_STACK pSourStack);

/* 在栈中删除元素:pElement(元素并没有从内存中释放，只是从栈中去除) 
   CH_BOOL CH_RemoveStackElement(PCH_STACK pStack,void* pStackElement)*/
#define  CH_RemoveStackElement CH_RemoveArrayElement

/* 删除栈中第 iIndex 个元素(元素并没有从内存中释放，只是从栈中去除) 
 * iIndex < 0 或 >元素个数,表示栈顶部元素
 * 返回值：成功，返回从栈中删除的元素；失败，返回 NULL 
   void* CH_RemoveStackElementIndex(PCH_STACK pStack,S32 iIndex); */
#define CH_RemoveStackElementIndex CH_RemoveArrayElementIndex

/* 取栈顶元素，但是并不移动栈指针,元素仍在栈中 */
#define CH_GetStackTopElement(pStack) CH_GetStackElement(pStack,pStack->iElementCount-1)

/* 取栈底元素，但是并不移动栈指针,元素仍在栈中 */
#define CH_GetStackBottomElement(pStack) CH_GetStackElement(pStack,0)








/*字符串函数*/

/*从字符串pSrc 中拷贝iLen 个字符到 pDest中
  *参数:iLen :拷贝的字节数,若iLen <=0 则把pSrc 全部拷贝。
  * 返回:成功- pDest
                  失败- NULL*/
S8 *CH_StrCopyLen(S8 *rp_Dest,S8 *rp_Src,S32 rl_Len);

/* 把字符串pSrc 拷贝到 pDest中
  * 返回:成功- pDest
                  失败- NULL
  * 函数原型:
		S8 *CH_StrCopy(S8 *pDest,S8 *pSrc);*/
#define CH_StrCopy(pDest,pSrc) CH_StrCopyLen(pDest,pSrc,-1)

/* 拷贝字符串pSrc到pDest，如果pDest是NULL,直接分配内存
 * 如果pDest不为NULL先释放内存，再分配内存 */
#define CH_ACopyStr(pDest,pSrc) CH_ACopyMem(pDest,pSrc,CH_StrLen(pSrc)+1)


/*计算字符c在字符串pInStr中的个数,iLen表示在字符串中查找的长度，iLen==-1表示全部计算*/
S32 CH_CountChar(S8 *pInStr,S32 iLen,S8 c);

/*从pInStr中过滤字符c*/
void CH_SkipChar(S8 *pInStr,S8 c);

/*插入字符到字符串中*/
S8 *CH_InsertCharToStr(S8 *pInStr,S8 c,S32 iIndex);

/* 把字符串中的iOldChar 替换为 iNewChar,替换长度为 iLen*/
CH_BOOL CH_ReplaceCharacter(S8* pInStr,S32 iLen,S8 iOldChar,S8 iNewChar);

/*把字符ch转换成大写*/
S8 CH_ASCCharToUpper(S8 ch);

/*把字符ch转换成小写*/
S8 CH_ASCCharToLower(S8 ch);

/*把字符串szStr转换成大写*/
S8* CH_ASCStrToUpper(S8* szStr);

/*把字符串szStr转换成小写*/
S8* CH_ASCStrToLower(S8* szStr);

/*判断是否时ASC字符的空格*/
CH_BOOL CH_IsAsciiSpace(S8 iChar);

/*判断是否时ASC字符的字母*/
/* CH_BOOL CH_IsAsciiAlpha(S8 iChar); */
#define CH_IsAsciiAlpha(iChar) ((CH_BOOL)(((iChar) >= 'a' && (iChar) <= 'z') ||((iChar) >= 'A' && (iChar) <= 'Z')))

/*判断是否时ASC字符的数字*/
/* CH_BOOL CH_IsAsciiDigit(S8 iChar); */
#define  CH_IsAsciiDigit(iChar) ((CH_BOOL)(((iChar) >= '0') && ((iChar) <= '9')))

/* 把szStr前面(左边)的 cSet 字符去调,并把字符串前移到
  * 头地址开始的地方*/
S8* CH_LeftTrim(S8* szStr,S8 cSet);

/* 把szStr中从 iStartPos 开始的 iLen 个字符去掉，
 * iLen == -1，表示从 iStartPos 开始的所有字符。*/
S8* CH_StrCut(S8* szStr,S32 iStartPos,S32 iLen);

/*把szStr后面(右边)的 cSet 字符去调*/
S8* CH_RightTrim(S8* szStr,S8 cSet);

/* 把szStr前面(左边)和后面(右边)的 cSet 字符去调,
  * 并把字符串前移到头地址开始的地方*/
S8* CH_AllTrim(S8* szStr,S8 cSet);

/* 把字符iChar追加到szDest 的后面,返回szDest,调用者要保证szDest的内存空间足够 */
S8* CH_AppendChar(S8 *szDest,S8 iChar);

/* 把szSrc追加到szDest 的后面,返回szDest,使用时应该保证szDest有足够的空间
 * 容纳两个字符串 */
S8* CH_AppendStr(S8 *szDest,S8* szSrc);

/* 把字符iChar追加到szDest 的后面,返回pDest,如果pDest空间不够,函数自动重新分配pDest的内存 
 * 参数：iDestBuffLen,表示pDest最大缓冲区大小*/
S8* CH_AutoAppendChar(S8 *pDest,S32 iDestBuffLen,S8 iChar);

/* 把szSrc追加到pDest 的后面,返回pDest,如果pDest空间不够,函数自动重新分配pDest的内存 
 * 参数：iDestBuffLen,表示pDest最大缓冲区大小*/
S8* CH_AutoAppendStr(S8 *pDest,S32 iDestBuffLen,S8* szSrc);

/* 把szStr前面(左边)的 szSet 字符串去调,并把字符串前移到
  * 头地址开始的地方*/
S8* CH_LeftStrTrim(S8* szStr,S8 *szSet);

/*把szStr后面(右边)的 szSet 字符串去调*/
S8* CH_RightStrTrim(S8* szStr,S8 *szSet);

/* 把szStr前面(左边)和后面(右边)的 szSet 字符串去掉,
  * 并把字符串前移到头地址开始的地方*/
S8* CH_AllStrTrim(S8* szStr,S8 *szSet);

/*判断字符串是否为空*/
#define CH_StrIsEmpty(pStr)  ((pStr) == NULL || CH_StrLen(pStr) == 0)

/*Compare characters in two buffers.
	Return Value    Relationship of First count Bytes of szSrc and szDest 
	< 0             szSrc less than szDest 
	0               szSrc identical to szDest 
	> 0             szSrc greater than szDest 
*/
S32 CH_StrCompare(S8* szSrc,S8 *szDest);

/* 比较两个字符串前 rl_Len 个字符,
 * 返回同上 */
S32 CH_StrnCompare(S8* szSrc,S8 *szDest,S32 rl_Len);

/* 大小写不敏感字符串比较 */
S32 CH_NotCaseSensitiveStrCompare(S8* szSrc,S8 *szDest);

/* 比较两个字符串前 rl_Len 个字符,
 * 返回同上 */
S32 CH_NotCaseSensitiveStrnCompare(S8* szSrc,S8 *szDest,S32 rl_Len);

/*返回c在字符串中的位置,第一位置是0,没有返回-1*/
S32 CH_FindCharPos(S8* pInDestBuff,S8 c);

/*在字符串中查找字符，并返回第一个该字符的地址,没有NULL*/
S8* CH_FindChar(S8* pInDestBuff,S8 c);

/*从右边开始查找字符 c 在字符串中的位置,第一位置是0,没有返回-1*/
S32 CH_RFindCharPos(S8* pInDestBuff,S8 c);

/*从右边开始在字符串中查找字符，并返回第一个该字符的地址,没有NULL*/
S8* CH_RFindChar(S8* pInDestBuff,S8 c);

/**********************************************************
 * 在字符串 string 中查找字符串 strCharSet 
 * 参数：
 * strCharSet:待查字符串            bIsCaseSensitive:是否大小写敏感
 * iFindLen:在 string 中最多查找的长度. 如iFindLen = -1,则全部查找
 * 成功返回在 string 中,以字符串 strCharSet开头的字符串指针
 * 失败或没有找到返回NULL
 ************************************************************/
S8 *CH_FindString(S8 *sString,S32 iFindLen,S8 *strCharSet,CH_BOOL bIsCaseSensitive);

/**********************************************************
 * 在字符串 string 中查找字符串 strCharSet 
 * 参数：
 * strCharSet:待查字符串            bIsCaseSensitive:是否大小写敏感
 * iFindLen:在 string 中最多查找的长度. 如iFindLen = -1,则全部查找
 * 成功返回以字符串 strCharSet 的位置,第一位置是0
 * 失败或没有找到返回 -1；
 ************************************************************/
S32 CH_GetSubStrPos(S8 *sString,S32 iFindLen,S8 *strCharSet,CH_BOOL bIsCaseSensitive);

/**********************************************************
 * 在字符串 string 中获取从iStartPos开始，长度为iLen的字符串
 * 成功返回szSubStr指针,
 * 失败返回 NULL；
 ************************************************************/
S8* CH_GetSubString(S8 *sString,S32 iStartPos,S32 iLen,S8 *szSubStr,S32 iMaxSubLen);

/*转换一个字符串到整数,iRadix = 10 是十进制转换
 * iRadix = 16 时16 进制转换,0 自动侦察,其它则出错*/
#define Radix_AUTODETECT 0
#define Radix_10                  10
#define Radix_16                  16
CH_BOOL  CH_StrToInteger(S32 *piOutInterger,S8 *szInString,S8 iRadix);

/* 辅助函数:用来实现 FindCharInSet（）*/
S8 GetFindInSetFilter(S8* rp_Set);

/* 函数功能：在字符串中查找字符集中的字符，并返回第一个找到的位置
 * 参数:   
 *      rp_Data:待查的字符串，以'\0'为结束标志
 *      rl_DataLen:在字符串中查找的长度，若等于 -1 ，则查找所有
 * 　　 rp_Set:要查找的字符集,以空字符结束
 * 返回：
 *      1.当在　rp_Data　中找到字符集(rp_Set)中任何一个字符时，查找结束，
 *      返回字符在 rp_Data 中位置(注意：第一个位置是 0)
 *      2.如果在　rp_Data　中不能找到字符集(rp_Set)中任何一个字符时，返回-1
 *  */
S32 CH_FindCharInSet(S8 *rp_Data,U32 rl_DataLen,S8* rp_Set);


/*功能：分解字符串为一组标记串。rsz_Source为要分解的字符串，rsz_Delim为分隔符字符串。
  
  说明：函数在 rsz_Source 中查找包含在 rsz_Delim 中的字符并用NULL('\0')来替换。
		函数先去掉头部的分界符,然后查找第一个分界符并用NULL('\0')来替换，直到找遍整个字符串。
		rsz_OutNewStr 在函数成功返回时指向下一个标记串。
  返回值: 返回去掉头部分界符的字符串地址，当没有标记串时则返回空字符NULL。*/
S8* CH_Strtok(S8* rsz_Source,S8* rsz_Delim,S8** rsz_OutNewStr);




#define CH_AUTOSTRING_BLOCKLEN 128
/* 
   定义一个长度可以动态变化的字符串结构
               定义这种结构可以避免过于频繁调用系统内存分配和
   释放函数，从而产生内存碎片。
  */
typedef struct 
{
	S8 *p_String;	/* 实际的储存字符串的指针*/
	U32 ul_Size;	/* p_String 能储存的最大长度，字符串p_String
				     的实际长度 可由函数CH_StrLen()得出，
				     且应该小于u_Size*/
	U32 ul_BlockLen;/* 每次增加的内存长度 */			    
}CH_AutoString_t,*PCH_AutoString_t;


/* 创建一个块长度为rul_BlockLen 的CH_AutoString_t 字符串
  * 若rul_BlockLen小于CH_AUTOSTRING_BLOCKLEN, 则ul_BlockLen 缺省为
  * CH_AUTOSTRING_BLOCKLEN 
  * 函数为p_String 数据成员分配ul_BlockLen 字节的空间(调用CH_InitAutoString实现)*/
PCH_AutoString_t CH_CreateAutoString(U32 rul_BlockLen);

/* 初始化rp_AutoString 的数据成员.
  * 调用函数时,rp_AutoString 必须是已经分配好内存，函数不负责
  * 为rp_AutoString 分配内存，函数为p_String 数据成员分配ul_BlockLen 
  * 字节的空间
  * 若rul_BlockLen小于CH_AUTOSTRING_BLOCKLEN, 则ul_BlockLen 缺省为
  * CH_AUTOSTRING_BLOCKLEN 
  */
CH_BOOL CH_InitAutoString(PCH_AutoString_t rp_AutoString,U32 rul_BlockLen);

/*插入字符到字符串中*/
PCH_AutoString_t CH_InsertCharToAutoStr(PCH_AutoString_t rp_Dest,S8 rc_Char,S32 rl_Index);

/*把普通字符rc_Char 追加到 rp_Dest中,函数自动调整缓冲长度*/
PCH_AutoString_t CH_AppendCharToAutostr(PCH_AutoString_t rp_Dest,S8  rc_Char);

/* 从字符串rp_Src 追加rl_CopyLen 个字符到 rp_Dest中,函数自动调整rp_Dest 的缓冲长度
  * 当rl_CopyLen < 0 或则rl_CopyLen > CH_StrLen(rp_Src) 时,拷贝rp_Src 中的所有字符串*/
PCH_AutoString_t CH_AppendSubstrToAutostr(PCH_AutoString_t rp_Dest,S8 * rp_Src,S32 rl_CopyLen);

/* 把普通字符串rp_Src 追加到 rp_Dest中,函数自动调整缓冲长度
  * 函数原型:
 		PCH_AutoString_t CH_AppendStrToAutostr(PCH_AutoString_t rp_Dest,S8 * rp_Src); */
#define CH_AppendStrToAutostr(rp_Dest,rp_Src) CH_AppendSubstrToAutostr((rp_Dest),(rp_Src),-1)

/*判断字符串是否为空*/
#define CH_AutostrIsEmpty(rp_Str)  ((rp_Str) == NULL || CH_StrIsEmpty((rp_Str)->p_String))

#define CH_ResetAutostr(rp_AutoString)\
{\
	if (rp_AutoString)\
	{\
		(*((rp_AutoString)->p_String)) = 0;\
	}\
}

#define CH_AutostrLen(rp_AutoString) CH_StrLen((rp_AutoString)->p_String)

/* 清除rp_AutoString 的数据成员，释放rp_AutoString->p_String的内存空间,
  * 但是函数并不释放rp_AutoString 结构。
  * 函数原型:
  * void CH_ClearAutoString(PCH_AutoString_t rp_AutoString)
  */
#define CH_ClearAutostr(rp_AutoString)\
{\
	if(rp_AutoString)\
	{\
		if((rp_AutoString)->p_String)\
			CH_FreeMem((rp_AutoString)->p_String);\
		(rp_AutoString)->p_String = NULL;\
		(rp_AutoString)->ul_Size = 0;\
	}\
}

/* 清除rp_AutoString 的数据成员，释放rp_AutoString->p_String的内存空间,
  * 释放rp_AutoString 结构的存储空间.置rp_AutoString 为NULL.
  * 函数原型:
  * void CH_DeleteAutostr(PCH_AutoString_t rp_AutoString)
  */
#define CH_DeleteAutostr(rp_AutoString)\
{\
	if(rp_AutoString)\
	{\
		CH_ClearAutostr(rp_AutoString);\
		CH_FreeMem(rp_AutoString);\
		rp_AutoString = NULL;\
	}\
}









/*其它通用函数*/

/* 从pSrc拷贝iSrcLen字节到pDest，如果pDest是NULL,直接分配内存
 * 如果pDest不为NULL先释放内存 */
#define CH_ACopyMem(pDest,pSrc,iSrcLen)\
{\
	if(pDest)	CH_FreeMem(pDest);\
	(pDest) = CH_AllocMem(iSrcLen);\
	if(pDest){\
		CH_MemCopy((pDest),(pSrc),(iSrcLen));\
	};\
}


/*分配 size个字节的内存,并初始化为 iValue*/
void *CH_AllocMemAndSetValue(S32 size,S8 iValue);

/*分配 size个字节的内存,并初始化为 0*/
#define CH_AllocMemAndZero(size) CH_AllocMemAndSetValue((size),0)

/*为TypeName类型变量分配内存并初始化为 0 ,返回分配的指针*/
#define CH_AllocTypeAndZero(TypeName) (TypeName*)CH_AllocMemAndSetValue(sizeof(TypeName),0)

/* 释放指针 pStr 的内存空间，并将pStr　=　NULL 
 * 注意：使用该函数宏时，必须保证 pStr 是动态分配的内存。
 * 函数原型：
 *			void CH_DeletePointer(void * pStr); */
#define CH_DeletePointer(pStr)\
{\
	if(pStr)\
	{\
		CH_FreeMem(pStr);\
		pStr = NULL;\
	}\
}

CH_BOOL CH_U32ToTime(U32 ul_time,PCH_TIME rp_Time);
CH_BOOL CH_U64ToTime(PU64 pu64_time,PCH_TIME rp_Time);
CH_BOOL CH_FormatTime(PCH_TIME rp_Time,S8* rpc_buf,S32 l_buflen,S8* rpc_Foramt);


#endif
