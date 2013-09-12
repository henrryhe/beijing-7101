/*******************************************************************************

File name   : stvout.c

Description : VOUT driver module.

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                     Name
----               ------------                                     ----
17 Jul 2000        Created                                          JG
13 Sep 2001        Add support of ST40GX1                           HSdLM
06 Dec 2001        Add Device Type for Digital output of STi5514    HSdLM
23 Apr 2002        New DeviceType for STi7020                       HSdLM
21 Aug 2003        New s/w hal design                               HSdLM
02 Jun 2004        Add Support for 7710                             AC
13 Sep 2004        Add ST40/OS21 support                            MH
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stdevice.h"

#ifdef ST_OSLINUX
#include "stvout_core.h"
#endif    /* ST_OSLINUX */

#include "sttbx.h"

#include "stdenc.h"
#include "stvout.h"
#include "vout_rev.h"
#include "vout_drv.h"
#include "denc_out.h"
#include "dac_out.h"

#ifdef STVOUT_HDOUT
#include "analog.h"
#include "digital.h"
#endif /* STVOUT_HDOUT */

#ifdef STVOUT_HDDACS
#include "analog.h"
#endif   /* STVOUT_HDDACS */

#ifdef STVOUT_SDDIG
#include "sd_dig.h"
#endif /* STVOUT_SDDIG */

#ifdef STVOUT_HDMI
#include "hdmi_api.h"
#endif /*STVOUT_HDMI*/

/* Private Constants -------------------------------------------------------- */
#define INVALID_DEVICE_INDEX (-1)

/* available cases : */
/* v3i2c :
          RGB  Y/C  CVBS
     RGB   pb   pb   ok
     Y/C   pb   pb   ok
     CVBS  ok   ok   pb     */

static const BOOL CheckOutDencV3_5[15][15] =
{               /* 0      RGB     YUV     Y/C   CVBS   SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV HDMI_888 HDMI_444 HDMI_422 */
/* 0   */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB */       {FALSE,  FALSE,  FALSE,  FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* YUV */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* Y/C */       {FALSE,  FALSE,  FALSE,  FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*CVBS */       {FALSE,  TRUE ,  FALSE,  TRUE , FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_8    */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 888_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_RGB   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_YUV   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_888*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_444*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_422*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE}
};

/* STi55XX :
          RGB  YUV  Y/C  CVBS
     RGB   pb   pb   ok   ok
     YUV   pb   pb   ok   ok
     Y/C   ok   ok   pb   ok
     CVBS  ok   ok   ok   pb    */

#if defined(ST_5528)||defined (ST_5100)
/* GNBvd35850 : Enable two CVBS and two YC outputs at the same time
  For 5528 and 5100...*/
static const BOOL CheckOutDenc[15][15] =
{              /* 0       RGB     YUV     Y/C   CVBS   SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV HDMI_888 HDMI_444 HDMI_422  */
/* 0   */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB */       {FALSE,  FALSE,  FALSE,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* YUV */       {FALSE,  FALSE,  FALSE,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* Y/C */       {FALSE,  TRUE ,  TRUE ,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*CVBS */       {FALSE,  TRUE ,  TRUE ,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_8    */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 888_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_RGB   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_YUV   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_888*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_444*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_422*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE}
};
#else
static const BOOL CheckOutDenc[15][15] =
{              /* 0       RGB     YUV     Y/C   CVBS   SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV HDMI_888 HDMI_444 HDMI_422  */
/* 0   */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB */       {FALSE,  FALSE,  FALSE,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* YUV */       {FALSE,  FALSE,  FALSE,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* Y/C */       {FALSE,  TRUE ,  TRUE ,  FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*CVBS */       {FALSE,  TRUE ,  TRUE ,  TRUE , FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_8    */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 888_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_RGB   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_YUV   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_888*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_444*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_422*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE}
};
#endif

static const BOOL CheckOutGx1[15][15] =
{              /* 0       RGB     YUV     Y/C   CVBS   SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV HDMI_888 HDMI_444 HDMI_422  */
/* 0   */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB */       {FALSE,  TRUE ,  FALSE,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* YUV */       {FALSE,  FALSE,  TRUE ,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* Y/C */       {FALSE,  TRUE ,  TRUE ,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*CVBS */       {FALSE,  TRUE ,  TRUE ,  TRUE , TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_8    */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 888_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_RGB   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* HD_YUV   */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_888*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_444*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_422*/    {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE}
};


/* Sti7015/20 :
          RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM
      RGB  pb   pb    pb   pb     ok     ok     ok    ok    ok    ok    ok
      YUV  pb   pb    pb   pb     ok     ok     ok    ok    ok    ok    ok
      Y/C  pb   pb    pb   ok     ok     ok     ok    ok    ok    ok    ok
     CVBS  pb   pb    ok   pb     ok     ok     ok    ok    ok    ok    ok
   HD_RGB  ok   ok    ok   ok     pb     pb     ok    ok    ok    ok    ok
   HD_YUV  ok   ok    ok   ok     pb     pb     ok    ok    ok    ok    ok
     DIG1  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok
     DIG2  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok
     DIG3  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok
     DIG4  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok
      SVM  ok   ok    ok   ok     ok     ok     ok    ok    ok    ok    pb  */

static const BOOL CheckOut7015[15][15] =  /* also valid for 7020 */
{/*                0     RGB    YUV    Y/C   CVBS    SVM  444_24 422_16 422_8   RGB888 HDRGD  HDYUV HDMI_888 HDMI_444 HDMI_422*/
/* 0      */    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE ,FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*  RGB   */    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE , TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/*  YUV   */    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE , TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/*  Y/C   */    {FALSE, FALSE, FALSE, FALSE, TRUE , TRUE , TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/* CVBS   */    {FALSE, FALSE, FALSE, TRUE , FALSE, TRUE , TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/*  SVM   */    {FALSE, TRUE , TRUE , TRUE , TRUE , FALSE, TRUE , TRUE , TRUE,  TRUE,  TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/* 444_24 */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/* 444_16 */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/* 444_8  */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/* RGB888 */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,   FALSE,  FALSE, FALSE},
/*  HDRGB */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*  HDYUV */    {FALSE, TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , TRUE , FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_888*/    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_444*/    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE,  FALSE, FALSE},
/*HDMI_422*/    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE,  FALSE, FALSE}
};
static const BOOL CheckOut7710 [15][15] =
{ /*              0       RGB     YUV     Y/C    CVBS   SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV HDMI_888 HDMI_444 HDMI_422 */
/* 0        */  {FALSE,  FALSE,  FALSE,  FALSE,  FALSE,FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB   */     {FALSE,  TRUE,   FALSE,  TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* YUV   */     {FALSE,  FALSE,  TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* Y/C      */  {FALSE,  TRUE,   FALSE,  TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  FALSE, FALSE,  TRUE,   TRUE,   TRUE},
/* CVBS     */  {FALSE,  FALSE,  TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  FALSE, FALSE,  TRUE,   TRUE,   TRUE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 422_8    */  {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 888_24*/     {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* HD_RGB   */  {FALSE,  TRUE,   FALSE,  TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  FALSE,  TRUE,   TRUE,   TRUE},
/* HD_YUV   */  {FALSE,  FALSE,  TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  FALSE, FALSE,  TRUE,   TRUE,   TRUE},
/*HDMI_888*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_444*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_422*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE}
};

/* Change authorized configurations */
static const BOOL CheckOut7100 [15][15] =
{ /*              0       RGB     YUV    Y/C    CVBS    SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV  HDMI_888 HDMI_444 HDMI_422*/
/* 0        */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB   */     {FALSE,  TRUE,   FALSE,  TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* YUV   */     {FALSE,  FALSE,  TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* Y/C      */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* CVBS     */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 422_8    */  {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 888_24*/     {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* HD_RGB   */  {FALSE,  TRUE,   FALSE,  TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  FALSE,  TRUE,   TRUE,   TRUE},
/* HD_YUV   */  {FALSE,  FALSE,  TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   FALSE, FALSE,  TRUE,   TRUE,   TRUE},
/*HDMI_888*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_444*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_422*/    {FALSE,  TRUE,   TRUE,   TRUE,   TRUE, FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE}
};

/* Change authorized configurations */
static const BOOL CheckOut7200 [15][15] =
{ /*              0       RGB     YUV    Y/C    CVBS    SVM   444_24 422_16 422_8  888_24  HD_RGB HD_YUV  HDMI_888 HDMI_444 HDMI_422*/
/* 0        */  {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* RGB   */     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* YUV   */     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* Y/C      */  {FALSE,  FALSE,  FALSE,  FALSE, TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* CVBS     */  {FALSE,  FALSE,  FALSE,  TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* SVM */       {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 444_24*/     {FALSE,  FALSE,  FALSE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  FALSE, FALSE,  FALSE,  FALSE, FALSE},
/* 422_16   */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 422_8    */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* 888_24*/     {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/* HD_RGB   */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE, FALSE,  TRUE,   TRUE,   TRUE},
/* HD_YUV   */  {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   FALSE, FALSE,  TRUE,   TRUE,   TRUE},
/*HDMI_888*/    {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_444*/    {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE},
/*HDMI_422*/    {FALSE,  TRUE,   TRUE,   TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE,  TRUE,   TRUE,  TRUE,   TRUE,   TRUE,   TRUE}
};

/* Private Variables (static)------------------------------------------------ */

static stvout_Device_t *DeviceArray;
static semaphore_t * InstancesAccessControl_p=NULL;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;

#ifdef ST_OSLINUX
#define VOUT_DENCVTGDVOVOS_MAPPING    	0x19011000
/*define the HDMI and HDCP Offsets*/
#if defined(STVOUT_HDMI)
#define HDMI_OFFSET                     0x00000800
#define HDCP_OFFSET                     0x00000400
#endif /*STVOUT_HDMI*/
#endif

/* Global Variables --------------------------------------------------------- */
/* not static because used in IsValidHandle macro */
stvout_Unit_t *stvout_UnitArray;
#ifdef VOUT_DEBUG
stvout_Device_t * stvout_DbgDev = DeviceArray;
stvout_Unit_t * stvout_DbgPtr = stvout_UnitArray;
#endif

/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    extern  U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif
/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stvout_Device_t * const Device_p, const STVOUT_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Open(stvout_Unit_t * const Unit_p);
static ST_ErrorCode_t Close(stvout_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(stvout_Device_t * const Device_p);

static ST_ErrorCode_t TestAvailableVout( const STVOUT_InitParams_t * const InitParams_p, U8 DencVersion );
static ST_ErrorCode_t TestOutputValidity( const STVOUT_InitParams_t * const InitParams_p ) ;
static int BitPos( int n);
static ST_ErrorCode_t TestOutputCompatibilty( const STVOUT_InitParams_t * const InitParams_p, U8 DencVersion );
static ST_ErrorCode_t SetOutputType( stvout_Device_t *Device);
static void GetCapability( stvout_Device_t *Device_p, STVOUT_Capability_t *Capability_p);

/* Functions ---------------------------------------------------------------- */

/*
******************************************************************************
Private Functions
******************************************************************************
*/

/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{

if (InstancesAccessControl_p == NULL)
{
    semaphore_t * TemporaryInterInstancesLockingSemaphore_p;
    BOOL          TemporarySemaphoreNeedsToBeDeleted;

    TemporaryInterInstancesLockingSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);

    /* We do not want to call semaphore_create within the task_lock/task_unlock boundaries because
        * semaphore_create internally calls semaphore_wait and can cause normal scheduling to resume,
        * therefore unlocking the critical region */
    STOS_TaskLock();
    if (InstancesAccessControl_p == NULL)
    {
        InstancesAccessControl_p = TemporaryInterInstancesLockingSemaphore_p;
        TemporarySemaphoreNeedsToBeDeleted = FALSE;
    }
    else
    {
        /* Remember to delete the temporary semaphore, if the InstancesAccessControl_p
            * was already created by another video instance */
        TemporarySemaphoreNeedsToBeDeleted = TRUE;
    }
    STOS_TaskUnlock();
    /* Delete the temporary semaphore */
    if(TemporarySemaphoreNeedsToBeDeleted)
    {
        STOS_SemaphoreDelete(NULL, TemporaryInterInstancesLockingSemaphore_p);
    }
}

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() function */


/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */

#if defined (ST_OSWINCE)
/*******************************************************************************
Name        : MapAnalogVOS
Description : Maps VOS "analog" registers
Parameters  : pointer on physical base address of registers
Returns     : ST_ErrorCode_t
*******************************************************************************/
static ST_ErrorCode_t MapAnalogVOS(void** baseAddress_pp)
{

    WCE_VERIFY(*baseAddress_pp == VOUT_BASE_ADDRESS);
    *baseAddress_pp = MapPhysicalToVirtual(*baseAddress_pp, VOUT_ANALOG_ADDRESS_WIDTH);
	if (*baseAddress_pp == NULL)
	{
		WCE_ERROR("MapAnalogVOS()");
		return ST_ERROR_BAD_PARAMETER;
	}
    return ST_NO_ERROR;
}

/*******************************************************************************
Name        : MapDigitalVOS
Description : Maps VOS HDMI & HDCP registers
Parameters  :
    baseAddress_pp       : pointer on physical base address of HDMI
    secondBaseAddress_pp : pointer on physical base address of HDCP
Returns     : ST_ErrorCode_t
*******************************************************************************/
static ST_ErrorCode_t MapDigitalVOS(void** baseAddress_pp, void** secondBaseAddress_pp)
{
    WCE_VERIFY(*baseAddress_pp == VOUT_HDMI_BASE_ADDRESS);
    WCE_VERIFY(*secondBaseAddress_pp == VOUT_HDCP_BASE_ADDRESS);
    *baseAddress_pp = MapPhysicalToVirtual(*baseAddress_pp, VOUT_HDMI_ADDRESS_WIDTH);
	if (*baseAddress_pp == NULL)
	{
		WCE_ERROR("MapDigitalVOS(HDMI)");
		return ST_ERROR_BAD_PARAMETER;
	}
    *secondBaseAddress_pp = MapPhysicalToVirtual(*secondBaseAddress_pp, VOUT_HDCP_ADDRESS_WIDTH);
	if (*baseAddress_pp == NULL)
	{
		WCE_ERROR("MapDigitalVOS(HDCP)");
		return ST_ERROR_BAD_PARAMETER;
	}
    return ST_NO_ERROR;
}
#endif /* ST_OSWINCE*/
/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(stvout_Device_t * const Device_p, const STVOUT_InitParams_t * const InitParams_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR ;
    STDENC_Capability_t   DencCapability;
    STDENC_OpenParams_t   DencOpenParams;
    STDENC_Handle_t       DencHandle;
    ST_DeviceName_t       TmpDencName;
#ifdef STVOUT_HDMI
    HDMI_Handle_t         HdmiHandle=NULL;
#endif /*STVOUT_HDMI*/

    /* Test initialisation parameters and exit if some are invalid. */
    ErrorCode = TestOutputValidity( InitParams_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        return(ErrorCode);
    }
    Device_p->DencVersion = 0;

    /* InitParams exploitation and other actions */
    if ((InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        switch ( InitParams_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /* no break */
            case STVOUT_DEVICE_TYPE_7015 :
                strncpy(TmpDencName, InitParams_p->Target.DencName,sizeof(TmpDencName)-1);
                break;
            case STVOUT_DEVICE_TYPE_7020 :
                strncpy(TmpDencName, InitParams_p->Target.GenericCell.DencName,sizeof(TmpDencName)-1);
                break;
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
                strncpy(TmpDencName, InitParams_p->Target.EnhancedCell.DencName,sizeof(TmpDencName)-1);
                break;
            case STVOUT_DEVICE_TYPE_V13 : /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_5525 : /*no break*/
            case STVOUT_DEVICE_TYPE_4629 : /*no break*/
            case STVOUT_DEVICE_TYPE_7710 : /*no break*/
            case STVOUT_DEVICE_TYPE_7100 :
                strncpy(TmpDencName, InitParams_p->Target.DualTriDacCell.DencName,sizeof(TmpDencName)-1);
                break;
            case STVOUT_DEVICE_TYPE_7200 :
                strncpy(TmpDencName, InitParams_p->Target.EnhancedDacCell.DencName,sizeof(TmpDencName)-1);
                break;
            case STVOUT_DEVICE_TYPE_GX1 :
                strncpy(TmpDencName, InitParams_p->Target.GenericCell.DencName,sizeof(TmpDencName)-1);
                break;

            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
            default:
                return(ST_ERROR_BAD_PARAMETER);
                break;
        }
        ErrorCode = STDENC_GetCapability( TmpDencName, &DencCapability);
        if ( OK(ErrorCode))
        {
            Device_p->DencVersion = DencCapability.CellId;
        }
        else
        {
            ErrorCode = STVOUT_ERROR_DENC_ACCESS;
        }
    }

    if ( OK(ErrorCode))
    {
        ErrorCode = TestOutputCompatibilty( InitParams_p, Device_p->DencVersion );
    }
    if ( OK(ErrorCode))
    {
        ErrorCode = TestAvailableVout(      InitParams_p, Device_p->DencVersion );
    }

    if ( OK(ErrorCode))
    {
        if ((InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
            (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
            (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
            (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) )
        {
            ErrorCode = STDENC_Open( TmpDencName, &DencOpenParams, &DencHandle);
            if (OK(ErrorCode))
            {
                Device_p->DencHandle = DencHandle;
                strncpy(Device_p->DencName, TmpDencName,sizeof(Device_p->DencName)-1);
                Device_p->DencName[ST_MAX_DEVICE_NAME - 1] = '\0';

                switch ( InitParams_p->DeviceType)
                {
                    case STVOUT_DEVICE_TYPE_DENC : /*no break*/
                    case STVOUT_DEVICE_TYPE_7015 :
                        break;
                    case STVOUT_DEVICE_TYPE_7020 :
                        Device_p->DeviceBaseAddress_p = InitParams_p->Target.GenericCell.DeviceBaseAddress_p;
                        Device_p->BaseAddress_p       = InitParams_p->Target.GenericCell.BaseAddress_p;
                        break;
                    case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
                        Device_p->DacSelect           = InitParams_p->Target.EnhancedCell.DacSelect;
                        break;
                    case STVOUT_DEVICE_TYPE_7710 :
                    case STVOUT_DEVICE_TYPE_7100 :
                        Device_p->DeviceBaseAddress_p = InitParams_p->Target.DualTriDacCell.DeviceBaseAddress_p;
                        Device_p->BaseAddress_p       = InitParams_p->Target.DualTriDacCell.BaseAddress_p;
                        Device_p->DacSelect           = InitParams_p->Target.DualTriDacCell.DacSelect;
                        #ifdef STVOUT_HDDACS
                           Device_p->HD_Dacs         = InitParams_p->Target.DualTriDacCell.HD_Dacs;
                           if(Device_p->HD_Dacs)
                           {
                              Device_p->Format              = InitParams_p->Target.DualTriDacCell.Format;
                           }
                        #endif
                        #if defined (ST_OSWINCE)
                            WCE_VERIFY(Device_p->DeviceBaseAddress_p == NULL);
                            #ifdef STVOUT_HDMI
                            Device_p->SecondBaseAddress_p = NULL; /* we'll use this at unmap time*/
                            #endif
                            ErrorCode = MapAnalogVOS(&(Device_p->BaseAddress_p));
                        #endif /* ST_OSWINCE*/
                       break;
                    case STVOUT_DEVICE_TYPE_7200 :
                        strncpy(Device_p->VTGName, InitParams_p->Target.EnhancedDacCell.VTGName,sizeof(Device_p->VTGName)-1);
                        Device_p->DeviceBaseAddress_p = InitParams_p->Target.EnhancedDacCell.DeviceBaseAddress_p;
                        Device_p->BaseAddress_p       = InitParams_p->Target.EnhancedDacCell.BaseAddress_p;
                        Device_p->DacSelect           = InitParams_p->Target.EnhancedDacCell.DacSelect;
                        #ifdef STVOUT_HDDACS
                            Device_p->HD_Dacs             = InitParams_p->Target.EnhancedDacCell.HD_Dacs;
                            if(Device_p->HD_Dacs)
                            {
                                Device_p->Format           = InitParams_p->Target.EnhancedDacCell.Format;
                            }
                            Device_p->HDFBaseAddress_p    = (void*)((U32)Device_p->BaseAddress_p + (U32)HD_TVOUT_HDF_OFFSET);
                        #endif
                        break;
                    case STVOUT_DEVICE_TYPE_5528 : /* no break */
                    case STVOUT_DEVICE_TYPE_V13 :  /* no break */
                    case STVOUT_DEVICE_TYPE_5525 : /*no break*/
                    case STVOUT_DEVICE_TYPE_4629 :
                        Device_p->DeviceBaseAddress_p = InitParams_p->Target.DualTriDacCell.DeviceBaseAddress_p;
                        Device_p->BaseAddress_p       = InitParams_p->Target.DualTriDacCell.BaseAddress_p;
                        Device_p->DacSelect           = InitParams_p->Target.DualTriDacCell.DacSelect;
                        break;
                    case STVOUT_DEVICE_TYPE_GX1 :
                        Device_p->DeviceBaseAddress_p = InitParams_p->Target.GenericCell.DeviceBaseAddress_p;
                        Device_p->BaseAddress_p       = InitParams_p->Target.GenericCell.BaseAddress_p;
                        break;
                    case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
                    default:
                        return(ST_ERROR_BAD_PARAMETER); /* done yet actually */
                        break;
                } /* switch  */
            } /* if (OK(ErrorCode)) */
            else
            {
                ErrorCode = STVOUT_ERROR_DENC_ACCESS;
            }
        }
        else /* if ANALOG_RGB/YUV/YC/CVBS */
        {
            switch ( InitParams_p->DeviceType)
            {
                case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
                  #ifdef STVOUT_DVO
                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.DVOCell.DeviceBaseAddress_p;
                    Device_p->BaseAddress_p       = InitParams_p->Target.DVOCell.BaseAddress_p;
                    break;
                  #endif
                case STVOUT_DEVICE_TYPE_7015 :
                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.HdsvmCell.DeviceBaseAddress_p;
                    Device_p->BaseAddress_p       = InitParams_p->Target.HdsvmCell.BaseAddress_p;
                    break;
                case STVOUT_DEVICE_TYPE_7710 :
                case STVOUT_DEVICE_TYPE_7100 :
                case STVOUT_DEVICE_TYPE_7200 :
                    switch (InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_HD_ANALOG_YUV :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                               if ((InitParams_p->DeviceType==STVOUT_DEVICE_TYPE_7710 )||
                                        (InitParams_p->DeviceType==STVOUT_DEVICE_TYPE_7100 ))
                                  {
                                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.DualTriDacCell.DeviceBaseAddress_p;
                                    Device_p->BaseAddress_p       = InitParams_p->Target.DualTriDacCell.BaseAddress_p;
                                    #ifdef STVOUT_HDDACS
                                        Device_p->HD_Dacs           = InitParams_p->Target.DualTriDacCell.HD_Dacs;
                                        if(Device_p->HD_Dacs)
                                        {
                                            Device_p->Format        = InitParams_p->Target.DualTriDacCell.Format;
                                        }
                                        Device_p->HDFBaseAddress_p    = (void*)((U32)Device_p->BaseAddress_p + (U32)HD_TVOUT_HDF_OFFSET);
                                    #endif
                                 }
                                 else
                                 {
                                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.EnhancedDacCell.DeviceBaseAddress_p;
                                    Device_p->BaseAddress_p       = InitParams_p->Target.EnhancedDacCell.BaseAddress_p;
                                    #ifdef STVOUT_HDDACS
                                        Device_p->HD_Dacs           = InitParams_p->Target.EnhancedDacCell.HD_Dacs;
                                        if(Device_p->HD_Dacs)
                                        {
                                            Device_p->Format        = InitParams_p->Target.EnhancedDacCell.Format;
                                        }
                                        Device_p->HDFBaseAddress_p    = (void*)((U32)Device_p->BaseAddress_p + (U32)HD_TVOUT_HDF_OFFSET);
                                    #endif
                                 }

                              #if defined (ST_OSWINCE)
                                 WCE_VERIFY(Device_p->DeviceBaseAddress_p == NULL);
                                #ifdef STVOUT_HDMI
                                    Device_p->SecondBaseAddress_p = NULL; /* we'll use this at unmap time*/
                                #endif
                                ErrorCode = MapAnalogVOS(&(Device_p->BaseAddress_p));
                              #endif  /*ST_OSWINCE*/
                              break;
#ifdef STVOUT_HDMI
                        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :  /* no break */
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :

#ifdef STVOUT_CEC_PIO_COMPARE
                             Device_p->DeviceBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.DeviceBaseAddress_p;
                             Device_p->BaseAddress_p       = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p;
                             Device_p->SecondBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.SecondBaseAddress_p;
#else /*STVOUT_CEC_PIO_COMPARE*/
                             Device_p->DeviceBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p;
                             Device_p->BaseAddress_p       = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p;
                             Device_p->SecondBaseAddress_p = InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p;

                             #if defined (ST_OSWINCE)
                                 ErrorCode = MapDigitalVOS(&(Device_p->BaseAddress_p), &(Device_p->SecondBaseAddress_p));
                             #endif  /*ST_OSWINCE*/
#endif /*STVOUT_CEC_PIO_COMPARE*/
                             break;
#endif /*STVOUT_HDMI*/
                        default :
                             break;
                    }
                    break;
                case STVOUT_DEVICE_TYPE_7020 :
                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.GenericCell.DeviceBaseAddress_p;
                    Device_p->BaseAddress_p       = InitParams_p->Target.GenericCell.BaseAddress_p;
                    break;
                case STVOUT_DEVICE_TYPE_5528 : /* no break*/
                case STVOUT_DEVICE_TYPE_V13 :  /* no break */
                case STVOUT_DEVICE_TYPE_5525 : /*no break*/
                case STVOUT_DEVICE_TYPE_4629 :
                    Device_p->DeviceBaseAddress_p = InitParams_p->Target.DualTriDacCell.DeviceBaseAddress_p;
                    Device_p->BaseAddress_p       = InitParams_p->Target.DualTriDacCell.BaseAddress_p;
                    break;
                case STVOUT_DEVICE_TYPE_DENC : /* no break */
                case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
                case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
                default:
                    return(ST_ERROR_BAD_PARAMETER);
                    break;
            } /* switch ( InitParams_p->DeviceType) */
        } /* if ANALOG_RGB/YUV/YC/CVBS */
    } /* if ( OK(ErrorCode)) */

    if (InitParams_p->DeviceType == STVOUT_DEVICE_TYPE_7200)
    {
       switch (InitParams_p->OutputType )
       {
        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
             strncpy(Device_p->VTGName, InitParams_p->Target.EnhancedDacCell.VTGName,sizeof(Device_p->VTGName)-1);
             break;
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
            strncpy(Device_p->VTGName, InitParams_p->Target.HDMICell.VTGName,sizeof(Device_p->VTGName)-1);
             break;
        default:
             break;
       }
    }

#if defined (ST_7100)||defined (ST_7109)
    if ( ErrorCode == ST_NO_ERROR)
    {
        Device_p->SYSCFGBaseAddress_p  =  (void *)(U32)(CFG_BASE_ADDRESS);
    }
#endif
#ifdef ST_OSLINUX
	if ( ErrorCode == ST_NO_ERROR)
	{
		/* Determinating length of mapping */
        Device_p->VoutMappedWidth            = (U32)STVOUT_MAPPING_WIDTH;
        /* Determinating length of SYSCFG mapping */
        Device_p->SYSCFGMappedWidth          =  (U32)(STVOUT_SYSCFG_WIDTH);
        /* base address of VOUT mapping */
        Device_p->VoutMappedBaseAddress_p    =  STLINUX_MapRegion( (void *) (Device_p->BaseAddress_p)      , (U32)(Device_p->VoutMappedWidth), "VOUT");
        if ( Device_p->VoutMappedBaseAddress_p )
	    {
            /** affecting VOUT Base Adress with mapped Base Adress **/
            Device_p->BaseAddress_p = Device_p->VoutMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_SYS;
        }
 #if !defined(ST_7200)&& !defined (ST_7111)&& !defined (ST_7105)

         /* base address of SYSCFG mapping */
        Device_p->SYSCFGMappedBaseAddress_p  =  STLINUX_MapRegion( (void *) (Device_p->SYSCFGBaseAddress_p), (U32)(Device_p->SYSCFGMappedWidth), "SYSCFG");
        if ( Device_p->SYSCFGMappedBaseAddress_p )
	    {
            /** affecting SYS CFG Base Adress with mapped Base Adress **/
            Device_p->SYSCFGBaseAddress_p = Device_p->SYSCFGMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_SYS;
        }
 #endif


#if defined(STVOUT_HDMI)
        /* Determinating length of HDCP and HDMI mapping */
        Device_p->HdmiMappedWidth = (U32)HDMI_MAPPING_WIDTH;
        Device_p->HdcpMappedWidth = (U32)HDCP_MAPPING_WIDTH;
        /* base address of HDCP and HDMI mapping */
        Device_p->HdmiMappedBaseAddress_p = STLINUX_MapRegion( (void *) (Device_p->BaseAddress_p)      , (U32) (Device_p->HdmiMappedWidth), "HDMI");
        if ( Device_p->HdmiMappedWidth )
	    {
            /** affecting HDMI Base Adress with mapped Base Adress **/
            Device_p->BaseAddress_p = Device_p->HdmiMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_SYS;
        }
        Device_p->HdcpMappedBaseAddress_p = STLINUX_MapRegion( (void *) (Device_p->SecondBaseAddress_p), (U32) (Device_p->HdcpMappedWidth), "HDCP");
        if ( Device_p->HdcpMappedWidth )
	    {
            /** affecting HDCP Base Adress with mapped Base Adress **/
            Device_p->SecondBaseAddress_p = Device_p->HdcpMappedBaseAddress_p;
	    }
        else
        {
             ErrorCode =  STVTG_ERROR_MAP_SYS;
        }

#endif /*STVOUT_HDMI*/
    }
#endif /* ST_OSLINUX */

    if ( OK(ErrorCode) )
    {
        Device_p->DeviceType = InitParams_p->DeviceType;
        Device_p->OutputType = InitParams_p->OutputType;
    }
    Device_p->AuxNotMain = FALSE;

    switch (Device_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_V13 : /* no break */
        case STVOUT_DEVICE_TYPE_5528: /* no break */
        case STVOUT_DEVICE_TYPE_5525 : /*no break*/
        case STVOUT_DEVICE_TYPE_4629:
             if ((Device_p->OutputType!=STVOUT_OUTPUT_ANALOG_CVBS)&& (Device_p->OutputType != STVOUT_OUTPUT_ANALOG_YC)\
                &&(Device_p->OutputType!=STVOUT_OUTPUT_ANALOG_RGB) && (Device_p->OutputType!=STVOUT_OUTPUT_ANALOG_YUV))
             {
                if OK(ErrorCode)
                {
                    ErrorCode = SetOutputType(Device_p);
                }
             }
             break;
       case STVOUT_DEVICE_TYPE_7710 :
       case STVOUT_DEVICE_TYPE_7100 :
       case STVOUT_DEVICE_TYPE_7200 :
             switch (Device_p->OutputType)
             {
                case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS :
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV :
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                #if defined (ST_7109)||defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                #endif
                     if (OK(ErrorCode))
                     {
                        ErrorCode = SetOutputType(Device_p);
                     }
                     break;
#ifdef STVOUT_HDMI
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :  /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                     if  (OK(ErrorCode))
                     {
                        ErrorCode = VOUT_HDMI_Init(InitParams_p,&HdmiHandle);
                     }
                     Device_p->HdmiHandle = HdmiHandle;
                     break;
#endif /* STVOUT_HDMI*/
               default :
                    break;
             }

        break;
        default :
             if (OK(ErrorCode))
             {
                ErrorCode = SetOutputType(Device_p);
             }
        break;
    }

    if ( OK(ErrorCode) && ((Device_p->DeviceType == STVOUT_DEVICE_TYPE_GX1) || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5528)
                                                                            || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_V13)
                                                                            || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_4629)
                                                                            || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_5525)))
    {
        switch(Device_p->DeviceType)
        {
            case STVOUT_DEVICE_TYPE_GX1 :
                 if ((Device_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                    (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
                    (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
                    (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS))
                {
                        stvout_HalSetDACSource(Device_p);
                }
                break;
            case STVOUT_DEVICE_TYPE_5528 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 : /* no break */
            case STVOUT_DEVICE_TYPE_V13  : /* no break*/
            case STVOUT_DEVICE_TYPE_4629 :
                /* Tridac Configuration wasn't done in driver init for STi5528 & STi4629 */
                break;
            default :
                break;
        }
    }

    if ( OK(ErrorCode))
    {
        ErrorCode = stvout_HalDisable( Device_p);
    }

    if ( OK(ErrorCode))
    {
         Device_p->CPUPartition_p = InitParams_p->CPUPartition_p;
         Device_p->VoutStatus = FALSE;
    }

    return(ErrorCode);
} /* End of Init() function */

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(stvout_Unit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (Unit_p->Device_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_7015 : /* no break */
        case STVOUT_DEVICE_TYPE_7020 : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
        case STVOUT_DEVICE_TYPE_5528 : /* no break */
        case STVOUT_DEVICE_TYPE_4629 : /* no break */
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :  /* no break */
        case STVOUT_DEVICE_TYPE_V13 : /* no break */
        case STVOUT_DEVICE_TYPE_5525 :
            break;
        case STVOUT_DEVICE_TYPE_7710 :
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
            switch (Unit_p->Device_p->OutputType)
            {
                case STVOUT_OUTPUT_ANALOG_RGB :
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :/* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                    break;
#ifdef STVOUT_HDMI
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                    ErrorCode = HDMI_Open(Unit_p->Device_p->HdmiHandle);
                    break;
#endif /*STVOUT_HDMI*/
                default :
                    break;
            }
            break;

        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* End of Open() function */

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(stvout_Unit_t * const Unit_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    switch (Unit_p->Device_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC :
        case STVOUT_DEVICE_TYPE_7015 :
        case STVOUT_DEVICE_TYPE_7020 :
        case STVOUT_DEVICE_TYPE_GX1 :
        case STVOUT_DEVICE_TYPE_5528 :
        case STVOUT_DEVICE_TYPE_4629 :
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
        case STVOUT_DEVICE_TYPE_V13 :
        case STVOUT_DEVICE_TYPE_5525 :
            break;
        case STVOUT_DEVICE_TYPE_7710 :
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
            switch (Unit_p->Device_p->OutputType)
            {
                case STVOUT_OUTPUT_ANALOG_RGB :
                case STVOUT_OUTPUT_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_ANALOG_YC :  /* no break */
                case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :/* no break */
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS:
                     break;
#ifdef STVOUT_HDMI
                case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /* no break */
                case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                     ErrorCode = HDMI_Close(Unit_p->Device_p->HdmiHandle);
                     break;
#endif /*STVOUT_HDMI*/
                default :
                    break;
            }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /* End of Close() function */
/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(stvout_Device_t * const Device_p)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    Device_p->VoutStatus = FALSE;

    ErrorCode = stvout_HalDisable(Device_p);

    /* TermParams exploitation and other actions */
    #if defined (STVOUT_HDDACS)
        if(Device_p->HD_Dacs)
        {
            stvout_HalSetUpsamplerOff(Device_p);
        }
    #endif
    if( ErrorCode == ST_NO_ERROR)
    {
        switch (Device_p->OutputType)
        {
            case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                break;
            default :
                ErrorCode = stvout_PowerOffDac(Device_p);
                break;
        }
    }

    if ((Device_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
        (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
        (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
        (Device_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        ErrorCode = STDENC_Close(Device_p->DencHandle);
    }

#ifdef STVOUT_HDMI
    if ((Device_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_RGB888)||
        (Device_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444)||
        (Device_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422))

    {
        ErrorCode = HDMI_Term (Device_p->HdmiHandle);
    }
#endif /*STVOUT_HDMI*/

#if defined (ST_OSWINCE)
    /* unmap the address range(s)*/
    if (Device_p->BaseAddress_p != NULL)
        UnmapPhysicalToVirtual(Device_p->BaseAddress_p);
#ifdef STVOUT_HDMI
    if (Device_p->SecondBaseAddress_p != NULL)
        UnmapPhysicalToVirtual(Device_p->SecondBaseAddress_p);
#endif
#endif /* ST_OSWINCE*/

#if defined (ST_OSLINUX)
 /** VOUT region **/
    STLINUX_UnmapRegion(Device_p->VoutMappedBaseAddress_p,   (U32) (Device_p->VoutMappedWidth));
 /** SYSCFG region **/
    STLINUX_UnmapRegion(Device_p->SYSCFGMappedBaseAddress_p, (U32) (Device_p->SYSCFGMappedWidth));
#if defined (STVOUT_HDMI)
 /** HDMI region **/
    STLINUX_UnmapRegion(Device_p->HdmiMappedBaseAddress_p, (U32) (Device_p->HdmiMappedWidth));
 /** HDMI region **/
    STLINUX_UnmapRegion(Device_p->HdcpMappedBaseAddress_p, (U32) (Device_p->HdcpMappedWidth));
#endif  /* STVOUT_HDMI */

#endif /* ST_OSLINUX */

    return (ERROR(ErrorCode));
} /* End of Term() function */


/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STVOUT_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/* ------------------------------------------------------------------------------
 *   Available Outputs :
 *
 *                         analog outputs          digital outputs   remarks
 *  device         denc outputs        others
 * stv0119    Y C CVBS R G B
 * sti5500    Y C CVBS R G B
 * sti5505    Y C CVBS R G B                       (YC422 8bits)     Dig out via PIO
 * sti5510    Y C CVBS R G B Y U V
 * sti5512    Y C CVBS R G B Y U V                 (YC/YUV 8bits)
 * sti5508    Y C CVBS R G B Y U V                 (YC 8bits)        Dig out via PIO
 * sti5518    Y C CVBS R G B Y U V                 (YC 8bits)        Dig out via PIO
 * sti5514    Y C CVBS R G B Y U V                 YCbCr422_8bits
 * ST40GX1    Y C CVBS R G B Y U V
 * sti7015/20 Y C CVBS               R G B Y U V   YCbCr444_24bits
 *                                                 YCbCr422_16bits
 *                                                 YCbCr422_8bits
 *                                                 RGB888_24bits
 *sti7710     Y C CVBS              R G B Y U V    RGB888_24bits
 *                                                 YCbCr444_24bits
 *                                                 YCbCr422_16bits
 * SD RGB and YUV outputs not available with 7710.
 * SVM output only available with 7015/20
 * ---------------------------------------------------------------------------- */
static ST_ErrorCode_t TestAvailableVout( const STVOUT_InitParams_t * const InitParams_p,
                                         U8                          DencVersion )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch ( InitParams_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
        case STVOUT_DEVICE_TYPE_V13 :
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
        case STVOUT_DEVICE_TYPE_5528 :
        case STVOUT_DEVICE_TYPE_4629 :
            if ( DencVersion > 3)
            {
                if ( !( (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
                        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
                        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) ) )
                {
                    ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
                }
            }
            else
            {
                if ( !( (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
                        (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) ) )
                {
                    ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
                }
            }
            break;

        case STVOUT_DEVICE_TYPE_7015 : /* no break */
        case STVOUT_DEVICE_TYPE_7020 :
            if ( !( (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)  ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_RGB) ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_YUV) ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_SVM)  ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS)  ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED)  ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED)  ||
                    (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS) ) )
            {
                ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
            }
            break;
        case STVOUT_DEVICE_TYPE_7710 :  /* no break */
        case STVOUT_DEVICE_TYPE_7100 :
        case STVOUT_DEVICE_TYPE_7200 :
            if (!( (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_RGB)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_YUV)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED)  ||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED)  ||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS) ||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_RGB888)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444)||
                   (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422)))
            {
               ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
            }

            break;
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
            if ( !( InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED))
            {
                ErrorCode = STVOUT_ERROR_VOUT_NOT_AVAILABLE;
            }
            break;
        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return (ErrorCode);
}

/*-----------------5/23/01 12:10PM------------------
 * test the validity of desired output,
 * input : Init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestOutputValidity(const STVOUT_InitParams_t * const InitParams_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    switch (InitParams_p->DeviceType)
    {
        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
        case STVOUT_DEVICE_TYPE_7015 : /* no break */
        case STVOUT_DEVICE_TYPE_7020 : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* no break */
        case STVOUT_DEVICE_TYPE_5528 : /* no break */
        case STVOUT_DEVICE_TYPE_V13 :  /* no break */
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
        case STVOUT_DEVICE_TYPE_4629 : /* no break */
        case STVOUT_DEVICE_TYPE_7710 : /* no break */
        case STVOUT_DEVICE_TYPE_7100 : /* no break */
        case STVOUT_DEVICE_TYPE_7200 :
             if (!((InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_RGB) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YUV) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_YC) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_CVBS) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_ANALOG_SVM) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_RGB) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_HD_ANALOG_YUV) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_RGB888) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444) ||
                  (InitParams_p->OutputType == STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422)))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return (ErrorCode);
}

/*-----------------5/31/01 3:47PM-------------------
 * BitPos() give bit position in a bit field
 * input : bit field
 * output: bit position (start from 1)
 * restriction: one bit by field.
 * example: field = 0000 1000 => bit position=4
 * --------------------------------------------------*/
static int BitPos( int n)
{
    int p;
    for ( p=0; n!=0; p++, n>>=1);
    return(p);
}

/*-----------------5/23/01 12:13PM------------------
 * test the compatibity between the output desired,
 * and theoutput already programmed
 * input : init parameter
 * output : Error code
 * --------------------------------------------------*/
static ST_ErrorCode_t TestOutputCompatibilty(
                const STVOUT_InitParams_t * const InitParams_p,
                U8 DencVersion )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    int Index;

    switch ( InitParams_p->DeviceType)
    {

        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                /* initialized device */
                if (((DeviceArray[Index].DeviceName[0] != '\0')&&(DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_DENC))\
                   || ((DeviceArray[Index].DeviceName[0] != '\0')&&(DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)))
                {
                    if (( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.DencName)) == 0 )||
                        ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.EnhancedCell.DencName)) == 0 ))
                    {
                        if ( DencVersion <= 5)
                        {
                            if ( !CheckOutDencV3_5 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                            {
                                ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            }
                        }
                        else
                        {
                            if ( !CheckOutDenc [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                            {
                                ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            }
                        }
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_GX1 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_GX1))
                {
                    if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.GenericCell.DencName)) == 0 )
                    {
                        if ( !CheckOutGx1 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                        {
                            ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                        }
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_5528 :
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
        case STVOUT_DEVICE_TYPE_V13 :
        case STVOUT_DEVICE_TYPE_4629 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && ((DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_5528) || (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_4629)
                    ||(DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_V13)||(DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_5525)))
                {
                    if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.DualTriDacCell.DencName)) == 0 )
                    {
                        if ( !CheckOutDenc [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                        {
                            ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                        }
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_7015 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_7015))
                {
                    switch ( InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                        case STVOUT_OUTPUT_ANALOG_YUV :
                        case STVOUT_OUTPUT_ANALOG_YC :
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.DencName)) == 0 )
                            {
                                if ( !CheckOut7015 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        case STVOUT_OUTPUT_ANALOG_SVM :
                        case STVOUT_OUTPUT_HD_ANALOG_RGB :
                        case STVOUT_OUTPUT_HD_ANALOG_YUV :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                            if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HdsvmCell.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HdsvmCell.DeviceBaseAddress_p)
                            {
                                if ( !CheckOut7015 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        default:
                            ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            break;
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_7020 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_7020))
                {
                    switch ( InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                        case STVOUT_OUTPUT_ANALOG_YUV :
                        case STVOUT_OUTPUT_ANALOG_YC :
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                            if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.GenericCell.DencName)) == 0 )
                            {
                                if ( !CheckOut7015 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        case STVOUT_OUTPUT_ANALOG_SVM :
                        case STVOUT_OUTPUT_HD_ANALOG_RGB :
                        case STVOUT_OUTPUT_HD_ANALOG_YUV :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                            if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.GenericCell.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.GenericCell.DeviceBaseAddress_p)
                            {
                                if ( !CheckOut7015 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        default:
                            ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            break;
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_7710 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_7710))
                {
                    switch ( InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB :
                        case STVOUT_OUTPUT_ANALOG_YUV : /* no break*/
                        case STVOUT_OUTPUT_ANALOG_YC :  /* no break*/
                        case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_HD_ANALOG_YUV :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED :
                            if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.DualTriDacCell.DencName)) == 0 )
                            {
                                if ( !CheckOut7710 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;

                        #ifdef STVOUT_HDMI
                        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :/*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                         #ifdef STVOUT_CEC_PIO_COMPARE
                           if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.DeviceBaseAddress_p &&
                                DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p)
                         #else
                           if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p &&
                                DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p)
                         #endif /*STVOUT_CEC_PIO_COMPARE*/
                            {
                                if ( !CheckOut7710 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                       #endif /* STVOUT_HDMI*/
                       default:
                           ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                           break;
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_7100 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_7100))
                {
                    switch ( InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_ANALOG_YUV :  /* no break*/
                        case STVOUT_OUTPUT_ANALOG_YC :  /* no break*/
                        case STVOUT_OUTPUT_ANALOG_CVBS : /* no break */
                            if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.DualTriDacCell.DencName)) == 0 )
                            {
                                if ( !CheckOut7100 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :/* no break*/
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* no break */
                        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                            if ( !CheckOut7100 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                            {
                                ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            }
                            break;
                        #ifdef STVOUT_HDMI
                        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :/*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                         #ifdef STVOUT_CEC_PIO_COMPARE
                           if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.DeviceBaseAddress_p &&
                                DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p)
                         #else /*STVOUT_CEC_PIO_COMPARE*/
                           if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p &&
                                DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p &&
                                DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p)
                         #endif /*STVOUT_CEC_PIO_COMPARE*/
                            {
                                if ( !CheckOut7100 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                       #endif /* STVOUT_HDMI*/
                       default:
                           ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                           break;
                    }
                }
            }
            break;
        case STVOUT_DEVICE_TYPE_7200 :
            for ( Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
            {
                if (   (DeviceArray[Index].DeviceName[0] != '\0') /* initialized device */
                    && (DeviceArray[Index].DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                    switch ( InitParams_p->OutputType)
                    {
                        case STVOUT_OUTPUT_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_ANALOG_YUV :  /* no break*/
                        case STVOUT_OUTPUT_ANALOG_YC :  /* no break*/
                        case STVOUT_OUTPUT_ANALOG_CVBS :
                           if ( (strcmp( DeviceArray[Index].DencName, InitParams_p->Target.EnhancedDacCell.DencName)) == 0 )
                            {
                                if ( !CheckOut7200 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                {
                                    ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                }
                            }
                            break;
                        case STVOUT_OUTPUT_HD_ANALOG_RGB : /* no break */
                        case STVOUT_OUTPUT_HD_ANALOG_YUV : /* no break */
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED :/* no break*/
                        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* no break */
                        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
                            if ( !CheckOut7200 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                            {
                                ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                            }
                            break;
                       #ifdef STVOUT_HDMI
                        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :/*no break*/
                        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                           #ifdef STVOUT_CEC_PIO_COMPARE
                                if ( DeviceArray[Index].BaseAddress_p      == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p &&
                                    DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.DeviceBaseAddress_p &&
                                    DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCellOne.BaseAddress_p)
                           #else
                                if ( DeviceArray[Index].BaseAddress_p       == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p &&
                                     DeviceArray[Index].DeviceBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p &&
                                     DeviceArray[Index].SecondBaseAddress_p == InitParams_p->Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p)
                           #endif
                                {
                                    if ( !CheckOut7200 [BitPos(InitParams_p->OutputType)] [BitPos(DeviceArray[Index].OutputType)])
                                    {
                                        ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                                    }
                                }
                                break;
                       #endif /* STVOUT_HDMI*/
                       default:
                           ErrorCode = STVOUT_ERROR_VOUT_INCOMPATIBLE;
                           break;
                    }
                }
             }
             break;
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
            ErrorCode = ST_NO_ERROR; /* only one output, no compatibility issue */
            break;
        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return( ErrorCode);
}


/*-----------------5/23/01 12:15PM------------------
 * programm the hardware
 * --------------------------------------------------*/
static ST_ErrorCode_t SetOutputType( stvout_Device_t *Device_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    void* Address = (void*) ((U32)Device_p->DeviceBaseAddress_p + (U32)Device_p->BaseAddress_p);

    switch ( Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB :
            ErrorCode = stvout_HalSetOutputsConfiguration(
                                Device_p->DencHandle,
                                Device_p->DencVersion,
                                Device_p->DeviceType,
                                Device_p->DacSelect,
                                VOUT_OUTPUTS_RGB );

            #if defined (STVOUT_HDDACS)
              if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)))
              {
                 ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_RGB);
              }
              if(Device_p->HD_Dacs)
              {
                 if ( OK(ErrorCode))
                 {
                     ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_RGB);
                 }
                 if ( OK(ErrorCode))
                 {
                     ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_RGB);
                 }
              }
            #endif
            break;
        case STVOUT_OUTPUT_ANALOG_YUV :
            ErrorCode = stvout_HalSetOutputsConfiguration(
                                Device_p->DencHandle,
                                Device_p->DencVersion,
                                Device_p->DeviceType,
                                Device_p->DacSelect,
                                VOUT_OUTPUTS_YUV );

            #if defined (STVOUT_HDDACS)
              if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)))
              {
                 ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_YUV);
              }
              if(Device_p->HD_Dacs)
              {
                 if ( OK(ErrorCode))
                 {
                     ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_YUV);
                 }
                 if ( OK(ErrorCode))
                 {
                     ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_YUV);
                 }
              }
            #endif

            break;
        case STVOUT_OUTPUT_ANALOG_YC :
            ErrorCode = stvout_HalSetOutputsConfiguration(
                                Device_p->DencHandle,
                                Device_p->DencVersion,
                                Device_p->DeviceType,
                                Device_p->DacSelect,
                                VOUT_OUTPUTS_YC );
           #if defined (STVOUT_HDDACS)
            if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)))
            {
                if ((Device_p->DacSelect & (STVOUT_DAC_1|STVOUT_DAC_2))==(STVOUT_DAC_1|STVOUT_DAC_2))
                {
                    if(Device_p->HD_Dacs)
                    {
                        ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_YC);
                        if ( OK(ErrorCode))
                        {
                            ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_YC);
                        }
                        if ( OK(ErrorCode))
                        {
                            ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_YC);
                        }

                    }
                }
            }
           #endif
            break;
        case STVOUT_OUTPUT_ANALOG_CVBS :
            ErrorCode = stvout_HalSetOutputsConfiguration(
                                Device_p->DencHandle,
                                Device_p->DencVersion,
                                Device_p->DeviceType,
                                Device_p->DacSelect,
                                VOUT_OUTPUTS_CVBS );
           #if defined (STVOUT_HDDACS)
            if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)))
            {
                if ((Device_p->DacSelect & (STVOUT_DAC_3))==(STVOUT_DAC_3)||(Device_p->DacSelect & (STVOUT_DAC_1))==(STVOUT_DAC_1))
                {
                    if(Device_p->HD_Dacs)
                    {
                        ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_CVBS);
                        if ( OK(ErrorCode))
                        {
                            ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_CVBS);
                        }
                        if ( OK(ErrorCode))
                        {
                            ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_CVBS);
                        }

                    }
                }
            }
           #endif
            break;
#if defined (STVOUT_HDOUT)
        case STVOUT_OUTPUT_HD_ANALOG_RGB :
            if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)
               ||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7200)))
            {
                ErrorCode = stvout_HalSetAnalogRgbYcbcr( Address, VOUT_OUTPUTS_HD_RGB);
                if (OK(ErrorCode))
                {
                    ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_HD_RGB);
                }
               #if defined (STVOUT_HDDACS)
                  if(Device_p->HD_Dacs)
                  {
                      if (OK(ErrorCode))
                      {
                          ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_HD_RGB);
                      }
                      if ( OK(ErrorCode))
                      {
                          ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_HD_RGB);
                      }
                  }
               #endif
            }
            break;
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            if ( OK(ErrorCode)&&((Device_p->DeviceType==STVOUT_DEVICE_TYPE_7100)||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7710)
                ||(Device_p->DeviceType==STVOUT_DEVICE_TYPE_7200)))
            {
                ErrorCode = stvout_HalSetAnalogRgbYcbcr( Address, VOUT_OUTPUTS_HD_YCBCR);
                if (OK(ErrorCode))
                {
                    ErrorCode = stvout_HalSetAnalogSDRgbYuv( Address, VOUT_OUTPUTS_HD_YCBCR);
                }
                #if defined (STVOUT_HDDACS)
                  if(Device_p->HD_Dacs)
                  {
                      if (OK(ErrorCode))
                      {
                          ErrorCode = stvout_HalGetUpsampler( Address, VOUT_OUTPUTS_HD_YCBCR);
                      }
                      if ( OK(ErrorCode))
                      {
                          ErrorCode = stvout_HalSetUpsampler( Device_p, VOUT_OUTPUTS_HD_YCBCR);
                      }
                  }
                #endif
            }
            break;
        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS :
            ErrorCode = stvout_HalSetDigitalMainAuxiliary( Address, MAIN);
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalRgbYcbcr( Address, VOUT_OUTPUTS_HD_YCBCR);
            }
            /* no Embedded Sync by default */
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            }
            break;

       case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /* SMPTE274/295 */
            ErrorCode = stvout_HalSetDigitalMainAuxiliary( Address, MAIN);
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalRgbYcbcr( Address, VOUT_OUTPUTS_HD_YCBCR);
            }
            /* no Embedded Sync by default */
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            }
            break;
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS :
            ErrorCode = stvout_HalSetDigitalMainAuxiliary( Address, MAIN);
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalRgbYcbcr( Address, VOUT_OUTPUTS_HD_RGB);
            }
            /* no Embedded Sync by default */
            if ( OK(ErrorCode))
            {
                ErrorCode = stvout_HalSetDigitalEmbeddedSyncHd( Address, FALSE);
            }
            break;
#endif /* #ifdef STVOUT_HDOUT */
#if defined(STVOUT_HDOUT) || defined(STVOUT_SDDIG)
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /* CCIR656 */
#ifdef STVOUT_HDOUT
            ErrorCode = stvout_HalSetDigitalMainAuxiliary( Address, AUXILIARY);

            if ( (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7015) || (Device_p->DeviceType == STVOUT_DEVICE_TYPE_7020))
            {
                if ( OK(ErrorCode))
                {
                    ErrorCode = stvout_HalSetDigitalRgbYcbcr( Address, VOUT_OUTPUTS_HD_YCBCR);
                }
                /* no Embedded Sync by default */
                if ( OK(ErrorCode))
                {
                    ErrorCode = stvout_HalSetDigitalEmbeddedSyncSd( Address, FALSE);
                }
            }
#endif /* STVOUT_HDOUT */
#ifdef STVOUT_SDDIG
            if (Device_p->DeviceType == STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT)
            {
                ErrorCode = stvout_HalSetDigitalOutputCCIRMode( Address, TRUE);
            }
#endif /* STVOUT_SDDIG */
            break;
#endif /* STVOUT_HDOUT || STVOUT_SDDIG */
#ifdef STVOUT_SVM
        case STVOUT_OUTPUT_ANALOG_SVM :
            break;
#endif /* STVOUT_SVM */
        default:
            Address= NULL;
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return( ErrorCode);
}


/* -----------------------------------------------------------------
 * returns informations on the capability supported by the device.
 * input : device (internal driver structure), capability (pointer)
 * output : capability
 * ----------------------------------------------------------------- */

static void GetCapability( stvout_Device_t *Device_p, STVOUT_Capability_t *Capability_p)
{
    Capability_p->AnalogOutputsAdjustableCapable = FALSE;
    Capability_p->SupportedAnalogOutputsAdjustable = 0;
    Capability_p->RGB.Min = 0;
    Capability_p->RGB.Max = 0;
    Capability_p->RGB.Step = 0;
    Capability_p->YC.Min = 0;
    Capability_p->YC.Max = 0;
    Capability_p->YC.Step = 0;
    Capability_p->YCRatio.Min = 0;
    Capability_p->YCRatio.Max = 0;
    Capability_p->YCRatio.Step = 0;
    Capability_p->CVBS.Min = 0;
    Capability_p->CVBS.Max = 0;
    Capability_p->CVBS.Step = 0;
    Capability_p->YUV.Min = 0;
    Capability_p->YUV.Max = 0;
    Capability_p->YUV.Step = 0;
    Capability_p->AnalogPictureControlCapable = FALSE;
    Capability_p->Brightness.Min = 0;
    Capability_p->Brightness.Max = 0;
    Capability_p->Brightness.Step = 0;
    Capability_p->Contrast.Min = 0;
    Capability_p->Contrast.Max = 0;
    Capability_p->Contrast.Step = 0;
    Capability_p->Saturation.Min = 0;
    Capability_p->Saturation.Max = 0;
    Capability_p->Saturation.Step = 0;
    Capability_p->AnalogLumaChromaFilteringCapable = FALSE;
    Capability_p->EmbeddedSynchroCapable = FALSE;

    Capability_p->SelectedOutput = Device_p->OutputType;

    switch ( Device_p->DeviceType )
    {
        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
        case STVOUT_DEVICE_TYPE_V13 :  /* no break */
        case STVOUT_DEVICE_TYPE_5528 : /* no break */
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
        case STVOUT_DEVICE_TYPE_4629 :
            Capability_p->SupportedOutputs =
                    STVOUT_OUTPUT_ANALOG_RGB |
                    STVOUT_OUTPUT_ANALOG_YC |
                    STVOUT_OUTPUT_ANALOG_CVBS ;
            if ( Device_p->DencVersion >= 6)
            {
                Capability_p->SupportedOutputs |=
                        STVOUT_OUTPUT_ANALOG_YUV ;
            }
        break;
        case STVOUT_DEVICE_TYPE_7015 : /* no break */
        case STVOUT_DEVICE_TYPE_7020 :
            Capability_p->SupportedOutputs =
                    STVOUT_OUTPUT_ANALOG_RGB |
                    STVOUT_OUTPUT_ANALOG_YUV |
                    STVOUT_OUTPUT_ANALOG_YC |
                    STVOUT_OUTPUT_ANALOG_CVBS |
                    STVOUT_OUTPUT_ANALOG_SVM |
                    STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS |
                    STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED |
                    STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED |
                    STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS |
                    STVOUT_OUTPUT_HD_ANALOG_RGB |
                    STVOUT_OUTPUT_HD_ANALOG_YUV;
            break;
        case STVOUT_DEVICE_TYPE_7710 :  /* no break */
        case STVOUT_DEVICE_TYPE_7100 :  /* no break */
        case STVOUT_DEVICE_TYPE_7200 :
            Capability_p->SupportedOutputs =
                    STVOUT_OUTPUT_ANALOG_RGB |
                    STVOUT_OUTPUT_ANALOG_YUV|
                    STVOUT_OUTPUT_ANALOG_YC |
                    STVOUT_OUTPUT_ANALOG_CVBS |
#ifdef STVOUT_HDMI
                    STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 |
                    STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 |
                    STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 |
#endif
                    STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED |
                    STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED |
                    STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS |
                    STVOUT_OUTPUT_HD_ANALOG_RGB |
                    STVOUT_OUTPUT_HD_ANALOG_YUV;
            break;
       case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
            Capability_p->SupportedOutputs = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
            break;
        default:
            Capability_p->SupportedOutputs = 0;
            break;
    }

    switch ( Device_p->OutputType)
    {
        case STVOUT_OUTPUT_ANALOG_RGB  : /*no break*/
        case STVOUT_OUTPUT_ANALOG_YUV  : /*no break*/
        case STVOUT_OUTPUT_ANALOG_YC   : /*no break*/
        case STVOUT_OUTPUT_ANALOG_CVBS :
            if ( Device_p->DencVersion >= 6)
            {
                Capability_p->AnalogOutputsAdjustableCapable = TRUE;
                Capability_p->SupportedAnalogOutputsAdjustable =
                        STVOUT_Y |
                        STVOUT_YC_RATIO |
                        STVOUT_CVBS |
                        STVOUT_RGB |
                        STVOUT_YUV ;
                Capability_p->RGB.Min = -8;
                Capability_p->RGB.Max = 7;
                Capability_p->RGB.Step = 1;
                Capability_p->YC.Min = -8;
                Capability_p->YC.Max = 7;
                Capability_p->YC.Step = 1;
                Capability_p->YCRatio.Min = 0;
                Capability_p->YCRatio.Max = 15;
                Capability_p->YCRatio.Step = 1;
                Capability_p->CVBS.Min = -8;
                Capability_p->CVBS.Max = 7;
                Capability_p->CVBS.Step = 1;
                Capability_p->YUV.Min = -8;
                Capability_p->YUV.Max = 7;
                Capability_p->YUV.Step = 1;

                if ( Device_p->DencVersion > 6)
                {
                    Capability_p->SupportedAnalogOutputsAdjustable |=
                        STVOUT_C ;
                    Capability_p->AnalogPictureControlCapable = TRUE;
                    Capability_p->Brightness.Min = 1;
                    Capability_p->Brightness.Max = 254;
                    Capability_p->Brightness.Step = 1;
                    Capability_p->Contrast.Min = -128;
                    Capability_p->Contrast.Max = 127;
                    Capability_p->Contrast.Step = 1;
                    Capability_p->Saturation.Min = 1;
                    Capability_p->Saturation.Max = 254;
                    Capability_p->Saturation.Step = 1;
                }

                if ( Device_p->DencVersion > 9)
                {
                    Capability_p->AnalogLumaChromaFilteringCapable = TRUE;
                }
            }
            break;

        case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS : /*no break*/
#ifdef STVOUT_HDMI
        case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 : /*no break*/
        case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 : /*no break*/
#endif
        case STVOUT_OUTPUT_HD_ANALOG_RGB : /*no break*/
        case STVOUT_OUTPUT_HD_ANALOG_YUV :
            Capability_p->EmbeddedSynchroCapable = TRUE;
            break;
        case STVOUT_OUTPUT_ANALOG_SVM :
            break;

        default:
            Capability_p->AnalogOutputsAdjustableCapable = FALSE;
            Capability_p->AnalogPictureControlCapable = FALSE;
            Capability_p->AnalogLumaChromaFilteringCapable = FALSE;
            Capability_p->EmbeddedSynchroCapable = FALSE;
            break;
    }
#ifdef STVOUT_HDMI
    switch (Device_p->DeviceType )
    {
        case STVOUT_DEVICE_TYPE_DENC : /* no break */
        case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
        case STVOUT_DEVICE_TYPE_GX1 :  /* no break */
        case STVOUT_DEVICE_TYPE_V13 :  /* no break */
        case STVOUT_DEVICE_TYPE_5525 : /* no break */
        case STVOUT_DEVICE_TYPE_5528 : /* no break */
        case STVOUT_DEVICE_TYPE_4629 : /* no break */
        case STVOUT_DEVICE_TYPE_7015 : /* no break */
        case STVOUT_DEVICE_TYPE_7020 : /* no break */
        case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
            Capability_p->HDCPCapable = FALSE;
        case STVOUT_DEVICE_TYPE_7710 :  /* no break */
        case STVOUT_DEVICE_TYPE_7100 :  /* no break */
        case STVOUT_DEVICE_TYPE_7200 :
            Capability_p->HDCPCapable = TRUE;
        default :
            break;
    }
#endif
}

/*
******************************************************************************
Public Functions
******************************************************************************
*/

/*
--------------------------------------------------------------------------------
Get revision of stvout API
--------------------------------------------------------------------------------
*/
ST_Revision_t STVOUT_GetRevision(void)
{

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
     */

    return(VOUT_Revision);
} /* End of STVOUT_GetRevision() function */


/*
--------------------------------------------------------------------------------
Get capabilities of xxxxx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_GetCapability(const ST_DeviceName_t DeviceName, STVOUT_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill capability structure */
            GetCapability( &DeviceArray[DeviceIndex], Capability_p);
        }
    }

    LeaveCriticalSection();

    return(ErrorCode);
} /* End of STVOUT_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise xxxxx driver
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Init(const ST_DeviceName_t DeviceName, const STVOUT_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STVOUT_Init :");
    /* Exit now if parameters are bad */
    if ((InitParams_p == NULL) ||                        /* There must be parameters */
        (InitParams_p->MaxOpen > STVOUT_MAX_OPEN) ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0') ||    /* Device name should not be empty */
        (InitParams_p->CPUPartition_p == NULL)
       )
    {
        stvout_Report( Msg, ST_ERROR_BAD_PARAMETER);
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        /* Allocate dynamic data structure */
        DeviceArray = (stvout_Device_t *) memory_allocate(InitParams_p->CPUPartition_p, STVOUT_MAX_DEVICE * sizeof(stvout_Device_t));
        AddDynamicDataSize(STVOUT_MAX_DEVICE * sizeof(stvout_Device_t));
        if (DeviceArray == NULL)
        {
            /* Error of allocation */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for device structure !"));
            return(ST_ERROR_NO_MEMORY);
        }
        stvout_UnitArray = (stvout_Unit_t *) memory_allocate(InitParams_p->CPUPartition_p, STVOUT_MAX_UNIT * sizeof(stvout_Unit_t));
        AddDynamicDataSize(STVOUT_MAX_UNIT * sizeof(stvout_Unit_t));
        if (stvout_UnitArray == NULL)
        {
            /* Error of allocation */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to allocate memory for unit structure !"));
            return(ST_ERROR_NO_MEMORY);
        }
        for (Index = 0; Index < STVOUT_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
            DeviceArray[Index].OutputType = (STVOUT_OutputType_t)0;
            DeviceArray[Index].DencName[0] = '\0';
            DeviceArray[Index].DeviceBaseAddress_p = NULL;
            DeviceArray[Index].BaseAddress_p = NULL;

        }

        for (Index = 0; Index < STVOUT_MAX_UNIT; Index++)
        {
            stvout_UnitArray[Index].UnitValidity = 0;
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        ErrorCode = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STVOUT_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STVOUT_MAX_DEVICE)
        {
            /* All devices initialised */
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            ErrorCode = Init(&DeviceArray[Index], InitParams_p);

            if (OK(ErrorCode))
            {
                /* Device available and successfully initialised: register device name */
                strncpy(DeviceArray[Index].DeviceName, DeviceName,sizeof(DeviceArray[Index].DeviceName)-1);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxOpen = InitParams_p->MaxOpen;

                sprintf( Msg, "STVOUT_Init Device '%s' (Denc version %d) :",
                        DeviceArray[Index].DeviceName,
                        DeviceArray[Index].DencVersion);
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STVOUT_Init() function */


/*
--------------------------------------------------------------------------------
open ...
(Standard instances management. Driver specific implementations should be put in Open() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Open(const ST_DeviceName_t DeviceName, const STVOUT_OpenParams_t * const OpenParams_p, STVOUT_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STVOUT_Open :");
    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        EnterCriticalSection();

        if (!FirstInitDone)
        {
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if device already initialised and return error if not so */
            DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
            if (DeviceIndex == INVALID_DEVICE_INDEX)
            {
                /* Device name not found */
                ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
            }
            else
            {
                /* Look for a free unit and check all opened units to check if MaxOpen is not reached */
                UnitIndex = STVOUT_MAX_UNIT;
                OpenedUnitForThisInit = 0;
                for (Index = 0; Index < STVOUT_MAX_UNIT; Index++)
                {
                    if ((stvout_UnitArray[Index].UnitValidity == STVOUT_VALID_UNIT) && (stvout_UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                    {
                        OpenedUnitForThisInit ++;
                    }
                    if (stvout_UnitArray[Index].UnitValidity != STVOUT_VALID_UNIT)
                    {
                        /* Found a free handle structure */
                        UnitIndex = Index;
                    }
                }
                if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxOpen) || (UnitIndex >= STVOUT_MAX_UNIT))
                {
                    /* None of the units is free or MaxOpen reached */
                    ErrorCode = ST_ERROR_NO_FREE_HANDLES;
                }
                else
                {
                    *Handle_p = (STVOUT_Handle_t) &stvout_UnitArray[UnitIndex];
                    stvout_UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                    /* API specific actions after opening */
                    ErrorCode = Open(&stvout_UnitArray[UnitIndex]);

                    if (OK(ErrorCode))
                    {
                        /* Register opened handle */
                        stvout_UnitArray[UnitIndex].UnitValidity = STVOUT_VALID_UNIT;
                        sprintf( Msg, "STVOUT_Open Device '%s' :", DeviceName);
                    }
                } /* End found unit unused */
            } /* End device valid */
        } /* End FirstInitDone */

        LeaveCriticalSection();
    }

    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End fo STVOUT_Open() function */


/*
--------------------------------------------------------------------------------
Close ...
(Standard instances management. Driver specific implementations should be put in Close() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Close(STVOUT_Handle_t Handle)
{
    stvout_Unit_t *Unit_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STVOUT_Close :");
    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        ErrorCode = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            ErrorCode = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (stvout_Unit_t *) Handle;

            /* API specific actions before closing */
            ErrorCode = Close(Unit_p);

            /* Close only if no errors */
            if (OK(ErrorCode))
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                sprintf( Msg, "STVOUT_Close Device '%s' :", Unit_p->Device_p->DeviceName);
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STVOUT_Close() function */


/*
--------------------------------------------------------------------------------
Terminate xxxxx driver
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVOUT_Term(const ST_DeviceName_t DeviceName, const STVOUT_TermParams_t *const TermParams_p)
{
    stvout_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    char Msg[100];

    sprintf( Msg, "STVOUT_Term :");
    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        EnterCriticalSection();

        if (!FirstInitDone)
        {
            ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if device already initialised and return error if NOT so */
            DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
            if (DeviceIndex == INVALID_DEVICE_INDEX)
            {
                /* Device name not found */
                ErrorCode = ST_ERROR_UNKNOWN_DEVICE;
            }
            else
            {
                /* Check if there is still 'open' on this device */
    /*            Found = FALSE;*/
                UnitIndex = 0;
                Unit_p = stvout_UnitArray;
                while ((UnitIndex < STVOUT_MAX_UNIT) && (!Found))
                {
                    Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVOUT_VALID_UNIT));
                    Unit_p++;
                    UnitIndex++;
                }

                if (Found)
                {
                    if (TermParams_p->ForceTerminate)
                    {
                        UnitIndex = 0;
                        Unit_p = stvout_UnitArray;
                        while (UnitIndex < STVOUT_MAX_UNIT)
                        {
                            if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STVOUT_VALID_UNIT))
                            {
                                /* Found an open instance: close it ! */
                                ErrorCode = Close(Unit_p);
                                if (ErrorCode != ST_NO_ERROR)
                                {
                                    /* If error: don't care, force close to force terminate... */
                                    ErrorCode = ST_NO_ERROR;
                                }
                                /* Un-register opened handle whatever the error */
                                Unit_p->UnitValidity = 0;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            Unit_p++;
                            UnitIndex++;
                        }
                    } /* End ForceTerminate: closed all opened handles */
                    else
                    {
                        /* Can't term if there are handles still opened, and ForceTerminate not set */
                        ErrorCode = ST_ERROR_OPEN_HANDLE;
                    }
                } /* End found handle not closed */

                /* Terminate if OK */
                if (OK(ErrorCode))
                {
                    /* API specific terminations */
                    ErrorCode = Term(&DeviceArray[DeviceIndex]);
                    /* Don't leave instance semi-terminated: terminate as much as possible */
                    /* free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';
                    DeviceArray[DeviceIndex].OutputType = (STVOUT_OutputType_t)0;
                    sprintf( Msg, "STVOUT_Term Device '%s' :", DeviceName);

                } /* End terminate OK */
            } /* End valid device */
        } /* End FirstInitDone */

        LeaveCriticalSection();
    }

    stvout_Report( Msg, ErrorCode);
    return(ErrorCode);
} /* End of STVOUT_Term() function */


/*-----------------5/23/01 12:06PM------------------
 * display informations, failed or ok.
 * input : string to display, Error code
 * output : none
 * --------------------------------------------------*/
void stvout_Report( char *stringfunction, ST_ErrorCode_t ErrorCode)
{
    char Msg[100];

    sprintf( Msg, stringfunction);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%s   failed, error 0x%x\n", Msg, ErrorCode));
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%s   Done\n", Msg));
    }
}

/* ----------------------------- End of file ------------------------------ */



