/**************************************************************************/
/**
 * @file sttkdma.h
 * 
 * @brief sttkdma - Allows flash writes while the cryptocore is performing 
 *                  signature checking
 * 
 * @par &copy; STMicroelectronics 2006
 ***************************************************************************/

#ifndef __STTKDMA_H_
#define __STTKDMA_H_

#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------- */

#define STTKDMA_DEVICE_NAME "sttkdma"

#define STTKDMA_DRIVER_ID 346
#define STTKDMA_DRIVER_BASE (STTKDMA_DRIVER_ID << 16)

/* Exported Types --------------------------------------------------- */

enum
{
  STTKDMA_ERROR_DEVICE_BUSY = STTKDMA_DRIVER_BASE,
  STTKDMA_ERROR_OPTION_NOT_SUPPORTED,
  STTKDMA_ERROR_DMA_FAILED,
  STTKDMA_ERROR_NOT_INITIALIZED,
  STTKDMA_ERROR_COMMAND_FAILED,
  STTKDMA_ERROR_NO_FIRMWARE_LOADED
};


typedef enum STTKDMA_DESChannel_e
{
    STTKDMA_CHANNEL_A,
    STTKDMA_CHANNEL_B,
    STTKDMA_CHANNEL_C
} STTKDMA_DESChannel_t;


typedef enum STTKDMA_Algorithm_e
{
    STTKDMA_ALGORITHM_PLAIN,
    STTKDMA_ALGORITHM_AES,
    STTKDMA_ALGORITHM_TDES
} STTKDMA_Algorithm_t;

typedef enum STTKDMA_AlgorithmMode_e
{
    STTKDMA_ALGORITHM_MODE_ECB,
    STTKDMA_ALGORITHM_MODE_CBC,
    STTKDMA_ALGORITHM_MODE_CTR
} STTKDMA_AlgorithmMode_t;


typedef enum STTKDMA_KeyPolarity_e
{
    STTKDMA_KEY_POLARITY_EVEN,
    STTKDMA_KEY_POLARITY_AUTOMATIC,
    STTKDMA_KEY_POLARITY_ODD
} STTKDMA_KeyPolarity_t;

typedef enum STTKDMA_PacketMode_e
{
    STTKDMA_PACKET_MODE_IEC13818,
    STTKDMA_PACKET_MODE_DSS
} STTKDMA_PacketMode_t;

typedef enum STTKDMA_Command_e
{



    STTKDMA_COMMAND_DECRYPT_FK = 0,

    STTKDMA_COMMAND_DECRYPT_CK = 1,
    STTKDMA_COMMAND_DECRYPT_CW = 2,

    STTKDMA_COMMAND_DECRYPT_CPCW = 3,
    STTKDMA_COMMAND_DECRYPT_HK = 10,
    STTKDMA_COMMAND_DECRYPT_PFA = 11,
    STTKDMA_COMMAND_DECRYPT_SFA = 12,
    STTKDMA_COMMAND_DECRYPT_KEY = 13,
    STTKDMA_COMMAND_DECRYPT_SW = 14,


    STTKDMA_COMMAND_DIRECT_CPCW = 20,

    STTKDMA_COMMAND_DIRECT_CW = 21

} STTKDMA_Command_t;

typedef enum STTKDMA_TKConfigCommand_e
{

    STTKDMA_TKCFG_AES_NOT_TDES_FOR_CW = 0,
    STTKDMA_TKCFG_SECURE_CW_OVERRIDE = 1,
    STTKDMA_TKCFG_OTP_SCK_OVERRIDE = 2,
    STTKDMA_TKCFG_SCK_FOR_CW_OVERRIDE = 3,
    STTKDMA_TKCFG_SCK_ALT_FOR_CPCW_OVERRIDE = 4,
    STTKDMA_TKCFG_ALT_FORMAT = 5,

    STTKDMA_TKCFG_DMA_ALT_FORMAT = 6,
    STTKDMA_TKCFG_LEGACY_KEYS = 7
} STTKDMA_TKConfigCommand_t;


typedef enum STKDMA_Buffer_e
{
   STKDMA_BUFFER_KERNEL_COHERENT = 0,
   STKDMA_BUFFER_USER_SPACE = 1 /* default */
} STKDMA_Buffer_t;

/* back ward compat marco */
#define sck_overrride sck_override

typedef struct STTKDMA_DMAConfig_s
{
    STTKDMA_Algorithm_t algorithm;
    STTKDMA_AlgorithmMode_t algorithm_mode;
    BOOL refresh;
    BOOL decrypt_not_encrypt;
    BOOL sck_override;
    U32 iv_seed[4];
} STTKDMA_DMAConfig_t;

typedef struct STTKDMA_DESDMAConfig_s
{
    STTKDMA_KeyPolarity_t key_polarity;
    STTKDMA_PacketMode_t packet_mode;
    BOOL decrypt_not_encrypt;
    BOOL sck_override;
    U8 header_size;
    U16 payload_size;
    U8 channel;
} STTKDMA_DESDMAConfig_t;

typedef struct STTKDMA_Key_s
{
    U8 key_data[16];
} STTKDMA_Key_t;

/* Exported Variables ----------------------------------------------- */

/* Exported Macros -------------------------------------------------- */

/* Exported Functions ----------------------------------------------- */

/******************************************************************************
 * Create driver resources and initialise the hardware
 *****************************************************************************/
ST_ErrorCode_t STTKDMA_Init(const ST_DeviceName_t DeviceName,
                            int interruptLevel);

/******************************************************************************
 * Return a text string indicating the driver version 
 *****************************************************************************/
ST_Revision_t STTKDMA_GetRevision(void);

/******************************************************************************
 * Release all the driver resources and shut down the driver
 *****************************************************************************/
ST_ErrorCode_t STTKDMA_Term(const ST_DeviceName_t DeviceName);


/******************************************************************************
 * Return customer Mode
 *****************************************************************************/
U32 STTKDMA_CustomerMode(void);

/******************************************************************************
 * Initiate a DMA operation
 *****************************************************************************/
ST_ErrorCode_t STTKDMA_StartDMA(
    STTKDMA_DMAConfig_t* Config,
    U8* SourceBuffer,
    U8* DestBuffer,
    U32 DataSize,
    U8 KeyIndex,
    BOOL Block);

/******************************************************************************
 * Initiate a DES DMA operation
 *****************************************************************************/
ST_ErrorCode_t STTKDMA_StartDESDMA(
    STTKDMA_DESDMAConfig_t* Config,
    U8* SourceBuffer,
    U8* DestBuffer,
    U32 DataSize,
    BOOL Block);

/******************************************************************************
 * Wait for the current DMA operation to complete
 *****************************************************************************/
ST_ErrorCode_t STTKDMA_WaitForDMAToComplete(U32 Timeout, BOOL* Complete);



/*****************************************************************************
 * Clear the CK and FK values in the TKDMA
 ****************************************************************************/
ST_ErrorCode_t STTKDMA_Reset(void);



/*****************************************************************************
 * Read the public section of the embedded key
 ****************************************************************************/
ST_ErrorCode_t STTKDMA_ReadPublicID(STTKDMA_Key_t* Key);



/*****************************************************************************
 * Read the counter from tkdma
 ****************************************************************************/
ST_ErrorCode_t STTKDMA_GetCounter(U32 *HiCounter, U32 *LowCounter, long long *msec);



/*****************************************************************************
 * Configure TK engine overrides
 ****************************************************************************/
ST_ErrorCode_t STTKDMA_ConfigureTK(STTKDMA_TKConfigCommand_t Command, BOOL on);

/****************************************************************************
 * Decrypt the supplied data and store it
 ***************************************************************************/
ST_ErrorCode_t STTKDMA_DecryptKey(STTKDMA_Command_t Command,
    STTKDMA_Key_t* Key,
    U8 Index,
    STTKDMA_Key_t* Offset);

/****************************************************************************
 * Set a DES key / PID pair
 ***************************************************************************/
ST_ErrorCode_t STTKDMA_StoreDESKey(STTKDMA_DESChannel_t Channel,
    U8 Index,
    STTKDMA_Key_t* Key,
    U32 PID);


/****************************************************************************
 * Select whether DMA goes from user or kernel buffers
 ***************************************************************************/
ST_ErrorCode_t STTKDMA_SetBufferType(STKDMA_Buffer_t bufferType);


#ifdef __cplusplus
}
#endif

#endif /* __STTKDMA */
