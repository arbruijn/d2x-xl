// DialHeap.cpp
// Heap management for Dijkstra Address Calculation Sort shortest path determination

#include "dialheap.h"

//-----------------------------------------------------------------------------

bool CDialHeap::Create (short nNodes)
{
if (!(m_index.Create (65536) && m_cost.Create (nNodes) && m_links.Create (nNodes) && m_pred.Create (nNodes)))
	return false;
Reset ();
return true;
}

//-----------------------------------------------------------------------------

void CDialHeap::Reset (void)
{
m_index.Clear (0xFF);
//m_links.Clear (0xFF);
m_cost.Clear (0xFF);
m_nIndex = 0;
}

//-----------------------------------------------------------------------------

bool CDialHeap::Push (short nNode, short nPredNode, ushort nNewCost)
{
	ushort nOldCost = m_cost [nNode];

if (nNewCost >= nOldCost)
	return false;

	ushort nIndex = nNewCost & 0xFFFF;

if (nOldCost < 0xFFFF) {	// node already in heap with higher m_cost, so unlink
	short h = nOldCost & 0xFFFF;
	short j = -1;
	for (short i = m_index [h]; i >= 0; i = m_links [i]) {
		if (i == nNode) {
			if (j < 0)
				m_index [h] = m_links [i];
			else
				m_links [j] = m_links [i];
			break;
			}
		}
	}
m_links [nNode] = m_index [nIndex];
m_index [nIndex] = nNode;
m_cost [nNode] = nNewCost;
m_pred [nNode] = nPredNode;
return true;
}

//-----------------------------------------------------------------------------

short CDialHeap::Pop (ushort& nCost)
{
	short	nNode;

for (int i = 65536; i; i--) {
	if (0 <= (nNode = m_index [m_nIndex])) {
		m_index [m_nIndex] = m_links [nNode];
		nCost = m_cost [nNode];
		m_cost [nNode] = 0;
		return nNode;
		}
	}
return -1;
}

//-----------------------------------------------------------------------------

short CDialHeap::Route (short* route, short nNode)
{
	short	h = nNode, i = 0;

while (0 >= (h = m_pred [h]))
	i++;
h = i + 1;
while (i >= 0) {
	route [i--] = nNode;
	nNode = m_pred [nNode];
	}
return h;
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

