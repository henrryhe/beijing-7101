///
/// @file       STF/Interface/Types/STFString.h
///
/// @brief      Foundation classes for string handling
///
/// @par OWNER: STCE STF group 
/// @author     Ulrich Mohr
///
/// @par SCOPE:
///
/// @date       2004-04-16 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///


#ifndef STFSTRING_H
#define STFSTRING_H

#include <stdarg.h>

#include "STF/Interface/Types/STFResult.h"
#include "STF/Interface/Types/STFBasicTypes.h"
#include "STF/Interface/STFCallingConventions.h"


////////////////////////////////////////////////////////////////////
//
//  STFString Class
//
////////////////////////////////////////////////////////////////////

class STFStringBuffer;

class LIBRARYCALL STFString 
	{ 	
	private:
		STFStringBuffer * buffer;

	public:
		STFString(void);
		STFString(const char * str);
		//STFString(const char * str, int32 len); for modula-like strings, not implemented 
		STFString(const char ch);
		STFString(const STFString & str);
		/*!
		 * Constructor from unit32.
		 */
		STFString(uint32 value, int32 digits = 0, int32 base = 10, char fill = '0');
		STFString(int32  value, int32 digits = 0, int32 base = 10, char fill = '0');
		STFString(uint16 value, int32 digits = 0, int32 base = 10, char fill = '0');
		~STFString();

		// returns the length of the string EXCLUDING the succeeding zero...
		int32 Length() const;

		int32 ToInt(uint32 base = 10);
		uint32 ToUnsigned(uint32 base = 10);

		//! puts the whole String in a char *.
		/*!
		 * @param str the buffer to put the string
		 * @param len the length of the buffer
		 * @returns true on success false otherwise
		 */
		bool Get(char * str, int32 len);


		STFString & operator = (const char * str);
		STFString & operator = (const STFString str);

		friend LIBRARYCALL STFString operator+ (const STFString u, const STFString v);
		STFString & operator+= (const STFString u);
		//! concat u n-times
		friend LIBRARYCALL STFString operator* (const STFString u, const int32 num);
		STFString & operator*= (const int32 num);
		/*!
		 * compares the Strings.
		 * @return 0 if both Strings are equal, -1 if this String has smaller char-values, +1 if str has smaller char-values
		 */
		int32 Compare(const STFString str);

		friend LIBRARYCALL bool operator==(const STFString u, const STFString v);
		friend LIBRARYCALL bool operator!=(const STFString u, const STFString v);
		friend LIBRARYCALL bool operator< (const STFString u, const STFString v);
		friend LIBRARYCALL bool operator> (const STFString u, const STFString v);
		friend LIBRARYCALL bool operator<=(const STFString u, const STFString v);
		friend LIBRARYCALL bool operator>=(const STFString u, const STFString v);

		/*!
		 * cuts away the left side.
		 */
		friend LIBRARYCALL STFString operator << (const STFString u, int32 index);
		/*!
		 * cuts the right side.
		 */
		friend LIBRARYCALL STFString operator >> (const STFString u, int32 index);
		STFString & operator <<= (int32 index);
		STFString & operator >>= (int32 index);

		STFString Seg(int32 start, int32 num) const;	//!< Extract seqment of string 
		STFString Caps(void) const;					//!< return uppercase string
		STFString DeCaps(void) const;					//!< return lowercase string
		STFString Head(int32 num) const;				//!< Return the first num characters
		STFString Tail(int32 num) const;				//!< Return the last num characters

		int32 First(STFString str) const;				//!< Find first occurrence of str
		int32 Next(STFString str, int32 pos) const;	//!< Find next occurrence of str
		int32 Last(STFString str) const;					//!< Find last occurrence of str
		int32 Prev(STFString str, int32 pos) const;		//!< Find previous occurrence of str

		int32 First(char c) const;							//!< Find first occurrence of c (-1 if not found)
		int32 Next(char c, int32 pos) const;				//!< Find next occurrence of c (-1 if not found)
		int32 Last(char c) const;							//!< Find last occurrence of c (-1 if not found)
		int32 Prev(char c, int32 pos) const;				//!< Find prevoius occurrence of c (-1 if not found)

		bool Contains(char c) const;						//!< Test if character occurs in string

		STFString Trim(void);							//!< Deletes spaces (and tabs) at beginning or end of string
		
		char operator[] (const int32 index) const;
		void Set(int32 pos, char val);

		void Format(const char * format, ...);
		void Format(const char * format, va_list varArgs);

//
// Unsafe functions
//

		operator char * (void) const;
	};

////////////////////////////////////////////////////////////////////
//
//  STFStringW Class (with WORD characters)
//
////////////////////////////////////////////////////////////////////

#define KSTRW_NOT_FOUND	0xffffffff

class STFStringBufferW;

class LIBRARYCALL STFStringW 
	{ 	
	private:
		STFStringBufferW * buffer;

	public:
		STFStringW(void);
		STFStringW(const char * str, bool fromUnicode = false);
		STFStringW(const char * str, uint32 len, bool fromUnicode = false);
		STFStringW(const uint16 ch);
		STFStringW(const STFString & str);
		STFStringW(const STFStringW & str);
		STFStringW(uint32 value, int32 digits = 0, int32 base = 10, uint16 fill = '0');
		STFStringW(int32  value, int32 digits = 0, int32 base = 10, uint16 fill = '0');
		~STFStringW();

		uint32 Length() const;											//!< Returns the length of the string EXCLUDING the trailing zero...

		/*!
		 * writes the content of this StringW in an char-array, in UTF-16.
		 * @param len the length of str, after the call the number of written characters.
		 * @param str the char-array to write to
		 */
		STFResult Get(char * str, uint32 & len) const;
		STFResult GetAsAscii(char * str, uint32 & len) const;
		uint16	GetChar(uint32 index) const;

		STFStringW & operator = (const char * str);
		STFStringW & operator = (const STFStringW & str);

		friend LIBRARYCALL STFStringW operator+ (const STFStringW & u, const STFStringW & v);
		STFStringW & operator+= (const STFStringW & u);

		int Compare(const STFStringW & str) const;

		friend LIBRARYCALL bool operator==(const STFStringW & u, const STFStringW & v);
		friend LIBRARYCALL bool operator!=(const STFStringW & u, const STFStringW & v);
		friend LIBRARYCALL bool operator< (const STFStringW & u, const STFStringW & v);
		friend LIBRARYCALL bool operator> (const STFStringW & u, const STFStringW & v);
		friend LIBRARYCALL bool operator<=(const STFStringW & u, const STFStringW & v);
		friend LIBRARYCALL bool operator>=(const STFStringW & u, const STFStringW & v);

		STFStringW Seg(uint32 start, uint32 num) const;							// Extract seqment of string
		STFStringW Caps(void) const;													// Return uppercase string
		STFStringW DeCaps(void) const;												// Return lowercase string
		STFStringW Head(uint32 num) const;											// Return the first num characters
		STFStringW Tail(uint32 num) const;											// Return the last num characters
		STFStringW Trim(void) const;													// Deletes spaces (and tabs) at beginning or end of string
		/*!
		 * Replace all occurrences of characters in 'what' with 'with', remove doubles.
		 * for Example for path-separators: foo\\bar\ -> foo/bar/
		 */
		STFStringW Normalize(const uint16 * what, uint32 howManyWhats, uint16 with);	
		uint32 First(const STFStringW & str) const;								// Find first occurrence of str
		uint32 Next(const STFStringW & str, uint32 pos) const;					// Find next occurrence of str
		uint32 Last(const STFStringW & str) const;									// Find last occurrence of str
		uint32 Prev(const STFStringW & str, uint32 pos) const;					// Find previous occurrence of str

		uint32 First(uint16 c) const;															// Find first occurrence of c (-1 if not found)
		uint32 Next(uint16 c, uint32 pos) const;											// Find next occurrence of c (-1 if not found)
		uint32 Last(uint16 c) const;															// Find last occurrence of c (-1 if not found)
		uint32 Prev(uint16 c, uint32 pos) const;											// Find prevoius occurrence of c (-1 if not found)

		bool Contains(uint16 c) const;														// Test if character occurs in string
	};

#endif
