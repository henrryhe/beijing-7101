/********************************************************************************
 *	 COPYRIGHT STMicroelectronics (C) 2006										*
 *   Filename   	: es_wma_parser.c											*
 *   Start       	: 07.04.2006                       							*
 *   By          	: Bineet Srivastava											*
 *   Contact     	: bineet.srivastava@st.com									*
 *   Description  	: The file contains the parser for the WMA and WMA Pro      *
 *					  and its APIs that will be exported to the Modules.		*
 ********************************************************************************
*/

/********************************************************************
********************************************************************
********************************************************************
			WARNING 
********************************************************************
********************************************************************
********************************************************************
WMA ASF PARSING IS Licensed, After agreement with Microsoft for WMA ASF parsing 
we will provide soure code for WMA ASF parsing , 

Regards, 
Audio Driver Team Noida, 
********************************************************************
********************************************************************
*********************************************************************/


/*****************************************************************************
Includes
*****************************************************************************/
#if defined(STAUD_INSTALL_WMA) || defined(STAUD_INSTALL_WMAPROLSL)


#include "es_wma_parser.h"




/************************************************************************************
Name         : ES_WMA_Parser_Init()

Description  : Initializes the Parser and Allocate memory for the structures .

Parameters   : STAVMEM_PartitionHandle_t      Partition
	       ES_WMA_ParserInit_t   *InitParams     Pointer to the params structure

Return Value :
    ST_NO_ERROR                     Success.
    The Values returns by STAVMEM_AllocBlock and STAVMEM_GetBlockAddress Functions
 ************************************************************************************/

ST_ErrorCode_t ES_WMA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle )
{
	return ST_NO_ERROR;
}

ST_ErrorCode_t ES_WMA_ParserGetParsingFunction(	ParsingFunction_t * ParsingFunc_p,
												GetFreqFunction_t * GetFreqFunction_p,
												GetInfoFunction_t * GetInfoFunction_p)
{

	*ParsingFunc_p			= (ParsingFunction_t)NULL;
	*GetFreqFunction_p	= (GetFreqFunction_t)NULL;
	*GetInfoFunction_p	= (GetInfoFunction_t)NULL;

	return ST_NO_ERROR;
}




/************************************************************************************
Name         : ES_WMA_Parser_Stop()

Description  : Reinitializes the Parser and its structure.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_WMA_Parser_Stop(ParserHandle_t const Handle)
{
	
	return ST_ERROR_FEATURE_NOT_SUPPORTED;
}

/************************************************************************************
Name         : ES_WMA_Parser_Start()

Description  : Starts the Parser.

Parameters   : PESParser_Handle_t const Handle

Return Value :
    ST_NO_ERROR                     Success.
 ************************************************************************************/

ST_ErrorCode_t ES_WMA_Parser_Start(ParserHandle_t const Handle)
{
	return ST_ERROR_FEATURE_NOT_SUPPORTED;

}

/* Handle EOF from Parser */

ST_ErrorCode_t ES_WMA_Handle_EOF(ParserHandle_t  Handle)
{
	return ST_ERROR_FEATURE_NOT_SUPPORTED;
}


ST_ErrorCode_t ES_WMA_SetStreamID(ParserHandle_t  Handle, U8 WMAStreamNumber)
{
	return ST_ERROR_FEATURE_NOT_SUPPORTED;
}


ST_ErrorCode_t ES_WMA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t	*CPUPartition_p)
{

	return ST_NO_ERROR;
}
#endif
