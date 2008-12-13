#ifndef _CQUICKSORT_H
#define _CQUICKSORT_H

#include "pstypes.h"

//-----------------------------------------------------------------------------

template < class _T > 
class CQuickSort {
	public:
		typedef int (*comparator) (_T*, _T*);

		void SortAscending (_T* buffer, uint left, uint right);
		void SortDescending (_T* buffer, uint left, uint right);
		void SortAscending (_T* buffer, uint left, uint right, comparator compare);
		void SortDescending (_T* buffer, uint left, uint right, comparator compare);
		inline void Swap (_T* left, _T* right);
	};

//-----------------------------------------------------------------------------

#include "cquicksort.cpp"

#endif //_CQUICKSORT_H
