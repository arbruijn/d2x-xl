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

extern int Inferno_verbose;

class CArgManager {
	private:
		CStack<char*>	m_argList;
		CFile				m_cf;
		char				m_filename [FILENAME_LEN];

		inline int ArgCount (void) { return (m_argList.Buffer () == NULL) ? NULL : int (m_argList.ToS ()); }

	public:
		CArgManager () { Init (); }
		~CArgManager () { Destroy (); }
		void Init (void);
		void Destroy (void);
		char* Filename (int bDebug = 0);
		void Load (int argC, char** argV);
		void Load (char* filename);
		int Parse (CFile* cfP = NULL);
		void PrintLog (void);
		int Find (const char* s);
		int Value (int t, int nDefault);
		inline char* Arg (int i) { return (i < ArgCount ()) ? m_argList [i] : ""; }
		inline char* operator[] (int i) { return Arg (i); }
};

extern CArgManager appArgs;

static inline int FindArg (const char * s) { return appArgs.Find (s); }
static inline int NumArg (int t, int nDefault) { return appArgs.Value (t, nDefault); }

#endif //_ARGS_H
