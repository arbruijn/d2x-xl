#ifndef _CARRAY_H
#define _CARRAY_H

//-----------------------------------------------------------------------------

template < class _T > class CArray {
	private:
		_T					*m_buffer;
		_T					*m_null;
		unsigned int	m_size;
	public:
		CArray () { Init (); }
		~CArray() { Destroy (); }
		inline void Init (void) { m_buffer = m_null = (_T *) NULL; }
		inline void Clear (void) { if (m_buffer) memset (m_buffer, 0, m_size); }
		inline void Destroy (void) { 
			if (m_buffer) {
				delete[] m_buffer;
				Init ();
				}
			}
		inline _T *Create (unsigned int size) {
			Destroy ();
			m_size = (m_buffer = new _T [size]) ? size : 0;
			return m_buffer;
			}
		inline _T* Buffer (void) { return m_buffer; }
		inline void SetBuffer (_T *buffer, unsigned int size = 0xffffffff) {
			m_buffer = buffer;
			m_size = size;
			}
		inline unsigned int Size (void) { return m_size; }
#if DBG
		inline _T& operator[] (unsigned int i) { return (i < m_size) ? m_buffer [i] : m_null [0]; }
#else
		inline _T& operator[] (unsigned int i) { return m_buffer [i]; }
#endif
	};

//-----------------------------------------------------------------------------

#endif //_CARRAY_H