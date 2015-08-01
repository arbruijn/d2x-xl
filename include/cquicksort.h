#ifndef _CQUICKSORT_H
#define _CQUICKSORT_H

#include "pstypes.h"

//-----------------------------------------------------------------------------

template < class _T > 
class CQuickSort {
	public:
		typedef int32_t (*comparator) (const _T*, const _T*);

		void SortAscending (_T* buffer, int32_t left, int32_t right);
		void SortDescending (_T* buffer, int32_t left, int32_t right);
		void SortAscending (_T* buffer, int32_t left, int32_t right, comparator compare);
		void SortDescending (_T* buffer, int32_t left, int32_t right, comparator compare);
		inline void Swap (_T* left, _T* right);
		int32_t BinSearch (_T* buffer, int32_t l, int32_t r, _T key);
	};

//-----------------------------------------------------------------------------

#include "cquicksort.cpp"

#endif //_CQUICKSORT_H
