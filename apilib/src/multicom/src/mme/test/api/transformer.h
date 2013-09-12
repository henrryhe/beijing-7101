#ifndef SIMPLE_TEST_H
#define SIMPLE_TEST_H

#include <mme.h>

#define TRANSPORT_NAME "shm"
#define TEST1_HANDLE_VALUE 815
#define TEST1_INSTANCE_NAME "simpleTest_inst"
#define TEST1_STATUS_DOUBLE 1.0
#define TEST1_STATUS_INT    0x12345678

#define SimpleParamsInit_instanceNameLength 16

enum	SimpleParamsInit {
	MME_OFFSET_SimpleParamsInit_instanceType,                             /* some instance id  */
	MME_OFFSET_SimpleParamsInit_instanceName,                             /* the instance name */
	MME_LENGTH_SimpleParamsInit = MME_OFFSET_SimpleParamsInit_instanceName +  /* 16 chars for instanceName */
                                      SimpleParamsInit_instanceNameLength

#define MME_TYPE_SimpleParamsInit_instanceType MME_UINT
#define MME_TYPE_SimpleParamsInit_instanceName char
};

typedef MME_GENERIC64 SimpleParamsInit_t[MME_LENGTH(SimpleParamsInit)];

enum	SimpleParamsStatus {
	MME_OFFSET_SimpleParamsStatus_status1,   /* some status  */
	MME_OFFSET_SimpleParamsStatus_status2,   /* more status */
	MME_LENGTH_SimpleParamsStatus

#define MME_TYPE_SimpleParamsStatus_status1 MME_UINT
#define MME_TYPE_SimpleParamsStatus_status2 double
};

typedef MME_GENERIC64 SimpleParamsStatus_t[MME_LENGTH(SimpleParamsStatus)];

/*{{{ TransformerCommandParams */
enum TransformerCommandParams {
	MME_OFFSET_TransformerCommandParams_SlowMode,
	MME_LENGTH_TransformerCommandParams

#define MME_TYPE_TransformerCommandParams_SlowMode MME_ERROR
};
typedef MME_GENERIC64 TransformerCommandParams_t[MME_LENGTH(TransformerCommandParams)];
/*}}}*/

MME_ERROR SimpleTest_AbortCommand(void *context, MME_CommandId_t commandId);
MME_ERROR SimpleTest_GetTransformerCapability(MME_TransformerCapability_t * capability);
MME_ERROR SimpleTest_InitTransformer(MME_UINT paramsLength, MME_GenericParams_t params, void** handle);
MME_ERROR SimpleTest_ProcessCommand(void *context, MME_Command_t * commandInfo);
MME_ERROR SimpleTest_TermTransformer(void *context);


#endif
