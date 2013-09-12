
// FILE:			IVDRTagUnits.h
//
// AUTHOR:		Stefan Herr
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		04.12.2002
//
// PURPOSE:		Interface of units that accept Tags
//
// SCOPE:		PUBLIC Header File

/*! \file
	 \brief IVDR Tag Units Interface
*/

#ifndef IVDRTAGUNITS_H
#define IVDRTAGUNITS_H

#include "VDR/Interface/Base/IVDRBase.h"
#include "VDR/Interface/Unit/IVDRTags.h"

//! Public Tag Unit Interface ID
static const VDRIID VDRIID_VDR_TAG_UNIT = 0x00000007;

//! Public Tag Unit Interface
/*! 
	 Defines functions to configure units with Tag lists
*/
class IVDRTagUnit : virtual public IVDRBase
	{
	public:
		//! Start configuration process
		/*! This function must be called before any Tags are sent to a unit.	*/
		virtual STFResult BeginConfigure(void) = 0;
		
		//! Tag Configuration function
		/* Send a list of Tags to a unit in order to configure properties.
		   Changes will not get into effect until CompleteConfigure is
			called. */
		virtual STFResult ConfigureTags(TAG * tags) = 0;

		//! Special variable parameter list version of Tag Configuration function
		virtual STFResult Configure(const TAGList & tagList) = 0;

		//! Complete configuration process
		/*! This function must be called in order to commit the changes of
		    all Configure calls done after BeginConfigure. It allows the
			 actual programming of configuration Tags to happen at a 
			 defined point in time. */
		virtual STFResult CompleteConfigure(void) = 0;
	};

#endif	// #ifndef IVDRTAGUNITS_H
