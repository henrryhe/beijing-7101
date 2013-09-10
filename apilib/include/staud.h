/******************************************************************************/
/*                                                                            */
/* File name   : STAUD.H                                                      */
/*                                                                            */
/* Description : Header of the STAUD                                          */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

#ifndef _STAUD_WRAPPER_H_
#define _STAUD_WRAPPER_H_

#if defined(ST_5105)||defined(ST_5107)
#include "staudlt.h"
#endif
#if defined(ST_5100)||defined(ST_5301)||defined(ST_5528)||defined(ST_7710)
#include "staudmmdsp.h"
#endif
#if defined(ST_7100)||defined(ST_7109)
#include "staudlx.h"
#endif
#endif




