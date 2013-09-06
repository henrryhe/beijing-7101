/*********************************************************************
    Copyright (c) 2009-2010 Embedded Internet Solutions, Inc
    All rights reserved. You are not allowed to copy or distribute
    the code without permission.
    There are the 2D_GFX Porting APIs needed by iPanel MiddleWare. 
    
    Note: the "int" in the file is 32bits
    
    $ver0.0.0.1 $author Zouxianyun 2009/02/03
*********************************************************************/
#ifndef _IPANEL_MIDDLEWARE_PORTING_2D_HARDWARE_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_2D_HARDWARE_API_FUNCTOTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	IPANEL_FALSE    = 0,
	IPANEL_TRUE     = 1
} IPANEL_BOOL;

typedef enum
{
	IPANEL_STATUS_2D_FINISH,
	IPANEL_STATUS_2D_BUSY,
	IPANEL_STATUS_2D_UNDEF
} IPANEL_STATUS_2D_e;

typedef enum
{
	IPANEL_ROP_BLACK,        /* Blackness */
	IPANEL_ROP_PSDon,        /* ~(PS+D) */
	IPANEL_ROP_PSDna,        /* ~PS & D */
	IPANEL_ROP_PSn,            /* ~PS */
	IPANEL_ROP_DPSna,        /* PS & ~D */
	IPANEL_ROP_Dn,              /* ~D */
	IPANEL_ROP_PSDx,          /* PS^D */
	IPANEL_ROP_PSDan,        /* ~(PS&D) */
	IPANEL_ROP_PSDa,          /* PS & D */
	IPANEL_ROP_PSDxn,        /* ~(PS^D) */
	IPANEL_ROP_D,                /* D */
	IPANEL_ROP_PSDno,        /* ~PS + D */
	IPANEL_ROP_PS,              /* PS */
	IPANEL_ROP_DPSno,        /* PS + ~D */
	IPANEL_ROP_PSDo,          /* PS+D */
	IPANEL_ROP_WHITE,       /* Whiteness */
	IPANEL_ROP_UNDEF
}IPANEL_ROP_CODE_e;

typedef enum
{
	IPANEL_DATA_CONV_NONE,              /* no opetation will be done between srouce and destination surface */
	IPANEL_DATA_CONV_ROP,                /* the source and destination surface will do ROP operation */
	IPANEL_DATA_CONV_ALPHA_SRC,    /* the source and destination surface will take source's alpha to do alpha operation */
	IPANEL_DATA_CONV_ALPHA_DST,     /* the source and destination surface will take destination's alpha to do alpha operation */
	IPANEL_DATA_CONV_UNDEF
}IPANEL_DATA_CONV_e;

typedef enum
{
	IPANEL_OUTALPHA_FROM_SRC,              /* out alpha from source bitmap */
	IPANEL_OUTALPHA_FROM_DST,              /* out alpha from destination bitmap */
	IPANEL_OUTALPHA_FROM_REG,              /* out alpha from global alpha */
	IPANEL_OUTALPHA_FROM_UNDEF
}IPANEL_OUTALPHA_FROM_e;

typedef enum
{
	IPANEL_COLORSPACE_SRC,                 /* do colorspace to source bitmap */
	IPANEL_COLORSPACE_DST,                 /* do colorspace to destination bitmap */
	IPANEL_COLORSPACE_UNDEF
}IPANEL_COLORSPACE_TARGET_e;

typedef enum
{
	IPANEL_SURFACE_TYPE_FB,            /* TV SCREEN Frame buffer, and 2D3D can operate it  */
	IPANEL_SURFACE_TYPE_SCRATCH,       /* scratch buffer, and 2D3D can operate it */
	IPANEL_SURFACE_TYPE_UNDEF
}IPANEL_SURFACE_TYPE_e;

typedef struct
{
	unsigned char r;               /**< Red component*/
	unsigned char g;               /**< Green component */
	unsigned char b;               /**< Blue component */
	unsigned char a;               /**< Alpha component. */
}IPANEL_ARGB_s;

typedef struct
{
	IPANEL_BOOL bColorSpace;                                           /* colorspace enable */
	IPANEL_COLORSPACE_TARGET_e enColorSpaceTarget; /* colorspace target */
	IPANEL_ARGB_s stColorMin;                                          /* minimum color in colorspace */
	IPANEL_ARGB_s stColorMax;                                         /* maximum color in colorspace */
}IPANEL_COLORSPACE_s;

typedef struct
{
	IPANEL_BOOL bAlphaGlobal;                             /* global alpha enable */
	IPANEL_OUTALPHA_FROM_e enOutAlphaFrom;   /* the out alpha from */
	unsigned char u8SrcAlpha;                                /* valid when enDataConv=IPANEL_DATA_CONV_ALPHA_SRC and src no alpha value such as 1555 0555 565*/
	unsigned char u8DstAlpha;                                /* valid when enDataConv=IPANEL_DATA_CONV_ALPHA_DST and dst no alpha value such as 1555 0555 565*/
	unsigned char u8AlphaGlobal;                           /* valid when bAlphaGlobal=true, global alpha operate. */
	unsigned char u8OutAlpha;                                /* valid when enOutAlphaFrom=IPANEL_OUTALPHA_FROM_REG*/
}IPANEL_ALPHA_s;

typedef struct
{
	IPANEL_DATA_CONV_e enDataConv;     /* operation choice */
	IPANEL_ALPHA_s stAlpha;                      /* alpha attribute */
	IPANEL_ROP_CODE_e enRopCode;         /* ROP type */
	IPANEL_COLORSPACE_s stColorSpace;  /* colorspace attribute */
}IPANEL_OPT_s;

typedef struct
{
	unsigned char *mem_start;
	void *platform_apprach_param;  //phy_addr_start
	unsigned short width;
	unsigned short height;
	unsigned short bytes_per_pixel;
	unsigned short reserved;
}IPANEL_2D_MEM_SURFACE_s;

typedef struct
{
	IPANEL_2D_MEM_SURFACE_s *mem_suf;
	unsigned short x;
	unsigned short y;
	unsigned short w;
	unsigned short h;
}IPANEL_SURFACE_s;

/*****************************************************************************
 Prototype    : ipanel_porting_2D_bitblt
 Description  : add a blit command
 Input        :IPANEL_SURFACE_s *src  source surface
                IPANEL_SURFACE_s *dst  destination surface
                IPANEL_OPT_s *opt     operation attribute
                IPANEL_BOOL sync   1 -sync block return, 0 - async nonblock return immediately
 Output       : None
 Return Value : <0x10000000 操作标示, =0xFFFFFFFF 失败
*****************************************************************************/
unsigned int  ipanel_porting_2D_bitblt(int num, IPANEL_SURFACE_s *src_list, IPANEL_SURFACE_s *dst_list, IPANEL_OPT_s *opt_list, IPANEL_BOOL sync);

/*****************************************************************************
 Prototype    : ipanel_porting_2D_rectangle
 Description  : add a rectangle command
 Input        : IPANEL_SURFACE_s *dst  destination surface
                IPANEL_ARGB_s stFillColor  the draw color
                IPANEL_OPT_s *opt     operation attribute
                IPANEL_BOOL sync   1 -sync block return, 0 - async nonblock return immediately
 Output       : None
 Return Value : <0x10000000 操作标示, =0xFFFFFFFF 失败
*****************************************************************************/
unsigned int ipanel_porting_2D_rectangle(int num, IPANEL_SURFACE_s *dst_list, IPANEL_ARGB_s *fillColor_list, IPANEL_OPT_s *opt_list, IPANEL_BOOL sync);

/*****************************************************************************
 Prototype    : ipanel_porting_get_2D_bitblt_status
 Description  : get the status of the 2d command which is 'handle'
 Input        : unsigned int handle --  ipanel_porting_2D_bitblt return handle
 Output       : None
 Return Value : IPANEL_STATUS_2D_FINISH
                IPANEL_STATUS_2D_BUSY
*****************************************************************************/
IPANEL_STATUS_2D_e ipanel_porting_get_2D_bitblt_status(unsigned int handle);

/*****************************************************************************
 Prototype    : ipanel_porting_alloc_2D_surface
 Description  : alloc a 2D surface accroding to surf->width, surf->height, surf->bytes_per_pixsel
 Input        : IPANEL_2D_MEM_SURFACE_s *surf: valid width, valid height, valid bytes_per_pixsel
 Output       : IPANEL_2D_MEM_SURFACE_s *surf: valid mem_start, valid phy_addr_start
 Return Value : IPANEL_OK success, IPANEL_FAIL fail.
*****************************************************************************/
int ipanel_porting_alloc_2D_surface(IPANEL_2D_MEM_SURFACE_s *surf, IPANEL_SURFACE_TYPE_e type);

/*****************************************************************************
 Prototype    : ipanel_porting_free_2D_surface
 Description  : free a 2D surface memory space include mem_start & phy_addr_start
 Input        : IPANEL_2D_MEM_SURFACE_s *surf: valid mem_start, valid phy_addr_start
 Output       : None
 Return Value : IPANEL_OK success, IPANEL_FAIL fail.
*****************************************************************************/
int ipanel_porting_free_2D_surface(IPANEL_2D_MEM_SURFACE_s *surf);

#ifdef __cplusplus
}
#endif

#endif

