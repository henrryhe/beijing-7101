/*******************************************************************************

File name   : evin_cmd.c

Description : EXTVIN module commands

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
28 Nov 2000        Created                                           BD
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include "stextvin.h"
#include "evin.h"
#include "extvi_pr.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */
#define CAST_CONST_HANDLE_TO_DATA_TYPE( DataType, Handle )  \
            ((DataType)(*(DataType*)((void*)&(Handle))))

/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STEXTVIN_SetAdcInputFormat
Description : Select Video Input Format for the ADC
Parameters  : Handle & Standard
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API SetAdcInputFormat function
*******************************************************************************/

ST_ErrorCode_t STEXTVIN_SetAdcInputFormat(
    const STEXTVIN_Handle_t Handle,
    const STEXTVIN_AdcInputFormat_t Format
)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_TDA8752 :
            ErrorCode = EXTVIN_SetAdcInputFormat(I2cHandle_TDA8752,Format);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
                break;
        }
    }
    return(ErrorCode);
 }


/*******************************************************************************
Name        : STEXTVIN_SetVipStandard
Description : Select Video Input Standard
Parameters  :  Handle & Standard
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API SetVipStandard function
*******************************************************************************/

 ST_ErrorCode_t STEXTVIN_SetVipStandard(
    const STEXTVIN_Handle_t      Handle,
    const STEXTVIN_VipStandard_t Standard
    )
 {
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
            ErrorCode = EXTVIN_SetSAA7114Standard(I2cHandle_SAA7114,Standard);
                break;

            case STEXTVIN_DB331 :
            ErrorCode = EXTVIN_SetSAA7111Standard(I2cHandle_SAA7111,Standard);
                break;

            case STEXTVIN_STV2310 :
            ErrorCode = EXTVIN_SetSTV2310Standard(I2cHandle_STV2310,Standard);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_INVALID_STANDARD_TYPE;
                break;
        }
    }
    return(ErrorCode);
 }


/*******************************************************************************
Name        : STEXTVIN_GetVipStatus
Description : Give the VIP status
Parameters  :  Handle & Status variable
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API STEXTVIN_GetVipStatus function
*******************************************************************************/

ST_ErrorCode_t STEXTVIN_GetVipStatus(STEXTVIN_Handle_t Handle,U8* Status_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
            ErrorCode = EXTVIN_GetSAA7114Status(I2cHandle_SAA7114,Status_p);
                break;
            case STEXTVIN_DB331 :
            ErrorCode = EXTVIN_GetSAA7111Status(I2cHandle_SAA7111,Status_p);
                break;
            case STEXTVIN_STV2310 :
            ErrorCode = EXTVIN_GetSTV2310Status(I2cHandle_STV2310,Status_p);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_INVALID_INPUT_TYPE;
                break;
        }
    }
    return(ErrorCode);
}



/*******************************************************************************
Name        : STEXTVIN_SelectVipInput
Description : Select Video Input Type
Parameters  :  Handle & Selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API STEXTVIN_SelectVipInput function
*******************************************************************************/

ST_ErrorCode_t STEXTVIN_SelectVipInput(
    const STEXTVIN_Handle_t Handle,
    const STEXTVIN_VipInputSelection_t Selection
)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;


        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
            ErrorCode = EXTVIN_SelectSAA7114Input(I2cHandle_SAA7114,Selection);
                break;

            case STEXTVIN_DB331 :
            ErrorCode = EXTVIN_SelectSAA7111Input(I2cHandle_SAA7111,Selection);
                break;

            case STEXTVIN_STV2310 :
            ErrorCode = EXTVIN_SelectSTV2310Input(I2cHandle_STV2310,Selection);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_INVALID_INPUT_TYPE;
                break;
        }
    }
    return(ErrorCode);
}


/*******************************************************************************
Name        : STEXTVIN_SelectVipOutput
Description : Select Video Input Type
Parameters  :  Handle & Selection
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API STEXTVIN_SelectVipInput function
*******************************************************************************/

ST_ErrorCode_t STEXTVIN_SelectVipOutput(
    const STEXTVIN_Handle_t Handle,
    const STEXTVIN_VipOutputSelection_t Selection
)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
            ErrorCode = EXTVIN_SelectVipOutput(I2cHandle_SAA7114,Selection);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_FUNCTION_NOT_IMPLEMENTED;
                break;
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STEXTVIN_WriteI2C
Description : Write I2C register.
Parameters  :  I2C Handle, Adress and Value
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STEXTVIN_WriteI2C(const STEXTVIN_Handle_t Handle, const U8 Adress, const U8 Value)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
                ErrorCode = EXTVIN_WriteI2C(I2cHandle_SAA7114, Adress, Value);
                break;

            case STEXTVIN_TDA8752 :
                ErrorCode = EXTVIN_WriteI2C(I2cHandle_TDA8752, Adress, Value);
                break;

            case STEXTVIN_STV2310 :
                ErrorCode = EXTVIN_WriteI2C(I2cHandle_STV2310, Adress, Value);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_FUNCTION_NOT_IMPLEMENTED;
                break;
        }
    }
    return(ErrorCode);
}

/*******************************************************************************
Name        : STEXTVIN_ReadI2C
Description : Read I2C register.
Parameters  :  I2C Handle, Adress and Value
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t STEXTVIN_ReadI2C(const STEXTVIN_Handle_t Handle, const U8 Adress, const U8* Value_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;

    stextvin_Unit_t *Unit_p;
    U8* Value_p_non_const =  CAST_CONST_HANDLE_TO_DATA_TYPE(U8*,Value_p);

    if (!IsValidHandle(Handle))
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    else
    {
        Unit_p = (stextvin_Unit_t *) Handle;

        switch (Unit_p->Device_p->DeviceType)
        {
            case STEXTVIN_SAA7114 :
                ErrorCode = EXTVIN_ReadI2C(I2cHandle_SAA7114, Adress, Value_p_non_const);
                break;

            case STEXTVIN_TDA8752 :
                ErrorCode = EXTVIN_ReadI2C(I2cHandle_TDA8752, Adress, Value_p_non_const);
                break;

            case STEXTVIN_STV2310 :
                ErrorCode = EXTVIN_ReadI2C(I2cHandle_STV2310, Adress, Value_p_non_const);
                break;

            default :
            ErrorCode = STEXTVIN_ERROR_FUNCTION_NOT_IMPLEMENTED;
                break;
        }
    }
    return(ErrorCode);
}

/*
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
*/


/* End of evin_cmd.c */
