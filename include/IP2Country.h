#ifndef __IP2Country_h
#define __IP2Country_h

#include "carray.h"

class CIP2Country {
	public:
		int	m_min;
		int	m_max;
		char	m_country [4];

	public:
		CIP2Country (const int minIP = 0, const int maxIP = 0, const char* country = "")
			: m_min (minIP), m_max (maxIP)
			{ strncpy (m_country, country, sizeof (m_country)); }

		inline int Min (void) { return m_min; }
		inline int Max (void) { return m_max; }

		inline CIP2Country& operator= (const CIP2Country& other) { 
			m_min = other.m_min, m_max = other.m_max;
			strncpy (m_country, other.m_country, sizeof (m_country));
			return *this;
			}

		inline bool operator< (const CIP2Country& other) { return other.m_min < m_max; }
		inline bool operator> (const CIP2Country& other) { return other.m_max < m_min; }
		inline bool operator<= (const CIP2Country& other) { return other.m_min <= m_max; }
		inline bool operator>= (const CIP2Country& other) { return other.m_max <= m_min; }
		inline bool operator== (const int ip) { return (ip >= m_min) && (ip <= m_max); }
	};

extern CStack<CIP2Country> ip2country;

int LoadIP2Country (void);

#endif __IP2Country_h
