#ifndef DG_DGNODE_H_
#define DG_DGNODE_H_

#include "Support/SystemHeaders.h"


template <class T, class SubT>
class DGEdge;

template <class T>
class DGNode {
public:
  DGNode(int32_t id, T *node);

  T *getT(void) const;

  using nodes_iterator = typename std::vector<DGNode<T> *>::iterator;
  using edges_iterator = typename std::unordered_set<DGEdge<T, T> *>::iterator;
  using edges_const_iterator =
      typename std::unordered_set<DGEdge<T, T> *>::const_iterator;

  edges_iterator begin_outgoing_edges() {
    return outgoingEdges.begin();
  }
  edges_iterator end_outgoing_edges() {
    return outgoingEdges.end();
  }
  edges_const_iterator begin_outgoing_edges() const {
    return outgoingEdges.begin();
  }
  edges_const_iterator end_outgoing_edges() const {
    return outgoingEdges.end();
  }

  edges_iterator begin_incoming_edges() {
    return incomingEdges.begin();
  }
  edges_iterator end_incoming_edges() {
    return incomingEdges.end();
  }
  edges_const_iterator begin_incoming_edges() const {
    return incomingEdges.begin();
  }
  edges_const_iterator end_incoming_edges() const {
    return incomingEdges.end();
  }

  std::unordered_set<DGEdge<T, T> *> getAllEdges(void);

  iterator_range<edges_iterator> getOutgoingEdges(void) {
    return make_range(outgoingEdges.begin(), outgoingEdges.end());
  }

  iterator_range<edges_iterator> getIncomingEdges(void) {
    return make_range(incomingEdges.begin(), incomingEdges.end());
  }

  uint64_t degree(void) const;

  uint64_t outDegree(void) const;

  uint64_t inDegree(void) const;

  void addIncomingEdge(DGEdge<T, T> *edge);

  void addOutgoingEdge(DGEdge<T, T> *edge);

  void removeConnectedEdge(DGEdge<T, T> *edge);

  void removeConnectedNode(DGNode<T> *node);

  std::string toString(void) const;

  raw_ostream &print(raw_ostream &stream);

protected:
  int32_t ID;
  T *theT;
  std::unordered_set<DGEdge<T, T> *> outgoingEdges;
  std::unordered_set<DGEdge<T, T> *> incomingEdges;
};

template <class T>
DGNode<T>::DGNode(int32_t id, T *node) : ID{ id },
                                         theT(node) {
  return;
}

template <class T>
T *DGNode<T>::getT(void) const {
  return theT;
}

template <class T>
raw_ostream &DGNode<T>::print(raw_ostream &stream) {
  theT->print(stream);
  return stream;
}

template <class T>
void DGNode<T>::addIncomingEdge(DGEdge<T, T> *edge) {
  this->incomingEdges.insert(edge);
}

template <class T>
void DGNode<T>::addOutgoingEdge(DGEdge<T, T> *edge) {
  this->outgoingEdges.insert(edge);
}

template <class T>
void DGNode<T>::removeConnectedEdge(DGEdge<T, T> *edge) {
  if (outgoingEdges.find(edge) != outgoingEdges.end()) {
    outgoingEdges.erase(edge);
  } else {
    incomingEdges.erase(edge);
  }

  return;
}

template <class T>
void DGNode<T>::removeConnectedNode(DGNode<T> *node) {
  std::unordered_set<DGEdge<T, T> *> outgoingEdgesToRemove{};
  for (auto edge : outgoingEdges) {
    if (edge->getDstNode() == node) {
      outgoingEdgesToRemove.insert(edge);
    }
  }
  for (auto edge : outgoingEdgesToRemove) {
    outgoingEdges.erase(edge);
  }

  std::unordered_set<DGEdge<T, T> *> incomingEdgesToRemove{};
  for (auto edge : incomingEdges) {
    if (edge->getSrcNode() == node) {
      incomingEdgesToRemove.insert(edge);
    }
  }
  for (auto edge : incomingEdgesToRemove) {
    incomingEdges.erase(edge);
  }
}

template <class T>
std::unordered_set<DGEdge<T, T> *> DGNode<T>::getAllEdges(void) {
  std::unordered_set<DGEdge<T, T> *> allConnectedEdges{ outgoingEdges.begin(),
                                                        outgoingEdges.end() };
  allConnectedEdges.insert(incomingEdges.begin(), incomingEdges.end());
  return allConnectedEdges;
}

template <class T>
uint64_t DGNode<T>::degree(void) const {
  return outgoingEdges.size() + incomingEdges.size();
}

template <class T>
uint64_t DGNode<T>::outDegree(void) const {
  return outgoingEdges.size();
}

template <class T>
uint64_t DGNode<T>::inDegree(void) const {
  return incomingEdges.size();
}

#endif // DG_DGNODE_H_
