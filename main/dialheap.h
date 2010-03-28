// DialHeap.h
// Heap management for Dijkstra Address Calculation Sort shortest path determination

#ifndef DIALHEAP_H
#define DIALHEAP_H

#include "carray.h"

class CDialHeap {
	private:
		CShortArray		m_index;
		CUShortArray	m_cost;
		CShortArray		m_links;
		CShortArray		m_pred;
		ushort			m_nIndex;

	public:
		bool Create (short nNodes);
		bool Destroy (void);
		void Reset (void);
		bool Push (short nNode, short nPredNode, ushort nCost);
		short Pop (ushort &nCost);
		short Route (short* route, short nNode);
		inline ushort Cost (short nNode) { return m_cost [nNode]; }
		inline bool Pushed (short nNode) { return Cost (nNode) < 0xFFFF; }
		inline bool Popped (short nNode) { return Cost (nNode) == 0; }
};

#endif //DIALHEAP_H
