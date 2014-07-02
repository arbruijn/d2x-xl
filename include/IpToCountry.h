#ifndef __IP2Country_h
#define __IP2Country_h

#include "carray.h"

class CIpToCountry {
	public:
		uint32_t	m_min;
		uint32_t	m_max;
		uint32_t	m_range;
		char	m_country [4];

	public:
		CIpToCountry (const uint32_t minIP = 0, const uint32_t maxIP = 0, const char* country = "")
			: m_min (minIP), m_max (maxIP), m_range (m_max - m_min + 1)
			{ strncpy (m_country, country, sizeof (m_country)); }

		inline uint32_t Min (void) { return m_min; }
		inline uint32_t Max (void) { return m_max; }
		inline uint32_t Range (void) { return m_range; }

		inline CIpToCountry& operator= (const CIpToCountry& other) { 
			m_min = other.m_min, m_max = other.m_max, m_range = other.m_range;
			strncpy (m_country, other.m_country, sizeof (m_country));
			return *this;
			}

		inline bool operator< (const CIpToCountry& other) { return (m_min < other.m_min) || ((m_min == other.m_min) && (m_range < other.m_range)); }
		inline bool operator> (const CIpToCountry& other) { return (m_min > other.m_min) || ((m_min == other.m_min) && (m_range > other.m_range)); }
		inline bool operator<= (const CIpToCountry& other) { return m_max <= other.m_min; }
		inline bool operator>= (const CIpToCountry& other) { return m_min >= other.m_max; }

		inline bool operator< (const uint32_t ip) { return m_max < ip; }
		inline bool operator> (const uint32_t ip) { return m_min > ip; }
		inline bool operator!= (const uint32_t ip) { return (ip < m_min) || (ip > m_max); }
		inline bool operator== (const uint32_t ip) { return (ip >= m_min) && (ip <= m_max); }
	};

extern CStack<CIpToCountry> ipToCountry;

int32_t LoadIpToCountry (void);
const char* CountryFromIP (uint32_t ip);

#endif //__IP2Country_h
