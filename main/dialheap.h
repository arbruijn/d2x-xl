// DialHeap.h
// Heap management for Dijkstra Address Calculation Sort shortest path determination

#ifndef DIALHEAP_H
#define DIALHEAP_H

#include "carray.h"

class CDialHeap {
	struct tHeapIndex {
		short		nNode;
		ushort	nCost;
	} tHeapIndex;

	private:
		CShortArray		m_index;
		CUShortArray	m_cost;
		CShortArray		m_links;
		CShortArray		m_pred;
		CShortArray		m_route;
		short				m_nNodes;
		ushort			m_nIndex;

	public:
		bool Create (short nNodes);
		void Destroy (void);
		void Reset (void);
		void Setup (short nNode);
		bool Push (short nNode, short nPredNode, ushort nCost);
		short Pop (ushort &nCost);
		short RouteLength (short nNode);
		short BuildRoute (short nNode, int bReverse = 0, short *route = NULL);
		inline ushort Cost (short nNode) { return m_cost [nNode]; }
		inline bool Pushed (short nNode) { return Cost (nNode) < 0xFFFF; }
		inline bool Popped (short nNode) { return Cost (nNode) == 0; }
		inline short* Route (void) { return m_route.Buffer (); }
};


#define BIDIRECTIONAL_DACS 0

# if BIDIRECTIONAL_DACS
extern CDialHeap dialHeaps [2];
#else
extern CDialHeap dialHeap;
#endif

#endif //DIALHEAP_H
