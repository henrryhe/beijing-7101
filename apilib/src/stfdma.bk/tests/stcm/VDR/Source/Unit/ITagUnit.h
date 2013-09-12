// FILE:			VDR/Source/Unit/ITagUnit.h
//
// AUTHOR:		Ulrich Sigmund, Stefan Herr
// COPYRIGHT:	(c) 2002 STMicroelectronics.  All Rights Reserved.
// CREATED:		05.12.2002
//
// PURPOSE:		Internal Interface of Units supporting Tags
//
// SCOPE:		INTERNAL Header File

#ifndef ITAGUNIT_H
#define ITAGUNIT_H

#include "VDR/Interface/Unit/IVDRTagUnit.h"

//! Internal Tag Unit interface ID
static const VDRIID VDRIID_TAG_UNIT = 0x80000002;

//! Internal interface for units that support Tags
class ITagUnit : virtual public IVDRTagUnit
	{
	public:
		/// Retrieve the Tag Unit IDs supported by the unit exposing this interface
		virtual STFResult GetTagIDs(VDRTID * & ids) = 0;

		/// Internal Begin of Configuration process (for counting)
		virtual STFResult InternalBeginConfigure(void) = 0;

		/// Abort configuration process after it was started with InternalBeginConfigure()
		virtual STFResult InternalAbortConfigure(void) = 0;

		/// Internal Completion of Configuration, leads to actual commitment of changes
		virtual STFResult InternalCompleteConfigure(void) = 0;

		/// Internal Tag configuration function
		virtual STFResult InternalConfigureTags(TAG * tags) = 0;

		/// Internal Update function to program just the required changes
		virtual STFResult	InternalUpdate(void) = 0;
	};


///////////////////////////////////////////////////////////////////////////////
// Macros for easy parsing of TAG lists during InternalConfigure() calls
///////////////////////////////////////////////////////////////////////////////

// A typical TAG parsing routine will look like this
//
//	Error VirtualMPEGDecoder::InternalConfigureTags(TAG *tags)
//		{
//		PARSE_TAGS_START(changeSet, tags)
//			GETSET(TAG_ID, variable, changeSetGroup)
//		PARSE_TAGS_END
//	
//		GNRAISE_OK;
//		}
//
 

// Macro to start parsing of a tag list
// tags: tag list
// affectedChangeSet: change set variable receiving the change set group flags
#define PARSE_TAGS_START(tags, affectedChangeSet) \
	TAG * tp = tags; \
	uint32 * __currentChangeSetX2f14 = &affectedChangeSet; \
	while (tp->id) { \
		switch (tp->id & ANYTAGKEY) {

// Macro to end parsing of a tag list
#define PARSE_TAGS_END \
	default: \
		*__currentChangeSetX2f14 |= 0x0; \
		break; \
		} tp += tp->skip; }
		
// Macro to set a variable or get a variable, and mark the change set group
// a SET operation belongs to.
// tagid: Tag ID
// variable: variable receiving the tag's value for a SET_ operation or read in a GET_ operation
// changeSetGroup: specifies bit number to be set in change set if tag value changes
#define GETSETC(tagid, variable, changeSetGroup)	\
	case CSET_##tagid:	\
		variable = VAL_##tagid(tp);	\
		*__currentChangeSetX2f14 |= 1 << changeSetGroup; \
		break;	\
	case CGET_##tagid:	\
		REF_##tagid(tp) = variable; \
		break;	\
	case CQRY_##tagid:	\
		QRY_TAG(tp) = true;	\
		break

// Variant for indexed tags
#define GETSETIC(tagid, variable, changeSetGroup)	\
	case CSET_##tagid:	\
		variable[TAG_INDEX(tp->id)] = VAL_##tagid(tp);	\
		*__currentChangeSetX2f14 |= 1 << changeSetGroup; \
		break;	\
	case CGET_##tagid:	\
		REF_##tagid(tp) = variable[TAG_INDEX(tp->id)]; \
		break;	\
	case CQRY_##tagid:	\
		QRY_TAG(tp) = true;	\
		break

// Macro for TAGs that can only be retrieved with GET.
// Value is read from variable.
// tagid: Tag ID
// variable: variable receiving the tag's value for a SET_ operation or read in a GET_ operation
#define GETONLY(tagid, variable) \
	case CSET_##tagid:	\
		STFRES_RAISE(STFRES_OBJECT_READ_ONLY);	\
		break;	\
	case CGET_##tagid:	\
		REF_##tagid(tp) = variable;	\
		break;	\
	case CQRY_##tagid:	\
		QRY_TAG(tp) = true;	\
		break

// Variant for indexed tags
#define GETONLYI(tagid, variable) \
	case CSET_##tagid:	\
		STFRES_RAISE(STFRES_OBJECT_READ_ONLY);	\
		break;	\
	case CGET_##tagid:	\
		REF_##tagid(tp) = variable[TAG_INDEX(tp->id)];	\
		break;	\
	case CQRY_##tagid:	\
		QRY_TAG(tp) = true;	\
		break

// Macro for TAGs that can only be retrieved with GET.
// Value is read by function.
#define GETONLYF(tagid, function) \
	case CSET_##tagid:	\
		STFRES_RAISE(STFRES_OBJECT_READ_ONLY);	\
		break;	\
	case CGET_##tagid:	\
		REF_##tagid(tp) = function;	\
		break;	\
	case CQRY_##tagid:	\
		QRY_TAG(tp) = true;	\
		break


///////////////////////////////////////////////////////////////////////////////
// Macros for easy updating of changed parameters in InternalUpdate() calls
///////////////////////////////////////////////////////////////////////////////

#define UPDATE_GROUP_CALL(affectedChangeSet, changeSetGroup, call) \
	if (affectedChangeSet & (1 << changeSetGroup)) \
		STFRES_REASSERT(call);

#endif	// #ifndef ITAGUNIT_H
