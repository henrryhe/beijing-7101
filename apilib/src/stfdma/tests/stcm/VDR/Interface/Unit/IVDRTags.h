///
/// @file   VDR/Interface/Unit/IVDRTags.h
///
/// @brief  Base TAG definitions
///
/// @date 2002-11-22 
///
/// @author Dietmar Heidrich, Stefan Herr
///
/// @par OWNER:
/// STCM Architecture Team
///
/// @par SCOPE:
/// PUBLIC Header File
///
/// Base TAG definitions
///
/// &copy; 2003 ST Microelectronics. All Rights Reserved.


#ifndef IVDRTAGS_H
#define IVDRTAGS_H


//@{
/// In order to be flexible, we use tags for setting and getting data and query
/// module capabilities. Tag pairs consist of the tag itself and a data item.
/// The tag tells which variable shall be set or retrieved. The data item is
/// the value written to the variable or a pointer to the location where the
/// retrieved value is to be stored.
/// Tags have 32 bits and are built the following way:
///
/// rccc uuuu uuuu uuuu uuuu ssss ssss ssss
///
/// "cc" is the tag command get, set or query. The tagging unit follows. The
/// tag specifier itself is contained in the lowest 15 bits.
/// Tags are type checked. Tags are constructed by the MKTAG_* macros and
/// terminated by TAGDONE. The inline functions below do the type checking and
/// are optimized to NOPs.  The r bit is used for reference tags.  These tags
/// do not carry a value, but a reference to a more extended structure;
///
///
///
/// Prefixes specifying the type of operation to be performed with this tag:
///		VAL	:	no specific operation, used as storage type
///		GET	:	retrieve the current value of an item
///		SET	:	change the value of an item
///		QRY	:	query, whether a unit supports a specific tag
///
#define TAG_VAL  0x00000000
#define TAG_GET  0x10000000
#define TAG_SET  0x20000000
#define TAG_QRY  0x30000000
//@}

///
/// Macro to retrieve the command type of a tag
///
#define TAG_COMMAND(x)	((x) & 0x30000000)
#define TAG_INDEX(x)		(((x) >> 8) & 0xf)
///
/// Specify that the tag does not carry its value, but a reference to the value.
/// This tag flag is used for structs as a tag argument.  To enable the use of
/// variable size tag arguments in a layered os, each reference tag does also
/// carry the size of the argument structure as one of its members;
/// 
#define TAG_REF  0x80000000

/// To mask out the tag type
#define ANYTYPE	0x0ffff000

/// To mask out the tag without the command
#define ANYTAG		0x0fffffff

/// To mask out the index
#define ANYTAGKEY	0xfffff0ff


/// @brief VDR Tag Type ID
///
/// Bit31                              Bit0
/// |                                     |
/// T-CC UUUU UUUU UUUU UUUU IIII RRRR RRRR
///
/// T: Marks reference tag
///
/// C: Command of tag
///
/// R: Range of 256 Tag IDs available for declaring Tags of the registered type
///
/// I: Index of tag
///
/// U: Unique value assigned by STCM Identifier Value Manager (STIVAMA) 
///
/// -: Reserved
typedef uint32 VDRTID;


/// Tag Type ID marking the end of Tag Type ID lists
static const VDRTID VDRTID_DONE = 0x00000000;


///
/// Structure of a single untyped tag.  In general tags should not be build using this
/// structre, but the functions created by a MKTAG or MKRTG macro.
///
///		id				:	Tag identifier, combines type, unit, reference and unique value
///		skip			:	Offset to the next tag of the list (should be one at init time)
///		size			:	Size of the data structure in a reference tag
///		data, data2	:	Up to 64 bits of tag specific data (either in or out, based on
///							tag type)
///
struct TAGITEM {
	uint32	id;  short skip, size;
	uint32	data; uint32 data2;
	};

struct TAG : TAGITEM {
	TAG (uint32 _id, uint32 _data, int size_ = 0) { data = _data; id = _id; skip = 1; size = size_;};
	TAG() {};
#if !__EDG__  || __EDG_VERSION__ < 240
	~TAG() {}	// This is mainly used as a workaround for a certain C++ frontend compiler bug.
#endif
	};

///
/// Get a reference to the query result field of a QRY tag
///
inline bool & QRY_TAG(TAG * tag) {return *((bool *)(tag->data));}


///
/// @brief TAG terminator
///
/// Each tag list has to be terminated by a single TAGDONE tag.
///
#define TAGDONE TAG(0,0)


///
/// @brief Global function to filter Tags
///
uint32 FilterTags(TAG  * tags, uint32 id, uint32 def);



///
/// Macro to build the various inline functions and constants that are used to process
/// each single tags.
///
///	Parameters:
///		name				:	The name of the tag
///		unit				:	The unit ID of the tag
///		val				:	A unique value for this unit ID
///		type				:	The Type of the tag value
///
///	Generated functions and constants:
///
///		SET_##name		:	Used to build set commands in a tag list
///		GET_##name		:	Used to build get commands in a tag list
///		QRY_##name		:	Used to build query commands in a tag list
///		VAL_##name		:	Used to extract the value of a tag during tag processing
///		FVAL_##name		:	Used to extract a value of a tag during tag filtering
///		REF_##name		:	Used to extract a reference to the tag target value for get tags during processing
///		TTYPE_##name	:	The type of the data member of a tag
///		CSET_##name		:	The constant used for the tag ID for a set command
///		CGET_##name		:	The constant used for the tag ID for a get command
///		CQRY_##name		:	The constant used for the tag ID for a query command
///		name				:	The tag ID without the command flags
///		
#define MKTAG(name, unit, val, type)	\
	inline TAG SET_##name(type x) {return TAG(val | unit | TAG_SET, (uint32)(x));}	\
	inline TAG GET_##name(type  &x) {return TAG(val | unit | TAG_GET, (uint32)(&x));}	\
	inline TAG QRY_##name(bool  &x) {return TAG(val | unit | TAG_QRY, (uint32)(&x));}	\
	inline type VAL_##name(TAG  * tag) {return (type)(tag->data);}	\
	inline type FVAL_##name(TAG  * tags, type def) {return (type)(FilterTags(tags, val | unit | TAG_GET, (uint32)def));}	\
	inline type  & REF_##name(TAG  * tag) {return *(type  *)(tag->data);}	\
	typedef type TTYPE_##name;	\
	static const uint32 CSET_##name = val | unit | TAG_SET;	\
	static const uint32 CGET_##name = val | unit | TAG_GET;	\
	static const uint32 CQRY_##name = val | unit | TAG_QRY;	\
	static const uint32 name = val | unit;

///
/// Macro to build an indexed tag.  An indexed tag requires an index value as an additional
/// parameter for each tag creation.  This type of tags can be used, if there is more than
/// one similar element in a TAGUnit that requires configuration, e.g. audio channels.
/// The index field has 4 bits, so indices from 0 to 15 can be used.  If you have a larger
/// range of elements, it is probably a better idea to provide a special interface for
/// configuring your unit.
///
#define MKITG(name, unit, val, type)	\
	inline TAG SET_##name(uint32 index, type x) {return TAG(val | unit | TAG_SET | (index << 8), (uint32)(x));}	\
	inline TAG GET_##name(uint32 index, type  &x) {return TAG(val | unit | TAG_GET | (index << 8), (uint32)(&x));}	\
	inline TAG QRY_##name(uint32 index, bool  &x) {return TAG(val | unit | TAG_QRY | (index << 8), (uint32)(&x));}	\
	inline type VAL_##name(TAG  * tag) {return (type)(tag->data);}	\
	inline type FVAL_##name(TAG  * tags, type def) {return (type)(FilterTags(tags, val | unit | TAG_GET, (uint32)def));}	\
	inline type  & REF_##name(TAG  * tag) {return *(type  *)(tag->data);}	\
	typedef type TTYPE_##name;	\
	static const uint32 CSET_##name = val | unit | TAG_SET;	\
	static const uint32 CGET_##name = val | unit | TAG_GET;	\
	static const uint32 CQRY_##name = val | unit | TAG_QRY;	\
	static const uint32 name = val | unit;

///
/// Macro to build a reference tag.  A reference tag does not carry the actual value with it,
/// but a reference to the data.  This can be used to transfer structures or arrays of data
/// using a single tag.  Only fixed sized structures can be transfered, due to the potential
/// use of a layered OS, that needs address translation during a kernel call.
///
#define MKRTG(name, unit, val, type)	\
	inline TAG SET_##name(type  &x) {return TAG(val | unit | TAG_SET | TAG_REF, (uint32)(void *)(&x));}	\
	inline TAG GET_##name(type  &x) {return TAG(val | unit | TAG_GET | TAG_REF, (uint32)(void *)(&x));}	\
	inline TAG QRY_##name(bool  &x) {return TAG(val | unit | TAG_QRY | TAG_REF, (uint32)(&x));}	\
	inline type VAL_##name(TAG  * tag) {return *(type  *)(tag->data);}	\
	inline type  & REF_##name(TAG  * tag) {return *(type  *)(tag->data);}	\
	typedef type TTYPE_##name;	\
	static const uint32 CSET_##name = val | unit | TAG_SET | TAG_REF;	\
	static const uint32 CGET_##name = val | unit | TAG_GET | TAG_REF;	\
	static const uint32 CQRY_##name = val | unit | TAG_QRY | TAG_REF;


///
/// @brief Tag List class
///
/// Base class for tag lists, contains the pointer to the list as a first
/// element.  This pointer is used in the ConfigureTags to derive the
/// actual list.
///
/// Generic number of tags class.  Contains the tags in either the fixed
/// size array, or the base array of 16 elements.  If the amount of tags
/// exceeds the current array, the array is doubled in size;
///
class TAGList
	{
	protected:
		TAG	*	etags;
		TAG		tags[16];		/// fixed size base array
		int		num, max;		/// number and maximum number in current array

		void Grow(void);		/// grow the array in case
	public:
		TAGList & operator + (const TAG & tag);	/// attach a new element
		const TAG * Tags(void) const {return etags ? etags : tags;}

		~TAGList(void)		/// destructor, deletes allocated array if exists
			{
			delete[] etags;
			}
	};


///
/// Fixed size classes of tag lists.  Each class encodes it its type
/// the number of actual elements, such that a counter is not needed.
/// Each size inherits from its larger size, to allow easy conversion
///
class TAGList15 : public TAGList
	{
	public:
		TAGList & operator+ (const TAG & tag);
	};

class TAGList14 : public TAGList15
	{
	public:
		TAGList15 & operator+ (const TAG & tag) {tags[14] = tag; return *this;}
	};

class TAGList13 : public TAGList14
	{
	public:
		TAGList14 & operator+ (const TAG & tag) {tags[13] = tag; return *this;}
	};

class TAGList12 : public TAGList13
	{
	public:
		TAGList13 & operator+ (const TAG & tag) {tags[12] = tag; return *this;}
	};

class TAGList11 : public TAGList12
	{
	public:
		TAGList12 & operator+ (const TAG & tag) {tags[11] = tag; return *this;}
	};

class TAGList10 : public TAGList11
	{
	public:
		TAGList11 & operator+ (const TAG & tag) {tags[10] = tag; return *this;}
	};

class TAGList9 : public TAGList10
	{
	public:
		TAGList10 & operator+ (const TAG & tag) {tags[9] = tag; return *this;}
	};

class TAGList8 : public TAGList9
	{
	public:
		TAGList9 & operator+ (const TAG & tag) {tags[8] = tag; return *this;}
	};

class TAGList7 : public TAGList8
	{
	public:
		TAGList8 & operator+ (const TAG & tag) {tags[7] = tag; return *this;}
	};

class TAGList6 : public TAGList7
	{
	public:
		TAGList7 & operator+ (const TAG & tag) {tags[6] = tag; return *this;}
	};

class TAGList5 : public TAGList6
	{
	public:
		TAGList6 & operator+ (const TAG & tag) {tags[5] = tag; return *this;}
	};

class TAGList4 : public TAGList5
	{
	public:
		TAGList5 & operator+ (const TAG & tag) {tags[4] = tag; return *this;}
	};

class TAGList3 : public TAGList4
	{
	public:
		TAGList4 & operator+ (const TAG & tag) {tags[3] = tag; return *this;}
	};

///
/// Base class with only two members.  Called as a startup for the list
///
class TAGList2 : public TAGList3
	{
	public:
		TAGList2(TAG t1, TAG t2)
			{
			tags[0] = t1; tags[1] = t2;
			etags = NULL;
			}

	TAGList3 & operator+ (const TAG & tag) {tags[2] = tag; return *this;}
	};

class TAGList1 : public TAGList2
	{
	public:
		TAGList1(TAG t) : TAGList2(t, TAGDONE)
			{
			}
	};

///
/// Final concatenation, yields a list with dynamic array size
///
inline TAGList & TAGList15::operator+ (const TAG & tag)
	{
	tags[15] = tag;
	num = 16; max = 16;

	return *this;
	}

///
/// Base concatenation of two tags into a tag list.  Each tag list has
/// at least two members, a value and the done.  A tag list with a
/// single element is of no use.
///

static inline TAGList2 operator+ (const TAG & t1, const TAG & t2)
	{
	return TAGList2(t1, t2);
	}

#endif	// of #ifndef IVDRTAGS_H

