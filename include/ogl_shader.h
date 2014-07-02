#ifndef _OGL_SHADER_H
#define _OGL_SHADER_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "glew.h"
#include "vecmat.h"
#include "ogl_lib.h"
#include "oglmatrix.h"


typedef struct tShaderData {
	GLhandleARB		shaders [2];
	GLhandleARB		program;
	int32_t*				refP;
} tShaderData;

#if defined(__macosx__) && defined(__LP64__)
#	define intptr_t	int64_t
#else
#	define intptr_t	int32_t
#endif

class CShaderManager {
	private:
		CStack<tShaderData>		m_shaders;
		int32_t							m_nCurrent;
		bool							m_bSuspendable;

	public:
		CShaderManager ();
		~CShaderManager ();
		void Init (void);
		void Destroy (bool bAll = true);
		void Setup (void);
		intptr_t Deploy (int32_t nShader, bool bSuspendable = false);
		int32_t Alloc (int32_t& nShader);
		char* Load (const char* filename);
		int32_t Create (int32_t nShader);
		int32_t Compile (int32_t nShader, const char* pszFragShader, const char* pszVertShader, bool bFromFile = 0);
		int32_t Link (int32_t nShader);
		int32_t Build (int32_t& nShader, const char* pszFragShader, const char* pszVertShader, bool bFromFile = 0);
		void Reset (int32_t nShader);
		void Delete (int32_t nShader);
		inline int32_t Current (void) { return m_nCurrent; }
		inline bool IsCurrent (int32_t nShader) { return m_nCurrent == nShader; }
		inline bool Rebuild (GLhandleARB& nShaderProg) {
			if (0 < intptr_t (nShaderProg))
				return true;
			nShaderProg = GLhandleARB (-(intptr_t (nShaderProg)));
			return false;
			}
		inline GLint Addr (const char* name) { return (m_nCurrent < 0) ? -1 : glGetUniformLocation (m_shaders [m_nCurrent].program, name); }

		inline bool Active (void) { return (m_nCurrent >= 0) && !m_bSuspendable; }

		inline bool Set (const char* name, int32_t var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform1i (addr, (GLint) var);
			return true;
			}

		inline bool Set (const char* name, float var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform1f (addr, (GLfloat) var);
			return true;
			}

		inline bool Set (const char* name, vec2& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform2fv (addr, 1, (GLfloat*) var);
			return true;
			}

		inline bool Set (const char* name, vec3& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform3fv (addr, 1, (GLfloat*) var);
			return true;
			}

		inline bool Set (const char* name, vec4& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform4fv (addr, 1, (GLfloat*) var);
			return true;
			}

		inline bool Set (const char* name, float x, float y) { 
			vec2 v = {x, y};
			return Set (name, v);
			}

		inline bool Set (const char* name, float x, float y, float z) { 
			vec3 v = {x, y, z};
			return Set (name, v);
			}

		inline bool Set (const char* name, float x, float y, float z, float w) { 
			vec4 v = {x, y, z, w};
			return Set (name, v);
			}

		inline bool Set (const char* name, CFloatVector3& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform3fv (addr, 1, (GLfloat*) var.v.vec);
			return true;
			}

		inline bool Set (const char* name, CFloatVector& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniform3fv (addr, 1, (GLfloat*) var.v.vec);
			return true;
			}

		inline bool Set (const char* name, COGLMatrix& var) { 
			GLint addr = Addr (name);
			if (addr < 0)
				return false;
			glUniformMatrix4fv (addr, 1, (GLboolean) 0, (GLfloat*) var.ToFloat ());
			return true;
			}

	private:
		void Dispose (GLhandleARB& shaderProg);
		void PrintLog (GLhandleARB handle, int32_t bProgram);
};

extern CShaderManager shaderManager;

#endif
