/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#ifndef _ARGS_H
#define _ARGS_H

#include "cstack.h"

extern int Inferno_verbose;

class CConfigManager {
	private:
		CStack<char*>	m_properties;
		CFile				m_cf;
		char				m_filename [FILENAME_LEN];
		char				m_null [1];

		inline int Count (void) { return (m_properties.Buffer () == NULL) ? 0 : int (m_properties.ToS ()); }

	public:
		CConfigManager () { Init (); }
		~CConfigManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		char* Filename (int bDebug = 0);
		void Load (int argC, char** argV);
		void Load (char* filename);
		int Parse (CFile* cfP = NULL);
		void PrintLog (void);
		int Find (const char* s);
		int Int (int t, int nDefault);
		int Int (const char* szArg, int nDefault);
		const char* Text (const char* szArg, const char* pszDefault = "");
		inline char* Property (int i) { return (--i < Count ()) ? m_properties [i] : m_null; }
		inline char* operator[] (int i) { return Property (i); }
};

extern CConfigManager appConfig;

static inline int FindArg (const char * s) { return appConfig.Find (s); }
static inline int NumArg (int t, int nDefault) { return appConfig.Int (t, nDefault); }

#endif //_ARGS_H
