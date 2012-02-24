// DialHeap.cpp
// Heap management for Dijkstra Address Calculation Sort shortest path determination

#include "dialheap.h"

#define FAST_RESET 1

//-----------------------------------------------------------------------------

bool CDialHeap::Create (short nNodes)
{
Destroy ();
m_nNodes = nNodes;
if (!(m_index.Create (65536) && m_dirtyIndex.Create (65536) && m_cost.Create (nNodes) && m_dirtyCost.Create (nNodes) && m_links.Create (nNodes) && m_pred.Create (nNodes) && m_edge.Create (nNodes)))
	return false;
m_index.Clear (0xFF);
m_cost.Clear (0xFF);
return true;
}

//-----------------------------------------------------------------------------

void CDialHeap::Destroy (void)
{
m_index.Destroy ();
m_dirtyIndex.Destroy ();
m_cost.Destroy ();
m_dirtyCost.Destroy ();
m_links.Destroy ();
m_pred.Destroy ();
m_edge.Destroy ();
m_route.Destroy ();
}

//-----------------------------------------------------------------------------

static int bFastReset = 1;

void CDialHeap::Reset (void)
{
#if FAST_RESET
#	if 0 //DBG
for (uint i = 0, j = m_dirtyIndex.ToS (); i < j; i++)
	m_index [m_dirtyIndex [i]] = -1;
for (uint i = 0, j = m_dirtyCost.ToS (); i < j; i++)
	m_cost [m_dirtyCost [i]] = 0xFFFFFFFF;
#	else
short* indexP = m_index.Buffer ();
ushort* dirtyIndexP = m_dirtyIndex.Buffer ();
uint i;
for (i = m_dirtyIndex.ToS (); i; i--, dirtyIndexP++)
	indexP [*dirtyIndexP] = -1;

uint* costP = m_cost.Buffer ();
uint* dirtyCostP = m_dirtyCost.Buffer ();
for (i = m_dirtyCost.ToS (); i; i--, dirtyCostP++)
	costP [*dirtyCostP] = 0xFFFFFFFF;
#	endif
m_dirtyIndex.Reset ();
m_dirtyCost.Reset ();
#else
m_index.Clear (0xFF);
#endif
m_cost.Clear (0xFF);
m_nIndex = 0;
}

//-----------------------------------------------------------------------------

void CDialHeap::Setup (short nNode)
{
Reset ();
Push (nNode, -1, -1, 0);
}

//-----------------------------------------------------------------------------

bool CDialHeap::Push (short nNode, short nPredNode, short nEdge, uint nNewCost)
{
	uint nOldCost = m_cost [nNode] & ~0x80000000;

if (nNewCost >= nOldCost)
	return false;

#if DBG
if (!nNewCost)
	nNewCost = nNewCost;
#endif

	ushort nIndex = ushort (nNewCost & 0xFFFF);

if (nOldCost < 0x7FFFFFFF) {	// node already in heap with higher cost, so unlink
	int h = ushort (nOldCost & 0xFFFF);
	for (int i = m_index [h], j = -1; i >= 0; j = i, i = m_links [i]) {
		if (i == nNode) {
			if (j < 0)
				m_index [h] = m_links [i];
			else
				m_links [j] = m_links [i];
			break;
			}
		}
	}
#if FAST_RESET
else
	m_dirtyCost.Push (nNode);
if (0 > m_index [nIndex])
	m_dirtyIndex.Push (nIndex);
#endif
m_links [nNode] = m_index [nIndex];
m_cost [nNode] = nNewCost;
m_index [nIndex] = nNode;
m_pred [nNode] = nPredNode;
m_edge [nNode] = nEdge;
return true;
}

//-----------------------------------------------------------------------------

short CDialHeap::Pop (uint& nCost)
{
	short	nNode;

for (int i = 65536; i; i--) {
	if (0 <= (nNode = m_index [m_nIndex])) {
		m_index [m_nIndex] = m_links [nNode];
		nCost = m_cost [nNode];
		m_cost [nNode] |= 0x80000000;
		return nNode;
		}
	m_nIndex++;
	}
return -1;
}

//-----------------------------------------------------------------------------

short CDialHeap::RouteLength (short nNode)
{
	short	h = nNode, i = 0;

while (0 <= (h = m_pred [h]))
	i++;
return i + 1;
}

//-----------------------------------------------------------------------------

short CDialHeap::BuildRoute (short nNode, int bReverse, tPathNode* route)
{
	short	h = RouteLength (nNode);

if (!route) {
	if (!m_route.Buffer ())
		m_route.Create (m_nNodes);
	route = m_route.Buffer ();
	}

if (bReverse) {
	for (int i = 0; i < h; i++) {
		route [i].nNode = nNode;
		route [i].nEdge = m_edge [nNode];
		nNode = m_pred [nNode];
		}
	}
else {
	for (int i = h - 1; i >= 0; i--) {
		route [i].nNode = nNode;
		route [i].nEdge = m_edge [nNode];
		nNode = m_pred [nNode];
		}
	}
return h;
}

//-----------------------------------------------------------------------------

