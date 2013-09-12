/*****************************************************************************

COPYRIGHT (C) STMicroelectronics 2006

Source file name : stclock.c

Revision History :

    24/05/00  Modified conditional compilation to now have a default case
              -- this was not present before and could lead to undefined
              values for the clock speed info structure.
              The defaults are applicable to the STi5510/12.

******************************************************************************/

/* Global include files */
#if !defined(ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#ifdef ST_OSLINUX
#include <linux/version.h>
#include <asm/param.h>

#else

#include "stsys.h"
#include "math.h"
#endif

#include "stdevice.h"
#include "stddefs.h"
#include "stlite.h"
#include "stcommon.h"
#include "stos.h"

#ifdef OS40
#include <chorus.h>
#include <exec/p_boardInfo.H>
#include <exec/chContext.h>
#endif

#if defined (ST_5508) || defined (ST_5518)
    #define CKG_CNT_ST20            (CKG_BASE_ADDRESS + 0xDD)
    #define CKG_DIV_ST20            (CKG_BASE_ADDRESS + 0xDE)
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    #define CKG_PLLFSDIV0           0x00
    #define CKG_PLLFSDIV1           0x04
    #define CKG_PLLFSDIV0_low       0x00
    #define CKG_PLLFSDIV0_high      0x01
    #define CKG_CLKDIV0_CONFIG0     0x10
    #define CKG_CLKDIV0_CONFIG1     0x14
    #define CKG_CLKDIV0_CONFIG2     0x18
    #define SYNTH0_CONFIG0          0x120
    #define SYNTH0_CONFIG1          0x124
    #define SYNTH1_CONFIG0          0x130
    #define SYNTH1_CONFIG1          0x134
    #define SYNTH2_CONFIG0          0x140
    #define SYNTH2_CONFIG1          0x144
#if defined(mb314_21)
    #define CKG_DEPTH_MASK          0x1F
    #define CKG_EVEN_MASK           0x40
    #define CKG_HALF_NOT_ODD_MASK   0x80
    #define CKG_DEPTH_SHIFT         0x00
    #define CKG_EVEN_SHIFT          0x06
    #define CKG_HALF_NOT_ODD_SHIFT  0x07
#else
    #define CKG_DEPTH_MASK          0x0F
    #define CKG_EVEN_MASK           0x20
    #define CKG_HALF_NOT_ODD_MASK   0x40
    #define CKG_DEPTH_SHIFT         0x00
    #define CKG_EVEN_SHIFT          0x05
    #define CKG_HALF_NOT_ODD_SHIFT  0x06
#endif
    #define CKG_DIVIDER_MODE        0xF8
    #define CKG_CLOCK_SEL1          0x200
#if defined (ST_5514)
    #define CKG_CLOCK_SEL2          0x204
#endif
    #define CLOCK_IN_27             27000000
    #define MPX_CLOCK_IN            27000000

#if defined(mb314_21)
static const U16 DividerTable[][6] = {
    {4,0x0,0x0002,0x01,1,1},
    {6,0x0,0x0006,0x02,0,0},
    {8,0x0,0x000c,0x03,1,1},
    {9,0x0,0x019c,0x08,0,1},
    {10,0x0,0x001c,0x04,0,0},
    {11,0x0,0x071c,0x0A,0,1},
    {12,0x0,0x0038,0x05,1,1},
    {13,0x0,0x1c78,0x0C,0,1},
    {14,0x0,0x0078,0x06,0,0},
    {15,0x0,0x7878,0x0E,0,1},
    {16,0x0,0x00f0,0x07,1,1},
    {17,0x1,0xe1f0,0x10,0,1},
    {18,0x0,0x01f0,0x08,0,0},
    {19,0x7,0xc1f0,0x12,0,1},
    {20,0x0,0x03e0,0x09,1,1},
    {22,0x0,0x07e0,0x0A,0,0},
    {24,0x0,0x0fc0,0x0B,1,1},
    {26,0x0,0x1fc0,0x0C,0,0},
    {28,0x0,0x3f80,0x0D,1,1},
    {30,0x0,0x7f80,0x0E,0,0},
    {32,0x0,0xff00,0x0F,1,1},
    {34,0x1,0xff00,0x10,0,0},
    {36,0x3,0xfe00,0x11,1,1},
    {38,0x7,0xfe00,0x12,0,0},
    {40,0xf,0xfc00,0x13,1,1},
    {0,0,0,0,0,0}
};
#endif

static U32 CPSL=0, CPSH=0;  /* Clock Per Second Low and Clock Per Second High */
static void MeasureTimers(U32, BOOL);
static U32 CalculateDivider(U32);
static void GetClockIn (U32 PLL_Section, U32 *ClockIn_p, BOOL *Bypass_p);
static U32 GetPLLFreq (U32 ClkIn);
static U32 GetSynthFreq(U32 Config0, U32 fin);


#define PWM_CONTROL         0x50
#define PWM_COUNT           0x60
#define PWM_CAPTURECOUNT    0x64
#define TimerHigh(Value) __optasm{ldc 0; ldclock; st Value;}

enum
{
    PLL_SEL0 = 0x01,
    PLL_SEL1 = 0x02,
    PLL_SEL2 = 0x04
};
#endif


/* structure prototypes */
typedef struct
{
    U32  u32CPUClockSpeed;
    U32  u32ClocksPerSecondHigh;
    U32  u32ClocksPerSecondLow;
    U32  u32LinkSpeed;
} ClockSpeedInfo;

/*********************************************************************
Function:    bGetClockSpeedInfo

Input:       None.

Outputs:     pstClockInfo - pointer to a structure of clock
                            speed info filled by this function.

Returns:     TRUE - success FALSE - failure

*********************************************************************/
BOOL bGetClockSpeedInfo( ClockSpeedInfo *pstClockSpeedInfo )
{
    BOOL result = TRUE;
#ifdef OS40
    BoardInfo *boardInfo = (BoardInfo*)chorusContext->ctxBoardInfo;
#endif
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    U32 PLLFreq;
    U32 Div;
    U32 ClkIn;
    BOOL Bypass;
#endif

    /* validate pointer passed into function */
    if ( pstClockSpeedInfo == (ClockSpeedInfo *) NULL )
    {
       	result = FALSE;
    }
    else
    {
#ifdef OS40
        pstClockSpeedInfo->u32CPUClockSpeed = boardInfo->systemClockFrequency;
        pstClockSpeedInfo->u32ClocksPerSecondHigh = 1000000;
        pstClockSpeedInfo->u32ClocksPerSecondLow = 1000000;
        pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */

#elif defined(ST_5580)
        /* The default clock is always used */
        pstClockSpeedInfo->u32CPUClockSpeed = 60750000;
        pstClockSpeedInfo->u32ClocksPerSecondHigh = 995902;
        pstClockSpeedInfo->u32ClocksPerSecondLow = 15561;
        pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */

#elif defined(ST_5508) || defined(ST_5518)
        U8 ReadValue;
        U8 Div;
        U8 Bypass;

        /* Read CKG DIV register to determine dividor */
        ReadValue = STSYS_ReadRegDev8(CKG_DIV_ST20);
        Div = (ReadValue >> 4);

        /* Read CKG CNT register to determine bypass setting */
        ReadValue = STSYS_ReadRegDev8(CKG_CNT_ST20);
        Bypass = !(ReadValue >> 5);

        /* Calculating the timer ticks
         *
         * For 60.75MHz we use:
         * High priority ticks = 60750000 / 61 = 995901
         * Low priority ticks  = 995901   / 64 = 15561
         *
         * For 81MHz we use:
         * High priority ticks = 81000000 / 61 = 1327869
         * Low priority ticks  = 1327869  / 64 = 20748
         *
         * Note that the PLL is 486MHz.  The following table of
         * values reflects clock speed:-
         *
         * Fcpu  27  48.6  60.75  81
         * -------------------------
         * D     -   5     4      3
         * B     1   0     0      0
         * -------------------------
         */
        if (Bypass)                     /* 27MHz? */
        {
            /* In bypass mode therefore we are operating at 27Mhz */
            pstClockSpeedInfo->u32CPUClockSpeed = 27000000;
            pstClockSpeedInfo->u32ClocksPerSecondHigh = 442623;
            pstClockSpeedInfo->u32ClocksPerSecondLow = 6916;
            pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */
        }
        else                            /* Check Div */
        {
            /* Need to check the dividor to compute CPU frequency */
            switch(Div)
            {
                case 2:                 /* 121.5Mhz clock */
                    pstClockSpeedInfo->u32CPUClockSpeed = 121500000;
                    pstClockSpeedInfo->u32ClocksPerSecondHigh = 1991804;
                    pstClockSpeedInfo->u32ClocksPerSecondLow = 31122;
                    pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */
                    break;
                case 3:                 /* 81Mhz clock */
                    pstClockSpeedInfo->u32CPUClockSpeed = 81000000;
                    pstClockSpeedInfo->u32ClocksPerSecondHigh = 1327869;
                    pstClockSpeedInfo->u32ClocksPerSecondLow = 20748;
                    pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */
                    break;
                case 4:                 /* 60.75 clock */
                    pstClockSpeedInfo->u32CPUClockSpeed = 60750000;
                    pstClockSpeedInfo->u32ClocksPerSecondHigh = 995902;
                    pstClockSpeedInfo->u32ClocksPerSecondLow = 15561;
                    pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */
                    break;
                case 5:                 /* 48.60 clock */
                    pstClockSpeedInfo->u32CPUClockSpeed = 48600000;
                    pstClockSpeedInfo->u32ClocksPerSecondHigh = 795721;
                    pstClockSpeedInfo->u32ClocksPerSecondLow = 12433;
                    pstClockSpeedInfo->u32LinkSpeed = 19200000;  /* no meaning */
                    break;
                default:                /* Unsupported setting */
                    result = FALSE;
                    break;
            }
        }

#elif defined(ST_5505)
       	pstClockSpeedInfo->u32CPUClockSpeed = 60000000;
        pstClockSpeedInfo->u32ClocksPerSecondHigh = 1000000;
        pstClockSpeedInfo->u32ClocksPerSecondLow = 15625;
        pstClockSpeedInfo->u32LinkSpeed = 19200000;

#elif defined(ST_5528) || defined(ST_5100) || defined(ST_5101)
        /* Quick initial implementation based on clock speeds quoted by board
          header file to match what is set by board cfg file. For an example of
          actual clockgen reg decoding, see DisplayClocks in 5528ckg.cfg */
               	
       	#if defined(ST_OS21)
       	pstClockSpeedInfo->u32CPUClockSpeed = STCOMMON_ST40CORE_CLOCK;/*200000000 from mb376.h*/
       	pstClockSpeedInfo->u32ClocksPerSecondLow = time_ticks_per_sec();
       	pstClockSpeedInfo->u32ClocksPerSecondHigh = pstClockSpeedInfo->u32ClocksPerSecondLow;
       	pstClockSpeedInfo->u32LinkSpeed = STCOMMON_IC_CLOCK;/*133000000 from mb376.h*/
        #else
        pstClockSpeedInfo->u32CPUClockSpeed = STCOMMON_ST20_CLOCK;
        pstClockSpeedInfo->u32ClocksPerSecondHigh = STCOMMON_ST20TICK_CLOCK;
        pstClockSpeedInfo->u32ClocksPerSecondLow = STCOMMON_ST20TICK_CLOCK/64;
        pstClockSpeedInfo->u32LinkSpeed = STCOMMON_IC_CLOCK;
        #endif
            /* PTI runs at the STBUS clock speed */

#elif defined(PROCESSOR_C1) /*7710 5105 5700 5188 5107*/
        pstClockSpeedInfo->u32CPUClockSpeed       = STCOMMON_ST20_CLOCK;
        pstClockSpeedInfo->u32ClocksPerSecondHigh = STCOMMON_ST20TICK_CLOCK;
        pstClockSpeedInfo->u32ClocksPerSecondLow  = STCOMMON_ST20TICK_CLOCK;
        pstClockSpeedInfo->u32LinkSpeed           = STCOMMON_IC_CLOCK;
        
#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)/*ST40*/
        pstClockSpeedInfo->u32CPUClockSpeed = STCOMMON_ST40CORE_CLOCK;
        #ifdef ST_OS21
       	pstClockSpeedInfo->u32ClocksPerSecondLow = time_ticks_per_sec();
       	#endif
       	#ifdef ST_OSLINUX
       	    pstClockSpeedInfo->u32ClocksPerSecondLow = STLINUX_GetClocksPerSecond();
       	#endif
       	pstClockSpeedInfo->u32ClocksPerSecondHigh = pstClockSpeedInfo->u32ClocksPerSecondLow;
       	pstClockSpeedInfo->u32LinkSpeed = STCOMMON_IC_CLOCK;
#elif defined(ST_5301) || defined(ST_8010) || defined(ST_5525)/*ST200*/
        pstClockSpeedInfo->u32CPUClockSpeed = STCOMMON_ST200CORE_CLOCK;
       	pstClockSpeedInfo->u32ClocksPerSecondLow = time_ticks_per_sec();
       	pstClockSpeedInfo->u32ClocksPerSecondHigh = pstClockSpeedInfo->u32ClocksPerSecondLow;
       	pstClockSpeedInfo->u32LinkSpeed = STCOMMON_IC_CLOCK;

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)

        /* Calculate pll_clock[0-7] */
        GetClockIn(PLL_SEL0, &ClkIn, &Bypass);
        if (Bypass == FALSE)
        {
            PLLFreq = GetPLLFreq(ClkIn);
            if (CPSH == 0)
            {
                MeasureTimers(PLLFreq,1);
            }
            Div = CalculateDivider(0);
            if (Div != 0)
            {
                pstClockSpeedInfo->u32CPUClockSpeed = (PLLFreq*2)/Div;
                pstClockSpeedInfo->u32ClocksPerSecondHigh = CPSH;
                pstClockSpeedInfo->u32ClocksPerSecondLow = CPSL;
                pstClockSpeedInfo->u32LinkSpeed = 19200000;     /* no meaning? */
            }
            else
            {
                result = FALSE;
            }
        }
        else
        {
            if (CPSH == 0)
            {
                MeasureTimers(ClkIn,0);
            }

            pstClockSpeedInfo->u32CPUClockSpeed = ClkIn;
            pstClockSpeedInfo->u32ClocksPerSecondHigh = CPSH;
            pstClockSpeedInfo->u32ClocksPerSecondLow = CPSL;
            pstClockSpeedInfo->u32LinkSpeed = 19200000;     /* no meaning? */
        }

#else   /* Default case for STi5510, STi5512 */

#define SPEED_SELECT_OFFSET    0x100
    	U8 SpeedSelect;

    	/* Read SpeedSelect 1-0 to determine CPU clock speed */
    	SpeedSelect = STSYS_ReadRegDev8(LPC_BASE_ADDRESS + SPEED_SELECT_OFFSET);

    	/* Mask for ls 2 bits and determine clock info */
    	switch( SpeedSelect & 0x03 )
    	{
            case 1:
                pstClockSpeedInfo->u32CPUClockSpeed = 60000000;
                pstClockSpeedInfo->u32ClocksPerSecondHigh = 1000000;
                pstClockSpeedInfo->u32ClocksPerSecondLow = 15625;
                pstClockSpeedInfo->u32LinkSpeed = 19200000;
                break;
            case 2:
                pstClockSpeedInfo->u32CPUClockSpeed = 39900000;
                pstClockSpeedInfo->u32ClocksPerSecondHigh = 997500;
                pstClockSpeedInfo->u32ClocksPerSecondLow = 15586;
                pstClockSpeedInfo->u32LinkSpeed = 19950000;
                break;
            case 3:
                pstClockSpeedInfo->u32CPUClockSpeed = 49875000;
                pstClockSpeedInfo->u32ClocksPerSecondHigh = 997500;
                pstClockSpeedInfo->u32ClocksPerSecondLow = 15586;
                pstClockSpeedInfo->u32LinkSpeed = 19950000;
                break;
            case 0:   /* times one mode */
	    default:  /* illegal value - can't happen with correct bit mask */
                result = FALSE;
	        break;
	    }
#endif
	}
	return( result );
}   /* end of bGetClockSpeedInfo() */

/********************************************************************
Function:    ST_GetClockSpeed

Input:       None.

Outputs:     None.

Returns:     U32 - CPU Clock Speed in Hz.

********************************************************************/
U32 ST_GetClockSpeed( void )
{
    ClockSpeedInfo stClockSpeedInfo;

    if ( bGetClockSpeedInfo( &stClockSpeedInfo ) != TRUE )
    {
        /* ERROR - cannot obtain clock speed info */
        return( 0 );
    }

    return( stClockSpeedInfo.u32CPUClockSpeed );

}   /* end of ST_GetClockSpeed() */

/********************************************************************
Function:    ST_SetClockSpeed

Input:       Clock speed in Hz.

Outputs:     None.

Returns:     ErrorCode

********************************************************************/
ST_ErrorCode_t ST_SetClockSpeed(U32 F)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined(ST_5508) || defined(ST_5518)
    U8 Div = 0;
    U8 Bypass = 0;

    switch (F)                          /* Check for valid frequency */
    {
        case 27000000:
            Bypass = 1;
            break;
        case 81000000:
            Div = 3;
            break;
        case 60750000:
            Div = 4;
            break;
        case 48600000:
            Div = 5;
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    if (Error == ST_NO_ERROR)
    {
        if (Bypass)
        {
            /* Set CKG CNT register to determine bypass setting */
            STSYS_WriteRegDev8(CKG_CNT_ST20, 0x00);
        }
        else
        {
            /* Set CKG DIV register to determine dividor */
            STSYS_WriteRegDev8(CKG_DIV_ST20, (Div << 4));

            /* Set CKG CNT register to determine bypass setting */
            STSYS_WriteRegDev8(CKG_CNT_ST20, 0xA0);
        }
    }

#else
    Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif

    return Error;
}

/********************************************************************

Function:    ST_GetClocksPerSecond

Input:       None.

Outputs:     None.

Returns:     U32 - clocks per second.

********************************************************************/
U32 ST_GetClocksPerSecond( void )
{
    #ifdef ST_OS20
    #if !defined(PROCESSOR_C1) /*7710 5105 5700 5188 5107*/
    S32 Priority;
    #endif /*#if !defined(PROCESSOR_C1)*/
    
    ClockSpeedInfo stClockSpeedInfo;

    if ( bGetClockSpeedInfo( &stClockSpeedInfo ) != TRUE )
    {
        /* ERROR - cannot obtain clock speed info */
        return( 0 );
    }

    #if !defined(PROCESSOR_C1) /*7710 5105 5700 5188 5107*/
    /* Get current priority...0 == Hi, 1 == Low */
    __optasm { ldpri; st Priority; }
    if (Priority)
    {
        return( stClockSpeedInfo.u32ClocksPerSecondLow );
    }
    else
    {
        return( stClockSpeedInfo.u32ClocksPerSecondHigh );
    }
    #endif /*#if !defined(PROCESSOR_C1)*/
    
    #if defined(PROCESSOR_C1) /*7710 5105 5700 5188 5107*/
    return( stClockSpeedInfo.u32ClocksPerSecondLow );
    #endif
    #endif /*#ifdef ST_OS20*/
    
    #if defined(ST_OS21) || defined(ST_OSWINCE)
    return( time_ticks_per_sec() );
    #endif
    
    #if defined(ST_OSLINUX)
        return(STLINUX_GetClocksPerSecond() ); /*in STOS. if MODULE its HZ (jiffies). In user mode, it is up to us to define a base here. Its set to ms ->1000 */
    #endif
    
}   /* end of ST_GetClocksPerSecond() */


/********************************************************************

Function:    ST_GetClocksPerSecondHigh

Input:       None.

Outputs:     None.

Returns:     U32 - clocks per second (High).

*********************************************************************/
U32 ST_GetClocksPerSecondHigh( void )
{
    ClockSpeedInfo stClockSpeedInfo;

    if ( bGetClockSpeedInfo( &stClockSpeedInfo ) != TRUE )
    {
        /* ERROR - cannot obtain clock speed info */
        return( 0 );
    }

    return( stClockSpeedInfo.u32ClocksPerSecondHigh );
}   /* end of ST_GetClocksPerSecondHigh() */


/********************************************************************

Function:    ST_GetClocksPerSecondLow

Input:       None.

Outputs:     None.

Returns:     U32 - clocks per second (Low).

********************************************************************/
U32 ST_GetClocksPerSecondLow( void )
{
    ClockSpeedInfo stClockSpeedInfo;

    if ( bGetClockSpeedInfo( &stClockSpeedInfo ) != TRUE )
    {
        /* ERROR - cannot obtain clock speed info */
        return( 0 );
    }

    return( stClockSpeedInfo.u32ClocksPerSecondLow );
}   /* end of ST_GetClocksPerSecondLow() */

/* End of stclock.c */



#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
/********************************************************************

Function:    MeasureTimers

Input:       PLLFreq_ClockSpeed
             PLL_Not_Clock

Outputs:     None.

Returns:     None.

********************************************************************/

void MeasureTimers(U32 PLLFreq_ClockSpeed, BOOL PLL_Not_Clock)
{
        U32 Timer1h, Timer2h, Timer1l, Timer2l, PWMtimer1, PWMtimer2;
        U32 ClockSpeed, Div, PWMControl;
        float Time;

        if (PLL_Not_Clock)
        {
            Div = CalculateDivider(2);
            ClockSpeed = (PLLFreq_ClockSpeed*2)/Div;
        }
        else
        {
            ClockSpeed = PLLFreq_ClockSpeed;
        }

        /*STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CONTROL),      0x00000000);*/
        /* unnecessary; datasheet says PWM_CAPTURECOUNT can be read or written at any time */
        /*STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CAPTURECOUNT), 0x00000000);*/
        /* unnecessary; just take 32-bit difference before and after */
        
        /* we must read/write PWM_CONTROL atomically because it is shared with
          STPWM. The counter module is also driven by STCOUNT, but this is not
          widely used. We only turn the module on, not off, but we do set the
          prescale value to 0 to avoid having to increase the delay to achieve
          the same accuracy - remember that the register may be uninitialised */

        interrupt_lock();
        PWMControl = STSYS_ReadRegDev32LE (PWM_BASE_ADDRESS + PWM_CONTROL);
        PWMControl &= ~0x000001F0; /* set CAPTURE divider = 0 */
        PWMControl |=  0x00000400;
        STSYS_WriteRegDev32LE ((PWM_BASE_ADDRESS + PWM_CONTROL), PWMControl);
        interrupt_unlock();
        
        PWMtimer1 = STSYS_ReadRegDev32LE (PWM_BASE_ADDRESS + PWM_CAPTURECOUNT);
        Timer1l = STOS_time_now();
        TimerHigh(Timer1h);

        STOS_TaskDelay (10000);
        
        TimerHigh(Timer2h);
        Timer2l = STOS_time_now();
        PWMtimer2 = STSYS_ReadRegDev32LE (PWM_BASE_ADDRESS + PWM_CAPTURECOUNT);
        
        Time = ((float)(U32)(PWMtimer2-PWMtimer1))/((float)ClockSpeed);

        CPSH = (U32) ((float)STOS_time_minus(Timer2h, Timer1h)/Time);
        CPSL = (U32) ((float)STOS_time_minus(Timer2l, Timer1l)/Time);
}

/********************************************************************

Function:    CalculateDivider

Input:       ClockNumber

Outputs:     None.

Returns:     Twice the value of the divider.

********************************************************************/
#if defined(mb314_21)
U32 CalculateDivider(U32 ClockNumber)
{
    U16 CKG_Config0, CKG_Config1, CKG_Config2;
    U32 Div, i;

    CKG_Config0 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x10);
    CKG_Config1 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x14);
    CKG_Config2 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x18);
    Div = 0;
    for (i = 0; DividerTable[i][0] != 0; i++)
    {
        if (/*(CKG_Config0 == DividerTable[i][2])          &&*/ /* This value is not unique */
            ((CKG_Config1 & 0x0F) == DividerTable[i][1]) &&
            ((CKG_Config2 & CKG_DEPTH_MASK) == (DividerTable[i][3] << CKG_DEPTH_SHIFT)) &&
            ((CKG_Config2 & CKG_EVEN_MASK) == (DividerTable[i][4] << CKG_EVEN_SHIFT)) &&
            ((CKG_Config2 & CKG_HALF_NOT_ODD_MASK) == (DividerTable[i][5] << CKG_HALF_NOT_ODD_SHIFT)))
        {
            Div = DividerTable[i][0];
            break;
        }
    }

    return (Div);
}
#else
U32 CalculateDivider(U32 ClockNumber)
{
    U16 CKG_Config0, CKG_Config1, CKG_Config2;
    U32 Div, i, Pattern, Depth, Changes;
    U32 First, Current, Last;

    Changes = 0;
    Div = 0;

    /* Loop is a fix for GNBvd26377 */
    i = 0;
    do
    {

        CKG_Config0 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x10);
        CKG_Config1 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x14);
        CKG_Config2 =  STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + ClockNumber*0x10 + 0x18);
        Pattern = CKG_Config0 | ((CKG_Config1 & 0x0F) << 16);
        i++;
    }
    while((Pattern == 0) && (i < 100));

    Depth = 11 + ((CKG_Config2 & CKG_DEPTH_MASK) >> CKG_DEPTH_SHIFT);

    First = Pattern & 0x01;
    Last = First;

    for (i = 1; i<Depth; i++)
    {
        Current = (Pattern >> i)  & 0x01;
        if (Current != Last)
            Changes++;
        Last = Current;
    }

    if (First != Last)
        Changes++;

    /* Calculate the divider. The value actually returned is divider*2 to avoid
       using float numbers */
    Div = (Depth * 4)/Changes;

    return (Div);
}
#endif
#endif
/********************************************************************

Function:    ST_GetClockInfo()

Input:       ClockInfo_p

Outputs:     None.

Returns:     ST_NO_ERROR.
             ST_ERROR_BAD_PARAMETER.

********************************************************************/
ST_ErrorCode_t ST_GetClockInfo (ST_ClockInfo_t *ClockInfo_p)
{
#if defined (ST_5514) || defined(ST_5516) || defined(ST_5517)
    U32 *Info_p;
    U32 PLLFreq;
    U32 Div, i;
    U32 ClkIn;
    BOOL Bypass;
#elif !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_7100) && !defined(ST_5301) && !defined(ST_8010) && !defined(ST_7109) && !defined(ST_5188) && !defined(ST_5525) && !defined(ST_5107) && !defined(ST_7200)
    ClockSpeedInfo stClockSpeedInfo;
#endif

    if (ClockInfo_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

#if defined (ST_5514) || defined(ST_5516) || defined(ST_5517)
    Info_p = (U32 *)ClockInfo_p;

    /* Calculate pll_clock[0-7] and timers */
    GetClockIn(PLL_SEL0, &ClkIn, &Bypass);
    if (Bypass == FALSE)
    {
        PLLFreq = GetPLLFreq(ClkIn);
        MeasureTimers(PLLFreq,1);
        for (i=0; i < 8; i++)
        {
            Div = CalculateDivider(i);
            *Info_p = (PLLFreq*2)/Div;
            Info_p++;
        }
    }
    else
    {
        MeasureTimers(ClkIn,0);
        for (i=0; i < 8; i++)
        {
            *Info_p = ClkIn;
            Info_p++;
        }
    }

    /* Store timers information */
    ClockInfo_p->HPTimer = CPSH;
    ClockInfo_p->LPTimer = CPSL;

#if defined (ST_5514)
    /* Calculate pll_clock[8] */
    GetClockIn(PLL_SEL1, &ClkIn, &Bypass);
    if (Bypass == FALSE)
    {
        PLLFreq = GetPLLFreq(ClkIn);
        for (i=8; i < 9; i++)
        {
            Div = CalculateDivider(i);
            *Info_p = (PLLFreq*2)/Div;
            Info_p++;
        }
    }
    else
    {
        for (i=0; i < 1; i++)
        {
            *Info_p = ClkIn;
            Info_p++;
        }
    }
#else
    /* No separate EMI clock on 5516, return STBus speed */
    *Info_p++ = ((U32 *)ClockInfo_p)[1];
#endif

#if defined (ST_5516) || defined(ST_5517)
    /* No MPX on 5516/17 */
    *Info_p++ = 0;
    i=12;
#else
    i=11;
#endif

    /* Calculate sdll_clock[0-2] */
    GetClockIn(PLL_SEL2, &ClkIn, &Bypass);
    if (Bypass == FALSE)
    {
        PLLFreq = GetPLLFreq(ClkIn);
        for (; i < 14; i++)
        {
            Div = CalculateDivider(i);
            *Info_p = (PLLFreq*2)/Div;
            Info_p++;
        }
    }
    else
    {
        for (; i < 14; i++)
        {
            *Info_p = ClkIn;
            Info_p++;
        }
    }

    /* Calculate clocks based in frequency synthesizer */
    /* PCM clock */
    ClockInfo_p->PCM = GetSynthFreq(SYNTH0_CONFIG0, CLOCK_IN_27);

    /* Smart Card clock */
    ClockInfo_p->SmartCard = GetSynthFreq(SYNTH1_CONFIG0, CLOCK_IN_27);

    /* Auxiliary clock */
    ClockInfo_p->Aux = GetSynthFreq(SYNTH2_CONFIG0, CLOCK_IN_27);
    
    /* Fill the 5528 fields with sensible values */
    ClockInfo_p->ST40 = 0; /* no ST40 on these devices */
    ClockInfo_p->ST40Per = 0; /* no ST40 on these devices */
    ClockInfo_p->UsbIrda = ClockInfo_p->CommsBlock;
    ClockInfo_p->PWM = ClockInfo_p->CommsBlock;
    ClockInfo_p->PCI = 0; /* no PCI on these devices */
    ClockInfo_p->VideoPipe = ClockInfo_p->Video2; /* Video2 played the role of pipe clock */
    
    ClockInfo_p->ST200 = 0; /* no ST200 on these devices */

#elif defined(ST_5528) || defined(ST_5100) || defined(ST_5101)
    
    /* Quick initial implementation based on clock speeds quoted by board
      header file to match what is set by board cfg file. For an example of
      actual clockgen reg decoding, see DisplayClocks in 5528ckg.cfg */

    ClockInfo_p->C2 = STCOMMON_ST20_CLOCK;
    
    ClockInfo_p->STBus = STCOMMON_IC_CLOCK;
    
    /* Most of the COMMS block now runs at STBUS speed.
      But see PWMCLK, LPCLK, TTXTCLK, IRDACLK */
    ClockInfo_p->CommsBlock = STCOMMON_IC_CLOCK;
    
    ClockInfo_p->HDDI = STCOMMON_HDD_CLOCK;
    
    /* LMI DDR 2*frequency, not video pipe clock.
      Note actual freq is half this, but data is clocked on both edges */
    ClockInfo_p->VideoMem = STCOMMON_LMI_CLOCK;
    
    /* Old Video/2, Video/3 architecture does not apply on 5528;
      the pipeline clocks and LMI clocks are separate */
    ClockInfo_p->Video2 = 0;
    
    ClockInfo_p->Video3 = 0;
    
    /* This is the main MMDSP clock; the SPDIF Formatter runs slower
      (but why do you need to know how fast it runs?) */
    ClockInfo_p->AudioDSP = STCOMMON_ADEC_CLOCK;
    
    ClockInfo_p->EMI = STCOMMON_EMI_CLOCK;
    
    ClockInfo_p->MPX = 0; /* MPX not present on 5528 */
    
    /* report synchronous flash clock after EMI divider */
    ClockInfo_p->Flash = STCOMMON_FLASH_CLOCK;
    
    /* report SDRAM clock after EMI divider */
    ClockInfo_p->SDRAM = STCOMMON_SDRAM_CLOCK;
    
    /* 27 MHz is sent to the ADSC block, where it is used to produce the
      desired audio output reference clocks */
    ClockInfo_p->PCM = 27000000;
    
    ClockInfo_p->SmartCard = STCOMMON_SMARTCARD_CLOCK;
    
    /* General purpose clock output (CLKOUT) */
    ClockInfo_p->Aux = STCOMMON_CLKOUT_CLOCK;
    
    #if defined(ST_OS21) || defined(ST_OSWINCE)
    ClockInfo_p->HPTimer = time_ticks_per_sec();
    ClockInfo_p->LPTimer = time_ticks_per_sec();
    #endif
    
    #ifdef ST_OS20
    ClockInfo_p->HPTimer = STCOMMON_ST20TICK_CLOCK;
    ClockInfo_p->LPTimer = STCOMMON_ST20TICK_CLOCK / 64;
    #endif
    
    #ifdef ST_OSLINUX
    ClockInfo_p->HPTimer = STLINUX_GetClocksPerSecond();
    ClockInfo_p->LPTimer = STLINUX_GetClocksPerSecond();
    #endif
    
    ClockInfo_p->ST40 = STCOMMON_ST40CORE_CLOCK;
    
    ClockInfo_p->ST40Per = STCOMMON_ST40PERIPHERAL_CLOCK;
    
    ClockInfo_p->UsbIrda = STCOMMON_USB_IRDA_CLOCK;
    
    ClockInfo_p->PWM = STCOMMON_PWM_CLOCK;
    
    ClockInfo_p->PCI = STCOMMON_PCI_CLOCK;
    
    ClockInfo_p->VideoPipe = STCOMMON_VDEC_SPDIFDEC_CLOCK;
    
    ClockInfo_p->ST200 = 0; /*no ST200 on these devices*/
    
    
#elif defined(PROCESSOR_C1) /*7710 5105 5700 5188 5107*/

    ClockInfo_p->C2 = STCOMMON_ST20_CLOCK;
    
    ClockInfo_p->STBus = STCOMMON_IC_CLOCK;
    
    /* Most of the COMMS block now runs at STBUS speed.
      But see PWMCLK, LPCLK, TTXTCLK, IRDACLK */
    ClockInfo_p->CommsBlock = STCOMMON_IC_CLOCK;
    
    ClockInfo_p->HDDI = STCOMMON_HDD_CLOCK;
    
    /* LMI DDR 2*frequency, not video pipe clock.
      Note actual freq is half this, but data is clocked on both edges */
    ClockInfo_p->VideoMem = STCOMMON_LMI_CLOCK;
    
    /* Old Video/2, Video/3 architecture does not apply;
      the pipeline clocks and LMI clocks are separate */
    ClockInfo_p->Video2 = 0;
    
    ClockInfo_p->Video3 = 0;
    
    /* This is the main MMDSP clock; the SPDIF Formatter runs slower */
    ClockInfo_p->AudioDSP = STCOMMON_ADEC_CLOCK;
    
    ClockInfo_p->EMI = STCOMMON_EMI_CLOCK;
    
    ClockInfo_p->MPX = 0; /* MPX not present??? */
    
    /* report synchronous flash clock after EMI divider */
    ClockInfo_p->Flash = STCOMMON_FLASH_CLOCK;
    
    /* report SDRAM clock after EMI divider */
    ClockInfo_p->SDRAM = STCOMMON_SDRAM_CLOCK;
    
    /* 27 MHz is sent to the ADSC block, where it is used to produce the
      desired audio output reference clocks */
    ClockInfo_p->PCM = 27000000;
    
    ClockInfo_p->SmartCard = STCOMMON_SMARTCARD_CLOCK;
    
    /* General purpose clock output (CLKOUT) */
    ClockInfo_p->Aux = STCOMMON_CLKOUT_CLOCK;
    
    ClockInfo_p->HPTimer = STCOMMON_ST20TICK_CLOCK;
    
    ClockInfo_p->LPTimer = STCOMMON_ST20TICK_CLOCK;
        
    ClockInfo_p->ST40 = STCOMMON_ST40CORE_CLOCK;
    
    ClockInfo_p->ST40Per = STCOMMON_ST40PERIPHERAL_CLOCK;
        
    ClockInfo_p->UsbIrda = STCOMMON_USB_IRDA_CLOCK;
    
    ClockInfo_p->PWM = STCOMMON_PWM_CLOCK;
    
    ClockInfo_p->PCI = STCOMMON_PCI_CLOCK;
    
    ClockInfo_p->VideoPipe = STCOMMON_VDEC_SPDIFDEC_CLOCK;
    
    ClockInfo_p->ST200 = 0; /*no ST200 on these devices*/


#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)/*ST40*/
    
    ClockInfo_p->C2 = STCOMMON_ST20_CLOCK;
    
    ClockInfo_p->STBus = STCOMMON_IC_CLOCK;
    
    /* Most of the COMMS block now runs at STBUS speed.
      But see PWMCLK, LPCLK, TTXTCLK, IRDACLK */
    ClockInfo_p->CommsBlock = STCOMMON_IC_CLOCK;
    
    ClockInfo_p->HDDI = STCOMMON_HDD_CLOCK;
    
    /* LMI DDR 2*frequency, not video pipe clock.
      Note actual freq is half this, but data is clocked on both edges */
    ClockInfo_p->VideoMem = STCOMMON_LMI_CLOCK;
    
    /* Old Video/2, Video/3 architecture does not apply;
      the pipeline clocks and LMI clocks are separate */
    ClockInfo_p->Video2 = 0;
    
    ClockInfo_p->Video3 = 0;
    
    /* This is the main MMDSP clock; the SPDIF Formatter runs slower */
    ClockInfo_p->AudioDSP = STCOMMON_ADEC_CLOCK;
    
    ClockInfo_p->EMI = STCOMMON_EMI_CLOCK;
    
    ClockInfo_p->MPX = 0; /* MPX not present */
    
    /* report synchronous flash clock after EMI divider */
    ClockInfo_p->Flash = STCOMMON_FLASH_CLOCK;
    
    /* report SDRAM clock after EMI divider */
    ClockInfo_p->SDRAM = STCOMMON_SDRAM_CLOCK;
    
    /* 27 MHz is sent to the ADSC block, where it is used to produce the
      desired audio output reference clocks */
    ClockInfo_p->PCM = 27000000;
    
    ClockInfo_p->SmartCard = STCOMMON_SMARTCARD_CLOCK;
    
    /* General purpose clock output (CLKOUT) */
    ClockInfo_p->Aux = STCOMMON_CLKOUT_CLOCK;
    
    #if defined(ST_OS21) || defined(ST_OSWINCE)
    ClockInfo_p->HPTimer = time_ticks_per_sec();
    ClockInfo_p->LPTimer = time_ticks_per_sec();
    #endif
    
    #ifdef ST_OSLINUX
        ClockInfo_p->HPTimer = STLINUX_GetClocksPerSecond();
        ClockInfo_p->LPTimer = STLINUX_GetClocksPerSecond();
    #endif
    
    ClockInfo_p->ST40 = STCOMMON_ST40CORE_CLOCK;
    
    ClockInfo_p->ST40Per = STCOMMON_ST40PERIPHERAL_CLOCK;
    
    ClockInfo_p->UsbIrda = STCOMMON_USB_IRDA_CLOCK;
    
    ClockInfo_p->PWM = STCOMMON_PWM_CLOCK;
    
    ClockInfo_p->PCI = STCOMMON_PCI_CLOCK;
    
    ClockInfo_p->VideoPipe = STCOMMON_VDEC_SPDIFDEC_CLOCK;
    
    ClockInfo_p->ST200 = 0; /*no ST200 on this device*/
    
    
#elif defined(ST_5301) || defined(ST_8010) || defined(ST_5525)/*ST200*/
    
    ClockInfo_p->C2 = STCOMMON_ST20_CLOCK;
    
    ClockInfo_p->STBus = STCOMMON_IC_CLOCK;
    
    /* Most of the COMMS block now runs at STBUS speed.
      But see PWMCLK, LPCLK, TTXTCLK, IRDACLK */
    ClockInfo_p->CommsBlock = STCOMMON_IC_CLOCK;
    
    ClockInfo_p->HDDI = STCOMMON_HDD_CLOCK;
    
    /* LMI DDR 2*frequency, not video pipe clock.
      Note actual freq is half this, but data is clocked on both edges */
    ClockInfo_p->VideoMem = STCOMMON_LMI_CLOCK;
    
    /* Old Video/2, Video/3 architecture does not apply;
      the pipeline clocks and LMI clocks are separate */
    ClockInfo_p->Video2 = 0;
    
    ClockInfo_p->Video3 = 0;
    
    /* This is the main MMDSP clock; the SPDIF Formatter runs slower */
    ClockInfo_p->AudioDSP = STCOMMON_ADEC_CLOCK;
    
    ClockInfo_p->EMI = STCOMMON_EMI_CLOCK;
    
    ClockInfo_p->MPX = 0; /* MPX not present */
    
    /* report synchronous flash clock after EMI divider */
    ClockInfo_p->Flash = STCOMMON_FLASH_CLOCK;
    
    /* report SDRAM clock after EMI divider */
    ClockInfo_p->SDRAM = STCOMMON_SDRAM_CLOCK;
    
    /* 27 MHz is sent to the ADSC block, where it is used to produce the
      desired audio output reference clocks */
    ClockInfo_p->PCM = 27000000;
    
    ClockInfo_p->SmartCard = STCOMMON_SMARTCARD_CLOCK;
    
    /* General purpose clock output (CLKOUT) */
    ClockInfo_p->Aux = STCOMMON_CLKOUT_CLOCK;
    
    ClockInfo_p->HPTimer = time_ticks_per_sec();
    ClockInfo_p->LPTimer = time_ticks_per_sec();
    
    ClockInfo_p->ST40 = STCOMMON_ST40CORE_CLOCK;
    
    ClockInfo_p->ST40Per = STCOMMON_ST40PERIPHERAL_CLOCK;
    
    ClockInfo_p->UsbIrda = STCOMMON_USB_IRDA_CLOCK;
    
    ClockInfo_p->PWM = STCOMMON_PWM_CLOCK;
    
    ClockInfo_p->PCI = STCOMMON_PCI_CLOCK;
    
    ClockInfo_p->VideoPipe = STCOMMON_VDEC_SPDIFDEC_CLOCK;
    
    ClockInfo_p->ST200 = STCOMMON_ST200CORE_CLOCK;
    

#else /* not 14/16/17/28 - just return basic clocks */

    if ( bGetClockSpeedInfo( &stClockSpeedInfo ) != TRUE )
    {
        /* ERROR - cannot obtain clock speed info */
        return( ST_ERROR_FEATURE_NOT_SUPPORTED );
    }

    memset(ClockInfo_p, 0, sizeof(*ClockInfo_p));

    ClockInfo_p->C2 = stClockSpeedInfo.u32CPUClockSpeed;
    ClockInfo_p->CommsBlock = stClockSpeedInfo.u32CPUClockSpeed;
    ClockInfo_p->HPTimer = stClockSpeedInfo.u32ClocksPerSecondHigh;
    ClockInfo_p->LPTimer = stClockSpeedInfo.u32ClocksPerSecondLow;
#endif

    return (ST_NO_ERROR);
}


#if defined (ST_5514)
/********************************************************************

Function:    GetClockIn()

Input:       PLL_Section

Outputs:     ClockIn_p
             Bypass_p

Returns:     None.

********************************************************************/
void GetClockIn (U32 PLL_Section, U32 *ClockIn_p, BOOL *Bypass_p)
{
    U8 SpeedSelect, DividerMode, ClockSelect1, ClockSelect2;
    U32 ClkIn;
    BOOL Bypass;

    ClockSelect1 = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + CKG_CLOCK_SEL1);
    ClockSelect2 = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + CKG_CLOCK_SEL2);
    SpeedSelect = (ClockSelect1 & 0x20) >> 5;
    DividerMode = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + CKG_DIVIDER_MODE) & 0x03;
    if ((DividerMode == 0x01) || (DividerMode == 0x02) ||
        ((SpeedSelect == 0x01) && (DividerMode == 0x03)))
    {
        /* We are in default or programmable mode */
        /* Obtain the clock path */
        if ((ClockSelect1 & PLL_Section) == PLL_Section)
        {
            /* PLLCS_CLOCK input to divider */
            if ((ClockSelect2 & 0x02) == 0x02)
            {
                /* PLL81CS input clock is mpxclock */
                ClkIn = MPX_CLOCK_IN;

                /* When using PLLCS, the final output clock(divider outputs)
                   is going to be the same as the input clock. This is simialar
                   to set a bypass */
                Bypass = TRUE;
            }
            else
            {
                /* PLL81CS input clock is clockin27 */
                ClkIn = CLOCK_IN_27;
                Bypass = TRUE;
            }
        }
        else
        {
            /* PLLFS_CLOCK input to divider */
            if ((ClockSelect2 & 0x01) == 0x01)
            {
                /* PLL81FS input clock is mpxclock */
                ClkIn = MPX_CLOCK_IN;
                if ((ClockSelect2 & 0x04) == 0x04)
                {
                    /* bypass in place, divider outputs sourced from mpxclock */
                    Bypass = TRUE;
                }
                else
                {
                    /* no bypass in place, divider outputs are a function of
                       PLL81 and programmable dividers */
                    Bypass = FALSE;
                }
            }
            else
            {
                /* PLL81FS input clock is clockin27 */
                ClkIn = CLOCK_IN_27;
                if ((ClockSelect1 & 0x08) == 0x08)
                {
                    /* bypass in place, divider outputs sourced from clockin27 */
                    Bypass = TRUE;
                }
                else
                {
                    /* no bypass in place, divider outputs are a function of
                       PLL81 and programmable dividers */
                    Bypass = FALSE;
                }
            }
        }

    }
    else
    {
        /* X1 mode. All the clocks are directly sourced with the 27Mhz clock */
        Bypass = TRUE;
        ClkIn = CLOCK_IN_27;
    }

    *ClockIn_p = ClkIn;
    *Bypass_p = Bypass;
}
#elif defined (ST_5516) || defined(ST_5517)
/********************************************************************

Function:    GetClockIn()

Input:       PLL_Section

Outputs:     ClockIn_p
             Bypass_p

Returns:     None.

********************************************************************/
void GetClockIn (U32 PLL_Section, U32 *ClockIn_p, BOOL *Bypass_p)
{
    U8 SpeedSelect, DividerMode, ClockSelect1;
    U32 ClkIn;
    BOOL Bypass;

    ClockSelect1 = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + CKG_CLOCK_SEL1);
    SpeedSelect = (ClockSelect1 & 0x20) >> 5;
    DividerMode = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + CKG_DIVIDER_MODE) & 0x03;
    if ((DividerMode == 0x01) || (DividerMode == 0x02) ||
        ((SpeedSelect == 0x01) && (DividerMode == 0x03)))
    {
        /* PLLFS_CLOCK input to divider */

        /* PLL81FS input clock is clockin27 */
        ClkIn = CLOCK_IN_27;
        if ((ClockSelect1 & 0x08) == 0x08)
        {
            /* bypass in place, divider outputs sourced from clockin27 */
            Bypass = TRUE;
        }
        else
        {
            /* no bypass in place, divider outputs are a function of
               PLL81 and programmable dividers */
            Bypass = FALSE;
        }
    }
    else
    {
        /* X1 mode. All the clocks are directly sourced with the 27Mhz clock */
        Bypass = TRUE;
        ClkIn = CLOCK_IN_27;
    }

    *ClockIn_p = ClkIn;
    *Bypass_p = Bypass;
}
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
/********************************************************************
Function:    GetPLLFreq()

Input:       None.

Outputs:     None.

Returns:     PLL FS Frequency.
********************************************************************/
U32 GetPLLFreq (U32 ClkIn)
{
    U8 N,M,P;
    U32 PLLFreq;

    /* Read the PLL frequency */
    M = STSYS_ReadRegDev8(CKG_BASE_ADDRESS + CKG_PLLFSDIV0_low);
    N = STSYS_ReadRegDev8(CKG_BASE_ADDRESS + CKG_PLLFSDIV0_high);
    P = STSYS_ReadRegDev8(CKG_BASE_ADDRESS + CKG_PLLFSDIV1) & 0x07;
    PLLFreq = ((2*((U32)N)*(ClkIn/1000))/(((U32)M) << P))*1000;

    return (PLLFreq);
}


/********************************************************************
Function:    GetSynthDivider()

Input:       Config0.

Outputs:     None.

Returns:     Synthesizer divider.
********************************************************************/
U32 GetSynthFreq(U32 Config0, U32 fin)
{
    double fout;
    double n, s, ipe1, nd1;
    double ipe2;
    double nd2;
    double Ntap;
    double fr,Tr,Tdl;
    double fmx, Pow, md2;
    U32 SynthConfig, Pow32, s32, n32, k, md232;

    SynthConfig = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + Config0);

    n32 = 1 << (SynthConfig & 0x03);
    n = (double)n32;
    s32 = 2 << ((SynthConfig & 0x1C) >> 2);
    s = (double) s32;
    md232 = (SynthConfig & 0x03E0) >> 5;
    md2 = (double) md232;

    SynthConfig = STSYS_ReadRegDev16LE(CKG_BASE_ADDRESS + Config0 + 0x04);

    ipe1 = (double)SynthConfig;

    nd1 = 32 - md2 - 1;
    k = 16;
    Ntap = 32;
    Pow32 = (1 << (k-1));
    Pow = (double)Pow32;
    fr = (double)16*(fin)/n;
    Tr = 1/(fr);
    Tdl = 1/(Ntap*fr);
    nd2   = nd1 + 1;
    ipe2  = ipe1 - Pow;
    fmx   = (Pow)/(ipe1*(Tr-nd2*Tdl)-ipe2*(Tr-nd1*Tdl));
    fout  = fmx/s;

    return ((U32)fout);
}
#endif


#if defined(ST_OSLINUX) && defined(MODULE)
EXPORT_SYMBOL(ST_GetClockInfo);
EXPORT_SYMBOL(ST_GetClockSpeed);
EXPORT_SYMBOL(ST_GetClocksPerSecond);
EXPORT_SYMBOL(ST_GetClocksPerSecondHigh);
EXPORT_SYMBOL(ST_GetClocksPerSecondLow);
EXPORT_SYMBOL(ST_SetClockSpeed);
#endif /* ST_OSLINUX & MODULE */

