/*****************************************************************************/
/**
 * File stsectoolnuid.h
 * 
 * Header file for 'Nagra Unique ID' Security Toolkit library. 
 * 
 * Library version: 1.4.0
 * Chip Family: 7109
 * Library name: stsectoolnuid
 * Variant: STD
 * OS platform: BARE
 * 
 * 
 * (C) STMicroelectronics 2007
 *****************************************************************************/


#if !defined(SECTOOLKIT_NUID_H)
#define SECTOOLKIT_NUID_H

/* Include stddefs.h from STAPI project dvdbr-prj-include */
#include <stddefs.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/* Exported Constants ----------------------------------------------- */

#define STSECTOOLNUID_DEVICE_NAME "stsectoolnuid"
#define STSECTOOLNUID_DRIVER_ID 349 /* STAPI reference number */
#define STSECTOOLNUID_DRIVER_BASE (STSECTOOLNUID_DRIVER_ID << 16)

/* Exported types --------------------------------------------------- */

typedef void STSECTOOLNUID_InitParams_t;
typedef void STSECTOOLNUID_TermParams_t;

enum
{
  STSECTOOLNUID_ERROR_OPTION_NOT_SUPPORTED = STSECTOOLNUID_DRIVER_BASE,
  STSECTOOLNUID_ERROR_COMMAND_FAILED
};


/*
 * Standard STAPI functions
 */
ST_ErrorCode_t STSECTOOLNUID_Init(const ST_DeviceName_t DeviceName,
                                   const STSECTOOLNUID_InitParams_t *InitParams,
                                   int interruptLevel);
ST_ErrorCode_t STSECTOOLNUID_Term(const ST_DeviceName_t DeviceName,
                                   const STSECTOOLNUID_TermParams_t *TermParam);
ST_Revision_t STSECTOOLNUID_GetRevision(void);
void STSECTOOLNUID_Identity(char* buffer);


/*
 * Library functions
 */

/*
 * Get the encrypted unique ID for the device.
 * (output) pUniqueId - ptr to 16 byte buffer to hold unique ID. MSB first.
 * (output) pCrc      - ptr to CRC result of pUniqueId
 * (temp)   pBuffer   - ptr to 256 byte non-cached working area.
 * Returns            - ST_NO_ERROR is no error. 
 */
ST_ErrorCode_t STSECTOOLNUID_GetId(U8* pUniqueId, U32* pCrc, U8* pBuffer);

/*
 * Get the unencrypted unique ID for the device.
 * Note: This function must be used AFTER STSECTOOLNUID_GetId.
 * Returns            - unique ID for device
 */
U32 STSECTOOLNUID_GetNuid(void);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(SECTOOLKIT_NUID_H) */
