/*******************************************************************************

File name   : extvi_pr.c

Description : x source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
04 November 2000   Created                                           BD
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sttbx.h"
#include "stextvin.h"
#include "extvi_reg.h"
#include "extvi_pr.h"
#include "stos.h"

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */
 ST_ErrorCode_t I2C_BlockWrite (STI2C_Handle_t  I2cHandle,U8 *Pfill,S32 Todo);
 ST_ErrorCode_t I2C_BlockRead (STI2C_Handle_t  I2cHandle,U8 *Pfill,S32 Todo,U8 SubAddress);

 /* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : I2C_BlockWrite
Description : Write a block of data at the I2C address
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_XXXInit function
*******************************************************************************/

ST_ErrorCode_t I2C_BlockWrite (STI2C_Handle_t  I2cHandle,U8 *Pfill,S32 Todo)
{
    U32 WriteTimeout = 10;
    U32 NumberWritten;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;


    STI2C_Write(I2cHandle, Pfill, Todo, WriteTimeout, &NumberWritten);

    if ((S32)NumberWritten != Todo)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't write %d I2C bytes"));
        ErrorCode = STEXTVIN_HW_FAILURE;

        return ErrorCode;
	}
    return ErrorCode;
}


/*******************************************************************************
Name        : I2C_BlockRead
Description : Read a block of data at the I2C address
Parameters  : None
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_XXXInit function
*******************************************************************************/

ST_ErrorCode_t I2C_BlockRead (STI2C_Handle_t  I2cHandle,U8 *Pfill,S32 Todo,U8 SubAddress)
{
    U32 WriteTimeout = 10;
    U32 ReadTimeout = 10;
    U32 NumberRead;
    U32 NumberWritten;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;


	/* Write sub-address */
    *Pfill = (U8)SubAddress;
    STI2C_WriteNoStop(I2cHandle, Pfill, 1, WriteTimeout, &NumberWritten);
      if ((S32)NumberWritten != 1)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't write I2C sub-address"));
        ErrorCode = STEXTVIN_HW_FAILURE;

        return ErrorCode;
	}

    /* Read one I2C register */
    STI2C_Read(I2cHandle, Pfill, Todo, ReadTimeout, &NumberRead);
    if ((S32)NumberRead != Todo)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't read I2C byte"));
        ErrorCode = STEXTVIN_HW_FAILURE;
        return ErrorCode;
	}

    return ErrorCode;
}

/*******************************************************************************
Name        : EXTVIN_SAA7114SoftReset
Description : Perform a SAA7114 Soft Reset
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SAA7114SoftReset (STI2C_Handle_t  I2cHandle)
{
    U8 Buf[2] = {0x88,0xD8};
    ST_ErrorCode_t  ErrorCode ;

    /* Software reset */
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],2)) != ST_NO_ERROR )
        return ErrorCode;

    Buf[1] = 0xF8;
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],2);
    return ErrorCode;
}

/*******************************************************************************
Name        : EXTVIN_GetSAA7114Status
Description : Give the VIP status
Parameters  : I2cHandle & Status variable
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_GetSAA7114Status function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_GetSAA7114Status(STI2C_Handle_t I2cHandle,U8* Status_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    ErrorCode = I2C_BlockRead (I2cHandle,Status_p,1,SAA7114_VID_STATUS);
    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_GetSAA7111Status
Description : Give the VIP status
Parameters  : I2cHandle & Status variable
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_GetSAA7111Status function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_GetSAA7111Status(STI2C_Handle_t I2cHandle,U8* Status_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    ErrorCode = I2C_BlockRead (I2cHandle,Status_p,1,SAA7111_STATUS_BYTE);
    return(ErrorCode);
}


/*******************************************************************************
Name        : EXTVIN_GetSTV2310Status
Description : Give the VIP status
Parameters  : I2cHandle & Status variable
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_GetSTV2310Status function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_GetSTV2310Status(STI2C_Handle_t I2cHandle,U8* Status_p)
{
    #define STV2310_DDECSTAT2 0x81
    const char* STDTAB[8] = { "PAL BGDHI", "SECAM", "NTSC M", "PAL M", "PAL N", "NTSC 4.43", "NO STD", "NO STD"};
    U8 Test, InpStd;

    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    ErrorCode = I2C_BlockRead (I2cHandle, Status_p, 1, STV2310_DDECSTAT2);
    if (ErrorCode==ST_NO_ERROR)
    {
        Test = (*Status_p) & 0x08;
        if ( Test == 0x08)
        {
            InpStd=(*Status_p) & 0x07;

            STTBX_Print(("Input standard detected : %s\n", STDTAB[InpStd]));
        }
        else
        {
            STTBX_Print(("Input standard not detected !\n"));
        }
    }
    return(ErrorCode);
}


/*******************************************************************************
Name        : EXTVIN_TDA8752Init()
Description : Fill the TDA8752 registers
Parameters  : I2cHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_TDA8752Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_TDA8752Init(STI2C_Handle_t  I2cHandle)
{
    U8 Buf[EXTVIN_TDA8752REG_MAX], *Pfill;
    S32 Todo = 0;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

	/* Fill register contents */
    Pfill = Buf;
    *Pfill++ = 0x00; *Pfill++ = 0x8F;        /* 00H OFFSETR */
    *Pfill++ = 0x01; *Pfill++ = 0x27;        /* 01H COARSER */
    *Pfill++ = 0x02; *Pfill++ = 0x00;        /* 02H FINER   */
    *Pfill++ = 0x03; *Pfill++ = 0x8F;        /* 03H OFFSETG */
    *Pfill++ = 0x04; *Pfill++ = 0x27;        /* 04H COARSEG */
    *Pfill++ = 0x05; *Pfill++ = 0x00;        /* 05H FINEG   */
    *Pfill++ = 0x06; *Pfill++ = 0x8F;        /* 06H OFFSETB */
    *Pfill++ = 0x07; *Pfill++ = 0x27;        /* 07H COARSEB */
    *Pfill++ = 0x08; *Pfill++ = 0x00;        /* 08H FINEB   */

    Todo = (S32)(Pfill - Buf);
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);

    if (ErrorCode)
    {
        STTBX_Print(("TDA8752 init error : I2C access failed\n"));
    }
    else
    {
        /* Set the default input mode   */
        ErrorCode = EXTVIN_SetAdcInputFormat (I2cHandle, STEXTVIN_HD_1280_720);
        if (ErrorCode)
        {
            STTBX_Print(("TDA8752 Default input format set error\n"));
        }
    }

    return ErrorCode;
}

/*******************************************************************************
Name        : EXTVIN_STV2310Init()
Description : Fill the STV2310 registers
Parameters  : I2cHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_STV2310Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_STV2310Init(STI2C_Handle_t  I2cHandle)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[2];

    /* VBI default mode :                       */
    /* - Enable Close caption data slicing.     */

    Buf[0] = 0x42;   /* Sub-address : VBICONT6  */
    Buf[1] = 0x01;   /* VBICONT6.CCEN           */
    ErrorCode = I2C_BlockWrite (I2cHandle,Buf,2);

    return(ErrorCode);
}


/*******************************************************************************
Name        : EXTVIN_SAA7114Init()
Description : Fill the SAA7114 registers
Parameters  : I2cHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SAA7114Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SAA7114Init(STI2C_Handle_t  I2cHandle)
{
    U8 Buf[EXTVIN_SAA7114REG_MAX], *Pfill;
    S32 Todo = 0;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    Buf[0] = SAA7114_CHIP_VERSION;
    Buf[1] = 0;
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],2)) == ST_NO_ERROR )
    {
        ErrorCode = I2C_BlockRead (I2cHandle,&Buf[0],1,SAA7114_CHIP_VERSION);
        if(Buf[0] == 0)
        {
            ErrorCode = STEXTVIN_HW_FAILURE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NO SAA7114 Chip found"));
        }
    }
    if(ErrorCode != ST_NO_ERROR )
        return(ErrorCode);

        /* Fill register contents */
    Pfill = Buf;
    *Pfill++ = SAA7114_FRONT_END_PART; /* Sub-address */

	/* Front-end part */
    *Pfill++ = 0x08;                /* 01H Horizontal increment delay */
    *Pfill++ = 0xC5;                /* 02H Analog input ctrl 1 : CVBS */
    *Pfill++ = 0x10;                /* 03H Analog input ctrl 2 */
    *Pfill++ = 0x90;                /* 04H Analog input ctrl 3 */
    *Pfill++ = 0x90;                /* 05H Analog input ctrl 4 */

	/* Decoder part */
    *Pfill++ = 0xEB;                /* 06H Horizontal sync start */
    *Pfill++ = 0xE0;                /* 07H Horizontal sync stop */
    *Pfill++ = 0xF8;                /* 08H Sync control */
    *Pfill++ = 0x1B;                /* 09H Luminance control */
    *Pfill++ = 0x80;                /* 0AH Luminance brightness control */
    *Pfill++ = 0x44;                /* 0BH Luminance contrast control */
    *Pfill++ = 0x40;                /* 0CH Luminance contrast control */
    *Pfill++ = 0x0;                 /* 0DH Luminance contrast control */
    *Pfill++ = 0x89;                /* 0EH Chrominance control 1 */
    *Pfill++ = 0x24;                /* 0FH Chrominance gain control */
    *Pfill++ = 0x0E;                /* 10H Chrominance control 2 */
    *Pfill++ = 0x0;                 /* 11H Mode/delay control */
    *Pfill++ = 0xFC;                /* 12H RT signal  control: FID and V123*/
    *Pfill++ = 0x0;                 /* 13H RT/X Pfillort output control */
    *Pfill++ = 0x68;                /* 14H Analog/ADC/compat control */
    *Pfill++ = 0x11;                /* 15H VGATE start, FID change */
    *Pfill++ = 0xFE;                /* 16H VGATE stop */
    *Pfill++ = 0x0;                 /* 17H Misc VGATE msb */
    *Pfill++ = 0x40;                /* 18H Raw data gain control */
    *Pfill++ = 0x80;                /* 19H Raw data offset control */


    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return ErrorCode;

    Pfill = Buf;
    *Pfill++ = SAA7114_TASK_IND_GLOB_SET;       /* Sub-address */

	/* X-Port, I-Port, scaler part */
    *Pfill++ = 0x18;                            /* 80H Global control 1 */

    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return ErrorCode;

    Pfill = Buf;
    *Pfill++ = SAA7114_X_PORT_IO_EN;        /* Sub-address */

	/* Task independent global settings */
    *Pfill++ = 0x00;                        /* 83H X-Port IO enable and output clock phase control */
    *Pfill++ = 0xa5;                        /* 84H I-Port signal definitions */
    *Pfill++ = 0x00;                        /* 85H I-Port signal polarities */
    *Pfill++ = 0x45;                        /* 86H I-Port FIFO flag control */
    *Pfill++ = 0x02;                        /* 87H I-Port I/O enable */
    *Pfill++ = 0xF8;                        /* 88H Power save control */

    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return ErrorCode;

    Pfill = Buf;
    *Pfill++ = SAA7114_TASK_A_DEFINITION;   /* Sub-address */

	/* Task A definition */
    *Pfill++ = 0x00;                        /* 90H Task handling control */
    *Pfill++ = 0x08;                        /* 91H X-Port format and config */
    *Pfill++ = 0x01;                        /* 92H X-Port input ref signal def*/
    *Pfill++ = 0x80;                        /* 93H I-Port format and config */

    /* Input and output windows  525_60HZ */
    *Pfill++ = 0x00;            /* 94H XO Horiz input window start lsb */
    *Pfill++ = 0x00;            /* 95H XO Horiz input window start msb */
    *Pfill++ = 0xD0;            /* 96H XS Horiz input window length lsb */
    *Pfill++ = 0x02;            /* 97H XS Horiz input window length msb */
    *Pfill++ = 0x11;            /* 98H YO Vert input window start lsb */
    *Pfill++ = 0x00;            /* 99H YO Vert input window start msb */
    *Pfill++ = 0xF2;            /* 9AH YS Vert input window length lsb */
    *Pfill++ = 0x00;            /* 9BH YS Vert input window length msb */
    *Pfill++ = 0xD0;            /* 9CH XD Horiz output window length lsb */
    *Pfill++ = 0x02;            /* 9DH XD Horiz output window length msb */
    *Pfill++ = 0xF2;            /* 9EH YD Vert output window length lsb */
    *Pfill++ = 0x00;            /* 9FH YD Vert output window length msb */

	/* FIR filtering and prescaling */
    *Pfill++ = 0x01;                        /* A0H Horiz prescaling */
    *Pfill++ = 0x00;                        /* A1H Accumulation length */
    *Pfill++ = 0x00;                        /* A2H prescaler DCgain and FIR prefilter control */
    *Pfill++ = 0x00;                        /* A3H reserved */
    *Pfill++ = 0x80;                        /* A4H Luminance brightness setting */
    *Pfill++ = 0x40;                        /* A5H Luminance contrast setting */
    *Pfill++ = 0x40;                        /* A6H Chrominance saturation setting */
    *Pfill++ = 0x00;                        /* A7H reserved */
    *Pfill++ = 0x00;                        /* A8H Horiz luminance scaling increment lsb */
    *Pfill++ = 0x04;                        /* A9H Horiz luminance scaling increment msb */
    *Pfill++ = 0x00;                        /* AAH Horiz luminance phase offset */
    *Pfill++ = 0x00;                        /* ABH reserved */
    *Pfill++ = 0x00;                        /* ACH Horiz chrominance scaling increment lsb */
    *Pfill++ = 0x02;                        /* ADH Horiz chrominance scaling increment msb */
    *Pfill++ = 0x00;                        /* AEH Horiz chrominance phase offset */
    *Pfill++ = 0x00;                        /* AFH reserved */
    *Pfill++ = 0x00;                        /* B0H Vert luminance scaling increment lsb */
    *Pfill++ = 0x04;                        /* B1H Vert luminance scaling increment msb */
    *Pfill++ = 0x00;                        /* B2H Vert chrominance scaling increment lsb */
    *Pfill++ = 0x04;                        /* B3H Vert chrominance scaling increment msb */
    *Pfill++ = 0x00;                        /* B4H Vert scaling mode control */
    *Pfill++ = 0x00;                        /* B5H reserved */
    *Pfill++ = 0x00;                        /* B6H reserved */
    *Pfill++ = 0x00;                        /* B7H reserved */
    *Pfill++ = 0x00;                        /* B8H Vert chrominance phase offset 00 */
    *Pfill++ = 0x00;                        /* B9H Vert chrominance phase offset 01 */
    *Pfill++ = 0x00;                        /* BAH Vert chrominance phase offset 10 */
    *Pfill++ = 0x00;                        /* BBH Vert chrominance phase offset 11 */
    *Pfill++ = 0x00;                        /* BCH Vert luminance phase offset 00 */
    *Pfill++ = 0x00;                        /* BDH Vert luminance phase offset 01 */
    *Pfill++ = 0x00;                        /* BEH Vert luminance phase offset 10 */
    *Pfill++ = 0x00;                        /* BFH Vert luminance phase offset 11 */

    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return ErrorCode;

	/* Task B definition */
    Buf[0] = SAA7114_TASK_B_DEFINITION;   /* Just change sub-address, keep same datas */

    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return ErrorCode;

    ErrorCode = EXTVIN_SAA7114SoftReset (I2cHandle);

    return ErrorCode;
}

/*******************************************************************************
Name        : EXTVIN_SAA7111Init()
Description : Fill the SAA7111 registers for DB331 board
Parameters  : I2cHandle
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SAA7114Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SAA7111Init(STI2C_Handle_t  I2cHandle)
{
    U8 Buf[EXTVIN_SAA7111REG_MAX], *Pfill;
    S32 Todo = 0;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    Buf[0] = SAA7111_CHIP_VERSION;
    Buf[1] = 0;
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],2)) == ST_NO_ERROR )
    {
        ErrorCode = I2C_BlockRead (I2cHandle,&Buf[0],1,SAA7111_CHIP_VERSION);
        if(Buf[0] == 0)
        {
            ErrorCode = STEXTVIN_HW_FAILURE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "NO DB331 daughter board found"));
        }
    }
    if(ErrorCode != ST_NO_ERROR )
        return(ErrorCode);

        /* Fill register contents */
    Todo = 0;
    Pfill = Buf;
    *Pfill++ = 0x01; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x0F; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x14; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x18; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x19; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x1D; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    *Pfill++ = 0x1E; *Pfill++ = 0x00;
    if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
        return(ErrorCode);
    Pfill = Buf;
    *Pfill++ = SAA7111_ANALOG_INPUT_CONTR_1; *Pfill++ = SAA7111_ANF_FILT | SAA7111_COMP_21;
    *Pfill++ = 0x00; /*  SAA7111_ANALOG_INPUT_CONTR_2 */
    *Pfill++ = 0x00; /*  SAA7111_ANALOG_INPUT_CONTR_3 */
    *Pfill++ = 0x00; /*  SAA7111_ANALOG_INPUT_CONTR_4 */
    *Pfill++ = 0xD9; /*  SAA7111_HORIZONTAL_SYNC_START */
    *Pfill++ = 0xFB; /*  SAA7111_HORIZONTAL_SYNC_STOP */
    *Pfill++ = SAA7111_VTRC | SAA7111_AUFD; /* SAA7111_SYNC_CONTROL */
    *Pfill++ = SAA7111_BPSS_38;             /* SAA7111_LUMINANCE_CONTROL */
    *Pfill++ = 0x80;                        /* SAA7111_LUMINANCE_BRIGHTNESS */
    *Pfill++ = 0x47;                        /* SAA7111_LUMINANCE_CONTRAST */
    *Pfill++ = 0x40;                        /* SAA7111_CHROMA_SATURATION */
    *Pfill++ = 0x0;                         /* SAA7111_CHROMA_HUE_CONTROL */
    *Pfill++ = SAA7111_CHBW_NOMIN;          /* SAA7111_CHROMA_CONTROL */
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return(ErrorCode);

    Pfill = Buf;
    *Pfill++ = SAA7111_FORMAT_DELAY_CONTROL; *Pfill++ = SAA7111_OF_CCIR656 | SAA7111_HDEL;
    *Pfill++ = SAA7111_OEHV | SAA7111_OEYC | SAA7111_GPWS; /* SAA7111_OUTPUT_CONTROL_1 */
    *Pfill++ = 0x0;                                        /* SAA7111_OUTPUT_CONTROL_2 */
    *Pfill++ = 0x0;                                        /* SAA7111_OUTPUT_CONTROL_3 */
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return(ErrorCode);


    Pfill = Buf;
    *Pfill++ = SAA7111_VGATE1_START; *Pfill++ = 0x00;
    *Pfill++ = 0x00;              /* SAA7111_VGATE1_STOP */
    *Pfill++ = 0x00;              /* SAA7111_VGATE1_MSB */
    Todo = (S32)(Pfill - Buf);
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);

    return ErrorCode;
}

/*******************************************************************************
Name        : EXTVIN_SelectSTV2310Input()
Description : Fill the STV2310 registers for selecting video input type
Parameters  : I2cHandle & selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SelectSTV2310Input function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SelectSTV2310Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[2];

    switch (Selection)
	  {
        case STEXTVIN_VideoComp:  /* CVBS */
            Buf[0] = 0x00;   /* Sub-address */
            Buf[1] = 0x04;   /* DDECCONT0.SVIDEOSEL */
            ErrorCode = I2C_BlockWrite (I2cHandle,Buf,2);
            break;
        case STEXTVIN_VideoYc:    /* SVHS */
            Buf[0] = 0x00;   /* Sub-address */
            Buf[1] = 0x10;   /* DDECCONT0.SVIDEOSEL */
            ErrorCode = I2C_BlockWrite (I2cHandle,Buf,2);
            break;
        case STEXTVIN_CCIR656:    /* Digital input */
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
            break;
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_SelectSAA7114Input()
Description : Fill the SAA7114 registers for selecting video input type
Parameters  : I2cHandle & selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SAA7114Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SelectSAA7114Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7114REG_MAX], *Pfill;
    S32 Todo = 0;
    BOOL Secam;

    Pfill = Buf;
    if( (ErrorCode = I2C_BlockRead (I2cHandle,&Buf[0],1,SAA7114_CHROMINANCE_CONTROL_1)) != ST_NO_ERROR )
        return(ErrorCode);
    Secam = (*Pfill == 0xD0) ? TRUE : FALSE;
    Pfill = Buf;
    *Pfill++ = SAA7114_SYNC_CONTROL + 1;   /* 0x9 Sub-address */
    if(Secam == TRUE)  /* Init Luminance control for all mode will be forced in CVBS if necessary */
    {
        *Pfill++ = 0x9B;
    }
    else
    {
        *Pfill++ = 0xC0;
    }
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return(ErrorCode);

          Todo = (S32)(Pfill - Buf);
    Pfill = Buf;
    switch (Selection)
	{
        case STEXTVIN_VideoComp:  /* CVBS */
            *Pfill++ = SAA7114_FRONT_END_PART + 1;   /* Sub-address */
            *Pfill++ =  0xC5;                        /* 02H Analog input ctrl 1 */
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL + 1;   /* 0x9 Sub-address */
            if(Secam == TRUE)  /* Init Luminance control for all mode will be forced in CVBS if necessary */
            {
                *Pfill++ = 0x1B;
            }
            else
            {
                *Pfill++ = 0x40;
            }
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_POW_SAV_CONT;   /* Sub-address 0x88 */
            *Pfill++ = 0xB8;
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_IND_GLOB_SET;     /* Sub-address */
            *Pfill++ = 0x18;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_A_DEFINITION;     /* Sub-address  */
            *Pfill++  = 0x00;                        /* 90H Task handling control */
            *Pfill++ = 0x08;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;
        case STEXTVIN_VideoYc:    /* SVHS */
            *Pfill++ = SAA7114_FRONT_END_PART;   /* Sub-address */
            *Pfill++ = 0x08;                     /* 01H Horizontal increment delay */
            *Pfill++ =  0xC8;                    /* 02H Analog input ctrl 1 */
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_POW_SAV_CONT;   /* Sub-address */
            *Pfill++ = 0xF8;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_IND_GLOB_SET;     /* Sub-address */
            *Pfill++ = 0x18;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_A_DEFINITION;     /* Sub-address  */
            *Pfill++  = 0x00;                        /* 90H Task handling control */
            *Pfill++ = 0x08;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_CCIR656:    /* Digital input */
            *Pfill++ = SAA7114_FRONT_END_PART;   /* Sub-address */
            *Pfill++ = 0x08;                     /* 01H Horizontal increment delay */
            *Pfill++ =  0xC8;                    /* 02H Analog input ctrl 1 */
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_POW_SAV_CONT;     /* Sub-address */
            *Pfill++ = 0x38;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_IND_GLOB_SET;     /* Sub-address */
            *Pfill++ = 0x19;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;

            Pfill = Buf;
            *Pfill++ = SAA7114_TASK_A_DEFINITION;     /* Sub-address  */
            *Pfill++  = 0x00;                        /* 90H Task handling control */
            *Pfill++ = 0x18;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
			break;
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
            break;
    }
    if( ErrorCode  == ST_NO_ERROR  )
        return(ErrorCode);
    ErrorCode = EXTVIN_SAA7114SoftReset (I2cHandle);
    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_SelectSAA7111Input()
Description : Fill the SAA7111 registers for selecting video input type
Parameters  : I2cHandle & selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SAA7114Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SelectSAA7111Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7111REG_MAX], *Pfill;
    S32 Todo = 0;

    Pfill = Buf;
    Todo = 0;
     switch(Selection)
		{

        case STEXTVIN_VideoComp:  /* CVBS */
        *Pfill++ = SAA7111_ANALOG_INPUT_CONTR_1;
        *Pfill++ = SAA7111_ANF_FILT | SAA7111_COMP_22;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_LUMINANCE_CONTROL;
        *Pfill++ = SAA7111_BPSS_38;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_OUTPUT_CONTROL_1;
        *Pfill++ = SAA7111_OEHV | SAA7111_OEYC | SAA7111_GPWS;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
        break;

        case STEXTVIN_VideoYc:    /* SVHS */
        *Pfill++ = SAA7111_LUMINANCE_CONTROL;
        *Pfill++ = SAA7111_BPSS_38 | SAA7111_BYPS;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_LUMINANCE_CONTROL;
        *Pfill++ = SAA7111_BPSS_38;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_OUTPUT_CONTROL_1;
        *Pfill++ = SAA7111_OEHV | SAA7111_OEYC | SAA7111_GPWS;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
        break;

        case STEXTVIN_CCIR656:    /* Digital input */
        *Pfill++ = SAA7111_ANALOG_INPUT_CONTR_1;
        *Pfill++ = SAA7111_OEHV | SAA7111_OEYC;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
        break;
        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
        break;
        }
    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_SelectVipOutput()
Description : Fill the SAA7114 registers for selecting video output type
Parameters  : I2cHandle & selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SelectVipOutput function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SelectVipOutput(STI2C_Handle_t I2cHandle,STEXTVIN_VipOutputSelection_t Selection)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7114REG_MAX], *Pfill;
    S32 Todo = 0;

    Pfill = Buf;
    switch (Selection)
	{
        *Pfill++ = SAA7114_TASK_A_DEFINITION + 3;   /* Sub-address */
        case STEXTVIN_ITU656:
            *Pfill++ =  0x80;                        /* 93H I port format and configuration */
			break;
        case STEXTVIN_NoITU656:
            *Pfill++ =  0x0;                        /* 93H I port format and configuration */
			break;
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIP output type not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_OUTPUT_TYPE;
            break;
        }
    if( ErrorCode != ST_NO_ERROR )
        return(ErrorCode);
    Todo = (S32)(Pfill - Buf);
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
    if( ErrorCode  == ST_NO_ERROR  )
        return(ErrorCode);
    ErrorCode = EXTVIN_SAA7114SoftReset (I2cHandle);

    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_SetSTV2310Standard()
Description : Fill the STV2310 registers for selecting video input standard
Parameters  : I2cHandle & standard
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SetSTV2310Standard function
*******************************************************************************/
ST_ErrorCode_t EXTVIN_SetSTV2310Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[2];

    switch (Standard)
	{
        case STEXTVIN_VIPSTANDARD_PAL_BGDHI:
            Buf[0] = 0x41;   /* Sub-address */
            Buf[1] = 0x00;   /* VBICONT5.NOTPAL = 0 */
                             /* VBICONT5.NOT50  = 0 */
            ErrorCode = I2C_BlockWrite (I2cHandle,Buf,2);
            break;
        case STEXTVIN_VIPSTANDARD_NTSC_M:
            Buf[0] = 0x41;   /* Sub-address */
            Buf[1] = 0x0c;   /* VBICONT5.NOTPAL = 1 */
                             /* VBICONT5.NOT50  = 1 */
            ErrorCode = I2C_BlockWrite (I2cHandle,Buf,2);
            break;
        case STEXTVIN_VIPSTANDARD_PAL4:
        case STEXTVIN_VIPSTANDARD_PAL_M:
        case STEXTVIN_VIPSTANDARD__NTSC4_50:
        case STEXTVIN_VIPSTANDARD_PALN:
        case STEXTVIN_VIPSTANDARD_NTSC4_60:
        case STEXTVIN_VIPSTANDARD_NTSC_N:
        case STEXTVIN_VIPSTANDARD_NTSC_JAPAN:
        case STEXTVIN_VIPSTANDARD_SECAM:
            /* Nothing special to do. */
            break;
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
            break;
    }

    return(ErrorCode);
}


/*******************************************************************************
Name        : EXTVIN_SetSAA7114Standard()
Description : Fill the SAA7114 registers for selecting video input standard
Parameters  : I2cHandle & standard
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SAA7114Init function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SetSAA7114Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7114REG_MAX], *Pfill;
    S32 Todo = 0;
    BOOL Cvbs;
    BOOL Stand625_50Hz = TRUE;

    Pfill = Buf;
    if( (ErrorCode = I2C_BlockRead (I2cHandle,&Buf[0],1,SAA7114_FRONT_END_PART+1)) != ST_NO_ERROR )
        return(ErrorCode);
    Cvbs = (*Pfill == 0xC5) ? TRUE : FALSE;
    Pfill = Buf;
    *Pfill++ = SAA7114_SYNC_CONTROL + 1;   /* 0x9 Sub-address */
    if(Cvbs == TRUE)  /* Init Luminance control for all mode will be forced in SECAM if necessary */
    {
        *Pfill++ = 0x40;
    }
    else
    {
        *Pfill++ = 0xC0;
    }
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return(ErrorCode);

    Pfill = Buf;
    *Pfill++ = SAA7114_CHROMINANCE_CONTROL_1;   /* 0xE Sub-address */
    switch (Standard)
	{
        case STEXTVIN_VIPSTANDARD_PAL_BGDHI:
            *Pfill++ =  0x81;
            *Pfill++ =  0x24;
            *Pfill++ =  0x06;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xB8;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);

            break;
        case STEXTVIN_VIPSTANDARD_NTSC_M:
            *Pfill++ =  0x89;
            *Pfill++ =  0x24;
            *Pfill++ =  0x0E;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xF8;
            Stand625_50Hz = FALSE;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_PAL4:
            *Pfill++ =  0x91;
            *Pfill++ =  0x24;
            *Pfill++ =  0x06;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xF8;
            Stand625_50Hz = FALSE;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD__NTSC4_50:
            *Pfill++ =  0x99;
            *Pfill++ =  0x24;
            *Pfill++ =  0x0E;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xB8;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_PALN:
            *Pfill++ =  0xA1;
            *Pfill++ =  0x24;
            *Pfill++ =  0x06;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xB8;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_NTSC4_60:
            *Pfill++ =  0xA9;
            *Pfill++ =  0x24;
            *Pfill++ =  0x0E;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xF8;
            Stand625_50Hz = FALSE;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_NTSC_N:
            *Pfill++ =  0xB9;
            *Pfill++ =  0x24;
            *Pfill++ =  0x0E;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xB8;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_PAL_M:
            *Pfill++ =  0xB1;
            *Pfill++ =  0x24;
            *Pfill++ =  0x06;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xF8;
            Stand625_50Hz = FALSE;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_NTSC_JAPAN:
            *Pfill++ =  0xC9;
            *Pfill++ =  0x24;
            *Pfill++ =  0x0E;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xF8;
            Stand625_50Hz = FALSE;
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        case STEXTVIN_VIPSTANDARD_SECAM:
            *Pfill++ =  0xD0;
            *Pfill++ =  0x80;
            *Pfill++ =  0x0;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL;   /* 0x8 Sub-address */
            *Pfill++ =  0xB8;
            Todo = (S32)(Pfill - Buf);
            if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
                break;
            Pfill = Buf;
            *Pfill++ = SAA7114_SYNC_CONTROL + 1;   /* 0x9 Sub-address */
            if(Cvbs == TRUE)
            {
               *Pfill++ = 0x1B;
            }
            else
            {
               *Pfill++ = 0x9B;
            }
            Todo = (S32)(Pfill - Buf);
            ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
            break;
    }

    Pfill = Buf;
    *Pfill++ = SAA7114_TASK_A_DEFINITION + 4;   /* Sub-address */
    if(Stand625_50Hz == TRUE)
    {
        /* Input and output windows  625_60HZ */
        *Pfill++ = 0x16;            /* 94H XO Horiz input window start lsb */
        *Pfill++ = 0x00;            /* 95H XO Horiz input window start msb */
        *Pfill++ = 0xd0;            /* 96H XS Horiz input window length lsb */
        *Pfill++ = 0x02;            /* 97H XS Horiz input window length msb */
        *Pfill++ = 0x11;            /* 98H YO Vert input window start lsb */
        *Pfill++ = 0x00;            /* 99H YO Vert input window start msb */
        *Pfill++ = 0x38;            /* 9AH YS Vert input window length lsb */
        *Pfill++ = 0x01;            /* 9BH YS Vert input window length msb */
        *Pfill++ = 0xd0;            /* 9CH XD Horiz output window length lsb */
        *Pfill++ = 0x02;            /* 9DH XD Horiz output window length msb */
        *Pfill++ = 0x38;            /* 9EH YD Vert output window length lsb */
        *Pfill++ = 0x01;            /* 9FH YD Vert output window length msb */
    }
    else
    {
        *Pfill++ = 0x00;            /* 94H XO Horiz input window start lsb */
        *Pfill++ = 0x00;            /* 95H XO Horiz input window start msb */
        *Pfill++ = 0xD0;            /* 96H XS Horiz input window length lsb */
        *Pfill++ = 0x02;            /* 97H XS Horiz input window length msb */
        *Pfill++ = 0x11;            /* 98H YO Vert input window start lsb */
        *Pfill++ = 0x00;            /* 99H YO Vert input window start msb */
        *Pfill++ = 0xf2;            /* 9AH YS Vert input window length lsb */
        *Pfill++ = 0x00;            /* 9BH YS Vert input window length msb */
        *Pfill++ = 0xD0;            /* 9CH XD Horiz output window length lsb */
        *Pfill++ = 0x02;            /* 9DH XD Horiz output window length msb */
        *Pfill++ = 0xf2;            /* 9EH YD Vert output window length lsb */
        *Pfill++ = 0x00;            /* 9FH YD Vert output window length msb */
    }
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle, &Buf[0], Todo)) != ST_NO_ERROR )
        return(ErrorCode);


    Buf[0] = SAA7114_TASK_B_DEFINITION + 4;   /* 0xC4 Sub-address */
    Todo = (S32)(Pfill - Buf);
    if( (ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo)) != ST_NO_ERROR )
        return(ErrorCode);
    ErrorCode = EXTVIN_SAA7114SoftReset (I2cHandle);
    return(ErrorCode);
}


ST_ErrorCode_t EXTVIN_SetSAA7111Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7111REG_MAX], *Pfill;
    S32 Todo = 0;

    Pfill = Buf;

    switch(Standard)
    {
        *Pfill++ = SAA7111_CHROMA_CONTROL;
        case STEXTVIN_VIPSTANDARD_PAL_BGDHI:
        case STEXTVIN_VIPSTANDARD_NTSC_M:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PAL_NTSC;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;

        case STEXTVIN_VIPSTANDARD_PAL4:
        case STEXTVIN_VIPSTANDARD__NTSC4_50:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PAL4_NTSC4;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;

        case STEXTVIN_VIPSTANDARD_PALN:
        case STEXTVIN_VIPSTANDARD_NTSC4_60:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PALN_NTSC4;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;

        case STEXTVIN_VIPSTANDARD_NTSC_N:
        case STEXTVIN_VIPSTANDARD_PAL_M:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PALM_NTSCN;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;
        case STEXTVIN_VIPSTANDARD_SECAM:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PAL4_SECAM;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;

        case STEXTVIN_VIPSTANDARD_NTSC_JAPAN:
        *Pfill++ = SAA7111_CHBW_NOMIN | SAA7111_PAL_NTSC;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_LUMINANCE_BRIGHTNESS;
        *Pfill++ = 0x95;
        if((ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2)) != ST_NO_ERROR )
                break;
        *Pfill++ = SAA7111_LUMINANCE_CONTRAST;
        *Pfill++ = 0x48;
        ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[(S32)2*Todo++],2);
            break;
        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Standard not supported"));
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
        break;
    }
    return(ErrorCode);
}


/*******************************************************************************
Name        : EXTVIN_SetAdcInputFormat()
Description : Fill the TDA8752 registers for selecting video Input format type
Parameters  : I2cHandle & Format
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_SetAdcInputFormat function
*******************************************************************************/

ST_ErrorCode_t EXTVIN_SetAdcInputFormat(STI2C_Handle_t I2cHandle,STEXTVIN_AdcInputFormat_t Format)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_TDA8752REG_MAX], *Pfill;
    S32 Todo = 0;

    Pfill = Buf;
	switch (Format)
	{
        case STEXTVIN_NTSC_640_480:
            *Pfill++ = 0x09; *Pfill++ = 0x47;        /* 09H CONTROL, HS, VS active low*/
            *Pfill++ = 0x0A; *Pfill++ = 0xE1;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x01;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x86;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_NTSC_720_480:
            *Pfill++ = 0x09; *Pfill++ = 0x44;        /* 09H CONTROL, HS, VS active low*/
            *Pfill++ = 0x0A; *Pfill++ = 0x81;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0xAD;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_VGA_640_480:
            *Pfill++ = 0x09; *Pfill++ = 0x45;        /* 09H CONTROL */
            *Pfill++ = 0x0A; *Pfill++ = 0xA9;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x90;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_SVGA_800_600:
            *Pfill++ = 0x09; *Pfill++ = 0x45;        /* 09H CONTROL */
            *Pfill++ = 0x0A; *Pfill++ = 0xB2;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x08;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_XGA_1024_768:
            *Pfill++ = 0x09; *Pfill++ = 0x47;        /* 09H CONTROL */
            *Pfill++ = 0x0A; *Pfill++ = 0xDA;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x90;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_SXGA_1280_1024:    /* <== PLL doesn't lock... */
            *Pfill++ = 0x09; *Pfill++ = 0x06;        /* 09H CONTROL, HS, VS active high */
            *Pfill++ = 0x0A; *Pfill++ = 0xBB;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x4C;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_SUN_1152_900:
            *Pfill++ = 0x09; *Pfill++ = 0x27;        /* 09H CONTROL */
            *Pfill++ = 0x0A; *Pfill++ = 0xFA;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0xEE;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_HD_1920_1080:       /* <== Updated, unvalidated Mode */
            *Pfill++ = 0x09; *Pfill++ = 0x45;        /* 09H CONTROL, HS, VS active low*/
            *Pfill++ = 0x0A; *Pfill++ = 0xBC;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x4C;    /* 0BH DIVIDER */
			break;
        case STEXTVIN_HD_1280_720:      /* <== Validated Mode */
            *Pfill++ = 0x09; *Pfill++ = 0x45;        /* 09H CONTROL HS active low */
            *Pfill++ = 0x0A; *Pfill++ = 0xBB;        /* 0AH VCO     */
            *Pfill++ = 0x0C; *Pfill++ = 0x00;    /* 0CH PHASE A */
            *Pfill++ = 0x0D; *Pfill++ = 0x00;    /* 0DH PHASE B */
            *Pfill++ = 0x0B; *Pfill++ = 0x39;    /* 0BH DIVIDER */
			break;
        default :
            STTBX_Print(("ADC input FORMAT not supported\n"));
            ErrorCode = STEXTVIN_ERROR_INVALID_OUTPUT_TYPE;
            break;
        }
    if( ErrorCode != ST_NO_ERROR )
        return(ErrorCode);
    Todo = (S32)(Pfill - Buf);
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
    return(ErrorCode);
}

/*------------------------------------------------------------------------------
 Write one SAA7114 I2C register
 Param:
 Return:		FALSE if no error occured
------------------------------------------------------------------------------*/
ST_ErrorCode_t EXTVIN_WriteI2C(STI2C_Handle_t I2cHandle, U8 Address, U8 Value)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    U8 Buf[EXTVIN_SAA7114REG_MAX], *Pfill;
    S32 Todo = 0;

    Pfill = Buf;

	/* Write sub-address */
    *Pfill++ = Address;
    /* Write data        */
    *Pfill++ = Value;

    Todo = (S32)(Pfill - Buf);
    ErrorCode = I2C_BlockWrite (I2cHandle,&Buf[0],Todo);
    if (ErrorCode != ST_NO_ERROR)
    {
		STTBX_Print(("Couldn't write 2 I2C bytes\n"));
    }
    /* Normal termination.  */
    return(ErrorCode);
}

/*******************************************************************************
Name        : EXTVIN_ReadI2C
Description : Read a register value at the I2C address
Parameters  : ...
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API EXTVIN_XXXInit function
*******************************************************************************/
ST_ErrorCode_t EXTVIN_ReadI2C (STI2C_Handle_t  I2cHandle, U8 SubAddress, U8 *Value_p)
{
    U32 WriteTimeout = 10;
    U32 ReadTimeout = 10;
    U32 NumberRead;
    U32 NumberWritten;
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;


	/* Write sub-address */
    *Value_p = (U8)SubAddress;
    STI2C_WriteNoStop(I2cHandle, Value_p, 1, WriteTimeout, &NumberWritten);
      if (NumberWritten != 1)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't write I2C sub-address"));
        ErrorCode = STEXTVIN_HW_FAILURE;
        return ErrorCode;
	}

    /* Read one I2C register */
    STI2C_Read(I2cHandle, Value_p, 1, ReadTimeout, &NumberRead);
    if (NumberRead != 1)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't read I2C byte"));
        ErrorCode = STEXTVIN_HW_FAILURE;
        return ErrorCode;
	}

    return ErrorCode;
}

/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of extvi_pr.c */
