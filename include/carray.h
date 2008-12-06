#ifndef _CARRAY_H
#define _CARRAY_H

#include "pstypes.h"

//-----------------------------------------------------------------------------

template < class _T > class CArray {

	template < class _T > class CArrayData {
		public:
			_T		*buffer;
			_T		null;
			uint	size;
			bool	bExternal;
			bool	bChild;
			};

	protected:
		CArrayData<_T>	m_data;

	public:
		CArray () { Init (); }
		
		~CArray() { Destroy (); }
		
		inline void Init (void) { 
			m_data.buffer = reinterpret_cast<_T *> (NULL); 
			m_data.bExternal = m_data.bChild = false;
			memset (&m_data.null, 0, sizeof (_T));
			}

		inline void Clear (ubyte filler = 0) { if (m_data.buffer) memset (m_data.buffer, filler, m_data.size); }
		
#ifdef _DEBUG
		inline int Index (_T* elem) { 
			if (!m_data.buffer || (elem < m_data.buffer) || (elem >= m_data.buffer + m_data.size))
				return -1;	// no buffer or element out of buffer
			uint i = static_cast<uint> (reinterpret_cast<ubyte*> (elem) - reinterpret_cast<ubyte*> (m_data.buffer));
			if (i % sizeof (_T))	
				return -1;	// elem in the buffer, but not properly aligned
			return static_cast<int> (elem - m_data.buffer); 
			}
#else
		inline uint Index (_T* elem) { return elem - m_data.buffer; }
#endif

#ifdef _DEBUG
		inline _T* Pointer (uint i) { 
			if (!m_data.buffer || (i >= m_data.size))
				return NULL;
			return m_data.buffer + i; 
			}
#else
		inline _T* Pointer (uint i) { return m_data.buffer + i; }
#endif

		inline void Destroy (void) { 
			if (m_data.buffer) {
#if 0
				if (m_data.bExternal)
					m_data.buffer = NULL;
				else 
#endif
				if (!m_data.bChild)
					delete[] m_data.buffer;
				Init ();
				}
			}
			
		inline _T *Create (uint size) {
			if (m_data.size != size) {
				Destroy ();
				m_data.size = (m_data.buffer = new _T [size]) ? size : 0;
				}
			return m_data.buffer;
			}
			
		inline _T* Buffer (void) { return m_data.buffer; }
		
		inline void SetBuffer (_T *buffer, bool bChild = false, uint size = 0xffffffff) {
			if (m_data.buffer != buffer) {
				m_data.buffer = buffer;
				m_data.size = size;
				m_data.bChild = bChild;
				if (buffer)
					m_data.bExternal = true;
				else
					m_data.bExternal = false;
				}
			}
			
		inline _T* Resize (unsigned int size) {
			if (!m_data.buffer)
				return Create (size);
			_T* p = new _T [size];
			if (!p)
				return m_data.buffer;
			memcpy (p, m_data.buffer, ((size < m_data.size) ? m_data.size : size) * sizeof (_T)); 
			delete[] m_data.buffer;
			return m_data.buffer = p;
			}
		inline uint Size (void) { return m_data.size; }
#if DBG
		inline _T& operator[] (uint i) { return (i < m_data.size) ? m_data.buffer [i] : m_data.null; }
#else
		inline _T& operator[] (uint i) { return m_data.buffer [i]; }
#endif

		inline _T& operator= (CArray<_T>& source) { return Copy (source); }

		inline _T& operator= (_T* source) { 
			if (m_data.buffer)
				memcpy (m_data.buffer, source, m_data.size * sizeof (_T)); 
			return m_data.buffer [0];
			}

		inline _T& Copy (CArray<_T>& source, uint offset = 0) { 
			if (source.m_data.buffer) {
				if (!m_data.buffer)
					Create (source.m_data.size);
				memcpy (m_data.buffer + offset, source.m_data.buffer, ((m_data.size < source.m_data.size) ? m_data.size : source.m_data.size) * sizeof (_T)); 
				}
			return m_data.buffer [0];
			}

		inline _T& operator+ (CArray<_T>& source) { 
			uint offset = m_data.size;
			if (m_data.buffer) 
				Resize (m_data.size + source.m_data.size);
			return Copy (source, offset);
			}

		inline _T* operator+ (uint i) { return m_data.buffer ? m_data.buffer + i : NULL; }

		CArray<_T>& Clone (CArray<_T>& clone) {
			clone.m_data = m_data;
			clone.m_data.bChild = true;
			return clone;
			}
	};

//-----------------------------------------------------------------------------

#endif //_CARRAY_H
