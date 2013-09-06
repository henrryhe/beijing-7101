/*****************************************************************************/
/**
 * File stsectoolfuse.h
 * 
 * Header file for 'fuse' Security Toolkit library. 
 * 
 * Library version: 1.1.3
 * Chip Family: 7109
 * Library name: stsectoolfuse
 * Variant: STD UTILITY
 * OS platform: BARE
 * 
 * (C) STMicroelectronics 2007
 *****************************************************************************/

#if !defined(SECTOOLKIT_FUSE_H)
#define SECTOOLKIT_FUSE_H

/* Include stddefs.h from STAPI project dvdbr-prj-include */
#include <stddefs.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/* Exported Constants ----------------------------------------------- */

#define STSECTOOLFUSE_DEVICE_NAME "stsectoolfuse"
#define STSECTOOLFUSE_DRIVER_ID 351 /* STAPI reference number */
#define STSECTOOLFUSE_DRIVER_BASE (STSECTOOLFUSE_DRIVER_ID << 16)

/* Exported types --------------------------------------------------- */

typedef void STSECTOOLFUSE_InitParams_t;
typedef void STSECTOOLFUSE_TermParams_t;

#define STSECTOOLFUSE_NOINT (-1)

/* Driver specific error constants */
enum
{
    STSECTOOLFUSE_ERROR_OTP_NOT_ENABLED = STSECTOOLFUSE_DRIVER_BASE,
    STSECTOOLFUSE_ERROR_WRITE_NOT_ENABLED,
    STSECTOOLFUSE_ERROR_PERM_WRITE_NOT_ENABLED,
    STSECTOOLFUSE_ERROR_NOT_OTP_ITEM_ERROR,
    STSECTOOLFUSE_ERROR_OTP_DATA_ERROR,
    STSECTOOLFUSE_ERROR_OTP_WRITE_ERROR
};


/* Fuse item descriptors */
typedef enum STSECTOOLFUSE_ITEM_e
{
    STSECTOOLFUSE_FIRST_ITEM = 0,
    STSECTOOLFUSE_ALL_OTP_ITEM,  
    STSECTOOLFUSE_BULK_OTP_ITEM,  
    STSECTOOLFUSE_GEN_PURPOSE_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_000_0FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_100_1FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_200_2FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_300_3FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_400_4FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_500_5FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_600_6FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKA_700_7FF_OTP_ITEM,  
    STSECTOOLFUSE_MTP_LOCKC_OTP_ITEM,  
    STSECTOOLFUSE_CUSTOMER_DATA_OTP_ITEM,  
    STSECTOOLFUSE_JTAG_PROT_OTP_ITEM,  
    STSECTOOLFUSE_T1F_SECURE_FILT_OTP_ITEM,  
    STSECTOOLFUSE_TKDMA_MODE1_CW_SECURE_OTP_ITEM,  
    STSECTOOLFUSE_CRYPT_CORE2_IFETCH_SRC_RST_OTP_ITEM,  
    STSECTOOLFUSE_CRYPT_CORE1_IFETCH_SRC_RST_OTP_ITEM,  
    STSECTOOLFUSE_CRYPT_CORE0_IFETCH_SRC_RST_OTP_ITEM,  
    STSECTOOLFUSE_CRYPT_SIGCHK_ENABLE_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_DH_DISABLE_SEC_ON_JPU_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_AUD_DISABLE_SEC_ON_JPU_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_SH4_DISABLE_SEC_ON_JPU_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_LX_DH_SPVSRONLY_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_LX_AUD_SPVSRONLY_OTP_ITEM,  
    STSECTOOLFUSE_SNIF_SH4_SPVSRONLY_OTP_ITEM,  
    STSECTOOLFUSE_HWFM_DISABLE_OTP_ITEM,  
    STSECTOOLFUSE_LAST_ITEM
} STSECTOOLFUSE_ITEM_t;


/*
 * Standard STAPI functions
 */
ST_ErrorCode_t STSECTOOLFUSE_Init(const ST_DeviceName_t DeviceName,
                                  const STSECTOOLFUSE_InitParams_t *InitParams,
                                  int interruptLevel);
ST_ErrorCode_t STSECTOOLFUSE_Term(const ST_DeviceName_t DeviceName,
                                  const STSECTOOLFUSE_TermParams_t *TermParam);
ST_Revision_t STSECTOOLFUSE_GetRevision(void);
void STSECTOOLFUSE_Identity(char* buffer);

/*
 * Library functions
 */
ST_ErrorCode_t STSECTOOLFUSE_StartOTP(void);
ST_ErrorCode_t STSECTOOLFUSE_StopOTP(void);
ST_ErrorCode_t STSECTOOLFUSE_ReadItem(const STSECTOOLFUSE_ITEM_t item, U32* const value);
ST_ErrorCode_t STSECTOOLFUSE_ReadSysRegItem(const STSECTOOLFUSE_ITEM_t item, U32* const value);
ST_ErrorCode_t STSECTOOLFUSE_ReadMtpItem(const STSECTOOLFUSE_ITEM_t item, U32* const value);
ST_ErrorCode_t STSECTOOLFUSE_WriteItem(const STSECTOOLFUSE_ITEM_t item, const U32 value);
ST_ErrorCode_t STSECTOOLFUSE_BulkReadItem(const STSECTOOLFUSE_ITEM_t item, const U32 location, const U32 size, U8* const data);
ST_ErrorCode_t STSECTOOLFUSE_BulkWriteItem(const STSECTOOLFUSE_ITEM_t item, const U32 location, const U32 size, const U8* const data);
ST_ErrorCode_t STSECTOOLFUSE_PermanentWriteEnable(const STSECTOOLFUSE_ITEM_t item);
ST_ErrorCode_t STSECTOOLFUSE_PermanentWriteDisable(const STSECTOOLFUSE_ITEM_t item);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* !defined(SECTOOLKIT_FUSE_H) */
