/********************************************************************************
 *   Filename   	: es_wma_parser.h											*
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

#ifndef _WMA_PARSER_H
#define _WMA_PARSER_H

/* Includes ----------------------------------------- */
#include "com_parser.h"

#ifdef __cplusplus
extern "C" {
#endif


//Functions
ST_ErrorCode_t ES_WMA_Parser_Init(ComParserInit_t *Init_p, ParserHandle_t *const handle);

ST_ErrorCode_t ES_WMA_Parser_Term(ParserHandle_t *const handle, ST_Partition_t	*CPUPartition_p);

ST_ErrorCode_t ES_WMA_ParserGetParsingFunction(  ParsingFunction_t   * ParsingFucntion_p,
												GetFreqFunction_t * GetFreqFunction_p,
												GetInfoFunction_t * GetInfoFunction_p);
ST_ErrorCode_t ES_WMA_Parser_Stop(ParserHandle_t const Handle);
ST_ErrorCode_t ES_WMA_Parser_Start(ParserHandle_t const Handle);

ST_ErrorCode_t ES_WMA_Handle_EOF(ParserHandle_t  Handle);
ST_ErrorCode_t ES_WMA_SetStreamID(ParserHandle_t  Handle, U8 WMAStreamNumber);


#ifdef __cplusplus
}
#endif



#endif /* _WMA_PARSER_H */

