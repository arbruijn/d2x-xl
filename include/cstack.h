#ifndef _CSTACK_H
#define _CSTACK_H

#include "carray.h"

//-----------------------------------------------------------------------------

template < class _T > class CStack : public CArray<_T> {
	protected:
		uint	m_tos;

	public:
		CStack () { Init (); }
		~CStack() { Destroy (); }

		inline void Init (void) { 
			m_tos = false;
			CArray<_T>::Init ();
			}

		inline bool Push (_T elem) { 
			if (m_tos <= m_data.length) 
				return false;
			m_data.buffer [m_tos++] = elem;
			return true;
			}

		inline _T Pop (void) {
			if (m_tos)
				m_tos--;
			return m_data.buffer [m_tos];
			}

		inline void Delete (uint i) {
			if (i < m_tos)
				if (i < --m_tos)
					m_data.buffer [i] = m_data.buffer [m_tos];
			}

		inline void Destroy (void) { 
			CArray<_T>::Destroy ();
			Init ();
			}

		inline _T *Create (uint length) {
			Destroy ();
			return CArray<_T>::Create (length);
			}
	};

//-----------------------------------------------------------------------------

#endif //_ARRAY_H