
#ifndef DG_MUST_MEMORY_DEPENDENCE_H_
#define DG_MUST_MEMORY_DEPENDENCE_H_

#include "IR/DG/MemoryDependence.h"


template <class T, class SubT>
class MustMemoryDependence : public MemoryDependence<T, SubT> {
public:
  MustMemoryDependence(DGNode<T> *src, DGNode<T> *dst, DataDependenceType t);

  MustMemoryDependence(const MustMemoryDependence<T, SubT> &edgeToCopy);

  MustMemoryDependence() = delete;

  std::string toString(void) override;

  static bool classof(const DGEdge<T, SubT> *s);
};

template <class T, class SubT>
MustMemoryDependence<T, SubT>::MustMemoryDependence(DGNode<T> *src,
                                                    DGNode<T> *dst,
                                                    DataDependenceType t)
  : MemoryDependence<T, SubT>(
      DGEdge<T, SubT>::DependenceKind::MUST_MEMORY_DEPENDENCE,
      src,
      dst,
      t) {
  return;
}

template <class T, class SubT>
MustMemoryDependence<T, SubT>::MustMemoryDependence(
    const MustMemoryDependence<T, SubT> &edgeToCopy)
  : MemoryDependence<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
std::string MustMemoryDependence<T, SubT>::toString(void) {
  if (this->getNumberOfSubEdges() > 0) {
    std::string edgesStr;
    raw_string_ostream ros(edgesStr);
    for (auto edge : this->getSubEdges()) {
      ros << edge->toString();
    }
    return ros.str();
  }
  std::string edgeStr;
  raw_string_ostream ros(edgeStr);
  ros << "Attributes: ";
  if (this->isLoopCarriedDependence()) {
    ros << "Loop-carried ";
  }
  ros << "Data ";
  ros << this->dataDepToString();
  ros << " (must) from memory\n";
  ros.flush();
  return edgeStr;
}

template <class T, class SubT>
bool MustMemoryDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  auto sKind = s->getKind();
  return (sKind == DGEdge<T, SubT>::DependenceKind::MUST_MEMORY_DEPENDENCE);
}

#endif
