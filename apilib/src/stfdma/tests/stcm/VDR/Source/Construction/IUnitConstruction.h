
// FILE:			VDR/Source/Construction/IUnitConstruction.h
// AUTHOR:		S. Herr
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		22.11.2002
//
// PURPOSE:		Definitions for Unit Construction process
//
// SCOPE:		PUBLIC Header File

#ifndef IUNITCONSTRUCTION_H
#define IUNITCONSTRUCTION_H

#include "VDR/Interface/Unit/IVDRGlobalUnitIDs.h"
#include "VDR/Source/Unit/IPhysicalUnit.h"


/// the configuration is complete, but the type of the configuration objects is incorrect
static const STFResult STFRES_BOARDCONSTRUCTION_INVALID_CONFIGURATION   = 0x8884400f;


/// the configuration is incomplet (e.g. there is an unsatisfied unit dependency)
static const STFResult STFRES_BOARDCONSTUCTION_INCOMPLETE_CONFIGURATION = 0x88844011;

/// there are more parameters than expected in the board construction
static const STFResult STFRES_BOARDCONSTRUCTION_TOO_MANY_PARAMETERS = 0x88844012;

// Function prototype for Physical Unit Creation (Factory) functions
typedef STFResult (* PhysicalUnitFactory)(long unitID, long * params, IPhysicalUnit *& physicalUnitInterface);

// Macro to declare the unit creation functions
#define DECLARE_UNIT_CREATION_FUNCTION(funcName) \
	STFResult funcName (VDRUID unitID, long * params, IPhysicalUnit *& physicalUnitInterface);
	

// Macro to unify the implementation of the unit creation functions
#define UNIT_CREATION_FUNCTION(funcName, type) \
	STFResult funcName(VDRUID unitID, long * params, IPhysicalUnit *& physicalUnitInterface) \
		{ \
		type * unit; \
		unit = new type(unitID); \
		STFRES_ASSERT(unit, STFRES_NOT_ENOUGH_MEMORY); \
		STFRES_REASSERT(unit->Create(params)); \
		physicalUnitInterface = static_cast<IPhysicalUnit*>(unit); \
		STFRES_RAISE_OK; \
		}

// 
// Control word definitions for setting up the Global Board Configuration structure
// obtained from the Unit Construction Graph
//

// Marks end of the Global Board Configuration structure
static const long UNITS_DONE		=	0;		


// Marks end of a "Local Unit ID" to "Global Unit ID" mapping
static const long MAPPING_DONE	=	0;

// Marks end of a parameter list
static const long PARAMS_DONE		 =	0;

// Identifies a DWORD parameter
static const long PARAMS_DWORD	 =	1;

// Identifies a STRING parameter
static const long PARAMS_STRING	 =	2;

// Identifies a STRING parameter
static const long PARAMS_POINTER  =	4;


//
// Macros used to set up the Global Board Configuration structure
//

#if _DEBUG
// leave this ON now, UnitConstruction.cpp controls board creation output
// via the _DEBUG_VERBOSE_UNIT_CREATION_OUTPUT setting
#define _DEBUG_VERBOSE_CREATE_UNIT	0 
#else
#define _DEBUG_VERBOSE_CREATE_UNIT	0 // do not change this ever!
#endif

// Creates a Physical Unit entry in the structure
#if _DEBUG_VERBOSE_CREATE_UNIT
#define CREATE_UNIT(id, call)	(long)#id, (long)#call, (id), (long)call
#else
#define CREATE_UNIT(id, call)	(id), (long)call
#endif

#if _DEBUG_VERBOSE_CREATE_UNIT
#define CREATE_UNIT_CONDITIONAL(id, call, mask, value) (long) #id, (long) #call, (long)((id) & VDRUIDF_CONSTUCTION_CONDITIONAL), (long)call, (long)mask, (long)value
#else
#define CREATE_UNIT_CONDITIONAL(id, call, mask, value) (long)((id) & VDRUIDF_CONSTUCTION_CONDITIONAL), (long)call, (long)mask, (long)value
#endif

// Creates a DWORD parameter of a parameter list
#define DWORD_PARAM(dw)	PARAMS_DWORD, (dw)

// Creates a STRING parameter of a parameter list
#define STRING_PARAM(str)	PARAMS_STRING,	(long)(str)

// array parameter
#define POINTER_PARAM(ptr) PARAMS_POINTER, (long)(ptr)


// Declaration of GlobalBoardConfiguration to
//??? "to" what? The comment breaks off in the middle of the sentence.
extern long GlobalBoardConfig[];


//
// Methods / Macros to simplify parsing of unit creation parameters
//



#define GET_DWORD_PARAM(var)\
	do {	\
		if (*createParams == PARAMS_DWORD) \
			{ \
			createParams++; \
			var = (uint32) *createParams; \
			createParams++; \
			} \
		} \
	while (0)
			
// The CHAIN_CONNECTION macros simplify the connecting of units within a 
// GenericStreamingChainUnit.  One need not remember the arcane rules for determining
// when a connector ID is relative 0 or relative 1, for example.  Also, since most 
// connections are from connector ID 0 to connector ID 0, this case is implemented
// separately, as a two parameter macro, while connections to IDs > 0 are handled 
// by the 4-parameter macro "CHAIN_CONNECTION_SPECIAL".
//
// Old way:
//	// Analyzer
//	DWORD_PARAM(0xff000000),	// Connect Chain, Out0 -> Unit0, In0
//	DWORD_PARAM(0x00010200),	// Connect Unit0, Out0 -> Unit2, In0
//	
//	// Replicator
//	DWORD_PARAM(0x02010100),	// Connect Unit2, Out0 -> Unit1, In0
//	DWORD_PARAM(0x0202ff02),	// Connect Unit2, Out1 -> Chain, In1
//	
//	// Decoder
//	DWORD_PARAM(0x0101ff01),	// Connect Unit1, Out0 -> Chain, In0
//
// New way:
//	// Analyzer (Unit0)
//	CHAIN_CONNECTION(CHAIN,0),	// Connect Chain, Out0 -> Unit0, In0
//	CHAIN_CONNECTION(0,2),		// Connect Unit0, Out0 -> Unit2, In0
//
//	// Replicator (Unit2)
//	CHAIN_CONNECTION(2,1),		// Connect Unit2, Out1 -> Unit1, In0
//	CHAIN_CONNECTION_SPECIAL(2,1,CHAIN,1),	// Connect Unit2, Out2 -> Chain, In1
//
//	// Decoder (Unit1)
//	CHAIN_CONNECTION(1,CHAIN),	// Connect Unit0, Out0 -> Chain, In0

#define CHAIN 0xFF
#define CHAIN_CONNECTION_SPECIAL(u1,c1,u2,c2) DWORD_PARAM( \
   (u1 << 24) | (((u1==CHAIN)?c1:(c1 + 1)) << 16) | (u2 << 8) | \
   ((u2==CHAIN) ? (c2+1) : c2))
#define CHAIN_CONNECTION(u1,u2)	CHAIN_CONNECTION_SPECIAL(u1,0,u2,0)

#endif	// of #ifndef IUNITCONSTRUCTION_H
