/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the Porting APIs needed by iPanel        */
/*    MiddleWare.                                                             */
/*    Maybe you should modify it accorrding to Platform.                      */
/*                                                                            */
/*    $author Zouxianyun 2005/04/28                                           */
/******************************************************************************/

#ifndef _IPANEL_MIDDLEWARE_PORTING_TUNER_API_FUNCTOTYPE_H_
#define _IPANEL_MIDDLEWARE_PORTING_TUNER_API_FUNCTOTYPE_H_

#include "ipanel_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************PARAMETERS FOR CABLE DELIVERY START***************/
typedef enum
{
    IPANEL_MODULATION_UNDEFINE 	= 0,
    IPANEL_MODULATION_QAM16 	= 1,
    IPANEL_MODULATION_QAM32 	= 2,
    IPANEL_MODULATION_QAM64 	= 3,
    IPANEL_MODULATION_QAM128 	= 4,
    IPANEL_MODULATION_QAM256 	= 5,
    IPANEL_MODULATION_QPSK 		= 6,
    IPANEL_MODULATION_8PSK 		= 7
} IPANEL_MODULATION_MODE_e;

typedef struct 
{
	UINT32_T 						tuningfreqhz; 	/* The LNBs IF output freq In 100Hz */
	UINT32_T  						symbolrate; 	/* symbol rate in Symbols per second */
	IPANEL_MODULATION_MODE_e 		qam;        	/* modulation type:IPANEL_MODULATION_MODE_e */
} IPANEL_DVBC_PARAMS;

/***************PARAMETERS FOR CABLE DELIVERY END***************/


/***************PARAMETERS FOR DVB-T DELIVERY START***************/
/*
** All possible Viterbi code rates for supported demodulators.
*/
typedef enum
{
    IPANEL_VCR_1_2 	= 0,
    IPANEL_VCR_2_3 	= 1,
    IPANEL_VCR_3_4 	= 2,
    IPANEL_VCR_5_6 	= 3,
    IPANEL_VCR_6_7 	= 4,
    IPANEL_VCR_7_8 	= 5,
    IPANEL_VCR_AUTO	= 6
} IPANEL_CODE_RATE_e;

/* Enum the COFDM hierarchy characteristics */
typedef enum
{
    IPANEL_HIERARCHY_UNDEFINE 		= 0,
    IPANEL_COFDM_HIER_NONE  		= 1,
    IPANEL_COFDM_HIER_ALPHA_1 		= 2,
    IPANEL_COFDM_HIER_ALPHA_2 		= 3,
    IPANEL_COFDM_HIER_ALPHA_4 		= 4
}IPANEL_COFDM_HIERARCHY_e;

/* Enum the COFDM bandwidth possibilities */
typedef enum
{
    IPANEL_BANDWIDTH_UNDEFINE 		= 0,
    IPANEL_COFDM_BANDWIDTH_8 		= 1,
    IPANEL_COFDM_BANDWIDTH_7 		= 2,
    IPANEL_COFDM_BANDWIDTH_6 		= 3
}IPANEL_COFDM_BANDWIDTH_e;

/* Enum the COFDM guard intervals  */
typedef enum
{
    IPANEL_GUARDINTERVAL_UNDEFINE 	= 0,
    IPANEL_COFDM_GI_1_32 			= 1,
    IPANEL_COFDM_GI_1_16 			= 2,
    IPANEL_COFDM_GI_1_8 			= 3,
    IPANEL_COFDM_GI_1_4 			= 4
}IPANEL_COFDM_GUARDINTERVAL_e;

/* Enum the COFDM transmission modes */
typedef enum
{
    IPANEL_TRANSMISSIONMODE_UNDEFINE 	= 0,
    IPANEL_COFDM_MODE_2k 				= 1,
    IPANEL_COFDM_MODE_8k 				= 2
}IPANEL_COFDM_TRANSMISSIONMODE_e;

typedef struct
{
    UINT32_T 						tuningFreqHz;	/* Tuning frequency in units of 100Hz */
    IPANEL_COFDM_BANDWIDTH_e 		bandwidth; 
    IPANEL_MODULATION_MODE_e 		modulation; 
    IPANEL_COFDM_HIERARCHY_e 		hierarchyinfo;
    IPANEL_CODE_RATE_e 				hpcode;
    IPANEL_CODE_RATE_e 				lpcode;
    IPANEL_COFDM_GUARDINTERVAL_e 	guardinterval;
    IPANEL_MODULATION_MODE_e		transmode;
} IPANEL_DVBT_PARAMS;

/***************PARAMETERS FOR DVB-T DELIVERY END***************/

/***************PARAMETERS FOR SATELlITE DELIVERY START***************/
/*
** QPSK symbol format.
*/
typedef enum
{
    IPANEL_IMQ_UNDEFINE = 0,
    IPANEL_IMQ_POSITIVE = 1,	/* (I, Q)  */
    IPANEL_IMQ_NEGATIVE = 2,	/* (I, -Q) */
    IPANEL_IMQ_AUTO 	= 3
}IPANEL_IMQ_VALUE_e;

/* type of polarization (sat) */
typedef enum
{
    IPANEL_PLR_UNDEFINE 	= 0,
    IPANEL_PLR_HORIZONTAL 	= 1,
    IPANEL_PLR_VERTICAL   	= 2,
    IPANEL_PLR_LEFT   		= 3,
    IPANEL_PLR_RIGHT   		= 4
}IPANEL_POLARIZATION_e;

typedef struct 
{
    UINT32_T 					tuningfreqhz;   	/* The LNBs IF output freq In 100Hz */
    UINT32_T 					symbolrate;    		/* In symbols/s */
    IPANEL_MODULATION_MODE_e 	modulation;       	
    IPANEL_POLARIZATION_e 		polarization;
    IPANEL_IMQ_VALUE_e 			imqsign;          	/* (i,q) or (i,-q) */
    IPANEL_CODE_RATE_e 			viterbicoderate;  	/* Viterbi code rate */
} IPANEL_DVBS_PARAMS;
/***************PARAMETERS FOR SATELlITE DELIVERY END***************/

typedef enum
{
    IPANEL_TUNER_LOST 		= 0,
    IPANEL_TUNER_LOCKED 	= 1,
    IPANEL_TUNER_LOCKING 	= 2
} IPANEL_TUNER_STATUS_e;

typedef enum
{
    IPANEL_TUNER_GET_QUALITY   	= 1,
    IPANEL_TUNER_GET_STRENGTH  	= 2,
    IPANEL_TUNER_GET_BER       	= 3,
    IPANEL_TUNER_GET_LEVEL     	= 4,
    IPANEL_TUNER_GET_SNR       	= 5,
    IPANEL_TUNER_GET_STATUS 	= 6,
    IPANEL_TUNER_LOCK 			= 7
} IPANEL_TUNER_IOCTL_e;

typedef enum
{
    IPANEL_ACC_NONE 	= 0,
    IPANEL_ACC_DVB_C 	= 1,
    IPANEL_ACC_DVB_S 	= 2,
    IPANEL_ACC_DVB_T 	= 3
} IPANEL_ACCESS_TYPE_e;

typedef struct
{
    UINT32_T 				id;		/* 一个标志号,驱动程序向iPanel MiddleWare发送事件时的参数。*/
    IPANEL_ACCESS_TYPE_e 	type;
    union
    {
        IPANEL_DVBC_PARAMS 	dvbc;
        IPANEL_DVBS_PARAMS 	dvbs;
        IPANEL_DVBT_PARAMS 	dvbt;
    } transport;
} IPANEL_ACC_TRANSPORT;

INT32_T ipanel_porting_tuner_ioctl(INT32_T tunerid, IPANEL_TUNER_IOCTL_e op, VOID *arg);
// next three functions should merge to *_ioctl
INT32_T ipanel_porting_tuner_get_signal_quality(INT32_T tunerid);
INT32_T ipanel_porting_tuner_get_signal_strength(INT32_T tunerid);
INT32_T ipanel_porting_tuner_get_signal_ber(INT32_T tunerid);

INT32_T ipanel_porting_tuner_lock_delivery(INT32_T tunerid, INT32_T frequency,
            INT32_T symbol_rate, INT32_T modulation, INT32_T request_id);
INT32_T ipanel_porting_tuner_get_status(INT32_T tunerid);


#ifdef __cplusplus
}
#endif

#endif//_IPANEL_MIDDLEWARE_PORTING_TUNER_API_FUNCTOTYPE_H_
