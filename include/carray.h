#ifndef _CARRAY_H
#define _CARRAY_H

//-----------------------------------------------------------------------------

template < class _T > class CArray {
	private:
		_T					*m_buffer;
		_T					*m_null;
		unsigned int	m_size;
		bool				m_bExternal;
		bool				m_bChild;
	public:
		CArray () { Init (); }
		~CArray() { Destroy (); }
		inline void Init (void) { 
			m_buffer = m_null = (_T *) NULL; 
			m_bExternal = m_bChild = false;
			}
		inline void Clear (void) { if (m_buffer) memset (m_buffer, 0, m_size); }
		inline void Destroy (void) { 
			if (m_buffer) {
#if 0
				if (m_bExternal)
					m_buffer = NULL;
				else 
#endif
				if (!m_bChild)
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
		inline void SetBuffer (_T *buffer, bool bChild = false, unsigned int size = 0xffffffff) {
			if (m_buffer != buffer) {
				m_buffer = buffer;
				m_size = size;
				m_bChild = bChild;
				if (buffer)
					m_bExternal = true;
				else
					m_bExternal = false;
				}
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