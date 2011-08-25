#ifndef _CSTRING_H
#define _CSTRING_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifndef DBG
#	ifdef _DEBUG
#		define DBG 1
#	else
#		define DBG 0
#	endif
#endif

#define DBG_ARRAYS	0 //DBG

#include "pstypes.h"
#include "cquicksort.h"
#include "cfile.h"

void ArrayError (const char* pszMsg);

//-----------------------------------------------------------------------------

class CString : public CArray < char > {
	private:
		uint	m_length;

	public:
		explicit CString (char* s, int l = -1) {
			Length () = (uint) ((l < 0) ? strlen (s) : l);
			Init (); 
			Create (Length () + 1);
			memcpy (Buffer (), s, Length ());
			Buffer () [Length ()] = '\0';
			}

		explicit CString (const CString& other) : CArray<char> (other) {
			Length () = other.Length ();
		}

		inline uint Length (void) { return m_length; }

		inline operator char*() { return Buffer (); }

		inline CString& operator= (char* s) {
			uint l = strlen (s);
			if (l + 1 > m_data.length ()) {
				Resize (l + 1);
				if (l + 1 > m_data.length ())
					return *this;
				}
			memcpy (Buffer (), s, l + 1);
			return *this;
			}

		inline bool operator== (const CString& other) { 
			return strcmp (Buffer (), other.Buffer ()) == 0; 
			}

		inline bool operator!= (const CString& other) { 
			return strcmp (Buffer (), other.Buffer ()) != 0; 
			}

		inline CString& operator+= (char* s) {
			uint l = strlen (s);
			if (Length () + l + 1 > m_data.length ()) {
				Resize (Length () + l + 1);
				if Length () + l + 1 > m_data.length ())
					return *this;
				}
			memcpy (Buffer (Length ()), s, l + 1);
			return *this;
			}

		inline CString operator+ (const CString other) {
			uint l = Length () + other.Length ();
			if (l > m_data.Length ()) {
				Resize (l + 1);
				if (l > m_data.Length ())
					return *this;
				}
			CString s (Buffer (), l + 1);
			s += other;
			return s;
			}
		
		inline CString SubStr (int nOffset, int nLength) {
			if (nOffset > Length ())
				nLength = 0;
			else if (nLength > Length () + 1 - nOffset)
				nLength = Length () + 1 - nOffset;
			return CString (Buffer () + nOffset, nLength);
			}

		inline CString& Delete (int nOffset, int nLength) {
			if (nOffset > Length ())
				return *this;
			if (if (nLength > Length () + 1 - nOffset)
				nLength = Length () + 1 - nOffset;
			memcpy (Buffer () + nOffset, Buffer () + nOffset + nLength, Length () + 2 - nOffset + nLength);
			Length () -= nLength;
			}
	};

//-----------------------------------------------------------------------------


#endif //_CARRAY_H
