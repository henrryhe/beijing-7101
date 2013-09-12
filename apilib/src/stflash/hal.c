/****************************************************************************

File Name   : hal.c

Description : Flash Memory Hardware Abstraction Layer

Copyright (C) 2005, ST Microelectronics

History     :

    10/09/03 Implemented WriteToBuffer feature in 16 bit M58LW064D flash.
             Added 5528 support.
             
    14/01/05 Added HAL_M28_Lock and HAL_M28_Unlock functions.


References  :

API.PDF "Flash Memory API" Reference DVD-API-004 Revision 1.9

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#ifndef ST_OS21
#include <ostime.h>
#endif
#include "stlite.h"
#include "stddefs.h"
#include "stos.h"

#include "stflash.h"
#include "stflashd.h"
#include "stflashdat.h"
#include "stcommon.h"
#include "hal.h"
#if defined(STFLASH_SPI_SUPPORT)
#include "stspi.h"
#endif

#if defined(ST_OS21)
U32 oldPriority;
#endif

void HAL_EnterCriticalSection(void) ;
void HAL_LeaveCriticalSection(void) ;

/* Private functions */
static void VppEnable( U32 *VppAddress );
static void VppDisable( U32* VppAddress );

#if defined(STFLASH_SPI_SUPPORT)

/* EPLD register address for CS */
#if defined(ST_5100) || defined(ST_5301)
U32 *SlaveSelect = (U32*)0x41300000;
#elif defined(ST_5105) || defined(ST_5107)
U32 *SlaveSelect = (U32*)0x45180000;
#elif defined(ST_8010)
U32 *SlaveSelect = (U32*)0x47000028;
#elif defined(ST_5525)
U32 *SlaveSelect = (U32*)0x0980C000;
#else
U32 *SlaveSelect = NULL;
#endif


#endif
/******For future use ******
ST_ErrorCode_t HAL_M28_Read( U32              Address,                  // offset from BaseAddress
                             U8               *Buffer,                  // base of receipt buffer
                             U32              NumberToRead,             // in bytes
                             U32              *NumberActuallyRead )
{
    return ST_NO_ERROR;
}

ST_ErrorCode_t HAL_M29_Read( U32              Address,                  // offset from BaseAddress
                             U8               *Buffer,                  // base of receipt buffer
                             U32              NumberToRead,             // in bytes
                             U32              *NumberActuallyRead )
{
    return ST_NO_ERROR;
}
***************************/


void HAL_M28_SetReadMemoryArray( U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32 )
{
    switch (AccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            *Addr08 = M28_READ_MA08;
            break;

        case STFLASH_ACCESS_16_BITS:
            *Addr16 = M28_READ_MA16;
            break;

        case STFLASH_ACCESS_32_BITS:
            *Addr32 = M28_READ_MA32;
            break;

        default:
            break;
    }
}

void HAL_M29_SetReadMemoryArray( U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32 )
{
    switch (AccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            *Addr08 = M29_READ_MA08;
            break;

        case STFLASH_ACCESS_16_BITS:
            *Addr16 = M29_READ_MA16;
            break;

        case STFLASH_ACCESS_32_BITS:
            *Addr32 = M29_READ_MA32;
            break;

        default:
            break;
    }
}

ST_ErrorCode_t HAL_M28_Write( HAL_WriteParams_t WriteParams,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten )
{
    U32             LeftToProg;         /* bytes left to program */
    U32             OperaWidth;         /* Operation Width (in bytes) */

    BOOL            DoingWrite;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    U8              *TempAddress;       /* for recaste */
    U8              *Access_Addr;       /* BaseAddress + Offset */
    U8              *SourceBuf08;       /* Buffer */
    U16             *SourceBuf16;       /* (U16*)Buffer */
    U32             *SourceBuf32;       /* (U32*)Buffer */
    DU8             *WriteAddr08;       /*  8 bit write access address */
    DU16            *WriteAddr16;       /* 16 bit write access address */
    DU32            *WriteAddr32;       /* 32 bit write access address */

    clock_t         StartTime;          /* time at which Program started */
    clock_t         Curr_Time;          /* current (low-priority) time */


    TempAddress = (U8*)WriteParams.BaseAddress;       /* recaste */
    Access_Addr = TempAddress + Offset;             /* user offset */

    /* Check addresses and length for MIN alignment.
       Takes no action for byte minimum, but will
       kick-in if minimum access width is bigger than a byte.*/

    if ((((U32)Access_Addr &
           WriteParams.MinAccessMask) != 0) ||    /* dest. off MIN align? */
        (((U32)Buffer &
           WriteParams.MinAccessMask) != 0) ||    /* source off MIN align? */
        ((NumberToWrite &
          WriteParams.MinAccessMask) != 0))     /* length off MIN align? */
    {
        *NumberActuallyWritten = 0;
        return(ST_ERROR_BAD_PARAMETER);          /* sorry, too hard to do */
    }

    /* determine the (initial) access width to be used */
    if ((((U32)Access_Addr &
          WriteParams.MaxAccessMask) == 0) &&  /* dest. on MAX boundary? */
        (((U32)Buffer &
          WriteParams.MaxAccessMask) == 0) &&  /* source on MAX boundary? */
        (NumberToWrite >=
         WriteParams.MaxAccessWidth))           /* enough bytes for MAX? */
    {
        OperaWidth = WriteParams.MaxAccessWidth;   /* high speed operation */
    }
    else
    {
        OperaWidth = WriteParams.MinAccessWidth;   /* lower speed operation */
    }

    /* form access pointers for all widths */
    WriteAddr08 = Access_Addr;
    WriteAddr16 = (DU16*)Access_Addr;
    WriteAddr32 = (DU32*)Access_Addr;
    SourceBuf08 = Buffer;
    SourceBuf16 = (U16*)Buffer;
    SourceBuf32 = (U32*)Buffer;

    /* perform precautionary clear across the Maximum access width */
    switch (WriteParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            *WriteAddr08 = CLEAR_SR08;
            break;

        case STFLASH_ACCESS_16_BITS:
            *WriteAddr16 = CLEAR_SR16;
            break;

        case STFLASH_ACCESS_32_BITS:
            *WriteAddr32 = CLEAR_SR32;
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

    /* turn on the programming voltage */
    VppEnable( WriteParams.VppAddress );

    LeftToProg = NumberToWrite;                     /* initialize counter */

    /* whilst more bytes to Program and no fatal errors */
    while ((LeftToProg > 0) &&
           ((RetValue == ST_NO_ERROR)))/* ||
             ( RetValue != STFLASH_ERROR_VPP_LOW) ) )*/
    {
        switch (OperaWidth)                      /* choose appropriate size */
        {
            case STFLASH_ACCESS_08_BITS:

                /* commence Program Command Sequence */
                *WriteAddr08 = PROG_SU08;           /* "Don't Care" address */
                *WriteAddr08 = *SourceBuf08++;      /* write from Buffer */
                StartTime = time_now();             /* Program start ticks */

                /* commence Read Status Register Command Sequence */
                *WriteAddr08 = READ_SR08;           /* "Don't Care" address */

                /* poll for completion of Programming */
                DoingWrite = TRUE;
                while (DoingWrite)
                {
                    if ((*WriteAddr08 & COM_STAT08) == COM_STAT08)
                    {
                        DoingWrite = FALSE;        /* Program sequence done */

                        /* check for Program Sequence failure */
                        if ((*WriteAddr08 & PRG_STAT08) != 0)
                        {
                            RetValue = STFLASH_ERROR_WRITE;
                        }
                        /* check for Vpp below minimum */
                        else if ((*WriteAddr08 & VPP_STAT08) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                    else
                    {
                        Curr_Time = time_now();
                        if (time_minus( Curr_Time, StartTime )>
                            (clock_t)WRITE_TIMEOUT)
                        {
                             /* Program sequence timed out */
                             DoingWrite = FALSE;
                             RetValue = ST_ERROR_TIMEOUT;
                        }
                    }
                }

                /* Program sequence completed successfully? */
                if ((RetValue == ST_NO_ERROR) ||
                    (RetValue == STFLASH_ERROR_VPP_LOW))
                {
                    /* move to next address in device */
                    ++WriteAddr08;
                    LeftToProg -= OperaWidth;
                }
                break;


            case STFLASH_ACCESS_16_BITS:

                /* commence Program Command Sequence */
                *WriteAddr16 = PROG_SU16;           /* "Don't Care" address */
                *WriteAddr16 = *SourceBuf16++;      /* write from Buffer */
                StartTime = time_now();             /* Program start ticks */

                /* commence Read Status Register Command Sequence */
                *WriteAddr16 = READ_SR16;           /* "Don't Care" address */

                /* poll for completion of Programming */
                DoingWrite = TRUE;
                while (DoingWrite)
                {
                    if ((*WriteAddr16 & COM_STAT16) == COM_STAT16)
                    {
                        DoingWrite = FALSE;        /* Program sequence done */

                        /* check for Program Sequence failure */
                        if ((*WriteAddr16 & PRG_STAT16) != 0)
                        {
                            RetValue = STFLASH_ERROR_WRITE;
                        }
                        /* check for Vpp below minimum */
                        else if ((*WriteAddr16 & VPP_STAT16) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                    else
                    {
                        Curr_Time = time_now();
                        if (time_minus( Curr_Time, StartTime) >
                            (clock_t)WRITE_TIMEOUT)
                        {
                            /* Program sequence timed out */
                            DoingWrite = FALSE;
                            RetValue = ST_ERROR_TIMEOUT;
                        }
                    }
                }

                /* Program sequence completed successfully? */
                if ((RetValue == ST_NO_ERROR) ||
                    (RetValue == STFLASH_ERROR_VPP_LOW))
                {
                    /* move to next address in device */
                    ++WriteAddr16;
                    LeftToProg -= OperaWidth;

                    /* less than MAX left? */
                    if (LeftToProg < WriteParams.MaxAccessWidth)
                    {
                        OperaWidth =                    /* drop down to MIN */
                            WriteParams.MinAccessWidth;

                        /* transfer ptrs */
                        WriteAddr08 = (DU8*)WriteAddr16;
                        SourceBuf08 = (U8*)SourceBuf16;
                    }
                }
                break;


            case STFLASH_ACCESS_32_BITS:

                /* commence Program Command Sequence */
                *WriteAddr32 = PROG_SU32;           /* "Don't Care" address */
                *WriteAddr32 = *SourceBuf32++;      /* write from Buffer */
                StartTime = time_now();             /* Program start ticks */

                /* commence Read Status Register Command Sequence */
                *WriteAddr32 = READ_SR32;           /* "Don't Care" address */

                /* poll for completion of Programming */
                DoingWrite = TRUE;
                while (DoingWrite)
                {
                    if ((*WriteAddr32 & COM_STAT32) == COM_STAT32)
                    {
                        DoingWrite = FALSE;        /* Program sequence done */

                        /* check for Program Sequence failure */
                        if ((*WriteAddr32 & PRG_STAT32) != 0)
                        {
                            RetValue = STFLASH_ERROR_WRITE;
                        }
                        /* check for Vpp below minimum */
                        else if ((*WriteAddr32 & VPP_STAT32) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                    else
                    {
                        Curr_Time = time_now();
                        if (time_minus( Curr_Time, StartTime ) >
                            (clock_t)WRITE_TIMEOUT)
                        {
                            /* Program sequence timed out */
                            DoingWrite = FALSE;
                            RetValue = ST_ERROR_TIMEOUT;
                        }
                    }
                }

                /* Program sequence completed successfully? */
                if ((RetValue == ST_NO_ERROR)) /* ||
                    ( RetValue != STFLASH_ERROR_VPP_LOW ) )*/
                {
                    /* move to next address in device */
                    ++WriteAddr32;
                    LeftToProg -= OperaWidth;

                    /* less than MAX left? */
                    if (LeftToProg < WriteParams.MaxAccessWidth)
                    {
                        OperaWidth =        /* drop down to MIN */
                            WriteParams.MinAccessWidth;


                        /* transfer ptrs */
                        WriteAddr08 = (DU8*)WriteAddr32;
                        WriteAddr16 = (DU16*)WriteAddr32;
                        SourceBuf08 = (U8*)SourceBuf32;
                        SourceBuf16 = (U16*)SourceBuf32;
                    }
                }
                break;


            default:
                /* this SHOULDN'T happen */
                RetValue = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }

    VppDisable( WriteParams.VppAddress );

    *NumberActuallyWritten =
                    NumberToWrite - LeftToProg;

    /* restore device to read mode */
    switch (WriteParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            WriteAddr08 = Access_Addr;
            break;

        case STFLASH_ACCESS_16_BITS:
            WriteAddr16 = (U16*)Access_Addr;
            break;

        case STFLASH_ACCESS_32_BITS:
            WriteAddr32 = (U32*)Access_Addr;
            break;

        default:
            break;
    }
    
    HAL_M28_SetReadMemoryArray( WriteParams.MaxAccessWidth, WriteAddr08, WriteAddr16, WriteAddr32 );

    return (RetValue);
}


ST_ErrorCode_t HAL_M28_WriteToBuffer( HAL_WriteParams_t WriteParams,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten )
{
    U32             LeftToProg;         /* bytes left to program */
    U32             OperaWidth;         /* Operation Width (in bytes) */

    BOOL            DoingWrite;
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    U8              *TempAddress;       /* for recaste */
    U8              *Access_Addr;       /* BaseAddress + Offset */
    U8              *SourceBuf08;       /* Buffer */
    U16             *SourceBuf16;       /* (U16*)Buffer */
    U32             *SourceBuf32;       /* (U32*)Buffer */
    DU8             *WriteAddr08;       /*  8 bit write access address */
    DU16            *WriteAddr16;       /* 16 bit write access address */
    DU32            *WriteAddr32;       /* 32 bit write access address */

    clock_t         StartTime = 0;          /* time at which Program started */
    clock_t         Curr_Time = 0;          /* current (low-priority) time */
    U32             WordsInBuffer;
    U32             counter = 0;


    TempAddress = (U8*)WriteParams.BaseAddress;       /* recaste */
    Access_Addr = TempAddress + Offset;               /* user offset */

    /* Check addresses and length for MIN alignment.
       Takes no action for byte minimum, but will
       kick-in if minimum access width is bigger than a byte.*/

    if ((((U32)Access_Addr &
          WriteParams.MinAccessMask) != 0) ||    /* dest. off MIN align? */
        (((U32)Buffer &
          WriteParams.MinAccessMask) != 0) ||    /* source off MIN align? */
        ((NumberToWrite &
          WriteParams.MinAccessMask) != 0))      /* length off MIN align? */
    {
        *NumberActuallyWritten = 0;
        return(ST_ERROR_BAD_PARAMETER);           /* sorry, too hard to do */
    }

    /* determine the (initial) access width to be used */
    if ((((U32)Access_Addr &
          WriteParams.MaxAccessMask) == 0) &&    /* dest. on MAX boundary? */
        (((U32)Buffer &
            WriteParams.MaxAccessMask) == 0) &&  /* source on MAX boundary? */
        (NumberToWrite >=
          WriteParams.MaxAccessWidth))           /* enough bytes for MAX? */
    {
        OperaWidth = WriteParams.MaxAccessWidth; /* high speed operation */
    }
    else
    {
        OperaWidth = WriteParams.MinAccessWidth; /* lower speed operation */
    }

    /* form access pointers for all widths */
    WriteAddr08 = Access_Addr;
    WriteAddr16 = (U16*)Access_Addr;
    WriteAddr32 = (U32*)Access_Addr;
    SourceBuf08 = Buffer;
    SourceBuf16 = (U16*)Buffer;
    SourceBuf32 = (U32*)Buffer;

    /* perform precautionary clear across the Maximum access width */
    switch (WriteParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            *WriteAddr08 = CLEAR_SR08;
            break;

        case STFLASH_ACCESS_16_BITS:
            *WriteAddr16 = CLEAR_SR16;
            break;

        case STFLASH_ACCESS_32_BITS:
            *WriteAddr32 = CLEAR_SR32;
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

    /* turn on the programming voltage */
    VppEnable( WriteParams.VppAddress );

    LeftToProg = NumberToWrite;                     /* initialize counter */

    /* whilst more bytes to Program and no fatal errors */
    while ((LeftToProg > 0) &&
          ((RetValue == ST_NO_ERROR)))/* ||
             ( RetValue != STFLASH_ERROR_VPP_LOW) ) )*/
    {
        switch (OperaWidth)                      /* choose appropriate size */
        {

            case STFLASH_ACCESS_08_BITS:

                /* commence Program Command Sequence */
                *WriteAddr08 = PROG_SU08;           /* "Don't Care" address */
                *WriteAddr08 = *SourceBuf08++;      /* write from Buffer */
                StartTime = time_now();             /* Program start ticks */

                /* commence Read Status Register Command Sequence */
                *WriteAddr08 = READ_SR08;           /* "Don't Care" address */

                /* poll for completion of Programming */
                DoingWrite = TRUE;
                while (DoingWrite)
                {
                    if ((*WriteAddr08 & COM_STAT08) == COM_STAT08)
                    {
                        DoingWrite = FALSE;        /* Program sequence done */

                        /* check for Program Sequence failure */
                        if ((*WriteAddr08 & PRG_STAT08) != 0)
                        {
                            RetValue = STFLASH_ERROR_WRITE;
                        }
                        /* check for Vpp below minimum */
                        else if ((*WriteAddr08 & VPP_STAT08) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                    else
                    {
                        Curr_Time = time_now();
                        if (time_minus( Curr_Time, StartTime ) >
                            (clock_t)WRITE_TIMEOUT)
                        {
                             /* Program sequence timed out */
                             DoingWrite = FALSE;
                             RetValue = ST_ERROR_TIMEOUT;
                        }
                    }
                }

                /* Program sequence completed successfully? */
                if ((RetValue == ST_NO_ERROR) ||
                    (RetValue == STFLASH_ERROR_VPP_LOW))
                {
                    /* move to next address in device */
                    ++WriteAddr08;
                    LeftToProg -= OperaWidth;
                }
                break;


            case STFLASH_ACCESS_16_BITS:

                /* Till 3 words(6bytes) Normal flash Write consumes less cycles
                   to write into flash */

                if (LeftToProg < (OperaWidth * FLASH_WRITE_BUFFER_SIZE))
                {

                    WriteAddr16 =(U16*)Access_Addr;
                    /* commence Program Command Sequence */
                    *WriteAddr16 = PROG_SU16;           /* "Don't Care" address */
                    *WriteAddr16 = *SourceBuf16++;      /* write from Buffer */
                    StartTime = time_now();             /* Program start ticks */

                    /* commence Read Status Register Command Sequence */
                    *WriteAddr16 = READ_SR16;           /* "Don't Care" address */

                    /* poll for completion of Programming */
                    DoingWrite= TRUE;
                    while (DoingWrite)
                    {
                        if ((*WriteAddr16 & COM_STAT16) == COM_STAT16)
                        {
                            DoingWrite = FALSE;        /* Program sequence done */

                            /* check for Program Sequence failure */
                            if ((*WriteAddr16 & PRG_STAT16) != 0)
                            {
                                RetValue = STFLASH_ERROR_WRITE;
                            }
                            /* check for Vpp below minimum */
                            else if ((*WriteAddr16 & VPP_STAT16) != 0)
                            {
                                RetValue = STFLASH_ERROR_VPP_LOW;
                            }
                        }
                        else
                        {
                            Curr_Time = time_now();
                            if (time_minus( Curr_Time, StartTime) >
                                (clock_t)WRITE_TIMEOUT)
                            {
                                /* Program sequence timed out */
                                DoingWrite= FALSE;
                                RetValue = ST_ERROR_TIMEOUT;
                            }
                        }
                    }

                    /* Program sequence completed successfully? */
                    if ((RetValue == ST_NO_ERROR) ||
                        (RetValue == STFLASH_ERROR_VPP_LOW))
                    {
                  
                        /* move to next address in device */
                        ++WriteAddr16;
                        Access_Addr+= OperaWidth;
                        LeftToProg -= OperaWidth;      
                        
                        /* less than MAX left? */
                        if (LeftToProg < WriteParams.MaxAccessWidth)
                        {
                            OperaWidth =                    /* drop down to MIN */
                                WriteParams.MinAccessWidth;

                            /* transfer ptrs */
                            WriteAddr08 = (DU8*)WriteAddr16;
                            SourceBuf08 = (U8*)SourceBuf16;
                        }

                    }
                }
                else
                {

                    /* commence Write To Buffer Command Sequence */
                    /* 1st cycle with BA & SetUp Command */
                    WriteAddr16 =(U16*)Access_Addr;
                    *WriteAddr16 = PROG_BUFF16;

                    /* set the counter to 0 */
                    counter = 0;

                    /* Check status register for availability of buffer */
                    do
                    {

                        if (counter < SETTLING_TIME)
                        {
                            counter++;
                        }
                        else
                        {
                            RetValue = ST_ERROR_TIMEOUT;
                            break;
                        }

                    }
                    while ((*WriteAddr16 & COM_STAT16) != COM_STAT16);

                    /* 2nd cycle with BaseAddress & Number to prog */
                    *WriteAddr16 = FLASH_WRITE_BUFFER_SIZE - 1;

                    WordsInBuffer = 0;

                    /* 3rd & subsequent cycles with Programming Address & Data */
                    while ((WordsInBuffer < FLASH_WRITE_BUFFER_SIZE) && (RetValue == ST_NO_ERROR))
                    {
                        *WriteAddr16++ = *SourceBuf16++;
                        Access_Addr += OperaWidth;
                        WordsInBuffer++;
                        LeftToProg-= OperaWidth;
                    }

                    /* Last cycle with 'don't care' address & confirm command*/
                    WriteAddr16 = (U16*)TempAddress;
                    *WriteAddr16 = PROG_CR16;

                    /* reset the counter */
                    counter = 0;

                    /* Wait until every Action is finished (StatusRegister Bit7 = 1) */
                    do
                    {
                        if (counter < SETTLING_TIME)
                        {
                            counter++;
                        }
                        else
                        {
                            RetValue = ST_ERROR_TIMEOUT;
                            break;
                        }

                    }
                    while ((*WriteAddr16 & COM_STAT16) != COM_STAT16);

                    /* commence Read Status Register Command Sequence */
                    *WriteAddr16 = READ_SR16;    /* "Don't Care" address in Flash */

                    /* poll for completion of Programming */
                    DoingWrite = TRUE;
                    while (DoingWrite)
                    {
                        if ((*WriteAddr16 & COM_STAT16) == COM_STAT16)
                        {
                            DoingWrite = FALSE;        /* Program sequence done */

                            /* check for Program Sequence failure */
                            if ((*WriteAddr16 & PRG_STAT16) != 0)
                            {
                                RetValue = STFLASH_ERROR_WRITE;
                            }

                            /* check for Vpp below minimum */
                            else if ((*WriteAddr16 & VPP_STAT16) != 0)
                            {
                                RetValue = STFLASH_ERROR_VPP_LOW;
                            }
                        }
                        else
                        {
                            Curr_Time = time_now();
                            if (time_minus( Curr_Time, StartTime ) >
                                (clock_t)WRITE_TIMEOUT)
                            {
                                /* Program sequence timed out */
                                DoingWrite= FALSE;
                                RetValue = ST_ERROR_TIMEOUT;
                            }
                        }
                    }
                }
                  
                break;

            case STFLASH_ACCESS_32_BITS:

                /* commence Program Command Sequence */
                *WriteAddr32 = PROG_SU32;           /* "Don't Care" address */
                *WriteAddr32 = *SourceBuf32++;      /* write from Buffer */
                StartTime = time_now();             /* Program start ticks */

                /* commence Read Status Register Command Sequence */
                *WriteAddr32 = READ_SR32;           /* "Don't Care" address */

                /* poll for completion of Programming */
                DoingWrite= TRUE;
                while (DoingWrite)
                {
                    if ((*WriteAddr32 & COM_STAT32) == COM_STAT32)
                    {
                         DoingWrite= FALSE;        /* Program sequence done */

                        /* check for Program Sequence failure */
                        if ((*WriteAddr32 & PRG_STAT32) != 0)
                        {
                            RetValue = STFLASH_ERROR_WRITE;
                        }
                        /* check for Vpp below minimum */
                        else if ((*WriteAddr32 & VPP_STAT32) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                    else
                    {
                        Curr_Time = time_now();
                        if (time_minus( Curr_Time, StartTime) >
                            (clock_t)WRITE_TIMEOUT)
                        {
                            /* Program sequence timed out */
                             DoingWrite= FALSE;
                            RetValue = ST_ERROR_TIMEOUT;
                        }
                    }
                }

                /* Program sequence completed successfully? */
                if ((RetValue == ST_NO_ERROR)) /* ||
                    ( RetValue != STFLASH_ERROR_VPP_LOW ) )*/
                {
                   /* move to next address in device */
                    ++WriteAddr32;
                    LeftToProg -= OperaWidth;

                    /* less than MAX left? */
                    if (LeftToProg < WriteParams.MaxAccessWidth)
                    {
                        OperaWidth =                    /* drop down to MIN */
                            WriteParams.MinAccessWidth;

                        /* transfer ptrs */
                        WriteAddr08 = (DU8*)WriteAddr32;
                        WriteAddr16 = (DU16*)WriteAddr32;
                        SourceBuf08 = (U8*)SourceBuf32;
                        SourceBuf16 = (U16*)SourceBuf32;
                    }
                }
                break;

            default:
                /* this SHOULDN'T happen */
                RetValue = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }

    VppDisable( WriteParams.VppAddress );


    *NumberActuallyWritten =
                    NumberToWrite - LeftToProg;

    /* restore device to read mode */
    switch (WriteParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:
            WriteAddr08 = Access_Addr;
            break;

        case STFLASH_ACCESS_16_BITS:
            WriteAddr16 = (U16*)Access_Addr;
            break;

        case STFLASH_ACCESS_32_BITS:
            WriteAddr32 = (U32*)Access_Addr;
            break;

        default:
            break;
    }
    HAL_M28_SetReadMemoryArray( WriteParams.MaxAccessWidth, WriteAddr08, WriteAddr16, WriteAddr32 );
    return (RetValue);
}

ST_ErrorCode_t HAL_M28_Lock( HAL_LockUnlockParams_t LockParams,
                             U32              Offset )
{

    ST_ErrorCode_t  RetVal = ST_NO_ERROR;
    U32 counter = 0;
    U8      *TempAddress;   /* for recasting */
    U8      *Access_Addr;   /* BaseAddress + Offset */
    DU32    *BlockAddr32;   /* 32 bit block address */
    DU16    *BlockAddr16;   /* 16 bit block address */
    DU8     *BlockAddr08;   /*  8 bit block address */
    
    TempAddress = (U8*)LockParams.BaseAddress;       /* recasting */
    Access_Addr = TempAddress + Offset;              /* block base */

    /* form all width pointers, only one will be used */
    BlockAddr32 = (U32*)(Access_Addr);
    BlockAddr16 = (U16*)(Access_Addr);
    BlockAddr08 = (Access_Addr);
    
    /* turn on the programming voltage */
    VppEnable( LockParams.VppAddress );

    switch (LockParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_16_BITS:   
                 
            *BlockAddr16 = M58LW064_BLKLOCK_16;
            *BlockAddr16 = BLK_LOCK_16;   
                        
            /* Set to read status reg*/
            *BlockAddr16 = READ_SR16;
    
            /* Check status register for availability of buffer */
            while (((*BlockAddr16 & COM_STAT16) != COM_STAT16) &&    \
                    (counter++ < SETTLING_TIME ))
            {
                task_delay(100);
            
                /* Set to read status reg */
                *BlockAddr16 = READ_SR16;
            }     
                                    
            /* check for Program Sequence failure */
            if ((*BlockAddr16 & PRG_STAT16) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            /* check for Vpp below minimum */
            else if ((*BlockAddr16 & VPP_STAT16) != 0)
            {
                RetVal= STFLASH_ERROR_VPP_LOW;
            }
            else if ((*BlockAddr16 & BUE_STAT16) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }

            break;

        case STFLASH_ACCESS_32_BITS:

            *BlockAddr32 = M58LW064_BLKLOCK_32;
            *BlockAddr32 = BLK_LOCK_32;

            *BlockAddr32 = READ_SR32;
                
            /* Check status register for availability of buffer */
            while (((*BlockAddr32 & COM_STAT32) != COM_STAT32) &&    \
                    (counter++ < SETTLING_TIME ))
            {
                task_delay(100);
                
                /* Set to read status reg */
                *BlockAddr32 = READ_SR32;
            }     
                                    
            /* check for Program Sequence failure */
            if ((*BlockAddr32 & PRG_STAT32) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            /* check for Vpp below minimum */
            else if ((*BlockAddr32 & VPP_STAT32) != 0)
            {
                RetVal= STFLASH_ERROR_VPP_LOW;
            }
            else if ((*BlockAddr32 & BUE_STAT32) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            break;
            
        default:
            RetVal = ST_ERROR_BAD_PARAMETER;
            break;               
            
    }

    /* restore to ReadArray mode */
    HAL_M28_SetReadMemoryArray( LockParams.MaxAccessWidth, BlockAddr08, BlockAddr16, BlockAddr32 );
    
    return (RetVal);
    
}

ST_ErrorCode_t HAL_M28_Unlock( HAL_LockUnlockParams_t UnlockParams,
                               U32              Offset )
{

    ST_ErrorCode_t  RetVal = ST_NO_ERROR;
    U32     counter = 0;
    U8      *TempAddress;   /* for recasting */
    U8      *Access_Addr;   /* BaseAddress + Offset */
    DU32    *BlockAddr32;   /* 32 bit block address */
    DU16    *BlockAddr16;   /* 16 bit block address */
    DU8     *BlockAddr08;   /*  8 bit block address */

    TempAddress = (U8*)UnlockParams.BaseAddress;     /* recasting */
    Access_Addr = TempAddress + Offset;              /* block base */

    /* form all width pointers, only one will be used */
    BlockAddr32 = (U32*)(Access_Addr);
    BlockAddr16 = (U16*)(Access_Addr);
    BlockAddr08 = (Access_Addr);
     
    /* turn on the programming voltage */
    VppEnable( UnlockParams.VppAddress );
    
    switch (UnlockParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_16_BITS:
          
            /* Issue unlock block cmd */
            *BlockAddr16 = M58LW064_BLKLOCK_16;
            *BlockAddr16 = BLK_UNLOCK_16;            
          
            /* Set to read status reg */
            *BlockAddr16 = READ_SR16;
            
            while (((*BlockAddr16 & COM_STAT16) != COM_STAT16) &&    \
                    (counter++ < SETTLING_TIME ))
            {
                task_delay(100);
                
                /* Set to read status reg */
                *BlockAddr16 = READ_SR16;
            }            	

            /* check for Program Sequence failure */
            if ((*BlockAddr16 & PRG_STAT16) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            /* check for Vpp below minimum */
            else if ((*BlockAddr16 & VPP_STAT16) != 0)
            {
                RetVal= STFLASH_ERROR_VPP_LOW;
            }
            /* check for Block Unprotect status */
            else if ((*BlockAddr16 & BPE_STAT16) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            break;

        case STFLASH_ACCESS_32_BITS:

            /* Issue unlock block cmd */
            *BlockAddr32 = M58LW064_BLKLOCK_32;
            *BlockAddr32 = BLK_UNLOCK_32;
            
            /* Set to read status reg */
            *BlockAddr32 = READ_SR32;
            
            while (((*BlockAddr32 & COM_STAT32) != COM_STAT32) &&    \
                    (counter++ < SETTLING_TIME ))
            {
                task_delay(100);
                
                /* Set to read status reg */
                *BlockAddr32 = READ_SR32;
            }    
            
            /* check for Program Sequence failure */
            if ((*BlockAddr32 & PRG_STAT32) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            /* check for Vpp below minimum */
            else if ((*BlockAddr32 & VPP_STAT32) != 0)
            {
                RetVal= STFLASH_ERROR_VPP_LOW;
            }
            /* check for Block Unprotect status */
            else if ((*BlockAddr32 & BPE_STAT32) != 0)
            {
                RetVal = STFLASH_ERROR_WRITE;
            }
            break;
            
        default:
            RetVal = ST_ERROR_BAD_PARAMETER;
            break;

    }
    
    /* restore to ReadArray mode */
    HAL_M28_SetReadMemoryArray( UnlockParams.MaxAccessWidth, BlockAddr08, BlockAddr16, BlockAddr32 );
    
    return (RetVal);    
    
}


ST_ErrorCode_t HAL_M29_Write( HAL_WriteParams_t WriteParams,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten )
{
    S32             LeftToProg;         /* bytes left to program */
    U32             OperaWidth;         /* Operation Width (in bytes) */

    clock_t         StartTime;          /* time at which Program started */
    clock_t         Curr_Time;          /* current (low-priority) time */
    U8              *Access_Addr;       /* BaseAddress + Offset */
    U16             *SourceBuf16;       /* (U16*)Buffer */
    DU16            *WriteAddr16;       /* 16 bit write access address */
    U32             *SourceBuf32;       /* (U32*)Buffer */
    DU32            *WriteAddr32;       /* 32 bit write access address */
    DU32            *Code1Addr;         /* Address for coded cycle 1 */
    DU32            *Code2Addr;         /* Address for coded cycle 2 */
    DU16            *Code1Addr16;       /* Address for coded cycle 1 */
    DU16            *Code2Addr16;       /* Address for coded cycle 2 */
    U16             Code1Data16;        /* Data for coded cycle 1 */
    U16             Code2Data16;        /* Data for coded cycle 2 */
    U32             Code1Data;          /* Data for coded cycle 1 */
    U32             Code2Data;          /* Data for coded cycle 2 */
    U32             Status1;            /* Successive status reads */

    ST_ErrorCode_t  RetValue = ST_NO_ERROR; /* default return value */

    /* This implementation only supports the M29W800T
       as it is configured on the ST mb282 & mb275 boards so
       we can only access in 16 or 32 bit widths */
    if ((WriteParams.MinAccessWidth != STFLASH_ACCESS_16_BITS) &&
        (WriteParams.MinAccessWidth != STFLASH_ACCESS_32_BITS))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Setup initial program address as a U8 pointer */
    Access_Addr = (U8*)WriteParams.BaseAddress + Offset;

    /* Check addresses and length for MIN alignment.
       Takes no action for byte minimum, but will
       kick-in if minimum access width is bigger than a byte.*/

    if ((((U32)Access_Addr &
          WriteParams.MinAccessMask) != 0) ||     /* dest. off MIN align? */
        (((U32)Buffer &
          WriteParams.MinAccessMask) != 0) ||     /* source off MIN align? */
        ((NumberToWrite &
          WriteParams.MinAccessMask) != 0))       /* length off MIN align? */
    {
        *NumberActuallyWritten = 0;
        return(ST_ERROR_BAD_PARAMETER);          /* sorry, too hard to do */
    }
    /* determine the (initial) access width to be used */
    if ((((U32)Access_Addr &
          WriteParams.MaxAccessMask) == 0) &&  /* dest. on MAX boundary? */
        (((U32)Buffer &
          WriteParams.MaxAccessMask) == 0) &&  /* source on MAX boundary? */
        (NumberToWrite >=
         WriteParams.MaxAccessWidth))           /* enough bytes for MAX? */
    {
        OperaWidth = WriteParams.MaxAccessWidth;   /* high speed operation */
    }
    else
    {
        OperaWidth = WriteParams.MinAccessWidth;   /* lower speed operation */
    }

    /* Setup pointers and data variables */
    WriteAddr16 = (U16*)Access_Addr;
    WriteAddr32 = (U32*)Access_Addr;
    SourceBuf16 = (U16*)Buffer;
    SourceBuf32 = (U32*)Buffer;
    Code1Addr   = (U32*)WriteParams.BaseAddress + M29W800T_CODED_ADDR1;
    Code2Addr   = (U32*)WriteParams.BaseAddress + M29W800T_CODED_ADDR2;
    Code1Data   = M29W800T_CODED_DATA1;
    Code2Data   = M29W800T_CODED_DATA2;
    Code1Addr16 = (U16*)WriteParams.BaseAddress + M29W800T_CODED_ADDR1;
    Code2Addr16 = (U16*)WriteParams.BaseAddress + M29W800T_CODED_ADDR2;
    Code1Data16 = M29W800T_CODED_DATA1 & 0xffff;
    Code2Data16 = M29W800T_CODED_DATA2 & 0xffff;
    LeftToProg  = NumberToWrite;

    /* whilst more bytes to Program and no fatal errors */
    while ((LeftToProg > 0) &&
          (RetValue == ST_NO_ERROR))
    {
        switch (OperaWidth)                      /* choose appropriate size */
        {

            case STFLASH_ACCESS_16_BITS:

                while (LeftToProg >= (S32)OperaWidth)
                {
                    U16 WriteValue;
                   
                    HAL_EnterCriticalSection();
                    /* Cycle 1 - Coded cycle 1 */
                    *Code1Addr16 = Code1Data16;

                    /* Cycle 2 - Coded cycle 2 */
                    *Code2Addr16 = Code2Data16;

                    /* Cycle 3 - Program Command */
                    *Code1Addr16 = M29W800T_PROGRAM & 0xffff;

                    /* Cycle 4 - Program Data */
                    WriteValue = *SourceBuf16++;
                    *WriteAddr16 = WriteValue;     /* write from Buffer */
                    
                    HAL_LeaveCriticalSection();
                    /* This algorithm works better than one based on data toggle polling (see datasheet) */
                    /* A delay is needed before the data toggle mechanism will exit successfully with the old algorithm*/
                    /* This is an issue for programming FFFFxxxx or xxxxFFFF after an erase */
                    /* see DDTS 9898 */

                    StartTime = time_now();             /* Program start ticks */
                    while (1)
                    {
                        Status1 = *WriteAddr16;   /* Read DQ5 & DQ7 at valid address */
                        if ((Status1 & M29W800T_DQ7_MASK)==(WriteValue & M29W800T_DQ7_MASK))
                        {   /* DQ7 = DATA */
                            RetValue = ST_NO_ERROR;
                            break;
                        }
                        else
                        {
                            if ((Status1 & M29W800T_DQ5_MASK)==1)
                            {   /* DQ5 = 1 */
                                Status1 = *WriteAddr16;
                                if ((Status1 & M29W800T_DQ7_MASK)==(WriteValue & M29W800T_DQ7_MASK))
                                {
                                    RetValue = ST_NO_ERROR;
                                    break;
                                }
                                else
                                {
                                    RetValue = STFLASH_ERROR_WRITE;
                                    break;
                                }
                            }
                            else
                            {

                                /* exit if timed out, otherwise continue the loop */
                                Curr_Time = time_now();
                                if ((time_minus( Curr_Time, StartTime) >
                                     (clock_t)WRITE_TIMEOUT))    /*zxc 2008-12-15 add make the TIME OUT more larger*/
                                {
                                    RetValue = ST_ERROR_TIMEOUT;
                                    break;
                                }

                            }
                        }
                    }

                    if (RetValue == ST_NO_ERROR)
                    {
                        /* Program sequence completed successfully? */
                        /* move to next address in device */
                        WriteAddr16++;
                        LeftToProg -= OperaWidth;
                    }
                    else
                    {
                        break;
                    }

                }

                /* Any more left to program? */
                if ((LeftToProg > 0) && (RetValue == ST_NO_ERROR))
                {
                    /* We are already at the minimum width */
                    RetValue = ST_ERROR_BAD_PARAMETER;
                }
                break;


            case STFLASH_ACCESS_32_BITS:

                while (LeftToProg >= (S32)OperaWidth)
                {
                    U32 WriteValue;

                    /* Cycle 1 - Coded cycle 1 */
                    *Code1Addr = Code1Data;

                    /* Cycle 2 - Coded cycle 2 */
                    *Code2Addr = Code2Data;

                    /* Cycle 3 - Program Command */
                    *Code1Addr = M29W800T_PROGRAM;

                    /* Cycle 4 - Program Data */
                    WriteValue = *SourceBuf32++;
                    *WriteAddr32 = WriteValue;     /* write from Buffer */


                    /* This algorithm works better than one based on data toggle polling (see datasheet) */
                    /* A delay is needed before the data toggle mechanism will exit successfully (old algorithm) */
                    /* This is an issue for programming FFFFxxxx or xxxxFFFF after an erase */
                    /* see DDTS 9898 */

                    StartTime = time_now();             /* Program start ticks */
                    while (1)
                    {
                        Status1 = *WriteAddr32;   /* Read DQ5 & DQ7 at valid address */
                        if ((Status1 & M29W800T_DQ7_MASK)==(WriteValue & M29W800T_DQ7_MASK))
                        {   /* DQ7 = DATA */
                            RetValue = ST_NO_ERROR;
                            break;
                        }
                        else
                        {
                            if ((Status1 & M29W800T_DQ5_MASK)==1)
                            {   /* DQ5 = 1 */
                                Status1 = *WriteAddr32;
                                if ((Status1 & M29W800T_DQ7_MASK)==(WriteValue & M29W800T_DQ7_MASK))
                                {
                                    RetValue = ST_NO_ERROR;
                                    break;
                                }
                                else
                                {
                                    RetValue = STFLASH_ERROR_WRITE; /* not ideal; use Pass variable instead*/
                                    break;
                                }
                            }
                            else
                            {    /* exit if timed out, otherwise continue the loop */
                                Curr_Time = time_now();
                                if (time_minus( Curr_Time, StartTime ) > (clock_t)WRITE_TIMEOUT)
                                {
                                    RetValue = ST_ERROR_TIMEOUT;
                                    break;
                                }
                            }
                        }
                    }

                    if (RetValue == ST_NO_ERROR)
                    {
                        /* Program sequence completed successfully? */
                        /* move to next address in device */
                        WriteAddr32++;
                        LeftToProg -= OperaWidth;
                    }
                    else
                    {
                        break;
                    }
                }

                /* Any more left to program? */
                if (LeftToProg > 0)
                {
                    OperaWidth =                    /* drop down to MIN */
                      WriteParams.MinAccessWidth;

                    /* transfer ptrs */
                    WriteAddr16 = (DU16*)WriteAddr32;
                    SourceBuf16 = (U16*)SourceBuf32;
                }
                break;

            default:
                /* this SHOULDN'T happen */
                RetValue = ST_ERROR_BAD_PARAMETER;
                break;
        }
    }

    /* Write back number actually written to caller */
    *NumberActuallyWritten =
                    NumberToWrite - LeftToProg;

    /* restore device to read mode */
    switch (WriteParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_16_BITS:
            WriteAddr16 = (U16*)Access_Addr;
            break;

        case STFLASH_ACCESS_32_BITS:
            WriteAddr32 = (U32*)Access_Addr;
            break;

        default:
            break;
    }

    HAL_M29_SetReadMemoryArray( WriteParams.MaxAccessWidth, NULL, WriteAddr16, WriteAddr32 );
    return (RetValue);
}



ST_ErrorCode_t HAL_M28_Erase( HAL_EraseParams_t EraseParams,
                              U32              Offset,                 /* offset from BaseAddress */
                              U32              NumberToErase )          /* in bytes */
{
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    BOOL DoingErase = TRUE; /* until completion or time-out */

    U8      *TempAddress;   /* for recasting */
    U8      *Access_Addr;   /* BaseAddress + Offset */
    DU32    *EraseAddr32;   /* 32 bit erase access address */
    DU16    *EraseAddr16;   /* 16 bit erase access address */
    DU8     *EraseAddr08;   /*  8 bit erase access address */

    clock_t StartTime;      /* time at which erase started */
    clock_t Curr_Time;      /* current (low-priority) time */

    UNUSED_PARAMETER(NumberToErase);

    TempAddress = (U8*)EraseParams.BaseAddress;     /* recasting */
    Access_Addr = TempAddress + Offset;             /* Erase block base */

    /* form all width pointers, only one will be used */
    EraseAddr32 = (U32*)Access_Addr;
    EraseAddr16 = (U16*)Access_Addr;
    EraseAddr08 = Access_Addr;

    VppEnable( EraseParams.VppAddress );

    /* Maximum access width MUST be used for all erasure, otherwise only
       every second or fourth device will be erased (not very useful)    */
    switch (EraseParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_08_BITS:

            /* Clear Status Register (precautionary) */
            *EraseAddr08 = CLEAR_SR08;              /* "Don't Care" address in Flash */

            /* commence Erase Command Sequence */
            *EraseAddr08 = ERASE_SU08;              /* "Don't Care" address in Flash */
            *EraseAddr08 = ERASE_CR08;              /* Confirm at Flash block base   */

            StartTime = time_now();                 /* ticks at Erase start */

            /* commence Read Status Register Command Sequence */
            *EraseAddr08 = READ_SR08;               /* "Don't Care" address in Flash */

            /* poll for completion of Erasure on the chip */
            while (DoingErase)
            {
                if ((*EraseAddr08 & COM_STAT08) == COM_STAT08)
                {
                    DoingErase = FALSE;        /* Erase sequence completed */

                    /* check for Erase sequence failure on the chip */

                    if ((*EraseAddr08 & ERA_STAT08) != 0)
                    {
                        RetValue = STFLASH_ERROR_ERASE;
                    }
                    else
                    {
                        /* check for Vpp below minimum on the chip */
                        if ((*EraseAddr08 & VPP_STAT08) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                }
                else
                {
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time,
                        StartTime ) >
                            (clock_t)ERASE_TIMEOUT)
                    {
                        DoingErase = FALSE;   /* Erase sequence timed out */
                        RetValue = ST_ERROR_TIMEOUT;
                    }
                }
            }

            break;

        case STFLASH_ACCESS_16_BITS:

            /* Clear Status Register (precautionary) */
            *EraseAddr16 = CLEAR_SR16;              /* "Don't Care" address in Flash */

            /* commence Erase Command Sequence */
            *EraseAddr16 = ERASE_SU16;              /* "Don't Care" address in Flash */
            *EraseAddr16 = ERASE_CR16;              /* Confirm at Flash block base   */

            StartTime = time_now();                 /* ticks at Erase start */

            /* commence Read Status Register Command Sequence */
            *EraseAddr16 = READ_SR16;               /* "Don't Care" address in Flash */

            /* poll for completion of Erasure on both chips */
            while (DoingErase)
            {
                if ((*EraseAddr16 & COM_STAT16) == COM_STAT16)
                {
                    DoingErase = FALSE;        /* Erase sequence completed */

                    /* check for Erase sequence failure on either chip */
                    if ((*EraseAddr16 & ERA_STAT16) != 0)
                    {
                        RetValue = STFLASH_ERROR_ERASE;
                    }
                    else
                    {
                        /* check for Vpp below minimum on either chip */
                        if ((*EraseAddr16 & VPP_STAT16) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                }
                else
                {
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time,
                        StartTime ) >
                            (clock_t)ERASE_TIMEOUT)
                    {
                        DoingErase = FALSE;    /* Erase sequence timed out */
                        RetValue = ST_ERROR_TIMEOUT;
                    }
                }
            }

            break;


        case STFLASH_ACCESS_32_BITS:

            /* Clear Status Register (precautionary) */
            *EraseAddr32 = CLEAR_SR32;              /* "Don't Care" address in Flash */

            /* commence Erase Command Sequence */
            *EraseAddr32 = ERASE_SU32;              /* "Don't Care" address in Flash */
            *EraseAddr32 = ERASE_CR32;              /* Confirm at Flash block base   */

            StartTime = time_now();                 /* ticks at Erase start */

            /* commence Read Status Register Command Sequence */
            *EraseAddr32 = READ_SR32;               /* "Don't Care" address in Flash */

            /* poll for completion of Erasure on all chips */
            while (DoingErase)
            {
                if ((*EraseAddr32 & COM_STAT32) == COM_STAT32)
                {
                    DoingErase = FALSE;        /* Erase sequence completed */

                    /* check for Erase sequence failure on any chip */

                    if ((*EraseAddr32 & ERA_STAT32) != 0)
                    {
                        RetValue = STFLASH_ERROR_ERASE;
                    }
                    else
                    {
                        /* check for Vpp below minimum on any chip */
                        if ((*EraseAddr32 & VPP_STAT32) != 0)
                        {
                            RetValue = STFLASH_ERROR_VPP_LOW;
                        }
                    }
                }
                else
                {
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time,
                        StartTime ) >
                            (clock_t)ERASE_TIMEOUT)
                    {
                        DoingErase = FALSE;    /* Erase sequence timed out */
                        RetValue = ST_ERROR_TIMEOUT;
                    }
                }
            }


            break;

        default:
            RetValue = ST_ERROR_BAD_PARAMETER;
            break;
    }

    VppDisable( EraseParams.VppAddress );

    /* restore to ReadArray mode */
    HAL_M28_SetReadMemoryArray( EraseParams.MaxAccessWidth, EraseAddr08, EraseAddr16, EraseAddr32 );


    return (RetValue);
}

ST_ErrorCode_t HAL_M29_Erase( HAL_EraseParams_t EraseParams,
                              U32              Offset,                 /* offset from BaseAddress */
                              U32              NumberToErase )         /* in bytes */
{
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    BOOL EraseLow   = FALSE;
    BOOL EraseHigh  = FALSE;
   
    DU32    *EraseAddr32 = NULL;   /* 32 bit erase access address */
    DU16    *EraseAddr16 = NULL;   /* 16 bit erase access address */
    DU32    *Code1Addr = NULL;
    DU32    *Code2Addr = NULL;
    DU16    *Code1Addr16 = NULL;
    DU16    *Code2Addr16 = NULL;
    U32     Code1Data = 0;
    U32     Code2Data = 0;
    U16     Code1Data16 = 0;
    U16     Code2Data16 = 0;

    clock_t         StartTime;      /* time at which erase started */
    clock_t         Curr_Time;      /* current (low-priority) time */
    const clock_t   ClocksPerSecond = ST_GetClocksPerSecond();
    U32 Data1, Data2;
    
    UNUSED_PARAMETER(NumberToErase);    

    switch (EraseParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_32_BITS:
            /* Setup access pointers */
            Code1Addr   = (U32*)EraseParams.BaseAddress +
            M29W800T_CODED_ADDR1;
            Code2Addr   = (U32*)EraseParams.BaseAddress +
            M29W800T_CODED_ADDR2;
            Code1Data   = M29W800T_CODED_DATA1;
            Code2Data   = M29W800T_CODED_DATA2;
            EraseAddr32 = (U32*)( (U32)EraseParams.BaseAddress + Offset );

            /* Start Erase */
            StartTime = time_now();       /* ticks at Erase start */


            /* Cycle 1 - Coded cycle 1 */
            *Code1Addr   = Code1Data;

            /* Cycle 2 - Coded cycle 2 */
            *Code2Addr   = Code2Data;

            /* Cycle 3 - Block Erase Command */
            *Code1Addr   = M29W800T_BLOCK_ERASE;

            /* Cycle 4 - Coded cycle 1 */
            *Code1Addr   = Code1Data;

            /* Cycle 5 - Coded cycle 2 */
            *Code2Addr   = Code2Data;

            /* Cycle 6 - Erase Confirm */
            *EraseAddr32 = M29W800T_ERASE_CONFIRM;

            /* Read DQ2, DQ5 & DQ6 */
            Data1 = *EraseAddr32 & M29W800T_DQ256_MASK;
            while ((EraseLow  == FALSE) ||
                  (EraseHigh == FALSE))
            {
                /* It typically takes 2 to 3 seconds to erase a block
                   (depending on size) so deschedule to avoid busy wait */
                task_delay( ClocksPerSecond );

                /* Read DQ2, DQ5 & DQ6 on all chips */
                Data2 = *EraseAddr32 & M29W800T_DQ256_MASK;

                /* Are DQ2 and DQ6 toggling? */
                if (Data1 == Data2)
                {
                    EraseLow = EraseHigh = TRUE;    /* Erase was successful */
                }
                /* High or Low DQ5 = 1? */
                else if ((Data2 & M29W800T_DQ5_MASK) != 0)
                {
                    Data1 = Data2;                  /* Store current value */
                    Data2 = *EraseAddr32 & M29W800T_DQ256_MASK;
                    /* Read DQ2, DQ5 & DQ6 */

                    if ((EraseLow == FALSE) &&            /* Low still erasing? */
                        (Data1 & M29W800T_LOW_DQ5_MASK))  /* Low DQ5 = 1? */
                    {
                        EraseLow = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_LOW_DQ26_MASK) !=
                            (Data2 & M29W800T_LOW_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }

                    if ((EraseHigh == FALSE) &&       /* High still erasing? */
                        (Data1 & M29W800T_HIGH_DQ5_MASK))  /* High DQ5 = 1? */
                    {
                        EraseHigh = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_HIGH_DQ26_MASK) !=
                            (Data2 & M29W800T_HIGH_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }
                }
                else
                {
                    /* Erase still on going, have we timed out? */
                    Data1 = Data2;
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time, StartTime ) >
                        (clock_t)ERASE_TIMEOUT)
                    {
                        RetValue = ST_ERROR_TIMEOUT;
                        break;
                    }
                }
            }
            break;

        case STFLASH_ACCESS_16_BITS:
            /* Setup access pointers */
            Code1Addr16 = (U16*)EraseParams.BaseAddress +
            M29W800T_CODED_ADDR1;
            Code2Addr16 = (U16*)EraseParams.BaseAddress +
                          M29W800T_CODED_ADDR2;
            Code1Data16 = M29W800T_CODED_DATA1 & 0xffff;
            Code2Data16 = M29W800T_CODED_DATA2 & 0xffff;
            EraseAddr16 = (U16*)( (U32)EraseParams.BaseAddress + Offset );

            /* Start Erase */
            StartTime = time_now();       /* ticks at Erase start */

            /* Cycle 1 - Coded cycle 1 */
            *Code1Addr16 = Code1Data16;

            /* Cycle 2 - Coded cycle 2 */
            *Code2Addr16 = Code2Data16;

            /* Cycle 3 - Block Erase Command */
            *Code1Addr16 = M29W800T_BLOCK_ERASE & 0xffff;

            /* Cycle 4 - Coded cycle 1 */
            *Code1Addr16 = Code1Data16;

            /* Cycle 5 - Coded cycle 2 */
            *Code2Addr16 = Code2Data16;

            /* Cycle 6 - Erase Confirm */
            *EraseAddr16 = M29W800T_ERASE_CONFIRM & 0xffff;

            /* Read DQ2, DQ5 & DQ6 */
            Data1 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;
            while (EraseLow  == FALSE)
            {
                /* It typically takes 2 to 3 seconds to erase a block
                   (depending on size) so deschedule to avoid busy wait */
                task_delay( ClocksPerSecond );

                /* Read DQ2, DQ5 & DQ6 on all chips */
                Data2 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;

                /* Are DQ2 and DQ6 toggling? */
                if (Data1 == Data2)
                {
                    EraseLow = TRUE;            /* Erase was successful */
                }
                /* High or Low DQ5 = 1? */
                else if ((Data2 & M29W800T_DQ5_MASK) != 0)
                {
                    Data1 = Data2;                  /* Store current value */
                    Data2 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;
                    /* Read DQ2, DQ5 & DQ6 */

                    if (Data1 & M29W800T_LOW_DQ5_MASK)  /* Low DQ5 = 1? */
                    {
                        EraseLow = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_LOW_DQ26_MASK) !=
                            (Data2 & M29W800T_LOW_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }
                }
                else
                {
                    /* Erase still on going, have we timed out? */
                    Data1 = Data2;
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time, StartTime ) >
                       (clock_t)ERASE_TIMEOUT)
                    {
                        RetValue = ST_ERROR_TIMEOUT;
                        break;
                    }
                }
            }
            break;
    }
    
    HAL_M29_SetReadMemoryArray( EraseParams.MaxAccessWidth, NULL, EraseAddr16, EraseAddr32);

    return (RetValue);
}

ST_ErrorCode_t HAL_AT49_Erase( HAL_EraseParams_t EraseParams,
                               U32              Offset,                 /* offset from BaseAddress */
                               U32              NumberToErase )         /* in bytes */
{
    ST_ErrorCode_t  RetValue = ST_NO_ERROR;

    BOOL EraseLow   = FALSE;
    BOOL EraseHigh  = FALSE;

    DU32    *EraseAddr32 = NULL;   /* 32 bit erase access address */
    DU16    *EraseAddr16 = NULL;   /* 16 bit erase access address */
    DU32    *Code1Addr = NULL;
    DU32    *Code2Addr = NULL;
    DU16    *Code1Addr16 = NULL;
    DU16    *Code2Addr16 = NULL;
    U32     Code1Data = 0 ;
    U32     Code2Data = 0;
    U16     Code1Data16 = 0;
    U16     Code2Data16 = 0;

    clock_t         StartTime;      /* time at which erase started */
    clock_t         Curr_Time;      /* current (low-priority) time */
    const clock_t   ClocksPerSecond = ST_GetClocksPerSecond();
    U32 Data1, Data2;

    UNUSED_PARAMETER(NumberToErase);    

    switch (EraseParams.MaxAccessWidth)
    {
        case STFLASH_ACCESS_32_BITS:
            /* Setup access pointers */
            Code1Addr   = (U32*)EraseParams.BaseAddress +
            M29W800T_CODED_ADDR1;
            Code2Addr   = (U32*)EraseParams.BaseAddress +
            M29W800T_CODED_ADDR2;
            Code1Data   = M29W800T_CODED_DATA1;
            Code2Data   = M29W800T_CODED_DATA2;
            EraseAddr32 = (U32*)( (U32)EraseParams.BaseAddress + Offset );

            /* Start Erase */
            StartTime = time_now();       /* ticks at Erase start */

            /* Cycle 1 - Coded cycle 1 */
            *Code1Addr   = Code1Data;

            /* Cycle 2 - Coded cycle 2 */
            *Code2Addr   = Code2Data;

            /* Cycle 3 - Block Erase Command */
            *Code1Addr   = M29W800T_BLOCK_ERASE;

            /* Cycle 4 - Coded cycle 1 */
            *Code1Addr   = Code1Data;

            /* Cycle 5 - Coded cycle 2 */
            *Code2Addr   = Code2Data;

            /* Cycle 6 - Erase Confirm */
            *EraseAddr32 = M29W800T_ERASE_CONFIRM;

            /* Read DQ2, DQ5 & DQ6 */
            Data1 = *EraseAddr32 & M29W800T_DQ256_MASK;
            while ((EraseLow  == FALSE) ||
                  (EraseHigh == FALSE))
            {
                /* It typically takes 2 to 3 seconds to erase a block
                   (depending on size) so deschedule to avoid busy wait */
                task_delay( ClocksPerSecond );

                /* Read DQ2, DQ5 & DQ6 on all chips */
                Data2 = *EraseAddr32 & M29W800T_DQ256_MASK;

                /* Are DQ2 and DQ6 toggling? */
                if (Data1 == Data2)
                {
                    EraseLow = EraseHigh = TRUE;    /* Erase was successful */
                }
                /* High or Low DQ5 = 1? */
                else if ((Data2 & M29W800T_DQ5_MASK) != 0)
                {
                    Data1 = Data2;                  /* Store current value */
                    Data2 = *EraseAddr32 & M29W800T_DQ256_MASK;
                    /* Read DQ2, DQ5 & DQ6 */

                    if ((EraseLow == FALSE) &&            /* Low still erasing? */
                        (Data1 & M29W800T_LOW_DQ5_MASK))  /* Low DQ5 = 1? */
                    {
                        EraseLow = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_LOW_DQ26_MASK) !=
                            (Data2 & M29W800T_LOW_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }

                    if ((EraseHigh == FALSE) &&       /* High still erasing? */
                        (Data1 & M29W800T_HIGH_DQ5_MASK))  /* High DQ5 = 1? */
                    {
                        EraseHigh = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_HIGH_DQ26_MASK) !=
                            (Data2 & M29W800T_HIGH_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }
                }
                else
                {
                    /* Erase still on going, have we timed out? */
                    Data1 = Data2;
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time, StartTime ) >
                        (clock_t)ERASE_TIMEOUT)
                    {
                        RetValue = ST_ERROR_TIMEOUT;
                        break;
                    }
                }
            }
            break;

        case STFLASH_ACCESS_16_BITS:
            /* Setup access pointers */
            Code1Addr16 = (U16*)EraseParams.BaseAddress +
                          M29DW640D_CODED_ADDR1;
            Code2Addr16 = (U16*)EraseParams.BaseAddress +
                          M29DW640D_CODED_ADDR2;
            Code1Data16 = M29DW640D_CODED_DATA1 & 0xffff;
            Code2Data16 = M29DW640D_CODED_DATA2 & 0xffff;

            EraseAddr16 = (U16*)( (U32)EraseParams.BaseAddress + Offset );

            /* Start Erase */
            StartTime = time_now();       /* ticks at Erase start */

            /* Cycle 1 - Coded cycle 1 */
            *Code1Addr16 = Code1Data16;

            /* Cycle 2 - Coded cycle 2 */
            *Code2Addr16 = Code2Data16;

            /* Cycle 3 - Block Erase Command */
            *Code1Addr16 = M29DW640D_BLOCK_ERASE & 0xffff;

            /* Cycle 4 - Coded cycle 1 */
            *Code1Addr16 = Code1Data16;

            /* Cycle 5 - Coded cycle 2 */
            *Code2Addr16 = Code2Data16;

            /* Cycle 6 - Erase Confirm */
            *EraseAddr16 = M29DW640D_ERASE_CONFIRM & 0xffff;

            /* Read DQ2, DQ5 & DQ6 */
            Data1 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;
            while (EraseLow  == FALSE)
            {
                /* It typically takes 2 to 3 seconds to erase a block
                   (depending on size) so deschedule to avoid busy wait */
                task_delay( ClocksPerSecond );

                /* Read DQ2, DQ5 & DQ6 on all chips */
                Data2 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;

                /* Are DQ2 and DQ6 toggling? */
                if (Data1 == Data2)
                {
                    EraseLow = TRUE;            /* Erase was successful */
                }
                /* High or Low DQ5 = 1? */
                else if ((Data2 & M29W800T_DQ5_MASK) != 0)
                {
                    Data1 = Data2;                  /* Store current value */
                    Data2 = *EraseAddr16 & M29W800T_DQ256_MASK & 0xffff;
                    /* Read DQ2, DQ5 & DQ6 */

                    if (Data1 & M29W800T_LOW_DQ5_MASK)  /* Low DQ5 = 1? */
                    {
                        EraseLow = TRUE;
                        /* Test for error condition */
                        if ((Data1 & M29W800T_LOW_DQ26_MASK) !=
                            (Data2 & M29W800T_LOW_DQ26_MASK))
                        {
                            RetValue = STFLASH_ERROR_ERASE;
                            break;
                        }
                    }
                }
                else
                {
                    /* Erase still on going, have we timed out? */
                    Data1 = Data2;
                    Curr_Time = time_now();
                    if (time_minus( Curr_Time, StartTime ) >
                       (clock_t)ERASE_TIMEOUT)
                    {
                        RetValue = ST_ERROR_TIMEOUT;
                        break;
                    }
                }
            }
            break;
    }
    HAL_M28_SetReadMemoryArray( EraseParams.MaxAccessWidth, NULL, EraseAddr16, EraseAddr32);

    return (RetValue);
}

/****************************************************************************
Name         : VppEnable()

Description  : Enables the Flash Bank associated with the
               supplied VppAddress ready for Program or
               Erase sequences, and performs the necessary
               settling delay to allow the voltage to
               stabilise.

Parameters   : U32* Hardware address

Return Value : void

See Also     : STFLASH_InitParams_t
 ****************************************************************************/

static void VppEnable( U32 *VppAddress )
{
    DU32 *VppAccess;

    VppAccess = VppAddress;
    if (VppAccess != NULL)
    {
        *VppAccess = VPP_HIGH;
        task_delay( VPP_DELAY );  /* allow Vpp to stabilise */
    }
}

/****************************************************************************
Name         : VppDisable()

Description  : Disables the specified Flash Bank following
               Program or Erase operations.

Parameters   : U32* Hardware address

Return Value : void

See Also     : STFLASH_InitParams_t
 ****************************************************************************/

static void VppDisable( U32* VppAddress )
{
    
#if !defined (ST_5528) && !defined(ST_5100) && !defined(ST_7710) && !defined(ST_5105) && \
    !defined(ST_7100) && !defined(ST_5301) && !defined(ST_8010) && !defined(ST_7109)  && \
    !defined(ST_5525) && !defined(ST_5107) && !defined(ST_7200)
    
    DU32 *VppAccess;

    VppAccess = VppAddress;

    if (VppAccess != NULL)
    {
        *VppAccess = VPP_LOW;
    }
#else
    UNUSED_PARAMETER(VppAddress);
#endif   

}


void HAL_EnterCriticalSection(void) 
{     
    task_lock();
#ifdef ST_OS21
    oldPriority = interrupt_mask_all();
#else	    
    interrupt_lock();
#endif

}

void HAL_LeaveCriticalSection(void)
{	  
#ifdef ST_OS21
    interrupt_unmask(oldPriority);
#else
    interrupt_unlock();
#endif
    task_unlock();
}

#if defined(STFLASH_SPI_SUPPORT)

ST_ErrorCode_t HAL_SPI_Write( U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten )
                              
{
   
    ST_ErrorCode_t  Error = ST_NO_ERROR; 
    U16 ReadBuf[1] = { 0 };	
    U16 WriteBuf[PAGE_SIZE];	
    U32 BytesToWrite = 0,NumWritten = 0,Offset1 = 0;
    U32 ActLen;
    U32 BytesWritten = 0;
    U32 Timeout = 0;
    U32 i=0;
    U32  Access_Addr;       /* BaseAddress + Offset */    
    stflash_Inst_t  *ThisElem;
    
    ThisElem = (stflash_Inst_t *)Handle;
    
    Access_Addr = (U32)((U32)(ThisElem->BaseAddress) + (U32)(Offset));   /* user offset */
  
     
    while ( NumberToWrite > 0)
    {      

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
       
        /* set chip select LOW for WREN cmd */       
        Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
#else	
        *SlaveSelect = CHIP_SELECT_LOW;
#endif  

        WriteBuf[0] = (U8)WRITE_ENABLE_CMD;

        Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 1000, &NumWritten );
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
            
        /* set chip select HIGH and then LOW */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
        
        Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
#else	
        *SlaveSelect = CHIP_SELECT_HIGH;
        *SlaveSelect = CHIP_SELECT_LOW;
#endif   
          
        WriteBuf[0] = (U8)PAGE_PROGRAM_CMD;
        WriteBuf[1] = (U8)(Access_Addr >> 16);
        WriteBuf[2] = (U8)(Access_Addr >> 8);
        WriteBuf[3] = (U8)(Access_Addr);

        Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 4, 1000, &NumWritten );
        if ((Error != ST_NO_ERROR) || (NumWritten != 4))
        {
            return(Error);
        }
        	
        Offset1 = Access_Addr & PAGE_MASK;

        if ( NumberToWrite > PAGE_SIZE)
        {
            BytesToWrite = PAGE_SIZE;
            if ((Offset1 + BytesToWrite) > PAGE_SIZE)
            {
                BytesToWrite -= Offset1;
            }    
            
        }
        else
        {

            if ((Offset1 + NumberToWrite) > PAGE_SIZE)
            {
                BytesToWrite = NumberToWrite - Offset1;
            }
            else
            {
                BytesToWrite = NumberToWrite;
            }

        }
        
        /* copy buffer */
        for ( i = 0; i <BytesToWrite; i++ )
        {
            WriteBuf[i] = (U16)*Buffer;
            Buffer++;
        } 
        
        Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, BytesToWrite, PAGE_WRITE_TIMEOUT, &NumWritten );
        if ((Error != ST_NO_ERROR) || (NumWritten != BytesToWrite))
        {                
            return(Error);
        } 
                          
        /* Drive CS HIGH to complete PP instruction */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR )
        {
            return(Error);
        }
#else	
        *SlaveSelect = CHIP_SELECT_HIGH;
#endif 

        /* set chip select LOW for reading status reg */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
       
        Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
#else	
        *SlaveSelect = CHIP_SELECT_LOW;
#endif    

        WriteBuf[0] = (U8)READ_STATUS_CMD;
        Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
        
        Timeout = 0;
        
        /* Check for WIP bit */
        do
        {
            /* No errors during Read Status Reg comm, so start reading the status */
            Error = STSPI_Read ( ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);
            if ((Error != ST_NO_ERROR)|| (ActLen != 1))
            {
                return(Error);  
            }
    
            if ( Timeout++ == REACHED_TIMEOUT )
            {
                return(ST_ERROR_TIMEOUT);
            }
        }
        while (ReadBuf[0] & 0x01);    
        
        
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        /* set chip select HIGH to finish read status reg */ 
        Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
    	    return(Error);
        }   
#else	
        *SlaveSelect = CHIP_SELECT_HIGH;
#endif
   
        /* update values for next iteration */
        NumberToWrite   -= NumWritten;
        Access_Addr     += NumWritten;
        BytesWritten    += NumWritten;           
          
    }       
    
    *NumberActuallyWritten = BytesWritten;  
                       
    return(Error);   
    
} /* HAL_SPI_Write */

ST_ErrorCode_t HAL_SPI_Erase( U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U32              NumberToErase )
{                       
    
    U32     NumWritten;
    U16     WriteBuf[4];
    ST_ErrorCode_t Error = ST_NO_ERROR;
    stflash_Inst_t  *ThisElem; 
    U32     Access_Addr;
    U16     ReadBuf[1] = { 0xFF };
    U32     ActLen;
    U32     Timeout = 0;
    
    UNUSED_PARAMETER(NumberToErase);    
    ThisElem = (stflash_Inst_t *)Handle;   
     
     Access_Addr = (U32)((U32)(ThisElem->BaseAddress) + (U32)(Offset));   /* user offset */
     
    /* set CS HIGH then LOW */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
    Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0] = (U8)WRITE_ENABLE_CMD;

    /* Enter Write Enable cmd */
    Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 1000, &NumWritten );
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
    
    /* finsih WREN cmd */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
    Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif    

    WriteBuf[0] = (U8)SECTOR_ERASE_CMD;
    WriteBuf[1] = (U8)(Access_Addr >> 16);
    WriteBuf[2] = (U8)(Access_Addr >> 8);
    WriteBuf[3] = (U8)(Access_Addr);

    Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 4, 100, &NumWritten );
    if ((Error != ST_NO_ERROR) ||  (NumWritten != 4))
    {
       return(Error);

    }

    /* should check for WIP bit ?? */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
#endif

        /* set chip select LOW for reading status reg */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
       
        Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
#else	
        *SlaveSelect = CHIP_SELECT_LOW;
#endif    

        WriteBuf[0] = (U8)READ_STATUS_CMD;
        Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
        if (Error != ST_NO_ERROR)
        {
            return(Error);
        }
        
        /* Check for WIP bit */
        do
        {
            /* No errors during Read Status Reg comm, so start reading the status */
            Error = STSPI_Read ( ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);
            if ((Error != ST_NO_ERROR)|| (ActLen != 1))
            {
                return(Error);  
            }
    
            if ( Timeout++ == REACHED_TIMEOUT )
            {
                return(ST_ERROR_TIMEOUT);
            }
        }
        while (ReadBuf[0] & 0x01);    
        
        
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        /* set chip select HIGH to finish read status reg */ 
        Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
        if (Error != ST_NO_ERROR)
        {
    	    return(Error);
        }   
#else	
        *SlaveSelect = CHIP_SELECT_HIGH;
#endif

    return(Error);

} /* HAL_SPI_Erase */


ST_ErrorCode_t HAL_SPI_Read(  U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToRead,           /* in bytes */
                              U32              *NumberActuallyRead )
{

    U32  NumWritten;
    U16  *WriteBuf;
    U32 i = 0;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Access_Addr;       /* BaseAddress + Offset */   
    stflash_Inst_t  *ThisElem;
    
    ThisElem = (stflash_Inst_t *)Handle;
    Access_Addr = (U32)((U32)(ThisElem->BaseAddress) + (U32)(Offset));   /* user offset */          
 
    /* allocate memory for temp buffer */
    WriteBuf = (U16*)STOS_MemoryAllocate(  ThisElem->DriverPartition, 2*NumberToRead ); /* since WriteBuf is U16 */
    if (WriteBuf == NULL)
    {
    	return(ST_ERROR_NO_MEMORY);
    }
    
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
    Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

   if(ThisElem->IsFastRead == FALSE)
   {
	    WriteBuf[0] = (U8)DATA_READ_CMD;
	    WriteBuf[1] = (U8)(Access_Addr >> 16);
	    WriteBuf[2] = (U8)(Access_Addr >> 8);
	    WriteBuf[3] = (U8)(Access_Addr);
	
	    Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 4, 1000, &NumWritten );
	    if ((Error != ST_NO_ERROR) || (NumWritten != 4))
	    {
	        return(Error);
	    }
	   
	    Error = STSPI_Read( ThisElem->SPIHandle, WriteBuf, NumberToRead, MEMORY_READ_TIMEOUT, NumberActuallyRead );
	    if ((Error != ST_NO_ERROR) || (*NumberActuallyRead != NumberToRead))
	    {
	        return(Error);
	    }
    }
    else
    {
    	Access_Addr = (U32)((U32)Access_Addr + (U32)SPI_BASE_ADDRESS);
      	for ( i = 0;i < NumberToRead; i++)
    	{
    		WriteBuf[i] = *(U16*)Access_Addr++;
    		
        }
        *NumberActuallyRead = NumberToRead;
     }	
 
   /* convertinf U16 to U8 */
    for ( i = 0;i < *NumberActuallyRead; i++)
    {
    	Buffer[i] = WriteBuf[i];
    }   

    
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
#endif   
    
    /* deallocate memory for temp buffer */
    STOS_MemoryDeallocate(  ThisElem->DriverPartition,WriteBuf);  
    
    return(Error);

} /* HAL_SPI_Read */

ST_ErrorCode_t HAL_SPI_Unlock( U32              Handle,
                               U32              Offset )
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 WriteBuf[1];
    U16 ReadBuf[1] = { 0xFF };
    U32 ActLen;
    U32 Timeout = 0;
    stflash_Inst_t  *ThisElem;    

    ThisElem = (stflash_Inst_t *)Handle;    
    UNUSED_PARAMETER(Offset);
    
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
#endif

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
#else
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)READ_STATUS_CMD;

    Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
    if ((Error != ST_NO_ERROR) || (ActLen != 1))
    {
        return(Error);
    }
    else /* Read the status now */
    {
        /* No errors during Read Status Reg comm, so start reading the status */
        Error = STSPI_Read (ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);        

        /* Check for block protection bits BP1 & BP0 */
        if ((ReadBuf[0] & 0x08) && (ReadBuf[0] & 0x04))
        {
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
             
             WriteBuf[0]  = (U8)WRITE_ENABLE_CMD;
             Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1 , 100, &ActLen );
             
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
             
            WriteBuf[0]    = (U8)BLOCK_UNLOCK_CMD;
            WriteBuf[1]    = (U8)0x00; /* BR1 =0 BP0 =0 to unlock */

            Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 2 , 100, &ActLen );
            if ((Error != ST_NO_ERROR) || (ActLen != 2))
            {
             	 return(Error);
            }
             
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif  


            /* set chip select LOW for reading status reg for WIP bit */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
                return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_LOW;
#endif    

            WriteBuf[0] = (U8)READ_STATUS_CMD;
            Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
            if (Error != ST_NO_ERROR)
            {
                return(Error);
            }
        
            /* Check for WIP bit */
            do
            {
                /* No errors during Read Status Reg comm, so start reading the status */
                Error = STSPI_Read ( ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);
                if ((Error != ST_NO_ERROR)|| (ActLen != 1))
                {
                    return(Error);  
                }
    
                if ( Timeout++ == REACHED_TIMEOUT )
                {
                    return(ST_ERROR_TIMEOUT);
                }
            }
            while (ReadBuf[0] & 0x01);    
        
        
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            /* set chip select HIGH to finish read status reg */ 
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }   
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif          
        }
       
    }

    return(Error);
    
} /* HAL_SPI_Unlock */

ST_ErrorCode_t HAL_SPI_Lock( U32              Handle,
                             U32              Offset )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 WriteBuf[1];
    U16 ReadBuf[1] = { 0xFF };
    U32 ActLen;
    U32 Timeout = 0;
    stflash_Inst_t  *ThisElem;

    ThisElem = (stflash_Inst_t *)Handle;    
    UNUSED_PARAMETER(Offset);    
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }
#else	
    *SlaveSelect = CHIP_SELECT_HIGH;
#endif

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
    STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
#else
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)READ_STATUS_CMD;

    Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
    if ((Error != ST_NO_ERROR) || (ActLen != 1))
    {
        return(Error);
    }
    else /* Read the status now */
    {
        /* No errors during Read Status Reg comm, so start reading the status */
        Error = STSPI_Read (ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);        

        /* Check for block protection bits BP1 & BP0 */
        if ((ReadBuf[0] & 0x08) && (ReadBuf[0] & 0x04))
        {
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
             
             WriteBuf[0]  = (U8)WRITE_ENABLE_CMD;
             Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1 , 100, &ActLen );
             
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
             
            WriteBuf[0]    = (U8)BLOCK_LOCK_CMD; /* Write Status register cmd actually */
            WriteBuf[1]    = (U8)0x1C; /* BR2 =1, BR1= 1 BP0 =1 to completely lock flash */

            Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 2 , 100, &ActLen );
            if ((Error != ST_NO_ERROR) || (ActLen != 2))
            {
             	 return(Error);
            }
             
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif   
            
            /* set chip select LOW for reading status reg for WIP bit */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
        
            Error = STPIO_Clear(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
                return(Error);
            }
#else	
            *SlaveSelect = CHIP_SELECT_LOW;
#endif    

            WriteBuf[0] = (U8)READ_STATUS_CMD;
            Error = STSPI_Write( ThisElem->SPIHandle, WriteBuf, 1, 100, &ActLen );
            if (Error != ST_NO_ERROR)
            {
                return(Error);
            }
        
            /* Check for WIP bit */
            do
            {
                /* No errors during Read Status Reg comm, so start reading the status */
                Error = STSPI_Read ( ThisElem->SPIHandle, ReadBuf, 1, 100, &ActLen);
                if ((Error != ST_NO_ERROR)|| (ActLen != 1))
                {
                    return(Error);  
                }
    
                if ( Timeout++ == REACHED_TIMEOUT )
                {
                    return(ST_ERROR_TIMEOUT);
                }
            }
            while (ReadBuf[0] & 0x01);    
        
        
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
            /* set chip select HIGH to finish read status reg */ 
            Error = STPIO_Set(ThisElem->CSHandle,ThisElem->SPICSBit);
            if (Error != ST_NO_ERROR)
            {
    	        return(Error);
            }   
#else	
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif    
         
        }
       
    }
    return(Error);
    
} /* HAL_SPI_Lock */

void HAL_SPI_ClkDiv ( U8  Divisor )
{
    *(U32*)(SPI_CLOCK_DIV_REG) = Divisor & 0xFE; /* even clk always */
    
} /* HAL_SPI_ClkDiv */

void HAL_SPI_Config( U8 PartNum, U32 CSHighTime, U32 CSHoldTime, U32 DataHoldTime )
{
    U32 Config = 0;
        
    Config = Config | PartNum;
    Config = Config | (CSHighTime << 4);
    Config = Config | (CSHoldTime << 16);
    Config = Config | (DataHoldTime << 24);
    
    *(U32*)(SPI_CONFIG_DATA_REG) = Config;
    
} /* HAL_SPI_Config */

void HAL_SPI_ModeSelect(BOOL IsContigMode,BOOL IsFastRead )
{
    U32 Config = 0;

    Config = Config |  IsContigMode;
    Config = Config |  (IsFastRead << 1);

    *(U32*)(SPI_MODE_SELECT_REG) = Config;

} /* HAL_SPI_ModeSelect */
#endif
