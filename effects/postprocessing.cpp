/* Image post processing (fullscreen effects)
	Dietfrid Mali
	22.3.2011
*/

#include "postprocessing.h"

CPostProcessManager postProcessManager;

//------------------------------------------------------------------------------

void CPostProcessManager::Destroy (void) 
{
while (m_effects) {
	CPostEffect* e = m_effects;
	m_effects = m_effects->Next ();
	delete e;
	}
m_nEffects = 0;
}

//------------------------------------------------------------------------------

void CPostProcessManager::Add (CPostEffect* e) 
{
e->Link (NULL, m_effects);
if (m_effects)
	m_effects->Link (e, m_effects->Next ());
m_effects = e;
}

//------------------------------------------------------------------------------

void CPostProcessManager::Render (void)
{
}

//------------------------------------------------------------------------------

void CPostProcessManager::Update (void)
{
for (CPostEffect* e = m_effects; e; ) {
	CPostEffect* h = e;
	e = e->Next ();
	if (h->Terminate ())
		h->Unlink ();
	else
		h->Update ();
	}
for (CPostEffect* e = m_effects; e; )
	e->Render ();
}

//------------------------------------------------------------------------------

