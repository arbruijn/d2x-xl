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
//#pragma omp critical
			m_tos += i;
			return true;
			}

		inline bool Push (const _T elem) { 
			if (!Grow ())
				return false;
//#pragma omp critical
			this->m_data.buffer [m_tos - 1] = elem;
			return true;
			}
	
		inline void Shrink (uint i = 1) {
//#pragma omp critical
			if (i >= m_tos)
				m_tos = 0;
			else
				m_tos -= i;
			}

		inline _T& Pop (void) {
//#pragma omp critical
			Shrink ();
			return this->m_data.buffer [m_tos];
			}

		inline void Truncate (uint i = 1) {
			if (i < m_tos)
				m_tos = i;
			}

		inline uint Find (_T& elem) {
			for (uint i = 0; i < m_tos; i++)
				if (this->m_data.buffer [i] == elem)
					return i;
			return m_tos;
			}

		inline uint ToS (void) { return m_tos; }

		inline _T* Top (void) { return (this->m_data.buffer && m_tos) ? this->m_data.buffer + m_tos - 1 : NULL; }

		inline bool Delete (uint i) {
			if (i >= m_tos) {
#if DBG
				ArrayError ("invalid stack access\n");
#endif
				return false;
				}
//#pragma omp critical
			if (i < --m_tos)
				memcpy (this->m_data.buffer + i, this->m_data.buffer + i + 1, sizeof (_T) * (m_tos - i));
			return true;
			}

		inline bool DeleteElement (_T& elem) { return Delete (Find (elem));	}

		inline _T& Pull (_T& elem, uint i) {
//#pragma omp critical
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

		inline void SortAscending (int left = 0, int right = -1) { 
			if (this->m_data.buffer)
				CQuickSort<_T>::SortAscending (this->m_data.buffer, left, (right >= 0) ? right : m_tos - 1); 
				}

		inline void SortDescending (int left = 0, int right = -1) {
			if (this->m_data.buffer)
				CQuickSort<_T>::SortDescending (this->m_data.buffer, left, (right >= 0) ? right : m_tos - 1);
			}
#ifdef _WIN32
		inline void SortAscending (comparator compare, int left = 0, int right = -1) {
			if (this->m_data.buffer)
				CQuickSort<_T>::SortAscending (this->m_data.buffer, left, (right >= 0) ? right : m_tos - 1, compare);
			}

		inline void SortDescending (comparator compare, int left = 0, int right = -1) {
			if (this->m_data.buffer)
				CQuickSort<_T>::SortDescending (this->m_data.buffer, left, (right >= 0) ? right : m_tos - 1, compare);
			}
#endif

		inline int BinSearch (_T key, int left = 0, int right = -1) {
			return this->m_data.buffer ? CQuickSort<_T>::BinSearch (this->m_data.buffer, left, (right >= 0) ? right : m_tos - 1, key) : -1;
			}

	};

//-----------------------------------------------------------------------------

#endif //_CSTACK_H
