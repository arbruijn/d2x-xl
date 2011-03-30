#ifndef __IP2Country_h
#define __IP2Country_h

#include "carray.h"

class CIpToCountry {
	public:
		int	m_min;
		int	m_max;
		char	m_country [4];

	public:
		CIpToCountry (const int minIP = 0, const int maxIP = 0, const char* country = "")
			: m_min (minIP), m_max (maxIP)
			{ strncpy (m_country, country, sizeof (m_country)); }

		inline int Min (void) { return m_min; }
		inline int Max (void) { return m_max; }

		inline CIpToCountry& operator= (const CIpToCountry& other) { 
			m_min = other.m_min, m_max = other.m_max;
			strncpy (m_country, other.m_country, sizeof (m_country));
			return *this;
			}

		inline bool operator< (const CIpToCountry& other) { return other.m_min < m_max; }
		inline bool operator> (const CIpToCountry& other) { return other.m_max < m_min; }
		inline bool operator<= (const CIpToCountry& other) { return other.m_min <= m_max; }
		inline bool operator>= (const CIpToCountry& other) { return other.m_max <= m_min; }
		inline bool operator== (const int ip) { return (ip >= m_min) && (ip <= m_max); }
	};

extern CStack<CIpToCountry> ipToCountry;

int LoadIpToCountry (void);
char* CountryFromIP (int ip);

#endif __IP2Country_h
