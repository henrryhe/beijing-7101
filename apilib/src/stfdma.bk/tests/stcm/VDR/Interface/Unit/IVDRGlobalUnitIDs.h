///
/// @file   VDR/Interface/Unit/IVDRGlobalUnitIDs.h
///
/// @brief  Definition of Global Unit IDs
///
/// @author Stefan Herr
///
/// @par OWNER:
/// STCM Architecture Team
///
/// @par SCOPE:
/// PUBLIC Header File
///
/// Definition of Global Unit IDs
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.

#ifndef IVDRGLOBALUNITIDS_H
#define IVDRGLOBALUNITIDS_H

#include "STF/Interface/Types/STFBasicTypes.h"


/// @brief Type definition for VDRUID (Unit ID)
///
///
/// Bit31                              Bit0
/// |                                     |
/// CLP- ---- UUUU UUUU UUUU UUUU UUUU UUUU
///
/// C: Customer-defined unit ID
///    0: Unit ID defined by ST - if public, registered with the 
///       STCM Identifier Value Manager
///    1: Unit ID defined by customer and not registered 
///
/// L: Local Unit ID flag
///    0: Unit ID available for access by an application for the purpose 
///       of Virtual Unit Set allocation
///    1: Unit ID local to a specific project and Global Board Configuration
///       and not accessible by an application running on the VDR Layer 
///
/// P: Precoditioned ID. This flag is used internal in the board construction and MUST not
///    be set in any unit id
///    0: The creation of the unit is to be done always
///    1: The creation of the unit is only to be done when it matches the hardware
///
/// U: Unique value assigned by STCM Identifier Value Manager (STIVAMA)
///
/// -: Reserved for future use
///
typedef uint32	VDRUID;

#define VDRUIDF_CONSTUCTION_CONDITIONAL 0x20000000

#endif	// of #ifndef IVDRGLOBALUNITIDS_H
