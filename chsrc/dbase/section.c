/*******************************************************************************

/*
	(c) Copyright changhong
  Name:section.c
  Description:Section filter implementation for changhong QAM5516 DBVC platform
  Authors:yxl
  Revision    :  1.0.0
  Date          Remark   
  2004-06-05    Created
*/
#include <string.h>
#include "stddefs.h"
#include "stcommon.h"


#include "..\report\report.h"
#include "..\main\initterm.h"
#include "sectionbase.h"
#include "..\dbase\vdbase.h"
static ST_Partition_t   *section_partition;/*20060322 add*/



BOOLEAN CH6_SectionInit ( void )
{
	ST_ErrorCode_t ErrCode;
	section_partition=SystemPartition;


	CH6_SectionFilterInit(MAX_NO_SLOT,MAX_NO_FILTER_TOTAL);

	

	
	return FALSE;
}

