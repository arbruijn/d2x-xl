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

extern int32_t Inferno_verbose;

class CConfigManager {
	private:
		CStack<char*>	m_properties;
		CFile				m_cf;
		char				m_filename [FILENAME_LEN];
		char				m_null [1];

		inline int32_t Count (void) { return (m_properties.Buffer () == NULL) ? 0 : int32_t (m_properties.ToS ()); }

	public:
		CConfigManager () { Init (); }
		~CConfigManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		char* Filename (int32_t bDebug = 0);
		void Load (int32_t argC, char** argV);
		void Load (char* filename);
		int32_t Parse (CFile* pFile = NULL);
		void PrintLog (void);
		int32_t Find (const char* s);
		int32_t Int (int32_t t, int32_t nDefault);
		int32_t Int (const char* szArg, int32_t nDefault);
		const char* Text (const char* szArg, const char* pszDefault = "");
		inline char* Property (int32_t i) { return (--i < Count ()) ? m_properties [i] : m_null; }
		inline char* operator[] (int32_t i) { return Property (i); }
};

extern CConfigManager appConfig;

static inline int32_t FindArg (const char * s) { return appConfig.Find (s); }
static inline int32_t NumArg (int32_t t, int32_t nDefault) { return appConfig.Int (t, nDefault); }

#endif //_ARGS_H
