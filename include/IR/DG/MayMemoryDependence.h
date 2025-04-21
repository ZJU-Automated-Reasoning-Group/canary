#ifndef DG_MAY_MEMORY_DEPENDENCE_H_
#define DG_MAY_MEMORY_DEPENDENCE_H_

#include "IR/DG/MemoryDependence.h"


template <class T, class SubT>
class MayMemoryDependence : public MemoryDependence<T, SubT> {
public:
  MayMemoryDependence(DGNode<T> *src, DGNode<T> *dst, DataDependenceType t);

  MayMemoryDependence(const MayMemoryDependence<T, SubT> &edgeToCopy);

  MayMemoryDependence() = delete;

  std::string toString(void) override;

  static bool classof(const DGEdge<T, SubT> *s);
};

template <class T, class SubT>
MayMemoryDependence<T, SubT>::MayMemoryDependence(DGNode<T> *src,
                                                  DGNode<T> *dst,
                                                  DataDependenceType t)
  : MemoryDependence<T, SubT>(
      DGEdge<T, SubT>::DependenceKind::MAY_MEMORY_DEPENDENCE,
      src,
      dst,
      t) {
  return;
}

template <class T, class SubT>
MayMemoryDependence<T, SubT>::MayMemoryDependence(
    const MayMemoryDependence<T, SubT> &edgeToCopy)
  : MemoryDependence<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
std::string MayMemoryDependence<T, SubT>::toString(void) {
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
  ros << " (may) from memory\n";
  ros.flush();
  return edgeStr;
}

template <class T, class SubT>
bool MayMemoryDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  auto sKind = s->getKind();
  return (sKind == DGEdge<T, SubT>::DependenceKind::MAY_MEMORY_DEPENDENCE);
}

#endif // DG_MAY_MEMORY_DEPENDENCE_H_
