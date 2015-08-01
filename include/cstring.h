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
#include "carray.h"

//-----------------------------------------------------------------------------

class CString : public CArray < char > {
	private:
		uint32_t	m_length;

	public:
		explicit CString (char* s, int l = -1) {
			m_length = (uint32_t) ((l < 0) ? strlen (s) : l);
			Init (); 
			Create (Length () + 1);
			memcpy (Buffer (), s, Length ());
			Buffer () [Length ()] = '\0';
			}

		CString (const CString& other) : CArray<char> (other) {
			m_length = other.Length ();
		}

		inline uint32_t Length (void) const { return m_length; }

		inline operator char*() { return Buffer (); }

		inline CString& operator= (char* s) {
			uint32_t l = (uint32_t) strlen (s);
			if (l + 1 > m_data.Length ()) {
				Resize (l + 1);
				if (l + 1 > m_data.Length ())
					return *this;
				}
			memcpy (Buffer (), s, l + 1);
			return *this;
			}

		inline const bool operator== (CString const & other) { 
			return strcmp (Buffer (), other.Buffer ()) == 0; 
			}

		inline bool operator!= (CString const & other) { 
			return strcmp (Buffer (), other.Buffer ()) != 0; 
			}

		inline CString& operator+= (char* s) {
			uint32_t l = (uint32_t) strlen (s);
			if (Length () + l + 1 > m_data.Length ()) {
				Resize (Length () + l + 1);
				if (Length () + l + 1 > m_data.Length ())
					return *this;
				}
			memcpy (Buffer (Length ()), s, l + 1);
			return *this;
			}

		inline CString& operator+= (CString const & other) { 
			*this += other.Buffer (); 
			return *this;
			}

		inline CString operator+ (const CString& other) {
			uint32_t l = Length () + other.Length ();
			if (l > m_data.Length ()) {
				Resize (l + 1);
				if (l > m_data.Length ())
					return *this;
				}
			CString s (Buffer (), l + 1);
			s += other;
			return s;
			}
		
		inline CString SubStr (uint32_t nOffset, uint32_t nLength) {
			if (nOffset > Length ())
				nLength = 0;
			else if (nLength > Length () + 1 - nOffset)
				nLength = Length () + 1 - nOffset;
			return CString (Buffer () + nOffset, nLength);
			}

		inline CString& Delete (uint32_t nOffset, uint32_t nLength) {
			if (nOffset > Length ())
				return *this;
			if (nLength > Length () + 1 - nOffset)
				nLength = Length () + 1 - nOffset;
			memcpy (Buffer () + nOffset, Buffer () + nOffset + nLength, Length () + 2 - nOffset + nLength);
			m_length -= nLength;
			return *this;
			}
	};

//-----------------------------------------------------------------------------


#endif //_CARRAY_H
