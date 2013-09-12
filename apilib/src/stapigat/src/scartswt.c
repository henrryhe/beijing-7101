/*******************************************************************************

File name   : scartswt.c

Description : Scart switch configuration source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
22 May 2002        Created                                          HSdLM
14 Oct 2002        Add support of mb382 (5517)                      HSdLM
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_SCART_SWITCH_CONFIG

#include "stdevice.h"

#include "stddefs.h"

#include "sti2c.h"
#include "cli2c.h"

#include "stcommon.h"
#include "sttbx.h"

#include "clpio.h"
#include "clevt.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#if defined(mb282b)
#define SCART_SWITCH_I2C_ADDRESS            0x94 /* STV6410 */

#elif defined(mb314)
#define SCART_SWITCH_I2C_ADDRESS            0x96 /* STV6410 */

#elif defined(mb361) || defined(mb382) || defined(mb390)|| defined(mb400) || defined(mb436) || defined(SAT5107)
#define SCART_SWITCH_I2C_ADDRESS            0x96 /* STV6413 */
#define ENC                                 0x5
#define GAIN_6DB                            0x1

#elif defined (espresso)
#define SCART_SWITCH_I2C_ADDRESS            0x96 /* STV6412 */

#elif defined(mb290)
#define SCART_SWITCH_I2C_ADDRESS            0x94 /* STV6410 */

#else
#error ERROR: no SCART switch known on this platform !
#endif

/* Private Variables (static)------------------------------------------------ */

#if defined(mb282b) || defined(mb314)
static U8 Buffer[] = {
      0x00, 0x05, /* 0: TV Audio Output: stereo from encoder inputs */
      0x01, 0x05, /* 1: Cinch Audio Output: from encoder inputs, normal gain */
      0x02, 0x00, /* 2: VCR/AUX Audio Output: muted */
      0x03, 0x68, /* 3: TV Video output: red not chroma, cvbs to rf, no filter
                                   Y/CVBS_ENC & R/C_ENC, chroma muted */
      0x04, 0x53, /* 4: avg level clamps, +6dB gain, Fast Blanking forced high,
                    RGB_Encoder not AUX */
      0x05, 0x00, /* 5: VCR/AUX Video Output: muted */
      0x06, 0x00, /* 6: Slow blanking switches: 'input mode' */
      0x07, 0x00, /* 7: Standby modes: all clamps and outputs on ... */
      0x08, 0x03  /* 8: ... except Cinch & RF MOD */
};

#elif defined(mb382)
static U8 Buffer[] = {
        0x00,0x00, /* REG 0: Stereo, no level adjust on audio*/
        0x01,0x01, /* REG 1: TV&CINCH audio set to Enc L/R mode */
        0x02,0x01, /* REG 2: TV video = Y/CVBS_ENC & R/C_ENC */
        0x03,0xA5, /* REG 3: FB High, RGB Enc, +4dB gain, 0dB extra gain, RGB+FB active */
        0x04,0x08, /* REG 4: R/C_TV = Red signal,    C_Gate low,  C_VCR Hi-Z, SLB=Normal, No interrupt */
        0x05,0x30, /* REG 5: SLB output 4/3 setup i2c for RGB mode */
        0x06,0x88  /* REG 6: All outputs enabled */
};
#elif defined (mb361)
static U8 Buffer[] = {
        0x00,0x00, /* REG 0: Stereo, no level adjust on audio*/
        0x01,0x01, /* REG 1: TV&CINCH audio set to Enc L/R mode */
        0x02,0x21, /* REG 2: TV video = Y/CVBS_ENC & R/C_ENC */
        0x03,0x84, /* REG 3: FB High, RGB Enc,RGB+FB active */
        0x04,0x18, /* REG 4: Low level,  C_VCR Hi-Z */
        0x05,0x30, /* REG 5: SLB output 4/3 setup i2c for RGB mode */
        0x06,0x88  /* REG 6: All outputs enabled */
};
        /* 0x00, ENC|GAIN_6DB,  */   /* TV_AUDIO_OUTPUT */
        /* 0x01, ENC|GAIN_6DB,  */   /* AUDIO_CINCH_OUTPUT */
        /* 0x02, 0x21,          */   /* VRC_AND_AUX_AUDIO_OUTPUT */
        /* 0x03, 0x85,          */   /* TV_VIDEO_OUTPUT_1 */
        /* 0x04, 0x18           */   /* TV_VIDEO_OUTPUT_2 */

#elif defined (espresso)
static U8 Buffer[] = {
        0x00,0x00, /* REG 0: Stereo, no level adjust on audio*/
        0x01,0x11, /* REG 1: TV&CINCH audio set to Enc L/R mode */
        0x02,0x11, /* REG 2: TV video = Y/CVBS_ENC & R/C_ENC */
        0x03,0x84, /* REG 3: FB High, RGB Enc,RGB+FB active */
        0x04,0x18, /* REG 4: Low level,  C_VCR Hi-Z */
        0x05,0x30, /* REG 5: SLB output 4/3 setup i2c for RGB mode */
        0x06,0x88  /* REG 6: All outputs enabled */
};

#elif defined(mb290)
static U8 Buffer[] = {
      0x00, 0x01, /* 0: TV Audio Output: stereo from aux inputs */
      0x01, 0x01, /* 1: Cinch Audio Output: from aux inputs, normal gain */
      0x02, 0x00, /* 2: VCR/AUX Audio Output: muted */
      0x03, 0x48, /* 3: TV Video output: red not chroma, cvbs to rf, no filter
                                   Y/CVBS_AUX & R/C_AUX, chroma muted */
      0x04, 0x93, /* 4: avg level clamps, +6dB gain, Fast Blanking forced high,
                    RGB_AUX not ENC */
      0x05, 0x00, /* 5: VCR/AUX Video Output: muted */
      0x06, 0x00, /* 6: Slow blanking switches: 'input mode' */
      0x07, 0x00, /* 7: Standby modes: all clamps and outputs on ... */
      0x08, 0x03  /* 8: ... except Cinch & RF MOD */
};

#elif defined(mb390)
static U8 Buffer[] = {
        0x00,0x00, /* 0: TV Audio Output: stereo from encoder inputs */
        0x01,0x11, /* REG 1: TV&CINCH audio set to Enc L/R mode */
        0x02,0x21, /* REG 2: TV video = Y/CVBS_ENC & R/C_ENC */
        0x03,0x86, /* REG 3: FB High, RGB Enc,RGB+FB active */
        0x04,0x18, /* REG 4: Low level,  C_VCR Hi-Z */
};
#elif defined (mb400) || defined (mb436) || defined (SAT5107)
static U8 Buffer[] = {
        0x00,0x00, /* REG 0: Stereo, no level adjust on audio*/
        0x01,0x11, /* REG 1: TV&CINCH audio set to Enc L/R mode */
        0x02,0x99, /* REG 2: TV video = Y/CVBS_ENC & R/C_ENC */
        0x03,0x84, /* REG 3: FB Low, RGB Enc, +4dB gain, 0dB extra gain, RGB+FB active */
        0x04,0x00, /* REG 4: R/C_TV = Red signal,    C_Gate low,  C_VCR Hi-Z, SLB=Normal, No interrupt */
        0x05,0x33, /* REG 5: SLB output 4/3 setup i2c for RGB mode */
        0x06,0x00  /* REG 6: All outputs enabled */
};
#endif



/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : ScartSwitch_Init
Description : Initialise the SCART switch to enable SD output on board
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
BOOL ScartSwitch_Init(void)
{
    STI2C_Handle_t     I2cHandle;
    STI2C_OpenParams_t I2cOpenParams;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    U32                ActLen;
    BOOL               RetOk = TRUE;
    U8                 Index;
    ST_ClockInfo_t     ClockInfo;

    ST_GetClockInfo(&ClockInfo);

    STTBX_Print(( "SCART Switch configuration: " ));

    /* Setup OpenParams structure here */
    I2cOpenParams.I2cAddress        = SCART_SWITCH_I2C_ADDRESS;   /* address of remote I2C device */
    I2cOpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BusAccessTimeOut  = ClockInfo.CommsBlock;

#if defined(mb390)
    ErrorCode = STI2C_Open (STI2C_FRONT_DEVICE_NAME, &I2cOpenParams, &I2cHandle);
#else
    ErrorCode = STI2C_Open (STI2C_BACK_DEVICE_NAME, &I2cOpenParams, &I2cHandle);
#endif
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("STI2C_Open(%s) failed, error=0x%0x", STI2C_BACK_DEVICE_NAME, ErrorCode));
        RetOk = FALSE;
    }
    else
    {
        for (Index = 0 ; Index < sizeof(Buffer)/sizeof(U8) ; Index+=2)
        {
            ErrorCode = STI2C_Write ( I2cHandle, &Buffer[Index], 2, ClockInfo.CommsBlock, &ActLen);
            if ( ErrorCode != ST_NO_ERROR)
            {
                STTBX_Print(("STI2C_Write() failed, error=0x%0x ", ErrorCode));
                RetOk = FALSE;
                break;
            }
        }
        ErrorCode = STI2C_Close (I2cHandle);
        if ( ErrorCode != ST_NO_ERROR)
        {
            STTBX_Print(("STI2C_Close() failed, error=0x%0x", ErrorCode));
            RetOk = FALSE;
        }
    }
    if (RetOk)
    {
        STTBX_Print(( "initialized"));
    }
    STTBX_Print(("\n"));
    return(RetOk);
} /* end ScartSwitch_Init() */

#endif /* USE_SCART_SWITCH_CONFIG */
/* End of scartswt.c */

