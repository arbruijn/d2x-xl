/* Console */

#ifndef _CVAR_H
#define _CVAR_H

#include "pstypes.h"

#ifndef TRACE
#if DBG
#	define TRACE	1
#else
#	define TRACE	0
#endif
#endif

/* CVar stuff */
class CCvar {
	private:
		static CCvar*	m_list;

	public:
		const char*	m_name;
		char*			m_text;
		double		m_value;
		CCvar*		m_next;

	public:
		CCvar (void) { Init (); }
		~CCvar (void) { Destroy (); }
		void Init (void);
		void Destroy (void);

		static CCvar* Create (void);
		static CCvar* Register (const char* name, char* value);
		static CCvar* Register (const char* name, double value);
		static void Set (const char* name, char* value);
		static double Value (const char* name);
		static char* Text (const char* name);
		inline double Value (void) { return m_value; }
		inline char* Text (void) { return m_text; }

	private:
		static CCvar* Find (const char* name);
		void Set (char* value);
};

extern CCvar	m_list;

/* Console CVars */
/* How discriminating we are about which messages are displayed */
extern CCvar con_threshold;

#endif /* _CVAR_H */

