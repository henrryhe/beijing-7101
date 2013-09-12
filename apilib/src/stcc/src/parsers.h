/*******************************************************************************

File name   : parsers.h

Description : stcc parsers manager header file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
20 July 2001        Created                                     Michel Bruant 
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STCC_PARSERS_H
#define __STCC_PARSERS_H

/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Functions ------------------------------------------------------- */

void STCC_ParseEIA708(stcc_Device_t * const Device_p, U32 i );
void STCC_ParseDTVvid21(stcc_Device_t * const Device_p,U32 i ,BOOL FlagDirecTV);
void STCC_ParseUdata130(stcc_Device_t * const Device_p,  U32 i );
void STCC_ParseDVS157(stcc_Device_t * const Device_p,  U32 i );
STCC_FormatMode_t STCC_Detect(stcc_Device_t * const Device_p,  U32 i );

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCC_PARSERS_H */

/* End of parsers.h */

