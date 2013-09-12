///
/// @file       STF/Source/Types/STFString.cpp
///
/// @brief      Foundation classes for string handling
///
/// @par OWNER: STCE STF group
/// @author     Viona 
///
/// @par SCOPE: Private implementation file
///
/// @date       2004-04-16 
///
/// &copy; 2003 STMicroelectronics Design and Applications. All Rights Reserved.
///

#include "STF/Interface/Types/STFString.h"
#include "STF/Interface/STFMemoryManagement.h"
#include "STF/Interface/STFCallingConventions.h"
#include "STF/Interface/STFDebug.h"
#include <stdio.h>
#include <stdarg.h>

////////////////////////////////////////////////////////////////////
//
//  Kernel String Buffer Class Declaration
//
////////////////////////////////////////////////////////////////////

class STFStringBuffer
	{
	protected:
		char * GetNewBuffer(uint32 size);

	public:
		int32		useCnt;
		int32		length;
		char	*	buffer;

		STFStringBuffer(const char * str);
		STFStringBuffer(const char ch);
		STFStringBuffer(STFStringBuffer * u, STFStringBuffer * v);
		STFStringBuffer(STFStringBuffer * u, int32 start, int32 num);
		STFStringBuffer(STFStringBuffer * u, int32 num);
		STFStringBuffer(bool sign, uint32 value, int32 digits, int32 base, char fill);

		~STFStringBuffer(void);

		int32 Compare(STFStringBuffer * u);

		void Obtain(void)		{ useCnt++; }
		void Release(void)	{ if (!--useCnt) delete this; }
	};

////////////////////////////////////////////////////////////////////
//
//  Kernel String Buffer Class Implementation
//
////////////////////////////////////////////////////////////////////

//
//  OS Specific Memory Allocator
//

inline char * STFStringBuffer::GetNewBuffer(uint32 size)
	{
	return new (PagedPool) char[size];
	}

//
//  Constructor (by character string)
//

STFStringBuffer::STFStringBuffer(const char * str)
	{
	const char * p;
	char * q;
	
	ASSERT(str);

	useCnt = 1;
	length = 0;
	p = str;
	while ((*p++) != 0) length++;

	buffer = GetNewBuffer(length + 2);
	
	p = str;
	q = buffer;

	while ((*q++ = *p++) != 0) ;
	}

//
//  Constructor (by character)
//

STFStringBuffer::STFStringBuffer(const char ch)
	{
	useCnt = 1;
   length = 1;

	buffer = GetNewBuffer(2);

	buffer[0] = ch;
	buffer[1] = 0;
	}

//
//  Constructor (by two String buffers)
//

STFStringBuffer::STFStringBuffer(STFStringBuffer * u, STFStringBuffer * v)
	{
	char * p, * q;

	useCnt = 1;
	length = u->length + v->length;
	buffer = GetNewBuffer(length + 1);

	p = buffer;
	q = u->buffer;
	while ((*p++ = *q++) != 0) ;
	p--;
	q = v->buffer;
	while ((*p++ = *q++) != 0) ;
	}

//
//  Constructor (by part of other buffer)
//  NOTE: start and num are specified in characters here
//

STFStringBuffer::STFStringBuffer(STFStringBuffer * u, int32 start, int32 num)
	{
	char * p, * q;
	int32 i;

	useCnt = 1;
	length = u->length - start;
	if (length > num) length = num;

	buffer = GetNewBuffer(length + 1);

	p = buffer;
	q = u->buffer + start;

	for(i=0; i<length; i++) *p++ = *q++;
	
	*p = 0;
	}

//
//  Constructor (put u n-times into new string buffer)
//

STFStringBuffer::STFStringBuffer(STFStringBuffer * u, int32 num)
	{
	char * p, * q;
	int32 i;

	useCnt = 1;

	length = u->length * num;

	buffer = GetNewBuffer(length + 1);

	p = buffer;
	for(i=0; i<num; i++)
		{
		q = u->buffer;
		while ((*p++ = *q++) != 0) ;
		p--;
		}
	p++;
	}

//
//  Constructor (by number)
//

STFStringBuffer::STFStringBuffer(bool sign, uint32 value, int32 digits, int32 base, char fill)
	{
	char lbuffer[12];
	int32 pos = digits;
	int32 i;

	useCnt = 1;

	if (!pos) pos = 10;

	do {
		i = (int32)(value % base);

		if (i < 10)
			lbuffer[--pos] = '0' + i;
		else
			lbuffer[--pos] = 'A' + i - 10;
			
		value /= base;
		} while (value && pos);

	if (digits)
		{		
		buffer = GetNewBuffer(digits + 1);

		for(i=0; i<pos; i++)
			buffer[i] = fill;
		if (sign)
			buffer[0] = '-';					
		for (i=pos; i<digits; i++)
			buffer[i] = lbuffer[i];

		buffer[digits] = 0;
		length = digits;
		}
	else
		{		
		if (sign)
			{
			buffer = GetNewBuffer(10 - pos + 2);
			buffer[0] = '-';
			i = 1;
			length = 10 - pos + 1;
			}
		else
			{
			buffer = GetNewBuffer(10 - pos + 1);
			i = 0;
			length = 10 - pos;
			}
			
		while (pos < 10)
			{
			buffer[i++] = lbuffer[pos++];
			}
		buffer[i] = 0;
		}
	}

//
//  Destructor
//

STFStringBuffer::~STFStringBuffer(void)
	{
	if (buffer)
		delete[] buffer;
	}

//
//  Compare two STFStringBuffers
//

int32 STFStringBuffer::Compare(STFStringBuffer * u)
	{
	char * p, * q;
	char cp, cq;

	p = buffer;
	q = u->buffer;

	do {
		cp = *p++;
		cq = *q++;
		if (cp < cq)
			return -1;
		else if (cp > cq)
			return 1;
		} while (cp != 0);
	
	return 0;
	}

////////////////////////////////////////////////////////////////////
//
//  Kernel String Class
//
////////////////////////////////////////////////////////////////////

//
//  Constructor (no init)
//

STFString::STFString(void)
	{
	buffer = NULL;
	}

//
//  Constructor (by character string)
//

STFString::STFString(const char * str)
	{
	if (str == NULL) buffer = NULL;
	else 
		{
		buffer = new (PagedPool) STFStringBuffer(str);
		}
	}

//
//  Constructor (by character)
//

STFString::STFString(const char ch)
	{
	buffer = new (PagedPool) STFStringBuffer(ch);
	}

//
//  Constructor (by INT)
//

STFString::STFString(int32 value, int32 digits, int32 base, char fill)
	{
	if (value < 0)
		buffer = new (PagedPool) STFStringBuffer(true, (uint32)(-value), digits, base, fill);
	else
		buffer = new (PagedPool) STFStringBuffer(false, (uint32)(value), digits, base, fill);
	}

//
//  Constructor (by uint16)
//

//#if !WIN32

STFString::STFString(uint16 value, int32 digits, int32 base, char fill)
	{
	buffer = new (PagedPool) STFStringBuffer(false, value, digits, base, fill);
	}

//#endif

//
//  Constructor (by uint32)
//

STFString::STFString(uint32 value, int32 digits, int32 base, char fill)
	{
	buffer = new (PagedPool) STFStringBuffer(false, value, digits, base, fill);
	}

//
//  Copy Constructor
//

STFString::STFString(const STFString & str)
	{
	buffer = str.buffer;
	if (buffer)
		buffer->Obtain();
	}

//
//  Destructor
//

STFString::~STFString(void)
	{
	if (buffer)
		buffer->Release();
	}

//
//  Return length (in characters)
//

int32 STFString::Length(void) const
	{
	if (buffer)
		return buffer->length;
	else
		return 0;
	}

//
//  Convert string to long
//

int32 STFString::ToInt(uint32 base)
	{
	long val = 0;
	bool sign;
	char c, * p;

	if (buffer && buffer->length)
		{
		p = buffer->buffer;
		if (*p == '-')
			{
			sign = true;
			p++;
			}
		else
			sign = false;
		
		while ((c = *p++) != 0)
			{       
			if      (c >= '0' && c <= '9') val = val * base + c - '0';
			else if (c >= 'a' && c <= 'f') val = val * base + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') val = val * base + c - 'A' + 10;
			else return 0;
			}
	
		if (sign)
			return -val;
		else
			return val;
		}
	else
		return 0;
	}

//
//  Convert string to uint32
//
	
uint32 STFString::ToUnsigned(uint32 base)
	{
	uint32 val = 0;
	char c, * p;

	if (buffer && buffer->length)
		{
		p = buffer->buffer;
		
		while ((c = *p++) != 0)
			{       
			if (c>='0' && c<= '9') val = val * base + c - '0';
			else if (c>='a' && c<='f') val = val * base + c - 'a' + 10;
			else if (c>='A' && c<='F') val = val * base + c - 'A' + 10;
			else return 0;
			}
	
		return val;
		}
	else
		return 0;
	}

//
//  Copy string into character array
//  Return values signals success
//  len is length of char * str

bool STFString::Get(char * str, int32 len)
	{
	char * p, * q;
	if (buffer)
		{
		if (len > buffer->length)
			{
			p = str;
			q = buffer->buffer;

			while ((*p++ = * q++) != 0) ;

			return true;
			}
		else
			return false;
		}
	else if (len > 0)
		{
		str[0] = 0;

		return true;
		}
	else
		return false;
	}

//
//  Copy string to NT Unicode String
//
/*
#if NT_KERNEL

bool STFString::Get(UNICODE_STRING & us)
	{
	RtlCopyUnicodeString(&us, &(buffer->us));

	return true;
	}

#endif
*/
//
//  Operator = (by char string)
//

STFString & STFString::operator= (const char * str)
	{
	if (buffer)
		buffer->Release();
	buffer = new (PagedPool) STFStringBuffer(str);
	return * this;
	}

//
//  Operator = (by NT Unicode String)
//
/*
#if NT_KERNEL

STFString & STFString::operator= (const UNICODE_STRING & str)
	{
	if (buffer)
		buffer->Release();
	buffer = new (PagedPool) STFStringBuffer(str.Buffer);
	return * this;
	}

#endif
*/

//
//  Operator = (by STFString)
//

STFString & STFString::operator= (const STFString str)
	{
	if (buffer)
		buffer->Release();
	buffer = str.buffer;
	if (buffer)
		buffer->Obtain();

	return * this;
	}

//
//  Operator + (Concatenation)
//

LIBRARYCALL STFString operator+ (const STFString u, const STFString v)
	{
	STFString str = u;
	str += v;
	return str;
	}

//
//  Operator += (Append)
//

STFString & STFString::operator += (const STFString u)
	{
	STFStringBuffer * bp;

	if (buffer)
		{
		if (u.buffer)
			{
			bp = new (PagedPool) STFStringBuffer(buffer, u.buffer);
			buffer->Release();
			buffer = bp;
			}
		}
	else
		{
		buffer = u.buffer;
		if (buffer) buffer->Obtain();
		}
	return *this;
	}

//
//  Operator *
//

LIBRARYCALL STFString operator* (const STFString u, const int32 num)
	{
	STFString str = u;
	str *= num;
	return str;
	}

//
//  Operator *=
//

STFString & STFString::operator *= (const int32 num)
	{
	STFStringBuffer * bp;

	if (buffer)
		{
		if (num)
			{
			bp = new (PagedPool) STFStringBuffer(buffer, num);
			buffer->Release();
			buffer = bp;
			}
		else
			{
			buffer->Release();
			buffer = NULL;
			}
		}

	return *this;
	}

//
//  Compare two STFStrings
//

int32 STFString::Compare(const STFString str)
	{
	if (buffer == str.buffer)
		return 0;
	else if (!buffer)
		return -1;
	else if (!str.buffer)
		return 1;
	else
		return buffer->Compare(str.buffer);
	}

//
//  Operator ==
//

LIBRARYCALL bool operator==(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) == 0;
	}

//
//  Operator !=
//

LIBRARYCALL bool operator!=(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) != 0;
	}

//
//  Operator <=
//

LIBRARYCALL bool operator<=(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) <= 0;
	}

//
//  Operator >=
//

LIBRARYCALL bool operator>=(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) >= 0;
	}

//
//  Operator <
//

LIBRARYCALL bool operator<(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) < 0;
	}

//
//  Operator >
//

LIBRARYCALL bool operator>(const STFString u, const STFString v)
	{
	return ((STFString)u).Compare(v) > 0;
	}


//
//  Operator []
//

char STFString::operator[] (const int32 index) const
	{	
	if (buffer)
		return buffer->buffer[index];
	else
		return 0;
	}

void STFString::Set(int32 pos, char val)
	{
	if (pos < this->Length())
		{
	
		STFString str = *this;

		str.buffer[pos] = val;

		*this = str;
		}
	}

//
//  Operator >>
//

LIBRARYCALL STFString operator >> (const STFString u, int32 num)
	{
	STFString str = u;

	str >>= num;
	return str;
	}

//
//  Operator <<
//

LIBRARYCALL STFString operator << (const STFString u, int32 num)
	{
	STFString str = u;

	str <<= num;
	return str;
	}

//
//  Operator <<=
//

STFString & STFString::operator <<= (int32 index)
	{
	STFStringBuffer * pb;

	if (buffer)
		{
		if (index < buffer->length)
			pb = new (PagedPool) STFStringBuffer(buffer, index, buffer->length - index);
		else
			pb = NULL;

		buffer->Release();
		buffer = pb;
		}

	return * this;
	}

//
//  Operator >>
//

STFString & STFString::operator >>= (int32 index)
	{
	STFStringBuffer * pb;

	if (buffer)
		{
		if (index < buffer->length)
			pb = new (PagedPool) STFStringBuffer(buffer, 0, buffer->length - index);
		else
			pb = NULL;

		buffer->Release();
		buffer = pb;
		}

	return * this;
	}

//
//  Return segment of string
//

STFString STFString::Seg(int32 start, int32 num) const
	{
	STFString str;

	if (buffer)
		{
		if (start + num > buffer->length)
			num = buffer->length - start;

		if (num > 0)
			str.buffer = new (PagedPool) STFStringBuffer(buffer, start, num);
		}

	return str;
	}

//
//  Return first num characters
//

STFString STFString::Head(int32 num) const
	{
	STFString str;

	if (buffer && num > 0)
		{
		if (num > buffer->length)
			num = buffer->length;

		str.buffer = new (PagedPool) STFStringBuffer(buffer, 0, num);
		}

	return str;
	}

//
//  Return last num characters
//

STFString STFString::Tail(int32 num) const
	{
	STFString str;

	if (buffer && num > 0)
		{
		if (num <= buffer->length)
			str.buffer = new (PagedPool) STFStringBuffer(buffer, buffer->length - num, num);
		else
			str.buffer = new (PagedPool) STFStringBuffer(buffer, 0, num);
		}

	return str;
	}

//
//  Delete whitespaces at beginning or end of string
//

STFString STFString::Trim(void)
	{
	int32 i = 0;	
	STFString str = *this;

	// delete leading tabs and spaces...
	i = 0;	
	while ((str[i] == ' ') || (str[i] == '\t'))
		i++;
	str <<= i;


	// delete trailing tabs and spaces ...
	i = str.Length()-1;
	while ((str[i] == ' ') || (str[i] == '\t'))
		i--;	
	str = str.Seg(0, i + 1);

	return str;
	}

//
//  Return upper case version of string
//

STFString STFString::Caps(void) const
	{
	STFString str;
	int32 i;
	char c;

	if (Length())
		{
		str.buffer = new (PagedPool) STFStringBuffer(buffer, 0, Length());

		for (i=0; i<str.Length(); i++)
			{
			c = str[i];
			if (c >= 'a' && c <= 'z')
				str.buffer->buffer[i] = c + 'A' - 'a';
			}
		}
		
	return str;
	}

//
//  Return lower case version of string
//

STFString STFString::DeCaps(void) const
	{
	STFString str;
	int32 i;
	char c;

	if (Length())
		{
		str.buffer = new (PagedPool) STFStringBuffer(buffer, 0, Length());

		for (i=0; i<str.Length(); i++)
			{
			c = str[i];
			if (c >= 'A' && c <= 'Z')
				str.buffer->buffer[i] = c - ('A' - 'a');
			}
		}
		
	return str;
	}

//
//  Find first occurrence of str
//

int32 STFString::First(STFString str) const
	{
	int32 i = 0;
	
	while (i <= Length()-str.Length() && Seg(i, str.Length()) != str)
		i++;

	if (Seg(i, str.Length()) != str)
		return -1;
	else		
		return i;	
	}

//
//  Find next occurrence of str
//
	
int32 STFString::Next(STFString str, int32 pos) const
	{
	int32 i = pos + 1;
	
	while (i <= Length()-str.Length() && Seg(i, str.Length()) != str)
		i++;

	if (Seg(i, str.Length()) != str)
		return -1;
	else			
		return i;	
	}

//
//  Find last occurrence of str
//
	
int32 STFString::Last(STFString str) const
	{
	int32 i = Length() - str.Length();
	
	while (i>=0 && Seg(i, str.Length()) != str)
		i--;
		
	if (Seg(i, str.Length()) != str)
		return -1;
	else
		return i;	
	}

//
//  Find previous occurrence of str
//
	
int32 STFString::Prev(STFString str, int32 pos) const
	{
	int32 i = pos - 1;
	
	while (i>=0 && Seg(i, str.Length()) != str)
		i--;

	if (Seg(i, str.Length()) != str)
		return -1;
	else			
		return i;	
	}

//
//  Find first occurrence of c (-1 if not found)
//

int32 STFString::First(char c) const
	{
	int32 i = 0;
	
	if (buffer)
		{
		while (i < Length())
			{
			if (buffer->buffer[i] == c)
				return i;

			i++;
			}
		}
		
	return -1;	
	}

//
//  Find next occurrence of c (-1 if not found)
//
	
int32 STFString::Next(char c, int32 pos) const
	{
	int32 i = 0;

	if (buffer)
		{
		i = pos + 1;
		while (i < Length())
			{
			if (buffer->buffer[i] == c)
				return i;

			i++;
			}
		}
		
	return -1;	
	}

//
//  Find last occurrence of c (-1 if not found)
//
	
int32 STFString::Last(char c) const
	{
	int32 i = 0;

	if (buffer)
		{
		i = Length() - 1;
		while (i >= 0)
			{
			if (buffer->buffer[i] == c)
				return i;

			i--;
			}
		}
		
	return -1;	
	}

//
//  Find previous occurrence of c (-1 if not found)
//
	
int32 STFString::Prev(char c, int32 pos) const
	{
	int32 i = 0;

	if (buffer)
		{
		i = pos - 1;
		while (i >= 0)
			{
			if (buffer->buffer[i] == c)
				return i;

			i--;
			}
		}
		
	return -1;
	}

//
//  Test if string contains c
//

bool STFString::Contains(char c) const
	{
	int32 i;

	if (buffer)
		{
		for (i=0; i<Length(); i++)
			{
			if (buffer->buffer[i] == c)
				return true;
			}
		}

	return false;
	}

//#if !NT_KERNEL
//
//  Format string
//

static const uint32 MAXFORMATSTRINGLENGTH = 2000;

void STFString::Format(const char * format, ...)
	{
	va_list varArgs;
	va_start(varArgs, format);

	char str[MAXFORMATSTRINGLENGTH];

	uint32 numWritten = vsprintf(str, format, varArgs);
	ASSERT(numWritten < MAXFORMATSTRINGLENGTH);	// if this assertion fails, the stack is most likely corrupted!

	va_end(varArgs);

	*this = str;
	}

void STFString::Format(const char * format, va_list varArgs)
	{		
	char str[MAXFORMATSTRINGLENGTH] ;				// if this assertion fails, the stack is most likely corrupted!

	uint32 numWritten = vsprintf(str, format, varArgs);
	ASSERT(numWritten < MAXFORMATSTRINGLENGTH);

	va_end(varArgs);

	*this = str;
	}

//#endif

//
//  Cast into NT Unicode String
//
/*
#if NT_KERNEL

STFString::operator UNICODE_STRING * (void) const
	{
	return &(buffer->us);
	}

#endif

//
//  Return

#if NT_KERNEL

STFString::operator UNICODE_STRING & (void)
	{
	return buffer->us;
	}

#endif
*/
//
//  Return pointer to string
//

//#if !NT_KERNEL

STFString::operator char * (void) const
	{
	if (buffer)
		return buffer->buffer;
	else
		return NULL;
	}

//#endif

////////////////////////////////////////////////////////////////////
//
//  Kernel String Buffer (uint16 characters) Class Declaration
//
////////////////////////////////////////////////////////////////////

class STFStringBufferW
	{
	protected:
		uint16 * GetNewBuffer(uint32 size);

	public:
		uint16		useCnt;
		uint32		length;
		uint16	*	buffer;

		STFStringBufferW(const char * str, bool fromUnicode = false);
		STFStringBufferW(const char * str, uint32 len, bool fromUnicode = false);
		STFStringBufferW(const uint16 ch);
		STFStringBufferW(uint32 size);																// Creates an UNINITIALIZED string with certain size
		STFStringBufferW(STFStringBufferW * u, STFStringBufferW * v);
		STFStringBufferW(STFStringBufferW * u, uint32 start, uint32 num);
		STFStringBufferW(bool sign, uint32 value, int32 digits, int32 base, uint16 fill);
		STFStringBufferW(const STFStringBufferW * u);

		~STFStringBufferW(void);

		uint32 Length(void) const	{ return length; }

		int32 Compare(STFStringBufferW * u);

		void Obtain(void)		{ useCnt++; }
		void Release(void)	{ if (!--useCnt) delete this; }
	};

////////////////////////////////////////////////////////////////////
//
//  Kernel String Buffer (uint16) Class Implementation
//
////////////////////////////////////////////////////////////////////

//
//  OS Specific Memory Allocator (allocates space for trailing 0)
//

inline uint16 * STFStringBufferW::GetNewBuffer(uint32 size)
	{
/*#if (ST20LITE && __EDG__ && __EDG_VERSION__ >= 240) || LINUX
		return new uint16[size + 1];
#else*/
		return new (PagedPool) uint16[size + 1];
//#endif
	}

//
//  Constructor (by character string)
//

STFStringBufferW::STFStringBufferW(const char * str, bool fromUnicode)
	{
	const char * src;
	uint16 * dest;
	uint32 i;

	useCnt = 1;
	length = 0;
	src = str;
	if (fromUnicode)
		{
		while (*src++ || *src++)
			length++;
		}
	else
		{
		while (*src++)
			length++;
		}

	buffer = GetNewBuffer(length);
	
	src = str;
	dest = buffer;

	if (fromUnicode)
		{
		for (i=0; i<length; i++)
			{
			*dest++ = (src[0] << 8) | src[1];
			src += 2;
			}
		}
	else
		{
		for (i=0; i<length; i++)
			*dest++ = (uint16)(*src++);
		}

	*dest = 0;
	}

//
//  Constructor (by character string with length in bytes)
//

STFStringBufferW::STFStringBufferW(const char * str, uint32 len, bool fromUnicode)
	{
	const char * src;
	uint16 * dest;
	uint32 i;

	useCnt = 1;

	if (fromUnicode)
		length = len >> 1;
	else
		length = len;

	buffer = GetNewBuffer(length);
	
	src = str;
	dest = buffer;

	if (fromUnicode)
		{
		for (i=0; i<length; i++)
			{
			*dest++ = (src[0] << 8) | src[1];
			src += 2;
			}
		}
	else
		{
		for (i=0; i<length; i++)
			*dest++ = (uint16)(*src++);
		}

	*dest = 0;
	}

//
//  Constructor (by character)
//

STFStringBufferW::STFStringBufferW(const uint16 ch)
	{
	useCnt = 1;
   length = 1;
	buffer = GetNewBuffer(2);

	buffer[0] = ch;
	buffer[1] = 0;
	}

//
//  Constructor (by size). Creates an UNINITIALIZED string.
//

STFStringBufferW::STFStringBufferW(uint32 size)
	{
	useCnt = 1;
   length = size;

	buffer = GetNewBuffer(size);
	buffer[0] = 0;
	}

//
//  Constructor (by two string buffers)
//

STFStringBufferW::STFStringBufferW(STFStringBufferW * u, STFStringBufferW * v)
	{
	uint16 * src;
	uint16 * dest;

	useCnt = 1;
	length = u->length + v->length;
	buffer = GetNewBuffer(length);

	src = u->buffer;
	dest = buffer;

	while ((*dest++ = *src++) != 0) ;

	dest--;	// Compensate trailing 0
	src = v->buffer;
	while ((*dest++ = *src++) != 0) ;
	}

//
//  Constructor (by part of other buffer)
//  NOTE: start and num are specified in characters here
//

STFStringBufferW::STFStringBufferW(STFStringBufferW * u, uint32 start, uint32 num)
	{
	uint16 * src;
	uint16 * dest;
	uint32 i;

	useCnt = 1;

	//
	//  Make sure we don't copy too much
	//

	if (start + num > u->Length())
		num = u->Length() - start;

	length = num;

	buffer = GetNewBuffer(length);

	dest = buffer;
	src = u->buffer + start;

	for (i=0; i<length; i++)
		*dest++ = *src++;
	*dest = 0;
	}

//
//  Constructor (by number)
//  'digits' includes preceeding fill values and a sign (if -)
//

#define KSTRW_MAX_NUM_LENGTH 10

STFStringBufferW::STFStringBufferW(bool sign, uint32 value, int32 digits, int32 base, uint16 fill)
	{
	uint16 lbuffer[KSTRW_MAX_NUM_LENGTH + 2];
	int32 pos = digits;
	int32 i;

	useCnt = 1;

	if (!pos)
		pos = KSTRW_MAX_NUM_LENGTH;	// Pos 11 is for '\0'

	do
		{
		i = (int32)(value % base);

		if (i < 10)
			lbuffer[--pos] = '0' + i;
		else
			lbuffer[--pos] = 'A' + i - 10;
			
		value /= base;
		}
	while (value && pos);

	//
	//  Now fill calculated digits into buffer
	//

	if (digits)
		{		
		buffer = GetNewBuffer(digits + 1);

		for (i=0; i<pos; i++)
			buffer[i] = fill;

		if (sign)
			buffer[0] = '-';

		for (i=pos; i<digits; i++)
			buffer[i] = lbuffer[i];

		buffer[digits] = 0;
		length = digits;
		}
	else
		{		
		if (sign)
			{
			buffer = GetNewBuffer(KSTRW_MAX_NUM_LENGTH - pos + 2);	// Incl. sign and '\0'
			buffer[0] = '-';
			i = 1;
			length = KSTRW_MAX_NUM_LENGTH - pos + 1;
			}
		else
			{
			buffer = GetNewBuffer(KSTRW_MAX_NUM_LENGTH - pos + 1);	// Incl. '\0'
			i = 0;
			length = KSTRW_MAX_NUM_LENGTH - pos;
			}
			
		while (pos < KSTRW_MAX_NUM_LENGTH)
			{
			buffer[i++] = lbuffer[pos++];
			}
		buffer[i] = 0;
		}
	}

//
//  Copy constructor
//

STFStringBufferW::STFStringBufferW(const STFStringBufferW * u)
	{
	uint16 * src;
	uint16 * dest;

	useCnt = 1;
	length = u->Length();

	buffer = GetNewBuffer(length);

	src = u->buffer;
	dest = buffer;

	while ((*dest++ = *src++) != 0) ;
	}

//
//  Destructor
//

STFStringBufferW::~STFStringBufferW(void)
	{
	if (buffer)
		delete[] buffer;
	}

//
//  Compare two STFStringBuffers
//

int32 STFStringBufferW::Compare(STFStringBufferW * u)
	{
	uint16 * strA;
	uint16 * strB;

	strA = buffer;
	strB = u->buffer;

	while (*strA && *strB)
		{
		if (*strA < *strB)
			return -1;
		else if (*strA > *strB)
			return 1;

		strA++;
		strB++;
		}

	if (length < u->length)
		return -1;
	else if (length > u->length)
		return 1;
	else
		return 0;
	}

////////////////////////////////////////////////////////////////////
//
//  Kernel String (uint16) Class
//
////////////////////////////////////////////////////////////////////

//
//  Constructor (no init)
//

STFStringW::STFStringW(void)
	{
	buffer = NULL;
	}

//
//  Constructor (by character string)
//

STFStringW::STFStringW(const char * str, bool fromUnicode)
	{
	buffer = new (PagedPool) STFStringBufferW(str, fromUnicode);
	}

//
//  Constructor (by character string and length, probably Unicode)
//

STFStringW::STFStringW(const char * str, uint32 len, bool fromUnicode)
	{
	buffer = new (PagedPool) STFStringBufferW(str, len, fromUnicode);
	}

//
//  Constructor (by character)
//

STFStringW::STFStringW(const uint16 ch)
	{
	buffer = new (PagedPool) STFStringBufferW(ch);
	}

//
//  Constructor (by STFString)
//

STFStringW::STFStringW(const STFString & str)
	{
	uint32 i;

	buffer = new (PagedPool) STFStringBufferW((uint32)str.Length());
	for (i=0; i<(uint32)str.Length(); i++)
		{		
		buffer->buffer[i] = 	(uint16)(/*(const uint8)*/ str[(int32)i]);
		}
	buffer->buffer[Length()] = 0;
	}

//
//  Copy Constructor
//

STFStringW::STFStringW(const STFStringW & str)
	{
	buffer = str.buffer;
	if (buffer)
		buffer->Obtain();
	}

//
//  Constructor (by uint32)
//

STFStringW::STFStringW(uint32 value, int32 digits, int32 base, uint16 fill)
	{
	buffer = new (PagedPool) STFStringBufferW(false, value, digits, base, fill);
	}

//
//  Constructor (by INT)
//

STFStringW::STFStringW(int32 value, int32 digits, int32 base, uint16 fill)
	{
	if (value < 0)
		buffer = new (PagedPool) STFStringBufferW(true, (uint32)(-value), digits, base, fill);
	else
		buffer = new (PagedPool) STFStringBufferW(false, (uint32)(value), digits, base, fill);
	}

//
//  Destructor
//

STFStringW::~STFStringW(void)
	{
	if (buffer)
		buffer->Release();
	}

//
//  Return length (in characters)
//

uint32 STFStringW::Length(void) const
	{
	if (buffer)
		return buffer->Length();
	else
		return 0;
	}

//
//  Copy string into character array
//  If there is not enough room to hold the string then as much as fits is filled in
//  and GNR_NOT_ENOUGH_MEMORY is returned
//

STFResult STFStringW::Get(char * str, uint32 & len) const
	{
	STFResult err = STFRES_OK;
	uint16 * src;
	uint32 i;

	if (buffer)
		{
		if ((len >> 1) > buffer->Length())
			len = buffer->Length();
		else
			{
			if ((len >> 1) - 1 < buffer->Length())
				len = (len >> 1) - 1;
			else
				len = buffer->Length();
			err = STFRES_NOT_ENOUGH_MEMORY;
			}

		src = buffer->buffer;
		i = len;

		while (i)
			{
			*str++ = (*src) & 0xff;
			*str++ = (*src++) >> 8;
			i--;
			}

		*str++ = 0;
		*str = 0;
		}
	else 
		{
		if (len > 1)
			{
			//
			//  STFStringW is empty and there is enough room for the 0 bytes
			//

			str[0] = 0;
			str[1] = 0;
			}
		else
			err = STFRES_NOT_ENOUGH_MEMORY;
		len = 0;
		}

	STFRES_RAISE(err);
	}

//
//  Copy string as ASCII into character array (even bytes only)
//  Return values signals success
//

STFResult STFStringW::GetAsAscii(char * str, uint32 & len) const
	{
	STFResult err = STFRES_OK;
	uint16 * src;
	uint32 i;

	if (buffer)
		{
		if (len > buffer->Length())
			len = buffer->Length();
		else
			{
			if (len - 1 < buffer->Length())
				len--;
			else
				len = buffer->Length();
			err = STFRES_NOT_ENOUGH_MEMORY;
			}

		src = buffer->buffer;
		i = len;

		while (i)
			{
			*str++ = (*src++) & 0xff;
			i--;
			}

		*str++ = 0;
		*str = 0;
		}
	else 
		{
		if (len > 1)
			{
			//
			//  STFStringW is empty and there is enough room for the 0 bytes
			//

			str[0] = 0;
			str[1] = 0;
			}
		else
			err = STFRES_NOT_ENOUGH_MEMORY;
		len = 0;
		}

	STFRES_RAISE(err);
	}

//
//  Get single character
//

uint16 STFStringW::GetChar(uint32 index) const
	{
	if (index < buffer->Length())
		return buffer->buffer[index];
	else
		return 0;
	}

//
//  Operator = (by BYTE string)
//

STFStringW & STFStringW::operator= (const char * str)
	{
	if (buffer)
		buffer->Release();
	buffer = new (PagedPool) STFStringBufferW(str);
	return * this;
	}

//
//  Operator = (by STFStringW)
//

STFStringW & STFStringW::operator= (const STFStringW & str)
	{
	if (buffer)
		buffer->Release();
	buffer = str.buffer;
	if (buffer)
		buffer->Obtain();

	return * this;
	}

//
//  Operator + (Concatenation)
//

STFStringW operator+ (const STFStringW & u, const STFStringW & v)
	{
	STFStringW str = u;
	str += v;
	return str;
	}

//
//  Operator += (Append)
//

STFStringW & STFStringW::operator += (const STFStringW & u)
	{
	STFStringBufferW * bp;

	if (buffer)
		{
		if (u.buffer)
			{
			bp = new (PagedPool) STFStringBufferW(buffer, u.buffer);
			buffer->Release();
			buffer = bp;
			}
		}
	else
		{
		buffer = u.buffer;
		if (buffer)
			buffer->Obtain();
		}

	return *this;
	}

//
//  Compare two STFStringWs
//

int32 STFStringW::Compare(const STFStringW & str) const
	{
	if (buffer == str.buffer)
		return 0;
	else if (!buffer)
		return -1;
	else if (!str.buffer)
		return 1;
	else
		return buffer->Compare(str.buffer);
	}

//
//  Operator ==
//

bool operator==(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) == 0;
	}

//
//  Operator !=
//

bool operator!=(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) != 0;
	}

//
//  Operator <=
//

bool operator<=(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) <= 0;
	}

//
//  Operator >=
//

bool operator>=(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) >= 0;
	}

//
//  Operator <
//

bool operator<(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) < 0;
	}

//
//  Operator >
//

bool operator>(const STFStringW & u, const STFStringW & v)
	{
	return u.Compare(v) > 0;
	}

//
//  Return segment of string
//

STFStringW STFStringW::Seg(uint32 start, uint32 num) const
	{
	STFStringW str;

	if (buffer)
		{
		if (start + num > buffer->Length())
			num = buffer->Length() - start;

		if (num > 0)
			str.buffer = new (PagedPool) STFStringBufferW(buffer, start, num);
		}

	return str;
	}

//
//  Return first num characters
//

STFStringW STFStringW::Head(uint32 num) const
	{
	STFStringW str;

	if (buffer && num > 0)
		{
		if (num > buffer->Length())
			num = buffer->Length();

		str.buffer = new (PagedPool) STFStringBufferW(buffer, 0, num);
		}

	return str;
	}

//
//  Return last num characters
//

STFStringW STFStringW::Tail(uint32 num) const
	{
	STFStringW str;

	if (buffer && num > 0)
		{
		if (num <= buffer->Length())
			str.buffer = new (PagedPool) STFStringBufferW(buffer, buffer->Length() - num, num);
		else
			str.buffer = new (PagedPool) STFStringBufferW(buffer, 0, num);
		}

	return str;
	}

//
//  Delete whitespaces at beginning or end of string
//

STFStringW STFStringW::Trim(void) const
	{
	STFStringW str;
	uint32 start = 0;
	uint32 end;

	if (buffer)
		{
		//
		//  Find leading tabs and spaces ...
		//

		while ((buffer->buffer[start] == ' ') || (buffer->buffer[start] == '\t'))
			start++;

		if (start >= Length())
			{
			str.buffer = new (PagedPool) STFStringBufferW((uint32)0);
			}
		else
			{
			//
			//  Find trailing tabs and spaces ...
			//

			end = Length() - 1;
			while ((buffer->buffer[end] == ' ') || (buffer->buffer[end] == '\t'))
				end--;	

			str = Seg(start, end - start + 1);
			}
		}

	return str;
	}

/*
//
//  Replace all occurrences of characters in 'what' with 'with', remove doubles
//

STFStringW STFStringW::Normalize(const STFStringW & what, uint16 with)
	{
	STFStringW str;
	uint32 i;
	uint32 write = 0;

	if (buffer)
		{
		str.buffer = new STFStringBufferW(buffer);

		for (i=0; i<Length(); i++)
			{
			if (what.Contains(str.buffer->buffer[write]))
				{
				str.buffer->buffer[write] = with;

				if (!write || str.buffer->buffer[write - 1] != with)
					write++;
				else
					str.buffer->length--;
				}
			else
				write++;
			}

		str.buffer->buffer[write] = 0;
		}

	return str;
	}
*/

//
//  Replace all occurrences of characters in 'what' with 'with', remove doubles
//

STFStringW STFStringW::Normalize(const uint16 * what, uint32 howManyWhats, uint16 with)
	{
	STFStringW str;
	uint32 i;
	uint32 j;
	uint32 write = 0;

	if (buffer)
		{
		str.buffer = new STFStringBufferW(Length());

		for (i=0; i<Length(); i++)
			{
			for (j=0; j<howManyWhats; j++)
				{
				if (what[j] == buffer->buffer[i])
					break;
				}

			//
			//  If we found a character inside 'what', then replace it
			//

			if (j < howManyWhats)
				{
				str.buffer->buffer[write] = with;

				if (!write || str.buffer->buffer[write - 1] != with)
					write++;
				else
					str.buffer->length--;
				}
			else
				{
				str.buffer->buffer[write] = buffer->buffer[i];
				write++;
				}
			}

		str.buffer->buffer[write] = 0;
		}

	return str;
	}

//
//  Return upper case version of string
//

STFStringW STFStringW::Caps(void) const
	{
	STFStringW str;
	uint16 * p;

	if (Length())
		{
		str.buffer = new STFStringBufferW(buffer);

		p = str.buffer->buffer;
		do
			{
			if (*p >= 'a' && *p <= 'z')
				*p = *p + 'A' - 'a';
			p++;
			}
		while (*p);
		}
		
	return str;
	}

//
//  Return lower case version of string
//

STFStringW STFStringW::DeCaps(void) const
	{
	STFStringW str;
	uint16 * p;

	if (Length())
		{
		str.buffer = new STFStringBufferW(buffer);

		p = str.buffer->buffer;
		do
			{
			if (*p >= 'A' && *p <= 'Z')
				*p = *p + 'a' - 'A';
			p++;
			}
		while (*p);
		}
		
	return str;
	}

//
//  Find first occurrence of str
//

uint32 STFStringW::First(const STFStringW & str) const
	{
	uint32 i = 0;
	
	while (i <= Length() - str.Length() && Seg(i, str.Length()) != str)
		i++;

	if (i <= Length() - str.Length())
		return i;
	else
		return KSTRW_NOT_FOUND;	
	}

//
//  Find next occurrence of str
//
	
uint32 STFStringW::Next(const STFStringW & str, uint32 pos) const
	{
	uint32 i = pos+1;
	
	while (i <= Length() - str.Length() && Seg(i, str.Length()) != str)
		i++;
		
	if (i <= Length() - str.Length())
		return i;
	else
		return KSTRW_NOT_FOUND;	
	}

//
//  Find last occurrence of str
//
	
uint32 STFStringW::Last(const STFStringW & str) const
	{
	int32 i = Length() - str.Length();
	
	while (i >= 0 && Seg(i, str.Length()) != str)
		i--;

	if (i >= 0)
		return (uint32)i;
	else
		return KSTRW_NOT_FOUND;	
	}

//
//  Find previous occurrence of str
//
	
uint32 STFStringW::Prev(const STFStringW & str, uint32 pos) const
	{
	int32 i = pos - 1;
	
	while (i >= 0 && Seg(i, str.Length()) != str)
		i--;
		
	if (i >= 0)
		return (uint32)i;
	else
		return KSTRW_NOT_FOUND;	
	}

//
//  Find first occurrence of c (KSTRW_NOT_FOUND if not found)
//

uint32 STFStringW::First(uint16 c) const
	{
	uint32 i = 0;
	
	if (buffer)
		{
		while (i < Length())
			{
			if (buffer->buffer[i] == c)
				return i;

			i++;
			}
		}
		
	return KSTRW_NOT_FOUND;	
	}

//
//  Find next occurrence of c (KSTRW_NOT_FOUND if not found)
//
	
uint32 STFStringW::Next(uint16 c, uint32 pos) const
	{
	uint32 i;

	if (buffer)
		{
		i = pos + 1;
		while (i < Length())
			{
			if (buffer->buffer[i] == c)
				return i;

			i++;
			}
		}
		
	return KSTRW_NOT_FOUND;	
	}

//
//  Find last occurrence of c (KSTRW_NOT_FOUND if not found)
//
	
uint32 STFStringW::Last(uint16 c) const
	{
	int32 i = 0;

	if (buffer)
		{
		i = Length() - 1;
		while (i >= 0)
			{
			if (buffer->buffer[i] == c)
				return (uint32)i;

			i--;
			}
		}
		
	return KSTRW_NOT_FOUND;	
	}

//
//  Find previous occurrence of c (KSTRW_NOT_FOUND if not found)
//
	
uint32 STFStringW::Prev(uint16 c, uint32 pos) const
	{
	int32 i;

	if (buffer)
		{
		i = pos - 1;
		while (i >= 0)
			{
			if (buffer->buffer[i] == c)
				return (uint32)i;

			i--;
			}
		}
		
	return KSTRW_NOT_FOUND;
	}

//
//  Test if string contains c
//

bool STFStringW::Contains(uint16 c) const
	{
	uint32 i = 0;

	if (buffer)
		{
		while (buffer->buffer[i])
			{
			if (buffer->buffer[i] == c)
				return true;
			i++;
			}
		}

	return false;
	}
