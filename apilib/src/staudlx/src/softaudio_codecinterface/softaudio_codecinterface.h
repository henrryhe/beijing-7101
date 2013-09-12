/********************************************************************************
 *   Filename   	: mme_wrapper.h											*
 *   Start       		: 07.06.2007                                                   							*
 *   By          		: udit Kumar											*
 *   Contact     	: udit-dlh.kumar@st.com										*
 *   Description  	: The file contains the MME wrapper layer						*
 *				 											*
 ********************************************************************************
 */


#ifndef MME_WRAPPER_LAYER_T
#define MME_WRAPPER_LAYER_T
#include "stcodec_interface.h"
#include "mme_interface.h"
#include "staudlx.h"
#include "audio_encodertypes.h"
#include "mixer_processortypes.h"

typedef struct MME_WrapperLayer_s
{
	char TransformerName[80];
	MME_TransformerHandle_t TransformerHandle;
	void *UserData;
	MME_GenericCallback_t UserCallbackFunction;
	struct MME_WrapperLayer_s *Next; 	

}MME_WrapperLayer_t; 
#endif 

