#ifndef _CSTACK_H
#define _CSTACK_H

#include "carray.h"

//-----------------------------------------------------------------------------

template < class _T > 
class CStack : public CArray< _T > {
	protected:
		uint	m_tos;
		uint	m_growth;

	public:
		CStack () { Init (); }
		CStack (uint nLength) { 
			Init (); 
			Create (nLength);
			}
		~CStack() { Destroy (); }

		inline void Reset (void) { m_tos = 0; }

		inline void Init (void) { 
			m_growth = 0;
			Reset ();
			CArray<_T>::Init ();
			}

		inline bool Grow (const uint i = 1) {
			if ((m_tos + i > this->m_data.length) && (!(m_growth && this->Resize (this->m_data.length + m_growth)))) {
#if DBG
				ArrayError ("invalid stack operation\n");
#endif
				return false;
				}
			m_tos += i;
			return true;
			}

		inline bool Push (const _T elem) { 
			if ((m_tos >= this->m_data.length) && (!(m_growth && this->Resize (this->m_data.length + m_growth)))) {
#if DBG
				ArrayError ("invalid stack operation\n");
#endif
				return false;
				}
			this->m_data.buffer [m_tos++] = elem;
			return true;
			}
	
		inline _T* Top (void) { return (this->m_data.buffer && m_tos) ? this->m_data.buffer + m_tos - 1 : NULL; }

		inline uint ToS (void) { return m_tos; }

		inline _T& Pop (void) {
			if (m_tos)
				m_tos--;
			return this->m_data.buffer [m_tos];
			}

		inline void Shrink (uint i = 1) {
			if (i >= m_tos)
				m_tos = 0;
			else
				m_tos -= i;
			}

		inline bool Delete (uint i) {
			if (i >= m_tos) {
#if DBG
				ArrayError ("invalid stack access\n");
#endif
				return false;
				}
			if (i < --m_tos)
				this->m_data.buffer [i] = this->m_data.buffer [m_tos];
			return true;
			}

		inline _T& Pull (_T& elem, uint i) {
			if (i < m_tos) {
				elem = this->m_data.buffer [i];
				Delete (i);
				}
			return elem;
			}

		inline _T Pull (uint i) {
			_T	v;
			return Pull (v, i);
			}

		inline void Destroy (void) { 
			CArray<_T>::Destroy ();
			m_tos = 0;
			}

		inline _T *Create (uint length) {
			Destroy ();
			return CArray<_T>::Create (length);
			}

		inline uint Growth (void) { return m_growth; }

		inline void SetGrowth (uint growth) { m_growth = growth; }
	};

//-----------------------------------------------------------------------------

#endif //_CSTACK_H
