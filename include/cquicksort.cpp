#ifdef _CQUICKSORT_H

//-----------------------------------------------------------------------------

template <typename _T>
inline void CQuickSort< _T >::Swap (_T* left, _T* right)
{
_T h = *left;
*left = *right;
*right = h;
}

//-----------------------------------------------------------------------------

template <typename _T>
void CQuickSort< _T >::SortAscending (_T* buffer, uint left, uint right) 
{
	uint	l = left,
			r = right;
	_T		m = buffer [(l + r) / 2];

do {
	while (buffer [l] < m)
		l++;
	while (buffer [r] > m)
		r--;
	if (l <= r) {
		if (l < r)
			Swap (buffer + l, buffer + r);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortAscending (buffer, l, right);
if (left < r)
	SortAscending (buffer, left, r);
};

//-----------------------------------------------------------------------------

template <typename _T>
void CQuickSort< _T >::SortDescending (_T* buffer, uint left, uint right) 
{
	uint	l = left,
			r = right;
	_T		m = buffer [(l + r) / 2];

do {
	while (buffer [l] > m)
		l++;
	while (buffer [r] < m)
		r--;
	if (l <= r) {
		if (l < r)
			Swap (buffer + l, buffer + r);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortDescending (buffer, l, right);
if (left < r)
	SortDescending (buffer, left, r);
};

//-----------------------------------------------------------------------------

template <typename _T>
void CQuickSort< _T >::SortAscending (_T* buffer, uint left, uint right, comparator compare) 
{
	uint	l = left,
			r = right;
	_T		m = buffer [(l + r) / 2];

do {
	while (compare (buffer + l, &m) < 0)
		l++;
	while (compare (buffer + r, &m) > 0)
		r--;
	if (l <= r) {
		if (l < r)
			Swap (buffer + l, buffer + r);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortAscending (buffer, l, right, compare);
if (left < r)
	SortAscending (buffer, left, r, compare);
};

//-----------------------------------------------------------------------------

template <typename _T>
void CQuickSort< _T >::SortDescending (_T* buffer, uint left, uint right, comparator compare) 
{
	uint	l = left,
			r = right;
	_T		m = buffer [(l + r) / 2];

do {
	while (compare (buffer + l, &m) > 0)
		l++;
	while (compare (buffer + r, &m) < 0)
		r--;
	if (l <= r) {
		if (l < r)
			Swap (buffer + l, buffer + r);
		l++;
		r--;
		}
	} while (l <= r);
if (l < right)
	SortDescending (buffer, l, right, compare);
if (left < r)
	SortDescending (buffer, left, r, compare);
};

//-----------------------------------------------------------------------------

#endif //_CQUICKSORT_H
