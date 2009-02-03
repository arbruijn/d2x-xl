#ifndef _CARRAY_H
#define _CARRAY_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#ifndef DBG
#	ifdef _DEBUG
#		define DBG 1
#	else
#		define DBG 0
#	endif
#endif

#include "pstypes.h"
#include "cquicksort.h"
#include "cfile.h"

//-----------------------------------------------------------------------------

template < class _T > 
class CArray : public CQuickSort < _T > {

	template < class _U > 
	class CArrayData {
		public:
			_U		*buffer;
			_U		null;
			uint	length;
			uint	pos;
			int	nMode;
			bool	bWrap;
			};

	protected:
		CArrayData<_T>	m_data;

	public:
		template < class _V >
		class Iterator {
			private:
				_V*			m_start;
				_V*			m_end;
				_V*			m_p;
				CArray<_V>&	m_a;
			public:
				Iterator (CArray<_V>& a) { m_a = a, m_p = NULL; }
				operator bool() const { return m_p != NULL; }
				_V* operator*() const { return m_p; }
				Iterator& operator++() { 
					if (m_p) {
						if (m_p < m_end)
							m_p++;
						else
							m_p = NULL;
						}
					return *this;
					}
				Iterator& operator--() { 
					if (m_p) {
						if (m_p > m_end)
							m_p--;
						else
							m_p = NULL;
						}
					return *this;
					}
				_V* Start (void) { m_p = m_start = m_a.Start (); m_end = m_a.End (); }
				_V* End (void) { m_p = m_start = m_a.End (); m_end = m_a.Start (); }
			};

		CArray () { 
			memset (&m_data.null, 0, sizeof (m_data.null));
			Init (); 
			}
		
		CArray (uint nLength) { 
			Init (); 
			Create (nLength);
			}
		
		~CArray() { Destroy (); }
		
		void Init (void) { 
			m_data.buffer = reinterpret_cast<_T *> (NULL); 
			m_data.length = 0;
			m_data.pos = 0;
			m_data.nMode = 0;
			m_data.bWrap = false;
			memset (&m_data.null, 0, sizeof (_T));
			}

		void Clear (ubyte filler = 0, uint count = 0xffffffff) { 
			if (m_data.buffer) 
				memset (m_data.buffer, filler, sizeof (_T) * ((count < m_data.length) ? count : m_data.length)); 
			}
		
		inline bool IsElement (_T* elem, bool bDiligent = false) {
			if (!m_data.buffer || (elem < m_data.buffer) || (elem >= m_data.buffer + m_data.length))
				return false;	// no buffer or element out of buffer
			if (bDiligent) {
				uint i = static_cast<uint> (reinterpret_cast<ubyte*> (elem) - reinterpret_cast<ubyte*> (m_data.buffer));
				if (i % sizeof (_T))	
					return false;	// elem in the buffer, but not properly aligned
				}
			return true;
			}

#if DBG
		inline int Index (_T* elem) { 
			if (IsElement (elem))
				return static_cast<int> (elem - m_data.buffer); 
			return -1;
			}
#else
		inline uint Index (_T* elem) { return elem - m_data.buffer; }
#endif

#if DBG
		inline _T* Pointer (uint i) { 
			if (!m_data.buffer || (i >= m_data.length))
				return NULL;
			return m_data.buffer + i; 
			}
#else
		inline _T* Pointer (uint i) { return m_data.buffer + i; }
#endif

		void Destroy (void) { 
			if (m_data.buffer) {
				if (!m_data.nMode)
					delete[] m_data.buffer;
				Init ();
				}
			}
			
		_T *Create (uint length) {
			if (m_data.length != length) {
				Destroy ();
				if ((m_data.buffer = new _T [length]))
					m_data.length = length;
				}
			return m_data.buffer;
			}
			
		inline _T* Buffer (uint i = 0) { return m_data.buffer + i; }
		
		void SetBuffer (_T *buffer, int nMode = 0, uint length = 0xffffffff) {
			if (m_data.buffer != buffer) {
				if (!(m_data.buffer = buffer))
					Init ();
				else {
					m_data.length = length;
					m_data.nMode = nMode;
					}
				}
			}
			
		_T* Resize (unsigned int length, bool bCopy = true) {
			if (m_data.nMode == 2)
				return m_data.buffer;
			if (!m_data.buffer)
				return Create (length);
			_T* p = new _T [length];
			if (!p)
				return m_data.buffer;
			if (bCopy)
				memcpy (p, m_data.buffer, ((length > m_data.length) ? m_data.length : length) * sizeof (_T)); 
			m_data.length = length;
			m_data.pos %= length;
			delete[] m_data.buffer;
			return m_data.buffer = p;
			}

		inline uint Length (void) { return m_data.length; }

		inline _T* Current (void) { return m_data.buffer ? m_data.buffer + m_data.pos : NULL; }

		inline size_t Size (void) { return m_data.length * sizeof (_T); }
#if DBG
		inline _T& operator[] (uint i) { 
			if (m_data.buffer && (i < m_data.length))
				return m_data.buffer [i];
			if (i == m_data.length)
				return m_data.null; 
			else
				return m_data.null; 
			}
#else
		inline _T& operator[] (uint i) { return m_data.buffer [i]; }
#endif

		inline _T& operator= (CArray<_T>& source) { return Copy (source); }

		inline _T& operator= (_T* source) { 
			if (m_data.buffer) 
				memcpy (m_data.buffer, source, m_data.length * sizeof (_T)); 
			return m_data.buffer [0];
			}

		_T& Copy (CArray<_T>& source, uint offset = 0) { 
			if (((static_cast<int> (m_data.length)) >= 0) && (static_cast<int> (source.m_data.length) > 0)) {
				if ((m_data.buffer && (m_data.length >= source.m_data.length + offset)) || Resize (source.m_data.length + offset, false)) {
					memcpy (m_data.buffer + offset, source.m_data.buffer, ((m_data.length - offset < source.m_data.length) ? m_data.length - offset : source.m_data.length) * sizeof (_T)); 
					}
				}
			return m_data.buffer [0];
			}

		inline _T& operator+ (CArray<_T>& source) { 
			uint offset = m_data.length;
			if (m_data.buffer) 
				Resize (m_data.length + source.m_data.length);
			return Copy (source, offset);
			}

		inline bool operator== (CArray<_T>& other) { 
			return (m_data.length == other.m_data.length) && !(m_data.length && memcmp (m_data.buffer, other.m_data.buffer)); 
			}

		inline bool operator!= (CArray<_T>& other) { 
			return (m_data.length != other.m_data.length) || (m_data.length && memcmp (m_data.buffer, other.m_data.buffer)); 
			}

		inline _T* Start (void) { return m_data.buffer; }

		inline _T* End (void) { return (m_data.buffer && m_data.length) ? m_data.buffer + m_data.length - 1 : NULL; }

		inline _T* operator++ (void) { 
			if (!m_data.buffer)
				return NULL;
			if (m_data.pos < m_data.length - 1)
				m_data.pos++;
			else if (m_data.bWrap) 
				m_data.pos = 0;
			else
				return NULL;
			return m_data.buffer + m_data.pos;
			}

		inline _T* operator-- (void) { 
			if (!m_data.buffer)
				return NULL;
			if (m_data.pos > 0)
				m_data.pos--;
			else if (m_data.bWrap)
				m_data.pos = m_data.length - 1;
			else
				return NULL;
			return m_data.buffer + m_data.pos;
			}

#if DBG

		inline _T* operator+ (uint i) { 
			if (m_data.buffer && (i < m_data.length))
				return m_data.buffer + i;
			if (i == m_data.length)
				return NULL;
			else
				return  NULL; 
			}

#else

		inline _T* operator+ (uint i) { return m_data.buffer ? m_data.buffer + i : NULL; }

#endif

		inline _T* operator- (uint i) { return m_data.buffer ? m_data.buffer - i : NULL; }

		CArray<_T>& ShareBuffer (CArray<_T>& child) {
			memcpy (&child.m_data, &m_data, sizeof (m_data));
			if (!child.m_data.nMode)
				child.m_data.nMode = 1;
			return child;
			}

		inline bool operator! () { return m_data.buffer == NULL; }

		inline uint Pos (void) { return m_data.pos; }

		inline void Pos (uint pos) { m_data.pos = pos % m_data.length; }

		size_t Read (CFile& cf, uint nCount = 0, uint nOffset = 0) { 
			if (!m_data.buffer)
				return -1;
			if (nOffset >= m_data.length)
				return -1;
			if (!nCount)
				nCount = m_data.length - nOffset;
			else if (nCount > m_data.length - nOffset)
				nCount = m_data.length - nOffset;
			return cf.Read (m_data.buffer + nOffset, sizeof (_T), nCount);
			}

		size_t Write (CFile& cf, uint nCount = 0, uint nOffset = 0) { 
			if (!m_data.buffer)
				return -1;
			if (nOffset >= m_data.length)
				return -1;
			if (!nCount)
				nCount = m_data.length - nOffset;
			else if (nCount > m_data.length - nOffset)
				nCount = m_data.length - nOffset;
			return cf.Write (m_data.buffer + nOffset, sizeof (_T), nCount);
			}

		inline void SetWrap (bool bWrap) { m_data.bWrap = bWrap; }

		inline void SortAscending (int left = 0, int right = 0) { 
			if (m_data.buffer) 
				CQuickSort<_T>::SortAscending (m_data.buffer, left, right ? right : m_data.length - 1); 
				}

		inline void SortDescending (int left = 0, int right = 0) {
			if (m_data.buffer) 
				CQuickSort<_T>::SortDescending (m_data.buffer, left, right ? right : m_data.length - 1);
			}
#ifdef _WIN32
		inline void SortAscending (comparator compare, int left = 0, int right = 0) {
			if (m_data.buffer) 
				CQuickSort<_T>::SortAscending (m_data.buffer, left, right ? right : m_data.length - 1, compare);
			}

		inline void SortDescending (comparator compare, int left = 0, int right = 0) {
			if (m_data.buffer) 
				CQuickSort<_T>::SortDescending (m_data.buffer, left, right ? right : m_data.length - 1, compare);
			}
#endif
	};

//-----------------------------------------------------------------------------

inline int operator- (char* v, CArray<char>& a) { return a.Index (v); }
inline int operator- (ubyte* v, CArray<ubyte>& a) { return a.Index (v); }
inline int operator- (short* v, CArray<short>& a) { return a.Index (v); }
inline int operator- (ushort* v, CArray<ushort>& a) { return a.Index (v); }
inline int operator- (int* v, CArray<int>& a) { return a.Index (v); }
inline int operator- (uint* v, CArray<uint>& a) { return a.Index (v); }

class CCharArray : public CArray<char> {
	public:
		inline char* operator= (const char* source) { 
			uint l = strlen (source) + 1;
			if ((l > this->m_data.length) && !this->Resize (this->m_data.length + l))
				return NULL;
			memcpy (this->m_data.buffer, source, l);
			return this->m_data.buffer;
		}
};

class CByteArray : public CArray<ubyte> {};
class CShortArray : public CArray<short> {};
class CUShortArray : public CArray<ushort> {};
class CIntArray : public CArray<int> {};
class CUIntArray : public CArray<uint> {};
class CFloatArray : public CArray<float> {};

//-----------------------------------------------------------------------------

template < class _T, uint length > 
class CStaticArray : public CArray < _T > {

	template < class _U, uint _length > 
	class CStaticArrayData {
		public:
			_U		buffer [_length];
			};

	protected:
		CStaticArrayData< _T, length > m_data;

	public:
		CStaticArray () { Create (length); }

		_T *Create (uint _length) { 
			this->SetBuffer (m_data.buffer, 2, _length); 
			return m_data.buffer;
			}
		void Destroy (void) { }
	};

//-----------------------------------------------------------------------------


#endif //_CARRAY_H
