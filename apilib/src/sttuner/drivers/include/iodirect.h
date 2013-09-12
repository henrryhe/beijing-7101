#include "ioarch.h"

extern	STI2C_Handle_t DEMOD_I2CHandle;
#if defined (STTUNER_DRV_SAT_LNB21) || defined (STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
extern	STI2C_Handle_t LNB_I2CHandle;
#endif
ST_ErrorCode_t STTUNER_IOTUNER_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams);
ST_ErrorCode_t STTUNER_IODEMOD_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams);
ST_ErrorCode_t STTUNER_IODEMOD_Close(IOARCH_Handle_t *Handle, STTUNER_IOARCH_CloseParams_t  *CloseParams);
ST_ErrorCode_t STTUNER_IODIRECT_ReadWrite(STTUNER_IOARCH_Operation_t Operation, U16 SubAddr, U8 FieldIndex, U8 FieldLength, U8 *Data, U32 TransferSize, BOOL REPEATER);
ST_ErrorCode_t tuner_repeater_on(void);
#if defined (STTUNER_DRV_SAT_LNB21) || defined (STTUNER_DRV_SAT_LNBH21) || defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
ST_ErrorCode_t STTUNER_IOLNB_Open(IOARCH_Handle_t *Handle, STTUNER_IOARCH_OpenParams_t  *OpenParams);
ST_ErrorCode_t STTUNER_IOLNB_Close(IOARCH_Handle_t *Handle, STTUNER_IOARCH_CloseParams_t  *OpenParams);
#endif

ST_ErrorCode_t ChipDemodGetField( U32 FieldId);
ST_ErrorCode_t ChipDemodSetField( U32 FieldId, U8 Value);
