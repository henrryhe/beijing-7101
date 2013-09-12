/*******************************************************************************
File Name   : parser.h

Description : H264 Parser API header

Copyright (C) 2004 STMicroelectronics

Date               Modification                                     Name
----               ------------                                     ----
31 Mar 2004        Created                                           OD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __PARSER_H
#define __PARSER_H

/* Includes ----------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "vcodh264.h"

/* Exported Constants ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* CODEC API functions */
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t h264par_GetStatistics(const PARSER_Handle_t ParserHandle, STVID_Statistics_t * const Statistics_p);
#endif
#ifdef STVID_DEBUG_GET_STATUS
static ST_ErrorCode_t h264par_GetStatus(const PARSER_Handle_t ParserHandle, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */
static ST_ErrorCode_t h264par_Init(const PARSER_Handle_t ParserHandle, const PARSER_InitParams_t * const InitParams_p);
static ST_ErrorCode_t h264par_Stop(const PARSER_Handle_t ParserHandle);
#ifdef STVID_DEBUG_GET_STATISTICS
static ST_ErrorCode_t h264par_ResetStatistics(const PARSER_Handle_t ParserHandle, const STVID_Statistics_t * const Statistics_p);
#endif
static ST_ErrorCode_t h264par_Start(const PARSER_Handle_t  ParserHandle, const PARSER_StartParams_t * const StartParams_p);
static ST_ErrorCode_t h264par_GetPicture(const PARSER_Handle_t ParserHandle, const PARSER_GetPictureParams_t * const GetPictureParams_p);
static ST_ErrorCode_t h264par_Term(const PARSER_Handle_t ParserHandle);
static ST_ErrorCode_t h264par_InformReadAddress(const PARSER_Handle_t ParserHandle, const PARSER_InformReadAddressParams_t * const InformReadAddressParams_p);
static ST_ErrorCode_t h264par_GetBitBufferLevel(const PARSER_Handle_t ParserHandle, PARSER_GetBitBufferLevelParams_t * const GetBitBufferLevelParams_p);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __PARSER_H */

/* End of parser.h */
