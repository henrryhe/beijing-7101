/*!
 *******************************************************************************
 * \file mpg2toh264target.c
 *
 * \brief Transcodec MPEG2 to H264 target decoder functions
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "mpg2toh264target.h"

#if !defined ST_OSLINUX
#include "sttbx.h"
#endif

/* Static functions prototypes ---------------------------------------------- */

/* Constants ---------------------------------------------------------------- */
#define LEVELS_TABLE_MAX 15

const U32 LevelsTable[LEVELS_TABLE_MAX][5] /*[index][level][max frame size (MB)][max MBPS][MinCR] [Max bit rate (bits)] */
= {
  {10, 99,    1485,     2,  64000},
  {11, 396,   3000,     2, 192000},
  {12, 396,   6000,     2, 384000},
  {13, 396,   11880,    2, 768000},
  {20, 396,   11880,    2, 2000000},
  {21, 792,   19800,    2, 4000000},
  {22, 1620,  20250,    2, 4000000},
  {30, 1620,  40500,    2, 10000000},
  {31, 3600,  108000,   4, 14000000},
  {32, 5120,  216000,   4, 20000000},
  {40, 8192,  245760,   4, 20000000},
  {41, 8192,  245760,   4, 50000000},
  {42, 8704,  522240,   4, 50000000},
  {50, 22080, 589824,   4, 135000000},
  {51, 36864, 983040,   4, 240000000}
};

/* Target decoder */
const TARGET_FunctionsTable_t TARGET_H264Functions =
{
    mpg2toh264Target_Init,
    mpg2toh264Target_Start,
    mpg2toh264Target_Term,
    mpg2toh264Target_GetPictureMaxSizeInByte
};

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : mpg2toh264Target_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Target_Init(const TARGET_Handle_t TargetHandle, const TARGET_InitParams_t * const InitParams_p)
{
    mpg2toh264target_PrivateData_t * TARGET_Data_p;
    TARGET_Properties_t * TARGET_Properties_p;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = ST_NO_ERROR;
    TARGET_Properties_p = (TARGET_Properties_t *) TargetHandle;

    if ((TargetHandle == NULL) || (InitParams_p == NULL))

    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Allocate TARGET PrivateData structure */
    TARGET_Properties_p->PrivateData_p = (mpg2toh264target_PrivateData_t *) memory_allocate(InitParams_p->CPUPartition_p, sizeof(mpg2toh264target_PrivateData_t));
    TARGET_Data_p = (mpg2toh264target_PrivateData_t *) TARGET_Properties_p->PrivateData_p;

    if (TARGET_Data_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }
    else
    {
        memset(TARGET_Data_p, 0xA5, sizeof(mpg2toh264target_PrivateData_t)); /* fill in data with 0xA5 by default */
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Target_Start
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Target_Start(const TARGET_Handle_t TargetHandle, const TARGET_StartParams_t * const StartParams_p)
{
    mpg2toh264target_PrivateData_t  *TARGET_Data_p;
    TARGET_Properties_t             *TARGET_Properties_p;
    ST_ErrorCode_t                  ErrorCode;

    UNUSED_PARAMETER(StartParams_p);

    ErrorCode = ST_NO_ERROR;
    TARGET_Properties_p = (TARGET_Properties_t *) TargetHandle;
    TARGET_Data_p = (mpg2toh264target_PrivateData_t *) TARGET_Properties_p->PrivateData_p;

    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Target_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t mpg2toh264Target_Term(const TARGET_Handle_t TargetHandle)
{
    mpg2toh264target_PrivateData_t * TARGET_Data_p;
    TARGET_Properties_t * TARGET_Properties_p;
    ST_ErrorCode_t ErrorCode;

    ErrorCode = ST_NO_ERROR;
    TARGET_Properties_p = (TARGET_Properties_t *) TargetHandle;
    TARGET_Data_p = (mpg2toh264target_PrivateData_t *) TARGET_Properties_p->PrivateData_p;

    /* Deallocate private data */
    if (TARGET_Data_p != NULL)
    {
        memory_deallocate(TARGET_Properties_p->CPUPartition_p, (void *) TARGET_Data_p);
    }

    return(ErrorCode);
}

/*******************************************************************************
Name        : mpg2toh264Target_GetPictureMaxSizeInByte
Description : Compute the maximum number of bytes for a picture
Parameters  :
Assumptions :
Limitations :
Returns     : TRUE/FALSE
*******************************************************************************/
U32 mpg2toh264Target_GetPictureMaxSizeInByte(const U32 PictureWidth, const U32 PictureHeight, const U32 TargetFrameRate, const U32 TargetBitRate)
{
    U32 MaxPictureSize;
    U32 Index;
    U32 MBNum;
    U32 MBRate;
    U32 MaxMBPS;
    U32 MinCR;

/* LD : 30/08/2007 : Add Bitrate test to properly compute the level */
/* LD : 30/08/2007 : Modify  MaxPictureSize computation due to overflow problem */

    /* Compute MaxMBPS and MinCR from Width, Height and TargetFrameRate */
    /* See table A-5, H264 specifications */
    MBNum = (PictureWidth+15)/16 * (PictureHeight+15)/16;
    MBRate = (MBNum * TargetFrameRate)/1000;

    /* looking for the minimum level for the input spatial resolution */
    for(Index=0; Index<LEVELS_TABLE_MAX; Index++)
    {
		    if(LevelsTable[Index][1] >= MBNum)
			     break;
    }

    for(; Index<LEVELS_TABLE_MAX; Index++)
    {
		    if(LevelsTable[Index][2] >= MBRate)
		      break;
    }

    for(; Index < LEVELS_TABLE_MAX; Index++)
    {
  		if(LevelsTable[Index][4] >= TargetBitRate)
	   	  break;
    }


    if(Index < LEVELS_TABLE_MAX)
    {
		    MaxMBPS = LevelsTable[Index][2];
		    MinCR = LevelsTable[Index][3];
    }
    else
    {
        /* Cannot find appropriate Level. Consider maximum allowed */
		    MaxMBPS = LevelsTable[LEVELS_TABLE_MAX-1][2];
		    MinCR = LevelsTable[LEVELS_TABLE_MAX-1][3];
    }

    /* A.3.1 d) formula (H264 specifications) */
    MaxPictureSize = ((384 * MaxMBPS) / MinCR) / TargetFrameRate;
    MaxPictureSize *= 1000;

/*    printf ("Width %d  Height %d  FR %d  BR %d  MaxMBPS %d  MinCR %d  MaxPictureSize %d --> Index %d\n",        */
/*            PictureWidth, PictureHeight, TargetFrameRate, TargetBitRate, MaxMBPS, MinCR, MaxPictureSize, Index);*/
    return (MaxPictureSize);
}


/* End of mpg2toh264target.c */
