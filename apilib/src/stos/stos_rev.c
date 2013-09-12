/*****************************************************************************
File name   :  stos_rev.c

Description :  Operating system independence file.

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                          Name
----               ------------                                          ----
08/06/2007         Created                                               WA
*****************************************************************************/

#include "stos.h"

/*******************************************************************************
Name        : STOS_GetRevision
Description : Get revision of the STOS driver
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_Revision_t STOS_GetRevision(void)
{
  static const char Revision[] = "STOS-REL_2.1.2";

  return((ST_Revision_t) Revision);
}

/* End of file */

