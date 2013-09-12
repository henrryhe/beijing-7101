/*****************************************************************************
File Name   : atapitest.h

Description : STATAPI test harness header file.

Copyright (C) 2000 STMicroelectronics

Reference   :

ST API Definition "STATAPI Driver API" DVD-API-22 Revision 0.1
*****************************************************************************/

#ifndef __ATAPITEST_H
#define __ATAPITEST_H

/* Added when porting for 8010 */
#include "sttbx.h"

#if !defined(DISABLE_TBX)
#define ATAPI_PRINT(__X__)          STTBX_Print(__X__);
#else
#define ATAPI_PRINT(__X__)          printf __X__;
#endif


#define ATAPI_DEBUG_PRINT(__X__) 	ATAPI_PRINT(("%s\n", __X__))

/* Drives control  */
typedef struct
{
    BOOL  Present;
    STATAPI_DriveType_t Type;
    STATAPI_Handle_t   Handle;
} ATAPI_Drive_t;


/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} ATAPI_TestResult_t;

/* Error messages */
typedef struct
{
    ST_ErrorCode_t Error;
    char ErrorMsg[32];
} ATAPI_ErrorMessage;

/* HDD Info */
typedef struct {

    BOOL   Removable;
    U16    Heads;
    U16    Cylinders;
    U16    CurrentCylinders;
    U16    SectorPerTrack;
    U8     DRQBlockSize;
    U16    CurrentSectorPerTrack;
    U32    AddressableSectors;
    U32    Capacity;
    U16    CurrentHeads;
    char   SerialNumber[21];
    char   ModelNumber[41];
    U16    QueueDepth;
    U16    MWDMA;
    U16    UDMA;
    U16    PIO;

} ATAPI_DeviceInfo_t;

typedef struct
{
    BOOL LBA;
    union{
        struct CHS_Value_s{
            U8  CylHigh;
            U8  CylLow;
            U8  Head;
            U8  Sector;
            }CHS_Value;
         U32    LBA_Value;
        }Value;

} ATAPI_Address_t;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32 Ref;
    STATAPI_DeviceAddr_t   DevAddress;

} ATAPI_TestParams_t;

/* Defines a test harness function */
struct ATAPI_TestEntry_s
{
    ATAPI_TestResult_t (*TestFunction)(ATAPI_TestParams_t *);
    char TestInfo[50];
    U32 RepeatCount;

};
typedef struct ATAPI_TestEntry_s ATAPI_TestEntry_t;
typedef struct UART_TestEntry_s UART_TestEntry_t;

#define TEST_RESULT_ZERO    {0,0}
#define ATAPI_TestPassed(x) x.NumberPassed++; STTBX_Print(("Result: **** PASS **** \n"))
#define ATAPI_TestFailed(x,reason) x.NumberFailed++; STTBX_Print(("Result: !!!! FAIL !!!! (%s)\n", reason))

#endif
