#ifndef PARAMS_H
#define PARAMS_H

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

#endif
