// DialHeap.h
// Heap management for Dijkstra Address Calculation Sort shortest path determination

#ifndef DIALHEAP_H
#define DIALHEAP_H

#include "cstack.h"

class CDialHeap {
	public:
		typedef struct tPathNode {
			short		nNode;
			short		nEdge;
		} tPathNode;

	private:
		CShortArray			m_index;
		CUIntArray			m_cost;
		CStack<ushort>		m_dirtyIndex;
		CStack<uint>		m_dirtyCost;
		CShortArray			m_links;
		CShortArray			m_pred;
		CShortArray			m_edge;
		CArray<tPathNode>	m_route;
		short					m_nNodes;
		ushort				m_nIndex;

	public:
		bool Create (short nNodes);
		void Destroy (void);
		void Reset (void);
		void Setup (short nNode);
		bool Push (short nNode, short nPredNode, short nSide, uint nCost);
		short Pop (uint &nCost);
		short RouteLength (short nNode);
		short BuildRoute (short nNode, int bReverse = 0, tPathNode* route = NULL);
		inline uint Cost (short nNode) { return m_cost [nNode]; }
		inline bool Pushed (short nNode) { return Cost (nNode) < 0xFFFFFFFF; }
		inline bool Popped (short nNode) { return !Pushed (nNode) && ((Cost (nNode) & 0x80000000) != 0); }
		inline tPathNode* Route (uint i = 0) { return m_route.Buffer (i); }

	private:
		int Scan (short* buffer, int nStart, int nLength);
};


#define BIDIRECTIONAL_DACS 0

# if BIDIRECTIONAL_DACS
extern CDialHeap dialHeaps [2];
#else
extern CDialHeap dialHeap;
#endif

#endif //DIALHEAP_H
