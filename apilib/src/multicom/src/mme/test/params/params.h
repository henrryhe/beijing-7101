#ifndef PARAMS_H
#define PARAMS_H

#define TRANSPORT_NAME "shm"
#define TEST1_HANDLE_VALUE 815
#define TEST1_INSTANCE_NAME "simpleTest_inst"
#define TEST1_STATUS_DOUBLE 1.0
#define TEST1_STATUS_INT    0x12345678
#define INPUT_SIZE          12

/* Init params */
/* The sublist */

enum	SimpleParamsSubInit {
	MME_OFFSET_SimpleParamsSubInit_valueA,
	MME_OFFSET_SimpleParamsSubInit_valueB,
	MME_OFFSET_SimpleParamsSubInit_valueC,
	MME_LENGTH_SimpleParamsSubInit

#define MME_TYPE_SimpleParamsSubInit_valueA long
#define MME_TYPE_SimpleParamsSubInit_valueB short
#define MME_TYPE_SimpleParamsSubInit_valueC char
};

typedef MME_GENERIC64 SimpleParamsSubInit_t[MME_LENGTH(SimpleParamsSubInit)];

/* The main init list */

#define SimpleParamsInit_instanceNameLength 16

enum	SimpleParamsInit {
	MME_OFFSET_SimpleParamsInit_instanceType,                                        /* some instance id  */
	MME_OFFSET_SimpleParamsInit_instanceName,                                        /* the instance name */
	MME_OFFSET_SimpleParamsInit_values = MME_OFFSET_SimpleParamsInit_instanceName +  /* 16 chars for instanceName */
                                             SimpleParamsInit_instanceNameLength
,
	MME_LENGTH_SimpleParamsInit        = MME_OFFSET_SimpleParamsInit_values +        /* + sizeof last field */
                                             MME_LENGTH_SimpleParamsSubInit + 1

#define MME_TYPE_SimpleParamsInit_instanceType MME_UINT
#define MME_TYPE_SimpleParamsInit_instanceName char
#define MME_TYPE_SimpleParamsInit_values       SimpleParamsSubInit_t
};

typedef MME_GENERIC64 SimpleParamsInit_t[MME_LENGTH(SimpleParamsInit)];

/* Status params (returned to host) */

enum	SimpleParamsStatus {
	MME_OFFSET_SimpleParamsStatus_status1,   /* some status  */
	MME_OFFSET_SimpleParamsStatus_status2,   /* more status */
	MME_LENGTH_SimpleParamsStatus

#define MME_TYPE_SimpleParamsStatus_status1 MME_UINT
#define MME_TYPE_SimpleParamsStatus_status2 double
};

typedef MME_GENERIC64 SimpleParamsStatus_t[MME_LENGTH(SimpleParamsStatus)];

/* Transform params (passed with MME_SendCommand) */

enum	SimpleParamsTransform {
	MME_OFFSET_SimpleParamsTransform_repetitionsA,
	MME_OFFSET_SimpleParamsTransform_repetitionsB,
	MME_LENGTH_SimpleParamsTransform

#define MME_TYPE_SimpleParamsTransform_repetitionsA short
#define MME_TYPE_SimpleParamsTransform_repetitionsB unsigned char
};

typedef MME_GENERIC64 SimpleParamsTransform_t[MME_LENGTH(SimpleParamsTransform)];

#endif
