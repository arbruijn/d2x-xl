/* Console */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_ 1

#include "pstypes.h"

#ifndef TRACE
#if DBG
#	define TRACE	1
#else
#	define TRACE	0
#endif
#endif

/* Priority levels */
#define CON_CRITICAL -2
#define CON_URGENT   -1
#define CON_NORMAL    0
#define CON_VERBOSE   1
#define CONDBG     2

int  con_init(void);
void con_resize(void);
void _CDECL_ con_printf(int level, const char *fmt, ...);

void con_show(void);
void con_draw(void);
void con_update(void);
int  con_events(int key);

/* CVar stuff */
typedef struct cvar_s {
	const char *name;
	char *string;
	dboolean archive;
	double value;
	struct cvar_s *next;
} cvar_t;

extern cvar_t *cvar_vars;

/* Register a CVar with the name and string and optionally archive elements set */
void cvar_registervariable (cvar_t *cvar);

/* Equivalent to typing <var_name> <value> at the console */
void cvar_set (const char *cvar_name, char *value);

/* Get a CVar's value */
double cvar (const char *cvar_name);

/* Console CVars */
/* How discriminating we are about which messages are displayed */
extern cvar_t con_threshold;

#endif /* _CONSOLE_H_ */

