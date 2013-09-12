/*
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddefs.h>
#include <stnet.h>

/*
 * These macros provide consistent output and easier error checking.
 */
#define NEW_TEST()	\
	STTBX_Print(("Test %3d ", ++testnr));

#define EXPECT_NOERROR()		\
        if (ST_ErrorCode != ST_NO_ERROR) {				\
            STTBX_Print(("FAILED at %s:%d error = %x, expected %x\n",	\
		__FILE__, __LINE__, ST_ErrorCode, ST_NO_ERROR));	\
            ++failures;							\
	} else								\
	        STTBX_Print(("PASSED\n"));

#define EXPECT_ERROR(ERRCODE)		\
        if (ST_ErrorCode != ERRCODE) {					\
            STTBX_Print(("FAILED at %s:%d error = %x, expected %x\n",	\
		__FILE__, __LINE__, ST_ErrorCode, ERRCODE));		\
            ++failures;							\
	} else								\
	        STTBX_Print(("PASSED\n"));

#define EXPECT(EXPR)	\
	if (!(EXPR)) {		\
            STTBX_Print(("FAILED at %s:%d  on " #EXPR "\n", __FILE__,	\
		__LINE__));						\
            ++failures;							\
	}


/* Return codes from test functions */
#define FATAL	1
#define	OK	0

/* Significant is that the device does exist, just the instance
 * number does not.
 * When the device does not exists, we expect an UNKNOW_DEVICE error.
 * When the device does exists, but the instance does not, we expect an
 * INVALID_HANDLE error.
 */
#define BAD_SB_HANDLE	0x409
#define BAD_DEV_HANDLE	0x809


/* These could be local */
ST_ErrorCode_t ST_ErrorCode;
STNET_InitParams_t InitParams;
STNET_OpenParams_t OpenParams;
STNET_StartParams_t StartParams;
STNET_TermParams_t TermParams;
STNET_Handle_t	Handle0, Handle1;

/* Global counters */
static int testnr = 0;
static int failures = 0;

const static char *name[] = { "NET0", "NET1" };

/*
 *	We have 1 test function for each API call to be tested
 */
int test_init()
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Init\n"));
	/* NULL parameter tests */
	NEW_TEST();
        ST_ErrorCode = STNET_Init(name[0],NULL);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	NEW_TEST();
	/* NULL parameter tests */
        InitParams.DeviceType  = STNET_DEVICE_UNKNOWN;
        ST_ErrorCode = STNET_Init(NULL,&InitParams);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	NEW_TEST();
	/* Bogus device type */
        ST_ErrorCode = STNET_Init(name[0],&InitParams);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	NEW_TEST();
        InitParams.DeviceType  = STNET_DEVICE_RECEIVER_SB_INJECTER;
        ST_ErrorCode = STNET_Init(name[0],&InitParams);
	EXPECT_NOERROR();
	if (ST_ErrorCode != ST_NO_ERROR) rc = FATAL;
	/* NET0 has now been initialized */

	NEW_TEST();
        ST_ErrorCode = STNET_Init(name[0],&InitParams);
	EXPECT_ERROR(ST_ERROR_ALREADY_INITIALIZED);

	NEW_TEST();
        ST_ErrorCode = STNET_Init(name[1],&InitParams);
	EXPECT_ERROR(ST_ERROR_ALREADY_INITIALIZED);

	return(rc);
}

int test_term()
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Term\n"));

	/* NULL parameter tests */
	NEW_TEST();
	ST_ErrorCode = STNET_Term(NULL, &TermParams);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	/* NULL parameter tests */
	NEW_TEST();
	ST_ErrorCode = STNET_Term(name[0], NULL);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Term(name[0], &TermParams);
	EXPECT_NOERROR();
	/* NET0 is now gone */

	/* NET0 is gone */
	NEW_TEST();
	ST_ErrorCode = STNET_Term(name[0], &TermParams);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	/* NET1  never existed */
	NEW_TEST();
	ST_ErrorCode = STNET_Term(name[1], &TermParams);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	return(rc);
}


int test_open(int nr)
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Open\n"));
	/* NULL parameter tests */
	NEW_TEST();
	ST_ErrorCode = STNET_Open(name[0], NULL, &Handle0);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	/* NULL parameter tests */
	NEW_TEST();
	OpenParams.TransmissionProtocol = STNET_PROTOCOL_RTP_TS;
	OpenParams.Connection.ReceiverPort = 1234;
	strcpy(OpenParams.Connection.ReceiverIPAddress, "127.0.0.1");
	sprintf(OpenParams.INJ_HandlerName, "INJ%d", nr);

	ST_ErrorCode = STNET_Open(NULL, &OpenParams, &Handle0);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	/* NULL parameter tests */
	NEW_TEST();
	ST_ErrorCode = STNET_Open(name[0], &OpenParams, NULL);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	/* Use an unitialized device */
	NEW_TEST();
	ST_ErrorCode = STNET_Open(name[1], &OpenParams, &Handle1);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Open(name[0], &OpenParams, &Handle0);
	EXPECT_NOERROR();

	/* Good call */
	NEW_TEST();
	OpenParams.TransmissionProtocol = STNET_PROTOCOL_TS;
	OpenParams.Connection.ReceiverPort = 1235;
	strcpy(OpenParams.Connection.ReceiverIPAddress, "192.168.0.1");
	sprintf(OpenParams.INJ_HandlerName, "INJ%d", nr);
	ST_ErrorCode = STNET_Open(name[0], &OpenParams, &Handle1);
	EXPECT_NOERROR();
	EXPECT(Handle0 != Handle1);

	return(rc);
}

int test_close()
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Close\n"));

	/* Bad handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Close(BAD_SB_HANDLE);
	EXPECT_ERROR(ST_ERROR_INVALID_HANDLE);

	/* Bad handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Close(BAD_DEV_HANDLE);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Close(Handle0);
	EXPECT_NOERROR();

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Close(Handle1);
	EXPECT_NOERROR();

	/* Already closed */
	NEW_TEST();
	ST_ErrorCode = STNET_Close(Handle0);
	EXPECT_ERROR(ST_ERROR_INVALID_HANDLE);

	return(rc);
}

int test_start()
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Start\n"));

	/* Unknown handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Start(BAD_SB_HANDLE, &StartParams);
	EXPECT_ERROR(ST_ERROR_INVALID_HANDLE);

	/* Unknown handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Start(BAD_DEV_HANDLE, &StartParams);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	NEW_TEST();
	ST_ErrorCode = STNET_Start(Handle0, &StartParams);
	EXPECT_NOERROR();

	/* Already started, no error */
	NEW_TEST();
	ST_ErrorCode = STNET_Start(Handle0, &StartParams);
	EXPECT_NOERROR();

	/* OK too */
	NEW_TEST();
	ST_ErrorCode = STNET_Start(Handle0, NULL);
	EXPECT_NOERROR();

	return(rc);
}

int test_stop()
{
	int	rc = OK;

	STTBX_Print(("Tests for STNET_Stop\n"));

	/* No such handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Stop(BAD_SB_HANDLE);
	EXPECT_ERROR(ST_ERROR_INVALID_HANDLE);

	/* No such handle */
	NEW_TEST();
	ST_ErrorCode = STNET_Stop(BAD_DEV_HANDLE);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Stop(Handle0);
	EXPECT_NOERROR();

	/* Good call */
	NEW_TEST();
	ST_ErrorCode = STNET_Stop(Handle0);
	EXPECT_NOERROR();

	return(rc);
}

int test_config(STNET_Handle_t	handle)
{
	int	rc = OK;

	STNET_ReceiverConfigParams_t	ConfigParams;

	STTBX_Print(("Tests for STNET_SetReceiverConfig\n"));
	/* No such handle */
	NEW_TEST();
	ST_ErrorCode = STNET_SetReceiverConfig(BAD_SB_HANDLE, &ConfigParams);
	EXPECT_ERROR(ST_ERROR_INVALID_HANDLE);

	/* No such handle */
	NEW_TEST();
	ST_ErrorCode = STNET_SetReceiverConfig(BAD_DEV_HANDLE, &ConfigParams);
	EXPECT_ERROR(ST_ERROR_UNKNOWN_DEVICE);

	NEW_TEST();
	ST_ErrorCode = STNET_SetReceiverConfig(handle, NULL);
	EXPECT_ERROR(ST_ERROR_BAD_PARAMETER);

	NEW_TEST();
	ConfigParams.WindowSize = 100;
	ConfigParams.WindowTimeout = 200;
	ST_ErrorCode = STNET_SetReceiverConfig(handle, &ConfigParams);
	EXPECT_NOERROR();

	NEW_TEST();
	ConfigParams.WindowSize = 0;
	ST_ErrorCode = STNET_SetReceiverConfig(handle, &ConfigParams);
	EXPECT_NOERROR();
	EXPECT(ConfigParams.WindowSize != 0);
/*	printf("size=%d\n", ConfigParams.WindowSize);
	system("cat /proc/stnet/handles"); */

	return(rc);
}

/* Main code: Call all test functions, bypass some in case of fatal issues. */
main()
{
	int	fatal;

	fatal = test_init();
	if (!fatal) {
		fatal = test_open(0);
		if (!fatal) {
			test_start();
			test_config(Handle0);
			test_stop();
		}
		test_close();
	}
	test_term();

	if (fatal)
		STTBX_Print(("Fatal errors were found, not all tests could"
			" be executed.\n"));
	STTBX_Print(("\n%d failures out of %d tests, %d%% pass rate.\n",
		failures, testnr, ((testnr-failures)*100)/testnr));
	exit(failures);
}
